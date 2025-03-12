#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define ROOT_DIR "www"

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE);

    printf("Request: %s\n", buffer);

    char *request_line = strtok(buffer, "\r\n");
    if (request_line == NULL) {
        perror("Invalid request");
        close(client_socket);
        return;
    }

    char method[10], path[100], protocol[10];
    sscanf(request_line, "%s %s %s", method, path, protocol);

    if (strcmp(method, "GET") != 0) {
        const char *msg = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        write(client_socket, msg, strlen(msg));
        close(client_socket);
        return;
    }

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%s", ROOT_DIR, path);
    if (strcmp(path, "/") == 0) {
        strcat(file_path, "index.html");
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        const char *msg = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(client_socket, msg, strlen(msg));
        close(client_socket);
        return;
    }

    const char *header = "HTTP/1.1 200 OK\r\n\r\n";
    write(client_socket, header, strlen(header));

    char file_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
        write(client_socket, file_buffer, bytes_read);
    }

    fclose(file);
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        handle_client(new_socket);
    }

    close(server_fd);
    return 0;
}
