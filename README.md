# EJERCICIO DE IPC Y SINCRONIZACIÓN DE PROCESOS
Proceso No. 1 (p1): recibe como parámetro desde la línea de comandos, la orden de ejecutar
un comando (no interactivo y sin argumentos) disponible en el sistema operativos Linux (p.
ej.: ls, date, uname, ps). Este proceso debe validar que se le haya pasado un parámetro
desde la línea de comandos, en caso de ejecutarse sin parámetros, debe indicar un
mensaje como: «Uso: p1 /ruta/al/ejecutable». El comando que se pasa como
parámetro debe pasarse con la ruta completa del ejecutable. Por ejemplo, el comando ls
hay que pasarlo al proceso No. 1 como /usr/bin/ls. Para conocer la ruta completa de
un comando se puede usar el comando which seguido del nombre del comando, por
ejemplo: which ls retorna con /usr/bin/ls. Consulte man which.

• Proceso No. 2 (p1-child): recibe por una tubería sin nombre el comando que se entregó al proceso
No. 1 y verifica si el archivo ejecutable asociado al comando recibido existe en el sistema
de archivos. En caso de que no exista, debe responder por una tubería sin nombre al
proceso No. 1 el mensaje «No se encuentra el archivo a ejecutar». También debe
verificar que el proceso No. 3 esté en ejecución, si el proceso No. 3 no está en ejecución,
responde por la misma tubería sin nombre que tiene con el proceso No. 1, el mensaje:
«Proceso p3 no parece estar en ejecución». El proceso No. 1 cuando reciba del
proceso No. 2 cualquier mensaje por la tubería entre ambos procesos, lo muestra por la
salida estándar (pantalla) y ambos procesos terminan su ejecución.

• Proceso No. 3 (p3): es un proceso autónomo, independientemente creado, se ejecuta de
primero (en una segunda terminal) y recibe por un área de memoria compartida con el
proceso No. 2, el comando verificado por este último y ejecuta el comando creando una
tubería sin nombre para la redirección de la salida estándar del proceso asociado al
comando. El resultado de la ejecución del comando (la salida del comando ejecutado) NO
se debe mostrar por la salida estándar en el proceso No. 3; se debe devolver al área de
memoria compartida con el proceso No. 2, este último pasa por la tubería sin nombre el
resultado de la ejecución del comando al proceso No. 1 y este último es quién muestra el
resultado de la ejecución del comando en la salida estándar.

Continua en la siguiente página...

La siguiente figura ilustra las descripciones anteriores para ejecutar el comando
/usr/bin/uname, que en el caso de CentOS 7.0, retorna el nombre del Kernel como una
cadena de caracteres con el valor Linux.
