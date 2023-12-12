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

#include "microtcp.h"
#include "../utils/crc32.h"
#include <netinet/in.h>


void *memset(void *ptr, int x, size_t n);
void *memcpy(void *dest, const void * src, size_t n);
time_t time( time_t *second );


microtcp_sock_t microtcp_socket (int domain, int type, int protocol){
    microtcp_sock_t microtcp_sock;

    //Allow only UDP sockets.
    if(type != 2){
        perror(" SOCKET MUST BE A TYPE OF UDP ");
        exit( EXIT_FAILURE );
    }
    
    //Check for errors
    microtcp_sock.sd = socket( domain , type , protocol );

    //Initializing microtcp_struct fields.
    microtcp_sock.state = UNBDOUND;

    microtcp_sock.curr_win_size = 0;
    microtcp_sock.init_win_size = 0;
    microtcp_sock.recvbuf = NULL;
    microtcp_sock.sendbuf = NULL;
    microtcp_sock.buf_fill_level = 0;
    microtcp_sock.ssthresh = 0;
    microtcp_sock.seq_number = 0;
    microtcp_sock.ack_number = 0;
    microtcp_sock.packets_send = 0;
    microtcp_sock.packets_received = 0;
    microtcp_sock.packets_lost = 0;
    microtcp_sock.bytes_send = 0;
    microtcp_sock.bytes_received =0;
    microtcp_sock.bytes_lost = 0;

    return microtcp_sock;
}

int microtcp_bind (microtcp_sock_t *socket, const struct sockaddr *address, socklen_t address_len){
    int bind_val;
    bind_val = bind(socket->sd, address, address_len);
    if(bind_val != -1) socket->state = BINDED;
    else socket->state = INVALID;

    return bind_val;
}

int microtcp_connect (microtcp_sock_t *socket, const struct sockaddr *address, socklen_t address_len){
    size_t client_seq_num = 0;
    microtcp_header_t *header = malloc(sizeof(microtcp_header_t));
    uint32_t retrieved_checksum = 0, checksum_num = 0, server_seq_num = 0;
    struct sockaddr addr = *address;

    if(header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    socket->client_ip = NULL;
    socket->server_ip = malloc(sizeof(struct sockaddr));
    if(socket->server_ip == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memcpy(socket->server_ip, address, sizeof(struct sockaddr));
    
    socket->recvbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the recvbuffer and initialize
    if(socket->recvbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->recvbuf, 0, sizeof(microtcp_header_t));
    socket->sendbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the sendbuffer and initialize
    if(socket->sendbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->sendbuf, 0, sizeof(microtcp_header_t));

    srand((unsigned int)time(NULL));            //Get a random value for the clients_sequence number
    client_seq_num = (size_t)rand();
    socket->seq_number = client_seq_num;

    //Create header of the SYN packet
    header->data_len = 0;
    header->ack_number = 0;
    header->seq_number = client_seq_num;
    header->future_use0 = 0;
    header->future_use1 = 0;
    header->future_use2 = 0;
    header->window = socket->curr_win_size; //NOT SURE
    header->checksum = 0;
    header->control = 0b0000000000000010;   //SYN Package

  
    memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->sendbuf,sizeof(microtcp_header_t));
    header->checksum = checksum_num;

    memset(socket->sendbuf,0,sizeof(microtcp_header_t));
    memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
   
    if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, socket->server_ip, address_len) == -1){ //SYN
        perror("(!) COULD NOT SEND SYN PACKET!\n");
        exit(EXIT_FAILURE);
    }
    else{printf("SENT SYN PACKAGE YAY!\n\n");}



    //Receiving SYN_ACK
    if(recvfrom(socket->sd, socket->recvbuf, sizeof(microtcp_header_t),0, &addr, &address_len) == -1){ //SYN
        perror("(!) COULD NOT RECEIVE SYN_ACK PACKET!\n");
        exit(0);
    }
    else printf("RECEIVED SYN_ACK PACKAGE YAY!\n");
    
    memset(header,0,sizeof(microtcp_header_t));
    //Retrieve the data of the header of the received packet
    memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

    //Check if checksum is correct
    retrieved_checksum = header->checksum;
    header->checksum = 0;
    memset(socket->recvbuf,0,sizeof(microtcp_header_t));
    memcpy(socket->recvbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->recvbuf,sizeof(microtcp_header_t));
    if(retrieved_checksum != checksum_num){
        perror("(!) Package has not been received correctly!\n");
        exit(EXIT_FAILURE);
    }
    printf("Package received correctly\n");
    
    printf("SYN_ACK - checksum: %d\n",header->checksum);
    printf("SYN_ACK - future_use0: %d\n",header->future_use0);
    printf("SYN_ACK - future_use1: %d\n",header->future_use1);
    printf("SYN_ACK - future_use2: %d\n",header->future_use2);
    printf("SYN_ACK - ack_number: %d\n",header->ack_number);
    printf("SYN_ACK - seq_number: %d\n",header->seq_number);
    printf("SYN_ACK - control: %d\n",header->control);
    printf("SYN_ACK - data_len: %d\n",header->data_len);
    printf("SYN_ACK - window: %d\n",header->window);
    printf("\n\n");

    socket->seq_number = client_seq_num;


    //Save important data and reset header
    server_seq_num = header->seq_number;
    memset(header, 0, sizeof(microtcp_header_t));

    //Create header of the ACK packet
    header->data_len = 0;
    header->ack_number = server_seq_num + 1;
    header->seq_number = client_seq_num + 1;
    header->future_use0 = 0;
    header->future_use1 = 0;
    header->future_use2 = 0;
    header->window = socket->curr_win_size; //NOT SURE
    header->checksum = 0;
    header->control = 0b000000000001000;   //SYN Package
  
    memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->sendbuf,sizeof(microtcp_header_t));
    header->checksum = checksum_num;
    memset(socket->sendbuf,0,sizeof(microtcp_header_t));
    memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
   
    //Sending ACK packet
    if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, address, address_len) == -1){ //SYN
        perror("(!) COULD NOT SEND ACK PACKET!\n");
        exit(EXIT_FAILURE);
    }
    else printf("SENT ACK PACKAGE YAY!\n\n");
    socket->seq_number = header->seq_number;

    free(socket->recvbuf);
    free(header);
    free(socket->sendbuf);
    return 1;

    // recvfrom(); //SYN-ACK
    // sendto();   //ACK

    // if(){
    //     socket->state = INVALID;
    // }else{

    // }
    // socket->state = ESTABLISHED;
}

