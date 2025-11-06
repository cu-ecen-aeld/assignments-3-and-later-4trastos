#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include "queue.h"

#define PORT "9000"
#define BUF_SIZE 1024
#define DATA_FILE_PATH "/var/tmp/aesdsocketdata"

// Global variables
volatile sig_atomic_t stop_requested = 0;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Linked list node structure for threads
typedef struct thread_slist_s thread_slist_t;
struct thread_slist_s
{
    pthread_t tid;
    atomic_int thread_done;
    SLIST_ENTRY(thread_slist_s) entries;
};

// Thread argument structure
typedef struct
{
    int client_fd;
    char ipstr[INET6_ADDRSTRLEN];
    atomic_int *p_thread_done;
} thread_args_t;

// Function prototypes
int get_client_ip(struct sockaddr_storage client_addr, char *ipstr, size_t ipstr_len);
void handle_signal(int signo);
void *thread_handle_client(void *arg);
void *timestamp_thread(void *arg);

int main(int argc, char *argv[])
{
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    int d_mode = 0;
    int opt;

    while ((opt = getopt(argc, argv, "d")) != -1)
    {
        switch (opt)
        {
        case 'd':
            d_mode = 1;
            break;
        default:
            printf("Usage: %s [-d]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    struct addrinfo hints, *p_res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, PORT, &hints, &p_res);
    if (status != 0)
    {
        syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sock_fd = socket(p_res->ai_family, p_res->ai_socktype, p_res->ai_protocol);
    if (sock_fd == -1)
    {
        syslog(LOG_ERR, "Failed to create socket");
        freeaddrinfo(p_res);
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        syslog(LOG_ERR, "setsockopt(SO_REUSEADDR) failed");
        freeaddrinfo(p_res);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    if (bind(sock_fd, p_res->ai_addr, p_res->ai_addrlen) == -1)
    {
        syslog(LOG_ERR, "Failed to bind socket");
        freeaddrinfo(p_res);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(p_res);
    syslog(LOG_INFO, "Socket bound to port %s", PORT);

    if (d_mode)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            syslog(LOG_ERR, "fork() failed");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
        if (pid > 0)
            exit(EXIT_SUCCESS);

        if (setsid() < 0)
        {
            syslog(LOG_ERR, "setsid() failed");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        chdir("/");
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
    }

    if (listen(sock_fd, 5) < 0)
    {
        syslog(LOG_ERR, "listen() failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Start timestamp thread
    pthread_t timestamp_tid;
    if (pthread_create(&timestamp_tid, NULL, timestamp_thread, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to create timestamp thread");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Linked list for threads
    SLIST_HEAD(thread_slist_head, thread_slist_s) thread_list;
    SLIST_INIT(&thread_list);

    while (!stop_requested)
    {
        struct sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            if (stop_requested)
                break;
            perror("accept failed");
            continue;
        }

        char ipstr[INET6_ADDRSTRLEN];
        if (get_client_ip(client_addr, ipstr, INET6_ADDRSTRLEN) != 0)
        {
            close(client_fd);
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", ipstr);

        thread_args_t *p_thread_args = malloc(sizeof(thread_args_t));
        thread_slist_t *p_new_node = malloc(sizeof(thread_slist_t));

        if (!p_thread_args || !p_new_node)
        {
            syslog(LOG_ERR, "Memory allocation failed");
            close(client_fd);
            free(p_thread_args);
            free(p_new_node);
            continue;
        }

        p_thread_args->client_fd = client_fd;
        memcpy(p_thread_args->ipstr, ipstr, INET6_ADDRSTRLEN);
        p_new_node->thread_done = 0;
        p_thread_args->p_thread_done = &(p_new_node->thread_done);

        if (pthread_create(&p_new_node->tid, NULL, thread_handle_client, p_thread_args) == 0)
        {
            SLIST_INSERT_HEAD(&thread_list, p_new_node, entries);
        }
        else
        {
            syslog(LOG_ERR, "pthread_create() failed for client thread");
            close(client_fd);
            free(p_thread_args);
            free(p_new_node);
        }

        // Clean finished threads
        thread_slist_t *p_node;
        thread_slist_t *p_tmp_node;
        SLIST_FOREACH_SAFE(p_node, &thread_list, entries, p_tmp_node)
        {
            if (p_node->thread_done == 1)
            {
                pthread_join(p_node->tid, NULL);
                SLIST_REMOVE(&thread_list, p_node, thread_slist_s, entries);
                free(p_node);
            }
        }
    }

    // Cleanup: join all threads
    thread_slist_t *p_node;
    thread_slist_t *p_tmp_node;
    SLIST_FOREACH_SAFE(p_node, &thread_list, entries, p_tmp_node)
    {
        pthread_join(p_node->tid, NULL);
        SLIST_REMOVE(&thread_list, p_node, thread_slist_s, entries);
        free(p_node);
    }

    pthread_join(timestamp_tid, NULL);
    close(sock_fd);
    remove(DATA_FILE_PATH);
    closelog();

    return 0;
}

/* ---------------------------
   Private function definitions
   --------------------------- */

int get_client_ip(struct sockaddr_storage client_addr, char *ipstr, size_t ipstr_len)
{
    void *addr;
    if (client_addr.ss_family == AF_INET)
    {
        struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
        addr = &(s->sin_addr);
    }
    else if (client_addr.ss_family == AF_INET6)
    {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
        addr = &(s->sin6_addr);
    }
    else
    {
        syslog(LOG_ERR, "Unknown client address family");
        return -1;
    }

    inet_ntop(client_addr.ss_family, addr, ipstr, ipstr_len);
    return 0;
}

void handle_signal(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
        stop_requested = 1;
    }
}

void *thread_handle_client(void *arg)
{
    thread_args_t *p_thread_args = (thread_args_t *)arg;
    char buffer[BUF_SIZE];

    int data_fd = open(DATA_FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (data_fd == -1)
    {
        syslog(LOG_ERR, "Failed to open data file");
        pthread_exit(NULL);
    }

    int bytes_read = 0;
    while ((bytes_read = recv(p_thread_args->client_fd, buffer, BUF_SIZE, 0)) > 0)
    {
        pthread_mutex_lock(&file_mutex);
        write(data_fd, buffer, bytes_read);
        pthread_mutex_unlock(&file_mutex);

        if (memchr(buffer, '\n', bytes_read))
            break;
    }

    close(data_fd);

    data_fd = open(DATA_FILE_PATH, O_RDONLY);
    while ((bytes_read = read(data_fd, buffer, BUF_SIZE)) > 0)
    {
        send(p_thread_args->client_fd, buffer, bytes_read, 0);
    }
    close(data_fd);

    close(p_thread_args->client_fd);
    syslog(LOG_INFO, "Closed connection from %s", p_thread_args->ipstr);

    *(p_thread_args->p_thread_done) = 1;
    free(p_thread_args);
    return NULL;
}

void *timestamp_thread(void *arg)
{
    (void)arg;
    while (!stop_requested)
    {
        sleep(10);
        if (stop_requested)
            break;

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[128];
        strftime(time_str, sizeof(time_str), "%a, %d %b %Y %T %z", tm_info);

        char line[160];
        snprintf(line, sizeof(line), "timestamp:%s\n", time_str);

        pthread_mutex_lock(&file_mutex);
        int fd = open(DATA_FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd >= 0)
        {
            write(fd, line, strlen(line));
            close(fd);
        }
        pthread_mutex_unlock(&file_mutex);
    }
    return NULL;
}
