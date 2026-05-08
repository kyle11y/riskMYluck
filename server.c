#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET socket_t;
typedef int socklen_compat_t;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
typedef socklen_t socklen_compat_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSE_SOCKET close
#endif

#define BUFFER_SIZE 4096
#define STARTING_HP 100
#define MAX_HP 100
#define MAX_ROUNDS 10
#define CARD_PULLS 10

#define GREEN_HEAL 5
#define RED_DAMAGE 5
#define BLACK_DAMAGE 10
#define GOLD_HEAL 20
#define SKULL_DAMAGE 20

#define SEPARATOR "=============================="
#define RESET "\033[0m"
#define GRAY "\033[90m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RED "\033[31m"
#define BLACK "\033[1;30m"
#define GOLD "\033[93m"
#define PURPLE "\033[35m"

typedef enum {
    CARD_GRAY,
    CARD_GREEN,
    CARD_RED,
    CARD_BLACK,
    CARD_GOLD,
    CARD_SKULL
} CardType;

typedef struct {
    int gray;
    int green;
    int red;
    int black;
    int gold;
    int skull;
} Stats;

int send_text(socket_t sock, const char *text) {
    size_t sent_total = 0;
    size_t length = strlen(text);


    while (sent_total < length) {
        int sent = send(sock, text + sent_total, (int)(length - sent_total), 0);


        if (sent <= 0) {
            return 0;
        }

        sent_total += (size_t)sent;
    }

    return 1;
}

int print_both(socket_t client_sock, const char *format, ...) {
    char message[BUFFER_SIZE];
    va_list args;


    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);


    printf("%s", message);
    fflush(stdout);
    return send_text(client_sock, message);
}

void remove_newline(char *text) {
    text[strcspn(text, "\r\n")] = '\0';
}

int receive_text(socket_t sock, char *buffer, int size) {
    int n;


    memset(buffer, 0, size);
    n = recv(sock, buffer, size - 1, 0);


    if (n <= 0) {
        return 0;
    }


    buffer[n] = '\0';
    remove_newline(buffer);
    return 1;
}

void ask_server_player(const char *prompt, char *answer, int size) {
    printf("%s", prompt);
    fflush(stdout);


    if (fgets(answer, size, stdin) == NULL) {
        answer[0] = '\0';
    }
    remove_newline(answer);
}

int send_client_prompt(socket_t client_sock, const char *prompt) {
    char prompt_message[BUFFER_SIZE];

    snprintf(prompt_message, sizeof(prompt_message), "__PROMPT__:%s\n", prompt);
    return send_text(client_sock, prompt_message);
}

int ask_client_player(socket_t client_sock, const char *prompt, char *answer, int size) {

    if (!send_client_prompt(client_sock, prompt)) {
        return 0;
    }


    return receive_text(client_sock, answer, size);
}

const char *card_name(CardType card) {

    if (card == CARD_GRAY) return "GRAY";
    if (card == CARD_GREEN) return "GREEN";
    if (card == CARD_RED) return "RED";
    if (card == CARD_BLACK) return "BLACK";
    if (card == CARD_GOLD) return "GOLD";
    return "SKULL";
}

const char *card_color(CardType card) {

    if (card == CARD_GRAY) return GRAY;
    if (card == CARD_GREEN) return GREEN;
    if (card == CARD_RED) return RED;
    if (card == CARD_BLACK) return BLACK;
    if (card == CARD_GOLD) return GOLD;
    return PURPLE;
}

const char *hp_color(int hp) {

    if (hp > 70) return GREEN;
    if (hp >= 31) return YELLOW;
    return RED;
}

int cap_hp(int hp) {

    if (hp > MAX_HP) return MAX_HP;
    if (hp < 0) return 0;
    return hp;
}

void add_stat(Stats *stats, CardType card) {

    if (card == CARD_GRAY) stats->gray++;
    else if (card == CARD_GREEN) stats->green++;
    else if (card == CARD_RED) stats->red++;
    else if (card == CARD_BLACK) stats->black++;
    else if (card == CARD_GOLD) stats->gold++;
    else if (card == CARD_SKULL) stats->skull++;
}

