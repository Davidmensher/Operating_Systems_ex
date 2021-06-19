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
#include <assert.h>
#include <signal.h>

int pcc_total[95];
int pcc_total_demo[95];
int SIGUSR1_cont = 1;
int processing_client = 0;

//prints the statistics
void print_stats(){
    int i;
    for(i = 0; i < 95; i++){
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    }
}

//update pcc_total if no error occurred in client
void update_pcc_total(){
    int i;
    for(i = 0; i < 95; i++){
        pcc_total[i] = pcc_total_demo[i];
    }
}

//sigusr1 handler
void my_signal_handler( int        signum,
                        siginfo_t* info,
                        void*      ptr)
{
    if(processing_client){
        SIGUSR1_cont = 0;
    }
    else{
        print_stats();
        exit(0);
    }
}

//verify if byte is printable
int is_printable_verifier(char b){
    if(b >= 32 && b <= 126){
        return 1;
    }
    return 0;
}

//counts printable bytes and update pcc_total_demo
int count_printable(char* data, int size){
    int i;
    char b;
    int counter = 0;
    int bool_flag;

    for(i = 0; i < size; i++){
        b = data[i];
        bool_flag = is_printable_verifier(b);
        if(bool_flag == 1){
            pcc_total_demo[b-32]++;
            counter++;
        }
    }

    return counter;
}



int main(int argc, char *argv[])
{
    int listenfd  = -1;
    int connfd    = -1;
    int total_bytes = 0;

    char* first_buffer;
    int  bytes_read =  0;
    int message_size;
    int real_message_size;
    char* message_buffer;
    char* data_buff;
    int nsent = 0;
    int totalsent;
    int set_ret;
    short port;
    int collapsed = 0;

    struct sockaddr_in serv_addr;
//    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

//    int data_buff;
    if(argc != 2){
        perror("\n Error : Number of arguments differ than 1 \n");
        exit(1);
    }

    port = atoi(argv[1]);

    // Structure to pass to the registration syscall
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    // Assign pointer to our handler function
    new_action.sa_sigaction = my_signal_handler;
    // Setup the flags
    new_action.sa_flags = SA_SIGINFO;
    // Register the handler
    if( 0 != sigaction(SIGUSR1, &new_action, NULL) )
    {
        perror("Signal handle registration " "failed. %s\n");
        exit(1);
    }

    listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    if(listenfd < 0){
        perror("\n Error : socket Failed. %s \n");
        exit(1);
    }

    set_ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if(set_ret < 0){
        perror("\n Error : setsockopt Failed. %s \n");
        exit(1);
    }

    memset( &serv_addr, 0, addrsize );

    serv_addr.sin_family = AF_INET;
    // INADDR_ANY = any local machine address
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if( 0 != bind( listenfd,
                   (struct sockaddr*) &serv_addr,
                   addrsize ) )
    {
        perror("\n Error : Bind Failed. %s \n");
        exit(1);
    }

    if( 0 != listen( listenfd, 10 ) )
    {
        perror("\n Error : Listen Failed. %s \n");
        exit(1);
    }

    while(SIGUSR1_cont){
        collapsed = 0;
        connfd = accept( listenfd,
                         (struct sockaddr*) &peer_addr,
                         &addrsize);

        processing_client = 1; //will indicate we currently processing client

        if( connfd < 0 )
        {
            perror("\n Error : Accept Failed. %s \n");
            exit(1);
        }

        ///start accepting the size of the message
        first_buffer = (char*)&message_size;
        total_bytes = 0;

        while (1){
            bytes_read = read(connfd, first_buffer+total_bytes, 4-total_bytes);
            if( bytes_read < 0 ){
                if(errno==ETIMEDOUT || errno==ECONNRESET || errno==EPIPE){
                    perror("TCP Error in read from client\n");
                    collapsed = 1;
                }
                else{
                    perror("Error in read from client\n");
                    exit(1);
                }
            }

            total_bytes += bytes_read;
            if(bytes_read<=0){
                break;
            }
        }
        real_message_size = ntohl(message_size);

        ///end accepting the size of the message


        ///start accepting the message
        message_buffer = (char*)malloc(real_message_size * sizeof(char));
        total_bytes = 0;

        while(1){
            bytes_read = read(connfd, message_buffer+total_bytes, real_message_size-total_bytes);
            if( bytes_read < 0 ){
                if(errno==ETIMEDOUT || errno==ECONNRESET || errno==EPIPE){
                    perror("TCP Error in read from client\n");
                    collapsed = 1;
                }
                else{
                    perror("Error in read from client\n");
                    exit(1);
                }
            }

            total_bytes += bytes_read;
            if(total_bytes==real_message_size){
                break;
            }
        }

//        printf("the string we get == %s\n", message_buffer);

        int count = count_printable(message_buffer, real_message_size);

//        printf("count == %d\n", count);
        ///end accepting the message

        ///start writing count back to client

        int count_to_send = htonl(count);
        data_buff = (char*)&count_to_send;

        totalsent = 0;

        while( totalsent < 4 )
        {
            nsent = write(connfd,
                          data_buff + totalsent,
                          4);

            if(nsent<0){
                if(errno==ETIMEDOUT || errno==ECONNRESET || errno==EPIPE){
                    perror("TCP Error in write to client\n");
                    collapsed = 1;
                }
                else{
                    perror("Error in write to client\n");
                    exit(1);
                }
            }

            totalsent  += nsent;
        }

        free(message_buffer);
        ///end writing count back to client

        //update pcc_total if not collapsed
        if(!collapsed){
            update_pcc_total();
        }

        close(connfd);

        processing_client = 0;
        ///total_bytes must be equal to 4

    }

    print_stats();
    exit(0);

}


