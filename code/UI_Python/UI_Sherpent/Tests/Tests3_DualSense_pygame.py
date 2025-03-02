import sys
import pygame
import numpy as np
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel
from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QImage, QPixmap

class GameWindow(QWidget):
    def __init__(self):
        super().__init__()

        # Initialisation de Pygame
        pygame.init()

        # Dimensions de la fenêtre
        self.width, self.height = 400, 400
        self.screen = pygame.Surface((self.width, self.height))
        self.setWindowTitle("Déplacer un carré")

        # Position initiale du carré
        self.x, self.y = 150, 150
        self.square_size = 50  # Taille du carré

        # Initialisation de l'interface PyQt
        self.init_ui()

        # Timer pour mettre à jour l'affichage
        self.running = True
        self.update_game()

    def init_ui(self):
        """Initialisation de l'interface graphique avec PyQt."""
        self.layout = QVBoxLayout()
        self.label = QLabel("Utilise les touches fléchées pour déplacer le carré", self)
        self.layout.addWidget(self.label)
        self.setLayout(self.layout)

    def update_game(self):
        """Met à jour l'affichage et le mouvement du carré."""
        pygame.event.pump()  # Récupère les événements

        # Récupérer l'état des touches (flèches directionnelles)
        keys = pygame.key.get_pressed()

        # Déplacement du carré en fonction des touches appuyées
        if keys[pygame.K_LEFT]:
            self.x -= 5
        if keys[pygame.K_RIGHT]:
            self.x += 5
        if keys[pygame.K_UP]:
            self.y -= 5
        if keys[pygame.K_DOWN]:
            self.y += 5

        # Limites de l'écran
        if self.x < 0: self.x = 0
        if self.x > self.width - self.square_size: self.x = self.width - self.square_size
        if self.y < 0: self.y = 0
        if self.y > self.height - self.square_size: self.y = self.height - self.square_size

        # Remplir l'écran en blanc
        self.screen.fill((255, 255, 255))

        # Dessiner le carré
        pygame.draw.rect(self.screen, (255, 0, 0), (self.x, self.y, self.square_size, self.square_size))

        # Sauvegarder l'image dans un tableau numpy et l'afficher dans PyQt
        self.show_pygame_screen()

        # Replanifier la mise à jour (60 FPS)
        QTimer.singleShot(16, self.update_game)

    def show_pygame_screen(self):
        """Convertir la surface Pygame en image PyQt et l'afficher dans un QLabel."""
        # Convertir la surface Pygame en tableau NumPy
        arr = pygame.surfarray.array3d(self.screen)

        # Convertir le tableau NumPy en QImage
        arr = np.transpose(arr, (1, 0, 2))  # Transpose pour adapter à QImage
        height, width, _ = arr.shape

        # Convertir le tableau NumPy en bytes et créer le QImage
        arr_bytes = arr.tobytes()
        image = QImage(arr_bytes, width, height, QImage.Format_RGB888)

        # Créer un QPixmap à partir de l'image
        pixmap = QPixmap.fromImage(image)
        self.label.setPixmap(pixmap)

    def quit_application(self):
        """Quitter l'application proprement."""
        self.running = False
        pygame.quit()
        QApplication.quit()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GameWindow()
    window.show()
    sys.exit(app.exec_())
