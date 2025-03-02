import pygame

# Initialisation de Pygame et de la manette
pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("Aucune manette détectée.")
    pygame.quit()
    exit()

joystick = pygame.joystick.Joystick(0)
joystick.init()
print(f"🎮 Manette détectée : {joystick.get_name()}")


running = True
try:
    while running:
        pygame.event.pump()  # Mise à jour des événements

        # Vérifier les boutons
        for i in range(joystick.get_numbuttons()):
            button_state = joystick.get_button(i)
            if button_state:
                print(f"🔘 Bouton {i} pressé")
        """
        Btn 0 : Croix (X)
        Btn 1 : Rond (O)
        Btn 2 : Carré (▢)
        Btn 3 : Triangle (△)
        Btn 4 : Share
        Btn 5 : PS
        Btn 6 : Options
        Btn 7 : Joystick gauche
        Btn 8 : Joystick droit
        Btn 9 : L1
        Btn 10 : R1
        Btn 11 : Flèche haut
        Btn 12 : Flèche bas
        Btn 13 : Flèche gauche
        Btn 14 : Flèche droite
        Btn 15 : Num pad        
        """

        # Vérifier les axes (joysticks + gâchettes)
        for i in range(4):
            axis_value = round(joystick.get_axis(i), 2)  # Arrondi pour éviter du bruit
            if axis_value <-0.15 or axis_value > 0.15:
                print(f"🕹️ Axe {i} : {axis_value}")

        axis_value_L2 = round(joystick.get_axis(4),2)
        if axis_value_L2 != -1.0:
            print(f"🕹️ Axe 4 : {axis_value_L2}")

        axis_value_R2 = round(joystick.get_axis(5), 2)
        if axis_value_R2 != -1.0:
            print(f"🕹️ Axe 5 : {axis_value_R2}")

        """
        Axe 0 : Joystick gauche - Left(-1) Right(1)
        Axe 1 : Joystick gauche - Up(-1) Down(1)
        Axe 2 : Joystick droit - Left(-1) Right(1)
        Axe 3 : Joystick droit - Up(-1) Down(1)
        Axe 4 : L2 - Pressed(1) Released(-1)
        Axe 5 : R2 - Pressed(1) Released(-1)
        """


except KeyboardInterrupt:
    print("\nArrêt du programme.")
finally:
    joystick.quit()
    pygame.quit()
