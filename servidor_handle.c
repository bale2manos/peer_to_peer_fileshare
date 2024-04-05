//
// Created by bale2 on 01/03/2024.
//

#include "servidor_handle.h"
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#define REGISTER 0


int handle_register(char* username) {
    char dirpath[MAX_FILEPATH_LENGTH];
    snprintf(dirpath, sizeof(dirpath), "%s%s", DATA_DIRECTORY, username);

    // Check if the directory already exists
    if (access(dirpath, F_OK) != -1) {
        perror("Error: username already exists");
        return 1;
    }

    // Create the directory
    if (mkdir(dirpath, 0700) == -1) {
        perror("Error creating directory");
        return 2;
    }

    printf("User registered successfully.\n");

    return 0;
}


int handle_unregister(char* username) {
    char dirpath[MAX_FILEPATH_LENGTH];
    snprintf(dirpath, sizeof(dirpath), "%s%s", DATA_DIRECTORY, username);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(dirpath, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }

    // Delete directory
    if (rmdir(dirpath) != 0) {
        perror("Error unregistering user");
        return 2;
    }
    printf("User unregistered successfully.\n");

    return 0;
}

/*
int handle_set_value(int key, char* value1, int N_value2, double* V_value2) {
    printf("Setting value for key %d\n", key);

    char filepath[MAX_VALUE1_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%d", DATA_DIRECTORY, key);

    // If the file already exists, return an error, as the key already exists
    if (access(filepath, F_OK) != -1) {
        perror("Error: key already exists");
        return -1;
    }

    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fprintf(file, "%s\n", value1);
    fprintf(file, "%d\n", N_value2);
    for (int i = 0; i < N_value2; i++) {
        fprintf(file, "%f\n", V_value2[i]);
    }
    printf("Set value finalizado\n");
    fclose(file);
    return 0;
}

int handle_get_value(int key, char* value1, int *N_value2, double *V_value2) {
    printf("Getting value for key %d\n", key);

    char filepath[MAX_VALUE1_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%d", DATA_DIRECTORY, key);

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    if (fscanf(file, "%s\n", value1) != 1) {
        perror("Error reading value1");
        return -1;
    }

    if (fscanf(file, "%d\n", N_value2) != 1) {
        perror("Error reading N_value2");
        return -1;
    }
    for (int i = 0; i < *N_value2; i++) {
        if (fscanf(file, "%lf\n", &V_value2[i]) != 1) {
            perror("Error reading V_value2");
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int handle_modify_value(int key, char* value1, int N_value2, double* V_value2){
    // TODO aqui pasar punteros o los valores?
    printf("Setting value for key %d\n", key);

    char filepath[MAX_VALUE1_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%d", DATA_DIRECTORY, key);

    if (access(filepath, F_OK) == -1) {
        perror("Error: file does not exist");
        return -1;
    }

    // Open and truncate the file
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fprintf(file, "%s\n", value1);
    fprintf(file, "%d\n", N_value2);
    for (int i = 0; i < N_value2; i++) {
        fprintf(file, "%f\n", V_value2[i]);
    }

    fclose(file);
    return 0;
}

int handle_delete_key(int key){
    printf("Deleting tuple from key %d\n",key);
    char filepath[MAX_VALUE1_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%d", DATA_DIRECTORY, key);

    if (access(filepath, F_OK) == -1) {
        perror("Error: file does not exist");
        return -1;
    }
    if (remove(filepath) != 0) {
        perror("Error deleting file");
        return -1;
    } else {
        printf("File deleted successfully.\n");
    }
    return 0;
}

int handle_exist(int key) {
    printf("Checking if key %d exists\n", key);

    char filepath[MAX_VALUE1_LENGTH + sizeof(DATA_DIRECTORY)];
    snprintf(filepath, sizeof(filepath), "%s%d", DATA_DIRECTORY, key);

    if (access(filepath, F_OK) == 0) {
        // File exists
        return 1;
    } else {
        // Check for errors
        if (errno == ENOENT) {
            // File doesn't exist
            return 0;
        } else {
            // Other errors
            perror("Error checking file existence");
            return -1;
        }
    }
}
 */