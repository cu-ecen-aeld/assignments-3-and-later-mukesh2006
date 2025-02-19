/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include "aesdchar.h"
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "aesdchar.h"
#include "aesd_ioctl.h"

#define ERROR -1

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Mukesh Jha");
MODULE_LICENSE("Dual BSD/GPL");

void aesd_cleanup_module(void);
int aesd_init_module(void);
int aesd_open(struct inode *inode, struct file *filp);

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence) ;
//long aesd_ioctl(struct file *filp, unsigned int command, unsigned long arg) ;
//int aesd_release(struct inode *inode, struct file *filp);

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    //PDEBUG("\naesd_open: open");
    struct aesd_dev *tmp_dev;
    tmp_dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    //PDEBUG("\naesd_open: entry buffer pointer val: %p",tmp_dev->entry);
    filp->private_data = tmp_dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
  //  PDEBUG("\naesd_release: release");    
    filp->private_data = NULL;
    return 0;
}


ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = ERROR;
    struct aesd_dev *dev = filp->private_data;
    size_t entry_offset = 0;
    size_t remaining_bytes = 0;
    PDEBUG("\naesd_read: read %zu bytes with offset %lld", count, *f_pos);


    if (filp == NULL || buf == NULL || f_pos == NULL || *f_pos < 0)
    {
        PDEBUG("\naesd_read: Invalid arguments");
        return retval;
    }

    if (mutex_lock_interruptible(&dev->mutex))
    {
        PDEBUG("\naesd_read: Failed to lock mutex");
        return -ERESTARTSYS;
    }
    PDEBUG("\naesd_read: Mutex locked");

    PDEBUG("\n aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer: %p, *f_pos: %d, &entry_offset: %p) ", dev->buffer, *f_pos, &entry_offset);

    struct aesd_buffer_entry *temp_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_offset);
    if (temp_entry == NULL)
    {
        PDEBUG("Failed to find entry for given file position");
        mutex_unlock(&dev->mutex);
        PDEBUG("Mutex unlocked due to entry not found");
        return retval;
    }

    remaining_bytes = temp_entry->size - entry_offset;
    if (remaining_bytes >= count)
        remaining_bytes = count;
	
    unsigned long bytes_not_copied = copy_to_user(buf, temp_entry->buffptr + entry_offset, remaining_bytes);
    if (bytes_not_copied != 0)
    {
        PDEBUG("Failed to copy data to user space");
        mutex_unlock(&dev->mutex);
        PDEBUG("Mutex unlocked due to copy failure");
        return -EFAULT;
    }
            
    *f_pos += remaining_bytes;
    mutex_unlock(&dev->mutex);
    PDEBUG("Mutex unlocked");
    retval = remaining_bytes;

    return retval;
 
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    PDEBUG("\naesd_write: write %zu bytes with offset %lld",count,*f_pos);
    ssize_t pos = 0;
    int prev_length = 0;
    bool rec_complete = false;
    struct aesd_dev *dev = NULL;
    char *data_ptr;
    char *ptr;

    // Check for null pointers
    if (filp == NULL || buf == NULL || f_pos == NULL)
    {
        return -EINVAL;
    }


    PDEBUG("\naesd_write: write %zu bytes with offset %lld", count, *f_pos);

    // Check for negative file position
    if (*f_pos < 0)
    {
        return -EINVAL;
    }
    // a. Allocate memory for each write command as it is received, supporting any length of write request (up to the length of memory which can be allocated through kmalloc), and saving the write command content within allocated memory.  
    // v. For the purpose of this assignment you can use kmalloc for all allocations regardless of size, and assume writes will be small enough to work with kmalloc.
    data_ptr = kmalloc(count, GFP_KERNEL);
    if (data_ptr == NULL) 
    {
        PDEBUG("\naesd_write: Memory Allocation Failed\n");
        return -ENOMEM;
    }

    // Copy data to the kernel space 
    if (copy_from_user(data_ptr, buf, count)) {
        PDEBUG("\naesd_write: Data copy failed\n");
        kfree(data_ptr);
        return -EFAULT;
    }

    // Search for newline character i. The write command will be terminated with a \n character as was done with the aesdsocket application. 
    //1. Write operations which do not include a \n character should be saved and appended by future write operations and should not be written to the circular buffer until terminated.
    ptr = memchr(data_ptr, '\n', count);
    if (ptr == NULL) 
    {
        pos = count;
    } else 
    {
        pos = ptr - data_ptr + 1;
        rec_complete = true;
    }

    // Lock mutex
    dev = filp->private_data;
    if (dev == NULL)
    {
        return -EINVAL;
    }


    //PDEBUG("\naesd_write: Locking mutex");
    if (mutex_lock_interruptible(&dev->mutex)) 
    {
        kfree(data_ptr);
        PDEBUG("\naesd_write: Failed to lock mutex");
        return -ERESTARTSYS;
    }
    //PDEBUG("\naesd_write: Mutex locked");

    prev_length = dev->entry.size;
    dev->entry.size += pos;

    // Reallocate buffer
    //PDEBUG("Reallocation start");
    char *new_data_ptr = krealloc(dev->entry.buffptr, dev->entry.size, GFP_KERNEL);
    if (new_data_ptr == NULL) 
    {
        //PDEBUG("Memory reallocation Failed\n");
        kfree(data_ptr);
        mutex_unlock(&dev->mutex);
        //PDEBUG("Mutex unlocked due to reallocation failure");
        return -ENOMEM;
    }
    //PDEBUG("Reallocation complete");

    dev->entry.buffptr = new_data_ptr; // Update buffptr to point to the new memory location

    // Copy data to buffer
    memcpy(dev->entry.buffptr + prev_length, data_ptr, pos);

    // Add entry to circular buffer if complete record received
    if (rec_complete) 
    {
        const char *ret_ptr = aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
        if (ret_ptr != NULL) 
        {
            kfree(ret_ptr);
        }
        
        dev->entry.size = 0;
        dev->entry.buffptr = NULL;
    }

    // Unlock mutex and clean up
   // PDEBUG("Unlocking mutex");
    mutex_unlock(&dev->mutex);
   // PDEBUG("Mutex unlocked");
    kfree(data_ptr);
    return pos;
}


