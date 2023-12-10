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
 * You can use this file to write a test microTCP client.
 * This file is already inserted at the build system.
 */

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>  
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include "../lib/microtcp.h"

int main(int argc,char **argv){
    microtcp_sock_t clientSocket; /* Socket Decriptor for Client */
    struct sockaddr_in server_addr;
    struct hostent *ptrh;


    //Check if port number is given
    if(argv[1] == NULL){
        perror("No port added!\nExecute the command with \"test_microtcp_client [port_number]\" \n");
        exit( EXIT_FAILURE );
    }
    clientSocket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    if(clientSocket.sd == -1){
        perror ( " SOCKET COULD NOT BE OPENED " );
        exit ( EXIT_FAILURE );
    }
    
    memset((char*)&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));

    ptrh = gethostbyname("127.0.0.1");
    memcpy(&server_addr.sin_addr,ptrh->h_addr,ptrh->h_length);

    if(microtcp_connect(&clientSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        printf("\nServer Not Ready !!\n");
        exit(1);
    }

    char *buffer = malloc(100);
    fgets(buffer, 50, stdin);
    size_t length = strlen(buffer);

    microtcp_send(&clientSocket, buffer, length, 0);

    // microtcp_shutdown(&clientSocket,0);
    
    free(buffer);

    return 0;
}