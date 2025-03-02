import sys
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout
from PyQt5.QtCore import Qt, QPointF, QTimer
from PyQt5.QtGui import QPainter, QBrush, QPen, QColor

class JoystickWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.setFixedSize(300, 300)
        self.joystick_center = QPointF(150, 150)
        self.joystick_radius = 40
        self.base_radius = 120
        self.offset = QPointF(0, 0)
        self.setFocusPolicy(Qt.StrongFocus)
        self.step = 20
        self.key_timer = QTimer(self)
        self.key_timer.timeout.connect(self.update_offset)
        self.active_key = None

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # Dessiner la base du joystick
        painter.setBrush(QBrush(QColor(200, 200, 200)))
        painter.drawEllipse(self.joystick_center, self.base_radius, self.base_radius)

        # Dessiner le joystick
        painter.setBrush(QBrush(QColor(100, 100, 250)))
        joystick_position = self.joystick_center + self.offset
        painter.drawEllipse(joystick_position, self.joystick_radius, self.joystick_radius)

    def mouseMoveEvent(self, event):
        if event.buttons() & Qt.LeftButton:
            self.move_joystick(event.pos())

    def mouseReleaseEvent(self, event):
        self.offset = QPointF(0, 0)
        self.update()

    def keyPressEvent(self, event):
        if not self.key_timer.isActive():
            self.active_key = event.key()
            self.key_timer.start(50)

    def keyReleaseEvent(self, event):
        if event.key() == self.active_key:
            self.key_timer.stop()
            self.active_key = None
            self.offset = QPointF(0, 0)
            self.update()

    def update_offset(self):
        if self.active_key == Qt.Key_Up:
            self.offset.setY(self.offset.y() - self.step)
        elif self.active_key == Qt.Key_Down:
            self.offset.setY(self.offset.y() + self.step)
        elif self.active_key == Qt.Key_Left:
            self.offset.setX(self.offset.x() - self.step)
        elif self.active_key == Qt.Key_Right:
            self.offset.setX(self.offset.x() + self.step)
        self.limit_offset()
        self.update()
        print("Position joystick :", self.get_position())

    def move_joystick(self, pos):
        delta = pos - self.joystick_center
        if delta.manhattanLength() > self.base_radius - self.joystick_radius:
            delta = delta * ((self.base_radius - self.joystick_radius) / delta.manhattanLength())
        self.offset = delta
        self.update()

    def limit_offset(self):
        if self.offset.manhattanLength() > self.base_radius - self.joystick_radius:
            self.offset = self.offset * ((self.base_radius - self.joystick_radius) / self.offset.manhattanLength())

    def get_position(self):
        return (self.offset.x() / (self.base_radius - self.joystick_radius),
                -self.offset.y() / (self.base_radius - self.joystick_radius))

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Joystick Widget")
        layout = QVBoxLayout()
        self.joystick = JoystickWidget()
        layout.addWidget(self.joystick)
        self.setLayout(layout)
        self.resize(320, 350)

        self.joystick.mouseMoveEvent = self.mouse_move_event

    def mouse_move_event(self, event):
        self.joystick.move_joystick(event.pos())
        print("Position joystick :", self.joystick.get_position())

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
