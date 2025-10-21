#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define RECV_BUFFER_SIZE 1024

// Variables globales para el estado y el socket del servidor
static volatile sig_atomic_t stop_server = 0;
static int server_fd = -1;
static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Estructura para pasar datos al hilo
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_data_t;

/**
 * @brief Manejador de señales. Solo establece la bandera de parada.
 * Para asegurar que accept() se interrumpe y el servidor cierra, 
 * cerramos server_fd en main() después de que la señal llegue.
 * @param signum Número de señal capturada.
 */
void signal_handler(int signum) {
    (void)signum;
    syslog(LOG_INFO, "Caught signal %d, setting stop flag.", signum);
    stop_server = 1;
}

/**
 * @brief Configura los manejadores para SIGINT y SIGTERM.
 */
void setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/**
 * @brief Configura el socket del servidor (bind y listen). Incluye SO_REUSEADDR.
 * @return File descriptor del socket del servidor o -1 en caso de fallo.
 */
int setup_server_socket(void) {
    int fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return -1;
    }
    
    // **APLICACIÓN DE CONSEJO:** Permite reuso rápido del puerto (SO_REUSEADDR)
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "Failed to set socket options: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Failed to bind to port %d: %s", PORT, strerror(errno));
        close(fd);
        return -1;
    }
    
    if (listen(fd, 10) < 0) {
        syslog(LOG_ERR, "Failed to listen on socket: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    return fd;
}

/**
 * @brief Lee el archivo de datos completo y lo envía al cliente.
 * @param client_fd Descriptor de archivo del cliente.
 * @return 0 en éxito, -1 en fallo.
 */
int send_file_contents(int client_fd) {
    int data_fd = open(DATA_FILE, O_RDONLY);
    if (data_fd < 0) {
        syslog(LOG_ERR, "Failed to open file for reading: %s", strerror(errno));
        return -1;
    }
    
    char file_buffer[RECV_BUFFER_SIZE];
    ssize_t bytes_read;
    ssize_t total_sent = 0;
    
    syslog(LOG_INFO, "Sending file contents back to client");
    while ((bytes_read = read(data_fd, file_buffer, sizeof(file_buffer))) > 0) {
        ssize_t sent = send(client_fd, file_buffer, bytes_read, 0);
        if (sent <= 0) {
            syslog(LOG_ERR, "Failed to send data to client: %s", strerror(errno));
            close(data_fd);
            return -1;
        }
        total_sent += sent;
    }
    syslog(LOG_INFO, "Sent total %zd bytes to client", total_sent);
    
    close(data_fd); // **Cierre de FD**
    return 0;
}

/**
 * @brief Función ejecutada por cada hilo para manejar un cliente.
 * @param arg Puntero a client_data_t.
 * @return NULL.
 */
void* handle_client_thread(void* arg) {
    client_data_t* client_data = (client_data_t*)arg;
    int client_fd = client_data->client_fd;
    struct sockaddr_in client_addr = client_data->client_addr;
    
    char client_ip[INET_ADDRSTRLEN];
    char recv_buffer[RECV_BUFFER_SIZE];
    ssize_t bytes_received;
    
    // Búfer dinámico para el paquete completo (puede crecer)
    char* packet_buffer = NULL;
    size_t packet_len = 0;
    
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    
    // Recibir datos
    while (!stop_server && (bytes_received = recv(client_fd, recv_buffer, sizeof(recv_buffer), 0)) > 0) {
        
        char* current_ptr = recv_buffer;
        ssize_t remaining_in_buffer = bytes_received;

        // Bucle para procesar posibles múltiples paquetes en un solo recv()
        while (remaining_in_buffer > 0) {
            char* newline_ptr = (char*)memchr(current_ptr, '\n', remaining_in_buffer);
            
            if (newline_ptr) {
                // Nueva línea encontrada. Paquete completo.
                size_t data_to_add = newline_ptr - current_ptr + 1; // Incluye '\n'
                
                // 1. Añadir el paquete actual al buffer dinámico
                char* new_packet_buffer = realloc(packet_buffer, packet_len + data_to_add);
                if (!new_packet_buffer) {
                    syslog(LOG_ERR, "Realloc failed while receiving packet");
                    goto cleanup;
                }
                packet_buffer = new_packet_buffer;
                memcpy(packet_buffer + packet_len, current_ptr, data_to_add);
                packet_len += data_to_add;
                
                // 2. Procesar el paquete completo
                
                // Reemplazamos el '\n' con '\0' temporalmente para la escritura/syslog.
                packet_buffer[packet_len - 1] = '\0'; 
                syslog(LOG_INFO, "Complete packet received: '%.*s'", (int)(packet_len - 1), packet_buffer);
                
                pthread_mutex_lock(&file_mutex);
                
                // A) Escribir el paquete al archivo
                int data_fd = open(DATA_FILE, O_CREAT | O_WRONLY | O_APPEND, 0644);
                if (data_fd >= 0) {
                    write(data_fd, packet_buffer, packet_len - 1); // Escribir el cuerpo
                    write(data_fd, "\n", 1); // Escribir la nueva línea
                    close(data_fd); // **Cierre de FD**
                } else {
                    syslog(LOG_ERR, "Failed to open file for writing: %s", strerror(errno));
                }
                
                // B) Leer y enviar el archivo completo de vuelta
                send_file_contents(client_fd);
                
                pthread_mutex_unlock(&file_mutex);
                
                // 3. Resetear el buffer de paquete y procesar datos restantes
                free(packet_buffer);
                packet_buffer = NULL;
                packet_len = 0;
                
                // Mover el puntero para procesar el siguiente paquete
                ssize_t consumed = data_to_add;
                current_ptr += consumed;
                remaining_in_buffer -= consumed;
                
            } else {
                // Nueva línea NO encontrada. Paquete parcial.
                
                // Reasignar memoria para el paquete
                char* new_packet_buffer = realloc(packet_buffer, packet_len + remaining_in_buffer);
                if (!new_packet_buffer) {
                    syslog(LOG_ERR, "Realloc failed while appending data");
                    goto cleanup;
                }
                packet_buffer = new_packet_buffer;
                
                // Copiar los nuevos datos al final y terminar el bucle interno
                memcpy(packet_buffer + packet_len, current_ptr, remaining_in_buffer);
                packet_len += remaining_in_buffer;
                remaining_in_buffer = 0;
            }
        }
    }
    
    // Manejo de errores de recepción
    if (bytes_received == 0) {
        syslog(LOG_INFO, "Client %s disconnected cleanly", client_ip);
    } else if (bytes_received < 0) {
        if (errno != EINTR) { 
            syslog(LOG_ERR, "Receive error from %s: %s", client_ip, strerror(errno));
        }
    }
    
cleanup:
    // Limpieza de recursos del hilo
    if (packet_buffer) {
        free(packet_buffer); // **Liberación de memoria**
    }
    
    // Cerrar y liberar el FD del cliente
    close(client_fd); // **Cierre de FD**
    free(client_data); // **Liberación de memoria**
    syslog(LOG_INFO, "Closed connection from %s", client_ip);
    
    return NULL;
}

/**
 * @brief Implementa el proceso de demonización (fork, setsid, close FDs).
 */
void daemonize(void) {
    pid_t pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    
    // Cierra descriptores de archivo estándar
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

/**
 * @brief Función principal del servidor.
 */
int main(int argc, char* argv[]) {
    int daemon_mode = 0;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }
    
    openlog("aesdsocket", LOG_PID, LOG_USER);
    setup_signals();
    
    // Eliminar archivo de datos existente al inicio
    remove(DATA_FILE);
    
    server_fd = setup_server_socket();
    if (server_fd < 0) {
        closelog();
        return -1;
    }
    
    syslog(LOG_INFO, "Server started on port %d", PORT);
    
    if (daemon_mode) {
        daemonize();
    }
    
    // Bucle principal para aceptar conexiones
    while (!stop_server) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            // Si accept() fue interrumpido por una señal
            if (errno == EINTR) {
                continue; 
            }
            
            if (!stop_server) {
                syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
            }
            continue;
        }
        
        // Asignación de datos del cliente (se liberará en el hilo)
        client_data_t* client_data = malloc(sizeof(client_data_t));
        if (!client_data) {
            syslog(LOG_ERR, "Failed to allocate client data");
            close(client_fd); // **Cierre de FD**
            continue;
        }
        client_data->client_fd = client_fd;
        client_data->client_addr = client_addr;
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_thread, client_data) != 0) {
            syslog(LOG_ERR, "Failed to create client thread: %s", strerror(errno));
            free(client_data); // **Liberación de memoria**
            close(client_fd);  // **Cierre de FD**
        } else {
            // Separar el hilo para liberar recursos al terminar.
            pthread_detach(thread_id);
        }
    }
    
    // --- Limpieza Final ---
    if (server_fd != -1) {
        // **APLICACIÓN DE CONSEJO:** shutdown() para terminar limpiamente las conexiones
        syslog(LOG_INFO, "Shutting down server socket for all further send and receives.");
        shutdown(server_fd, SHUT_RDWR); 
        close(server_fd); // **Cierre de FD final**
    }
    
    remove(DATA_FILE);
    pthread_mutex_destroy(&file_mutex);
    
    syslog(LOG_INFO, "Server shutdown complete");
    closelog();
    
    return 0;
}
