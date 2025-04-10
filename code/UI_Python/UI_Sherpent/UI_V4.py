import sys
import math
import struct
import time
from PyQt5 import QtCore
import asyncio
from qasync import QEventLoop, asyncSlot, asyncClose

from PyQt5.QtWidgets import QApplication, QMainWindow, QSlider, QLabel, QButtonGroup
from PyQt5.QtCore import Qt, QTimer
from UI_SherpentV4 import Ui_MainWindow  # Import de l'interface générée par Qt Designer
from PyQt5.QtGui import QPainter, QPen, QBrush, QPolygonF, QColor
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
        #qp.setBrush(QBrush(Qt.gray))
        angle_acc = 0

        #self.update_color_module(qp)

        list_modules = self.sherpent.get_modules()
        nbr_modules = self.sherpent.get_nbr_modules()
        for i in range(0, nbr_modules):
            module = list_modules[i]

            charge = module.get_charge()

            # Choix de la couleur en fonction de la charge
            if charge >= 75.0:
                qp.setBrush(QBrush(Qt.green))
            elif charge >= 50.0:
                qp.setBrush(QBrush(QColor("yellow")))
            elif charge >= 25.0:
                qp.setBrush(QBrush(QColor("orange")))
            else:
                qp.setBrush(QBrush(Qt.red))

            x, y = module.get_front_position()
            width = module.module_width
            length = module.module_length

            angle_acc += module.get_angle(1) # Angle en degré
            if i==0:
                angle_acc += 90
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

    def update_color_module(self, qp):
        for module in self.sherpent.get_modules():
            charge = module.get_charge()
            if charge >= 75.0:
                qp.setBrush(QBrush(Qt.green))

            elif charge < 75.0 and charge >= 50.0:
                qp.setBrush(QBrush(QColor("yellow")))

            elif charge < 50.0 and charge >= 25.0:
                qp.setBrush(QBrush(QColor("orange")))

            elif charge < 25.0:
                qp.setBrush(QBrush(Qt.red))


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
        QTimer.singleShot(100, self.controller.check_controller_status)

        self.controller.start()

        self.address = "9C:9E:6E:8D:F8:12"  # Adresse du périphérique Bluetooth
        self.characteristic_uuid = "0000ff01-0000-1000-8000-00805f9b34fb"  # UUID de la caractéristique
        self.bt_manager = BluetoothManager(self.sherpent, self.address)

        self.update_nbr_modules(10)
        self.show()

        # Timer pour update l'affichage du sherpent
        self.update_timer = QTimer(self)
        self.update_timer.timeout.connect(self.update_value_module)
        self.update_timer.start(200)

        # Lancer la connexion au démarrage
        QTimer.singleShot(0, self.defer_start_bluetooth)
        self.joystick_timer = QTimer(self)
        self.joystick_timer.timeout.connect(self.send_joystick_to_ble)
        #self.joystick_timer.start(200)

    def init_ui(self):
        """ Connection entre les boutons du UI et leurs actions """
        #self.resize(400,400)

        # Ajout d'un layout pour mettre le widget du sherpent
        layout = QtWidgets.QVBoxLayout(self.ui.widgetAffichageSherpent)
        layout.addWidget(self.module_widget)
        self.ui.widgetAffichageSherpent.setLayout(layout)

        self.ui.simBtn.toggled.connect(self.btnSim_toggled)

        self.button_group.setExclusive(True)
        self.button_group.addButton(self.ui.startBtn)
        self.button_group.addButton(self.ui.stopBtn)
        self.button_group.buttonClicked.connect(self.start_stop_btn_toggled)

        self.ui.frame.setStyleSheet("""
            QFrame#frame {
                border: 2px solid black;
                border-radius: 6px;
                background-color: white;
            }
        """)

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

    def handle_notification(self, sender, data):
        #print(f"Notif : {data}")
        msg_size, msg_ID, segment_ID = struct.unpack("BBB", data[:3])

        if msg_ID == 10:
            valeur = round(struct.unpack("B", data[3:4])[0] / 255*100, 1)
            self.sherpent.get_modules()[segment_ID].set_charge(valeur)
            #print(f"Segment ID : {segment_ID} Valeur : {valeur}")

        elif msg_ID == 9:
            valeur = struct.unpack("b", data[3:4])[0]
            self.sherpent.get_modules()[segment_ID].set_angle(2, valeur)
            #print(f"Angle #{segment_ID} : {valeur}")
        elif msg_ID == 8:
            valeur = struct.unpack("b", data[3:4])[0]
            self.sherpent.get_modules()[segment_ID].set_angle(1, valeur)
            #print(f"Angle #{segment_ID} : {valeur}")


    def defer_start_bluetooth(self):
        asyncio.create_task(self.start_bluetooth())

    async def start_bluetooth(self):
        if await self.bt_manager.connect(self.characteristic_uuid):
            await self.bt_manager.start_notifications(
                self.characteristic_uuid,
                self.handle_notification
            )
            # Start le timer
            self.joystick_timer.start(1000)

    def send_joystick_to_ble(self):
        asyncio.create_task(self.bt_manager.send_joystick_vectors(self.characteristic_uuid))

    @asyncClose
    async def closeEvent(self, event):
        print("🔚 Fermeture de la fenêtre, déconnexion Bluetooth...")
        self.update_timer.stop()
        self.joystick_timer.stop()
        await self.bt_manager.stop_notifications(self.characteristic_uuid)
        await self.bt_manager.disconnect()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)

    # Intégration asyncio <-> Qt grâce à qasync
    loop = QEventLoop(app)
    asyncio.set_event_loop(loop)

    window = SherpentControl()
    window.show()
    #asyncio.run(window.run())
    #sys.exit(app.exec_())

    with loop:
        loop.run_forever()
