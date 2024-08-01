#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PROMPT "\r\n."

#define BUFFER_SIZE 4096
#define SEND_BUF_SZ 2048
#define RECV_BUF_SZ 2048
#define PROMPT_BUF_SZ 2048
#define PROMPT_SZ 3

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_command(int sockfd, const char *command) {
    if (send(sockfd, command, strlen(command) + 1, 0) < 0) {
        error("Error sending command");
    }
}

void receive_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        // Check for the command prompt indicator
        if (strstr(buffer, "Command") != NULL) {
            break;
        }
    }
    if (bytes_received < 0) {
        error("Error receiving response");
    }
}

void receive_figlet_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    struct iovec iov;
    struct msghdr msg;
    int bytes_received;
    ssize_t len = 0;

    while ((bytes_received = recv(sockfd, buffer + len, 1, 0)) > 0) {
        len += bytes_received;
        if (len > RECV_BUF_SZ - 1) {
            perror("recv buf overflow");
            exit(EXIT_FAILURE);
        }
        if (len >= PROMPT_SZ && (0 == strncmp(buffer + len - PROMPT_SZ, PROMPT, PROMPT_SZ))) {
            break; // while
        }
    }
    if (bytes_received < 0) {
        error("Error receiving response");
    }
    buffer[len] = '\0'; 
    printf("%s", buffer);
    return; 
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <font> <text>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *font = argv[1];
    const char *text = argv[2];

    const char *hostname = "telehack.com";
    const int port = 23;

    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Error connecting to server");
    }

    // Read initial server response and wait for command prompt
    receive_response(sockfd);

    // Send figlet command with font and text
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "figlet /%s %s\n\r", font, text);
    send_command(sockfd, command);

    // Read and print figlet response from server
    receive_figlet_response(sockfd);

    close(sockfd);
    return 0;
}
