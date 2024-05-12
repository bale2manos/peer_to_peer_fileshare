Para poder ejecutar la aplicación sin problemas debemos seguir los siguientes sencillos pasos:

1. **Compilación:**
    - Ejecutar ambos Makefile en la terminal:
        - `make` (para compilar el cliente y el servidor normal)
        - `make -f Makefile.servidor_rpc` (para compilar lo relativo al servidor RPC)

2. **Servicio RPC:**
    - Para que el servicio RPC funcione correctamente, ejecutar:
        - `sudo rpcbind start`

3. **Paquetes necesarios:**
    - Para que el servicio web funcione correctamente, es necesario instalar los siguientes paquetes:
        - `spyne` (para el servidor web)
        - `zeep` (para el cliente)

4. **Directorio para el Cliente:**
    - Es un requisito obligatorio que el cliente disponga de un directorio en el mismo nivel que el ejecutable `client.py` con el nombre `./local_storage/<nombre_usuario>`, donde deben existir los archivos que se quieren publicar.

5. **Levantar los Servidores:**
    - Ejecutar los siguientes comandos para levantar los servidores necesarios:
        - `./server  -p <puerto_server_central>`
        - `./server_rpc`
        - `python3 web_server.py`
        - `python3 client.py -s <address> -p <puerto_server_central>`

Es importante destacar que los archivos deben crearse de forma manual. Por ejemplo, si "usuario1" publica "fichero1", este debe ser creado manualmente en `./local_storage/usuario1`, ya que de lo contrario, la operación `GET_FILE` de ese archivo devolverá un error.