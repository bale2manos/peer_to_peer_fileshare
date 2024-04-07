//
// Created by bale2 on 01/03/2024.
//

#ifndef EX1_SERVIDOR_HANDLE_H
#include <stdio.h>
#define EX1_SERVIDOR_HANDLE_H
#define DATA_DIRECTORY "./database/"
#define BUFFER_SIZE 256
#define MAX_FILEPATH_LENGTH 256

int handle_register(char * username);

int handle_unregister(char * username);

void remove_files_in_directory(char* dirpath);

int handle_connect(char* username, int port, char* address);

int write_connection(const char *address, int port, FILE *file);

int handle_publish(char* userName, char* fileName, char* description);

int handle_delete(char* username, char* filename);

int handle_list_users(char* username, int* n_connections_str, FILE* user_list);

int handle_set_value(int key, char* value1, int N_value2, double* V_value2);

int handle_get_value(int key, char* value1, int* N_value2, double* V_value2);

int handle_modify_value(int key, char* value1, int N_value2, double* V_value2);

int handle_delete_key(int key);

int handle_exist(int key);

#endif //EX1_SERVIDOR_HANDLE_H