void add_turn_stats(Stats *total, Stats turn) {

    total->gray += turn.gray;
    total->green += turn.green;
    total->red += turn.red;
    total->black += turn.black;
    total->gold += turn.gold;
    total->skull += turn.skull;
}

CardType get_random_card(int difficulty) {
    int roll = rand() % 100;


    if (difficulty == 1) {
        if (roll < 35) return CARD_GRAY;
        if (roll < 70) return CARD_GREEN;
        if (roll < 88) return CARD_RED;
        if (roll < 96) return CARD_BLACK;
        if (roll < 99) return CARD_GOLD;
        return CARD_SKULL;
    }


    if (difficulty == 3) {
        if (roll < 30) return CARD_GRAY;
        if (roll < 45) return CARD_GREEN;
        if (roll < 75) return CARD_RED;
        if (roll < 92) return CARD_BLACK;
        if (roll < 94) return CARD_GOLD;
        return CARD_SKULL;
    }


    if (roll < 35) return CARD_GRAY;
    if (roll < 60) return CARD_GREEN;
    if (roll < 85) return CARD_RED;
    if (roll < 95) return CARD_BLACK;
    if (roll < 98) return CARD_GOLD;
    return CARD_SKULL;
}

int choose_difficulty(socket_t client_sock) {
    char input[BUFFER_SIZE];
    int choice = 0;


    send_text(client_sock, "\nWaiting for Player 1 to choose difficulty...\n");


    while (choice < 1 || choice > 3) {

        printf("\n%s\n", SEPARATOR);
        printf("DIFFICULTY MODE\n");
        printf("1. Easy\n");
        printf("2. Normal\n");
        printf("3. Hard\n");
        printf("Choose difficulty: ");
        fflush(stdout);

        fgets(input, sizeof(input), stdin);
        choice = atoi(input);


        if (choice < 1 || choice > 3) {

            printf("Please enter 1, 2, or 3.\n");
        }
    }

    return choice;
}

const char *difficulty_name(int difficulty) {

    if (difficulty == 1) return "Easy";
    if (difficulty == 3) return "Hard";
    return "Normal";
}

void show_hp(socket_t client_sock, const char *p1_name, int p1_hp, const char *p2_name, int p2_hp) {

    print_both(client_sock,
               "%s HP: %s%d%s\n"
               "%s HP: %s%d%s\n",
               p1_name, hp_color(p1_hp), p1_hp, RESET,
               p2_name, hp_color(p2_hp), p2_hp, RESET);
}

void show_scoreboard(socket_t client_sock, const char *p1_name, const char *p2_name, int p1_wins, int p2_wins) {

    print_both(client_sock,
               "\n%s\n"
               "SCOREBOARD\n\n"
               "%s Wins: %d\n"
               "%s Wins: %d\n"
               "%s\n",
               SEPARATOR, p1_name, p1_wins, p2_name, p2_wins, SEPARATOR);
}

void show_stats(socket_t client_sock, const char *name, Stats stats) {

    print_both(client_sock,
               "\n%s pulled:\n"
               "Gray: %d\n"
               "Green: %d\n"
               "Red: %d\n"
               "Black: %d\n"
               "Gold: %d\n"
               "Skull: %d\n",
               name,
               stats.gray,
               stats.green,
               stats.red,
               stats.black,
               stats.gold,
               stats.skull);
}

int yes_answer(const char *answer) {

    while (*answer != '\0' && isspace((unsigned char)*answer)) {
        answer++;
    }


    return tolower((unsigned char)*answer) == 'y';
}

