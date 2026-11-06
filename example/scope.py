#!/usr/bin/env -S uv run

# /// script
# dependencies = [
#     "cobs",
#     "matplotlib",
#     "numpy",
# ]
# ///

import struct
import threading
import socket
import time
import cobs.cobs
import numpy as np
import matplotlib.pyplot as plt

FRAME_NUM = 10
FRAME_FLOATS = 16
BATCH_SIZE = 4 + FRAME_NUM * FRAME_FLOATS * 4

AXES = [
    ("position [rad]", [("$\\theta_e$", 9, 1), ("$\\theta_m$", 10, 1)]),
    ("current [A]", [("$I_u$", 1, 1), ("$I_v$", 2, 1), ("$I_w$", 3, 1)]),
    ("current [A]", [("$I_d$", 4, 1), ("$I_q$", 5, 1), ("$I_{qsp}$", 6, 1)]),
    ("voltage [V]", [("$U_d$", 7, 1), ("$U_q$", 8, 1)]),
    (
        "duty cycle [%]",
        [("$PWM_u$", 11, 100), ("$PWM_v$", 12, 100), ("$PWM_w$", 13, 100)],
    ),
]


class Client:
    def __init__(self, message_timeout=1):
        self.on_message = None
        self.sock = None
        self.host = None
        self.port = None
        self._thread = None
        self._stop_event = threading.Event()
        self.message_timeout = message_timeout
        self._last_message = None

    def connect(self, host=None, port=None):
        if host is not None:
            self.host = host
        if port is not None:
            self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(1.0)
        self.sock.connect((self.host, self.port))
        self._last_message = time.monotonic()

    def loop_start(self):
        if self._thread is not None and self._thread.is_alive():
            return
        self._stop_event.clear()
        self._thread = threading.Thread(
            target=self._loop,
            name="ClientLoop",
            daemon=True,
        )
        self._thread.start()

    def loop_stop(self):
        self._stop_event.set()
        if self.sock is not None:
            try:
                self.sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                self.sock.close()
            except OSError:
                pass
        if self._thread is not None:
            self._thread.join()
        self._thread = None
        self.sock = None

    def _reconnect(self):
        old_sock = self.sock
        self.sock = None
        if old_sock is not None:
            try:
                old_sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                old_sock.close()
            except OSError:
                pass
        if self._stop_event.is_set():
            return
        while not self._stop_event.is_set():
            try:
                self.connect()
                return
            except OSError:
                time.sleep(1)

    def _loop(self):
        buffer = bytearray()
        while not self._stop_event.is_set():
            try:
                if (
                    self._last_message is not None
                    and time.monotonic() - self._last_message >= self.message_timeout
                ):
                    self._reconnect()
                    buffer.clear()
                    continue
                data = self.sock.recv(65536)
                if not data:
                    if not self._stop_event.is_set():
                        self._reconnect()
                        buffer.clear()
                    continue
                buffer.extend(data)
                while True:
                    delimiter = buffer.find(0)
                    if delimiter < 0:
                        break
                    encoded = bytes(buffer[:delimiter])
                    del buffer[: delimiter + 1]
                    if not encoded:
                        continue
                    try:
                        decoded = cobs.cobs.decode(encoded)
                        self._last_message = time.monotonic()
                        if self.on_message:
                            self.on_message(decoded)
                    except cobs.cobs.DecodeError as e:
                        print(f"Invalid COBS frame: {e}")
            except socket.timeout:
                continue
            except OSError:
                if not self._stop_event.is_set():
                    self._reconnect()
                    buffer.clear()


class Series:
    def __init__(self, ax, label, scale=1):
        (self.line,) = ax.plot(
            [],
            [],
            label=label,
            linewidth=1,
            marker="o",
            markersize=2.2,
            markeredgewidth=0,
        )
        self.scale = scale

    def set(self, x, y):
        self.line.set_data(x, y * self.scale if self.scale != 1 else y)


