import cv2                #para manejar las imágenes y la ventana de video
import mediapipe as mp    #inteligencia artificial de Google para detectar manos
import requests           #para pedir las fotos/videos del esp
import numpy as np        #sirve para convertir los datos brutos de la foto en números que entienda Python

mp_hands = mp.solutions.hands               # Accedemos a la herramienta específica de "Manos" dentro de MediaPipe
mp_dibujo = mp.solutions.drawing_utils      # Accedemos a la herramienta para "Dibujar" los palitos y puntos

# Configuramos cómo va a comportarse el detector
manos = mp_hands.Hands(
    static_image_mode=False,      # false para videos y true para una sola foto
    max_num_hands=1,              # le decimos que solo busque UNA mano
    min_detection_confidence=0.7, # Tiene que estar al 70% seguro de que es una mano para marcarla.
    min_tracking_confidence=0.7   # Tiene que estar al 70% seguro para seguir el movimiento de los dedos.
)

url_cam = "http://10.76.73.52/cam.jpg"

# Bucle principal
while True:
    try:
        # Le pedimos la imagen con un tiempo de espera máximo de 1 segundo para que no se trabe
        respuesta = requests.get(url_cam, timeout=1) 
        
        # Los datos llegan como bytes (ceros y unos) los convertimos a un arreglo de números
        img_array = np.array(bytearray(respuesta.content), dtype=np.uint8)
        
        # Decodificamos ese arreglo binario para crear una imagen real que OpenCV pueda entender
        frame = cv2.imdecode(img_array, -1)

        # Si por alguna razón la imagen llegó vacía saltamos al siguiente intento(volvemos al principio del while)
        if frame is None:
            continue

        #0 para voltear la imagen verticalmente
        frame = cv2.flip(frame, 0)

        #de blanco a negro lo cambiamos para que sea a color
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        
        # Le enviamos la imagen a MediaPipe para que busque manos
        resultado = manos.process(frame_rgb)

        # si hay una mano
        if resultado.multi_hand_landmarks:
            # Recorremos las manos encontradas (aunque configuramos solo 1)
            for mano_puntos in resultado.multi_hand_landmarks:
                # dibuja los puntos rojos y las líneas blancas sobre tu mano
                mp_dibujo.draw_landmarks(frame, mano_puntos, mp_hands.HAND_CONNECTIONS)

        # Abre una ventana con el título Monitor Camara y muestra frama(la variable que guarda la imagen)
        cv2.imshow("Monitor Camara - Solo Deteccion", frame)

    #Si se desconecta el wifi la imagen en la pantalla se queda en lo ultimo y se vuelve al princio del buble
    except Exception:
        pass

    # Si presionamos la tecla 'q', rompemos el bucle y cerramos todo
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Al terminar, cerramos todas las ventanas abiertas
cv2.destroyAllWindows()
# Cerramos el detector de manos para liberar memoria
manos.close()