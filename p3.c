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

    // Crear memoria compartida para comunicación
    shm_unlink("/memoriaCompartida");
    char *memoriaCompartida;
    int areaCompartida = shm_open("/memoriaCompartida", O_CREAT | O_RDWR, 0666);
    if (areaCompartida == -1) {
        perror("Error al crear memoria compartida");
        return -1;
    }
    if (ftruncate(areaCompartida, 4096) == -1) {
        perror("Error al truncar memoria compartida");
        return -1;
    }
    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
    if (memoriaCompartida == MAP_FAILED) {
        perror("Error al mapear memoria compartida");
        shm_unlink("/memoriaCompartida");
        return -1;
    }

    // creacion de semaforoP3
    sem_unlink("/semaforoP3");
    sem_t *semaforoP3;
    semaforoP3 = sem_open("/semaforoP3", O_CREAT, 0666, 0);
    if (semaforoP3 == SEM_FAILED) {
        perror("Error al abrir semáforo");
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");
        sem_unlink("/semaforoP3");
        return -1;
    }

    printf("Esperando...\n");
    // Esperar a que el proceso 2
    sem_wait(semaforoP3);

    // Leer ruta de programa a ejecutar
    char ruta[4096];
    strcpy(ruta, memoriaCompartida);

    // Salir si se recibe "exit"
    if (strcmp(ruta, "exit") == 0) {

        // Liberar recursos
        sem_close(semaforoP3);
        sem_unlink("/semaforoP3");

        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");

        return 0;
    }
    printf("Leyendo de la memoria...\n");
    printf("Leido %s\n", ruta);

    // Conectar con el semáforo del proceso 2
    sem_t *semaforoP2;
    semaforoP2 = sem_open("/semaforoP2", O_RDWR);
    if (semaforoP2 == SEM_FAILED) {
        perror("Error al abrir semáforo");
        sem_close(semaforoP3);
        sem_unlink("/semaforoP3");
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");
        return -1;
    }

    // Crear tubería para redirigir salida de programa a ejecutar
    int tuberia[2];
    if (pipe(tuberia) == -1) {
        perror("Error al crear tubería");
        sem_close(semaforoP3);
        sem_close(semaforoP2);
        sem_unlink("/semaforoP3");
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");
        return -1;
    }

    pid_t pid = fork();

    // Proceso hijo
    if (pid == 0) {
        close(tuberia[1]);

        // Leer mensaje de la tubería
        int buffer[255];
        read(tuberia[0], buffer, sizeof(buffer));
        char *mensaje = (char *)buffer;

        printf("Ejecutando %s\n", ruta);

        // Escribir mensaje en memoria compartida
        sprintf(memoriaCompartida, "%s", mensaje);
        sem_post(semaforoP2);

        printf("Proceso terminado\n");    
        // Esperar señal de proceso 2 para liberar recursos
        sem_wait(semaforoP3);
        
        // Liberacion de recursos
        sem_close(semaforoP2);
        sem_close(semaforoP3);
        sem_unlink("/semaforoP3");
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");
        close(tuberia[0]);
        close(tuberia[1]);
    } else {
        // Proceso padre
        close(tuberia[0]);
        sem_close(semaforoP2);
        sem_close(semaforoP3);

        // Control de errores
        if (dup2(tuberia[1], STDOUT_FILENO) == -1) {
            perror("Error al redirigir salida");
            return -1;
        }
        if (execl(ruta, ruta, (char *) NULL) == -1) {
            perror("Error al ejecutar el comando");
            close(tuberia[0]);
            close(tuberia[1]);
            return -1;
        }
    }
    return 0;
}