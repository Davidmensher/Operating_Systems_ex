#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>

// MINIMAL ERROR HANDLING FOR EASE OF READING

int main(int argc, char *argv[])
{
  int  sockfd     = -1;
  int  bytes_read =  0;
  int totalsent;
  int nsent = 0;
  char* data_buff;
  int printable_counter;
  char* count_buff;
  int total_bytes;
  char* IP_Address;
  short server_port;
  char* path_to_file;
  char* data_buff2;
  int length;
  FILE* f;
  struct sockaddr_in serv_addr; // where we Want to get to
  int fread_ret;

  //struct sockaddr_in my_addr;   // where we actually connected through
  //struct sockaddr_in peer_addr; // where we actually connected to
//  socklen_t addrsize = sizeof(struct sockaddr_in );

  if(argc != 4){
      perror("\n Error : Number of arguments differ than 3 \n");
      exit(1);
  }

  IP_Address = argv[1];
  server_port = atoi(argv[2]);
  path_to_file = argv[3];

  //memset(recv_buff, 0,sizeof(recv_buff));
  if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("\n Error : Could not create socket \n");
      exit(1);
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(server_port); // Note: htons for endiannes
  inet_pton(AF_INET, IP_Address, &serv_addr.sin_addr);
  //serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded...

  ///reading data from file
  f = fopen (path_to_file, "rb");

  if (!f)
  {
      perror("Error in opening file");
      exit(1);
  }

  fseek (f, 0, SEEK_END);
  length = ftell (f);
  fseek (f, 0, SEEK_SET);
  data_buff2 = malloc (length * sizeof(char));
  if (data_buff2)
  {
      fread_ret = fread(data_buff2, 1, length, f);
      if(fread_ret == 0){
          perror("Error in fread");
          exit(1);
      }
  }
  fclose (f);

  ///finish reading data from file


  if( connect(sockfd,
              (struct sockaddr*) &serv_addr,
              sizeof(serv_addr)) < 0)
  {
    perror("\n Error : Connect Failed. %s \n");
    return 1;
  }


//    data_buff2 = "HeyIamDavid";
//    int num = strlen(data_buff2);
    int num_to_send = htonl(length);
    data_buff = (char*)&num_to_send;

    totalsent = 0;
    int notwritten = 4;

    // keep looping until nothing left to write
    while( totalsent < 4 )
    {
        nsent = write(sockfd,
                      data_buff + totalsent,
                      4);

        if(nsent<0){
            perror("Error in Client syscall\n");
            exit(1);
        }

        totalsent  += nsent;
    }

    ///write message to server
    totalsent = 0;
    notwritten = length;
    // keep looping until nothing left to write
//    printf("### data_buff2 == %s\n", data_buff2);
    while( notwritten > 0 )
    {
        nsent = write(sockfd,
                      data_buff2 + totalsent,
                      notwritten);

        if(nsent<0){
            perror("Error in Client syscall\n");
            exit(1);
        }

        totalsent  += nsent;
        notwritten -= nsent;
    }

    ///done writings!

    ///read int from client
    count_buff = (char*)&printable_counter;
    total_bytes = 0;

    while (1){
        bytes_read = read(sockfd, count_buff+total_bytes, 4-total_bytes);
        if( bytes_read < 0 ){
            perror("Error in Client syscall\n");
            exit(1);
        }

        total_bytes += bytes_read;
        if(bytes_read<=0){
            break;
        }
    }
    int real_printable_counter = ntohl(printable_counter);


    printf("# of printable characters: %u\n", real_printable_counter);

  close(sockfd); // is socket really done here?
  //printf("Write after close returns %d\n", write(sockfd, recv_buff, 1));
  return 0;
}
