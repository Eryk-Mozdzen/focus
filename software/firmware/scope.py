# /// script
# dependencies = [
#     "paho-mqtt",
#     "msgpack",
#     "matplotlib",
# ]
# ///

import paho.mqtt.client as mqtt
import msgpack
import matplotlib.pyplot as plt

series = {
    "time": [],
    "signal_1": [],
    "signal_2": [],
    "signal_3": [],
}

plt.ion()
fig = plt.figure()
ax = fig.add_subplot(111)
line1, = ax.plot([], [], label="signal 1", linewidth=1)
line2, = ax.plot([], [], label="signal 2", linewidth=1)
line3, = ax.plot([], [], label="signal 3", linewidth=1)
ax.legend()
ax.grid()

def on_message(client, userdata, msg):
    try:
        data = msgpack.unpackb(msg.payload, raw=False)
        index = int(data["index"])
        print(index)
        if index == 0:
            line1.set_xdata(series["time"])
            line2.set_xdata(series["time"])
            line3.set_xdata(series["time"])
            line1.set_ydata(series["signal_1"])
            line2.set_ydata(series["signal_2"])
            line3.set_ydata(series["signal_3"])
            ax.relim()
            ax.autoscale_view()
            fig.canvas.draw()
            fig.canvas.flush_events()

            series["time"] = []
            series["signal_1"] = []
            series["signal_2"] = []
            series["signal_3"] = []
        series["time"].extend([0.0001 * (data["index"] + i) for i in range(10)])
        series["signal_1"].extend(data["signal_1"])
        series["signal_2"].extend(data["signal_2"])
        series["signal_3"].extend(data["signal_3"])
    except Exception as e:
        print(f"invalid message: {msg.payload} ({e})")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.subscribe("focus/scope")

try:
    client.loop_forever()
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()
