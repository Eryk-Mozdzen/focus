# /// script
# dependencies = [
#     "paho-mqtt",
#     "msgpack",
#     "pyjson",
# ]
# ///

import paho.mqtt.client as mqtt
import msgpack
import json

def on_message(client, userdata, msg):
    try:
        data = msgpack.unpackb(msg.payload, raw=False)
        print(json.dumps(data, indent=4))
    except Exception as e:
        print(f"invalid message: {msg.payload} ({e})")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.subscribe("focus/state")

try:
    client.loop_forever()
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()
