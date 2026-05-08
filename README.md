# Risk vs Luck

Risk vs Luck is a two-player command-line socket game written in C. Player 1 runs the server, Player 2 connects with the client, and both players take turns pulling random cards that can heal, damage themselves, or damage the opponent.

## Game Overview

- Two players connect through a TCP socket.
- Each player starts with 100 HP.
- Player 1 chooses the difficulty: Easy, Normal, or Hard.
- Players take turns pulling 10 cards per turn.
- The game lasts up to 10 rounds, or ends early when a player's HP reaches 0.
- The winner is the player with HP remaining, or the player with higher HP after the final round.
- The game tracks wins and card statistics across replays.

## Card Effects

| Card | Effect |
| --- | --- |
| Gray | No HP change |
| Green | Heal self by 5 HP |
| Red | Damage self by 5 HP |
| Black | Damage opponent by 10 HP |
| Gold | Heal self by 20 HP |
| Skull | Damage self by 20 HP |

## Requirements

- GCC or another C compiler
- Two terminals
- A local network connection, or `localhost` when running both programs on one computer

## Compile

On Linux, macOS, Git Bash, or MSYS2:

```bash
gcc -Wall -Wextra -std=c11 -o server server.c
gcc -Wall -Wextra -std=c11 -o client client.c
```

On Windows with Winsock:

```bash
gcc -Wall -Wextra -std=c11 -o server.exe server.c -lws2_32
gcc -Wall -Wextra -std=c11 -o client.exe client.c -lws2_32
```

## Run

Open one terminal for Player 1 and start the server:

```bash
./server 5000
```

Open another terminal for Player 2 and connect the client:

```bash
./client 127.0.0.1 5000
```

On Windows, use `server.exe` and `client.exe` instead:

```bash
server.exe 5000
client.exe 127.0.0.1 5000
```

## Controls

- Player 1 enters commands directly in the server terminal.
- Player 2 enters commands in the client terminal when prompted.
- Press Enter after each answer.
- Type `y` or `n` when asked to play again.

## Socket Reference

The project uses a simple client-server TCP flow:

- Server: `socket()`, `bind()`, `listen()`, `accept()`, `send()`, `recv()`
- Client: `socket()`, `getaddrinfo()`, `connect()`, `send()`, `recv()`

## Files

- `server.c` - Main game state, rules, random card draws, and socket server.
- `client.c` - Client connection and Player 2 prompt handling.
- `server.sh` - Bash/netcat server script from an earlier prototype.
- `client.sh` - Bash/netcat client script from an earlier prototype.
