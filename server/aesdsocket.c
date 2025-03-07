#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#include "aesdsocket.h"
#include "singly_linked_list.h"

#define ARGS "d"

int file_fd = 0;
int sock_fd = 0;
int client_fd = 0;

singly_linked_list_t* list = NULL;
pthread_mutex_t file_mutex;

void signal_handler(int s)
{
    if (s == SIGINT || s == SIGTERM)
    {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        close(client_fd);
        close(sock_fd);
        close(file_fd);
#ifndef USE_AESD_CHAR_DEVICE
        remove(FILE);
#endif
        node_t *ptr = (node_t *)sll_front(list);
        while (ptr != NULL)
        {
            thread_data_t *data = (thread_data_t *)(ptr->value);
            if (data)
            {
                pthread_join(data->tid, NULL); // Wait for the thread to finish
                if (data->client_fd > 0) close(data->client_fd); // Now safe to close
                if (data->file_fd > 0) close(data->file_fd);
                free(data);
            }
            ptr = ptr->next;
        }

        sll_destroy_list(list); // Free the linked list itself
        pthread_mutex_destroy(&file_mutex); // Clean up the mutex
        exit(0);
    }
}

#ifndef USE_AESD_CHAR_DEVICE
void timer_handler(union sigval sv) {
    (void)sv;  // Unused parameter

    time_t now;
    struct tm *tm_info;
    char timestamp[100];

    time(&now);
    tm_info = localtime(&now);
    int size = strftime(timestamp, sizeof(timestamp),
                        "timestamp:%a, %d %b %Y %H:%M:%S %z\n", tm_info);

    pthread_mutex_lock(&file_mutex);
    write(file_fd, timestamp, size);
    pthread_mutex_unlock(&file_mutex);
}

int init_timer(int firstRun, int interval) {
    struct sigevent sev;
    timer_t timerId;
    struct itimerspec ts;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_handler;
    sev.sigev_value.sival_ptr = NULL;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    sev.sigev_notify_attributes = &attr;

    if (timer_create(CLOCK_REALTIME, &sev, &timerId) != 0) {
        syslog(LOG_ERR, "Failed to create timer");
        return -1;
    }

    pthread_attr_destroy(&attr);

    ts.it_value.tv_sec = firstRun;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = interval;
    ts.it_interval.tv_nsec = 0;

    if (timer_settime(timerId, 0, &ts, NULL) != 0) {
        syslog(LOG_ERR, "Failed to start the timer");
        return -1;
    }

    return 0;
}
#endif

/*
* This function will handle the communication between the client. It will:
* 1) Receive the packets from the client, and write them to the file
* 2) Take all packets written to the file, and send them back to the client
*/
void handle_client(thread_data_t *d)
{
    char buf[BUFFER_SIZE];
    int bytes_read = 0;
    int bytes_written = 0;

    while(true)
    {
        pthread_mutex_lock(d->file_mutex);
        bytes_read = recv(d->client_fd, buf, BUFFER_SIZE - 1, 0);
        pthread_mutex_unlock(d->file_mutex);
        if (bytes_read < 0)
        {
            syslog(LOG_ERR, "[handle_client] recv error - %s", strerror(errno));
            return;
        }
        else if (bytes_read == 0)
        {
            // Terminated session
            break;
        }

        buf[bytes_read] = '\0';

        pthread_mutex_lock(d->file_mutex);
        bytes_written = write(d->file_fd, buf, bytes_read);
        pthread_mutex_unlock(d->file_mutex);
        if (bytes_written < 0)
        {
            syslog(LOG_ERR, "[handle_client] write error - %s", strerror(errno));
            return;
        }
        else if (bytes_written != bytes_read)
        {
            syslog(LOG_ERR, "[handle_client] write error - bytes mismatch. Read %d bytes, but wrote %d bytes", bytes_read, bytes_written);
            return;
        }

        if (strchr(buf, '\n') == NULL)
        {
            continue;
        }

#ifndef USE_AESD_CHAR_DEVICE
        // Packet received - sending it back!
        if (lseek(d->file_fd, 0, SEEK_SET) == -1)
        {
            syslog(LOG_ERR, "[handle_client] lseek error - %s", strerror(errno));
            return;
        }
#endif

        while((bytes_read = read(d->file_fd, buf, BUFFER_SIZE)) != 0)
        {
            syslog(LOG_INFO, "Read from driver: %s", buf);
            if (bytes_read == -1)
            {
                syslog(LOG_ERR, "[handle client] read error - %s", strerror(errno));
                return;
            }
            
            bytes_written = send(d->client_fd, buf, bytes_read, 0);
            if (bytes_written == -1)
            {
                syslog(LOG_ERR, "[handle_client] send error - %s", strerror(errno));
                return;
            }
            else if (bytes_written != bytes_read)
            {
                syslog(LOG_ERR, "[handle_client] send error - bytes mismatch. Read %d bytes, but wrote %d bytes", bytes_read, bytes_written);
                return;
            }
        }
    }
}

