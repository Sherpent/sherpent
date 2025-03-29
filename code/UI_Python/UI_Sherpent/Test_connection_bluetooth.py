from bleak import BleakClient
import struct

MAC_ADDRESS = "9C:9E:6E:8D:F7:EA"
UUID_WRITE_CHARACTERISTIC = "0000ff01-0000-1000-8000-00805f9b34fb"

async def master_connect():
    async with BleakClient(MAC_ADDRESS) as client:
        data = bytearray([0x02,0x02])
        await client.write_gatt_char(UUID_WRITE_CHARACTERISTIC, data, response = False)

async def send_ble():
   async with BleakClient(MAC_ADDRESS) as client:
       data = struct.pack("BBff",12,7, 0, 0)
       await client.write_gatt_char(UUID_WRITE_CHARACTERISTIC, data, response = False)
#"""

def notification_handler(sender, data):
    value = float.fromhex(data.hex())  # Conversion en float
    print(f"Notification reçue : {value}")

async def listen_notifications():
    async with BleakClient(MAC_ADDRESS) as client:
        await client.start_notify(UUID_WRITE_CHARACTERISTIC, notification_handler)
        await asyncio.sleep(60)  # Écouter 60 sec
        await client.stop_notify(UUID_WRITE_CHARACTERISTIC)


#"""

import asyncio
asyncio.run(master_connect())
asyncio.run(send_ble())
asyncio.run(listen_notifications())

