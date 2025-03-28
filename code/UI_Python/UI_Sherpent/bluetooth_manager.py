from PyQt5 import QtCore
import time
import random

class BluetoothManager(QtCore.QThread):
    """Gestion du bluetooth en arrière-plan"""


    def __init__(self, sherpent, parent=None):
        super().__init__(parent)

        self.sherpent = sherpent
        self.running = True


    def run(self):
        while self.running:
            if self.sherpent.demarrage:

                if not self.sherpent.simulation_activated:
                    # Sherpent envoie des données
                    self.send_vector()
                    self.receive_data()

            time.sleep(1)

    def stop(self):
        """Arrête la boucle."""
        self.running = False

    def send_vector(self):
        #print("Send vector")
        vector_x = self.sherpent.vecteur_x
        vector_y = self.sherpent.vecteur_y*-1
        print(f"Vecteur : x={vector_x} y={vector_y}")

    def receive_data(self):

        list_module = self.sherpent.get_modules()



        for i in range(len(list_module)):
            valeurs_servo1 = random.randint(0, 180)
            valeurs_servo2 = random.randint(0, 180)

            list_module[i].set_angle(1,valeurs_servo1)
            list_module[i].set_angle(2, valeurs_servo2)

        #print("received")
