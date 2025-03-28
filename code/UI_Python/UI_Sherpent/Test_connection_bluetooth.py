import serial
import time

# Remplace 'COM5' par le port attribué à ton ESP32 (Windows)
# Sur Linux, utilise '/dev/rfcomm0' ou un autre port détecté
PORT_BT = "COM5"
BAUDRATE = 115200  # Doit correspondre à la vitesse de l'ESP32

try:
    # Ouvrir la connexion série Bluetooth
    ser = serial.Serial(PORT_BT, BAUDRATE, timeout=1)
    time.sleep(2)  # Laisser le temps à la connexion de s'établir

    # Envoyer un message
    message = "Hello ESP32!\n"
    ser.write(message.encode())  # Convertir en bytes avant d'envoyer
    print(f"Envoyé : {message}")

    # Lire une réponse (si l'ESP32 envoie quelque chose)
    response = ser.readline().decode().strip()
    if response:
        print(f"Réponse de l'ESP32 : {response}")

    ser.close()  # Fermer la connexion

except Exception as e:
    print(f"Erreur : {e}")
