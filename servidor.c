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
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
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

int get_client_address(int sc, char *address, size_t addr_size) {
    // Get the address of the client
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(sc, (struct sockaddr *) &addr, &addr_len) == -1) {
        perror("Error getting client address");
        return -1;
    }
    if (inet_ntop(AF_INET, &addr.sin_addr, address, addr_size) == NULL) {
        perror("Error converting address to string");
        return -1;
    }
    return 0;
}


void cleanup() {
    // Close server socket
    close(sd);

    // Destroy mutex, condition variable, and thread attributes
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&file_mutex);
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
    int n_users = 0;
    int n_content = 0;
    int client_port;
    char client_address[INET_ADDRSTRLEN];

    char operation[BUFFER_SIZE];
    ret = readLine(sc, operation, BUFFER_SIZE);
    if (ret < 0) {
        printf("Error en recepción op\n");
        //return -1;
        pthread_exit(NULL);
    }
    printf("s > %s FROM USER\n", operation);
    int res = 0;

    pthread_mutex_lock(&file_mutex);
    if (strcmp(operation, "REGISTER") == 0) {
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción REGISTER\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_register(username);
    } else if (strcmp(operation, "UNREGISTER") == 0) {
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción UNREGISTER\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_unregister(username);
    } else if (strcmp(operation, "CONNECT") == 0) {
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción CONNECT\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        char port_str[BUFFER_SIZE];
        if (readLine(sc, port_str, BUFFER_SIZE) < 0) {
            printf("Error en recepción CONNECT\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        char client_address[INET_ADDRSTRLEN];
        if (get_client_address(sc, client_address, sizeof(client_address)) < 0) {
            send_result(sc, 3);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_connect(username, atoi(port_str), client_address);
    } else if (strcmp(operation, "PUBLISH") == 0) {
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción PUBLISH\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        char fileName[BUFFER_SIZE];
        if (readLine(sc, fileName, BUFFER_SIZE) < 0) {
            printf("Error en recepción PUBLISH\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        char description[BUFFER_SIZE];
        if (readLine(sc, description, BUFFER_SIZE) < 0) {
            printf("Error en recepción PUBLISH\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_publish(username, fileName, description);

    } else if (strcmp(operation, "DELETE") == 0) {
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción PUBLISH\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        char fileName[BUFFER_SIZE];
        if (readLine(sc, fileName, BUFFER_SIZE) < 0) {
            printf("Error en recepción PUBLISH\n");
            send_result(sc, 2);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_delete(username, fileName);
    } else if (strcmp(operation, "LIST_USERS") == 0) {
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción LIST_USERS\n");
            send_result(sc, 3);
            close(sc);
            pthread_exit(NULL);
        }

        FILE *user_list = fopen("users_connected.txt", "w");
        if (user_list == NULL) {
            perror("Error opening file");
            send_result(sc, 3);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_list_users(username, &n_users, user_list);
        fclose(user_list);
    } else if (strcmp(operation, "LIST_CONTENT") == 0){
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción LIST_USERS\n");
            send_result(sc, 4);
            close(sc);
            pthread_exit(NULL);
        }

        char owner[BUFFER_SIZE];
        if (readLine(sc, owner, BUFFER_SIZE) < 0) {
            printf("Error en recepción LIST_USERS\n");
            send_result(sc, 4);
            close(sc);
            pthread_exit(NULL);
        }

        FILE *user_content = fopen("user_content.txt", "w");
        if (user_content == NULL) {
            perror("Error opening file");
            send_result(sc, 4);
            close(sc);
            pthread_exit(NULL);
        }

        res = handle_list_content(username, owner, &n_content, user_content);
        fclose(user_content);
    } else if (strcmp(operation, "DISCONNECT") == 0){
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción DISCONNECT\n");
            send_result(sc, 3);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_disconnect(username);
    } else if (strcmp(operation, "GET_FILE") == 0){
        char username[BUFFER_SIZE];
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción GET_FILE\n");
            send_result(sc, 3);
            close(sc);
            pthread_exit(NULL);
        }
        char owner[BUFFER_SIZE];
        if (readLine(sc, owner, BUFFER_SIZE) < 0) {
            printf("Error en recepción GET_FILE\n");
            send_result(sc, 3);
            close(sc);
            pthread_exit(NULL);
        }
        res = handle_get_file(username, owner, client_address, &client_port);
    } else {
        printf("Operación no reconocida\n");
        res = 1;
    }
    pthread_mutex_unlock(&file_mutex);


    send_result(sc, res);
    if (strcmp(operation, "LIST_USERS") == 0){
        printf("Socket: %d\n", sc);
        send_result(sc, n_users);

        pthread_mutex_lock(&file_mutex);
        FILE *user_list = fopen("users_connected.txt", "r");
        printf("Sending users connected\n");
        if (user_list == NULL) {
            perror("Error opening file");
            close(sc);
            pthread_exit(NULL);
        }
        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), user_list)) {
            printf("Sending: %s\n", line);
            if (writeLine(sc, line) < 0) {
                perror("Error sending result\n");
                pthread_exit(NULL);
            }
        }
        fclose(user_list);
        pthread_mutex_unlock(&file_mutex);
        // Delete file
        if (remove("users_connected.txt") != 0) {
            perror("Error deleting file");
            close(sc);
            pthread_exit(NULL);
        }
    }
    else if (strcmp(operation, "LIST_CONTENT") == 0){
        printf("Socket: %d\n", sc);
        send_result(sc, n_content);

        pthread_mutex_lock(&file_mutex);
        FILE *user_content = fopen("user_content.txt", "r");
        printf("Sending user content\n");
        if (user_content == NULL) {
            perror("Error opening file");
            close(sc);
            pthread_exit(NULL);
        }
        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), user_content)) {
            printf("Sending: %s\n", line);
            if (writeLine(sc, line) < 0) {
                perror("Error sending result\n");
                pthread_exit(NULL);
            }
        }
        fclose(user_content);
        pthread_mutex_unlock(&file_mutex);
        // Delete file
        if (remove("user_content.txt") != 0) {
            perror("Error deleting file");
            close(sc);
            pthread_exit(NULL);
        }
    }
    else if (strcmp(operation, "GET_FILE") == 0){
        // Send client address and port
        printf("Sending client address and port\n");
        printf("Address: %s\n", client_address);
        if (writeLine(sc, client_address) < 0) {
            perror("Error sending result\n");
            send_result(sc, 2);
            pthread_exit(NULL);
        }
        printf("Port: %d\n", client_port);
        char port_str[BUFFER_SIZE];
        sprintf(port_str, "%d", client_port);
        if (writeLine(sc, port_str) < 0) {
            perror("Error sending result\n");
            send_result(sc, 2);
            pthread_exit(NULL);
        }
    }

    close(sc);
    pthread_exit(0);
}


int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        printf("Usage: ./server -p <port>\n");
        return EXIT_FAILURE;
    }

    pthread_t thid;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    pthread_cond_init(&cond_mensaje, NULL);

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("Error setting up signal handler");
        return EXIT_FAILURE;
    }


    int sc;
    int port = atoi(argv[2]);
    sd = serverSocket(INADDR_ANY, port, SOCK_STREAM);
    if (sd < 0) {
        printf("SERVER: Error en serverSocket\n");
        return 0;
    }

    // Crear directorio si no existe
    if (mkdir(DATA_DIRECTORY, 0777) == -1) {
        if (errno != EEXIST) {
            perror("Error creating data directory");
            return -1;
        }
    }

    printf("s > init server: %s:%d\n", "localhost", port);

    while (1) {
        // aceptar cliente
        if ((sc = serverAccept(sd)) < 0) {
            printf("Error accepting client connection\n");
            continue;
        }

        // procesar petición
        if (pthread_create(&thid, &thread_attr, tratar_peticion, &sc) != 0) {
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
    pthread_mutex_destroy(&file_mutex);
    pthread_cond_destroy(&cond_mensaje);
    pthread_attr_destroy(&thread_attr);

    return 0;
}


// HANDLERS
