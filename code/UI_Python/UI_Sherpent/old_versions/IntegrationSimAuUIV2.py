import sys
import math
from PyQt5.QtWidgets import QApplication, QMainWindow, QSlider, QLabel
from PyQt5.QtCore import Qt
from UI_SherpentV2 import Ui_MainWindow  # Import de l'interface générée par Qt Designer
from PyQt5.QtGui import QPainter, QPen, QBrush, QPolygonF
from PyQt5.QtCore import QPointF
from PyQt5.QtWidgets import QWidget


class ModulesSherpentWidget(QWidget):
    """ Classe pour dessiner le serpent articulé """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.num_module = 0
        self.segment_length = 80
        self.segment_width = 20
        self.angles = []
        self.position_tete = []
        self.setStyleSheet("background-color: white;")

    def update_angles(self, dictPosition, nbrModule):
        """ Met à jour les angles et la position de la tête """
        self.position_tete=[]
        self.position_tete.append(dictPosition["x"])
        self.position_tete.append(dictPosition["y"])

        self.num_module = nbrModule

        self.angles = []
        for i in range(len(dictPosition)-2):
            self.angles.append(dictPosition[f"angle{i}"])

        self.update()

    def paintEvent(self, event):
        """ Dessine le serpent articulé """
        qp = QPainter(self)
        qp.setRenderHint(QPainter.Antialiasing)
        qp.setPen(QPen(Qt.black, 2))
        qp.setBrush(QBrush(Qt.gray))

        if self.num_module > 0:
            x, y = self.position_tete
            angle_acc = 0

            for i in range(self.num_module):
                angle_acc += self.angles[i]
                angle_rad = math.radians(angle_acc)

                x_new = x - self.segment_length * math.cos(angle_rad)
                y_new = y + self.segment_length * math.sin(angle_rad)

                dx = self.segment_width / 2 * math.sin(angle_rad)
                dy = -self.segment_width / 2 * math.cos(angle_rad)

                points = QPolygonF([
                    QPointF(x - dx, y + dy),
                    QPointF(x + dx, y - dy),
                    QPointF(x_new + dx, y_new - dy),
                    QPointF(x_new - dx, y_new + dy)
                ])

                qp.drawPolygon(points)
                x, y = x_new, y_new


class SherpentControl(QMainWindow):
    """ Interface principale utilisant l'UI de Qt Designer """
    def __init__(self):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        self.sherpent = ModulesSherpentWidget(self.ui.widgetAffichageSherpent)
        self.sherpent.setGeometry(0, 0, self.ui.widgetAffichageSherpent.width(), self.ui.widgetAffichageSherpent.height())

        self.ui.sendBtn.clicked.connect(self.get_UI_commands)
        self.ui.nbrModuleSpinBox.valueChanged.connect(self.updateNbrSlider)


        self.nbrModules = 0
        self.position = {"x":0, "y":0}
        self.dictLabelSlider = {}
        self.dictSlider = {}

        self.show()

    def get_UI_commands(self):
        """ Envoie les valeurs de positions au Widget de la sim lors de l'appuie du btn send"""
        #print("Btn send")
        self.position["x"] = self.ui.xPosSpinBox.value()
        self.position["y"] = self.ui.yPosSpinBox.value()
        #print(f"Dict position x : {self.position['x']} y : {self.position['y']}")

        self.sherpent.update_angles(self.position, self.nbrModules)

    def updateValueSlider(self):
        """ Lit les valeurs des sliders et les affichent dans leur label """
        for i in range(0, len(self.dictSlider)):
            self.position[f"angle{i}"] = self.dictSlider[f"slider{i}"].value()
            self.dictLabelSlider[f"label{i}"].setText(f"Angle {i + 1}: {self.position[f'angle{i}']}°")

    def updateNbrSlider(self):
        """ Ajoute ou supprime des sliders d'angle selon le nombre de module"""
        oldNbrModule = self.nbrModules
        newNbrModule = self.ui.nbrModuleSpinBox.value()
        #print("update")

        if newNbrModule > oldNbrModule:
            # Ajout des sliders
            for i in range(newNbrModule):
                if not(f"angle{i}" in self.position):
                    self.position.update({f"angle{i}":0})

                    label = QLabel(f"Angle {i + 1}: 0°", self)
                    slider = QSlider(Qt.Horizontal, self)
                    slider.setMinimum(-180)
                    slider.setMaximum(180)
                    slider.setValue(0)

                    self.dictLabelSlider[f"label{i}"] = label
                    self.dictSlider[f"slider{i}"] = slider

                    self.ui.commandWidgetVerticalLayout.addWidget(label)
                    self.ui.commandWidgetVerticalLayout.addWidget(slider)

                    slider.valueChanged.connect(self.updateValueSlider)

                    #print("slider ajouter")
        elif newNbrModule < oldNbrModule :
            #print("Try remove slider")
            #print(f"New : {newNbrModule}, Old : {oldNbrModule}")
            for i in range(newNbrModule, oldNbrModule):
                label = self.dictLabelSlider.pop(f"label{i}", None)
                slider = self.dictSlider.pop(f"slider{i}", None)
                self.position.pop(f"angle{i}", None)

                if label:
                    self.ui.commandWidgetVerticalLayout.removeWidget(label)
                    label.deleteLater()

                if slider:
                    self.ui.commandWidgetVerticalLayout.removeWidget(slider)
                    slider.deleteLater()
                #print("Slider remove")

        self.nbrModules = newNbrModule


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SherpentControl()
    window.show()
    sys.exit(app.exec_())
