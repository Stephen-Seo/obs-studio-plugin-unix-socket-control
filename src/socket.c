#include "socket.h"

// standard library includes
#include <stdlib.h>

// unix includes
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define UNIX_SOCKET_HANDLER_SOCKET_NAME "/tmp/obs-studio-plugin-unix-socket-handler-socket"

UnixSocketHandler init_unix_socket_handler(void) {
    UnixSocketHandler handler;

    memset(&handler, 0, sizeof(UnixSocketHandler));

    umask(S_IRWXO);

    // Set up linked list of events.
    handler.events_head.next = &handler.events_tail;
    handler.events_tail.prev = &handler.events_head;

    handler.events_head.type = UNIX_SOCKET_EVENT_HEAD;
    handler.events_tail.type = UNIX_SOCKET_EVENT_TAIL;

    // Set up unix socket.
    handler.socket_descriptor = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (handler.socket_descriptor == -1) {
        handler.flags = 0xFFFFFFFFFFFFFFFF;
        return handler;
    }

    handler.name.sun_family = AF_UNIX;
    strncpy(handler.name.sun_path,
            UNIX_SOCKET_HANDLER_SOCKET_NAME,
            sizeof(handler.name.sun_path) - 1);

    int ret = bind(handler.socket_descriptor,
                   (const struct sockaddr*) &handler.name,
                   sizeof(handler.name));
    if (ret == -1) {
        close(handler.socket_descriptor);
        handler.socket_descriptor = -1;
        handler.flags = 0xFFFFFFFFFFFFFFFF;
        return handler;
    }

    return handler;
}

void cleanup_unix_socket_handler(UnixSocketHandler *handler) {
    if (handler->flags == 0xFFFFFFFFFFFFFFFF) {
        return;
    }

    if (handler->socket_descriptor >= 0) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        unlink(UNIX_SOCKET_HANDLER_SOCKET_NAME);
    }
    handler->flags = 0xFFFFFFFFFFFFFFFF;
}

int is_unix_socket_handler_valid(UnixSocketHandler handler) {
    return handler.flags == 0xFFFFFFFFFFFFFFFF ? 0 : 1;
}
