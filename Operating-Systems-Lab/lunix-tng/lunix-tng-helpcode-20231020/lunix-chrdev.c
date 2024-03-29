/*
 * lunix-chrdev.c
 *
 * Implementation of character devices
 * for Lunix:TNG
 *
 * Konstantinos Papadopoulos 03120152
 * Areti Mei 03120062
 *
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "lunix.h"
#include "lunix-chrdev.h"
#include "lunix-lookup.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*
 * Global data
 */
struct cdev lunix_chrdev_cdev;

static int lunix_chrdev_state_needs_refresh(struct lunix_chrdev_state_struct *state)
{

	struct lunix_sensor_struct *sensor;
	int type;
	uint32_t last_update_of_sensor;
	uint32_t last_update_of_buffer;
    
   	 sensor = state->sensor;
   	 type = state->type;

	last_update_of_sensor= sensor->msr_data[type]->last_update;

	last_update_of_buffer = state->buf_timestamp;
	debug("last update of senor %d",last_update_of_sensor );
	//debug("%u",last_update_of_sensor);
	debug("last update of buffer %d",last_update_of_buffer);
	return (last_update_of_sensor > last_update_of_buffer);
}

/*
 * Updates the cached state of a character device
 * based on sensor data. Must be called with the
 * character device state lock held.
 */
static int lunix_chrdev_state_update(struct lunix_chrdev_state_struct *state)
{
	struct lunix_sensor_struct *sensor;
	uint32_t data;
   	 uint32_t time;
   	 int ret;

   	 WARN_ON(!(sensor = state->sensor));

	/*
 	* Any new data available?
 	*/
    
	if(lunix_chrdev_state_needs_refresh(state)) {
  	  /*
  	   * Grab the raw data quickly, hold the
  	   * spinlock for as little as possible.
  	   */
  	  spin_lock_irq(&sensor->lock);
  	  data = sensor->msr_data[state->type]->values[0];
  	  time = sensor->msr_data[state->type]->last_update;
  	  spin_unlock_irq(&sensor->lock);
  	 
  	  /* Why use spinlocks?  acquires a spin lock and disables interrupts on the local CPU. It is used to protect critical sections of code from concurrent access by multiple processors or interrupt handlers. any code must, while holding a spinlock, be atomic and  cannot sleep*/

  	 
  	  /*
  	   * Now we can take our time to format them,
  	   * holding only the private state semaphore
  	   */
  	   
  	  if(state->type == TEMP) state->buf_lim = sprintf(state->buf_data, "%ld.%03ldC\t", lookup_temperature[data]/1000, lookup_temperature[data]%1000);
  	  else if(state->type == BATT) state->buf_lim = sprintf(state->buf_data, "%ld.%02ldpercent\t", lookup_voltage[data]/100, lookup_voltage[data]%100);
  	  else if(state->type == LIGHT) state->buf_lim = sprintf(state->buf_data, "%ldCd\t", lookup_light[data]);
  	  state->buf_timestamp = time;
  	  ret = 0;
	 }
   	 else {
       		 ret = -EAGAIN;
   	 }
   	 
   	debug("leaving update\n");
	return ret;
}

/*************************************
 * Implementation of file operations
 * for the Lunix character device
 *************************************/

//kathe state anaferetai se mia sysgkekrimenh metrhsh, dhladh se sygkekrimeno sensor kai typo metrhshs

static int lunix_chrdev_open(struct inode *inode, struct file *filp)
{
	struct lunix_chrdev_state_struct *chrdev_state;
	unsigned int sensor, type;
	int ret;

	//debug("entering open: %d\n", sensor);
	ret = -ENODEV;

	/* Write operation is not permitted */
	if ((filp->f_flags & O_WRONLY) || (filp->f_flags & O_RDWR))
  	  return -EPERM;

	/* Device does not support seeking */
	if ((ret = nonseekable_open(inode, filp)) < 0)
  	  goto out;
	/*
 	* Associate this open file with the relevant sensor based on
 	* the minor number of the device node [/dev/sensor<NO>-<TYPE>]
 	*/
	sensor = iminor(inode) / 8;
	type = iminor(inode) % 8;
    
	/* Allocate a new Lunix character device private state structure */
	if (!(chrdev_state = kmalloc(sizeof(struct lunix_chrdev_state_struct), GFP_KERNEL))) {
  	  printk(KERN_ERR "Failed to allocate memory for character device private state\n");
  	  ret = -ENOMEM;
  	  goto out;
	}

	/* Initialization of the device private state structure */
	chrdev_state->type = type;
	chrdev_state->sensor = &lunix_sensors[sensor];
	chrdev_state->buf_lim = 0;
	chrdev_state->buf_timestamp = 0;
	sema_init(&chrdev_state->lock, 1);
	
	/* Save the device state */
	filp->private_data = chrdev_state;
	debug("character device state initialized successfully\n");



	ret = 0;    
out:
	debug("leaving, with ret = %d\n", ret);
	return ret;
}

