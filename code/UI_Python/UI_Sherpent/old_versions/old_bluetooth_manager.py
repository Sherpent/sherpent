from PyQt5 import QtCore
import time
import random
from bleak import BleakClient
import struct
import asyncio

from bleak.exc import BleakDeviceNotFoundError


class BluetoothManager(QtCore.QThread):
    """Gestion du bluetooth en arrière-plan"""


    def __init__(self, sherpent, parent=None):
        super().__init__(parent)

        self.sherpent = sherpent
        self.running = True

        self.MAC_ADDRESS = "9C:9E:6E:8D:F7:EA"
        self.UUID_WRITE_CHARACTERISTIC = "0000ff01-0000-1000-8000-00805f9b34fb"

        try :
            # Connect as a master
            asyncio.run(self.master_connect())

        except BleakDeviceNotFoundError:
            print("---> Sherpent non disponible")
            self.running = False

    def run(self):
        while self.running:
            if self.sherpent.demarrage:

                if not self.sherpent.simulation_activated:
                    # Sherpent envoie des données
                    self.send_vector()
                    #self.receive_data()

            time.sleep(1)

    def stop(self):
        """Arrête la boucle."""
        self.running = False

    def send_vector(self):
        #print("Send vector")
        vector_x = self.sherpent.vecteur_x
        vector_y = self.sherpent.vecteur_y*-1

        asyncio.run(self.send_ble(vector_x,vector_y))
        print(f"Vecteur : x={vector_x} y={vector_y}")

    def receive_data(self):

        list_module = self.sherpent.get_modules()

        for i in range(len(list_module)):
            valeurs_servo1 = random.randint(0, 180)
            valeurs_servo2 = random.randint(0, 180)

            list_module[i].set_angle(1,valeurs_servo1)
            list_module[i].set_angle(2, valeurs_servo2)

        #print("received")

    async def master_connect(self):
        async with BleakClient(self.MAC_ADDRESS) as client:
            data = bytearray([0x02, 0x02])
            await client.write_gatt_char(self.UUID_WRITE_CHARACTERISTIC, data, response=False)

    async def send_ble(self, vecteur_x, vecteur_y):
       async with BleakClient(self.MAC_ADDRESS) as client:
           data = struct.pack("BBff",12,7, vecteur_x, vecteur_y)
           await client.write_gatt_char(self.UUID_WRITE_CHARACTERISTIC, data, response = False)

    async def listen_notifications(self):
        async with BleakClient(self.MAC_ADDRESS) as client:
            await client.start_notify(self.UUID_WRITE_CHARACTERISTIC, self.notification_handler)
            #await asyncio.sleep(20)  # Écouter 60 sec
            if not self.running:
                await client.stop_notify(self.UUID_WRITE_CHARACTERISTIC)

    def notification_handler(self, data):
        # msg_ID = data[1]
        # print(msg_ID)
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