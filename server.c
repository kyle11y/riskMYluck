/*
 * Packet Salvage - TCP socket game server
 *
 * Based on the Lab Exercise 10 client/server socket flow:
 * socket() -> bind() -> listen() -> accept() -> send()/recv()
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
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
#define NODE_COUNT 5
#define TARGET_REPAIRS 5

typedef struct {
    const char *name;
    int health;
    int fault;
} Node;

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

static int send_line(socket_t sock, const char *message) {
    if (send_all(sock, message) < 0) {
        return -1;
    }
    return send_all(sock, "\n");
}

static int send_format(socket_t sock, const char *format, ...) {
    char message[512];
    va_list args;

    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    return send_line(sock, message);
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

static int ask_client(socket_t client_sock, const char *label, char *answer, size_t size) {
    if (send_format(client_sock, "__PROMPT__:%s", label) < 0) {
        return 0;
    }
    return recv_line(client_sock, answer, size) >= 0;
}

static void clean_name(char *name) {
    size_t write_at = 0;
    size_t read_at = 0;

    while (name[read_at] != '\0' && isspace((unsigned char)name[read_at])) {
        read_at++;
    }

    while (name[read_at] != '\0' && write_at < 24) {
        if (isspace((unsigned char)name[read_at])) {
            if (write_at > 0 && name[write_at - 1] != ' ') {
                name[write_at++] = ' ';
            }
        } else {
            name[write_at++] = name[read_at];
        }
        read_at++;
    }

    if (write_at > 0 && name[write_at - 1] == ' ') {
        write_at--;
    }
    name[write_at] = '\0';

    if (name[0] == '\0') {
        strcpy(name, "Player");
    }
}

static int random_between(int min, int max) {
    return min + rand() % (max - min + 1);
}

static int starts_with_ignore_case(const char *value, const char *prefix) {
    while (*prefix != '\0') {
        if (tolower((unsigned char)*value) != tolower((unsigned char)*prefix)) {
            return 0;
        }
        value++;
        prefix++;
    }
    return 1;
}

static Node *find_node(Node nodes[], const char *target) {
    if (target == NULL || target[0] == '\0') {
        return NULL;
    }

    if (isdigit((unsigned char)target[0])) {
        int index = atoi(target) - 1;
        if (index >= 0 && index < NODE_COUNT) {
            return &nodes[index];
        }
    }

    for (int i = 0; i < NODE_COUNT; i++) {
        if (starts_with_ignore_case(nodes[i].name, target)) {
            return &nodes[i];
        }
    }

    return NULL;
}

static void split_command(char *command, char *action, size_t action_size, char **target) {
    size_t index = 0;

    while (*command != '\0' && isspace((unsigned char)*command)) {
        command++;
    }

    while (*command != '\0' && !isspace((unsigned char)*command) && index + 1 < action_size) {
        action[index++] = (char)tolower((unsigned char)*command);
        command++;
    }
    action[index] = '\0';

    while (*command != '\0' && isspace((unsigned char)*command)) {
        command++;
    }

    *target = command;
}

static void send_status(socket_t client_sock, Node nodes[], int integrity, int repairs) {
    send_format(client_sock, "Packet integrity: %d/100 | Faults repaired: %d/%d",
                integrity, repairs, TARGET_REPAIRS);
    send_line(client_sock, "Nodes:");

    for (int i = 0; i < NODE_COUNT; i++) {
        send_format(client_sock, "  %d. %-7s health=%3d/100 status=%s",
                    i + 1,
                    nodes[i].name,
                    nodes[i].health,
                    nodes[i].fault ? "FAULT" : "clear");
    }
}

static void send_help(socket_t client_sock) {
    send_line(client_sock, "Commands:");
    send_line(client_sock, "  scan          - show node health and active faults");
    send_line(client_sock, "  patch <node>  - repair a faulty node or improve a clear one");
    send_line(client_sock, "  route <node>  - reinforce one node before the next fault");
    send_line(client_sock, "  shield        - restore packet integrity");
    send_line(client_sock, "  quit          - leave the game");
}

static void create_network_event(socket_t client_sock, Node nodes[], int *integrity) {
    int index = random_between(0, NODE_COUNT - 1);
    int damage = random_between(8, 18);
    Node *node = &nodes[index];

    if (node->fault) {
        damage += 5;
        *integrity -= random_between(4, 8);
    }

    node->fault = 1;
    node->health -= damage;
    if (node->health < 0) {
        node->health = 0;
    }
    if (node->health == 0) {
        *integrity -= 15;
    }
    if (*integrity < 0) {
        *integrity = 0;
    }

    send_format(client_sock, "Network event: %s took %d damage and now has an active fault.",
                node->name, damage);
}

static int process_command(socket_t client_sock,
                           const char *player_name,
                           Node nodes[],
                           int *integrity,
                           int *repairs,
                           char *command) {
    char action[32];
    char *target = NULL;

    split_command(command, action, sizeof(action), &target);

    if (action[0] == '\0') {
        send_line(client_sock, "Please enter a command. Type 'help' for options.");
        return 0;
    }

    if (strcmp(action, "help") == 0) {
        send_help(client_sock);
        return 0;
    }

    if (strcmp(action, "scan") == 0) {
        send_status(client_sock, nodes, *integrity, *repairs);
        return 0;
    }

    if (strcmp(action, "quit") == 0) {
        send_line(client_sock, "You left the mission.");
        return -1;
    }

    if (strcmp(action, "shield") == 0) {
        int gained = random_between(5, 10);
        *integrity += gained;
        if (*integrity > 100) {
            *integrity = 100;
        }
        send_format(client_sock, "%s boosted packet integrity by %d.", player_name, gained);
        return 1;
    }

    if (strcmp(action, "patch") == 0 || strcmp(action, "route") == 0) {
        Node *node = find_node(nodes, target);

        if (node == NULL) {
            send_line(client_sock, "Unknown node. Use 'scan' to see valid node names and numbers.");
            return 0;
        }

        if (strcmp(action, "patch") == 0) {
            if (node->fault) {
                int restored = random_between(22, 32);
                node->fault = 0;
                node->health += restored;
                if (node->health > 100) {
                    node->health = 100;
                }
                (*repairs)++;
                *integrity += 3;
                if (*integrity > 100) {
                    *integrity = 100;
                }
                send_format(client_sock, "%s patched %s. Fault repaired (%d/%d).",
                            player_name, node->name, *repairs, TARGET_REPAIRS);
            } else {
                int restored = random_between(10, 18);
                node->health += restored;
                if (node->health > 100) {
                    node->health = 100;
                }
                send_format(client_sock, "%s tuned %s, restoring %d health.",
                            player_name, node->name, restored);
            }
            return 1;
        }

        if (strcmp(action, "route") == 0) {
            int reinforced = random_between(15, 24);
            node->health += reinforced;
            if (node->health > 100) {
                node->health = 100;
            }
            send_format(client_sock, "%s rerouted traffic through %s, adding %d health.",
                        player_name, node->name, reinforced);
            return 1;
        }
    }

    send_line(client_sock, "Unknown command. Type 'help' for options.");
    return 0;
}

static void play_game(socket_t client_sock) {
    Node nodes[NODE_COUNT] = {
        {"Gateway", 100, 0},
        {"Cache", 100, 0},
        {"Auth", 100, 0},
        {"Relay", 100, 0},
        {"Archive", 100, 0}
    };
    char player_name[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    int integrity = 100;
    int repairs = 0;
    int running = 1;

    send_line(client_sock, "Connected to Packet Salvage.");
    send_line(client_sock, "Enter your player name.");
    if (!ask_client(client_sock, "Player name", player_name, sizeof(player_name))) {
        return;
    }
    clean_name(player_name);

    send_format(client_sock, "Welcome, %s.", player_name);
    send_line(client_sock, "Mission: repair network faults before packet integrity reaches zero.");
    send_line(client_sock, "Type 'help' for commands.");

    while (running && integrity > 0 && repairs < TARGET_REPAIRS) {
        send_line(client_sock, "");
        create_network_event(client_sock, nodes, &integrity);
        send_status(client_sock, nodes, integrity, repairs);

        while (running) {
            send_line(client_sock, "");
            send_line(client_sock, "Your command: scan | patch <node> | route <node> | shield | help | quit");
            if (!ask_client(client_sock, "Command", command, sizeof(command))) {
                running = 0;
                break;
            }

            int result = process_command(client_sock, player_name, nodes, &integrity, &repairs, command);
            if (result < 0) {
                running = 0;
                break;
            }
            if (result > 0) {
                break;
            }
        }
    }

    send_line(client_sock, "");
    if (repairs >= TARGET_REPAIRS) {
        send_line(client_sock, "MISSION SUCCESS: enough faults were repaired and the relay is stable.");
    } else if (integrity <= 0) {
        send_line(client_sock, "MISSION FAILED: packet integrity dropped to zero.");
    } else {
        send_line(client_sock, "MISSION ENDED: client disconnected.");
    }
    send_line(client_sock, "__END__");
}

int main(int argc, char *argv[]) {
    socket_t server_sock;
    socket_t client_sock;
    int port_no;
    int option = 1;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
#ifdef _WIN32
    int client_size;
#else
    socklen_t client_size;
#endif

    if (argc < 2) {
        printf("Usage: %s port_no\n", argv[0]);
        return 1;
    }

    srand((unsigned int)time(NULL));
    network_startup();

    printf("Server starting ...\n");
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        die_with_error("Error: socket() Failed.");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    port_no = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((unsigned short)port_no);

    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option));

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        CLOSE_SOCKET(server_sock);
        die_with_error("Error: bind() Failed.");
    }

    if (listen(server_sock, 5) == SOCKET_ERROR) {
        CLOSE_SOCKET(server_sock);
        die_with_error("Error: listen() Failed.");
    }

    printf("Server listening to port %d ...\n", port_no);
    printf("Waiting for connection ...\n");

    client_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_size);
    if (client_sock == INVALID_SOCKET) {
        CLOSE_SOCKET(server_sock);
        die_with_error("Error: accept() Failed.");
    }

    printf("Client successfully connected ...\n");
    play_game(client_sock);

    printf("Closing connection ...\n");
    CLOSE_SOCKET(client_sock);
    CLOSE_SOCKET(server_sock);
    network_cleanup();

    return 0;
}
