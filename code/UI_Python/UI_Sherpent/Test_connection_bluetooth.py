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
    #msg_ID = data[1]
    #print(msg_ID)
    msg_size, msg_ID, segment_ID = struct.unpack("BBB", data[:3])
    if msg_ID == 10:
        valeur = struct.unpack("B", data[3:4])[0]
        print(f"Valeur : {valeur}")
    elif msg_ID == 9:
        valeur = struct.unpack("b", data[3:4])[0]
        print(f"Angle #{segment_ID} : {valeur}")
    elif msg_ID == 8:
        valeur = struct.unpack("b", data[3:4])[0]
        print(f"Angle #{segment_ID} : {valeur}")


    #value = float.fromhex(data.hex())  # Conversion en float
    #print(f"Notification reçue : {value}")

async def listen_notifications():
    async with BleakClient(MAC_ADDRESS) as client:
        await client.start_notify(UUID_WRITE_CHARACTERISTIC, notification_handler)
        await asyncio.sleep(20)  # Écouter 60 sec
        await client.stop_notify(UUID_WRITE_CHARACTERISTIC)


#"""

import asyncio
asyncio.run(master_connect())
asyncio.run(send_ble())
#asyncio.run(listen_notifications())

