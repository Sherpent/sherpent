import sys
import math
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QSlider, QLabel, QFrame
from PyQt5.QtGui import QPainter, QPen, QBrush, QPolygonF
from PyQt5.QtCore import Qt, QPointF


class ModulesSherpentWidget(QFrame):
    """
    Classe pour calculer et dessiner la position du sherpent
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.num_module = 5
        self.segment_length = 80
        self.segment_width = 20
        self.angles = []
        for i in range(self.num_module):
            self.angles.append(30)
        self.position_tete = [250, 20]

        self.setStyleSheet("background-color: white;")

    def update_angles(self, angles, position_tete):
        """ Met à jour les angles et redessine l'interface """
        self.angles = angles
        self.position_tete = position_tete
        self.update()

    def paintEvent(self, event):
        """ Calculs et dessine le sherpent selon les angles données"""
        qp = QPainter(self)
        qp.setRenderHint(QPainter.Antialiasing)
        qp.setPen(QPen(Qt.black, 2))
        qp.setBrush(QBrush(Qt.gray))

        x = self.position_tete[0]
        y = self.position_tete[1]
        angle_acc = 0

        for i in range(self.num_module):
            angle_acc += self.angles[i]

            # Angle en rad
            angle_rad = math.radians(angle_acc)

            x_new = x - self.segment_length * math.cos(angle_rad)
            y_new = y + self.segment_length * math.sin(angle_rad)

            dx = self.segment_width / 2 * math.sin(angle_rad)
            dy = self.segment_width / 2 * math.cos(angle_rad)*-1

            points = QPolygonF([
                QPointF(x - dx, y + dy),  # Coin supérieur gauche
                QPointF(x + dx, y - dy),  # Coin inférieur gauche
                QPointF(x_new + dx, y_new - dy),  # Coin inférieur droit
                QPointF(x_new - dx, y_new + dy)  # Coin supérieur droit
            ])

            # Dessiner le segment sous forme de rectangle fixe
            qp.drawPolygon(points)

            # Déplacer le point de départ au bout du segment
            x, y = x_new, y_new

class SherpentControl(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Sherpent")
        self.hauteur = 800
        self.largeur = 1000
        self.setGeometry(100, 100, self.largeur, self.hauteur)

        layout = QVBoxLayout()

        # Zone de dessin
        self.sherpent = ModulesSherpentWidget(self)
        self.sherpent.setFixedHeight(self.hauteur//2)  # Fixer la hauteur du dessin
        layout.addWidget(self.sherpent)

        # Sliders et labels
        self.sliders = []
        self.labels = []
        self.angles = []
        for i in range(self.sherpent.num_module):
            self.angles.append(30)


        self.position_tete =[250,20]

        for i in range(len(self.angles)):
            label = QLabel(f"Angle {i+1}: {self.angles[i]}°", self)
            slider = QSlider(Qt.Horizontal, self)
            slider.setMinimum(-180)
            slider.setMaximum(180)
            slider.setValue(self.angles[i])
            slider.valueChanged.connect(self.update_position)

            self.labels.append(label)
            self.sliders.append(slider)
            layout.addWidget(label)
            layout.addWidget(slider)

        # Slider x
        label_slider_x = QLabel(f"Position x : {self.position_tete[0]}", self)
        slider_x = QSlider(Qt.Horizontal, self)
        slider_x.setMinimum(0)
        slider_x.setMaximum(self.largeur)
        slider_x.setValue(self.position_tete[0])
        slider_x.valueChanged.connect(self.update_position)

        self.labels.append(label_slider_x)
        self.sliders.append(slider_x)
        layout.addWidget(label_slider_x)
        layout.addWidget(slider_x)

        # Slider y
        label_slider_y = QLabel(f"Position y : {self.position_tete[1]}", self)
        slider_y = QSlider(Qt.Horizontal, self)
        slider_y.setMinimum(0)
        slider_y.setMaximum(self.hauteur//2)
        slider_y.setValue(self.position_tete[1])
        slider_y.valueChanged.connect(self.update_position)

        self.labels.append(label_slider_y)
        self.sliders.append(slider_y)
        layout.addWidget(label_slider_y)
        layout.addWidget(slider_y)

        self.setLayout(layout)

    def update_position(self):
        """Mise à jour des angles et du dessin."""
        for i in range(len(self.sliders)-2):
            self.angles[i] = self.sliders[i].value()
            self.labels[i].setText(f"Angle {i + 1}: {self.angles[i]}°")


        self.position_tete[0] = self.sliders[self.sherpent.num_module].value()
        self.labels[self.sherpent.num_module].setText(f"Position x : {self.position_tete[0]}")

        self.position_tete[1] = self.sliders[self.sherpent.num_module+1].value()
        self.labels[self.sherpent.num_module+1].setText(f"Position y : {self.position_tete[1]}")

        self.sherpent.update_angles(self.angles, self.position_tete)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SherpentControl()
    window.show()
    sys.exit(app.exec_())