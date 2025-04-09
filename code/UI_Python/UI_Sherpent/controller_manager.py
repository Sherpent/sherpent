import pygame
import threading
from PyQt5 import QtCore
import math

class ControllerManager(QtCore.QThread):
    """Gestion de la manette en arrière-plan avec pygame."""

    controller_connected = QtCore.pyqtSignal(bool)

    def __init__(self, sherpent, parent=None):
        super().__init__(parent)

        self.sherpent = sherpent
        self.running = True
        pygame.init()
        pygame.joystick.init()


        self.index_module = 0

        if pygame.joystick.get_count() > 0:
            self.joystick = pygame.joystick.Joystick(0)
            self.joystick.init()
            print(f"Manette détectée : {self.joystick.get_name()}")

        else:
            self.joystick = None

    def run(self):
        """Boucle d'écoute des événements de la manette."""
        while self.running:
            if self.sherpent.demarrage:

                pygame.event.pump()  # Met à jour les événements pygame
                if self.joystick:
                    x_axis = self.joystick.get_axis(0)  # Axe X (gauche-droite)
                    y_axis = self.joystick.get_axis(1)*-1  # Axe Y (haut-bas)
                    r2_trigger = self.joystick.get_axis(5)  # Gâchette droite R2
                    l2_trigger = self.joystick.get_axis(4)  # Gâchette gauche L2
                    r1_trigger = self.joystick.get_button(10)  # Gâchette droite R2
                    l1_trigger = self.joystick.get_button(9)  # Gâchette gauche L2

                    if x_axis < 0.15 and x_axis > -0.15:
                        x_axis = 0
                    if y_axis < 0.15 and y_axis > -0.15:
                        y_axis = 0

                    if self.sherpent.simulation_activated:
                        nbr_modules = self.sherpent.get_nbr_modules()

                        # Récupère le premier module
                        if nbr_modules > 0:
                            if r1_trigger:
                                #print("trigger")
                                if self.index_module + 1 >= nbr_modules:
                                    #print("Ajout d'un index inexistant")
                                    self.index_module = self.sherpent.get_nbr_modules()-1
                                else:
                                    self.index_module += 1

                            elif l1_trigger:
                                self.index_module -= 1
                                if self.index_module < 0:
                                    self.index_module = 0

                            module_zero = self.sherpent.get_modules()[0]

                            # Mise à jour de la position avec le joystick
                            new_x = module_zero.get_front_position()[0] + int(x_axis * 5)
                            new_y = module_zero.get_front_position()[1] + int(y_axis * 5)
                            module_zero.set_front_position(new_x, new_y)


                            # Mise à jour de l'angle avec les gâchettes
                            module_x = self.sherpent.get_modules()[self.index_module]
                            angle = module_x.get_angle(1) + (r2_trigger - l2_trigger) * 5
                            module_x.set_angle(1, round(angle,1))

                    else:

                        self.sherpent.vecteur_x = round(x_axis,1)
                        self.sherpent.vecteur_y = round(y_axis,1)

                        self.sherpent.side_winding = round( ((l2_trigger*-1)+r2_trigger) ,1)

                pygame.time.wait(100)  # Évite d'utiliser 100% du CPU


    def stop(self):
        """Arrête la boucle de la manette."""
        self.running = False
        pygame.quit()

    def check_controller_status(self):
        """Vérifie si la manette est connectée après un court délai."""
        if self.joystick:
            print("---> Signal emis")
            self.controller_connected.emit(True)

        else:
            self.controller_connected.emit(False)
