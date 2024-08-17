#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <arpa/inet.h>

int create_and_bind_socket(const char *address, int port) {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(address);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, SOMAXCONN) < 0) {
        perror("Error on listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int setup_epoll(int sockfd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Error creating epoll");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("Error adding socket to epoll");
        close(sockfd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    return epoll_fd;
}

void handle_request(int client_fd, const char *base_dir) {
    char buffer[1024];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Error reading from socket");
        close(client_fd);
        return;
    }
    buffer[bytes_read] = '\0';

    // Извлечение имени файла из запроса
    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    if (!method || !path || strcmp(method, "GET") != 0) {
        write(client_fd, "HTTP/1.1 405 Method Not Allowed\r\n\r\n", 35);
        close(client_fd);
        return;
    }

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s%s", base_dir, path);

    struct stat file_stat;
    if (stat(filepath, &file_stat) < 0) {
        write(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n", 26);
        close(client_fd);
        return;
    }

    if (!(file_stat.st_mode & S_IRUSR)) {
        write(client_fd, "HTTP/1.1 403 Forbidden\r\n\r\n", 26);
        close(client_fd);
        return;
    }

    int file_fd = open(filepath, O_RDONLY);
    if (file_fd < 0) {
        write(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 39);
        close(client_fd);
        return;
    }

    write(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19);

    char file_buffer[1024];
    ssize_t file_bytes;
    while ((file_bytes = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
        write(client_fd, file_buffer, file_bytes);
    }

    close(file_fd);
    close(client_fd);
}

void run_server(int sockfd, const char *base_dir) {
    int epoll_fd = setup_epoll(sockfd);

    struct epoll_event events[10];
    while (1) {
        int num_fds = epoll_wait(epoll_fd, events, 10, -1);
        for (int i = 0; i < num_fds; i++) {
            if (events[i].data.fd == sockfd) {
                int client_fd = accept(sockfd, NULL, NULL);
                if (client_fd < 0) {
                    perror("Error on accept");
                    continue;
                }

                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("Error adding client socket to epoll");
                    close(client_fd);
                }
            } else {
                handle_request(events[i].data.fd, base_dir);
            }
        }
    }

    close(epoll_fd);
    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <directory> <address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *base_dir = argv[1];
    const char *address = argv[2];
    int port = atoi(argv[3]);

    int sockfd = create_and_bind_socket(address, port);
    run_server(sockfd, base_dir);

    return 0;
}

