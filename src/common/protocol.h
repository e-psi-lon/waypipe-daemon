/**
 * @file protocol.h
 * @brief Protocol definitions for WaypipeDaemon client-server communication
 *
 * This file defines the message protocol used for communication between
 * the WaypipeDaemon server and its clients. It includes message structures,
 * types, and functions for creating, sending, receiving, and managing messages.
 */

#ifndef WAYPIPEDAEMON_PROTOCOL_H
#define WAYPIPEDAEMON_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Maximum size of a message in bytes (65 KB)
 *
 * This limit prevents excessive memory allocation and potential DoS attacks.
 */
#define MAX_MESSAGE_SIZE ((uint16_t)65536)

#define UINT8(x) ((uint8_t)(x))

/**
 * @brief Header for all protocol messages
 *
 * Contains metadata about the message type and payload length.
 */
typedef struct {
    uint8_t type;      /**< Message type identifier (see message_type_t) */
    uint16_t length;    /**< Length of the message payload in bytes */
} message_header_t;

/**
 * @brief Enumeration of all supported message types
 *
 * Defines the protocol message types for client-server communication.
 * Types 1-99 are reserved for command messages, 100+ for responses.
 */
typedef enum {
    MSG_HELLO = 1,          /**< Initial handshake message from a client */
    MSG_READY = 2,          /**< Server ready acknowledgment */
    MSG_SEND = 3,           /**< Data transmission message */
    MSG_RESPONSE_OK = 100,  /**< Success response */
    MSG_RESPONSE_ERROR = 101 /**< Error response */
} message_type_t;

/**
 * @brief Complete message structure with header and variable-length payload
 *
 * This structure uses a flexible array member (FAM) for the payload,
 * allowing variable-length messages without additional pointer indirection.
 * The length of the array is defined by the length field in the header.
 */
typedef struct {
    message_header_t header;  /**< Message header containing type and length */
    char data[];              /**< Variable-length payload data */
} message_t;

/**
 * @brief Convert a message type to its string representation
 *
 * @param type The message's type to convert
 * @param buf Buffer to store the resulting string
 * @param buf_size Size of the buffer in bytes
 */
void get_message_type_string(message_type_t type, char *buf, size_t buf_size);

/**
 * @brief Create a new message with the specified type and data
 *
 * Allocates memory for a message structure and copies the provided data
 * into it. The caller is responsible for freeing the returned message
 * using free_message().
 *
 * @param type The message type
 * @param data Pointer to the payload data (can be NULL if length is 0)
 * @param length Length of the payload data in bytes
 * @return Pointer to the newly created message, or NULL on failure
 */
message_t *create_message(message_type_t type, const char *data, size_t length);

/**
 * @brief Read a message from a socket
 *
 * Reads a complete message from the specified socket file descriptor.
 * This function blocks until a complete message is received or an error occurs.
 * The caller is responsible for freeing the returned message using free_message().
 *
 * @param sockfd Socket file descriptor to read from
 * @return Pointer to the received message, or NULL on error or connection close
 */
message_t *read_message(int sockfd);

/**
 * @brief Send a message through a socket
 *
 * Sends a complete message through the specified socket file descriptor.
 * This function ensures that the entire message (header and payload) is sent.
 *
 * @param sockfd Socket file descriptor to write to
 * @param msg Pointer to the message to send
 * @return 0 on success, -1 on error
 */
int send_message(int sockfd, const message_t *msg);

/**
 * @brief Free memory allocated for a message
 *
 * Frees the memory allocated by create_message() or read_message().
 * Null-safe: passing NULL has no effect.
 *
 * @param msg Pointer to the message to free (can be NULL)
 */
void free_message(message_t *msg);

/**
 * @brief Free an array of messages
 *
 * Convenience function to free multiple messages at once.
 * Frees each message in the array and can optionally free the array itself.
 *
 * @param msgs Array of message pointers to free
 * @param count Number of messages in the array
 */
void free_messages(message_t **msgs, size_t count);

#endif //WAYPIPEDAEMON_PROTOCOL_H