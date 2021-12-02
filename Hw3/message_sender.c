#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]){
    int file_desc;
    int ret_val;
    int i;
    unsigned long channel_id;
    int length;
    char* path;
    char* the_message;

    if(argc != 4){
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

    the_message = argv[3];
    length = strlen(the_message);
    i = write(file_desc, the_message, length);

    if(i != length){
        perror("didnt writed everything\n");
        exit(1);
    }

    close(file_desc);

    return 0;
}
