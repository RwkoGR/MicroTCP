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

    socket->recvbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the recvbuffer and initialize
    memset(socket->recvbuf, 0, sizeof(microtcp_header_t));
    socket->sendbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the sendbuffer and initialize
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
   
    if(sendto(socket->sd, socket->sendbuf, sizeof(microtcp_header_t), 0, address, address_len) == -1){ //SYN
        perror(" COULD NOT SEND SYN PACKET! ");
        exit(EXIT_FAILURE);
    }
    else{printf("(!) SENT SYN PACKAGE YAY!\n\n");}



    //Receiving SYN_ACK
    if(recvfrom(socket->sd, socket->recvbuf, sizeof(microtcp_header_t),0, &addr, &address_len) == -1){ //SYN
        perror(" COULD NOT RECEIVE SYN_ACK PACKET! ");
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
        perror(" (!) Package has not been received correctly! ");
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
        perror(" COULD NOT SEND ACK PACKET! ");
        exit(EXIT_FAILURE);
    }
    else printf("(!) SENT ACK PACKAGE YAY!\n\n");


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

    socket->recvbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the recvbuffer and initialize
    memset(socket->recvbuf, 0, sizeof(microtcp_header_t));
    socket->sendbuf = malloc(sizeof(microtcp_header_t));  //Allocate space for the sendbuffer and initialize
    memset(socket->sendbuf, 0, sizeof(microtcp_header_t));

    //Find clients' address
    add_in = (struct sockaddr_in *)address;
    add_in->sin_family = AF_INET;
    add_in->sin_addr.s_addr = INADDR_ANY;   
    socklen_t addrlen = sizeof(*add_in);

    if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t),0,(struct sockaddr *)add_in, &addrlen) == -1){ //SYN
        perror(" COULD NOT RECIEVE PACKET! ");
        exit(EXIT_FAILURE);
    }
    else printf("RECIEVED SYN PACKAGE YAY!\n");

    //Retrieve the data of the header of the received packet
    memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

    //Check if checksum is correct
    retrieved_checksum = header->checksum;
    header->checksum = 0;
    memcpy (socket->recvbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
    if(retrieved_checksum != checksum_num){
        perror(" (!) Package has not been received correctly! ");
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
        perror(" COULD NOT SENT SYN_ACK PACKET! ");
        exit(EXIT_FAILURE);
    }
    else printf("(!) SENT SYN_ACK PACKAGE YAY!\n\n");

    //Reciving ACK 
    memset(header, 0, sizeof(microtcp_header_t));
    if(recvfrom(socket->sd,socket->recvbuf,sizeof(microtcp_header_t),0,(struct sockaddr *)add_in, &addrlen) == -1){ //SYN
        perror(" COULD NOT RECIEVE ACK PACKET! ");
        exit(EXIT_FAILURE);
    }else printf("RECIEVED ACK PACKAGE YAY!\n");

    //Retrieve the data of the header of the received packet
    memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));

    //Check if checksum is correct
    retrieved_checksum = header->checksum;
    header->checksum = 0;
    memcpy (socket->recvbuf, header, sizeof(microtcp_header_t));
    checksum_num = crc32(socket->recvbuf, sizeof(microtcp_header_t));
    if(retrieved_checksum != checksum_num){
        perror(" (!) Package has not been received correctly! ");
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
  /* Your code here */
}

ssize_t microtcp_send (microtcp_sock_t *socket, const void *buffer, size_t length,int flags){
  /* Your code here */
}

ssize_t microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags){
  /* Your code here */
}
