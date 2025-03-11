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
#include <linux/uaccess.h> // copy_to_user and copy_from_user
#include <linux/slab.h> // For kmalloc, krealloc
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("David Vuong"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

static long aesd_seekto(struct file *filp, unsigned int str_index, unsigned int str_offset)
{
    struct aesd_dev *dev = NULL;
    struct aesd_circular_buffer *buffer = NULL;
    struct aesd_buffer_entry *entry = NULL;
    uint8_t index;
    size_t retval = 0;
    size_t pos = 0;
    uint8_t found = 0;

    PDEBUG("aesd_seekto");

    if (str_index >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        PDEBUG("str_index (%d) out of max range (10)", str_index);
        return -EINVAL;
    }

    dev = (struct aesd_dev *)(filp->private_data);
    if (!dev) return -EFAULT;

    buffer = &dev->buf;

    if (mutex_lock_interruptible(&dev->m) != 0)
    {
        PDEBUG("Unable to acquire mutex");
        return -ERESTARTSYS;
    }

    if (buffer->entry[str_index].buffptr == NULL)
    {
        PDEBUG("No data at index %d", str_index);
        retval = -EINVAL;
        goto done;
    }

    AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index)
    {
        PDEBUG("Index %d", index);
        if (index == str_index)
        {
            if (str_offset >= entry->size)
            {
                PDEBUG("str_offset (%d) is greater than the last entry size (%d)", str_offset, entry->size);
                retval = -EINVAL;
                goto done;
            }

            pos += str_offset;
            PDEBUG("Found the element we want. Updating pos to %d", pos);
            found = 1;
            break;
        }
        PDEBUG("Iterating through index %d", index);
        pos += entry->size;
    }

    if (found)
    {
        filp->f_pos = pos;
    }

done:
    mutex_unlock(&dev->m);
    return retval;
}

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = NULL;

    PDEBUG("open");

    if (!inode || !filp) return -1;

    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    ssize_t n_bytes_read = 0;
    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry = NULL;
    size_t entry_offset = 0;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    if (!filp || !buf || !f_pos)
    {
        return -EFAULT;
    }

    dev = (struct aesd_dev *)(filp->private_data);
    if (!dev)
    {
        return -EFAULT;
    }

    if (mutex_lock_interruptible(&dev->m) != 0)
    {
        return -ERESTARTSYS;
    }

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buf, (size_t)(*f_pos), &entry_offset);
    if (entry == NULL)
    {
        *f_pos = 0;
        retval = 0;
        goto done;
    }

    n_bytes_read = entry->size - entry_offset;
    n_bytes_read = (count > n_bytes_read) ? n_bytes_read : count;

    if(copy_to_user(buf, entry->buffptr + entry_offset, n_bytes_read))
    {
        retval = -EFAULT;
        goto done;
    }

    *f_pos += n_bytes_read;
    retval = n_bytes_read;
done:
    mutex_unlock(&dev->m);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry = NULL;
    char *free_entry = NULL;
    int i = 0;

    if (!filp || !buf || !f_pos) return -EFAULT;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    dev = (struct aesd_dev *)(filp->private_data);
    if (!dev)
    {
        return -EFAULT;
    }

    if (mutex_lock_interruptible(&dev->m) != 0)
    {
        return -ERESTARTSYS;
    }

    entry = &dev->entry;

    if (entry->size == 0)
    {
        entry->buffptr = kmalloc(sizeof(char) * count, GFP_KERNEL);
    }
    else
    {
        entry->buffptr = (char *)krealloc(entry->buffptr, count + entry->size, GFP_KERNEL);
    }

    if (!entry->buffptr)
    {
        retval = -ENOMEM;
        goto done;
    }

    if (copy_from_user((char *)(entry->buffptr + entry->size), buf, count))
    {
        retval = -EFAULT;
        goto done;
    }

    retval = count;
    entry->size += count;

    for(i = 0; i < entry->size; i++)
    {
        if (entry->buffptr[i] == '\n')
        {
            free_entry = aesd_circular_buffer_add_entry(&dev->buf, entry);
            if (free_entry) kfree(free_entry);

            entry->size = 0;
            entry->buffptr = NULL;
        }
    }

    // *f_pos += retval;
done:
    mutex_unlock(&dev->m);
    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    struct aesd_dev *dev = NULL;
    loff_t new_pos;

    if (!filp) return -EFAULT;

    dev = (struct aesd_dev *)(filp->private_data);
    if (!dev) return -EFAULT;

    switch (whence)
    {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = filp->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = dev->buf.size + offset;
            break;
        default:
            return -EINVAL;
    }

    if (new_pos < 0 || new_pos > dev->buf.size)
    {
        return -EINVAL;
    }

    filp->f_pos = new_pos;
    return new_pos;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_seekto seekto;
    ssize_t retval = 0;

    if (!filp) return -EFAULT;

    switch(cmd)
    {
        case AESDCHAR_IOCSEEKTO:
            if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0)
            {
                retval = -EFAULT;
            }
            else
            {
                retval = aesd_seekto(filp, seekto.write_cmd, seekto.write_cmd_offset);
            }
            break;
        default:
            return -EINVAL;
    }

    return retval;
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

    mutex_init(&aesd_device.m);
    aesd_circular_buffer_init(&aesd_device.buf);

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
    aesd_circular_buffer_deinit(&aesd_device.buf);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
