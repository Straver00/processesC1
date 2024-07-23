#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>

int main(int argc, char const *argv[]) {
    //tuberias para el proceso 1 y 2

    int tuberiaEnvio[2], tuberiaRecepcion[2];
    if (pipe(tuberiaEnvio) == -1) {
        perror("Error al crear tuberia en el proceso 1");
        return -1;
    }
    if (pipe(tuberiaRecepcion) == -1) {
        perror("Error al crear tuberia en el proceso 1");
        return -1;
    }

    //semaforo del proceso 1
    sem_unlink("/semaforoP1");
    sem_t *semaforoP1;
    semaforoP1 = sem_open("/semaforoP1", O_CREAT, 0666, 0);
    if (semaforoP1 == SEM_FAILED) {
        perror("Error al crear semaforo en el proceso 1");
        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);
        return -1;
    }
    
    //semaforo del proceso 2
    sem_unlink("/semaforoP2");
    sem_t *semaforoP2;
    semaforoP2 = sem_open("/semaforoP2", O_CREAT, 0666, 0);
    if (semaforoP2 == SEM_FAILED) {
        perror("Error al crear semaforo en el proceso 1");
        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);
        sem_close(semaforoP1);
        sem_unlink("/semaforoP1");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error al crear proceso hijo");
        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);
        sem_close(semaforoP2);
        sem_close(semaforoP1);
        sem_unlink("/semaforoP1");
        sem_unlink("/semaforoP2");
        return -1;
    }

    if (pid > 0) {
        //Proceso padre (p1)
        close(tuberiaEnvio[0]);
        close(tuberiaRecepcion[1]);

        const char *ruta;

        //caso de un solo argumento 
        if (argc == 1) {
            printf("Uso: p1 /ruta/al/ejecutable\n");

            //envio a proceso p2
            ruta = "exit";
            write(tuberiaEnvio[1], &ruta, sizeof(ruta));
            sem_post(semaforoP2);

            //liberación de recursos
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP1");
            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }
        
        //caso para 2 argumentos
        ruta = argv[1];
        write(tuberiaEnvio[1], &ruta, sizeof(ruta));
        sem_post(semaforoP2);

        //espera para recibir mensajes de proceso p2
        while (1) {
            sem_wait(semaforoP1);

            //lee el mensaje de proceso p2
            char mensaje[4096];
            int bytesRead = read(tuberiaRecepcion[0], mensaje, sizeof(mensaje) - 1);
            if (bytesRead >= 0) {
                mensaje[bytesRead] = '\0';
            }

            if (strcmp(mensaje, "exit") == 0) {
                sem_post(semaforoP2);
                break;
            }
            printf("%s", mensaje);
            
            sem_post(semaforoP2);
        }

        //liberación de recursos
        sem_close(semaforoP2);
        sem_close(semaforoP1);
        sem_unlink("/semaforoP1");
        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);

    } else if (pid == 0) {
        //Proceso hijo (p2)
        
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);

        // Esperar a recibir la ruta del proceso 1
        sem_wait(semaforoP2);

        char* ruta;
        read(tuberiaEnvio[0], &ruta, sizeof(ruta));

        // Verificar si se recibió la señal de salida
        char *memoriaCompartida;
        int areaCompartida = shm_open("/memoriaCompartida", O_RDWR, 0666);

        if (strcmp(ruta, "exit") == 0) {

            if (areaCompartida != -1) {

                // Enviar señal de salida al proceso 3

                memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
                
                sem_t *semaforoP3;
                semaforoP3 = sem_open("/semaforoP3", O_RDWR);
                sprintf(memoriaCompartida, "%s", "exit");

                sem_post(semaforoP3);
                sem_close(semaforoP3);

                munmap(memoriaCompartida, 4096);
            }

            //liberacion de recursos
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");
            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }

        // Verificar si la ruta es válida
        if (access(ruta, X_OK) != 0) {
            
            // Enviar mensaje a p1
            const char *mensaje = "No se encuentra el archivo a ejecutar\n";
            write(tuberiaRecepcion[1], mensaje, strlen(mensaje));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            // Enviar salida proceso p1
            const char *salidaM = "exit";
            write(tuberiaRecepcion[1], salidaM, strlen(salidaM));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            // Liberacion de recursos p3
            if (areaCompartida != -1) {
                memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
                
                sem_t *semaforoP3;
                semaforoP3 = sem_open("/semaforoP3", O_RDWR);
                sprintf(memoriaCompartida, "%s", "exit");

                sem_post(semaforoP3);
                sem_close(semaforoP3);

                munmap(memoriaCompartida, 4096);
            }

            // liberacion de recursos
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");
            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }
        
        // Comprobar proceso 3 en ejecución
        if (areaCompartida == -1) {

            // Enviar mensaje a p1
            const char *mensaje = "Proceso p3 no parece estar en ejecucion\n";
            write(tuberiaRecepcion[1], mensaje, strlen(mensaje));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);
            
            // Enviar salida proceso p1
            const char *salidaM = "exit";
            write(tuberiaRecepcion[1], salidaM, strlen(salidaM));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            // Liberacion de recursos
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");
            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }

        // Mapear memoria compartida
	    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);

        // Verificar si se mapeó correctamente
        if (memoriaCompartida == MAP_FAILED) {
            perror("Error al mapear memoria compartida");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);

            sem_close(semaforoP1);
            sem_close(semaforoP2);
            sem_unlink("/semaforoP1");
            sem_unlink("/semaforoP2");

            shm_unlink("/memoriaCompartida");

            return -1;
        }

        // Iniciar semáforo para sincronización
        sem_t *semaforoP3;
        semaforoP3 = sem_open("/semaforoP3", O_RDWR);
        
        // Esperar señal de proceso 3 para continuar
        sprintf(memoriaCompartida, "%s", ruta);
        sem_post(semaforoP3);
        sem_wait(semaforoP2);

        // Leer el resultado de ejecución desde la memoria compartida
        char buffer[4096];
        strcpy(buffer, memoriaCompartida);

        // Enviar mensaje a proceso 1
        write(tuberiaRecepcion[1], buffer, strlen(buffer));
        write(tuberiaRecepcion[1], "Procesos terminados\n", 21);
        sem_post(semaforoP1);
        sem_wait(semaforoP2);

        // Enviar salida proceso 1
        const char *salida = "exit";
        write(tuberiaRecepcion[1], salida, strlen(salida));
        sem_post(semaforoP1);
        sem_wait(semaforoP2);

        // liberacion de recursos p3
        sem_post(semaforoP3);
        
        // Liberar recursos
        sem_close(semaforoP1);
        sem_close(semaforoP2);
        sem_close(semaforoP3);
        sem_unlink("/semaforoP2");
        munmap(memoriaCompartida, 4096);
        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);
    }

    return 0;
}