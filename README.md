# Packet Salvage

## Proposal Details

1. **Game Title:** Packet Salvage
2. **Game Description:** Packet Salvage is an original cooperative command-line game where players act as a repair crew for a damaged network relay. The server controls the game state, creates random network faults, and broadcasts updates. Each client connects through TCP sockets and sends turn commands to help keep packet integrity above zero.
3. **Game Mechanics:**
   - Start the server, then connect one to three clients.
   - Players take turns sending commands from their own client terminal.
   - `scan` displays the relay nodes and their current condition.
   - `patch <node>` repairs an active fault or restores health to a clear node.
   - `route <node>` reinforces a node so it can survive future faults.
   - `shield` restores packet integrity.
   - The team wins after repairing enough faults. The team loses if packet integrity reaches zero.
4. **Names of 3 Members:**
   - Member 1:
   - Member 2:
   - Member 3:

## How to Run

Open one terminal for the server:

```bash
bash server.sh 5050
```

Open another terminal for the client:

```bash
bash client.sh 127.0.0.1 5050
```

The `.sh` files compile the C files first, then run the game.

Manual compile commands:

```bash
gcc -Wall -Wextra -std=c11 -o server server.c
gcc -Wall -Wextra -std=c11 -o client client.c
```

On Windows/MSYS2 or Git Bash, add Winsock:

```bash
gcc -Wall -Wextra -std=c11 -o server.exe server.c -lws2_32
gcc -Wall -Wextra -std=c11 -o client.exe client.c -lws2_32
```

## Socket Reference

This project follows the client-server socket flow from the lab exercise:

- Server: creates a TCP socket, then uses `bind()`, `listen()`, `accept()`, `send()`, and `recv()`.
- Client: creates a TCP socket, then uses `connect()`, `send()`, and `recv()`.
