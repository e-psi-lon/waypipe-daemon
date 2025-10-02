# Waypipe Daemon

Waypipe Daemon is a lightweight daemon that facilitates the use of Waypipe, a tool for running graphical applications over SSH using the Wayland protocol. The daemon manages a single Waypipe session and uses multiplexing to handle multiple applications.

## Architecture

### Daemon

The daemon provides a persistent Waypipe session manager that runs in the background. Written in C for performance, it handles the lifecycle of remote graphical applications.

### Client

The client is lightweight and short-lived. It starts the daemon (if not already started by a previous client), sends a request to launch an application, and exits. This design minimizes overhead and keeps the process list clean, since the daemon is the only process that remains actively running. The client is also written in C for performance and consistency.