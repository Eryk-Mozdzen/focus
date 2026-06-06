#!/usr/bin/env -S uv run

# /// script
# dependencies = [
#     "paho-mqtt",
#     "msgpack",
#     "matplotlib",
# ]
# ///

import threading
import paho.mqtt.client as mqtt
import msgpack
import matplotlib.pyplot as plt

class Plotter:
    def __init__(self):
        self.data_accumulator = []
        self.data_display = []
        self.data_display_lock = threading.Lock()

        plt.ion()
        self.fig, self.ax = plt.subplots()

        self.line1, = self.ax.plot([], [], label="channel 1", linewidth=1)
        self.line2, = self.ax.plot([], [], label="channel 2", linewidth=1)
        self.line3, = self.ax.plot([], [], label="channel 3", linewidth=1)

        self.scatter1 = self.ax.scatter([], [], s=5, color=self.line1.get_color())
        self.scatter2 = self.ax.scatter([], [], s=5, color=self.line2.get_color())
        self.scatter3 = self.ax.scatter([], [], s=5, color=self.line3.get_color())

        self.ax.legend()
        self.ax.grid()

        self.timer = self.fig.canvas.new_timer(interval=100)
        self.timer.add_callback(self.on_update)

        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_message = self.on_message
        self.client.connect("localhost", 1883, 60)
        self.client.subscribe("focus/scope")

    def on_message(self, client, userdata, message):
        message = msgpack.unpackb(message.payload)

        if message["count"] == 0:
            with self.data_display_lock:
                self.data_display = self.data_accumulator.copy()
            self.data_accumulator.clear()

        for i in range(len(message["ch1"])):
            self.data_accumulator.append({
                "timestamp": message["dt"] * (message["count"] + i),
                "channel_1": message["ch1"][i],
                "channel_2": message["ch2"][i],
                "channel_3": message["ch3"][i],
            })

    def on_update(self):
        with self.data_display_lock:
            timestamp = [item["timestamp"] for item in self.data_display]
            channel_1 = [item["channel_1"] for item in self.data_display]
            channel_2 = [item["channel_2"] for item in self.data_display]
            channel_3 = [item["channel_3"] for item in self.data_display]

        if len(timestamp) > 0:
            self.line1.set_data(timestamp, channel_1)
            self.line2.set_data(timestamp, channel_2)
            self.line3.set_data(timestamp, channel_3)

            self.scatter1.set_offsets(list(zip(timestamp, channel_1)))
            self.scatter2.set_offsets(list(zip(timestamp, channel_2)))
            self.scatter3.set_offsets(list(zip(timestamp, channel_3)))

        self.ax.relim()
        self.ax.autoscale_view()

        self.fig.canvas.draw_idle()

    def start(self):
        self.timer.start()
        self.client.loop_start()

        plt.show(block=True)

        self.client.loop_stop()
        self.client.disconnect()
        self.timer.stop()

if __name__ == "__main__":
    plotter = Plotter()
    plotter.start()
