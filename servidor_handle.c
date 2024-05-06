//
// Created by bale2 on 01/03/2024.
//

#include "servidor_handle.h"
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


int handle_register(char *username) {

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


    return 0;
}

void remove_files_in_directory(char *dirpath) {
    struct dirent *entry;
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    size_t dirpath_len = strlen(dirpath);
    size_t max_filepath_len = dirpath_len + NAME_MAX + 2;
    char *filepath = (char *) malloc(max_filepath_len);
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
        if (entry->d_type == DT_DIR) {
            // Recursively remove files in subdirectory
            remove_files_in_directory(filepath);
            // Remove the directory itself
            if (rmdir(filepath) != 0) {
                printf("Error removing directory %s\n", filepath);
                perror("Error removing directory");
            }
        } else {
            // Remove regular file
            if (remove(filepath) != 0) {
                printf("Error removing file %s\n", filepath);
                perror("Error removing file");
            }
        }
    }

    free(filepath);
    closedir(dir);
}


int handle_unregister(char *username) {
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

    return 0;
}

int handle_connect(char *username, int port, char *address) {
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

    // Write the address to the file
    if (write_connection(address, port, file) != 0) {
        return 3;
    }

    // Check if the user has a folder called 'files'
    char user_files[MAX_FILEPATH_LENGTH];
    snprintf(user_files, sizeof(user_files), "%s%s/files", DATA_DIRECTORY, username);
    if (access(user_files, F_OK) == -1) {
        // Create a folder in the same directory called 'files'
        if (mkdir(user_files, 0700) == -1) {
            perror("Error creating directory");
            return 3;
        }
    }

    return 0;
}

int handle_publish(char *userName, char *fileName, char *description) {
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

    // Construir la ruta para el nuevo archivo de texto
    char new_file_path[2 * MAX_FILEPATH_LENGTH];
    snprintf(new_file_path, sizeof(new_file_path), "%s/files/%s", user, fileName);

    // Comprobar si el archivo ya existe
    if (access(new_file_path, F_OK) != -1) {
        perror("Error: file already exists");
        return 3;
    }

    // Abrir el archivo de texto para escribir el contenido
    FILE *new_file = fopen(new_file_path, "w");
    if (new_file == NULL) {
        perror("Error creating new file");
        return 4;
    }

    // Escribir la descripción en el archivo de texto
    fprintf(new_file, "%s\n", description);

    // Cerrar el archivo
    fclose(new_file);

    return 0;


}

int handle_delete(char *username, char *filename) {
    char user[MAX_FILEPATH_LENGTH];
    snprintf(user, sizeof(user), "%s%s", DATA_DIRECTORY, username);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(user, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }
    char user_connect[MAX_FILEPATH_LENGTH];
    snprintf(user_connect, sizeof(user_connect), "%s%s/connect", DATA_DIRECTORY, username);

    // If the file does not exist, return an error, as the user is not connected
    if (access(user_connect, F_OK) == -1) {
        perror("Error: user is not connected");
        return 2;
    }

    // Construir la ruta completa del archivo a eliminar
    char file_to_delete[2 * MAX_FILEPATH_LENGTH];
    snprintf(file_to_delete, sizeof(file_to_delete), "%s/files/%s", user, filename);

    // Comprobar si el archivo existe
    if (access(file_to_delete, F_OK) == -1) {
        perror("Error: file does not exist");
        return 3;
    }

    // Intentar eliminar el archivo
    if (remove(file_to_delete) != 0) {
        perror("Error deleting file");
        return 4;
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
    return 0;
}

int handle_list_users(char *username, int *n_connections, FILE *user_list) {
    char user[MAX_FILEPATH_LENGTH];
    snprintf(user, sizeof(user), "%s%s", DATA_DIRECTORY, username);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(user, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }

    // Check if user is connected
    char user_connect[MAX_FILEPATH_LENGTH];
    snprintf(user_connect, sizeof(user_connect), "%s%s/connect", DATA_DIRECTORY, username);
    if (access(user_connect, F_OK) == -1) {
        perror("Error: user is not connected");
        return 2;
    }

    // Open the directory
    DIR *dir = opendir(DATA_DIRECTORY);
    if (dir == NULL) {
        perror("Error opening directory");
        return 3;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue; // Skip . and ..
            }

            char user_connect_path[2 * MAX_FILEPATH_LENGTH];
            snprintf(user_connect_path, sizeof(user_connect_path), "%s%s/connect", DATA_DIRECTORY, entry->d_name);
            if (access(user_connect_path, F_OK) == 0) {
                FILE *connect_file = fopen(user_connect_path, "r");
                if (connect_file == NULL) {
                    perror("Error opening connect file");
                    closedir(dir);
                    return 4;
                }

                char address[INET_ADDRSTRLEN];
                int port;

                if (fscanf(connect_file, "%s\n%d\n", address, &port) != 2) {
                    perror("Error reading connect file");
                    fclose(connect_file);
                    closedir(dir);
                    return 5;
                }

                // Write connection info to user_list file
                fprintf(user_list, "%s %s %d\n", entry->d_name, address, port);
                fclose(connect_file);

                (*n_connections)++;
            }
        }
    }

    closedir(dir);
    return 0;
}

