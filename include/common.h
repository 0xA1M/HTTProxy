#ifndef COMMON_H
#define COMMON_H

/* Standard Libraries */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX Networking Library */
#include <netdb.h>

/* POSIX Multi-Threading Library */
#include <pthread.h>

/* GNU Error Number Library */
#include <errno.h>

/* Logging Library */
#include "clog.h"

/* Parser */
#include "parser.h"

/* Thread Pool */
#define MAX_THREADS 64 // Handle 64 simultaneous connections

extern pthread_t thread_pool[MAX_THREADS];
extern int thread_count;
extern pthread_mutex_t lock;

#define MAX_HTTP_LEN 8192    // Max length of the http request/response
#define MAX_HOSTNAME_LEN 256 // Max length of the hostname
#define MAX_PORT_LEN 6       // Max length of the port number
#define PROXY_TID_INDEX 0    // Proxy thread id index in the thread pool array

/*****************************************************
 *            Thread Management Functions            *
 *****************************************************/

/**
 * @brief Find an empty slot in the thread pool.
 *
 * This function searches for an available slot in the `thread_pool` array,
 * which represents the thread pool's current state. If an empty slot (0) is
 * found, the function returns the index of the slot.
 *
 * @returns
 *   - Index of the first empty slot (>=1) on success.
 *   - -1 if there is no available slot or the thread pool is at capacity.
 *
 * @note
 *   This function locks `lock` for mutual exclusion as it reads and modifies
 *   shared resources (`thread_count` and `thread_pool`). It ensures no other
 *   thread can modify these variables concurrently.
 */
int find_empty_slot(void);

/**
 * @brief Remove a thread from the thread pool.
 *
 * This function removes a thread identified by `tid` (thread ID) from the
 * `thread_pool` array. If the thread is found, it is removed (set to 0),
 * and the `thread_count` is decremented.
 *
 * @parameters:
 *   - @param tid: Thread ID to remove from the pool.
 *
 * @note
 *   The function locks `lock` to ensure thread-safe access to shared
 *   resources (`thread_pool` and `thread_count`).
 *
 * @note
 *   If the specified thread ID is not found in the `thread_pool`, an error
 *   is logged, and the program exits.
 */
void remove_thread(const pthread_t tid);

/*********************************************************
 *            Connection Management Functions            *
 *********************************************************/

/**
 * @brief Forward data to a specified destination file descriptor.
 *
 * This function attempts to send `len` bytes of data from the `buffer` to the
 * destination file descriptor (`dest_fd`). It continues sending until all
 * data is transferred or an error occurs.
 *
 * @parameters:
 *   - @param dest_fd: File descriptor of the destination socket to which data
 *     should be sent.
 *   - @param buffer: Pointer to the data buffer to be sent.
 *   - @param len: Length of the data in the buffer.
 *
 * @returns
 *   - 0 on successful data transfer.
 *   - -1 if an error occurs during sending.
 *
 * @note
 *   This function handles partial sends by attempting to resend until all
 *   data has been forwarded.
 */
int forward(const int dest_fd, const unsigned char *buffer, const long len);

/*****************************************************
 *            Memory Management Functions            *
 *****************************************************/

/**
 * @brief Free the memory associated with a Request structure.
 *
 * This function releases all dynamically allocated memory within a `Request`
 * structure, including the request line (method, URI, version), headers, and
 * body. Finally, it frees the `Request` struct itself and sets the original
 * pointer to `NULL` to prevent dangling references.
 *
 * @parameters:
 *   - @param req: Double pointer to the `Request` structure to be freed.
 */
void free_req(Request **req);

/**
 * @brief Free the memory associated with a Response structure.
 *
 * This function releases all dynamically allocated memory within a `Response`
 * structure, including the response line (version, status code, reason phrase),
 * headers, and body. Finally, it frees the `Response` struct itself and sets
 * the original pointer to `NULL` to prevent dangling references.
 *
 * @parameters:
 *   - @param res: Double pointer to the `Response` structure to be freed.
 */
void free_res(Response **res);

/*********************************************************
 *            HTTP Message Printing Functions            *
 *********************************************************/

/**
 * @brief Prints an HTTP request message to stdout with formatted styling
 *
 * @param req Pointer to a Request structure containing the HTTP request details
 *           Must not be NULL
 *
 * @details Displays the following components:
 *          - Request line (method, URI, version)
 *          - Headers as key-value pairs
 *          - Message body (if present)
 *          All output is formatted with visual separators and styling
 */
void print_res(const Response *res);

/**
 * @brief Prints an HTTP response message to stdout with formatted styling
 *
 * @param res Pointer to a Response structure containing the HTTP response
 * details Must not be NULL
 *
 * @details Displays the following components:
 *          - Response line (version, status code, reason phrase)
 *          - Headers as key-value pairs
 *          - Message body (if present)
 *          All output is formatted with visual separators and styling
 *
 */
void print_req(const Request *req);

/**
 * @brief Prints a buffer's contents in hexadecimal format
 *
 * This function takes a buffer and prints its contents as hexadecimal values,
 * formatting the output in rows of 32 bytes. Each byte is printed as a
 * two-digit hexadecimal number followed by a space. A newline is inserted after
 * every 32 bytes to improve readability.
 *
 * Example output:
 * 48 45 4C 4C 4F 20 57 4F 52 4C 44 21 00 ...
 * 54 48 49 53 20 49 53 20 41 20 54 45 53 ...
 *
 * @param buffer     Pointer to the buffer containing the data to be printed.
 * @param buffer_len Length of the buffer in bytes
 */
void print_hex(const unsigned char *buffer, const size_t buffer_len);

/********************************************************
 *            HTTP Message Parsing Functions            *
 ********************************************************/

/**
 * @brief Searches for and returns the value of a specified HTTP header
 *
 * @param target The header name to search for
 * @param headers Array of Header structures containing key-value pairs
 * @param headers_count Number of headers in the array
 *
 * @return Pointer to the header value if found, NULL if not found
 *
 * @note The returned pointer points to the original header value and should not
 * be modified
 */
const char *get_header_value(const char *target, const Header *headers,
                             const size_t headers_count);

/**
 * @brief Parses and returns the size of an HTTP chunked transfer encoding chunk
 *
 * This function extracts and converts the hexadecimal chunk size from a chunk
 * header. The chunk header format should be: "<size in hex>\r\n"
 *
 * @param chunk Pointer to the start of the chunk header
 * @param chunk_len Length of the available chunk data
 *
 * @return The chunk size as an integer, or -1 if parsing fails
 *
 * @note The function expects the chunk to start with a hex number followed by
 * \r\n
 * @note The maximum chunk size is limited by the size_t type
 */
int get_chunk_size(const unsigned char *chunk, const size_t chunk_len);

#endif /* COMMON_H */
