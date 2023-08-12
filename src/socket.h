#ifndef OBS_STUDIO_PLUGIN_UNIX_SOCKET_CONTROL_SOCKET_H_
#define OBS_STUDIO_PLUGIN_UNIX_SOCKET_CONTROL_SOCKET_H_

// standard library includes
#include <stdint.h>
#include <threads.h>

// unix includes
#include <sys/socket.h>
#include <sys/un.h>

// local includes
#include "common_constants.h"

typedef struct UnixSocketHandler {
    /*
     * All ones - invalid/cleaned-up instance
     */
    uint64_t flags;
    struct sockaddr_un name;
    thrd_t *thread;
    mtx_t *mutex;
    /*
     * ???? 0001 - thread should stop
     */
    volatile uint64_t ccflags;

    int socket_descriptor;
} UnixSocketHandler;

void init_unix_socket_handler(UnixSocketHandler *handler);
void cleanup_unix_socket_handler(UnixSocketHandler *handler);
int is_unix_socket_handler_valid(UnixSocketHandler handler);

#endif
