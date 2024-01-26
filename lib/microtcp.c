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
    microtcp_sock.cwnd = MICROTCP_INIT_CWND;
    microtcp_sock.ssthresh = MICROTCP_INIT_SSTHRESH;
    microtcp_sock.seq_number = 0;
    microtcp_sock.ack_number = 0;
    microtcp_sock.last_ack_number = 0;
    microtcp_sock.duplicate_ack_count = 0;
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
    socket->relative_seq_number = client_seq_num;
    socket->init_win_size = MICROTCP_WIN_SIZE - 32;
    socket->curr_win_size = MICROTCP_WIN_SIZE - 32;

    //Create header of the SYN packet
    header->data_len = 0;
    header->ack_number = 0;
    header->seq_number = client_seq_num;
    header->future_use0 = 0;
    header->future_use1 = 0;
    header->future_use2 = 0;
    header->window = socket->init_win_size; //NOT SURE
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
    header->window = socket->init_win_size; //NOT SURE
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
    socket->ack_number = header->ack_number;


    free(socket->recvbuf);
    socket->recvbuf = malloc(MICROTCP_RECVBUF_LEN);  //Allocate space for the recvbuffer and initialize
    if(socket->recvbuf == NULL){                      //with win_size
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->recvbuf, 0, sizeof(socket->recvbuf));
    free(header);
    free(socket->sendbuf);
    socket->state = ESTABLISHED;


    return 1;

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
        return -1;
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
        return -1;
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
    //(size_t)rand();
    socket->seq_number = server_seq_num;
    socket->relative_seq_number = server_seq_num;
    socket->ack_number = clients_seq_num + 1;
    socket->init_win_size = MICROTCP_WIN_SIZE - 32;
    socket->curr_win_size = MICROTCP_WIN_SIZE - 32;
    
    header->data_len = 0;
    header->ack_number = socket->ack_number;
    header->seq_number = socket->seq_number;
    header->future_use0 = 0;
    header->future_use1 = 0;
    header->future_use2 = 0;
    header->window = socket->init_win_size;
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
        return -1;
    }
    else printf("SENT SYN_ACK PACKAGE YAY!\n\n");


    //Reciving ACK 
    memset(header, 0, sizeof(microtcp_header_t));
    if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t), 0,(struct sockaddr *)add_in, &addrlen) == -1){ //ACK
        perror("(!) COULD NOT RECEIVE ACK PACKET!\n");
        return -1;
    }
    else printf("RECEIVED ACK PACKAGE YAY!\n");

    //Retrieve the data of the header of the received packet
    memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

    //Check if checksum is correct
    retrieved_checksum = header->checksum;
    header->checksum = 0;
    memcpy (socket->recvbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
    if(retrieved_checksum != checksum_num){
        perror("(!) Package has not been received correctly!\n");
        return -1;
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


    socket->state = ESTABLISHED;
    socket->seq_number += 1;

    free(socket->recvbuf);
    socket->recvbuf = malloc(MICROTCP_RECVBUF_LEN);  //Allocate space for the recvbuffer and initialize
    if(socket->recvbuf == NULL){                      //with init_win_size
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->recvbuf, 0, sizeof(microtcp_header_t));
    free(header);
    free(socket->sendbuf);
    
    return 0;
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

    socket->sendbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the sendbuffer and initialize

    if(socket->sendbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    memset(socket->sendbuf, 0, sizeof(microtcp_header_t));

    //Server-side
    if(socket->state == CLOSING_BY_PEER){
        printf("\nSERVER SIDE!\n");
        //Find clients' address
        socklen_t addrlen = sizeof(*(socket->client_ip));


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
        header->window = socket->init_win_size; //NOT SURE
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
        server_seq_num = 0;
        //(size_t)rand();

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
        printf("\nCLIENT SIDE!\n");
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
        printf("header_checksum: %d\n",checksum_num);
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

ssize_t microtcp_send (microtcp_sock_t *socket, const void *buffer, size_t length, int flags){
    size_t remaining = 0, bytes_to_send = 0, data_sent = 0, buffer_offset = 0, retransmit_length;
    int chunks = 0, result = 0, check_split = 0, retransmit_package = 0;
    int *array_of_seq = NULL;

    printf("1length: %d\n",length);
    remaining = length;
    while(data_sent < length){
        printf("in while\n");
        printf("2length: %d\n",length);
        printf("socket->curr_win_size: %d\n",socket->curr_win_size );
        printf("socket->cwnd: %d\n",socket->cwnd );
        printf("remaining: %d\n",remaining);
        bytes_to_send = min_for3(socket->curr_win_size , socket->cwnd ,remaining);
        chunks = bytes_to_send / MICROTCP_MSS;

        if(bytes_to_send % MICROTCP_MSS){
            array_of_seq = malloc((chunks + 1) * sizeof(int));
        }else{
            array_of_seq = malloc(chunks * sizeof(int));
        }

        printf("chunks before sent: %d\n", chunks);
        for(int i = 0; i < chunks; i++){
            printf("in for\n");
            array_of_seq[i] = socket->seq_number - socket->relative_seq_number;
            result = our_send(socket, buffer + buffer_offset, MICROTCP_MSS, flags);
            buffer_offset += MICROTCP_MSS;
            // if(result == -1) return -1;         //check later to retrasmit the pakage
        }

        /* Check if there is a semi - filled chunk*/
        if(bytes_to_send % MICROTCP_MSS){
            array_of_seq[chunks] = socket->seq_number - socket->relative_seq_number;
            result = our_send(socket, buffer + buffer_offset, bytes_to_send % MICROTCP_MSS, flags);
            buffer_offset += (bytes_to_send % MICROTCP_MSS);
            // if(result == -1) return -1;         //check later to retrasmit the pakage
            chunks++;
            check_split = 1;
        }

        /* Get the ACKs */
        for(int i = 0; i < chunks; i++){ 
            printf("\n\nseq num:%d\n\n",array_of_seq[i]);
            printf("in for receive\n");
            result = our_receive(socket, flags);

            //Package received correctly
            if(result == 0){
                socket->packets_send++;
                socket->cwnd += MICROTCP_MSS;
            }
            //If we got 1 dup ack save the package number
            else if(result == 1){
                buffer_offset = array_of_seq[i-1]; 
                retransmit_length = array_of_seq[i] - array_of_seq[i-1];
            }
            //If we got 3 dup acks save the package number
            else if(result == 3){
                socket->ssthresh = socket->cwnd / 2;
                socket->cwnd = socket->ssthresh + 1;
                retransmit_package = 1; //true

                // buffer_offset = array_of_seq[i-3]; //change buffer_offset to the position of the package we have to retransmit
                // retransmit_length = array_of_seq[i-2] - array_of_seq[i-3];

            }//if timeout occured
            else if(result == -2){
                socket->ssthresh = socket->cwnd / 2;
                socket->cwnd = min(MICROTCP_MSS , socket->ssthresh);
                retransmit_package = 1; //true
            }
            // if(result == -1) return -1;
        }

        if(bytes_to_send == 0){
            // srand(time(NULL));   // Initialization, should only be called once.
            // int random = rand() % MICROTCP_ACK_TIMEOUT_US+1;  
            // wait(&random);
            result = our_send(socket, NULL, 0, flags);
            if(result == -1){
                printf("(!) Error sending empty packet!\n");
                exit(EXIT_FAILURE);
            }   
            result = our_receive(socket, flags);
            if(result == -1){
                printf("(!) Error receiving empty packet!\n");
                exit(EXIT_FAILURE);
            }
        }
        
        /* Retransmissions */
        if(retransmit_package == 1){
            printf("We have to retransmit!\n\n");
            
            //send the package with size of the package we have to retrasmit
            our_send(socket, buffer + buffer_offset, retransmit_length, flags);
            retransmit_package = 0;
            retransmit_length = 0;
        }

        /* Update congestion control */
        free(array_of_seq);
        remaining -= bytes_to_send;
        data_sent += bytes_to_send;
        check_split = 0;
    }

    // if(length > MICROTCP_MSS) length = length - (length - MICROTCP_MSS);
    // if((length + sizeof(microtcp_header_t)) > socket->curr_win_size) length = socket->curr_win_size - sizeof(microtcp_header_t);
    // if(length <= 0) while(wait_until_space_available(length) != 0);

    return length;
}

ssize_t microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags){
    microtcp_header_t *recv_header = malloc(sizeof(microtcp_header_t));
    microtcp_header_t *ack_header = malloc(sizeof(microtcp_header_t));
    size_t packet_size = sizeof(microtcp_header_t) + length, data_received = 0;
    uint32_t retrieved_checksum = 0, checksum_num = 0;

    socket->sendbuf = malloc(sizeof(microtcp_header_t));
    memset(buffer, 0, length);

    if(socket->sendbuf == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    if(recv_header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    if(ack_header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    socklen_t addrlen = sizeof(*(socket->client_ip));

    //initalize so it goes in while
    recv_header->data_len = MICROTCP_MSS;
    while(1){
        /*Server receives a package!*/
        if(socket->server_ip == NULL){
            if(recvfrom(socket->sd, socket->recvbuf, packet_size, 0, (struct sockaddr *)socket->client_ip, &addrlen) == -1){ //SYN
                perror("(!) COULD NOT RECEIVE PACKET!\n");
                return -1;
            }
            else printf("RECEIVED PACKAGE YAY!\n");
            
        }/*Client receive a package*/
        else{
            if(recvfrom(socket->sd, socket->recvbuf, packet_size, 0, socket->server_ip, &addrlen) == -1){ //SYN
                perror("(!) COULD NOT RECEIVE PACKET!\n");
                return -1;
            }
            else printf("RECEIVED PACKAGE YAY!\n");
        }

        //Retrieve the data of the header of the received packet
        memcpy(recv_header, socket->recvbuf, sizeof(microtcp_header_t));


        //!TEST TO SEE IF DUP_ACKS WORK!
        // if(recv_header->seq_number == 5497){
        //     checksum_num = 0;
        //     retrieved_checksum = 0;
        //     memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
        //     continue;
        // }
        
        socket->buf_fill_level += (size_t)recv_header->data_len; //Data added to recv_buff

        //Copy data to our temp buffer

        char **buffer_msg = malloc(recv_header->data_len);
        memset(buffer_msg, 0, recv_header->data_len);
        if(buffer_msg == NULL){
            printf("(!) Memory allocation failed!\n");
            exit(EXIT_FAILURE);
        }
        if(recv_header->data_len != 0){
            memcpy(buffer_msg, socket->recvbuf + sizeof(microtcp_header_t), recv_header->data_len);
        }

        //Check if checksum is correct
        retrieved_checksum = recv_header->checksum;
        recv_header->checksum = 0;
        packet_size = sizeof(microtcp_header_t) + recv_header->data_len;
        memset(socket->recvbuf, 0, packet_size);
        memcpy(socket->recvbuf, recv_header, sizeof(microtcp_header_t));
        if(recv_header->data_len != 0)
            memcpy(socket->recvbuf + sizeof(microtcp_header_t), buffer_msg, recv_header->data_len);
        checksum_num = crc32(socket->recvbuf, packet_size);


        //CHECK ACK_NUMBERS && SEQUENCE_NUMBERS
        if(retrieved_checksum != checksum_num){
            perror("(!) Package has not been received correctly!\n");
            continue;
        }

        //CHECK IF THAT WORKS
        if(recv_header->seq_number != socket->ack_number){
            if(our_send(socket, NULL, 0, flags) == -1){
                printf("(!) Error sending ACK packet!\n");
            }
            checksum_num = 0;
            retrieved_checksum = 0;
            memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
            free(buffer_msg);
            perror("(!) Package has not been received correctly!\n");
            continue;
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

        //If message is FIN_ACK
        if(recv_header->control == 0b0000000000001001){ //FIN_ACK
            printf("(!) Connection closed by peer!\n");
            socket->state = CLOSING_BY_PEER;
            return -1;
        }

        data_received += recv_header->data_len;

        socket->curr_win_size = recv_header->window;

        socket->ack_number = recv_header->seq_number + recv_header->data_len;

        //Send acknowledgement
            memset(ack_header,0,sizeof(microtcp_header_t));

            // socket->seq_number = recv_header->seq_number + recv_header->data_len;

            ack_header->data_len = 0;
            ack_header->ack_number = socket->ack_number;
            ack_header->seq_number = socket->seq_number;
            ack_header->future_use0 = 0;
            ack_header->future_use1 = 0;
            ack_header->future_use2 = 0;
            ack_header->window = socket->init_win_size - socket->buf_fill_level;
            ack_header->checksum = 0;
            ack_header->control = 0b0000000000001000; //Ack 


            
            
            memcpy(socket->sendbuf, ack_header, sizeof(microtcp_header_t));
            checksum_num = crc32(socket->sendbuf, sizeof(microtcp_header_t));
            ack_header->checksum = checksum_num;
            memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
            memcpy(socket->sendbuf, ack_header, sizeof(microtcp_header_t));
            //Sending ACK 
            if(socket->client_ip == NULL){
                
                if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, socket->server_ip, sizeof(*(socket->server_ip))) == -1){
                    perror("(!) COULD NOT SENT ACK PACKET!\n");
                    return -1;
                }
                else printf("SENT ACK PACKAGE!\n\n");
            }else{
                if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, (struct sockaddr *)socket->client_ip,sizeof(*(socket->client_ip))) == -1){
                    perror("(!) COULD NOT SENT ACK PACKET!\n");
                    return -1;
                }
                else printf("SENT ACK PACKAGE!\n\n");
            }

        if(recv_header->data_len == 0){
            memset(socket->recvbuf, 0, MICROTCP_RECVBUF_LEN);
            socket->buf_fill_level = 0;
            checksum_num = 0;
            retrieved_checksum = 0;
            memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
            memset(recv_header, 0, sizeof(microtcp_header_t));
            packet_size = sizeof(microtcp_header_t) + length - data_received;
            free(buffer_msg);
            continue;
        }
        // When our rwnd is "0" (leave 32 bytes for the incoming empty probes)
        // if(socket->init_win_size - socket->buf_fill_level == 32){
        //     //copy data to users_buffer
        //     if(length >= data_received){
        
                // length -= MICROTCP_RECVBUF_LEN - 32;
        //     }
        //     else{
        //         printf("(!)Not enough space in user's buffer to accomodate data.\n");
        //         return -1;
        //     }
        //     socket->buf_fill_level = socket->buf_fill_level - sizeof(microtcp_header_t) - (size_t)recv_header->data_len;

        // }

        
        
        //Return data to user and release from recv_buff


        checksum_num = 0;
        retrieved_checksum = 0;
        memset(socket->sendbuf, 0, sizeof(microtcp_header_t));
        free(buffer_msg);
        break;
    }
    
    memcpy(buffer, socket->recvbuf + sizeof(microtcp_header_t), data_received);
    // memset(socket->recvbuf, 0, socket->buf_fill_level);
    // socket->buf_fill_level = 0;
    free(recv_header);
    free(ack_header);
    free(socket->sendbuf);

    
    return data_received;
}

ssize_t min_for3(size_t a, size_t b, size_t c){
	return min(a,min(b,c));
}

ssize_t our_send(microtcp_sock_t *socket, const void *buffer, size_t length, int flags){
    size_t packet_size = sizeof(microtcp_header_t) + length;
    uint32_t checksum_num = 0, retrieved_checksum;
    microtcp_header_t *send_header = malloc(sizeof(microtcp_header_t));

    printf("in our send\n");
    
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
    send_header->window = socket->init_win_size - socket->buf_fill_level;
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
    if(buffer != NULL && length != 0){
        memcpy(socket->sendbuf + sizeof(microtcp_header_t), (char *)buffer, length);      //Add data
    }
    checksum_num = crc32(socket->sendbuf, packet_size);
    send_header->checksum = checksum_num;
    memset(socket->sendbuf, 0, packet_size);
    memcpy(socket->sendbuf, send_header, sizeof(microtcp_header_t));    //Add header
    if(buffer != NULL && length != 0){
        memcpy(socket->sendbuf + sizeof(microtcp_header_t), (char *)buffer, length);      //Add data
    }
    /*Server sends a package!*/
    if(socket->server_ip == NULL){
        if(sendto(socket->sd, socket->sendbuf, packet_size, flags, (struct sockaddr *)socket->client_ip, sizeof(*(socket->client_ip))) == -1){
            perror("(!) COULD NOT SEND PACKET!\n");
            return -1;
        }
        
    }/*Client sends a package*/
    else{
        if(sendto(socket->sd, socket->sendbuf, packet_size, flags, socket->server_ip, sizeof(*(socket->server_ip))) == -1){
            perror("(!) COULD NOT SEND PACKET!\n");
            return -1;
        }
    }
    printf("SENT PACKAGE\n");
    socket->seq_number += send_header->data_len;

    free(send_header);
    free(socket->sendbuf);

    return length;
}


ssize_t our_receive(microtcp_sock_t* socket, int flags){
    microtcp_header_t *recv_ack_header = malloc(sizeof(microtcp_header_t));
    size_t packet_size = sizeof(microtcp_header_t);
    int result = 0;
    uint32_t checksum_num = 0, retrieved_checksum = 0;

     printf("in our receive\n");

    if(recv_ack_header == NULL){
        printf("(!) Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    socklen_t addrlen = sizeof(*(socket->client_ip));

    struct timeval timeout;
    timeout. tv_sec = 0;
    timeout. tv_usec = MICROTCP_ACK_TIMEOUT_US;
    if (setsockopt( socket->sd , SOL_SOCKET, SO_RCVTIMEO , & timeout , sizeof( struct timeval)) < 0) {
        perror(" setsockopt");
        return -2;
    }
    /*Server receives a package!*/
    if(socket->server_ip == NULL){
        if((result = recvfrom(socket->sd, socket->recvbuf, packet_size, 0, (struct sockaddr *)socket->client_ip, &addrlen)) < 0){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            return result;
        }
        else printf("RECEIVED PACKAGE YAY!\n");
        
        
    }/*Client receive a package*/
    else{
        if((result = recvfrom(socket->sd, socket->recvbuf, packet_size, 0, socket->server_ip, &addrlen) < 0)){ //SYN
            perror("(!) COULD NOT RECEIVE PACKET!\n");
            return result;
        }
        else printf("RECEIVED PACKAGE YAY!\n");
    }

    //Retrieve the data of the header of the received packet
    memcpy(recv_ack_header, socket->recvbuf, sizeof(microtcp_header_t));

    //Check if checksum is correct
    retrieved_checksum = recv_ack_header->checksum;
    recv_ack_header->checksum = 0;
    memset(socket->recvbuf, 0, sizeof(microtcp_header_t));
    memcpy(socket->recvbuf, recv_ack_header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));

    //CHECK ACK_NUMBERS && SEQUENCE_NUMBERS
    if(retrieved_checksum != checksum_num){
        perror("(!) Package has not been received correctly!\n");
        return -1;
    }
    printf("Package received ACK correctly\n");
    printf("Package - checksum: %d\n",recv_ack_header->checksum);
    printf("Package - future_use0: %d\n",recv_ack_header->future_use0);
    printf("Package - future_use1: %d\n",recv_ack_header->future_use1);
    printf("Package - future_use2: %d\n",recv_ack_header->future_use2);
    printf("Package - ack_number: %d\n",recv_ack_header->ack_number);
    printf("Package - seq_number: %d\n",recv_ack_header->seq_number);
    printf("Package - control: %d\n",recv_ack_header->control);
    printf("Package - data_len: %d\n",recv_ack_header->data_len);
    printf("Package - window: %d\n",recv_ack_header->window);
    printf("\n\n");

    if(socket->last_ack_number == recv_ack_header->ack_number && recv_ack_header->window != 0){
        socket->duplicate_ack_count++;
        if(socket->duplicate_ack_count == 3){
            socket->duplicate_ack_count = 0;
            return 3;   //3 duplicate acks
        }
        else if(socket->duplicate_ack_count == 1){
            return 1;   //duplicate ack
        }
    }else{
        socket->last_ack_number = recv_ack_header->ack_number;
    }
    socket->curr_win_size = recv_ack_header->window;


    return 0;
}


//TODO: fixare to problhma me to infinite loop kai ta 32 bytes poy xanontai sthn send kai receive



//CWND : Einai gia to congestion control. LastByteSent-LastByteAcked <= cwnd.
//Ta bytes poy stelneis ana pasa stigmh, dhladh ayta poy hdh einai ka8odon kai 
//ayta poy stelneis twra, prepei na einai mikrotera se mege8os apo to cwnd alliws
//ousiastika stelneis perissotera apo oti mporei na dex8ei o allos.


//SLOW START: 3ekinaei me cwnd = 1MSS,se ka8e RTT(ack) to cwnd diplasiazetai.
//Otan prokypsei congestion to ssthresh pairnei thn timh ssthresh = 1/2 *cwnd
//Thn epomenh fora poy 8a 3ekhnsei pali to cwnd na au3anetai, molis ftasei/perasei
//to ssthresh tote stamataei na diplasiazetai kai au3anetai grammika kata 1.


//1st step: pare arketo xwro gia ta arxika mnmt sto three way hanshake                      +

//2nd step: sto telos ths connect kai ths accept, malloc to init_window_size gia ton recv   +
//kai free sto telos ths shutdown.

//3rd step: bgale ola ta malloc poy ginontai ston socket->recv_buff                         +

//4th step: Sthn receive, Bale sto diko soy socket->curr_win = header->window.              +
//Bale socket->buff_fill_level += received_data. Steile ACK ston sender me 
//header->window = socket->init_window - buff_fill_Level (DIKO SOU CURR_WINDOW_SIZE).

//5th step: Sthn send,bale ta acknowledgments. Bale to header->window = socket->init_window - buff_fill_Level (DIKO SOU CURR_WINDOW_SIZE).
//Bale sthn send periorismo: X <= MICROTCP_MSS && X <= socket->curr_window_size(Tou alounou)

//6th step: Sto telos ths receive, socket->buff_fill_level -= received_data                 +

//7th step: Sthn send, an to socket->curr_window_size == 0, prin kan ftia3eis ton header. Stelne header
//xwris data mexri na pareis ACK me header->window > 0 ana random diasthmata meta3y 0...MICROTCP_ACK_TIMEOUT_US.Meta
//ananewse to socket->curr_win_size = header->window kai steile to paketo soy opou SIZE < socket->window.