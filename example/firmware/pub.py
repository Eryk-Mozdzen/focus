# /// script
# dependencies = [
#     "paho-mqtt",
#     "msgpack",
# ]
# ///

import paho.mqtt.client as mqtt
import time
import math
import msgpack

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect("localhost", 1883, 60)
client.loop_start()

try:
    t0 = time.time()
    while True:
        t = time.time() - t0
        val = math.sin(2 * math.pi * 0.15 * t)
        payload = msgpack.packb({"val": val}, use_bin_type=True)

        client.publish("focus/control", payload)
        time.sleep(0.01)

except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()
