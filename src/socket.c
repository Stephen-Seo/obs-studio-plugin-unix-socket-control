#include "socket.h"
#include "common_constants.h"

// standard library includes
#include <stdlib.h>
#include <stdio.h>

// unix includes
#include <string.h>
#include <sys/stat.h>
#include <threads.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// obs-studio includes
#include <obs-frontend-api.h>

int unix_socket_handler_thread_function(void *ud) {
    UnixSocketHandler *handler = (UnixSocketHandler*)ud;

    struct timespec duration;
    duration.tv_sec = 0;
    duration.tv_nsec = 10000000;
    int ret;
    int data_socket;
    char buffer[8];
    char ret_buffer[8];
    unsigned int ticks;

    while(1) {
        mtx_lock(handler->mutex);
        if ((handler->ccflags & 1) != 0) {
            mtx_unlock(handler->mutex);
            return 0;
        }
        mtx_unlock(handler->mutex);

        data_socket = accept(handler->socket_descriptor, 0, 0);
        if (data_socket == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                thrd_sleep(&duration, 0);
                continue;
            }
            // Error. TODO handle this.
            break;
        }

        ticks = 0;
        memset(ret_buffer, 0, sizeof(ret_buffer));
        while (1) {
            mtx_lock(handler->mutex);
            if ((handler->ccflags & 1) != 0) {
                mtx_unlock(handler->mutex);
                close(data_socket);
                return 0;
            }
            mtx_unlock(handler->mutex);

            ret = read(data_socket, buffer, sizeof(buffer));
            if (ret == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    if (++ticks > 200) {
                        // Timed out.
                        ret = -1;
                        break;
                    }
                    thrd_sleep(&duration, 0);
                    continue;
                }
                // Error. TODO handle this.
                break;
            }

            if (buffer[0] == UNIX_SOCKET_EVENT_START_RECORDING) {
                obs_frontend_recording_start();
                ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
            } else if (buffer[0] == UNIX_SOCKET_EVENT_STOP_RECORDING) {
                obs_frontend_recording_stop();
                ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
            } else if (buffer[0] == UNIX_SOCKET_EVENT_TOGGLE_RECORDING) {
                ret_buffer[0] = UNIX_SOCKET_EVENT_TOGGLE_RECORDING;
                if (obs_frontend_recording_active()) {
                    obs_frontend_recording_stop();
                    ret_buffer[1] = UNIX_SOCKET_EVENT_STOP_RECORDING;
                } else {
                    obs_frontend_recording_start();
                    ret_buffer[1] = UNIX_SOCKET_EVENT_START_RECORDING;
                }
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
                if (obs_frontend_replay_buffer_active()) {
                    ret_buffer[1] |= 4;
                }
            } else if (buffer[0] == UNIX_SOCKET_EVENT_START_REPLAY_BUFFER) {
                obs_frontend_replay_buffer_start();
                ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
            } else if (buffer[0] == UNIX_SOCKET_EVENT_STOP_REPLAY_BUFFER) {
                obs_frontend_replay_buffer_stop();
                ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
            } else if (buffer[0] == UNIX_SOCKET_EVENT_SAVE_REPLAY_BUFFER) {
                if (obs_frontend_replay_buffer_active()) {
                    obs_frontend_replay_buffer_save();
                    ret_buffer[0] = UNIX_SOCKET_EVENT_NOP;
                } else {
                    ret = -1;
                    break;
                }
            }
            ret = 0;
            break;
        }

        if (ret == 0) {
            ret = write(data_socket, ret_buffer, sizeof(ret_buffer));
            if (ret == -1) {
                // Error. TODO handle this.
                close(data_socket);
                continue;
            }
        }

        close(data_socket);
    }

    return 0;
}

void init_unix_socket_handler(UnixSocketHandler *handler) {
    memset(handler, 0, sizeof(UnixSocketHandler));

    snprintf(handler->socket_filename, sizeof(handler->socket_filename),
             UNIX_SOCKET_HANDLER_SOCKET_FMT_STRING, getenv("USER"));

    umask(S_IRWXO);

    // Set up unix socket.
    handler->socket_descriptor = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (handler->socket_descriptor == -1) {
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        fprintf(stderr, "ERROR: unix-socket-control: Failed to create socket!\n");
        return;
    }

    int ret = fcntl(handler->socket_descriptor, F_SETFL, O_NONBLOCK, 1);
    if (ret == -1) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        fprintf(stderr, "ERROR: unix-socket-control: Failed to set socket non-blocking!\n");
        return;
    }

    handler->name.sun_family = AF_UNIX;
    strncpy(handler->name.sun_path,
            handler->socket_filename,
            sizeof(handler->name.sun_path) - 1);

    ret = bind(handler->socket_descriptor,
                   (const struct sockaddr*) &handler->name,
                   sizeof(handler->name));
    if (ret == -1) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        fprintf(stderr, "ERROR: unix-socket-control: Failed to bind socket to filename!\n");
        return;
    }

    ret = listen(handler->socket_descriptor, 20);
    if (ret == -1) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        unlink(handler->socket_filename);
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        fprintf(stderr, "ERROR: unix-socket-control: Failed to set socket listen!\n");
        return;
    }

    // Set up concurrency.
    handler->mutex = malloc(sizeof(mtx_t));
    ret = mtx_init(handler->mutex, mtx_plain);
    if (ret != thrd_success) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        unlink(handler->socket_filename);
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        fprintf(stderr, "ERROR: unix-socket-control: Failed to init mutex!\n");
        return;
    }

    handler->thread = malloc(sizeof(thrd_t));
    ret = thrd_create(handler->thread,
                      unix_socket_handler_thread_function,
                      handler);
    if (ret != thrd_success) {
        close(handler->socket_descriptor);
        handler->socket_descriptor = -1;
        unlink(handler->socket_filename);
        mtx_destroy(handler->mutex);
        handler->flags = 0xFFFFFFFFFFFFFFFF;
        fprintf(stderr, "ERROR: unix-socket-control: Failed to init thread!\n");
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
        unlink(handler->socket_filename);
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

int is_unix_socket_handler_valid(const UnixSocketHandler *handler) {
    return handler->flags == 0xFFFFFFFFFFFFFFFF ? 0 : 1;
}
