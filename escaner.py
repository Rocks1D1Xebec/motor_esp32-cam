import cv2  # Importamos OpenCV, sirve para mostrar la imagen de la cámara, dibujar líneas, texto y trabajar con imágenes en general
import mediapipe as mp  #librería para detección de manos
import requests  #para pedir la imagen de la camara y enviarle datos al motor
import numpy as np  #para manejar imágenes como listas de números
import math  #mas funciones matematicas

# link para pedir la imagen de la cámara y enviarle datos al motor
url_cam = "http://192.168.4.1/cam.jpg"
url_motor = "http://192.168.4.1/control"

# El verdadero juntos(0.2) y separado(1.2)
DISTANCIA_MIN = 0.2
DISTANCIA_MAX = 1.2

mp_hands = mp.solutions.hands  # Aqui accedemos al modulo de deteccion de manos
mp_dibujo = mp.solutions.drawing_utils  # Esto nos permite dibujar los puntos y líneas de la mano en pantalla

# Creamos el detector de manos
manos = mp_hands.Hands(
    static_image_mode=False,  # False significa que es como un video, no solo una foto
    max_num_hands=1,  # Solo detecta una mano para evitar confusión
    min_detection_confidence=0.7,  # Qué tan seguro debe estar para decir “esto es una mano”
    min_tracking_confidence=0.7  # Qué tan seguro debe estar para seguir la mano mientras se mueve
)

# Este while hace que el programa nunca se detenga
while True:
    try:
        # Pedimos una imagen a la ESP32-CAM
        respuesta = requests.get(url_cam, timeout=1)

        # la imagen llega en bytes y los convertimos en un array de numeros para que se puedo procesar la imagen 
        img_array = np.array(bytearray(respuesta.content), dtype=np.uint8)

        # Convertimos esos números en una imagen real
        frame = cv2.imdecode(img_array, -1)

        # Si la imagen llegó mal, volvemos a intentar
        if frame is None:
            continue

        # Volteamos la imagen para que se vea como espejo
        frame = cv2.flip(frame, 0)

        alto, ancho, _ = frame.shape# alto = qué tan alta es la imagen, ancho = qué tan ancha es la imagen, _ = colores (no lo usamos(si lo borras no hay ventana del cv2(no me preguntes porque porque no entendi la explicacion de la ia xd)))

        # Convertimos la imagen de BGR(blanco y negro) a RGB(colores) porque MediaPipe solo entiende RGB
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # MediaPipe analiza la imagen buscando una mano
        resultado = manos.process(frame_rgb)

        # Si MediaPipe encontró una mano
        if resultado.multi_hand_landmarks:
            for mano_puntos in resultado.multi_hand_landmarks:
                
                # Guardamos puntos importantes de la mano
                p4 = mano_puntos.landmark[4]  # Punta del pulgar
                p8 = mano_puntos.landmark[8]  # Punta del dedo índice
                p0 = mano_puntos.landmark[0]  # Muñeca
                p9 = mano_puntos.landmark[9]  # Base del dedo medio
                # obtnemos las coordenadas en píxeles
                x4, y4 = int(p4.x * ancho), int(p4.y * alto)
                x8, y8 = int(p8.x * ancho), int(p8.y * alto)

                # Calculamos la distancia entre el pulgar y el índice
                dist_dedos = math.hypot(x8 - x4, y8 - y4)

                # Calculamos el tamaño de la mano como referencia, para que no afecte si esta cerca o lejos
                dist_ref = math.hypot(p9.x * ancho - p0.x * ancho, p9.y * alto - p0.y * alto)

                # Normalizamos la distancia para quefuncione igual aunque la mano esté cerca o lejos
                medida_normalizada = dist_dedos / dist_ref

                # Convertimos el gesto en un numero entre 0 y 255
                potencia = (
                    (medida_normalizada - DISTANCIA_MIN)
                    * 255
                    / (DISTANCIA_MAX - DISTANCIA_MIN)
                )

                # Aseguramos que el valor no se salga del rango
                potencia_final = int(np.clip(potencia, 0, 255))

                try:
                    # Enviamos la potencia al ESP32 por internet
                    requests.get(
                        f"{url_motor}?potencia={potencia_final}",
                        timeout=0.1
                    )
                except:
                    # Si falla el envío, el programa sigue funcionando
                    pass

                # Dibujamos una línea entre pulgar e índice
                cv2.line(frame, (x4, y4), (x8, y8), (0, 255, 0), 3)

                # Mostramos el valor del motor en la imagen
                cv2.putText(
                    frame,
                    f"Motor: {potencia_final}",
                    (50, 50),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    1,
                    (0, 255, 255),
                    2
                )

                # Dibujamos todos los puntos y líneas de la mano
                mp_dibujo.draw_landmarks(
                    frame,
                    mano_puntos,
                    mp_hands.HAND_CONNECTIONS
                )

        # Mostramos la imagen en una ventana
        cv2.imshow("Control Gestual de Motor", frame)

    except Exception as e:
        # Si ocurre un error grave, lo mostramos en la consola
        print(f"Error en el sistema: {e}")

    # Si se presiona la tecla "q", se cierra el programa
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Cerramos todas las ventanas
cv2.destroyAllWindows()

# Liberamos el detector de manos
manos.close()