static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset) {
    if (filp == NULL)
        return -EINVAL;

    struct aesd_dev *dev = NULL;
    unsigned int offset = 0;
    int i;

    dev = filp->private_data;
    if (dev == NULL)
        return -EINVAL;

    PDEBUG("Locking mutex");
    // Lock the mutex 
    if (mutex_lock_interruptible(&dev->mutex)) {
        // Failed to acquire mutex
        PDEBUG("Failed to lock mutex");
        return -ERESTARTSYS; 
    }
    PDEBUG("Mutex locked");

    // Checking if write_cmd is within supported range or not
    if (write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
        PDEBUG("Invalid write command: %d", write_cmd);
        mutex_unlock(&dev->mutex);
        return -EINVAL; 
    }
    
    
    for (i = 0; i < write_cmd; i++) {
        offset += dev->buffer.entry[i].size;
    }

    // Checking if write_cmd_offset is within the size of the specified write command or not
    if (write_cmd_offset >= dev->buffer.entry[write_cmd].size) {
        PDEBUG("Invalid write command offset: %d", write_cmd_offset);
        mutex_unlock(&dev->mutex);
        return -EINVAL; // Invalid write command offset
    }

   
    filp->f_pos = offset + write_cmd_offset;

    PDEBUG("File position updated to: %d", filp->f_pos);

    mutex_unlock(&dev->mutex); // Release the mutex
    return 0; // Success
}

long aesd_ioctl(struct file *filp, unsigned int command, unsigned long arg) {

    struct aesd_seekto aesd_seekto;
    long ret_val = 0;
    
    if (filp == NULL)
        return -EINVAL;


    switch (command) {

    case AESDCHAR_IOCSEEKTO:   
        loff_t totalCnt = 0;
        // Copy parameters from user space
        if (copy_from_user(&aesd_seekto, (const void __user *)arg, sizeof(aesd_seekto)) != 0) {
            // Failed to copy data from user space
            PDEBUG("Data copy failed\n");
            return -EFAULT;
        } else {
/*            // Adjusting file offset
            ret_val = aesd_adjust_file_offset(filp, aesd_seekto.write_cmd, aesd_seekto.write_cmd_offset);
             PDEBUG("aesd_ioctl: aesd_adjust_file_offset: return_val: %d ", ret_val);
            if (ret_val != 0)
                return -EFAULT;

*/

size_t i=0;

                while(i < aesd_seekto.write_cmd)
                {
                    //totalCnt += aesd_device.circBuff.entry[i].size;

                    totalCnt +=aesd_device.buffer.entry[i].size;
                    i++;
                }

                totalCnt += aesd_seekto.write_cmd_offset;

                filp->f_pos = totalCnt;
                PDEBUG("1111111111111111111 aesd_ioctl: aesd_adjust_file_offset: filp->f_pos: %d ", filp->f_pos);
        }

        break;
    default:
        // Invalid ioctl command
        ret_val = -ENOTTY;
        break;
    }

    return ret_val;
}


struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_ioctl, 
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
	aesd_circular_buffer_init(&aesd_device.buffer);
	mutex_init(&aesd_device.mutex);


    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    struct aesd_buffer_entry *entry;
    uint8_t index = 0;

    // free all entries in circular buffer
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index)
    {
        if(entry->buffptr != NULL)
        {
	        kfree(entry->buffptr);
        }

    }  
    mutex_destroy(&aesd_device.mutex);
    unregister_chrdev_region(devno, 1);
}

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence) {
 
    struct aesd_dev *dev = NULL;
    loff_t f_offset = 0;
    uint8_t index = 0;
    struct aesd_buffer_entry *entry;
    loff_t total_size = 0;

    if (filp == NULL)
        return -EINVAL;


    dev = filp->private_data;
    if (dev == NULL)
        return -EINVAL;

    PDEBUG("Locking mutex");
    // Lock the mutex 
    if (mutex_lock_interruptible(&dev->mutex)) {
        // Failed to acquire mutex
        PDEBUG("Failed to lock mutex");
        return -ERESTARTSYS; 
    }
    PDEBUG("Mutex locked");

    // Calculating the total size of the circular buffer
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buffer, index) {
        total_size += entry->size;
    }
    mutex_unlock(&dev->mutex);


    f_offset = fixed_size_llseek(filp, offset, whence, total_size);

    return f_offset;
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
