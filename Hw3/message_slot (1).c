#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
//Our custom definitions of IOCTL operations

#include "message_slot.h"
MODULE_LICENSE("GPL");

struct Inner_Node* files_channels[256];
int busy_array[256];
//================== free FUNCTION ===========================
static void free_list(struct Inner_Node* inner_remove_node){

    //freeing the inner linked list (the list of the channels)
    struct Inner_Node* next_inner = inner_remove_node->next;
    while(next_inner != NULL){
        kfree(inner_remove_node);
        inner_remove_node = next_inner;
        next_inner = inner_remove_node->next;
    }

    kfree(inner_remove_node);

}

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file)
{
    int curr_minor = iminor(inode);
    struct file_info_Node* f_node;


    f_node = (struct file_info_Node*)kmalloc(sizeof(struct file_info_Node),GFP_KERNEL);
    f_node->minor = curr_minor;
    f_node->curr_channel_node = NULL;

    busy_array[curr_minor] = 1;

    file->private_data = (void*)(f_node);
    
    printk("Done opening device file, 101\n");
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
//    int curr_minor;
    struct file_info_Node* f_node;
//    struct Inner_Node* first;
    f_node = (struct file_info_Node*)file->private_data;

    printk("Invoking device_release\n");

    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{

    int i;
    struct file_info_Node* f_node;
    char* the_message;

    f_node = (struct file_info_Node*)file->private_data;
    printk("invoking device read! minor= %d \n", f_node->minor);

    if(f_node->curr_channel_node->channel_id == 0){
        return -EINVAL;
    }
    if(f_node->curr_channel_node->message_len == 0){
        return -EWOULDBLOCK;
    }
    if(length < f_node->curr_channel_node->message_len){
        return -ENOSPC;
    }

    the_message = (f_node->curr_channel_node)->the_message;

    for( i = 0; i < f_node->curr_channel_node->message_len && i < BUF_LEN; ++i ){
        put_user(the_message[i], &buffer[i]);
    }

//invalid argument error
     return i;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    int i;
    struct file_info_Node* f_node = (struct file_info_Node*)file->private_data;
    char* the_message;
    
    printk("invoking device write! minor= %d \n", f_node->minor);

    if(f_node->curr_channel_node->channel_id == 0){
        return -EINVAL;
    }
    if(length > 128 || length==0){
        return -EMSGSIZE;
    }

    the_message = (f_node->curr_channel_node)->the_message;

    for( i = 0; i < length && i < BUF_LEN; ++i )
    {
        get_user(the_message[i], &buffer[i]);
    }

    f_node->curr_channel_node->message_len = i;
    // return the number of input characters used

    return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long channel_id)
{
    int flag = 0;
    struct file_info_Node* f_node;
    int curr_minor;
    struct Inner_Node* inner_Node;
    struct Inner_Node* prev;
    struct Inner_Node* new_head;
    struct Inner_Node* new_node;
    
    f_node = (struct file_info_Node*)file->private_data;
    curr_minor = f_node->minor;
    inner_Node = files_channels[curr_minor];

    printk("in ioctl\n");
    if(channel_id == 0){
        return -EINVAL;
    }

    if(f_node->curr_channel_node != NULL){
        if(f_node->curr_channel_node->channel_id == channel_id){
            return SUCCESS;
        }
    }
   
    //we need to change the curr_channel_node, creat new one if needed

    if(MSG_SLOT_CHANNEL == ioctl_command_id )
    {

        if(inner_Node == NULL){
            new_head=  (struct Inner_Node*)kmalloc(sizeof(struct Inner_Node),GFP_KERNEL);
            if(new_head==NULL){
                return -ENOMEM;
            }
            new_head->channel_id = channel_id;
            new_head->message_len = 0;
            new_head->next = NULL;
            files_channels[curr_minor] = new_head;

            f_node->curr_channel_node = new_head;

            file->private_data = (void*)(f_node); //needed?

        }
        else{ //channels list isn't empty
           
            while(inner_Node != NULL && inner_Node->channel_id != channel_id){
                prev = inner_Node;
                inner_Node = inner_Node->next;
            }

            if(inner_Node != NULL){
                if(inner_Node->channel_id == channel_id){
                    flag = 1;
                    f_node->curr_channel_node = inner_Node;
                    file->private_data = (void*)(f_node);
                }
            }

            if(flag == 0){ // channel doesn't exist yet, we will creat it
               
                new_node=  (struct Inner_Node*)kmalloc(sizeof(struct Inner_Node),GFP_KERNEL);
                if(new_node==NULL){
                    return -ENOMEM;
                }

                new_node->channel_id = channel_id;
                new_node->message_len = 0;
                new_node->next =NULL;

                prev->next = new_node;

                f_node->curr_channel_node = new_node;

                file->private_data = (void*)(f_node); //needed?
            }
        }
    }
    else{
        return -EINVAL;
    }

    return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
            .owner	        = THIS_MODULE,
            .read           = device_read,
            .write          = device_write,
            .open           = device_open,
            .unlocked_ioctl = device_ioctl,
            .release        = device_release,
        };
//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{   
    int rc = -1;
    
    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );
    // Negative values signify an error
    

    if( rc < 0 )
    {
        printk( KERN_ALERT " registraion failed for negative file_desc\n");
        return rc;
    }

    printk("done init\n");

    return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    int i;
    // Unregister the device
    // Should always succeed

    //do we really need to free everything?
    for(i = 0; i < 256; i++){
        if(files_channels[i] != NULL){
            free_list(files_channels[i]);
        }
    }

    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    printk("Done cleanup!\n");
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================


