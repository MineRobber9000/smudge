#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#include "player.h"

#define PORT "1337"
#define BACKLOG 10

// https://tools.ietf.org/html/rfc854  -- negotiations
// http://www.iana.org/assignments/telnet-options/telnet-options.xhtml
// https://tools.ietf.org/html/rfc857  -- echo
// https://tools.ietf.org/html/rfc1184 -- linemode
#define T_IAC  "\xff"//255
#define T_WILL "\xfb"//251
#define T_WONT "\xfc"//252
#define T_DO   "\xfd"//253
#define T_DONT "\xfe"//254
#define T_SE   "\xf0"//240
#define T_SB   "\xfa"//250

#define T_CRLF "\r\n"

#define T_ECHO "\x01"//  1
#define T_LINE "\x22"// 34
#define T_NAWS "\x1f"// 31

#define T_CSI  "\x1b["
#define T_CLR  "2J"
#define T_ORIG "1;1H"
#define T_HIDE "?25l" // tput civis [man terminfo]
#define T_SHOW "?25h" // tput cnorm [man terminfo]

world_t world;

void sendall(int sockfd, const void *buf, size_t len) {
    int i;
    int r;
    if (len == 0) {
        len = strlen(buf);
    }
    for (i=0; i<len;) {
        r = send(sockfd, buf + i, len - i, 0);
        if (r == -1) {
            perror("sendall");
            pthread_exit(NULL);
        }
        i += r;
    }
}

int can_recv(int sockfd) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    if (select(sockfd+1, &set, NULL, NULL, &timeout) == -1) {
        perror("select");
        pthread_exit(NULL);
    }
    return FD_ISSET(sockfd, &set);
}

char recvchr(int sockfd) {
    char c;
    int n = recv(sockfd, &c, 1, 0);
    if (n == -1) { // bad news
        perror("recv");
        pthread_exit(NULL);
    }
    if (n == 0) { // orderly shutdown
        pthread_exit(NULL);
    }
    return c;
}

