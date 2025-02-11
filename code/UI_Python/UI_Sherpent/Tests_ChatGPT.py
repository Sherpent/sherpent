import sys
import math
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QSlider, QLabel, QFrame
from PyQt5.QtGui import QPainter, QPen, QBrush, QPolygonF
from PyQt5.QtCore import Qt, QPointF

class RobotArmWidget(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.num_segments = 3        # Nombre de segments
        self.segment_length = 80     # Longueur fixe des segments
        self.segment_width = 15      # Largeur fixe des segments
        self.angles = [30, -45, 20]  # Angles initiaux
        self.setStyleSheet("background-color: white;")  # Fond blanc

    def update_angles(self, angles):
        """Met à jour les angles et redessine l'interface."""
        self.angles = angles
        self.update()

    def paintEvent(self, event):
        """Dessine le bras robotique avec des rectangles de taille fixe."""
        qp = QPainter(self)
        qp.setRenderHint(QPainter.Antialiasing)
        qp.setPen(QPen(Qt.black, 2))
        qp.setBrush(QBrush(Qt.gray))  # Remplissage gris des segments

        # Position initiale (centre en bas du widget)
        x, y = self.width() // 2, self.height() - 20
        angle_acc = 0

        for i in range(self.num_segments):
            angle_acc += self.angles[i]  # Somme des angles

            # Angle en radians
            angle_rad = math.radians(angle_acc)

            # Calcul des coordonnées du bout du segment
            x_new = x + self.segment_length * math.cos(angle_rad)
            y_new = y - self.segment_length * math.sin(angle_rad)

            # Décalage pour l'épaisseur des segments
            dx = self.segment_width / 2 * math.sin(angle_rad)
            dy = self.segment_width / 2 * math.cos(angle_rad)

            # Points du rectangle (même taille pour tous)
            points = QPolygonF([
                QPointF(x - dx, y + dy),  # Coin supérieur gauche
                QPointF(x + dx, y - dy),  # Coin inférieur gauche
                QPointF(x_new + dx, y_new - dy),  # Coin inférieur droit
                QPointF(x_new - dx, y_new + dy)   # Coin supérieur droit
            ])

            # Dessiner le segment sous forme de rectangle fixe
            qp.drawPolygon(points)

            # Déplacer le point de départ au bout du segment
            x, y = x_new, y_new

class RobotControl(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Bras Robotique")
        self.setGeometry(100, 100, 500, 600)

        layout = QVBoxLayout()

        # Zone de dessin (bras robotique)
        self.robot_arm = RobotArmWidget(self)
        self.robot_arm.setFixedHeight(300)  # Fixer la hauteur du dessin
        layout.addWidget(self.robot_arm)

        # Sliders et labels
        self.sliders = []
        self.labels = []
        self.angles = [30, -45, 20]

        for i in range(len(self.angles)):
            label = QLabel(f"Angle {i+1}: {self.angles[i]}°", self)
            slider = QSlider(Qt.Horizontal, self)
            slider.setMinimum(-180)
            slider.setMaximum(180)
            slider.setValue(self.angles[i])
            slider.valueChanged.connect(self.update_angles)

            self.labels.append(label)
            self.sliders.append(slider)
            layout.addWidget(label)
            layout.addWidget(slider)

        self.setLayout(layout)

    def update_angles(self):
        """Mise à jour des angles et du dessin."""
        for i in range(len(self.sliders)):
            self.angles[i] = self.sliders[i].value()
            self.labels[i].setText(f"Angle {i+1}: {self.angles[i]}°")
        self.robot_arm.update_angles(self.angles)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = RobotControl()
    window.show()
    sys.exit(app.exec_())