int play_turn(socket_t client_sock,
              int player,
              int round,
              int difficulty,
              const char *p1_name,
              const char *p2_name,
              int *p1_hp,
              int *p2_hp,
              Stats *p1_total,
              Stats *p2_total) {
    char input[BUFFER_SIZE];
    CardType cards[CARD_PULLS];
    Stats turn_stats = {0, 0, 0, 0, 0, 0};
    const char *current_name = (player == 1) ? p1_name : p2_name;
    const char *enemy_name = (player == 1) ? p2_name : p1_name;
    int *current_hp = (player == 1) ? p1_hp : p2_hp;
    int *enemy_hp = (player == 1) ? p2_hp : p1_hp;
    int heal;
    int self_damage;
    int enemy_damage;



    print_both(client_sock,
               "\n%s\n"
               "ROUND %d/%d\n"
               "%s\n"
               "%s's turn\n",
               SEPARATOR, round, MAX_ROUNDS, SEPARATOR, current_name);
    show_hp(client_sock, p1_name, *p1_hp, p2_name, *p2_hp);


    if (player == 1) {

        send_text(client_sock, "\nWaiting for Player 1 to pull cards...\n");


        ask_server_player("\nPress ENTER to pull 10 luckies: ", input, sizeof(input));
    } else {

        printf("\nWaiting for Player 2 to pull cards...\n");
        fflush(stdout);


        if (!ask_client_player(client_sock, "Press ENTER to pull 10 luckies: ", input, sizeof(input))) {
            return 0;
        }
    }


    print_both(client_sock, "\nPulled cards:\n");


    for (int i = 0; i < CARD_PULLS; i++) {
        cards[i] = get_random_card(difficulty);
        add_stat(&turn_stats, cards[i]);


        print_both(client_sock,
                   "[%d] %s%s%s\n",
                   i + 1,
                   card_color(cards[i]),
                   card_name(cards[i]),
                   RESET);
    }


    if (player == 1) {
        add_turn_stats(p1_total, turn_stats);
    } else {
        add_turn_stats(p2_total, turn_stats);
    }


    heal = (turn_stats.green * GREEN_HEAL) + (turn_stats.gold * GOLD_HEAL);
    self_damage = (turn_stats.red * RED_DAMAGE) + (turn_stats.skull * SKULL_DAMAGE);
    enemy_damage = turn_stats.black * BLACK_DAMAGE;


    *current_hp = cap_hp(*current_hp + heal - self_damage);
    *enemy_hp = cap_hp(*enemy_hp - enemy_damage);



    print_both(client_sock,
               "\n%s\n"
               "RESULTS\n"
               "%s\n"
               "Gray x%d | Green x%d | Red x%d | Black x%d | Gold x%d | Skull x%d\n"
               "+%d HP from Green/Gold\n"
               "-%d HP from Red/Skull\n"
               "%s takes -%d HP from Black\n\n",
               SEPARATOR,
               SEPARATOR,
               turn_stats.gray,
               turn_stats.green,
               turn_stats.red,
               turn_stats.black,
               turn_stats.gold,
               turn_stats.skull,
               heal,
               self_damage,
               enemy_name,
               enemy_damage);

    show_hp(client_sock, p1_name, *p1_hp, p2_name, *p2_hp);
    return 1;
}

void show_game_rules(socket_t client_sock, const char *p1_name, const char *p2_name, int difficulty) {

    print_both(client_sock,
               "\n%s\n"
               "GAME START\n"
               "%s\n"
               "%s vs %s\n"
               "Difficulty: %s\n"
               "Max HP: %d\n"
               "Max rounds: %d\n\n"
               "Gray = nothing\n"
               "Green = +5 HP\n"
               "Red = -5 HP to yourself\n"
               "Black = -10 HP to enemy\n"
               "Gold = rare +20 HP\n"
               "Skull = rare -20 HP to yourself\n",
               SEPARATOR,
               SEPARATOR,
               p1_name,
               p2_name,
               difficulty_name(difficulty),
               MAX_HP,
               MAX_ROUNDS);
}

