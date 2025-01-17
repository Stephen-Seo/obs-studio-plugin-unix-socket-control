// standard library includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// unix includes
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// local includes
#include "common_constants.h"

void print_usage(const char *name) {
    printf("Usage:\n");
    printf("  %s\n", name);
    printf("    [--start-recording \n");
    printf("   | --stop-recording\n");
    printf("   | --toggle-recording\n");
    printf("   | --start-streaming\n");
    printf("   | --stop-streaming\n");
    printf("   | --toggle-streaming\n");
    printf("   | --start-replay-buffer\n");
    printf("   | --stop-replay-buffer\n");
    printf("   | --toggle-replay-buffer\n");
    printf("   | --save-replay-buffer\n");
    printf("   | --get-status]\n");
    printf("    --wait\n");
}

void cleanup_data_socket(int *data_socket) {
    if (*data_socket >= 0) {
        close(*data_socket);
    }
}

int main(int argc, char **argv) {
    UnixSocketEventType type = UNIX_SOCKET_EVENT_NOP;

    const char *program_name = argv[0];
    ++argv;
    --argc;
    if (argc == 0) {
        print_usage(program_name);
        return 1;
    }
    while (argc != 0) {
        if (strncmp(argv[0], "--start-recording", 17) == 0) {
            type = UNIX_SOCKET_EVENT_START_RECORDING |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--stop-recording", 16) == 0) {
            type = UNIX_SOCKET_EVENT_STOP_RECORDING |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--start-streaming", 17) == 0) {
            type = UNIX_SOCKET_EVENT_START_STREAMING |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--toggle-recording", 18) == 0) {
            type = UNIX_SOCKET_EVENT_TOGGLE_RECORDING |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--stop-streaming", 16) == 0) {
            type = UNIX_SOCKET_EVENT_STOP_STREAMING |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--toggle-streaming", 18) == 0) {
            type = UNIX_SOCKET_EVENT_TOGGLE_STREAMING |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--start-replay-buffer", 21) == 0) {
            type = UNIX_SOCKET_EVENT_START_REPLAY_BUFFER |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--stop-replay-buffer", 20) == 0) {
            type = UNIX_SOCKET_EVENT_STOP_REPLAY_BUFFER |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--toggle-replay-buffer", 22) == 0) {
            type = UNIX_SOCKET_EVENT_TOGGLE_REPLAY_BUFFER |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--save-replay-buffer", 20) == 0) {
            type = UNIX_SOCKET_EVENT_SAVE_REPLAY_BUFFER |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--get-status", 12) == 0) {
            type = UNIX_SOCKET_EVENT_GET_STATUS |
                (UNIX_SOCKET_EVENT_WAIT & type);
        } else if (strncmp(argv[0], "--wait", 6) == 0) {
            type |= UNIX_SOCKET_EVENT_WAIT;
        } else {
            puts("ERROR: Invalid arg!");
            print_usage(program_name);
            return 2;
        }
        ++argv;
        --argc;
    }

    char socket_filename[108];
    snprintf(socket_filename, sizeof(socket_filename),
             UNIX_SOCKET_HANDLER_SOCKET_FMT_STRING, getenv("USER"));

    struct sockaddr_un addr;
    int ret;
    __attribute__((cleanup(cleanup_data_socket))) int data_socket = -1;
    unsigned char send_buf = (unsigned char)type;
    char buffer[8];

    memset(buffer, 0, sizeof(buffer));

    data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (data_socket == -1) {
        // Error. TODO handle this.
        return 3;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path,
            socket_filename,
            sizeof(addr.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr*) &addr, sizeof(addr));
    if (ret == -1) {
        // Error. TODO handle this.
        return 4;
    }

    ret = write(data_socket, &send_buf, 1);
    if (ret == -1) {
        // Error. TODO handle this.
        return 5;
    }

    ret = read(data_socket, buffer, sizeof(buffer));
    if (ret == -1 || ret < 8) {
        return 6;
    }

    if (type == UNIX_SOCKET_EVENT_GET_STATUS && buffer[0] == UNIX_SOCKET_EVENT_GET_STATUS) {
        printf("Is recording: %s\nIs streaming: %s\nReplay buffer active: %s\n",
               (buffer[1] & 1) != 0 ? "true" : "false",
               (buffer[1] & 2) != 0 ? "true" : "false",
               (buffer[1] & 4) != 0 ? "true" : "false");
    } else if (buffer[0] != UNIX_SOCKET_EVENT_NOP
            && buffer[0] != UNIX_SOCKET_EVENT_TOGGLE_RECORDING
            && buffer[0] != UNIX_SOCKET_EVENT_TOGGLE_STREAMING
            && buffer[0] != UNIX_SOCKET_EVENT_TOGGLE_REPLAY_BUFFER) {
        // Error. TODO handle this.
        return 7;
    } else {
        switch(type & UNIX_SOCKET_EVENT_MASK) {
            case UNIX_SOCKET_EVENT_START_RECORDING:
                puts("Sent event \"start recording\"!");
                break;
            case UNIX_SOCKET_EVENT_STOP_RECORDING:
                puts("Sent event \"stop recording\"!");
                break;
            case UNIX_SOCKET_EVENT_TOGGLE_RECORDING:
                switch(buffer[1]) {
                case UNIX_SOCKET_EVENT_START_RECORDING:
                    puts("Sent event \"toggle recording\", recording STARTED!");
                    break;
                case UNIX_SOCKET_EVENT_STOP_RECORDING:
                    puts("Sent event \"toggle recording\", recording STOPPED!");
                    break;
                default:
                    puts("Sent event \"toggle recording\", recording status UNKNOWN!");
                    break;
                }
                break;
            case UNIX_SOCKET_EVENT_START_STREAMING:
                puts("Sent event \"start streaming\"!");
                break;
            case UNIX_SOCKET_EVENT_STOP_STREAMING:
                puts("Sent event \"stop streaming\"!");
                break;
            case UNIX_SOCKET_EVENT_TOGGLE_STREAMING:
                switch(buffer[1]) {
                case UNIX_SOCKET_EVENT_START_STREAMING:
                    puts("Sent event \"toggle streaming\", stream STARTED!");
                    break;
                case UNIX_SOCKET_EVENT_STOP_STREAMING:
                    puts("Sent event \"toggle streaming\", stream STOPPED!");
                    break;
                default:
                    puts("Sent event \"toggle streaming\", stream status UNKNOWN!");
                    break;
                }
                break;
            case UNIX_SOCKET_EVENT_START_REPLAY_BUFFER:
                puts("Sent event \"start replay-buffer\"!");
                break;
            case UNIX_SOCKET_EVENT_STOP_REPLAY_BUFFER:
                puts("Sent event \"stop replay-buffer\"!");
                break;
            case UNIX_SOCKET_EVENT_TOGGLE_REPLAY_BUFFER:
                switch(buffer[1]) {
                case UNIX_SOCKET_EVENT_START_REPLAY_BUFFER:
                    puts("Sent event \"toggle replay-buffer\", replay-buffer STARTED!");
                    break;
                case UNIX_SOCKET_EVENT_STOP_REPLAY_BUFFER:
                    puts("Sent event \"toggle replay-buffer\", replay-buffer STOPPED!");
                    break;
                default:
                    puts("Sent event \"toggle replay-buffer\", replay-buffer status UNKNOWN!");
                    break;
                }
                break;
            case UNIX_SOCKET_EVENT_SAVE_REPLAY_BUFFER:
                puts("Sent event \"save replay-buffer\"!");
                break;
            default:
                // Error. TODO handle this
                return 8;
        }
    }

    return 0;
}