void *client_thread(void *t)
{
    thread_data_t *data = (thread_data_t *)t;

    // Open file for storing packet data
    data->file_fd = open(FILE, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (data->file_fd < 0)
    {
        syslog(LOG_ERR, "Failed to open file %s - %s", FILE, strerror(errno));
        goto done;
    }

    syslog(LOG_INFO, "Accepted connection from %s", (data->client_ip != NULL) ? data->client_ip : "an unknown IP");

    handle_client(data);

    syslog(LOG_INFO, "Closed connection from %s", (data->client_ip != NULL) ? data->client_ip : "an unknown IP");

    close(data->file_fd);
done:
    close(data->client_fd);
    data->thread_complete_success = true;

    return t;
}

void join_completed_threads()
{
    node_t *ptr = (node_t *)sll_front(list);
    node_t *next = NULL;
    thread_data_t *data = NULL;

    while(ptr != NULL)
    {
        data = (thread_data_t *)(ptr->value);
        if (data->thread_complete_success)
        {
            pthread_join(data->tid, NULL);
            next = ptr->next;  // Save next node
            sll_remove_node(list, ptr->value);
            if (data->client_fd > 0) close(data->client_fd);
            if (data->file_fd > 0) close(data->file_fd);
            free(data);
            ptr = next;  // Move forward
        }
        ptr = ptr->next;
    }
}

int main(int argc, char **argv)
{
    int daemon_mode = 0;
    int o = 0;
    int rc = 0;
    int opt = 1;
    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    struct sockaddr_in client_addr = {0};
    socklen_t client_addr_len = sizeof(client_addr);
    struct sigaction a = {0};
    pid_t pid;
    thread_data_t *thread_data = NULL;

    openlog(NULL, 0, LOG_USER);

    while ((o = getopt(argc, argv, ARGS)) != -1)
    {
        switch (o)
        {
            case 'd':
                daemon_mode = 1;
                break;
            default:
                break;
        }
    }

    // Setup signal handler
    a.sa_handler = signal_handler;
    if (sigaction(SIGTERM, &a, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to register signal handler for SIGTERM - %s", strerror(errno));
        return -1;
    }

    if (sigaction(SIGINT, &a, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to register signal handler for SIGINT - %s", strerror(errno));
        return -1;
    }

    // Set up lists, mutex, etc.
    if ((rc = pthread_mutex_init(&file_mutex, NULL)) != 0)
    {
        syslog(LOG_ERR, "Failed to initialize file mutex");
        goto close_file;
    }

    if ((list = sll_init_list()) == NULL)
    {
        syslog(LOG_ERR, "Failed to create linked list");
        rc = -1;
        goto destroy_file_mutex;
    }

    // Getting address info
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(NULL, PORT, &hints, &res);
    if (rc != 0)
    {
        syslog(LOG_ERR, "getaddrinfo error - %s", gai_strerror(rc));
        goto free_list;
    }

    // Open socket
    sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        syslog(LOG_ERR, "socket error - %s", strerror(errno));
        rc = -1;
        goto free_addr_info;
    }

    // Allow address reuse
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (rc != 0)
    {
        syslog(LOG_ERR, "setsockopt error - %s", strerror(errno));
        goto free_addr_info;
    }

    // Bind socket to port
    rc = bind(sock_fd, res->ai_addr, res->ai_addrlen);
    if (rc != 0)
    {
        syslog(LOG_ERR, "bind error - %s", strerror(errno));
        goto close_sock;
    }

    if (daemon_mode)
    {
        pid = fork();
        if (pid < 0)
        {
            syslog(LOG_ERR, "fork failed - %s", strerror(errno));
            goto close_sock;
        }
        if (pid > 0)
        {
            // Parent process
            return 0;
        }
    }

#ifndef USE_AESD_CHAR_DEVICE
    // Set up timer
    if ((rc = init_timer(1, 10)) != 0)
    {
        goto close_sock;
    }
#else
    syslog(LOG_INFO, "USE_AESD_CHAR_DEVICE is set");
#endif

    // Listen for incoming connections
    rc = listen(sock_fd, BACKLOG);
    if (rc != 0)
    {
        syslog(LOG_ERR, "listen error - %s", strerror(errno));
        goto close_sock;
    }

    // We don't need res anymore
    freeaddrinfo(res);

    // Server loop - accept connection and spawn thread
    while (true)
    {
        client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            syslog(LOG_ERR, "accept error - %s", strerror(errno));
            continue;
        }

        thread_data = (thread_data_t *)malloc(sizeof(thread_data_t));
        if (!thread_data)
        {
            syslog(LOG_ERR, "Failed to malloc thread_data_t");
            close(client_fd);
            continue;
        }
        
        thread_data->client_ip = inet_ntoa(client_addr.sin_addr);
        thread_data->file_fd = 0;
        thread_data->client_fd = client_fd;
        thread_data->file_mutex = &file_mutex;
        thread_data->thread_complete_success = false;

        if ((rc = pthread_create(&thread_data->tid, NULL, client_thread, thread_data)) != 0)
        {
            syslog(LOG_ERR, "Failed to spawn thread for new connection %s", thread_data->client_ip);
            free(thread_data);
            close(client_fd);
            continue;
        }

        sll_insert_node(list, (void *)thread_data);
        join_completed_threads();
    }

close_sock:
    close(sock_fd);
free_addr_info:
    if (res) freeaddrinfo(res);
free_list:
    sll_destroy_list(list);
destroy_file_mutex:
    pthread_mutex_destroy(&file_mutex);
close_file:
    close(file_fd);
#ifndef USE_AESD_CHAR_DEVICE
    remove(FILE);
#endif
    return rc;
}
