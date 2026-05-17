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

data_accumulator = []
data_plot = []
lock = threading.Lock()

plt.ion()
fig, ax = plt.subplots()
line1, = ax.plot([], [], label="signal 1", linewidth=1)
line2, = ax.plot([], [], label="signal 2", linewidth=1)
line3, = ax.plot([], [], label="signal 3", linewidth=1)
scatter1 = ax.scatter([], [], s=10, color=line1.get_color())
scatter2 = ax.scatter([], [], s=10, color=line2.get_color())
scatter3 = ax.scatter([], [], s=10, color=line3.get_color())
ax.legend()
ax.grid()

def on_message(client, userdata, message):
    global data_accumulator
    global data_plot

    try:
        message = msgpack.unpackb(message.payload)

        if message["timestamp"] == 0:
            with lock:
                data_plot = data_accumulator.copy()
            data_accumulator.clear()

        for i in range(len(message["signal_1"])):
            data_accumulator.append({
                "timestamp": message["timestamp"] + (0.0001 * i),
                "signal_1": message["signal_1"][i],
                "signal_2": message["signal_2"][i],
                "signal_3": message["signal_3"][i],
            })

    except Exception as e:
        print(e)

def update_plot():
    global data_plot

    with lock:
        timestamp = [item["timestamp"] for item in data_plot]
        signal_1 = [item["signal_1"] for item in data_plot]
        signal_2 = [item["signal_2"] for item in data_plot]
        signal_3 = [item["signal_3"] for item in data_plot]

    line1.set_data(timestamp, signal_1)
    line2.set_data(timestamp, signal_2)
    line3.set_data(timestamp, signal_3)

    if timestamp:
        scatter1.set_offsets(list(zip(timestamp, signal_1)))
        scatter2.set_offsets(list(zip(timestamp, signal_2)))
        scatter3.set_offsets(list(zip(timestamp, signal_3)))

    ax.relim()
    ax.autoscale_view()

    fig.canvas.draw_idle()

timer = fig.canvas.new_timer(interval=100)
timer.add_callback(update_plot)
timer.start()

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.subscribe("focus/scope")

client.loop_start()

try:
    plt.show(block=True)
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()