int microtcp_accept (microtcp_sock_t *socket, struct sockaddr *address,socklen_t address_len){
    size_t server_seq_num = 0;
    struct sockaddr_in *add_in;
    microtcp_header_t *header = malloc(sizeof(microtcp_header_t));
    microtcp_header_t data;
    uint32_t retrieved_checksum = 0,checksum_num = 0, clients_seq_num = 0;

    if(header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    socket->recvbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the recvbuffer and initialize
    if(socket->recvbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->recvbuf, 0, sizeof(microtcp_header_t));

    socket->sendbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the sendbuffer and initialize
    if(socket->sendbuf == NULL){
        printf("(!)Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
    //Find clients' address
    add_in = (struct sockaddr_in *)address;
    add_in->sin_family = AF_INET;
    add_in->sin_addr.s_addr = INADDR_ANY;   
    
    socket->server_ip = NULL;
    socket->client_ip = add_in;
    socklen_t addrlen = sizeof(*add_in);

    if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t), 0,(struct sockaddr *)socket->client_ip, &addrlen) == -1){ //SYN
        perror("(!) COULD NOT RECEIVE PACKET!\n");
        exit(EXIT_FAILURE);
    }
    else printf("RECEIVED SYN PACKAGE YAY!\n");

    //Retrieve the data of the header of the received packet
    memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

    //Check if checksum is correct
    retrieved_checksum = header->checksum;
    header->checksum = 0;
    memcpy (socket->recvbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
    if(retrieved_checksum != checksum_num){
        perror("(!) Package has not been received correctly!\n");
        exit(EXIT_FAILURE);
    }
    printf("Package received correctly\n");
    
    printf("SYN - checksum: %d\n",header->checksum);
    printf("SYN - future_use0: %d\n",header->future_use0);
    printf("SYN - future_use1: %d\n",header->future_use1);
    printf("SYN - future_use2: %d\n",header->future_use2);
    printf("SYN - ack_number: %d\n",header->ack_number);
    printf("SYN - seq_number: %d\n",header->seq_number);
    printf("SYN - control: %d\n",header->control);
    printf("SYN - data_len: %d\n",header->data_len);
    printf("SYN - window: %d\n",header->window);
    printf("\n\n");
    
    clients_seq_num = header->seq_number;

    //Create header of the ACK package
    memset(header,0,sizeof(microtcp_header_t));

    server_seq_num = (size_t)rand();
    socket->seq_number = server_seq_num;

    header->data_len = 0;
    header->ack_number = clients_seq_num + 1;
    header->seq_number = server_seq_num;
    header->future_use0 = 0;
    header->future_use1 = 0;
    header->future_use2 = 0;
    header->window = socket->curr_win_size; //NOT SURE
    header->checksum = 0;
    header->control = 0b0000000000001010; //Ack Syn

    
    memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->sendbuf, sizeof(microtcp_header_t));
    header->checksum = checksum_num;
    memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
    memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
 
    //Sending SYN_ACK 
    if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, (struct sockaddr *)add_in, addrlen) == -1){
        perror("(!) COULD NOT SENT SYN_ACK PACKET!\n");
        exit(EXIT_FAILURE);
    }
    else printf("SENT SYN_ACK PACKAGE YAY!\n\n");

    //Reciving ACK 
    memset(header, 0, sizeof(microtcp_header_t));
    if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t), 0,(struct sockaddr *)add_in, &addrlen) == -1){ //ACK
        perror("(!) COULD NOT RECEIVE ACK PACKET!\n");
        exit(EXIT_FAILURE);
    }else printf("RECEIVED ACK PACKAGE YAY!\n");

    //Retrieve the data of the header of the received packet
    memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

    //Check if checksum is correct
    retrieved_checksum = header->checksum;
    header->checksum = 0;
    memcpy (socket->recvbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
    if(retrieved_checksum != checksum_num){
        perror("(!) Package has not been received correctly!\n");
        exit(EXIT_FAILURE);
    }
    printf("Package received correctly\n");

    //Print to check
    printf("ACK - checksum: %d\n",header->checksum);
    printf("ACK - future_use0: %d\n",header->future_use0);
    printf("ACK - future_use1: %d\n",header->future_use1);
    printf("ACK - future_use2: %d\n",header->future_use2);
    printf("ACK - ack_number: %d\n",header->ack_number);
    printf("ACK - seq_number: %d\n",header->seq_number);
    printf("ACK - control: %d\n",header->control);
    printf("ACK - data_len: %d\n",header->data_len);
    printf("ACK - window: %d\n",header->window);
    printf("\n\n");

    free(socket->recvbuf);
    free(header);
    free(socket->sendbuf);
    
    return 1;
}

