# Descripción

El proyecto consiste en la creación de un sistema distribuido utilizando seis servidores: cinco servidores nodo y un servidor centro de datos. El propósito del sistema es entrenar una red neuronal para jugar TicTacToe. Este informe detalla la arquitectura del sistema, los mecanismos de comunicación entre servidores, y los procedimientos de manejo de fallos y actualización de listas de servidores.

## Arquitectura del sistema

1. **Servidor centro de datos:**
   - Contiene el dataset de entrenamiento.
   - Mantiene una lista actualizada de las IP de los servidores nodo y el servidor principal designado, así como la tarea que tienen (nodo/nodo designado).
   - Distribuye el dataset equitativamente entre los servidores nodo usando UDP.
   - Actualiza la lista de servidores nodo en caso de fallos o nuevas conexiones.

2. **Servidor nodo:**
   - Cada uno contiene una red neuronal para entrenar.
   - Reciben 1/5 del dataset del servidor centro de datos.
   - Entrenan su red neuronal con el dataset recibido, junto con los resultados del entrenamiento anterior, a excepción del primer entrenamiento.
   - Envían los datos de entrenamiento resultantes al servidor nodo designado mediante TCP.

3. **Servidor nodo designado:**
   - Recibe los datos de entrenamiento de los otros servidores nodo y los procesa.
   - Genera el resultado final del entrenamiento.
   - Permite que un cliente se conecte y juegue TicTacToe contra la red neuronal entrenada.

## Mecanismos de Comunicación

1. **UDP (User Datagram Protocol):**
   - Utilizado por el servidor centro de datos para distribuir el dataset a los servidores nodo.
   - Permite una transmisión eficiente de datos aunque no garantice la entrega.
   - Se proporciona un CheckSum para verificar si los datos están corruptos.
   - El reenvío de datos se hará utilizando el número de secuencia del envío corrupto.

2. **TCP (Transmission Control Protocol):**
   - Utilizado por los servidores nodo para enviar los datos de entrenamiento resultantes al servidor nodo designado.
   - Garantiza la entrega fiable de datos, esencial para la integridad del entrenamiento final.

## Manejo de Fallos y Actualizaciones

1. **Timeout y Reintentos:**
   - Si un servidor nodo no responde en un tiempo determinado, se considera que ha fallado, para esto se debe utilizar un KeepAlive.
   - El servidor designado informa al servidor centro de datos y este actualiza la lista de servidores y redistribuye las tareas entre los servidores disponibles.

2. **Nuevas Conexiones:**
   - Al conectarse un nuevo servidor nodo, el servidor centro de datos actualiza la lista de servidores.
   - Redistribuye el dataset para incluir al nuevo servidor en el proceso de entrenamiento.
