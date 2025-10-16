#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>

#define MAXUSERS 100
#define MAXLEN 1000

char userids[MAXUSERS][MAXLEN];
int clientfds[MAXUSERS];
int numUsers = 1;
int signalPipe[2];

void pexit(char *errmsg) {
    fprintf(stderr, "%s\n", errmsg);
    exit(1);
}

void write_signal(int sig) {
    write(signalPipe[1], "x", 1);
}

void* dedicatedServer(void* arg) {
    int index = *((int*)arg);
    free(arg);

    char buffer[MAXLEN];
    int n;
    while ((n = read(clientfds[index], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "list") == 0) {
            for (int i = 0; i < numUsers; i++) {
                char response[MAXLEN];
                sprintf(response, "%s\n", userids[i]);
                write(clientfds[index], response, strlen(response));
            }
        }
        else if (strncmp(buffer, "send ", 5) == 0) {
            char* target = strtok(buffer + 5, " ");
            char* msg = strtok(NULL, "");

            if (!target || !msg) continue;

            int found = -1;
            for (int i = 0; i < numUsers; i++) {
                if (strcmp(userids[i], target) == 0) {
                    found = i;
                    break;
                }
            }

            if (found != -1) {
                char response[MAXLEN];
                sprintf(response, "%s says %s\n", userids[index], msg);
                write(clientfds[found], response, strlen(response));

                sprintf(response, "sent to %s\n", target);
                write(clientfds[index], response, strlen(response));
            }
            else {
                char response[MAXLEN];
                sprintf(response, "Sorry, %s has not joined yet.\n", target);
                write(clientfds[index], response, strlen(response));
            }
        }
                else if (strncmp(buffer, "broadcast ", 10) == 0) {
            char* msg = buffer + 10;
            char response[MAXLEN];
            sprintf(response, "BROADCAST from %s: %s\n", userids[index], msg);

            for (int i = 0; i < numUsers; i++) {
                if (clientfds[i] > 0) {
                    write(clientfds[i], response, strlen(response));
                }
            }

            sprintf(response, "Broadcast sent to all users\n");
            write(clientfds[index], response, strlen(response));
        }
        else if (strncmp(buffer, "random ", 7) == 0) {
            if (numUsers <= 1) {
                char response[MAXLEN];
                sprintf(response, "No other users available to send random message\n");
                write(clientfds[index], response, strlen(response));
                continue;
            }

            char* msg = buffer + 7;
            int randomIndex;
            do {
                randomIndex = rand() % numUsers;
            } while (randomIndex == index);

            char response[MAXLEN];
            sprintf(response, "RANDOM MESSAGE from %s: %s\n", userids[index], msg);
            write(clientfds[randomIndex], response, strlen(response));

            sprintf(response, "Random message sent to %s\n", userids[randomIndex]);
            write(clientfds[index], response, strlen(response));
        }
        else if (strcmp(buffer, "close") == 0) {
            char response[MAXLEN];
            sprintf(response, "Closing your connection. Goodbye!\n");
            write(clientfds[index], response, strlen(response));
            break;
        }
    }
        char message[MAXLEN];
    sprintf(message, "%s has left.\n", userids[index]);
        printf("%s", message);
         for (int i = 0; i < numUsers; i++) {
        if (i != index && clientfds[i] > 0) {
            write(clientfds[i], message, strlen(message));
        }
    }

    close(clientfds[index]);
        for (int i = index; i < numUsers - 1; i++) {
        strcpy(userids[i], userids[i+1]);
        clientfds[i] = clientfds[i+1];
    }
    numUsers--;

    return NULL;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, write_signal);
    signal(SIGTERM, write_signal);
    pipe(signalPipe);

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char buffer[1025];
    time_t ticks;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        pexit("socket() error.");

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(buffer, '0', sizeof(buffer));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int port = 4999;
    do {
        port++;
        serv_addr.sin_port = htons(port);
    } while (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0);
    printf("bind() succeeds for port #%d\n", port);

    if (listen(listenfd, 10) < 0)
        pexit("listen() error.");

    while(1) {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        printf("connected to client %d.\n", numUsers);
                char *prompt = "Please enter your username: ";
                write(connfd, prompt, strlen(prompt));
                int n = read(connfd, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            close(connfd);
            continue;
        }
        buffer[n] = '\0';
        char *cptr = strchr(buffer, '\n');
        if (cptr) *cptr = '\0';

        if (numUsers >= MAXUSERS) {
            close(connfd);
            continue;
        }

        strcpy(userids[numUsers], buffer);
        clientfds[numUsers] = connfd;
                sprintf(buffer, "%s joined.\n", userids[numUsers]);
        for(int i = 0; i < numUsers; i++) {
            write(clientfds[i], buffer, strlen(buffer));
        }
                int *index = malloc(sizeof(int));
        *index = numUsers;
        pthread_t t;
        pthread_create(&t, NULL, dedicatedServer, index);
        pthread_detach(t);

        numUsers++;
    }

    close(listenfd);
    return 0;
}
