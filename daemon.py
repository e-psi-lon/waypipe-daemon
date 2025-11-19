#!/usr/bin/env python3
"""
Stub Python daemon for WaypipeDaemon.
Listens on a Unix socket and responds to client messages.
"""

import socket
import struct
import sys
import os
from pathlib import Path

# Protocol constants (must match C client)
MSG_HELLO = 1
MSG_READY = 2
MSG_SEND = 3
MSG_RESPONSE_OK = 100
MSG_RESPONSE_ERROR = 101

def get_socket_path():
    """Get the socket path for the current user."""
    uid = os.getuid()
    return f"/run/user/{uid}/waypipe-daemon.sock"

def create_message(msg_type, data=None):
    """Create a message with header and optional data."""
    if data is None:
        data = b""
    elif isinstance(data, str):
        data = data.encode('utf-8')
    
    # Pack header: type (uint32) + length (uint32)
    header = struct.pack('II', msg_type, len(data))
    return header + data

def parse_message(data):
    """Parse a message and return (type, payload)."""
    if len(data) < 8:
        return None, None
    
    msg_type, length = struct.unpack('II', data[:8])
    payload = data[8:8+length]
    return msg_type, payload

def handle_client(client_socket, client_addr, ready_already_sent=False):
    """Handle a single client connection."""
    try:
        print(f"[*] Client connected from {client_addr}")

        # If READY was already sent (first connection with --send-ready), skip to reading command
        if ready_already_sent:
            print(f"[*] READY already sent, waiting for command...")
            data = client_socket.recv(4096)
            if not data:
                print(f"[!] Client disconnected without sending command")
                return

            msg_type, payload = parse_message(data)

            if msg_type == MSG_SEND:
                command = payload.decode('utf-8', errors='ignore')
                print(f"[*] Received command: \"{command}\"")

                # Send success response
                response = create_message(MSG_RESPONSE_OK, "Command received")
                client_socket.sendall(response)
                print(f"[*] Sent MSG_RESPONSE_OK")
            else:
                print(f"[!] Expected MSG_SEND, got {msg_type}")
            return

        # Normal flow: expect HELLO first
        # Read initial message
        data = client_socket.recv(4096)
        if not data:
            print(f"[!] Client disconnected without sending message")
            return
        
        msg_type, payload = parse_message(data)
        
        if msg_type == MSG_HELLO:
            print(f"[*] Received MSG_HELLO from client")
            # Send MSG_READY response
            response = create_message(MSG_READY)
            client_socket.sendall(response)
            print(f"[*] Sent MSG_READY")

            # Wait for command
            data = client_socket.recv(4096)
            if not data:
                print(f"[!] Client disconnected after HELLO")
                return
            
            msg_type, payload = parse_message(data)
            
            if msg_type == MSG_SEND:
                command = payload.decode('utf-8', errors='ignore')
                print(f"[*] Received command: \"{command}\"")

                # Send success response
                response = create_message(MSG_RESPONSE_OK, "Command received")
                client_socket.sendall(response)
                print(f"[*] Sent MSG_RESPONSE_OK")
            else:
                print(f"[!] Unexpected message type: {msg_type}")
        else:
            print(f"[!] Expected MSG_HELLO, got {msg_type}")

    except Exception as e:
        print(f"[!] Error handling client: {e}")
    finally:
        client_socket.close()
        print(f"[*] Client disconnected")

def main():
    """Main daemon loop."""
    # Check for send-ready flag
    send_ready = "--send-ready" in sys.argv

    socket_path = get_socket_path()
    
    # Remove existing socket if it exists
    if os.path.exists(socket_path):
        os.remove(socket_path)
    
    # Create Unix socket
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(socket_path)
    sock.listen(1)
    
    print(f"[*] Daemon listening on {socket_path}")

    # If send_ready flag is set, wait for the first client connection and send READY
    if send_ready:
        try:
            print(f"[*] Waiting for client connection to send READY signal...")
            client_socket, client_addr = sock.accept()
            print(f"[*] Client connected, sending READY signal...")

            # Send READY message to the connecting client
            ready_msg = create_message(MSG_READY)
            client_socket.sendall(ready_msg)
            print(f"[*] READY signal sent. Processing first client...")

            # Handle the first client (already sent READY, so skip that step)
            handle_client(client_socket, client_addr, ready_already_sent=True)
        except Exception as e:
            print(f"[!] Error sending READY signal: {e}")

    try:
        while True:
            client_socket, client_addr = sock.accept()
            handle_client(client_socket, client_addr, ready_already_sent=False)
    except KeyboardInterrupt:
        print("[*] Daemon shutting down...")
    finally:
        sock.close()
        if os.path.exists(socket_path):
            os.remove(socket_path)
        print("[*] Daemon stopped")

if __name__ == "__main__":
    main()

