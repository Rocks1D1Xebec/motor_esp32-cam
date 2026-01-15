import cv2                # para manejar la imagen y video
import mediapipe as mp    # para detectar la mano y los puntos
import requests           # para enviar la potencia a la ESP32
import numpy as np        # para los calculos matematicos

print(mp.__version__)
#Actualizare el codigo de poco a poco