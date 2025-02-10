#include <stdio.h>
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

#define PORT "9000"
#define BUFFER_SIZE 1024
#define BACKLOG 10
#define FILE "/var/tmp/aesdsocketdata"

/*
* This function will handle the communication between the client. It will:
* 1) Receive the packets from the client, and write them to the file
* 2) Take all packets written to the file, and send them back to the client
*/
void handle_client(int client_fd, int file_fd)
{
    char buf[BUFFER_SIZE];
    int bytes_read = 0;
    int bytes_written = 0;
    int rc = 0;

    while(true)
    {
        bytes_read = recv(client_fd, buf, BUFFER_SIZE - 1, 0);
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

        bytes_written = write(file_fd, buf, bytes_read);
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

        // Packet received - sending it back!
        rc = lseek(file_fd, 0, SEEK_SET);
        if (rc == -1)
        {
            syslog(LOG_ERR, "[handle_client] lseek error - %s", strerror(errno));
            return;
        }

        while((bytes_read = read(file_fd, buf, BUFFER_SIZE)) != 0)
        {
            if (bytes_read == -1)
            {
                syslog(LOG_ERR, "[handle client] read error - %s", strerror(errno));
                return;
            }
            
            bytes_written = send(client_fd, buf, bytes_read, 0);
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

int main()
{
    int rc = 0;
    int file_fd = 0;
    int sock_fd = 0;
    int client_fd = 0;
    int opt = 1;
    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    struct sockaddr_in client_addr = {0};
    socklen_t client_addr_len = sizeof(client_addr);
    char *client_ip = NULL;

    openlog(NULL, 0, LOG_USER);

    // Open file for storing packet data
    file_fd = open(FILE, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (file_fd < 0)
    {
        syslog(LOG_ERR, "Failed to open file %s - %s", FILE, strerror(errno));
        return -1;
    }

    // Getting address info
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(NULL, PORT, &hints, &res);
    if (rc != 0)
    {
        syslog(LOG_ERR, "getaddrinfo error - %s", gai_strerror(rc));
        goto close_file;
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

    // Listen for incoming connections
    rc = listen(sock_fd, BACKLOG);
    if (rc != 0)
    {
        syslog(LOG_ERR, "listen error - %s", strerror(errno));
        goto close_sock;
    }

    // Server loop
    while (true)
    {
        client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            syslog(LOG_ERR, "accept error - %s", strerror(errno));
            continue;
        }

        client_ip = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", (client_ip != NULL) ? client_ip : "an unknown IP");

        handle_client(client_fd, file_fd);
        
        syslog(LOG_INFO, "Closed connection from %s", (client_ip != NULL) ? client_ip : "an unknown IP");
        close(client_fd);
    }

close_sock:
    close(sock_fd);
free_addr_info:
    freeaddrinfo(res);
close_file:
    close(file_fd);
    remove(FILE);
    return rc;
}