void* server(void* sockfd_void) {
    int sockfd = (int) (intptr_t) sockfd_void;
    char c;
    int term_width = 0;
    int term_height = 0;

    bool echo_status = false;
    bool line_status = false;
    bool naws_status = false;

    sendall(sockfd, T_IAC T_WILL T_ECHO, 3);
    sendall(sockfd, T_IAC T_DO T_NAWS, 3);
    sendall(sockfd, T_IAC T_DO T_LINE, 3);
    sendall(sockfd, T_IAC T_SB T_LINE "\x01" "\x00" T_IAC T_SE, 7);
    sendall(sockfd, T_CSI T_CLR,  4);
    sendall(sockfd, T_CSI T_HIDE, 6);

    player_t* player = player_init(&world, pthread_self());
    char buf[20*80];

    while (1) {
        usleep(16000);
        sendall(sockfd, T_CSI T_ORIG, 6);
        player->ping = (long int) time(NULL);
        if (!echo_status || !line_status || !naws_status) {
            sendall(sockfd, "Negotiating terminal parameters...", 0);
            sendall(sockfd, T_CRLF, 2);
        } else if (term_width < 80 || term_height < 20) {
            sendall(sockfd, "Please resize your terminal window to 80x20.", 0);
            sendall(sockfd, T_CRLF, 2);
        } else {
            player_draw(player, (char*)buf, 80, 20);
            for (int i=0; i<20; i++) {
                sendall(sockfd, buf + (i*80), 80);
                sendall(sockfd, T_CRLF, 2);
            }
        }

        if (can_recv(sockfd)) {
            c = recvchr(sockfd);
            if (c == T_IAC[0]) {
                c = recvchr(sockfd);
                if (c == T_WILL[0]) {
                    c = recvchr(sockfd);
                    if (c == T_LINE[0]) {
                        line_status = true;
                    }
                    if (c == T_NAWS[0]) {
                        naws_status = true;
                    }
                }
                if (c == T_WONT[0]) {
                    c = recvchr(sockfd);
                    if (c == T_LINE[0]) {
                        sendall(sockfd, "Could not negotiate LINEMODE.", 0);
                        close(sockfd);
                        pthread_exit(NULL);
                    }
                    if (c == T_NAWS[0]) {
                        sendall(sockfd, "Could not negotiate NAWS.", 0);
                        close(sockfd);
                        pthread_exit(NULL);
                    }
                }
                if (c == T_DO[0]) {
                    c = recvchr(sockfd);
                    if (c == T_ECHO[0]) {
                        echo_status = true;
                    }
                }
                if (c == T_DONT[0]) {
                    c = recvchr(sockfd);
                    if (c == T_ECHO[0]) {
                        sendall(sockfd, "Could not negotiate ECHO.", 0);
                        close(sockfd);
                        pthread_exit(NULL);
                    }
                }
                if (c == T_SB[0]) {
                    c = recvchr(sockfd);
                    if (c == T_NAWS[0]) {
                        term_width  = (((int)(unsigned char)recvchr(sockfd))<<8) +
                                       ((int)(unsigned char)recvchr(sockfd));
                        term_height = (((int)(unsigned char)recvchr(sockfd))<<8) +
                                       ((int)(unsigned char)recvchr(sockfd));
                        if (recvchr(sockfd) != T_IAC[0]) {
                            sendall(sockfd, "Could not negotiate NAWS!", 0);
                            close(sockfd);
                            pthread_exit(NULL);
                        }
                        if (recvchr(sockfd) != T_SE[0]) {
                            sendall(sockfd, "Could not negotiate NAWS!", 0);
                            close(sockfd);
                            pthread_exit(NULL);
                        }
                        sendall(sockfd, T_CSI T_CLR, 4);
                    } else {
                        while (1) {
                            c = recvchr(sockfd);
                            if (c == T_IAC[0]) {
                                c = recvchr(sockfd);
                                if (c == T_SE[0]) {
                                    break;
                                }
                            }
                        }
                    }
                }
            } else if (c == '\x03') {
                sendall(sockfd, T_CSI T_CLR, 4);
                sendall(sockfd, T_CSI T_ORIG, 6);
                sendall(sockfd, "^C" T_CRLF, 4);
                sendall(sockfd, T_CSI T_SHOW, 6);
                close(sockfd);
                pthread_exit(NULL);
            } else {
                player_char(player, c);
            }
        }
    }
    return 0;
}

int spawn_server(int sockfd) {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attr, server, (void*)(intptr_t)sockfd) != 0) {
        return -1;
    }
    pthread_attr_destroy(&attr);
    return 0;
}

int spawn_world(world_t* world) {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attr, world_loop, (void*)world) != 0) {
        return -1;
    }
    pthread_attr_destroy(&attr);
    return 0;
}


void start_server() {
    signal(SIGPIPE, SIG_IGN);
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *addrinfo, *c_addrinfo;
    int sockfd, new_fd;
    int yes=1;
    char buf[512];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &addrinfo) != 0) {
        fprintf(stderr, "Error (getaddrinfo)");
        exit(1);
    }

    for (
        c_addrinfo = addrinfo;
        c_addrinfo != NULL;
        c_addrinfo = c_addrinfo->ai_next
    ) {
        sockfd = socket(
                c_addrinfo->ai_family,
                c_addrinfo->ai_socktype,
                c_addrinfo->ai_protocol
                );
        if (sockfd == -1) {
            perror("socket");
            continue;
        }
        if (setsockopt(
            sockfd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &yes,
            sizeof(int)
        ) == -1) {
            perror("setsockopt");
            continue;
        }
        if (bind(sockfd, c_addrinfo->ai_addr, c_addrinfo->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }
        break;
    }

    if (c_addrinfo == NULL) {
        fprintf(stderr, "Failed to bind socket.");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    freeaddrinfo(addrinfo);

    addr_size = sizeof their_addr;
    while (1) {
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        if (spawn_server(new_fd) != 0) {
            fprintf(stderr, "Error (pthread)");
        }
    }
}


int main(void) {
    if (spawn_world(&world) != 0) {
        fprintf(stderr, "Error (pthread)");
    }
    start_server();
    return 0;
}