static int lunix_chrdev_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data); //ousiastika diagrafoume to state mesa sto file pointer
	return 0;
}

static long lunix_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return -EINVAL;
}

static ssize_t lunix_chrdev_read(struct file *filp, char __user *usrbuf, size_t cnt, loff_t *f_pos)
{


    ssize_t ret = 0;
    struct lunix_sensor_struct *sensor;
    struct lunix_chrdev_state_struct *state;
    size_t bytes_to_copy;


    state = filp->private_data;


    sensor = state->sensor;

    debug("entering read");
    /* Lock? */
    if (down_interruptible(&state->lock)) { //the process sleeping on the semaphore can be woken up by a signal,this is what we want in most cases.
   	 ret = -ERESTARTSYS;  // the system call should be restarted because it was interrupted by a signal.
   	 goto out;
    }

    /*
	 * If the cached character device state needs to be
	 * updated by actual sensor data (i.e., we need to report
	 * on a "fresh" measurement), do so
	 */
    
    if (*f_pos == 0) {//we are at the beggining of the file, 
   	 if (lunix_chrdev_state_update(state) == -EAGAIN) { //if there are no new available data to update the state, we sleep
   	         up(&state->lock);
   		 /* The process needs to sleep until new data arrive*/
   		 if (wait_event_interruptible(sensor->wq, lunix_chrdev_state_update(state) != -EAGAIN)) {   //The process adds itself to the wait queue (sensor->wq) and goes to sleep if the condition (lunix_chrdev_state_update(state) != -EAGAIN) is not true, if we dont have new data available. When new data arrives, the condition becomes falso, so the process is awake and proceeds. 		 
	       		 ret = -ERESTARTSYS;
	       		 goto out;
   		 }
   		 down_interruptible(&state->lock);
   	 }
    }
    
    
    debug( "efyge apo if f_pos==0");
    /* Determine the number of cached bytes to copy to userspace */
    bytes_to_copy = MIN(cnt, (size_t)(state->buf_lim - *f_pos));

    /* Copy data to userspace */
    if (copy_to_user(usrbuf, state->buf_data + *f_pos, bytes_to_copy)) {
   	 ret = -EFAULT; // Error copying data to userspace
   	 goto out;
    }
    debug( "copied dato to user: %lu", bytes_to_copy);
    /* Update file position and return the number of bytes read */
    *f_pos += bytes_to_copy;
    ret = bytes_to_copy;
    
    /* Auto-rewind on EOF mode */
    if (*f_pos == state->buf_lim)
	*f_pos = 0;

out:
    /* Unlock? */
    debug("leavinggg");
    up(&state->lock);
    return ret;
}

static int lunix_chrdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static struct file_operations lunix_chrdev_fops =
{
   	 .owner 		 = THIS_MODULE,
	.open  		 = lunix_chrdev_open,
	.release   	 = lunix_chrdev_release,
	.read  		 = lunix_chrdev_read,
	.unlocked_ioctl = lunix_chrdev_ioctl,
	.mmap  		 = lunix_chrdev_mmap
};

int lunix_chrdev_init(void)
{
	/*
 	* Register the character device with the kernel, asking for
 	* a range of minor numbers (number of sensors * 8 measurements / sensor)
 	* beginning with LINUX_CHRDEV_MAJOR:0
 	*/
	int ret;
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
    
	debug("Startring initialization\n");
        
	cdev_init(&lunix_chrdev_cdev, &lunix_chrdev_fops);
	
	
	lunix_chrdev_cdev.owner = THIS_MODULE; 
        
        
	// Create a device number using the major and minor numbers
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	
	// Reserving a range of character device numbers
	ret = register_chrdev_region(dev_no, lunix_minor_cnt, "lunix");
	if (ret < 0) {
  	  debug("failed to register region, ret = %d\n", ret);
  	  goto out;
	}    
    
	ret = cdev_add(&lunix_chrdev_cdev, dev_no, lunix_minor_cnt);
	if (ret < 0) {
  	  debug("failed to add character device\n");
  	  goto out_with_chrdev_region;
	}
    
	debug("Init completed successfully\n");
	return 0;

out_with_chrdev_region://when smthing goes wrong
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
out:
	return ret;
}

void lunix_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
  	 
	debug("entering\n");
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	cdev_del(&lunix_chrdev_cdev);
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
	debug("leaving\n");
}

