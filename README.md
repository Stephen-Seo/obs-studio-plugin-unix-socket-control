# obs-studio-plugin-unix-socket-control

An obs-studio plugin that accepts commands over a unix socket to start/stop
recording/streaming or shows recording/streaming active status.

This exists because keyboard shortcuts don't work on Wayland, and I wanted a
workaround to tell obs-studio to start/stop recording (via the use of invoking a
toggle script that uses this plugin's client with a configured Sway keyboard
shortcut).
