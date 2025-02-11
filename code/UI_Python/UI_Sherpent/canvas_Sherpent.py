'''
import sys
from PyQt5 import QtWidgets, QtGui, QtCore
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *

class MyWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.setGeometry(0, 0, 1920, 1080)

    def paintEvent(self, event):
        # Création de l'objet rectangle
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # Création des couleurs des bordures et du filling
        painter.setBrush(QColor(0, 255, 0))
        painter.setPen(QColor(0, 0, 255))

        painter.translate(70, 70)
        painter.rotate(30)
        painter.drawRect(0, 0, 300, 100)

        painter.end()


app = QtWidgets.QApplication([])
rectangle_1 = MyWidget()
rectangle_2 = MyWidget()
#rectangle_1.show()
imageVar = QImage(rectangle_1.size(), QImage.Format.Format_ARGB32)
rectangle_1.render(imageVar)
imageVar.save("rectangle_1.png")

#app.exec()
'''

import sys
from PyQt5 import QtWidgets, QtGui, QtCore
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *


class MyWidget:
    def __init__(self, x_offset=0, y_offset=0):
        self.x_offset = x_offset  # Décalage en X
        self.y_offset = y_offset  # Décalage en Y

    def draw(self, painter):
        # Création des couleurs des bordures et du remplissage
        painter.setBrush(QColor(0, 255, 0))
        painter.setPen(QColor(0, 0, 255))

        # Appliquer un décalage et une rotation avant de dessiner
        painter.translate(self.x_offset, self.y_offset)
        painter.rotate(30)  # Rotation de 30 degrés
        painter.drawRect(0, 0, 300, 100)  # Dessiner un rectangle


class MyWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setGeometry(0, 0, 1920, 1080)
        self.setWindowTitle("Dessiner plusieurs rectangles")

    def paintEvent(self, event):
        # Créer un QImage pour dessiner l'image finale
        imageVar = QImage(1920, 1080, QImage.Format_ARGB32)
        imageVar.fill(QtCore.Qt.white)  # Remplir l'image d'un fond blanc

        # Créer un QPainter pour dessiner sur le QImage
        painter = QPainter(imageVar)

        # Créer plusieurs objets MyWidget (rectangles)
        widget_1 = MyWidget(x_offset=70, y_offset=70)
        widget_2 = MyWidget(x_offset=400, y_offset=200)

        # Dessiner les widgets sur l'image
        widget_1.draw(painter)
        widget_2.draw(painter)

        painter.end()  # Fin du dessin sur l'image

        # Sauver l'image dans un fichier
        imageVar.save("multiple_rectangles.png")

        # Afficher l'image finale dans le widget
        painter.begin(self)
        painter.drawImage(0, 0, imageVar)
        painter.end()


# Lancer l'application PyQt
app = QtWidgets.QApplication([])

window = MyWindow()
window.show()

app.exec_()


