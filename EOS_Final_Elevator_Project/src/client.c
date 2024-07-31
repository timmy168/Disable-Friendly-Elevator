#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sockop.h"
#include "ascii.h"

#define SERVICE "tcp"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [ip] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    char *server_port = argv[2];

    // Connect to the server
    int sockfd = connectsock(server_ip, server_port, SERVICE);
    if (sockfd < 0) {
        perror("connectsock");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    char name[10];
    int type, time, loc, des;
    char direc[4];

    // Get user input
    printf("Enter data in the format: name <name> type <type> time <time> loc <loc> des <des> direc <up/down>\n");
    scanf("name %s type %d loc %d des %d direc %s", name, &type, &loc, &des, direc);

    // Construct the message to send
    snprintf(buffer, sizeof(buffer), "name %s type %d loc %d des %d direc %s", name, type, loc, des, direc);

    // Send the message to the server
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("write");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Data sent to server: %s\n", buffer);
    memset(buffer, 0, sizeof(buffer));

    // Receive and display response from the server continuously
    while (1) 
    {
        int bytes_read = read(sockfd, buffer, BUFFER_SIZE);
        if (bytes_read < 0) {
            perror("read");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            printf("Server closed the connection.\n");
            break;
        } else {
            // buffer[bytes_read] = '\0'; // Null-terminate the string
            printf("Response from server: %s\n", buffer);
            if((strcmp(buffer, "leave") == 0))
            {
                if(type==0) door();
                else leave_d();
            }
            else if((strcmp(buffer, "enter") == 0))
            {
                if(type==0) door();
                else enter_d();
            }
        }
    }

    // Close the socket
    close(sockfd);

    return 0;
}