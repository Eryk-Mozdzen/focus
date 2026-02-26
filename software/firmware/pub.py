import paho.mqtt.client as mqtt
import time
import json
import math

client = mqtt.Client()
client.connect("localhost", 1883, 60)
client.loop_start()

try:
    t0 = time.time()
    while True:
        t = time.time() - t0
        val = math.sin(2 * math.pi * 0.15 * t)
        payload = json.dumps({"val": val})

        client.publish("test/rapid", payload)
        time.sleep(0.01)

except KeyboardInterrupt:
    print("Stopping publisher...")
    client.loop_stop()
    client.disconnect()
