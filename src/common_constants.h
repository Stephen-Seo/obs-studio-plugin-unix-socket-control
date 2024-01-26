#ifndef OBS_STUDIO_PLUGIN_UNIX_SOCKET_CONTROL_COMMON_CONSTANTS_H_
#define OBS_STUDIO_PLUGIN_UNIX_SOCKET_CONTROL_COMMON_CONSTANTS_H_

#define UNIX_SOCKET_HANDLER_SOCKET_FMT_STRING "/tmp/%s_obs-studio-plugin-unix-socket-handler-socket"

typedef enum UnixSocketEventType {
    UNIX_SOCKET_EVENT_NOP = 0,
    UNIX_SOCKET_EVENT_START_RECORDING,
    UNIX_SOCKET_EVENT_STOP_RECORDING,
    UNIX_SOCKET_EVENT_START_STREAMING,
    UNIX_SOCKET_EVENT_STOP_STREAMING,
    UNIX_SOCKET_EVENT_GET_STATUS,
    UNIX_SOCKET_EVENT_START_REPLAY_BUFFER,
    UNIX_SOCKET_EVENT_STOP_REPLAY_BUFFER,
    UNIX_SOCKET_EVENT_SAVE_REPLAY_BUFFER,
    UNIX_SOCKET_EVENT_TOGGLE_RECORDING
} UnixSocketEventType;

#endif