int handle_list_content(char *username, char *owner, int *num_content, FILE *content_list) {
    char user[MAX_FILEPATH_LENGTH];
    snprintf(user, sizeof(user), "%s%s", DATA_DIRECTORY, username);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(user, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }

    // Check if user is connected
    char user_connect[MAX_FILEPATH_LENGTH];
    snprintf(user_connect, sizeof(user_connect), "%s%s/connect", DATA_DIRECTORY, username);
    if (access(user_connect, F_OK) == -1) {
        perror("Error: user is not connected");
        return 2;
    }

    // Open the directory
    DIR *dir = opendir(DATA_DIRECTORY);
    if (dir == NULL) {
        perror("Error opening directory");
        return 4;
    }

    // Check if owner is registered
    char owner_path[MAX_FILEPATH_LENGTH];
    snprintf(owner_path, sizeof(owner_path), "%s%s", DATA_DIRECTORY, owner);
    if (access(owner_path, F_OK) == -1) {
        perror("Error: owner does not exist");
        return 3;
    }

    // Check if owner is connected
    char owner_connect[MAX_FILEPATH_LENGTH];
    snprintf(owner_connect, sizeof(owner_connect), "%s%s/connect", DATA_DIRECTORY, owner);
    if (access(owner_connect, F_OK) == -1) {
        printf("Owner is not connected\n");
        return 0;
    }

    // Check files in owner directory
    char owner_files[MAX_FILEPATH_LENGTH];
    snprintf(owner_files, sizeof(owner_files), "%s%s/files", DATA_DIRECTORY, owner);
    DIR *owner_dir = opendir(owner_files);
    if (owner_dir == NULL) {
        perror("Error opening directory");
        return 4;
    }

    struct dirent *entry;
    while ((entry = readdir(owner_dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue; // Skip . and ..
            }

            // Write file name and content to user_list file
            fprintf(content_list, "\t%s", entry->d_name);
            (*num_content)++;
            // Open file and write its content to content_list
            char file_path[MAX_FILEPATH_LENGTH * 2 + 6];
            snprintf(file_path, sizeof(file_path), "%s/files/%s", owner_path, entry->d_name);
            FILE *file = fopen(file_path, "r");
            if (file == NULL) {
                perror("Error opening file");
                return 4;
            }

            char description[BUFFER_SIZE];
            if (fgets(description, sizeof(description), file) == NULL) {
                perror("Error reading file");
                fclose(file);
                return 4;
            }

            fprintf(content_list, " %s\n", description);
            fclose(file);
        }
    }
    closedir(owner_dir);
    closedir(dir);
    return 0;
}

int handle_disconnect(char *user) {
    char user_register[MAX_FILEPATH_LENGTH];
    snprintf(user_register, sizeof(user_register), "%s%s", DATA_DIRECTORY, user);
    if (access(user_register, F_OK) == -1) {
        perror("Error: user does not exist");
        return 1;
    }

    char user_connect[MAX_FILEPATH_LENGTH];
    snprintf(user_connect, sizeof(user_connect), "%s%s/connect", DATA_DIRECTORY, user);
    if (access(user_connect, F_OK) == -1) {
        perror("Error: user is not connected");
        return 2;
    }
    if (remove(user_connect) != 0) {
        perror("Error deleting file");
        return 3;
    }

    return 0;
}

int handle_get_file(char *username, char *owner, char *address, int *port) {
    char user[MAX_FILEPATH_LENGTH];
    snprintf(user, sizeof(user), "%s%s", DATA_DIRECTORY, username);

    // If the directory doesn't exist, return an error, as the username does not exist
    if (access(user, F_OK) == -1) {
        perror("Error: user does not exist");
        return 2;
    }

    // Check if user is connected
    char user_connect[MAX_FILEPATH_LENGTH];
    snprintf(user_connect, sizeof(user_connect), "%s%s/connect", DATA_DIRECTORY, username);
    if (access(user_connect, F_OK) == -1) {
        perror("Error: user is not connected");
        return 2;
    }

    // Open owner connect file
    char owner_connect[MAX_FILEPATH_LENGTH];
    snprintf(owner_connect, sizeof(owner_connect), "%s%s/connect", DATA_DIRECTORY, owner);
    FILE *connect_file = fopen(owner_connect, "r");
    if (connect_file == NULL) {
        perror("Error opening connect file");
        return 2;
    }
    if (fscanf(connect_file, "%s\n%d\n", address, port) != 2) {
        perror("Error reading connect file");
        fclose(connect_file);
        return 2;
    }

    fclose(connect_file);
    return 0;
}
