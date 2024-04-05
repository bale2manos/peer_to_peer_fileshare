//
// Created by bale2 on 01/03/2024.
//

#ifndef EX1_SERVIDOR_HANDLE_H
#define EX1_SERVIDOR_HANDLE_H
#define DATA_DIRECTORY "./database/"
#define BUFFER_SIZE 1024
#define MAX_FILEPATH_LENGTH 256

int handle_register(char * username);

int handle_unregister(char * username);

int handle_set_value(int key, char* value1, int N_value2, double* V_value2);

int handle_get_value(int key, char* value1, int* N_value2, double* V_value2);

int handle_modify_value(int key, char* value1, int N_value2, double* V_value2);

int handle_delete_key(int key);

int handle_exist(int key);

#endif //EX1_SERVIDOR_HANDLE_H
