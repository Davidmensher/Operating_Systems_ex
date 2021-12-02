#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
    int file_desc;
    int ret_val;
    int i;
    unsigned long channel_id;
    char* path;
    char buffer[BUF_LEN];
    
    if(argc != 3){
        perror("Number of args different than 4\n");
        exit(1);
    }

    
    path = argv[1];
    file_desc = open(path, O_RDWR );
    if( file_desc < 0 )
    {
        perror("faild open file\n");
        exit(1);
    }
    
    channel_id = atoi(argv[2]);
    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id);
    if(ret_val != 0){
        perror("failed in ioctl\n");
        exit(1);
    }
    
    i = read(file_desc, buffer, BUF_LEN);
    if(i < 0){
        perror("failed in read\n");
        exit(1);
    }
   
    ret_val = write(STDOUT_FILENO, buffer,i);
    if(ret_val == -1){
        perror("faild write to STDOUT\n");
        exit(-1);
    }
    i = write(STDOUT_FILENO,"\n", 2);

    if(ret_val == -1){
        perror("faild write to STDOUT\n");
        exit(-1);
    }

    close(file_desc);
    return 0;
}

