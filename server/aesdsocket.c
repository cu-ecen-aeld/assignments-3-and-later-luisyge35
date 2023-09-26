#include <arpa/inet.h>  // Para inet_ntop
#include <getopt.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include "sys/socket.h"
#include "sys/types.h"

// Variables
static struct addrinfo *serverInfo;
static struct addrinfo hints;
static int status;
static int mySocket;
static int connection;

char *readFile(const char *filename, size_t *outFileSize) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("fopen");
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *fileContent = (char *)malloc(fileSize + 1);
  if (fileContent == NULL) {
    perror("malloc");
    fclose(file);
    return NULL;
  }

  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';

  // printf("%s\n", fileContent);

  fclose(file);

  *outFileSize = fileSize;

  return fileContent;
}

void signalHandler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    // Clean up and exit the program
    syslog(LOG_INFO, "Caught signal, exiting\n");
    closelog();
    freeaddrinfo(serverInfo);
    close(mySocket);
    close(connection);
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  bool daemonMode = false;

  // Parse command-line options
  int input;
  while ((input = getopt(argc, argv, "d")) != -1) {
    switch (input) {
      case 'd':
        daemonMode = true;
        break;
      default:
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        exit(1);
    }
  }

  // Set up signal handlers for SIGINT and SIGTERM
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  openlog("aesdsocket", LOG_PID, LOG_USER);

  // Remove the log file at the beginning of the program
  if (remove("/var/tmp/aesdsocketdata") != 0) {
    perror("remove");
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;  // use my IP

  mySocket = socket(AF_INET, SOCK_STREAM, 0);  // Usamos AF_INET aquí

  if (mySocket == -1) {
    perror("socket");
    exit(-1);
  }

  if (getaddrinfo(NULL, "9000", &hints, &serverInfo) != 0) {
    printf("Error getting addr info\n");
    exit(-1);
  }

  int opt = 1;

  setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  connection = bind(mySocket, serverInfo->ai_addr, serverInfo->ai_addrlen);

  if (connection != 0) {
    perror("bind");
    exit(-1);
  }

  if (listen(mySocket, 1) != 0) {
    perror("listen");
    exit(-1);
  }

  // If running in daemon mode, fork and let the parent exit
  if (daemonMode) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(1);
    } else if (pid > 0) {
      // Parent process exits
      exit(0);
    }

    // Child process continues
    setsid();    // Create a new session
    umask(0);    // Set the file mode creation mask to 0
    chdir("/");  // Change working directory to root

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

  while (1) {
    // Aceptar una conexión entrante
    struct sockaddr_storage clientAddr;
    socklen_t addrSize = sizeof(clientAddr);
    int clientSocket = accept(mySocket, (struct sockaddr *)&clientAddr, &addrSize);

    if (clientSocket == -1) {
      perror("accept");
      exit(-1);
    }

    // Mostrar la dirección IP del cliente
    char clientIP[INET6_ADDRSTRLEN];
    if (clientAddr.ss_family == AF_INET) {
      struct sockaddr_in *s = (struct sockaddr_in *)&clientAddr;
      inet_ntop(AF_INET, &(s->sin_addr), clientIP, INET6_ADDRSTRLEN);
    } else {
      struct sockaddr_in6 *s = (struct sockaddr_in6 *)&clientAddr;
      inet_ntop(AF_INET6, &(s->sin6_addr), clientIP, INET6_ADDRSTRLEN);
    }

    syslog(LOG_INFO, "Accepted connection from %s", clientIP);

    // Open the log file in "append" mode to start appending new data
    FILE *logFile = fopen("/var/tmp/aesdsocketdata", "a");
    if (logFile == NULL) {
      perror("fopen");
      exit(1);
    }

    // Buffer to hold received data
    char buffer[1024];
    ssize_t bytesReceived;
    char *fileContent;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
      buffer[bytesReceived] = '\0';
      fwrite(buffer, 1, bytesReceived, logFile);
      fflush(logFile);
      size_t fileSize;
      char *fileContent = readFile("/var/tmp/aesdsocketdata", &fileSize);
      for (uint64_t i = 0; i < bytesReceived; i++) {
        if (buffer[i] == '\n') {
          if (fileContent != NULL) {
            send(clientSocket, fileContent, fileSize, 0);
          }
        }
      }
      free(fileContent);
    }

    if (bytesReceived == -1) {
      perror("recv");
    }

    fclose(logFile);
    close(clientSocket);
  }

  return 0;
}