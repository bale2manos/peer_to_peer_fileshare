 //
// Created by bale2 on 01/03/2024.
//

#include "servidor_handle.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
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

 void remove_files_in_directory(char* dirpath) {
     struct dirent *entry;
     DIR *dir = opendir(dirpath);
     if (dir == NULL) {
         perror("Error opening directory");
         return;
     }

     size_t dirpath_len = strlen(dirpath);
     size_t max_filepath_len = dirpath_len + NAME_MAX + 2;
     char *filepath = (char*)malloc(max_filepath_len);
     if (filepath == NULL) {
         perror("Memory allocation failed");
         closedir(dir);
         return;
     }

     while ((entry = readdir(dir)) != NULL) {
         size_t filename_len = strlen(entry->d_name);
         if (dirpath_len + filename_len + 2 > max_filepath_len) {
             fprintf(stderr, "Error: File path too long\n");
             continue; // Skip this file
         }
         snprintf(filepath, max_filepath_len, "%s/%s", dirpath, entry->d_name);
         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
             continue; // Skip . and ..
         }
         if (remove(filepath) != 0) {
             printf("Error removing file %s\n", filepath);
             perror("Error removing file");
         }
     }

     free(filepath);
     closedir(dir);
 }


int handle_unregister(char* username) {
    char dirpath[MAX_FILEPATH_LENGTH];
    snprintf(dirpath, sizeof(dirpath), "%s%s", DATA_DIRECTORY, username);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(dirpath, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }

    // Check if the directory is empty and if not remove all the files inside it
    remove_files_in_directory(dirpath);

    // Delete directory
    if (rmdir(dirpath) != 0) {
        perror("Error unregistering user");
        return 2;
    }
    printf("User unregistered successfully.\n");

    return 0;
}

int handle_connect(char* username, int port, char* address) {
    char user[MAX_FILEPATH_LENGTH];
    snprintf(user, sizeof(user), "%s%s", DATA_DIRECTORY, username);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(user, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }

    char user_connect[MAX_FILEPATH_LENGTH];
    snprintf(user_connect, sizeof(user_connect), "%s%s/connect", DATA_DIRECTORY, username);
    // If the file does exist, return an error, as the user is already connected
    if (access(user_connect, F_OK) == 0) {
        perror("Error: user already connected");
        return 2;
    }

    // Create the file of connection
    FILE *file = fopen(user_connect, "w");
    if (file == NULL) {
        perror("Error opening file");
        return 3;
    }

    // Create a JSON to store the fileNames and descriptions
    char user_json[MAX_FILEPATH_LENGTH];
    snprintf(user_json, sizeof(user_json), "%s%s/files.json", DATA_DIRECTORY, username);
    FILE *file_json = fopen(user_json, "w");
    if (file_json == NULL) {
        perror("Error opening file");
        return 3;
    }

    fprintf(file_json, "{\n");
    fprintf(file_json, "    \"files\": [\n");
    fprintf(file_json, "    ]\n");
    fprintf(file_json, "}\n");

    fclose(file_json);



    // Write the address to the file
    if (write_connection(address, port, file) != 0) {
        return 3;
    }

    return 0;
}

 int write_connection(const char *address, int port, FILE *file) {
     if (file == NULL) {
         perror("Error opening file");
         return 1;
     }

     if (fprintf(file, "%s\n", address) < 0) {
         perror("Error writing address");
         fclose(file);
         return 2;
     }

     if (fprintf(file, "%d\n", port) < 0) {
         perror("Error writing port");
         fclose(file);
         return 3;
     }

     fclose(file);
     printf("User connected successfully.\n");
     return 0;
 }

int handle_publish(char* userName, char* fileName, char* description){
    char user[MAX_FILEPATH_LENGTH];
    snprintf(user, sizeof(user), "%s%s", DATA_DIRECTORY, userName);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(user, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }

    char user_connect[MAX_FILEPATH_LENGTH];
    snprintf(user_connect, sizeof(user_connect), "%s%s/connect", DATA_DIRECTORY, userName);

    // If the file does not exist, return an error, as the user is not connected
    if (access(user_connect, F_OK) == -1) {
        perror("Error: user is not connected");
        return 2;
    }

    // Append the new file entry to the JSON
    int res = appendToFileJson(userName, fileName, description);
    if ( res == 1) {
        return 3;
    }
    else if (res != 0) {
        return 4;
    }
    printf("File published successfully.\n");
    return 0;
}

#include <stdbool.h>

 int appendToFileJson(const char *userName, const char *fileName, const char *description) {
     char user_json[MAX_FILEPATH_LENGTH];
     snprintf(user_json, sizeof(user_json), "%s%s/files.json", DATA_DIRECTORY, userName);
     FILE *file_json = fopen(user_json, "r+");
     if (file_json == NULL) {
         perror("Error opening json file");
         return 4;
     }

     // Read existing content from the file
     fseek(file_json, 0, SEEK_END);
     long fileSize = ftell(file_json);
     fseek(file_json, 0, SEEK_SET);
     char *fileContent = (char *)malloc(fileSize + 1);
     fread(fileContent, 1, fileSize, file_json);
     fclose(file_json);

     // Parse JSON
     cJSON *root = cJSON_Parse(fileContent);
     if (root == NULL) {
         printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
         free(fileContent);
         return 5;
     }

     // Get "files" array
     cJSON *filesArray = cJSON_GetObjectItem(root, "files");
     if (!cJSON_IsArray(filesArray)) {
         printf("Error: 'files' is not an array.\n");
         cJSON_Delete(root);
         free(fileContent);
         return 6;
     }

     // Check if the filename already exists
     bool exists = false;
     cJSON *fileItem = NULL;
     cJSON_ArrayForEach(fileItem, filesArray) {
         cJSON *fileNameItem = cJSON_GetObjectItem(fileItem, "fileName");
         if (fileNameItem && cJSON_IsString(fileNameItem) && strcmp(fileNameItem->valuestring, fileName) == 0) {
             exists = true;
             break;
         }
     }

     // If filename already exists, return 15
     if (exists) {
         cJSON_Delete(root);
         free(fileContent);
         return 1;
     }

     // Create new JSON object for the new file entry
     cJSON *newFile = cJSON_CreateObject();
     cJSON_AddStringToObject(newFile, "fileName", fileName);
     cJSON_AddStringToObject(newFile, "description", description);

     // Add the new file entry to the "files" array
     cJSON_AddItemToArray(filesArray, newFile);

     // Write the updated JSON back to the file
     file_json = fopen(user_json, "w");
     if (file_json == NULL) {
         perror("Error opening json file");
         cJSON_Delete(root);
         free(fileContent);
         return 7;
     }
     char *updatedContent = cJSON_Print(root);
     fprintf(file_json, "%s", updatedContent);

     // Cleanup
     fclose(file_json);
     cJSON_Delete(root);
     free(fileContent);
     free(updatedContent);

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