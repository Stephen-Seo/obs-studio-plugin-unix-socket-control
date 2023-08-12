// plugin includes
#include <obs-module.h>
#include <obs-frontend-api.h>

// local includes
#include "socket.h"

static UnixSocketHandler global_unix_socket_handler={
    .flags = 0xFFFFFFFFFFFFFFFF
};

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("unix-socket-control", "en-US")

bool obs_module_load(void) {
    init_unix_socket_handler(&global_unix_socket_handler);

    return is_unix_socket_handler_valid(global_unix_socket_handler) ? true : false;
}

void obs_module_unload(void) {
    cleanup_unix_socket_handler(&global_unix_socket_handler);
}
