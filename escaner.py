import cv2                # para manejar la imagen y video
import mediapipe as mp    # para detectar la mano y los puntos
import requests           # para enviar la potencia a la ESP32
import numpy as np        # para los calculos matematicos

# links que se usan para obtener la imagen y enviar el control del motor
url_cam = "http://192.168.0.14/cam.jpg"
url_motor = "http://192.168.0.14/control"
#le decimos al mediapipe que lo usaremos para detectetar manos
mp_manos = mp.solutions.hands # Acceso al modelo de manos
mp_dibujo = mp.solutions.drawing_utils # Herramienta para dibujar puntos y líneas

#configuramos la deteccion de manos
manos = mp_manos.Hands(
    static_image_mode=False,     # False porque es video en tiempo real
    max_num_hands=1,              # Solo detectaremos una mano
    min_detection_confidence=0.7, # 70% de seguridad para detectar la mano
    min_tracking_confidence=0.7   # 70% de seguridad para seguir el movimiento
)


#Actualizare el codigo de poco a poco