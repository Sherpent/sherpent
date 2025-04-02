from PyQt5 import QtCore
import time
import random
from bleak import BleakClient
import struct
import asyncio

from bleak.exc import BleakDeviceNotFoundError


class BluetoothManager(QtCore.QThread):
    """Gestion du bluetooth en arrière-plan"""


    def __init__(self, sherpent, address, parent=None):
        super().__init__(parent)
        """Initialise le client Bluetooth avec l'adresse du périphérique."""
        self.address = address
        self.client = BleakClient(address)

        self.sherpent = sherpent

    async def connect(self):
        """Établit la connexion avec le périphérique Bluetooth."""
        try:
            await self.client.connect()
            if self.client.is_connected:
                print(f"Connecté à {self.address}")
            return self.client.is_connected
        except Exception as e:
            print(f"Erreur de connexion : {e}")
            return False

    async def disconnect(self):
        """Ferme la connexion Bluetooth."""
        if self.client.is_connected:
            await self.client.disconnect()
            print("Déconnecté.")

    async def read_data(self, characteristic_uuid):
        """Lit une valeur depuis une caractéristique donnée."""
        if self.client.is_connected:
            try:
                data = await self.client.read_gatt_char(characteristic_uuid)
                print(f"Données reçues : {data}")
                return data
            except Exception as e:
                print(f"Erreur de lecture : {e}")
        else:
            print("Non connecté.")
        return None

    async def write_data(self, characteristic_uuid, data):
        """Écrit des données dans une caractéristique donnée."""
        if self.client.is_connected:
            try:
                await self.client.write_gatt_char(characteristic_uuid, data)
                print(f"Données envoyées : {data}")
            except Exception as e:
                print(f"Erreur d'écriture : {e}")