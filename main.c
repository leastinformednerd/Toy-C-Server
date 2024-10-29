#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <threads.h>
#include <stdatomic.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define PENINDG_CONNECTIONS_MAX 100

// Handles error in the init (sync) portion of the program
// Since this is a toy server it just closes connection silently on failure in
// multithreaded contexts
void handle_error(char* msg) {
    perror(msg);
    exit(-1);
}

atomic_bool accepting_new_connections = 1;

atomic_uint_least16_t existing_connections = 0;

// Cleanup function for a thread 
void thread_exit(int conn_fd, void* buffer, int res) {
    if (buffer)
        free(buffer);

    close(conn_fd);
    atomic_fetch_sub(&existing_connections, 1);

    thrd_exit(res);
}

// Since this function's only argument is an integral type we can just pass it as the pointer and cast
int process_connection(void* _conn_fd) {
    int conn_fd = (int) _conn_fd;
    atomic_fetch_add(&existing_connections, 1);

    int64_t size;
    if (read(conn_fd, &size, 8) < 8)
        thread_exit(conn_fd, NULL, -1);

    // This is used a signal to stop accepting new connections
    // Existing connections will be nicely cleaned up
    if (size < 0) {
        atomic_store(&accepting_new_connections, 0);
        thread_exit(conn_fd, NULL, 0);
    }

    char* buffer = malloc(size);

    if (read(conn_fd, buffer, size) != size)
        thread_exit(conn_fd, (void*) buffer, -1);

    char* rev_buffer = malloc(size);

    for (int pos = 0; pos != size; pos++)
        rev_buffer[size - 1 - pos] = buffer[pos];

    free(buffer);

    if (write(conn_fd, rev_buffer, size) != size)
        thread_exit(conn_fd, (void*) rev_buffer, -1);
    
    thread_exit(conn_fd, (void*) rev_buffer, 0);
}

// Accept a connection and spin up a thread to respond to it
void try_handle_connection(int sock) {
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_size = sizeof(peer_addr);

    int curfd = accept(
        sock,
        (struct sockaddr*) &peer_addr,
        &peer_addr_size
    );

    if (curfd == -1) {
        atomic_store(&accepting_new_connections, 0);
        printf("Failed to accept. Entering graceful shutdown");
        return;
    }

    if (!atomic_load(&accepting_new_connections)) {
        close(curfd);

        if (atomic_load(&existing_connections) == 0){ 
            close(sock);
            exit(0);
        }

        return;
    }

    thrd_t _thread_id;

    thrd_create(
        &_thread_id,
        process_connection,
        (void*) curfd
    );
}

int main() {
    int conn = socket(AF_INET, SOCK_STREAM, 0);
    if (conn == -1) handle_error("Failed to create a socket");

    struct sockaddr_in local_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr = INADDR_ANY,
    };

    if ( bind(
                conn,
                (struct sockaddr *) &local_addr,
                sizeof(local_addr)
            ) == -1) handle_error("Failed to bind to localhost");

    if ( listen(
                conn,
                PENINDG_CONNECTIONS_MAX
        ) == -1) handle_error("Failed to begin listening");

    while (1) try_handle_connection(conn);
}
