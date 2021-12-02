#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 240

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define DEVICE_RANGE_NAME "message_slot"
#define SUCCESS 0
#define BUF_LEN 128


struct Inner_Node{
    char the_message[BUF_LEN]; //should be a array of bytes?
    unsigned long channel_id; //a field specifying the channel id
    int message_len;
    struct Inner_Node* next;
};

struct file_info_Node{
    int minor;
    struct Inner_Node* curr_channel_node;
};

#endif //HW4_OS_MESSAGE_SLOT_H
