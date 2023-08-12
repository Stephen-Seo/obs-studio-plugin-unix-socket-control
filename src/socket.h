#ifndef OBS_STUDIO_PLUGIN_UNIX_SOCKET_CONTROL_SOCKET_H_
#define OBS_STUDIO_PLUGIN_UNIX_SOCKET_CONTROL_SOCKET_H_

// standard library includes
#include <stdint.h>

// unix includes
#include <sys/socket.h>
#include <sys/un.h>

typedef enum UnixSocketEventType {
    UNIX_SOCKET_EVENT_NOP,
    UNIX_SOCKET_EVENT_HEAD,
    UNIX_SOCKET_EVENT_TAIL,
    UNIX_SOCKET_EVENT_START_RECORDING,
    UNIX_SOCKET_EVENT_STOP_RECORDING,
    UNIX_SOCKET_EVENT_START_STREAMING,
    UNIX_SOCKET_EVENT_STOP_STREAMING
} UnixSocketEventType;

typedef struct UnixSocketEvent {
    struct UnixSocketEvent *next;
    struct UnixSocketEvent *prev;
    UnixSocketEventType type;
} UnixSocketEvent;

typedef struct UnixSocketHandler {
    /*
     * All ones - invalid/cleaned-up instance
     */
    uint64_t flags;
    UnixSocketEvent events_head;
    UnixSocketEvent events_tail;
    struct sockaddr_un name;
    int socket_descriptor;
} UnixSocketHandler;

UnixSocketHandler init_unix_socket_handler(void);
void cleanup_unix_socket_handler(UnixSocketHandler *handler);
int is_unix_socket_handler_valid(UnixSocketHandler handler);

#endif
