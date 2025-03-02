import sys
import pygame
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QPushButton
from PyQt5.QtCore import QTimer


class GamepadApp(QWidget):
    def __init__(self):
        super().__init__()

        # Initialisation de Pygame et de la manette
        pygame.init()
        pygame.joystick.init()

        if pygame.joystick.get_count() == 0:
            print("Aucune manette détectée.")
            pygame.quit()
            sys.exit()

        self.joystick = pygame.joystick.Joystick(0)
        self.joystick.init()
        self.setWindowTitle(f"🎮 {self.joystick.get_name()}")

        # Interface utilisateur
        self.init_ui()

        # Variables pour les axes
        self.axis_labels = []

        self.running = True
        self.update_gamepad()

    def init_ui(self):
        # Création du layout et des widgets
        self.layout = QVBoxLayout()

        # Label pour afficher les boutons pressés
        self.button_label = QLabel("Press any button", self)
        self.layout.addWidget(self.button_label)

        # Label pour afficher les axes des joysticks
        self.axis_label = QLabel("Joystick axes:", self)
        self.layout.addWidget(self.axis_label)

        # Label pour afficher les valeurs des axes
        self.axis_values_label = QLabel("", self)
        self.layout.addWidget(self.axis_values_label)

        # Bouton pour quitter
        self.quit_button = QPushButton("Quitter", self)
        self.quit_button.clicked.connect(self.quit_application)
        self.layout.addWidget(self.quit_button)

        self.setLayout(self.layout)

    def update_gamepad(self):
        """Mettre à jour les événements de la manette"""
        try:
            pygame.event.pump()  # Mise à jour des événements

            button_pressed = []
            for i in range(self.joystick.get_numbuttons()):
                button_state = self.joystick.get_button(i)
                if button_state:
                    button_pressed.append(f"Btn {i}")

            # Mettre à jour l'affichage des boutons pressés
            if button_pressed:
                self.button_label.setText("Boutons pressés: " + ", ".join(button_pressed))
            else:
                self.button_label.setText("Aucun bouton pressé")

            # Vérifier les axes des joysticks et des gâchettes
            axis_values = []
            for i in range(4):
                axis_value = round(self.joystick.get_axis(i), 2)  # Arrondi pour éviter du bruit
                if axis_value < -0.15 or axis_value > 0.15:
                    axis_values.append(f"Axe {i}: {axis_value}")

            # Vérifier les gâchettes
            axis_value_L2 = round(self.joystick.get_axis(4), 2)
            if axis_value_L2 != -1.0:
                axis_values.append(f"Axe 4: {axis_value_L2}")

            axis_value_R2 = round(self.joystick.get_axis(5), 2)
            if axis_value_R2 != -1.0:
                axis_values.append(f"Axe 5: {axis_value_R2}")

            # Mettre à jour l'affichage des axes
            if axis_values:
                self.axis_values_label.setText("\n".join(axis_values))
            else:
                self.axis_values_label.setText("Aucun axe déplacé")

            # Replanifier la mise à jour de la manette
            if self.running:
                QTimer.singleShot(100, self.update_gamepad)  # Met à jour tous les 100ms

        except KeyboardInterrupt:
            self.quit_application()

    def quit_application(self):
        """Quitter l'application proprement"""
        self.running = False
        self.joystick.quit()
        pygame.quit()
        QApplication.quit()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GamepadApp()
    window.show()
    sys.exit(app.exec_())
