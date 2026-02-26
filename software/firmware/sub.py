import paho.mqtt.client as mqtt
import json

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8")
        data = json.loads(payload)
        print(data["val"])
    except Exception as e:
        print(f"Error parsing message: {msg.payload} ({e})")

client = mqtt.Client()
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.subscribe("test/topic")

client.loop_forever()