int microtcp_shutdown (microtcp_sock_t *socket, int how){
    size_t server_seq_num = 0, client_seq_num = 0 ;
    microtcp_header_t *header = malloc(sizeof(microtcp_header_t));
    microtcp_header_t data;
    uint32_t retrieved_checksum = 0,checksum_num = 0, clients_seq_num = 0;

    if(header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    socket->recvbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the recvbuffer and initialize
    if(socket->recvbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->recvbuf, 0, sizeof(microtcp_header_t));
    socket->sendbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the sendbuffer and initialize

    if(socket->sendbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->sendbuf, 0, sizeof(microtcp_header_t));

    //Server-side
    if(socket->state == CLOSING_BY_PEER){
        printf("SERVER SIDE!\n");
        //Find clients' address
        socklen_t addrlen = sizeof(*(socket->client_ip));

        if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t),0,(struct sockaddr *)socket->client_ip, &addrlen) == -1){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("RECEIVED FIN_ACK PACKAGE YAY!\n");

        //Retrieve the data of the header of the received packet
        memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

        //Check if checksum is correct
        retrieved_checksum = header->checksum;
        header->checksum = 0;
        memcpy(socket->recvbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
        if(retrieved_checksum != checksum_num){
            perror("(!) Package has not been received correctly!\n");
            exit(EXIT_FAILURE);
        }
        printf("Package received correctly\n");
        
        printf("SYN - checksum: %d\n",header->checksum);
        printf("SYN - future_use0: %d\n",header->future_use0);
        printf("SYN - future_use1: %d\n",header->future_use1);
        printf("SYN - future_use2: %d\n",header->future_use2);
        printf("SYN - ack_number: %d\n",header->ack_number);
        printf("SYN - seq_number: %d\n",header->seq_number);
        printf("SYN - control: %d\n",header->control);
        printf("SYN - data_len: %d\n",header->data_len);
        printf("SYN - window: %d\n",header->window);
        printf("\n\n");
        
        clients_seq_num = header->seq_number;


        //Sending ACK package
        //Create header of the ACK package
        socket->state = CLOSING_BY_PEER;

        memset(header,0,sizeof(microtcp_header_t));

        header->data_len = 0;
        header->ack_number = clients_seq_num + 1;
        header->seq_number = 0;
        header->future_use0 = 0;
        header->future_use1 = 0;
        header->future_use2 = 0;
        header->window = socket->curr_win_size; //NOT SURE
        header->checksum = 0;
        header->control = 0b0000000000001000; //ACK

        
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->sendbuf, sizeof(microtcp_header_t));
        header->checksum = checksum_num;
        memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
    
        //Sending SYN_ACK 
        if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, (struct sockaddr *)socket->client_ip, sizeof(*(socket->client_ip))) == -1){
            perror("(!) COULD NOT SEND ACK PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("SENT ACK PACKAGE YAY!\n\n");




        //Sending FIN_ACK package
        //Create header of the ACK package
        memset(header,0,sizeof(microtcp_header_t));
        server_seq_num = (size_t)rand();

        header->data_len = 0;
        header->ack_number = 0;
        header->seq_number = server_seq_num;
        header->future_use0 = 0;
        header->future_use1 = 0;
        header->future_use2 = 0;
        header->window = socket->curr_win_size; //NOT SURE
        header->checksum = 0;
        header->control = 0b0000000000001001; //ACK

        
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->sendbuf, sizeof(microtcp_header_t));
        header->checksum = checksum_num;
        memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
    
        //Sending SYN_ACK 
        if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, (struct sockaddr *)socket->client_ip, sizeof(*(socket->client_ip))) == -1){
            perror("(!) COULD NOT SEND ACK PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("SENT FIN_ACK PACKAGE YAY!\n\n");


        //Reveiving ACK
        if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t),0,(struct sockaddr *)socket->client_ip, &addrlen) == -1){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("RECEIVED ACK PACKAGE YAY!\n");

        //Retrieve the data of the header of the received packet
        memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

        //Check if checksum is correct
        retrieved_checksum = header->checksum;
        header->checksum = 0;
        memcpy(socket->recvbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
        if(retrieved_checksum != checksum_num){
            perror("(!) Package has not been received correctly!\n");
            exit(EXIT_FAILURE);
        }
        printf("Package received correctly\n");

        socket->state = CLOSED;
        
        printf("SYN - checksum: %d\n",header->checksum);
        printf("SYN - future_use0: %d\n",header->future_use0);
        printf("SYN - future_use1: %d\n",header->future_use1);
        printf("SYN - future_use2: %d\n",header->future_use2);
        printf("SYN - ack_number: %d\n",header->ack_number);
        printf("SYN - seq_number: %d\n",header->seq_number);
        printf("SYN - control: %d\n",header->control);
        printf("SYN - data_len: %d\n",header->data_len);
        printf("SYN - window: %d\n",header->window);
        printf("\n\n");
        
        clients_seq_num = header->seq_number;
        printf("(!) Connection closed by peer!\n");
    }
    //Client-side
    else{
        printf("CLIENT SIDE!\n");
        //Create header of the ACK package
        memset(header,0,sizeof(microtcp_header_t));

        header->data_len = 0;
        header->ack_number = 0;
        header->seq_number = socket->seq_number;
        header->future_use0 = 0;
        header->future_use1 = 0;
        header->future_use2 = 0;
        header->window = socket->curr_win_size; //NOT SURE
        header->checksum = 0;
        header->control = 0b0000000000001001; //Ack Fin

        
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->sendbuf, sizeof(microtcp_header_t));
        header->checksum = checksum_num;
        memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
    
        //Sending SYN_ACK 
        if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, (struct sockaddr *)socket->server_ip, sizeof(*(socket->server_ip))) == -1){
            perror("(!) COULD NOT SEND FIN_ACK PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("SENT FIN_ACK PACKAGE YAY!\n\n");




        //Receiving ACK 
        //Find server's address
        socklen_t addrlen = sizeof(*(socket->server_ip));

        if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t),0,(struct sockaddr *)socket->server_ip, &addrlen) == -1){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("RECEIVED ACK PACKAGE YAY!\n");

        socket->state = CLOSING_BY_HOST;

        //Retrieve the data of the header of the received packet
        memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

        //Check if checksum is correct
        retrieved_checksum = header->checksum;
        header->checksum = 0;
        memcpy(socket->recvbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
        if(retrieved_checksum != checksum_num){
            perror("(!) Package has not been received correctly!\n");
            exit(EXIT_FAILURE);
        }
        printf("Package received correctly\n");
        
        printf("SYN - checksum: %d\n",header->checksum);
        printf("SYN - future_use0: %d\n",header->future_use0);
        printf("SYN - future_use1: %d\n",header->future_use1);
        printf("SYN - future_use2: %d\n",header->future_use2);
        printf("SYN - ack_number: %d\n",header->ack_number);
        printf("SYN - seq_number: %d\n",header->seq_number);
        printf("SYN - control: %d\n",header->control);
        printf("SYN - data_len: %d\n",header->data_len);
        printf("SYN - window: %d\n",header->window);
        printf("\n\n");
        
        server_seq_num = header->seq_number;

        
        
        //Receiving FIN_ACK 
        if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t),0,(struct sockaddr *)socket->server_ip, &addrlen) == -1){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("RECEIVED FIN_ACK PACKAGE YAY!\n");

        //Retrieve the data of the header of the received packet
        memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

        //Check if checksum is correct
        retrieved_checksum = header->checksum;
        header->checksum = 0;
        memcpy(socket->recvbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
        if(retrieved_checksum != checksum_num){
            perror("(!) Package has not been received correctly!\n");
            exit(EXIT_FAILURE);
        }
        printf("Package received correctly\n");

        socket->state = CLOSED;
        
        printf("SYN - checksum: %d\n",header->checksum);
        printf("SYN - future_use0: %d\n",header->future_use0);
        printf("SYN - future_use1: %d\n",header->future_use1);
        printf("SYN - future_use2: %d\n",header->future_use2);
        printf("SYN - ack_number: %d\n",header->ack_number);
        printf("SYN - seq_number: %d\n",header->seq_number);
        printf("SYN - control: %d\n",header->control);
        printf("SYN - data_len: %d\n",header->data_len);
        printf("SYN - window: %d\n",header->window);
        printf("\n\n");
        
        server_seq_num = header->seq_number;



        //Sending ACK package
        //Create header of the ACK package
        memset(header,0,sizeof(microtcp_header_t));

        header->data_len = 0;
        header->ack_number = server_seq_num + 1;
        header->seq_number = socket->seq_number + 1;
        header->future_use0 = 0;
        header->future_use1 = 0;
        header->future_use2 = 0;
        header->window = socket->curr_win_size; //NOT SURE
        header->checksum = 0;
        header->control = 0b0000000000001000; //Ack Fin

        
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
        checksum_num = crc32(socket->sendbuf, sizeof(microtcp_header_t));
        header->checksum = checksum_num;
        memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
        memcpy(socket->sendbuf, header, sizeof(microtcp_header_t));
    
        //Sending SYN_ACK 
        if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, (struct sockaddr *)socket->server_ip, sizeof(*(socket->server_ip))) == -1){
            perror("(!) COULD NOT SEND ACK PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("(!) SENT ACK PACKAGE YAY!\n\n");
    }

    free(socket->recvbuf);
    free(socket->sendbuf);
    free(header);
}

