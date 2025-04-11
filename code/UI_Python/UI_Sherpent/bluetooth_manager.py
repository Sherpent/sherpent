from PyQt5 import QtCore
from bleak import BleakClient
import struct

class BluetoothManager(QtCore.QThread):
    """Gestion du bluetooth en arrière-plan"""

    def __init__(self, sherpent, address, parent=None):
        super().__init__(parent)
        """Initialise le client Bluetooth avec l'adresse du périphérique."""
        self.address = address
        self.client = BleakClient(address)

        self.sherpent = sherpent
        self.previous_command = None

    async def connect(self, characteristic_uuid):
        """Établit la connexion avec le périphérique Bluetooth."""
        try:
            await self.client.connect()
            if self.client.is_connected:
                # Log as master
                print(f"Connecté à {self.address}")
                data = bytearray([0x02, 0x02])
                await self.client.write_gatt_char(characteristic_uuid, data, response=False)

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

                print(data)

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

    async def start_notifications(self, characteristic_uuid, callback):
        if self.client.is_connected:
            try:
                await self.client.start_notify(characteristic_uuid, callback)
                print("🔔 Notifications activées.")
            except Exception as e:
                print(f"❌ Erreur de notification : {e}")

    async def stop_notifications(self, characteristic_uuid):
        if self.client.is_connected:
            try:
                await self.client.stop_notify(characteristic_uuid)
                print("🔕 Notifications désactivées.")
            except Exception as e:
                print(f"❌ Erreur stop_notify : {e}")

    async def send_joystick_vectors(self, characteristic_uuid, check: bool = True):
        if self.client.is_connected:
            # Accès à self.sherpent pour lire les vecteurs
            x = self.sherpent.vecteur_x
            y = self.sherpent.vecteur_y
            z = self.sherpent.side_winding
            w = self.sherpent.etat_bouche
            print(f"Envoie -> X : {x}, Y : {y}, Z : {z}, W : {w}")

            data = struct.pack("BBffff", 20, 7, x, y, z,w)
            try:
                if not check or data != self.previous_command:
                    await self.client.write_gatt_char(characteristic_uuid, data)
                #print(f"📤 Envoyé joystick : x={x}, y={y} → {data.hex()}")
                self.previous_command = data
            except Exception as e:
                print(f"❌ Erreur envoi joystick : {e}")

