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
#include "aesd-circular-buffer.h"
//#define __KERNEL__

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Jahnavi Pinnamaneni"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;




int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev = NULL;
	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
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
	ssize_t offset = 0;
	ssize_t bytes_to_user = 0;
	ssize_t count_bytes = 0;
	struct aesd_buffer_entry * read_entry = NULL;
	struct aesd_dev *dev = filp->private_data;
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */
	
	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;
	
	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->device_buffer, *f_pos, &offset);

	if(read_entry == NULL)
	{
		PDEBUG("Error: aesd_read: No data found\n");
		goto out;
	}

	if((read_entry->size - offset) > count)
		count_bytes = count;
	else
		count_bytes = (read_entry->size - offset);

	bytes_to_user = copy_to_user(buf, &read_entry->buffptr[offset], count_bytes);
	if (bytes_to_user) {
		retval = 0;
		goto out;
	}

	*f_pos = *f_pos + count_bytes;
	retval = count_bytes;

  out:
	mutex_unlock(&dev->lock);
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct aesd_dev *dev = filp->private_data;
	size_t copy_from_user_bytes;
	ssize_t retval = -ENOMEM;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	if(dev->entry.size == 0)
		dev->entry.buffptr = kzalloc(count*(sizeof(char)),GFP_KERNEL);
	else
		dev->entry.buffptr = krealloc(dev->entry.buffptr,(dev->entry.size + count), GFP_KERNEL);
	if(dev->entry.buffptr == NULL)
	{
		PDEBUG("ERROR: aesd_write, no memory available for dynamic allocation\n");
		goto out;
	}

	copy_from_user_bytes = copy_from_user((void*)&dev->entry.buffptr[dev->entry.size], buf, count);
	if (copy_from_user_bytes) {
		retval = -EFAULT;
		goto out;
	}
	dev->entry.size += count;
	retval = count;

	PDEBUG("Recieved data\n");
	if(strchr((char*)(dev->entry.buffptr),'\n'))
	{
		PDEBUG("Detected \n");
		if(dev->device_buffer.full)
		{
			kfree(dev->device_buffer.entry[dev->device_buffer.out_offs].buffptr);
		}
			aesd_circular_buffer_add_entry(&dev->device_buffer, &dev->entry);
			dev->entry.buffptr = NULL;
			dev->entry.size = 0;
			PDEBUG("Write complete no full\n");
	}
		
  out:
	mutex_unlock(&dev->lock);
	return retval;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
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

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */

	mutex_init(&aesd_device.lock);
	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	uint8_t index;

	struct aesd_buffer_entry *entry;
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	//AESD_CIRCULAR_BUFFER_FOREACH
	kfree(aesd_device.entry.buffptr);
  	AESD_CIRCULAR_BUFFER_FOREACH(entry,&aesd_device.device_buffer,index) {
  		kfree(entry->buffptr);
	}
	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