class Plotter:
    def __init__(self):
        self._batches = []
        self._times = []
        self._display = None
        self._display_gen = 0
        self._display_lock = threading.Lock()
        self._drawn_gen = 0
        self._background = None
        self._cursor_dirty = False

        plt.ion()
        self.fig, self.ax = plt.subplots(nrows=len(AXES), ncols=1, sharex=True)

        self.series = []
        for ax, (ylabel, curves) in zip(self.ax, AXES):
            group = []
            for label, col, scale in curves:
                group.append((Series(ax, label, scale), col))
            self.series.append(group)
            ax.set_ylabel(ylabel)
            ax.grid()
            ax.legend(loc="upper right")

        self.ax[0].set_xlim(0, 500)

        self.cursor_lines = [
            ax.axvline(x=0, color="black", lw=1, visible=False, animated=True)
            for ax in self.ax
        ]

        self.fig.canvas.mpl_connect("motion_notify_event", self.on_mouse_move)
        self.fig.canvas.mpl_connect("draw_event", self.on_draw)

        self.timer = self.fig.canvas.new_timer(interval=100)
        self.timer.add_callback(self.on_update)

        self.client = Client()
        self.client.on_message = self.on_message
        self.client.connect("192.168.8.1", 8100)

    @staticmethod
    def parse_batch(batch):
        if len(batch) != BATCH_SIZE:
            raise ValueError(f"Invalid batch size: {len(batch)}")
        count = struct.unpack_from("<I", batch, 0)[0]
        frames = np.frombuffer(batch, dtype="<f4", offset=4).reshape(
            FRAME_NUM, FRAME_FLOATS
        )
        return count, frames

    def on_message(self, message):
        count, frames = Plotter.parse_batch(message)

        if count == 0 and self._batches:
            data = np.concatenate(self._batches)
            t = np.concatenate(self._times)
            with self._display_lock:
                self._display = (t, data)
                self._display_gen += 1
            self._batches.clear()
            self._times.clear()

        self._batches.append(frames)
        self._times.append(np.arange(count, count + FRAME_NUM))

    def on_mouse_move(self, event):
        if event.inaxes in self.ax and event.xdata is not None:
            x = event.xdata
            for line in self.cursor_lines:
                line.set_xdata([x, x])
                line.set_visible(True)
        else:
            for line in self.cursor_lines:
                line.set_visible(False)
        self._cursor_dirty = True

    def on_draw(self, event):
        self._background = self.fig.canvas.copy_from_bbox(self.fig.bbox)
        self._draw_cursor()
        self._cursor_dirty = False

    def _draw_cursor(self):
        for ax, line in zip(self.ax, self.cursor_lines):
            ax.draw_artist(line)

    def _blit_cursor(self):
        if self._background is None:
            return
        canvas = self.fig.canvas
        canvas.restore_region(self._background)
        self._draw_cursor()
        canvas.blit(self.fig.bbox)
        self._cursor_dirty = False

    def on_update(self):
        with self._display_lock:
            gen = self._display_gen
            display = self._display

        if display is not None and gen != self._drawn_gen:
            self._drawn_gen = gen
            t, data = display
            for ax, group in zip(self.ax, self.series):
                lo, hi = np.inf, -np.inf
                for s, col in group:
                    y = data[:, col]
                    s.set(t, y)
                    lo = min(lo, float(y.min()) * s.scale)
                    hi = max(hi, float(y.max()) * s.scale)
                if np.isfinite(lo) and np.isfinite(hi):
                    pad = (hi - lo) * 0.05 or 1.0
                    ax.set_ylim(lo - pad, hi + pad)
            self.fig.canvas.draw_idle()
        elif self._cursor_dirty:
            self._blit_cursor()

    def start(self):
        self.timer.start()
        self.client.loop_start()

        plt.show(block=True)

        self.client.loop_stop()
        self.timer.stop()


if __name__ == "__main__":
    plotter = Plotter()
    plotter.start()
