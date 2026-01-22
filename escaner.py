import cv2  # Importamos OpenCV para manejar la cámara y el video
import mediapipe as mp  # Librería de Google para detectar manos
import requests  # Para pedir la imagen y enviar la potencia por Wi-Fi
import numpy as np  # Para manejar las imágenes como si fueran tablas de números
import math  # Para hacer cálculos de distancias entre dedos

# Direcciones para hablar con las placas. Nota: Cambia la IP del motor por la real.
url_cam = "http://192.168.4.1/cam.jpg"
url_motor = "http://192.168.4.2/motor" 

# Margen para que el motor no gire si los dedos están casi juntos
mar_d_error = 0.2

mp_hands = mp.solutions.hands  # Accedemos a las herramientas de manos
mp_dibujo = mp.solutions.drawing_utils  # Herramienta para dibujar puntos en los dedos

# Configuramos el detector de manos
manos = mp_hands.Hands(
    static_image_mode=False, # Procesamos como video fluido
    max_num_hands=1, # Solo nos importa una mano
    min_detection_confidence=0.7, # 70% de seguridad de que es una mano
    min_tracking_confidence=0.7 # 70% de seguridad al seguir el movimiento
)

while True:
    try:
        # 1. PEDIR FOTO: Le pedimos la imagen actual a la ESP32-CAM
        respuesta = requests.get(url_cam, timeout=1)
        # Convertimos los datos crudos en una imagen que Python entienda
        img_array = np.array(bytearray(respuesta.content), dtype=np.uint8)
        frame = cv2.imdecode(img_array, -1)

        if frame is None: continue # Si la foto falló, saltamos al siguiente intento

        frame = cv2.flip(frame, 1) # Volteamos la imagen para que sea como un espejo
        alto, ancho, _ = frame.shape # Obtenemos el tamaño de la foto

        # MediaPipe necesita los colores en orden RGB, pero OpenCV los da en BGR. Los cambiamos:
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        resultado = manos.process(frame_rgb) # ¡Aquí ocurre la magia! Se detecta la mano

        if resultado.multi_hand_landmarks: # Si se detectó al menos una mano
            for mano_puntos in resultado.multi_hand_landmarks:
                # Obtenemos la ubicación de 4 puntos clave (coordenadas X y Y)
                p4 = mano_puntos.landmark[4]  # Punta del pulgar
                p8 = mano_puntos.landmark[8]  # Punta del índice
                p9 = mano_puntos.landmark[9]  # Base del dedo medio (para referencia)
                p0 = mano_puntos.landmark[0]  # Muñeca (para referencia)

                # Calculamos la distancia entre pulgar e índice
                dist_dedos = math.hypot(p8.x - p4.x, p8.y - p4.y)
                # Calculamos el tamaño de la mano (para que no importe si la alejas o acercas)
                dist_ref = math.hypot(p9.x - p0.x, p9.y - p0.y)

                # Convertimos la distancia en un número de 0 a 255 para el motor
                medida_normalizada = dist_dedos / dist_ref
                potencia = (medida_normalizada - mar_d_error) * 255
                potencia_final = int(np.clip(potencia, 0, 255)) # Aseguramos que esté entre 0 y 255

                try:
                    # 2. ENVIAR POTENCIA: Le mandamos el número a la placa del motor
                    requests.get(f"{url_motor}?valor={potencia_final}", timeout=0.05)
                except:
                    pass # Si falla un envío, no pasa nada, el bucle sigue

                # Dibujamos líneas y texto en la pantalla para ver qué está pasando
                x4, y4 = int(p4.x * ancho), int(p4.y * alto)
                x8, y8 = int(p8.x * ancho), int(p8.y * alto)
                cv2.line(frame, (x4, y4), (x8, y8), (0, 255, 0), 3) # Línea verde entre dedos
                cv2.putText(frame, f"Motor: {potencia_final}", (50, 50), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2)
                
                # Dibujamos los puntos y conexiones de la mano (esqueleto)
                mp_dibujo.draw_landmarks(frame, mano_puntos, mp_hands.HAND_CONNECTIONS)

        cv2.imshow("Control por Gestos", frame) # Mostramos la ventana con el video

    except Exception as e:
        print(f"Error: {e}") # Si algo falla, lo escribimos en consola

    if cv2.waitKey(1) & 0xFF == ord('q'): # Si presionas 'q', el programa se cierra
        break

cv2.destroyAllWindows() # Limpiamos las ventanas al salir
manos.close() # Apagamos el detector de manos