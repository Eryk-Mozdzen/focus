#!/usr/bin/env -S uv run

# /// script
# dependencies = [
#     "asyncio",
#     "cobs",
#     "datetime",
# ]
# ///

import asyncio
import cobs.cobs
import struct
import datetime


class Bus:
    def __init__(self):
        self.lock = asyncio.Lock()
        self.values = {
            "focus_voltage": None,
            "focus_velocity": None,
        }
        self.subscribers = []

    def subscribe(self):
        q = asyncio.Queue()
        self.subscribers.append(q)
        return q

    async def put(self, item):
        timestamp, data = item

        async with self.lock:
            self.values.update(data)
            snapshot = self.values.copy()

        message = (timestamp, snapshot)

        for q in self.subscribers:
            await q.put(message)


class Focus:
    def __init__(self, ip):
        self.ip = ip

    async def __aenter__(self):
        self.reader2, self.writer2 = await asyncio.open_connection(self.ip, 8200)
        return self

    async def __aexit__(self, exc_type, exc, tb):
        self.writer2.close()
        await self.writer2.wait_closed()

    async def run(self, queue):
        buffer = bytearray()

        while True:
            data = await self.reader2.read(4096)
            buffer.extend(data)
            delimiter = buffer.find(0)
            if delimiter < 0:
                await asyncio.sleep(0.01)
                continue
            frame = buffer[:delimiter].copy()
            del buffer[: delimiter + 1]
            try:
                decoded = cobs.cobs.decode(frame)
            except Exception as e:
                print(e)
                continue
            if len(decoded) != 9 * 4:
                continue
            values = struct.unpack("<9f", decoded)
            await queue.put(
                (
                    datetime.datetime.now(datetime.UTC),
                    {
                        "focus_voltage": values[2],
                        "focus_velocity": values[1],
                    },
                )
            )


class Printer:
    def __init__(self):
        pass

    async def run(self, queue):
        while True:
            _, data = await queue.get()
            print(data)


async def main():
    async with Focus("192.168.8.1") as focus:
        bus = Bus()

        printer = Printer()

        tasks = [
            asyncio.create_task(focus.run(bus)),
            asyncio.create_task(printer.run(bus.subscribe())),
        ]

        await tasks[0]
        for task in tasks:
            task.cancel()


if __name__ == "__main__":
    asyncio.run(main())
