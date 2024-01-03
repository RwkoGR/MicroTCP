/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * You can use this file to write a test microTCP server.
 * This file is already inserted at the build system.
 */

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<netdb.h>
#include "../lib/microtcp.h"

int main(int argc,char **argv) {
    microtcp_sock_t serverSocket;
    int client_connected;
    struct sockaddr_in client_addr,server_addr;
    struct hostent *ptrh;


    //Check if port number is given
    if(argv[1] == NULL){
        perror("No port added!\nExecute the command with \"test_microtcp_server [port_number]\"");
        exit( EXIT_FAILURE );
    }

    serverSocket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    if(serverSocket.sd == -1){
        perror ( " (!) SOCKET COULD NOT BE OPENED " );
        exit ( EXIT_FAILURE );
    }

    memset((char*)&server_addr,0,sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    

    if(microtcp_bind(&serverSocket,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        printf("(!) Bind Failure!\n");
    }
    else{
        printf("Bind Success!\n");
    }

   
    client_connected = microtcp_accept(&serverSocket,(struct sockaddr*)&client_addr,sizeof(client_addr));

    if (client_connected != -1){
        printf("Connection accepted!\n");
    }
    else{
        printf("(!) Connection not accepted!\n");
    }


    char buffer[100000];
    while(1){
        microtcp_recv(&serverSocket, &buffer, sizeof(buffer), 0);

        if(!strcmp(buffer, "shutdown") || !strcmp(buffer, "SHUTDOWN")){
            serverSocket.state = CLOSING_BY_PEER;
            printf("(!) Initiating shutdown process!\n");
            microtcp_shutdown(&serverSocket,0);
        }

        printf("Client: %s\n",buffer);
        memset(buffer, 0, sizeof(buffer));
        
        printf("Server: ");
        if(fgets(buffer, sizeof(buffer), stdin) != NULL){
            // Remove the newline character, if present
            size_t length = strlen(buffer);
            if (length > 0 && buffer[length - 1] == '\n') {
                buffer[length - 1] = '\0';
            }
        }
        else{
            perror("fgets failed");
        }
        size_t length = strlen(buffer);
        microtcp_send(&serverSocket, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
    
    }
    free(buffer);

    return 0;
}

//TODO: fix acknowledgments for sent messages.