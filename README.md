# tcp-employee-db

Multi-client TCP server in C. Manages employee records over a custom binary protocol, persists to a binary file. No threads — uses `poll()` to handle multiple clients concurrently.

Built as a systems programming project to get hands-on with socket programming, binary protocols, and event-driven I/O.

## How it works

Clients connect and go through a handshake before sending commands. The server tracks each client's state with a simple FSM (CONNECTED → HELLO → MSG). Invalid messages get an error response.

Protocol messages use a fixed 4-byte header (`type` + `len`, network byte order), followed by a payload. The database file has a magic number + filesize in the header so the server catches corruption on startup.

## Build & run

```bash
mkdir -p bin obj
make
```

Start server:
```bash
./bin/final -n -f mydb.db -p 8080   # -n creates new db
./bin/final -f mydb.db -p 8080      # open existing
```

Run client:
```bash
./src/client/main -h 127.0.0.1 -p 8080
```

Client commands: `add Name,Address,Hours` | `list` | `remove Name` | `quit`

## Structure
