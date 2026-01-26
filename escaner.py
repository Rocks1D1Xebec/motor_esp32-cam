import cv2 # Herramienta para usar la cámara
import mediapipe as mp # Herramienta para reconocer manos
import serial # Herramienta para enviar datos por el cable USB
import numpy as np # Herramienta para hacer cálculos de números
import math # Herramienta para matemáticas (como medir distancias)
import time # Herramienta para manejar el tiempo

# 1. Configuramos la "dirección" de nuestro ESP32 y la velocidad de charla
PUERTO = "COM4"
BAUDIOS = 115200

try:
    # Intentamos abrir la línea de comunicación con el ESP32
    esp = serial.Serial(PUERTO, BAUDIOS, timeout=1)
    time.sleep(2) # Esperamos 2 segundos para que el ESP32 "despierte" bien
    print("¡Conectado con éxito!")
except:
    # Si algo sale mal (cable suelto, puerto errado), avisamos aquí
    print("No encontré el ESP32. Revisa el cable.")

# 2. Preparamos al "detective" de manos (MediaPipe)
mp_hands = mp.solutions.hands
mp_dibujo = mp.solutions.drawing_utils
manos = mp_hands.Hands(
    static_image_mode=False, # Decimos que es un video, no una foto fija
    max_num_hands=1,         # Solo nos interesa rastrear una mano
    min_detection_confidence=0.7, # Qué tan seguro debe estar de que es una mano
    min_tracking_confidence=0.7   # Qué tan bien debe seguir el movimiento
)

# 3. Encendemos la cámara de la computadora
cap = cv2.VideoCapture(0)
mar_d_error = 0.2  # Un pequeño margen para que el motor se apague si los dedos están casi juntos

while True: # Este ciclo se repite sin parar
    try:
        # La cámara toma una foto
        ret, frame = cap.read()
        if not ret: continue

        # Volteamos la imagen para que funcione como un espejo
        frame = cv2.flip(frame, 1)
        alto, ancho, _ = frame.shape
        # Cambiamos los colores para que la Inteligencia Artificial los entienda mejor
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # Le pedimos al detective que busque manos en la foto
        resultado = manos.process(frame_rgb)

        # Si el detective encontró una mano...
        if resultado.multi_hand_landmarks:
            for mano_puntos in resultado.multi_hand_landmarks:
                
                # Identificamos puntos clave: punta del pulgar (4) e índice (8)
                p4 = mano_puntos.landmark[4]  
                p8 = mano_puntos.landmark[8]  
                # Puntos de referencia para saber qué tan grande es la mano en pantalla
                p9 = mano_puntos.landmark[9]  
                p0 = mano_puntos.landmark[0]  

                # Medimos la distancia actual entre los dedos pulgar e índice
                dist_dedos_dec = math.hypot(p8.x - p4.x, p8.y - p4.y)
                
                # Medimos el tamaño de la mano para que no importe si la alejas de la cámara
                dist_ref_dec = math.hypot(p9.x - p0.x, p9.y - p0.y)
                
                # Calculamos un valor de apertura (0 = cerrado, 1 = abierto)
                medida_normalizada = dist_dedos_dec / dist_ref_dec
                
                # Convertimos esa apertura en un número para el motor (entre 0 y 255)
                potencia_final = int(np.clip((medida_normalizada - mar_d_error) * 255, 0, 255))

                # --- MOMENTO CLAVE: Creamos el mensaje "M:número" y lo enviamos ---
                mensaje = f"M:{potencia_final}\n"
                esp.write(mensaje.encode())

                # Dibujamos un círculo y líneas en la pantalla para ver qué está pasando
                x4, y4 = int(p4.x * ancho), int(p4.y * alto)
                x8, y8 = int(p8.x * ancho), int(p8.y * alto)

                # Dibujamos una línea verde entre los dedos
                cv2.line(frame, (x4, y4), (x8, y8), (0, 255, 0), 3)
                # Escribimos la potencia en la esquina de la pantalla
                cv2.putText(frame, f"Motor: {potencia_final}", (50, 50), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

                # Dibujamos todos los puntos de la mano detectada
                mp_dibujo.draw_landmarks(frame, mano_puntos, mp_hands.HAND_CONNECTIONS)

        # Mostramos la ventana con todo lo que dibujamos
        cv2.imshow("Control Gestual", frame)

    except Exception as e:
        print(f"Algo falló: {e}")

    # Si presionas la tecla 'q', el programa se detiene
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Al terminar, soltamos la cámara y cerramos el cable USB
cap.release()
esp.close()
cv2.destroyAllWindows()
manos.close()