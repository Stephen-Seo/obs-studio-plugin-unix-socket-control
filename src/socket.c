#include "socket.h"

// standard library includes
#include <stdlib.h>

// unix includes
#include <string.h>
#include <sys/stat.h>
#include <threads.h>
#include <unistd.h>

// obs-studio includes
#include <obs-frontend-api.h>

int unix_socket_handler_thread_function(void *ud) {
    UnixSocketHandler *handler = (UnixSocketHandler*)ud;

    int ret;
    int data_socket;
    char buffer[8];
    char ret_buffer[8];

    while(1) {
        mtx_lock(handler->mutex);
        if ((handler->ccflags & 1) != 0) {
            mtx_unlock(handler->mutex);
            break;
        }
        mtx_unlock(handler->mutex);

        data_socket = accept(handler->socket_descriptor, 0, 0);
        if (data_socket == -1) {
            // Error. TODO handle this.
            break;
        }

        memset(ret_buffer, 0, sizeof(ret_buffer));

        ret = read(data_socket, buffer, sizeof(buffer));
        if (ret == -1) {
            // Error. TODO handle this.
            break;
        }

        if (buffer[0] == UNIX_SOCKET_EVENT_START_RECORDING) {
            obs_frontend_recording_start();
            ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
        } else if (buffer[0] == UNIX_SOCKET_EVENT_STOP_RECORDING) {
            obs_frontend_recording_stop();
            ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
        } else if (buffer[0] == UNIX_SOCKET_EVENT_START_STREAMING) {
            obs_frontend_streaming_start();
            ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
        } else if (buffer[0] == UNIX_SOCKET_EVENT_STOP_STREAMING) {
            obs_frontend_streaming_stop();
            ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
        } else if (buffer[0] == UNIX_SOCKET_EVENT_GET_STATUS) {
            ret_buffer[0] = UNIX_SOCKET_EVENT_GET_STATUS;
            if (obs_frontend_recording_active()) {
                ret_buffer[1] |= 1;
            }
            if (obs_frontend_streaming_active()) {
                ret_buffer[1] |= 2;
            }
        }

        ret = write(data_socket, ret_buffer, sizeof(ret_buffer));
        if (ret == -1) {
            // Error. TODO handle this.
            break;
        }

        close(data_socket);
    }

    return 0;
}

void init_unix_socket_handler(UnixSocketHandler *handler) {
    memset(handler, 0, sizeof(UnixSocketHandler));

    umask(S_IRWXO);

    // Set up unix socket.
    handler->socket_descriptor = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (handler->socket_descriptor == -1) {
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        return;
    }

    handler->name.sun_family = AF_UNIX;
    strncpy(handler->name.sun_path,
            UNIX_SOCKET_HANDLER_SOCKET_NAME,
            sizeof(handler->name.sun_path) - 1);

    int ret = bind(handler->socket_descriptor,
                   (const struct sockaddr*) &handler->name,
                   sizeof(handler->name));
    if (ret == -1) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        return;
    }

    ret = listen(handler->socket_descriptor, 20);
    if (ret == -1) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        unlink(UNIX_SOCKET_HANDLER_SOCKET_NAME);
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        return;
    }

    // Set up concurrency.
    handler->mutex = malloc(sizeof(mtx_t));
    ret = mtx_init(handler->mutex, mtx_plain);
    if (ret != thrd_success) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        unlink(UNIX_SOCKET_HANDLER_SOCKET_NAME);
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        return;
    }

    handler->thread = malloc(sizeof(thrd_t));
    ret = thrd_create(handler->thread,
                      unix_socket_handler_thread_function,
                      handler);
    if (ret != thrd_success) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        unlink(UNIX_SOCKET_HANDLER_SOCKET_NAME);
        mtx_destroy(handler->mutex);
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        return;
    }

    return;
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
    if (handler->mutex) {
        mtx_lock(handler->mutex);
        handler->ccflags |= 1;
        mtx_unlock(handler->mutex);
        thrd_join(*handler->thread, 0);

        free(handler->thread);
        handler->thread = 0;

        mtx_destroy(handler->mutex);
        free(handler->mutex);
        handler->mutex = 0;
    }

    handler->flags = 0xFFFFFFFFFFFFFFFF;
}

int is_unix_socket_handler_valid(UnixSocketHandler handler) {
    return handler.flags == 0xFFFFFFFFFFFFFFFF ? 0 : 1;
}
