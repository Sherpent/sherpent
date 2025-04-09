import sys
import math
import time
from PyQt5 import QtCore
import asyncio

from PyQt5.QtWidgets import QApplication, QMainWindow, QSlider, QLabel, QButtonGroup
from PyQt5.QtCore import Qt
from UI_SherpentV4 import Ui_MainWindow  # Import de l'interface générée par Qt Designer
from PyQt5.QtGui import QPainter, QPen, QBrush, QPolygonF
from PyQt5.QtCore import QPointF
from PyQt5.QtWidgets import QWidget
from PyQt5 import QtWidgets

from sherpent_class import Sherpent
from module_class import Modules
from controller_manager import ControllerManager
from bluetooth_manager import BluetoothManager

class ModulesSherpentWidget(QWidget):
    """ Classe pour dessiner le serpent articulé """
    def __init__(self, parent = None, sherpent= None):
        super().__init__(parent)
        self.sherpent = sherpent

    def paintEvent(self, event):
        """ Dessine le serpent articulé """
        qp = QPainter(self)
        qp.setRenderHint(QPainter.Antialiasing)
        qp.setPen(QPen(Qt.black, 2))
        qp.setBrush(QBrush(Qt.gray))
        angle_acc = 0

        list_modules = self.sherpent.get_modules()
        nbr_modules = self.sherpent.get_nbr_modules()
        for i in range(0, nbr_modules):
            module = list_modules[i]

            x, y = module.get_front_position()
            width = module.module_width
            length = module.module_length

            angle_acc += module.get_angle(1) # Angle en degré
            angle_rad = math.radians(angle_acc)

            x_new = x - length * math.cos(angle_rad)
            y_new = y + length * math.sin(angle_rad)

            dx = width / 2 * math.sin(angle_rad)
            dy = -width / 2 * math.cos(angle_rad)

            points = QPolygonF([
                QPointF(x - dx, y + dy),
                QPointF(x + dx, y - dy),
                QPointF(x_new + dx, y_new - dy),
                QPointF(x_new - dx, y_new + dy)
            ])

            qp.drawPolygon(points)

            if i < nbr_modules-1:
                list_modules[i+1].set_front_position(x_new, y_new)

    def update_modules(self):
        self.update()

class SherpentControl(QMainWindow):
    """ Interface principale utilisant l'UI de Qt Designer """
    def __init__(self):
        super().__init__()

        self.controller_is_connected = False


        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        self.sherpent = Sherpent()

        self.module_widget = ModulesSherpentWidget(self.ui.widgetAffichageSherpent, self.sherpent)

        self.button_group = QButtonGroup()
        self.init_ui()

        # Lancement du thread du controller
        self.controller = ControllerManager(self.sherpent)
        self.controller.controller_connected.connect(self.update_controller_status)
        QtCore.QTimer.singleShot(100, self.controller.check_controller_status)

        self.controller.start()

        #self.bluetooth = BluetoothManager(self.sherpent)
        #self.bluetooth.start()

        self.address = "9C:9E:6E:8D:F7:EA"  # Adresse du périphérique Bluetooth
        self.characteristic_uuid = "0000ff01-0000-1000-8000-00805f9b34fb"  # UUID de la caractéristique
        self.bt_manager = BluetoothManager(self.sherpent, self.address)


        self.update_nbr_modules(5)
        self.show()

        # Timer pour update l'affichage du sherpent
        self.update_timer = QtCore.QTimer(self)
        self.update_timer.timeout.connect(self.update_value_module)
        self.update_timer.start(100)

    def init_ui(self):
        """ Connection entre les boutons du UI et leurs actions """
        # Ajout d'un layout pour mettre le widget du sherpent
        layout = QtWidgets.QVBoxLayout(self.ui.widgetAffichageSherpent)
        layout.addWidget(self.module_widget)
        self.ui.widgetAffichageSherpent.setLayout(layout)

        self.ui.simBtn.toggled.connect(self.btnSim_toggled)

        self.button_group.setExclusive(True)
        self.button_group.addButton(self.ui.startBtn)
        self.button_group.addButton(self.ui.stopBtn)
        self.button_group.buttonClicked.connect(self.start_stop_btn_toggled)

    def btnSim_toggled(self, etat):
        if etat:
            print("On simulation")
            self.sherpent.simulation_activated = True
        else :
            self.sherpent.simulation_activated = False

    def start_stop_btn_toggled(self, bouton):

        if bouton.text() == "Start":
            print("Btn start")
            self.sherpent.demarrage = True

        elif bouton.text() == "Stop":
            print("Btn stop")
            self.sherpent.demarrage = False

    def update_nbr_modules(self, nbr_module):

        for i in range(nbr_module):
            module = self.sherpent.add_module()
            module.label_servo1 = getattr(self.ui, f"valeur_servo1_{i}_label", None)
            module.label_servo2 = getattr(self.ui, f"valeur_servo2_{i}_label", None)
            module.label_charge = getattr(self.ui, f"valeur_charge{i}_label", None)

            #print(f"Module {i} - Servo1 Label: {module.label_servo1}")  # Debug

        print(f"Nbr de modules = {self.sherpent.get_nbr_modules()}")

        module0 = self.sherpent.get_modules()[0]
        module0.set_front_position(300,50)
        module0.set_angle(1,90)

    def update_value_module(self):
        list_modules = self.sherpent.get_modules()
        for i in range(self.sherpent.get_nbr_modules()):
            servo1 = list_modules[i].get_angle(1)
            servo2 = list_modules[i].get_angle(2)
            charge = list_modules[i].get_charge()

            list_modules[i].label_servo1.setText(f"{servo1} °")
            list_modules[i].label_servo2.setText(f"{servo2} °")
            list_modules[i].label_charge.setText(f"{charge} °")

            # On enlève tous les borders des widgets
            module_widget = getattr(self.ui, f"module{i}_widget", None)
            module_widget.setStyleSheet(f"""
                QWidget#module{i}_widget {{
                    border: 1px solid black;
                    border-radius: 5px;
                }}
            """)

        # On met un border sur le widget actif
        index = self.controller.index_module
        module_ON = getattr(self.ui, f"module{index}_widget", None)
        module_ON.setStyleSheet(f"""
                QWidget#module{index}_widget {{
                    border: 3px solid black;
                    border-radius: 5px;
                }}
            """)

        self.module_widget.update_modules()

    def update_controller_status(self, connected):
        if connected:
            print("---> Signal recu : Manette connecté")
            self.controller_is_connected = True
        else:
            print("---> Signal recu : Manette pas connecté")
            self.controller_is_connected = False

    def closeEvent(self, event):
        self.controller.stop()
        #self.bluetooth.stop()
        self.controller.wait()
        #self.bluetooth.wait()
        event.accept()

    async def run(self):
        """Gère la connexion Bluetooth et les interactions."""
        if await self.bt_manager.connect():
            await self.bt_manager.read_data(self.characteristic_uuid)
            #await self.bt_manager.write_data(self.characteristic_uuid, b'\x01')
            #await self.bt_manager.disconnect()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SherpentControl()
    window.show()
    asyncio.run(window.run())
    sys.exit(app.exec_())
