#include <stdio.h>
#include <mqueue.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "servidor_handle.h"
#include "comm.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int mensaje_no_copiado = true;
pthread_cond_t cond_mensaje = PTHREAD_COND_INITIALIZER;
pthread_attr_t thread_attr;
int sd;

void send_result(int sc, int res) {
    char res_str[BUFFER_SIZE];
    sprintf(res_str, "%d", res);
    if (writeLine(sc, res_str) < 0) {
        perror("Error sending result\n");
        exit(0);
    }
}

void cleanup() {
    // Close server socket
    close(sd);

    // Destroy mutex, condition variable, and thread attributes
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_mensaje);
    pthread_attr_destroy(&thread_attr);

    // Exit or perform any additional cleanup
    exit(EXIT_SUCCESS);
}

void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nReceived CTRL+C. Cleaning up...\n");
        cleanup();
    }
}

void *tratar_peticion(void *sc_ptr) {
    pthread_mutex_lock(&mutex);
    int sc = *(int *) sc_ptr;
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_mutex_unlock(&mutex);
    ssize_t ret;

    printf("Handling petition\n");
    printf("socket: %d\n", sc);
    char operation[BUFFER_SIZE];
    ret = readLine(sc, operation, BUFFER_SIZE);
    if (ret < 0) {
        printf("Error en recepción op\n");
        //return -1;
        exit(0);
    }
    printf("La operación es: %s\n",operation);
    int res = 0;

    if (strcmp(operation, "REGISTER") == 0) {
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción REGISTER\n");
            send_result(sc, 2);
            exit(0);
        }
        res = handle_register(username);
    }
    else if(strcmp(operation,"UNREGISTER") == 0){
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción UNREGISTER\n");
            send_result(sc, 2);
            exit(0);
        }
        res = handle_unregister(username);
    }


    /*
    char key[BUFFER_SIZE];
    int key_int;
    char value1[BUFFER_SIZE];
    char N_value2[BUFFER_SIZE];
    int N_value2_int;
    char element[BUFFER_SIZE];
    double V_value2[MAX_VALUE2_LENGTH];

    char get_value1[BUFFER_SIZE];
    int get_N_value2;
    double get_V_value2[MAX_VALUE2_LENGTH];
    switch (operation_int) {
        case INIT:
            res = handle_init();
            break;
        case SET_VALUE:
            if (readLine(sc, key, BUFFER_SIZE) < 0) {
                printf("Error en recepción key\n");
                //return -1;
                exit(0);
            }
            key_int = atoi(key);
            if (readLine(sc, value1, BUFFER_SIZE) < 0) {
                printf("Error en recepción Value 1\n");
                //return -1;
                exit(0);
            }
            if (readLine(sc, N_value2, BUFFER_SIZE) < 0) {
                printf("Error en recepción N_value2\n");
                //return -1;
                exit(0);
            }
            N_value2_int = atoi(N_value2);
            for (int i = 0; i < N_value2_int; i++) {
                if (readLine(sc, element, BUFFER_SIZE) < 0) {
                    printf("Error en recepción V_value2\n");
                    //return -1;
                    exit(0);
                }
                V_value2[i] = strtod(element, NULL);
            }
            res = handle_set_value(key_int, value1, N_value2_int, V_value2);
            break;
        case GET_VALUE:
            if (readLine(sc, key, BUFFER_SIZE) < 0) {
                printf("Error en recepción key\n");
                //return -1;
                exit(0);
            }
            key_int = atoi(key);
            res = handle_get_value(key_int, get_value1, &get_N_value2, get_V_value2);
            break;
        default:
            fprintf(stderr, "Invalid operation\n");
            break;
        */
    //Le mandamos el resultado al cliente.
    send_result(sc, res);

    /*
    if (operation_int == GET_VALUE){
        //Mandar también los datos a recuperar
        if (writeLine(sc, get_value1) < 0) {
            perror("Error sending GET_VALUE message\n");
            exit(0);
        }
        char N_value2_str[BUFFER_SIZE];
        sprintf(N_value2_str,"%d",get_N_value2);
        if (writeLine(sc, N_value2_str) < 0) {
            perror("Error sending GET_VALUE message\n");
            exit(0);
        }
        for (int i = 0; i < get_N_value2; i++) {
            char element_str[BUFFER_SIZE];
            sprintf(element_str,"%f",get_V_value2[i]);
            if (writeLine(sc, element_str) < 0) {
                perror("Error sending GET_VALUE message\n");
                exit(0);
            }
        }
    }
     */
    // close socket
    close(sc);
    pthread_exit(0);
}






int main(int argc, char* argv[]) {
    if (argc != 2){
        perror("usage ./servidor <port>");
    }

    pthread_t thid;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_mensaje, NULL);

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("Error setting up signal handler");
        return EXIT_FAILURE;
    }


    int sc;
    int port = atoi(argv[1]);
    sd = serverSocket(INADDR_ANY, port,SOCK_STREAM);
    if (sd < 0) {
        printf ("SERVER: Error en serverSocket\n");
        return 0;
    }

    // Crear directorio si no existe
    if (mkdir(DATA_DIRECTORY, 0777) == -1) {
        if (errno != EEXIST) {
            perror("Error creating data directory");
            return -1;
        }
    }

    while (1)
    {
        // aceptar cliente
        if ((sc = serverAccept(sd)) < 0) {
            printf("Error en serverAccept\n");
            continue ;
        }

        // procesar petición
        if (pthread_create(&thid, &thread_attr,  tratar_peticion, &sc) != 0){
            perror("Error creating thread");
            return -1;
        }
        pthread_mutex_lock(&mutex);
        while (mensaje_no_copiado) {
            pthread_cond_wait(&cond_mensaje, &mutex);
        }
        mensaje_no_copiado = true;
        pthread_mutex_unlock(&mutex);
    }
    close(sd);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_mensaje);
    pthread_attr_destroy(&thread_attr);

    return 0;
}


// HANDLERS
