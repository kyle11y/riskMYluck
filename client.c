/*
 * Packet Salvage - TCP socket game client
 *
 * Based on the Lab Exercise 10 client/server socket flow:
 * socket() -> connect() -> send()/recv()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSE_SOCKET close
#endif

#define BUFFER_SIZE 256

static void die_with_error(const char *error_msg) {
    printf("%s\n", error_msg);
    exit(1);
}

static void network_startup(void) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        die_with_error("Error: WSAStartup() Failed.");
    }
#endif
}

static void network_cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

static int send_all(socket_t sock, const char *message) {
    size_t sent_total = 0;
    size_t length = strlen(message);

    while (sent_total < length) {
        int sent = send(sock, message + sent_total, (int)(length - sent_total), 0);
        if (sent <= 0) {
            return -1;
        }
        sent_total += (size_t)sent;
    }

    return 0;
}

static int recv_line(socket_t sock, char *buffer, size_t size) {
    size_t used = 0;

    while (used + 1 < size) {
        char ch;
        int received = recv(sock, &ch, 1, 0);

        if (received <= 0) {
            return -1;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            break;
        }

        buffer[used++] = ch;
    }

    buffer[used] = '\0';
    return (int)used;
}

static void send_user_input(socket_t sock, const char *label) {
    char input[BUFFER_SIZE];

    printf("%s: ", label);
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        strcpy(input, "quit\n");
    }

    if (strchr(input, '\n') == NULL) {
        size_t length = strlen(input);
        if (length + 1 < sizeof(input)) {
            input[length] = '\n';
            input[length + 1] = '\0';
        }
    }

    if (send_all(sock, input) < 0) {
        die_with_error("Error: send() Failed.");
    }
}

int main(int argc, char *argv[]) {
    socket_t client_sock;
    int port_no;
    struct addrinfo hints;
    struct addrinfo *server_list = NULL;
    struct addrinfo *server = NULL;
    char port_text[16];
    char buffer[BUFFER_SIZE];

    if (argc < 3) {
        printf("Usage: %s hostname port_no\n", argv[0]);
        return 1;
    }

    network_startup();

    printf("Client starting ...\n");
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == INVALID_SOCKET) {
        die_with_error("Error: socket() Failed.");
    }

    port_no = atoi(argv[2]);
    snprintf(port_text, sizeof(port_text), "%d", port_no);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    printf("Looking for host '%s'...\n", argv[1]);
    if (getaddrinfo(argv[1], port_text, &hints, &server_list) != 0) {
        CLOSE_SOCKET(client_sock);
        die_with_error("Error: No such host.");
    }
    printf("Host found!\n");

    printf("Connecting to server at port %d...\n", port_no);
    for (server = server_list; server != NULL; server = server->ai_next) {
        if (connect(client_sock, server->ai_addr, (int)server->ai_addrlen) != SOCKET_ERROR) {
            break;
        }
    }

    freeaddrinfo(server_list);

    if (server == NULL) {
        CLOSE_SOCKET(client_sock);
        die_with_error("Error: connect() Failed.");
    }

    printf("Connection successful!\n\n");

    while (recv_line(client_sock, buffer, sizeof(buffer)) >= 0) {
        if (strcmp(buffer, "__END__") == 0) {
            break;
        }

        if (strncmp(buffer, "__PROMPT__:", 11) == 0) {
            send_user_input(client_sock, buffer + 11);
        } else {
            printf("%s\n", buffer);
        }
    }

    printf("Disconnected from server.\n");
    CLOSE_SOCKET(client_sock);
    network_cleanup();

    return 0;
}
