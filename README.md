
# CSIE3510 Real-Time Online Chatroom Application - Phase 1

This is a simple client-server chatroom application for the CSIE3510 Socket Programming Project, created in C using TCP sockets. 
Phase 1 covers basic client-server communication with user registration, login, logout, and graceful exit.

## Project Structure

- **server.c**: The server program that listens for client connections, manages user registration and authentication, and handles commands from the client.
- **client.c**: The client program that connects to the server, sends commands, and receives responses.

- **users.txt**: The text file that stores the username and corresponding password.

## Features

1. **User Registration**: Allows new users to register with a username and password.
2. **User Login and Logout**: Registered users can log in and log out securely.
3. **Exit**: Clients can disconnect gracefully by typing the `EXIT` command.
4. **Error Handling**: Appropriate responses are provided for unknown or invalid commands.

## How to Compile and Run

### Step 1: Compile the Code

Run the following commands in the terminal to compile both server and client programs:

```bash
gcc server.c -o server
gcc client.c -o client
```
or simply run the Makefile.
### Step 2: Run the Server

Run the server program with:

```bash
./server
```

The server will start listening on port `8080` for incoming client connections.

### Step 3: Run the Client

Open a new terminal and run the client program with:

```bash
./client
```

The client will attempt to connect to the server running on `127.0.0.1` (localhost) and port `8080`.

## Usage Guide

1. **Register a New User**:
    - Type `REGISTER` and follow the prompts to enter a new username and password.

2. **Login**:
    - Type `LOGIN` and enter your username and password.

3. **Logout**:
    - Type `LOGOUT` to log out of the server.

4. **Exit**:
    - Type `EXIT` to disconnect from the server and close the client application.

### Example Commands

- `REGISTER` - Register a new user.
- `LOGIN` - Login with an existing username and password.
- `LOGOUT` - Logout from the server.
- `EXIT` - Close the client connection.

## Requirements

- **C Compiler**: Ensure `gcc` or an equivalent C compiler is installed on your system.
- **Environment**: This program is intended to run on Unix/Linux systems.

## Notes

- **Data Persistence**: User data is saved in `users.txt` on the server side.
- **Error Handling**: The server provides error messages for unrecognized commands, failed logins, and registration issues.

## Authors

- Created for CSIE3510 by Hsin Wei, Chen.

