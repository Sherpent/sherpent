import sys
import math
from PyQt5.QtWidgets import QApplication, QMainWindow, QSlider, QLabel
from PyQt5.QtCore import Qt
from test import Ui_MainWindow  # Import de l'interface générée par Qt Designer
from PyQt5.QtGui import QPainter, QPen, QBrush, QPolygonF
from PyQt5.QtCore import QPointF
from PyQt5.QtWidgets import QWidget


class ModulesSherpentWidget(QWidget):
    """ Classe pour dessiner le serpent articulé """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.num_module = 5
        self.segment_length = 80
        self.segment_width = 20
        self.angles = [30] * self.num_module
        self.position_tete = [250, 20]
        self.setStyleSheet("background-color: white;")

    def update_angles(self, angles, position_tete):
        """ Met à jour les angles et redessine """
        self.angles = angles
        self.position_tete = position_tete
        self.update()

    def paintEvent(self, event):
        """ Dessine le serpent articulé """
        qp = QPainter(self)
        qp.setRenderHint(QPainter.Antialiasing)
        qp.setPen(QPen(Qt.black, 2))
        qp.setBrush(QBrush(Qt.gray))

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

        self.sliders = []
        self.labels = []
        self.angles = [30] * 5
        self.position_tete = [250, 20]

        for i in range(len(self.angles)):
            label = QLabel(f"Angle {i+1}: {self.angles[i]}°", self)
            slider = QSlider(Qt.Horizontal, self)
            slider.setMinimum(-180)
            slider.setMaximum(180)
            slider.setValue(self.angles[i])
            slider.valueChanged.connect(self.update_position)

            self.ui.verticalLayout_2.addWidget(label)
            self.ui.verticalLayout_2.addWidget(slider)
            self.labels.append(label)
            self.sliders.append(slider)

        self.show()

    def update_position(self):
        """ Met à jour les angles et rafraîchit l'affichage """
        for i in range(len(self.angles)):
            self.angles[i] = self.sliders[i].value()
            self.labels[i].setText(f"Angle {i + 1}: {self.angles[i]}°")

        self.sherpent.update_angles(self.angles, self.position_tete)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SherpentControl()
    window.show()
    sys.exit(app.exec_())
