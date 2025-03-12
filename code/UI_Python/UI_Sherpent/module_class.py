from PyQt5.QtWidgets import QSlider, QLabel

class Modules:
    def __init__(self, no_module):
        self.module_angle_1 = 0.0
        self.module_angle_2 = 0.0

        self.front_pos_x = 0
        self.front_pos_y = 0
        self.back_pos_x = 0
        self.back_pos_y = 0

        self.module_width = 20
        self.module_length = 80

        self.module_color = 0
        self.label = QLabel("")
        self.slider = QSlider()

        self.module_name = ""

        self.set_name(f"module_{no_module}")

        print(f"Nom du module : {self.module_name}")

    def set_angle(self, angle_ID, angle_value):
        if angle_ID == 1:
            self.module_angle_1 = angle_value

        elif angle_ID == 2:
            self.module_angle_2 = angle_value

    def get_angle(self, angle_ID):
        if angle_ID == 1:
            return self.module_angle_1

        elif angle_ID == 2:
            return self.module_angle_2

    def set_front_position(self, x, y):
        self.front_pos_x = x
        self.front_pos_y = y

    def get_front_position(self):
        return [self.front_pos_x, self.front_pos_y]

    def set_back_position(self, x, y):
        self.back_pos_x = x
        self.back_pos_y = y

    def get_back_position(self):
        return [self.back_pos_x, self.back_pos_y]

    def set_name(self, name):
        self.module_name = name

    def get_name(self):
        return self.module_name

