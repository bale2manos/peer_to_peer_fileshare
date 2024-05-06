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
#include "servidor_rpc.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
int mensaje_no_copiado = true;
pthread_cond_t cond_mensaje = PTHREAD_COND_INITIALIZER;
pthread_attr_t thread_attr;
int sd;


void print_rpc_servidor(char *string_to_print) {
    CLIENT *clnt;
    enum clnt_stat retval_1;
    int result_1;

    char *host = "localhost";

    clnt = clnt_create(host, SERVIDOR_RPC, VERSION_RPC, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }

    retval_1 = print_rpc_1(string_to_print, &result_1, clnt);
    if (retval_1 != RPC_SUCCESS) {
        clnt_perror(clnt, "call failed");
    }
    clnt_destroy(clnt);
}


void send_info_to_rpc(char *username, char *operation, char *c_time_string) {
    char rpc_string[2 * BUFFER_SIZE + 103];
    sprintf(rpc_string, "%s %s %s", username, operation, c_time_string);
    print_rpc_servidor(rpc_string);
}

void send_result(int sc, int res) {
    char res_str[BUFFER_SIZE];
    sprintf(res_str, "%d", res);
    if (writeLine(sc, res_str) < 0) {
        printf("ERROR ERROR ERROR\n");
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

void exit_error(int socket, char *message) {
    perror(message);
    close(socket);
    printf("s > ");
    fflush(stdout);
    pthread_exit(NULL);
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
    char username[BUFFER_SIZE];

    char operation[BUFFER_SIZE];
    ret = readLine(sc, operation, BUFFER_SIZE);
    if (ret < 0) {
        printf("Error en recepción op\n");
        //return -1;
        printf("s > ");
        fflush(stdout);
        pthread_exit(NULL);
    }
    int res = 0;
    char c_time_string[100];
    char owner[BUFFER_SIZE];
    if (strcmp(operation, "GET_FILE") != 0) {

        ret = readLine(sc, c_time_string, 100);
        if (ret < 0) {
            exit_error(sc, "Error receiving c_time_string");
        }
    }

    if (strcmp(operation, "REGISTER") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);
        if (strcmp(username, "__NO_USER__") == 0) {
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }
        pthread_mutex_lock(&file_mutex);
        res = handle_register(username);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "UNREGISTER") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);
        if (strcmp(username, "__NO_USER__") == 0) {
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }
        pthread_mutex_lock(&file_mutex);
        res = handle_unregister(username);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "CONNECT") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);
        if (strcmp(username, "__NO_USER__") == 0) {
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }

        char port_str[BUFFER_SIZE];
        if (readLine(sc, port_str, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving port");
        }
        char client_address[INET_ADDRSTRLEN];
        if (get_client_address(sc, client_address, sizeof(client_address)) < 0) {
            send_result(sc, 3);
            exit_error(sc, "Error getting client address");
        }
        pthread_mutex_lock(&file_mutex);
        res = handle_connect(username, atoi(port_str), client_address);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "PUBLISH") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving username") ;
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);
        if (strcmp(username, "__NO_USER__") == 0) {
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }

        char fileName[BUFFER_SIZE];
        if (readLine(sc, fileName, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving filename");
        }
        char description[BUFFER_SIZE];
        if (readLine(sc, description, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving description");
        }
        pthread_mutex_lock(&file_mutex);
        res = handle_publish(username, fileName, description);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "DELETE") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);
        if (strcmp(username, "__NO_USER__") == 0) {
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }

        char fileName[BUFFER_SIZE];
        if (readLine(sc, fileName, BUFFER_SIZE) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error receiving filename");
        }
        pthread_mutex_lock(&file_mutex);
        res = handle_delete(username, fileName);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "LIST_USERS") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            send_result(sc, 3);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);

        if (strcmp(username, "__NO_USER__") == 0) {
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }

        pthread_mutex_lock(&file_mutex);
        char user_list_name[MAX_FILEPATH_LENGTH * 3];
        sprintf(user_list_name, "%s_users_connected.txt", username);
        FILE *user_list = fopen(user_list_name, "w");
        if (user_list == NULL) {
            send_result(sc, 3);
            exit_error(sc, "Error opening file");
        }
        res = handle_list_users(username, &n_users, user_list);
        fclose(user_list);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "LIST_CONTENT") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            send_result(sc, 4);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);

        if (strcmp(username, "__NO_USER__") == 0) {
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }

        if (readLine(sc, owner, BUFFER_SIZE) < 0) {
            send_result(sc, 4);
            exit_error(sc, "Error receiving owner");
        }

        pthread_mutex_lock(&file_mutex);
        char user_content_name[MAX_FILEPATH_LENGTH * 3];
        sprintf(user_content_name, "%s%s_content.txt", username, owner);
        FILE *user_content = fopen(user_content_name, "w");
        if (user_content == NULL) {
            send_result(sc, 4);
            exit_error(sc, "Error opening file");
        }
        res = handle_list_content(username, owner, &n_content, user_content);
        fclose(user_content);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "DISCONNECT") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción DISCONNECT\n");
            send_result(sc, 3);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        send_info_to_rpc(username, operation, c_time_string);

        if (strcmp(username, "__NO_USER__") == 0) {
            perror("Error: user does not exist");
            send_result(sc, 1);
            close(sc);
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&file_mutex);
        res = handle_disconnect(username);
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "GET_FILE") == 0) {
        if (readLine(sc, username, BUFFER_SIZE) < 0) {
            printf("Error en recepción GET_FILE\n");
            send_result(sc, 3);
            exit_error(sc, "Error receiving username");
        }
        printf("%s FROM %s\n", operation, username);
        if (strcmp(username, "__NO_USER__") == 0) {
            perror("Error: user does not exist");
            send_result(sc, 1);
            exit_error(sc, "Error: user does not exist");
        }
        char owner[BUFFER_SIZE];
        if (readLine(sc, owner, BUFFER_SIZE) < 0) {
            send_result(sc, 3);
            exit_error(sc, "Error receiving owner");
        }

        pthread_mutex_lock(&file_mutex);
        res = handle_get_file(username, owner, client_address, &client_port);
        pthread_mutex_unlock(&file_mutex);
    } else {
        printf("Operación %s no reconocida\n", operation);
        res = 1;
    }


    send_result(sc, res);
    if (res != 0) {
        close(sc);
        printf("s > ");
        fflush(stdout);
        pthread_exit(NULL);
    }

    if (strcmp(operation, "LIST_USERS") == 0) {
        send_result(sc, n_users);

        pthread_mutex_lock(&file_mutex);
        char users_connected_path[MAX_FILEPATH_LENGTH * 2];
        sprintf(users_connected_path, "%s_users_connected.txt", username);
        if (access(users_connected_path, F_OK) == -1) {
            pthread_mutex_unlock(&file_mutex);
            exit_error(sc, "Error: file user_connected does not exist");
        }


        FILE *user_list = fopen(users_connected_path, "r"); // TODO change name file to avoid race condition
        if (user_list == NULL) {
            pthread_mutex_unlock(&file_mutex);
            exit_error(sc, "Error opening file users_connected");
        }
        // If file is empty, send empty message
        fseek(user_list, 0, SEEK_END);
        if (ftell(user_list) == 0) {
            if (writeLine(sc, "---------------") < 0) {
                pthread_mutex_unlock(&file_mutex);
                exit_error(sc, "Error sending result");
            }
            fclose(user_list);
            pthread_mutex_unlock(&file_mutex);
            close(sc);
            printf("s > ");
            fflush(stdout);
            pthread_exit(NULL);
        }

        // Devolver el puntero al inicio del archivo
        fseek(user_list, 0, SEEK_SET);

        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), user_list)) {

            if (writeLine(sc, line) < 0) {
                pthread_mutex_unlock(&file_mutex);
                exit_error(sc, "Error sending result");
            }
        }
        fclose(user_list);

        // Delete file
        if (remove(users_connected_path) != 0) {
            pthread_mutex_unlock(&file_mutex);
            exit_error(sc, "Error deleting file");
        }
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "LIST_CONTENT") == 0) {
        send_result(sc, n_content);
        pthread_mutex_lock(&file_mutex);

        char user_content_name[MAX_FILEPATH_LENGTH * 3];
        sprintf(user_content_name, "%s%s_content.txt", username, owner);
        FILE *user_content = fopen(user_content_name, "r");
        if (user_content == NULL) {
            pthread_mutex_unlock(&file_mutex);
            exit_error(sc, "Error opening file");
        }

        // If file is empty, send empty message
        fseek(user_content, 0, SEEK_END);
        if (ftell(user_content) == 0) {
            if (writeLine(sc, "---------------") < 0) {
                pthread_mutex_unlock(&file_mutex);
                exit_error(sc, "Error sending result");
            }
            fclose(user_content);
            pthread_mutex_unlock(&file_mutex);
            exit_error(sc, "Error: file is empty");
        }
        // Devolver el puntero al inicio del archivo
        fseek(user_content, 0, SEEK_SET);

        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), user_content)) {
            if (writeLine(sc, line) < 0) {
                pthread_mutex_unlock(&file_mutex);
                exit_error(sc, "Error sending result");
            }
        }
        fclose(user_content);

        // Delete file
        if (remove(user_content_name) != 0) {
            pthread_mutex_unlock(&file_mutex);
            exit_error(sc, "Error deleting file");
        }
        pthread_mutex_unlock(&file_mutex);
    } else if (strcmp(operation, "GET_FILE") == 0) {
        // Send client address and port

        if (writeLine(sc, client_address) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error sending result");
        }
        char port_str[BUFFER_SIZE];
        sprintf(port_str, "%d", client_port);
        if (writeLine(sc, port_str) < 0) {
            send_result(sc, 2);
            exit_error(sc, "Error sending result");
        }
    }

    close(sc);
    printf("s > ");
    fflush(stdout);
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
    printf("s > ");

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
