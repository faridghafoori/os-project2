#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <sstream>
#include "constants.h"
#include "util.h"

using namespace std;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int get_total_fine(string car_id, int socket)
{
    int numbytes = 0;
    char buf[MAXBUF];

    string request = GETFINE + REQUESTDELIM + car_id;
    send(socket, request.c_str(), strlen(request.c_str()), 0);
    if ((numbytes = recv(socket, buf, MAXBUF - 1, 0)) == -1)
    {
        perror("recv");
        return 1;
    }
    buf[numbytes] = '\0';
    if (strstr(buf, "ERROR:") != NULL)
    {
        printf("%s\n", buf);
        return 1;
    }
    else
    {
        string response(buf);
        stringstream ss(response);
        int fine = -1;
        ss >> fine;
        return fine;
    }
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXBUF];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 3)
    {
        printf("usage: address port\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
    {
        printf("getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            printf("client: socket\n");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            printf("client: connect\n");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        printf("client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo);

    while (true)
    {
        printf("\nWelcome!\nPossible Commands:\n\t1- report <car_id>\n\t2- exit\n\n");

        string command;
        getline(cin, command);
        if (command == "exit")
        {
            break;
        }
        else
        {
            vector<string> cmd = split_string(command, ' ');
            if (cmd.size() == 2 && cmd[0] == "report")
            {
                printf("Fetching your car's report ...\n");
                printf("Car %s total fine is : $%d\n", cmd[1].c_str(), get_total_fine(cmd[1], sockfd));
            }
            else
            {
                printf("Invalid command!!\n");
                continue;
            }
        }
    }

    close(sockfd);

    return 0;
}