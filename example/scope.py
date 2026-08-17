#!/usr/bin/env -S uv run

# /// script
# dependencies = [
#     "cobs",
#     "matplotlib",
# ]
# ///

import struct
import threading
import socket
import time
import cobs.cobs
import matplotlib.pyplot as plt


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
            except OSError as e:
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
                data = self.sock.recv(4096)
                if not data:
                    if not self._stop_event.is_set():
                        self._reconnect()
                        buffer.clear()
                    continue
                buffer.extend(data)
                while 0 in buffer:
                    delimiter = buffer.index(0)
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
    def __init__(self, ax, label):
        (self.line,) = ax.plot([], [], label=label, linewidth=1)
        self.scatter = ax.scatter([], [], s=5, color=self.line.get_color())

    def set(self, x, y):
        self.line.set_data(x, y)
        self.scatter.set_offsets(list(zip(x, y)))


class Plotter:
    def __init__(self):
        self.data_accumulator = {}
        self.data_display = {}
        self.data_display_lock = threading.Lock()

        plt.ion()
        self.fig, self.ax = plt.subplots(nrows=4, ncols=1, sharex=True)
        self.series_theta_e = Series(self.ax[0], "$\\theta_e$")
        self.series_theta_m = Series(self.ax[0], "$\\theta_m$")
        self.ax[0].set_ylabel("position [rad]")
        self.ax[0].grid()
        self.ax[0].legend(loc="upper right")
        self.series_current_u = Series(self.ax[1], "$I_u$")
        self.series_current_v = Series(self.ax[1], "$I_v$")
        self.series_current_w = Series(self.ax[1], "$I_w$")
        self.ax[1].set_ylabel("current [A]")
        self.ax[1].grid()
        self.ax[1].legend(loc="upper right")
        self.series_current_d = Series(self.ax[2], "$I_d$")
        self.series_current_q = Series(self.ax[2], "$I_q$")
        self.series_current_q_sp = Series(self.ax[2], "$I_{qsp}$")
        self.ax[2].set_ylabel("current [A]")
        self.ax[2].grid()
        self.ax[2].legend(loc="upper right")
        self.series_voltage_d = Series(self.ax[3], "$U_d$")
        self.series_voltage_q = Series(self.ax[3], "$U_q$")
        self.ax[3].set_ylabel("voltage [V]")
        self.ax[3].grid()
        self.ax[3].legend(loc="upper right")

        self.timer = self.fig.canvas.new_timer(interval=100)
        self.timer.add_callback(self.on_update)

        self.client = Client()
        self.client.on_message = self.on_message
        self.client.connect("192.168.8.1", 8100)

    def parse_batch(batch):
        frame_num = 10
        frame_size = 64

        if len(batch) != ((frame_size * frame_num) + 4):
            raise ValueError(f"Invalid batch size: {len(batch)}")

        count = struct.unpack_from("<I", batch, 0)[0]
        offset = 4
        frames = []
        for i in range(frame_num):
            frames.append(
                (
                    count + i,
                    {
                        "u_vbus": struct.unpack_from("<f", batch, offset + 0)[0],
                        "i_uvw": list(struct.unpack_from("<3f", batch, offset + 4)),
                        "i_dq": list(struct.unpack_from("<2f", batch, offset + 16)),
                        "i_q_sp": struct.unpack_from("<f", batch, offset + 24)[0],
                        "u_dq": list(struct.unpack_from("<2f", batch, offset + 28)),
                        "theta_em": list(struct.unpack_from("<2f", batch, offset + 36)),
                        "spare": list(struct.unpack_from("<5f", batch, offset + 44)),
                    },
                )
            )
            offset += frame_size
        return frames

    def on_message(self, message):
        frames = Plotter.parse_batch(message)

        for i, frame in frames:
            if i == 0:
                with self.data_display_lock:
                    self.data_display = self.data_accumulator.copy()
                self.data_accumulator.clear()

            if "t" not in self.data_accumulator:
                self.data_accumulator["t"] = []
            self.data_accumulator["t"].append(i)

            for key, value in frame.items():
                if isinstance(value, float):
                    if key not in self.data_accumulator:
                        self.data_accumulator[key] = []
                    self.data_accumulator[key].append(value)
                if isinstance(value, list):
                    if key not in self.data_accumulator:
                        self.data_accumulator[key] = [[] for _ in range(len(value))]
                    for k, v in enumerate(value):
                        self.data_accumulator[key][k].append(v)

    def on_update(self):
        with self.data_display_lock:
            data = self.data_display.copy()

        if "t" in data:
            self.series_theta_e.set(data["t"], data["theta_em"][0])
            self.series_theta_m.set(data["t"], data["theta_em"][1])
            self.series_current_u.set(data["t"], data["i_uvw"][0])
            self.series_current_v.set(data["t"], data["i_uvw"][1])
            self.series_current_w.set(data["t"], data["i_uvw"][2])
            self.series_current_d.set(data["t"], data["i_dq"][0])
            self.series_current_q.set(data["t"], data["i_dq"][1])
            self.series_current_q_sp.set(data["t"], data["i_q_sp"])
            self.series_voltage_d.set(data["t"], data["u_dq"][0])
            self.series_voltage_q.set(data["t"], data["u_dq"][1])

        for a in self.ax:
            a.relim()
            a.autoscale_view()
        self.fig.canvas.draw_idle()

    def start(self):
        self.timer.start()
        self.client.loop_start()

        plt.show(block=True)

        self.client.loop_stop()
        self.timer.stop()


if __name__ == "__main__":
    plotter = Plotter()
    plotter.start()