ssize_t microtcp_send (microtcp_sock_t *socket, const void *buffer, size_t length,int flags){
    microtcp_header_t *send_header = malloc(sizeof(microtcp_header_t));
    size_t packet_size = sizeof(microtcp_header_t) + length;
    uint32_t checksum_num = 0;

    if(send_header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    send_header->data_len = length;
    send_header->ack_number = socket->ack_number;
    send_header->seq_number = socket->seq_number;
    send_header->future_use0 = 0;
    send_header->future_use1 = 0;
    send_header->future_use2 = 0;
    send_header->window = socket->curr_win_size; //NOT SURE
    send_header->checksum = 0;
    send_header->control = 0b0000000000001000;   //ACK

    socket->sendbuf = malloc(packet_size);
    if(socket->sendbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    /*Create checksum number and add to the packet*/
    memset(socket->sendbuf, 0, packet_size);
    memcpy(socket->sendbuf, send_header, sizeof(microtcp_header_t));    //Add header
    memcpy(socket->sendbuf + sizeof(microtcp_header_t), (char *)buffer, length);      //Add data
    checksum_num = crc32(socket->sendbuf, packet_size);
    send_header->checksum = checksum_num;
    memset(socket->sendbuf, 0, packet_size);
    memcpy(socket->sendbuf, send_header, sizeof(microtcp_header_t));    //Add header
    memcpy(socket->sendbuf + sizeof(microtcp_header_t), (char *)buffer, length);      //Add data
    
    /*Server sends a package!*/
    if(socket->server_ip == NULL){
        if(sendto(socket->sd, socket->sendbuf, packet_size, flags, (struct sockaddr *)socket->client_ip, sizeof(*(socket->client_ip))) == -1){
            perror("(!) COULD NOT SEND PACKET!\n");
            exit(EXIT_FAILURE);
        }
        
    }/*Client sends a package*/
    else{
        if(sendto(socket->sd, socket->sendbuf, packet_size, flags, socket->server_ip, sizeof(*(socket->server_ip))) == -1){
            perror("(!) COULD NOT SEND PACKET!\n");
            exit(EXIT_FAILURE);
        }
    }

    socket->seq_number = socket->seq_number + length + sizeof(microtcp_header_t);

    free(send_header);
    free(socket->sendbuf);
}

ssize_t microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags){
    microtcp_header_t *recv_header = malloc(sizeof(microtcp_header_t));
    size_t packet_size = sizeof(microtcp_header_t) + length;
    uint32_t retrieved_checksum = 0, checksum_num = 0;

    if(recv_header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    socket->recvbuf = malloc(packet_size);  //Allocate space for the recvbuffer and initialize
    if(socket->recvbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->recvbuf, 0, packet_size);

    socklen_t addrlen = sizeof(*(socket->client_ip));
    /*Server receives a package!*/
    if(socket->server_ip == NULL){
        if(recvfrom(socket->sd, socket->recvbuf, packet_size, 0, (struct sockaddr *)socket->client_ip, &addrlen) == -1){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("RECEIVED PACKAGE YAY!\n");
        
    }/*Client receive a package*/
    else{
        if(recvfrom(socket->sd, socket->recvbuf, packet_size, 0, socket->server_ip, &addrlen) == -1){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            exit(EXIT_FAILURE);
        }
        else printf("RECEIVED PACKAGE YAY!\n");
    }
    
    
    //Retrieve the data of the header of the received packet
    memcpy(recv_header, socket->recvbuf, sizeof(microtcp_header_t));
    char **buffer_msg;
    buffer_msg = malloc(recv_header->data_len);
    memcpy(buffer_msg, socket->recvbuf + sizeof(microtcp_header_t), (size_t)recv_header->data_len);

    //Check if checksum is correct
    retrieved_checksum = recv_header->checksum;
    recv_header->checksum = 0;
    packet_size = sizeof(microtcp_header_t) + recv_header->data_len;
    socket->recvbuf = realloc(socket->recvbuf, packet_size);
    memset(socket->recvbuf, 0, packet_size);
    memcpy(socket->recvbuf, recv_header, sizeof(microtcp_header_t));
    memcpy(socket->recvbuf + sizeof(microtcp_header_t), buffer_msg, recv_header->data_len);
    checksum_num = crc32(socket->recvbuf, packet_size);

    if(retrieved_checksum != checksum_num){
        perror("(!) Package has not been received correctly!\n");
        exit(EXIT_FAILURE);
    }
    printf("Package received correctly\n");
    
    printf("Package - checksum: %d\n",recv_header->checksum);
    printf("Package - future_use0: %d\n",recv_header->future_use0);
    printf("Package - future_use1: %d\n",recv_header->future_use1);
    printf("Package - future_use2: %d\n",recv_header->future_use2);
    printf("Package - ack_number: %d\n",recv_header->ack_number);
    printf("Package - seq_number: %d\n",recv_header->seq_number);
    printf("Package - control: %d\n",recv_header->control);
    printf("Package - data_len: %d\n",recv_header->data_len);
    printf("Package - window: %d\n",recv_header->window);
    printf("\n\n");

    memset(buffer, 0, length);
    memcpy(buffer, socket->recvbuf + sizeof(microtcp_header_t), recv_header->data_len);
    socket->ack_number = recv_header->seq_number + recv_header->data_len;
    
    free(recv_header);
    free(socket->recvbuf);
    free(buffer_msg);
}


//TODO: Fix sequence and ack numbers