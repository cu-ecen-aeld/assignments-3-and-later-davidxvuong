#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <stdbool.h>
#include <pthread.h>

#define PORT "9000"
#define BUFFER_SIZE 1024
#define BACKLOG 10

#ifdef USE_AESD_CHAR_DEVICE
#define FILE "/dev/aesdchar"
#else
#define FILE "/var/tmp/aesdsocketdata"
#endif

typedef struct {
    pthread_mutex_t *file_mutex;
    int file_fd;
    int client_fd;
    char *client_ip;
    pthread_t tid;
    bool thread_complete_success;
} thread_data_t;

#endif