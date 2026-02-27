import paho.mqtt.client as mqtt
import msgpack

def on_message(client, userdata, msg):
    try:
        data = msgpack.unpackb(msg.payload, raw=False)
        print(data["val"])
    except Exception as e:
        print(f"Error parsing message: {msg.payload} ({e})")

client = mqtt.Client()
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.subscribe("test/topic")

client.loop_forever()
