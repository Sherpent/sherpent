import sys
import math
import time
from PyQt5 import QtCore

from PyQt5.QtWidgets import QApplication, QMainWindow, QSlider, QLabel
from PyQt5.QtCore import Qt
from UI_SherpentV2 import Ui_MainWindow  # Import de l'interface générée par Qt Designer
from PyQt5.QtGui import QPainter, QPen, QBrush, QPolygonF
from PyQt5.QtCore import QPointF
from PyQt5.QtWidgets import QWidget

from sherpent_class import Sherpent
from module_class import Modules
from controller_manager import ControllerManager

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
        self.module_widget.setGeometry(0, 0, self.ui.widgetAffichageSherpent.width(),
                                         self.ui.widgetAffichageSherpent.height())

        # Lancement du thread du controller
        self.controller = ControllerManager(self.sherpent)
        self.controller.controller_connected.connect(self.update_controller_status)
        QtCore.QTimer.singleShot(100, self.controller.check_controller_status)

        self.controller.start()

        self.init_ui()
        self.show()

        # Timer pour update l'affichage du sherpent
        self.update_timer = QtCore.QTimer(self)
        self.update_timer.timeout.connect(self.module_widget.update_modules)
        self.update_timer.start(100)

    def init_ui(self):
        """ Connection entre les boutons du UI et leurs actions """
        self.ui.nbrModuleSpinBox.valueChanged.connect(self.update_nbr_modules)
        self.ui.xPosSpinBox.valueChanged.connect(self.update_positions_modules)
        self.ui.yPosSpinBox.valueChanged.connect(self.update_positions_modules)
        #self.ui.sendBtn.clicked.connect(self.get_UI_commands)

    def update_nbr_modules(self):
        old_nbr_module = self.sherpent.get_nbr_modules()
        new_nbr_module = self.ui.nbrModuleSpinBox.value()

        if new_nbr_module > old_nbr_module:
            for i in range(old_nbr_module, new_nbr_module):
                self.sherpent.add_module()
                self.add_slider(self.sherpent.get_nbr_modules())

        elif new_nbr_module < old_nbr_module:
            for i in range(old_nbr_module, new_nbr_module, -1):
                self.delete_slider(i)
                self.sherpent.delete_module(i-1)

        self.module_widget.update_modules()

        print(f"Nbr de modules = {self.sherpent.get_nbr_modules()}")

    def add_slider(self, no_slider):
        """ Ajout d'un slider """
        # Création du Label
        label = QLabel(f"Angle {no_slider}: 0°", self)

        # Création du Slider
        slider = QSlider(Qt.Horizontal, self)
        slider.setMinimum(-180)
        slider.setMaximum(180)
        slider.setValue(0)

        list_modules = self.sherpent.get_modules()
        list_modules[no_slider-1].label = label
        list_modules[no_slider-1].slider = slider

        self.ui.commandWidgetVerticalLayout.addWidget(label)
        self.ui.commandWidgetVerticalLayout.addWidget(slider)

        slider.valueChanged.connect(self.update_positions_modules)

    def delete_slider(self, no_slider):
        list_modules = self.sherpent.get_modules()

        label = list_modules[no_slider-1].label
        self.ui.commandWidgetVerticalLayout.removeWidget(label)
        label.deleteLater()

        slider = list_modules[no_slider-1].slider
        self.ui.commandWidgetVerticalLayout.removeWidget(slider)
        slider.deleteLater()

    def update_value_slider(self):
        """ Lit la valeur des sliders, puis met à jour l'angle du module """
        list_modules = self.sherpent.get_modules()
        for i in range(0, self.sherpent.get_nbr_modules()):
            angle = list_modules[i].slider.value()
            list_modules[i].set_angle(1,angle)
            list_modules[i].label.setText(f"Angle {i + 1}: {angle}°")

    def update_value_spinBox_position(self):
        if self.sherpent.get_nbr_modules() > 0:
            list_modules = self.sherpent.get_modules()
            x = self.ui.xPosSpinBox.value()
            y = self.ui.yPosSpinBox.value()

            list_modules[0].set_front_position(x,y)

    def update_positions_modules(self):
        if not self.controller_is_connected:
            self.update_value_slider()
            self.update_value_spinBox_position()
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
        self.controller.wait()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SherpentControl()
    window.show()
    sys.exit(app.exec_())
