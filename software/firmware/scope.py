import paho.mqtt.client as mqtt
import msgpack
import matplotlib.pyplot as plt

series = {
    "time": [],
    "current_u": [],
    "current_v": [],
    "current_w": [],
}

plt.ion()
fig = plt.figure()
ax = fig.add_subplot(111)
line1, = ax.plot([], [], label="U", linewidth=1)
line2, = ax.plot([], [], label="V", linewidth=1)
line3, = ax.plot([], [], label="W", linewidth=1)
# line4, = ax.plot([], [], label="sum", linewidth=1)
ax.legend()
ax.grid()

def on_message(client, userdata, msg):
    try:
        data = msgpack.unpackb(msg.payload, raw=False)
        series["time"].extend([0.00002 * (data["index"] + i) for i in range(10)])
        series["current_u"].extend(data["current_u"])
        series["current_v"].extend(data["current_v"])
        series["current_w"].extend(data["current_w"])
        index = int(data["index"])
        print(index)
        if index >= 990:
            line1.set_xdata(series["time"])
            line2.set_xdata(series["time"])
            line3.set_xdata(series["time"])
            # line4.set_xdata(series["time"])
            line1.set_ydata(series["current_u"])
            line2.set_ydata(series["current_v"])
            line3.set_ydata(series["current_w"])
            # line4.set_ydata([u+v+w for u, v, w in zip(series["current_u"], series["current_v"], series["current_w"])])
            ax.relim()
            ax.autoscale_view()
            fig.canvas.draw()
            fig.canvas.flush_events()

            series["time"] = []
            series["current_u"] = []
            series["current_v"] = []
            series["current_w"] = []
    except Exception as e:
        print(f"invalid message: {msg.payload} ({e})")

client = mqtt.Client()
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.subscribe("focus/scope")

try:
    client.loop_forever()
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()