int main(int argc, char *argv[]) {
    socket_t server_sock;
    socket_t client_sock;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_compat_t client_size;
    int port_no;
    int option = 1;

    char p1_name[50];
    char p2_name[50];
    char p1_answer[BUFFER_SIZE];
    char p2_answer[BUFFER_SIZE];

    int p1_wins = 0;
    int p2_wins = 0;
    int playing = 1;


#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif


    if (argc < 2) {

        printf("Usage: %s port_no\n", argv[0]);
        return 1;
    }


    srand((unsigned int)time(NULL));
    port_no = atoi(argv[1]);


    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {

        printf("Error: socket() failed.\n");
        return 1;
    }


    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_no);


    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option));


    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {

        printf("Error: bind() failed. Try another port.\n");
        CLOSE_SOCKET(server_sock);
        return 1;
    }


    listen(server_sock, 5);


    printf("Server listening on port %d...\n", port_no);
    printf("Waiting for Player 2...\n");


    client_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_size);
    if (client_sock == INVALID_SOCKET) {

        printf("Error: accept() failed.\n");
        CLOSE_SOCKET(server_sock);
        return 1;
    }


    printf("Player 2 connected!\n\n");



    printf("%s\nRisk vs Luck\n%s\n", SEPARATOR, SEPARATOR);


    ask_server_player("Enter Player 1 name: ", p1_name, sizeof(p1_name));


    send_text(client_sock, "==============================\nRisk vs Luck\n==============================\n");


    if (!ask_client_player(client_sock, "Enter Player 2 name: ", p2_name, sizeof(p2_name))) {

        printf("Player 2 disconnected.\n");
        CLOSE_SOCKET(client_sock);
        CLOSE_SOCKET(server_sock);
        return 0;
    }


    if (strlen(p1_name) == 0) strcpy(p1_name, "Player 1");
    if (strlen(p2_name) == 0) strcpy(p2_name, "Player 2");


    while (playing) {
        int p1_hp = STARTING_HP;
        int p2_hp = STARTING_HP;
        int round = 1;
        int turn = 1;
        int difficulty;
        int connected = 1;
        Stats p1_stats = {0, 0, 0, 0, 0, 0};
        Stats p2_stats = {0, 0, 0, 0, 0, 0};


        difficulty = choose_difficulty(client_sock);


        print_both(client_sock, "\nSelected difficulty: %s\n", difficulty_name(difficulty));
        show_game_rules(client_sock, p1_name, p2_name, difficulty);
        show_scoreboard(client_sock, p1_name, p2_name, p1_wins, p2_wins);


        while (p1_hp > 0 && p2_hp > 0 && round <= MAX_ROUNDS) {
            connected = play_turn(client_sock,
                                  turn,
                                  round,
                                  difficulty,
                                  p1_name,
                                  p2_name,
                                  &p1_hp,
                                  &p2_hp,
                                  &p1_stats,
                                  &p2_stats);


            if (!connected) {

                printf("Player 2 disconnected.\n");
                break;
            }


            if (turn == 1) {
                turn = 2;
            } else {
                turn = 1;
                round++;
            }
        }


        if (!connected) {
            break;
        }


        print_both(client_sock, "\n%s\nFINAL WINNER\n%s\n", SEPARATOR, SEPARATOR);


        if (p1_hp <= 0 && p2_hp <= 0) {

            print_both(client_sock, "DRAW! Both players reached 0 HP.\n");
        } else if (p1_hp <= 0) {

            print_both(client_sock, "%s wins!\n", p2_name);
            p2_wins++;
        } else if (p2_hp <= 0) {

            print_both(client_sock, "%s wins!\n", p1_name);
            p1_wins++;
        } else if (p1_hp > p2_hp) {

            print_both(client_sock, "Maximum rounds reached. %s wins!\n", p1_name);
            p1_wins++;
        } else if (p2_hp > p1_hp) {

            print_both(client_sock, "Maximum rounds reached. %s wins!\n", p2_name);
            p2_wins++;
        } else {

            print_both(client_sock, "Maximum rounds reached. DRAW!\n");
        }


        show_hp(client_sock, p1_name, p1_hp, p2_name, p2_hp);


        print_both(client_sock, "\n%s\nLUCK STATISTICS\n%s\n", SEPARATOR, SEPARATOR);
        show_stats(client_sock, p1_name, p1_stats);
        show_stats(client_sock, p2_name, p2_stats);
        show_scoreboard(client_sock, p1_name, p2_name, p1_wins, p2_wins);


        if (!send_client_prompt(client_sock, "Play again? (y/n): ")) {
            break;
        }


        ask_server_player("\nPlay again? (y/n): ", p1_answer, sizeof(p1_answer));


        if (!receive_text(client_sock, p2_answer, sizeof(p2_answer))) {
            break;
        }


        if (yes_answer(p1_answer) && yes_answer(p2_answer)) {

            print_both(client_sock, "\nBoth players chose yes. Starting again...\n");
        } else {

            print_both(client_sock, "\nReplay declined. Closing sockets safely...\n");
            playing = 0;
        }
    }



    printf("Closing connection...\n");
    CLOSE_SOCKET(client_sock);
    CLOSE_SOCKET(server_sock);


#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
