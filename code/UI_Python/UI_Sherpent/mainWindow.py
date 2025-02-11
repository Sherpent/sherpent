import sys
from PyQt5 import QtWidgets, QtGui, QtCore
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *

from canvas_Sherpent import MyWidget

class MyWindow(QMainWindow):
    def __init__(self):
        super(MyWindow, self).__init__()
        self.setGeometry(0, 0, 1920, 1080)
        self.setWindowTitle("My first window!")
        self.initUI()
    def initUI(self):
        self.label = QtWidgets.QLabel(self)
        self.label.setText("my first label")
        self.label.move(50, 50)
        self.startButton = QtWidgets.QPushButton(self)
        self.startButton.setText("Start Me")
        self.startButton.move(960, 540)
        self.startButton.setStyleSheet('background-color:green')
        self.startButton.clicked.connect(self.clicked)

        self.image = QtWidgets.QLabel(self)
        self.image.move(100, 100)
        self.image.setPixmap(QtGui.QPixmap("rectangle_1.png"))
    def clicked(self):
        self.label.setText("my second label is bigger than u")
        self.update()
    def update(self):
        self.label.adjustSize()
        self.image.adjustSize()
    def showSherpent(self) -> None:
        self.image.setPixmap(QtGui.QPixmap("rectangle_1.png"))

def window():
    app = QApplication(sys.argv)
    win = MyWindow()
    win.show()
    sys.exit(app.exec_())

window()  # make sure to call the function