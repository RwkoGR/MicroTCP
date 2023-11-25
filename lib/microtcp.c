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
    uint32_t checksum_num = 0;

    socket->recvbuf = malloc(sizeof(uint8_t));  //Allocate space for the recvbuffer and initialize
    memset(socket->recvbuf, 0, sizeof(uint8_t));
    socket->sendbuf = malloc(sizeof(uint8_t));  //Allocate space for the sendbuffer and initialize
    memset(socket->sendbuf, 0, sizeof(uint8_t));

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
    // header->checksum = 0;
    header->control = 0b0000000000000010;

  
    memcpy(socket->sendbuf, header, sizeof(header));
    checksum_num = crc32(socket->sendbuf,sizeof(socket->sendbuf));
    header->checksum = checksum_num;
    printf("(!)Checksum = %d\n",header->checksum);
    printf("(!)ack client= %d\n",header->seq_number);

    memset(socket->sendbuf,0,sizeof(header));

    memcpy(socket->sendbuf, header, sizeof(header));
    printf("(!)Checksum = %d\n",header->checksum);
    if(sendto(socket->sd,socket->sendbuf,sizeof(socket->sendbuf),0,address,address_len) == -1){ //SYN
        perror(" COULD NOT SEND SYN PACKET! ");
        exit(0);
    }
    else{printf("SENT SYN PACKAGE YAY!\n");}
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
    uint32_t checksum_num = 0;

    socket->recvbuf = malloc(sizeof(uint8_t));  //Allocate space for the recvbuffer and initialize
    memset(socket->recvbuf, 0, sizeof(uint8_t));
    socket->sendbuf = malloc(sizeof(uint8_t));  //Allocate space for the sendbuffer and initialize
    memset(socket->sendbuf, 0, sizeof(uint8_t));

    // srand((unsigned int)time(NULL));            //Get a random value for the clients_sequence number
    // server_seq_num = (size_t)rand();
    // socket->seq_number = server_seq_num;

    // //Create header of the SYN packet
    // header->data_len = 0;
    // header->ack_number = 0;
    // header->seq_number = server_seq_num;
    // header->future_use0 = 0;
    // header->future_use1 = 0;
    // header->future_use2 = 0;
    // header->window = socket->curr_win_size; //NOT SURE
    // header->checksum = 0;
    // header->control = 0b0000000000000010;


    //FInd clients' address
    add_in = (struct sockaddr_in *)address;
    add_in->sin_family = AF_INET;
    add_in->sin_addr.s_addr = INADDR_ANY;   



    if(recvfrom(socket->sd,socket->recvbuf,sizeof(socket->recvbuf),0,add_in->sin_addr.s_addr,sizeof(add_in->sin_addr.s_addr)) == -1){ //SYN
        perror(" COULD NOT RECIEVE SYN PACKET! ");
        exit(0);
    }
    else{printf("RECIEVED SYN PACKAGE YAY!\n");}
    memcpy(header, socket->recvbuf, sizeof(microtcp_header_t));
    printf("Checksum recieved: %d\n",header->checksum);
    printf("client recieved: %d\n",header->seq_number);
    
    address = ((struct sockaddr *)add_in);
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
