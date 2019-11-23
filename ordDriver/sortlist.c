/*----------------------------------------------------------------------------*/
/* File: sortlist.c                                                             */
/* Date: 12/11/19                                                           */
/* Author: Victor Campelo                                                       */
/* Version: 0.1                                                               */
/*----------------------------------------------------------------------------*/

/* This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/aio.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/sort.h> /* sort */
#include <linux/security.h>
#include <linux/uaccess.h> /* copy_from/to_user */
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
//MACROS: INFORMATIONS ABOUT MODULE
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joao Victor Campelo do Vale");
MODULE_DESCRIPTION("A module to ordenate a list of number");

#define SUCCESS 0
#define DEVICE_NAME "sortlist"
#define BUFFER_LENGTH 10
#define MAX_DSIZE	3071

/* GLOBAL */
int SIZE = 0;
int qtdNo = 0;
int memory_major = 0;
int bytes_operated = 0;
/* SET MAJOR NUMBER TO 0 WHEN USE DYNAMIC ALLOCATION */
int major=0;
//MODULE PARAM
//S_IRUGO - PERMISSION OF READ FOR ALL USERS
module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "device major number");

struct my_dev {
	char data[MAX_DSIZE+1];
	size_t size;              
    struct semaphore sem;     /* mutual exclusion */
	struct cdev      cdev;
	struct class     *class;
	struct device    *device;
} *temp_dev;

//----------------------------------------------------------------------------------------------
//LINKED LIST
struct stlista {
	int    value;
	char   c[BUFFER_LENGTH];
	struct list_head list_H;
};
//INITIALIZATION
LIST_HEAD(my_list);
//----------------------------------------------------------------------------------------------

/* STATEMENT OF FUNCTIONS  */
static int sort_open(struct inode *inode, struct file *filp);
static int sort_release(struct inode *inode, struct file *filp);
static ssize_t sort_read(struct file *filp,  char *buf, size_t count, loff_t *f_pos);
static ssize_t sort_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int sort_init(void);
void sort_exit(void);

/* STATEMENT OF FUNCTIONS init AND exit */
module_init(sort_init);
module_exit(sort_exit);

static struct file_operations temp_fops = {
	.owner   = THIS_MODULE,
	.read    = sort_read,
	.write   = sort_write,
	.open    = sort_open,
	.release = sort_release
};

int sort_init(void) {
	int rv;
	//MACRO TO CONVERT MAJOR AND MINOR NUMBET TO dev_t
	dev_t devno = MKDEV(major, 0);

	if(major) {
		//REGISTER DEVICE WITH MAJOT DEVNO 
		//1 = QUANTITY OF DEVICES, 
		//sortlist IS A NAME OF THE DEVICE
		rv = register_chrdev_region(devno, 1, DEVICE_NAME);
		if(rv < 0){
			printk(KERN_WARNING "Can't use the major number %d; try atomatic allocation...\n", major);
			rv = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
			major = MAJOR(devno);
		}
	}
	else {
        //DYNAMIC ALLOCATION DEVICE AND REGISTER. 
		//0 - minor number 1 - number of devices
		rv = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
		major = MAJOR(devno);
	}

	if(rv < 0) return rv;
    //PHYSICAL MEMORY ALLOCATION. 
	//GFP_KERNEL = BLOCK MEMORY PROCESS 
	temp_dev = kmalloc(sizeof(struct my_dev), GFP_KERNEL);
	if(temp_dev == NULL){
		rv = -ENOMEM;
		unregister_chrdev_region(devno, 1);
		return rv;
	}
    //initialize alocated memory with kmalloc
	memset(temp_dev, 0, sizeof(struct my_dev));
	cdev_init(&temp_dev->cdev, &temp_fops);
	temp_dev->cdev.owner = THIS_MODULE;
	temp_dev->size = MAX_DSIZE;
	sema_init (&temp_dev->sem, 1);
	//hand over struct cdev to VSF
	rv = cdev_add (&temp_dev->cdev, devno, 1);
	if (rv) printk(KERN_WARNING "Error %d adding device sortlist", rv);
	//create a class to use device_create
	temp_dev->class = class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(temp_dev->class)) {
		cdev_del(&temp_dev->cdev);
		unregister_chrdev_region(devno, 1);
		printk(KERN_WARNING "%s: can't create udev class\n", DEVICE_NAME);
		rv = -ENOMEM;
		return rv;
	}
    //create device file /dev automatically
	temp_dev->device = device_create(temp_dev->class, NULL,
		MKDEV(major, 0), "%s", DEVICE_NAME);
	if(IS_ERR(temp_dev->device)){
		class_destroy(temp_dev->class);
		cdev_del(&temp_dev->cdev);
		unregister_chrdev_region(devno, 1);
		printk(KERN_WARNING "%s: can't create udev device\n", DEVICE_NAME);
		rv = -ENOMEM;
		return rv;
	}

	printk(KERN_WARNING "Hello world from Template Module\n");
	printk(KERN_WARNING "sortlist device MAJOR is %d, dev addr: %lx\n", major, (unsigned long)temp_dev);

	return SUCCESS;
}

void sort_exit(void) {
	device_destroy(temp_dev->class, MKDEV(major, 0));
	class_destroy(temp_dev->class);
	cdev_del(&temp_dev->cdev); 
	kfree(temp_dev);
	unregister_chrdev_region(MKDEV(major, 0), 1);
	printk(KERN_WARNING "Good bye from Template Module\n");
}

static int sort_open(struct inode *inode, struct file *filp) {
	printk(KERN_DEBUG "[SORTLIST]: OPENING");
	return SUCCESS;
}
static int sort_release(struct inode *inode, struct file *filp) {
	printk(KERN_DEBUG "[SORTLIST]: CLOSING");
	return SUCCESS;
}

static ssize_t sort_read(struct file *filp,  char *buf, size_t count, loff_t *f_pos) { 
  	printk(KERN_DEBUG "[SORTLIST]: RUM READ() FUNCTION\n\n");
	int rv=0;
	int len = 0;
	char msg[MAX_DSIZE];
	if (*f_pos >= SIZE)
		return rv;
	if (down_interruptible (&temp_dev->sem))//star critical region
		return -ERESTARTSYS;		
	if (*f_pos + count > MAX_DSIZE)//verificate whether still have data to read
		count = MAX_DSIZE - *f_pos; 

	printk(KERN_INFO "READ: ");
	struct stlista *list = kmalloc(sizeof(struct stlista),GFP_KERNEL);
	// strcat(msg, "\n");
	// len+=strlen("\n");
	list_for_each_entry(list, &my_list, list_H) {
		//CREATE A CHAR[] AND FILL WITH ONLY VALID CHARACTERS
		//READ UNTIL '\0'
		//ADD SPACE
		int i = 0;
		char c[strlen(list->c)];
		c[i] = list->c[i];
		while(c[i] != '\0'){
			i++;
			c[i] = list->c[i];
		}
		strcat(msg, c);
		strcat(msg, " ");
		len+=strlen(c);
		len+=strlen(" ");
		printk(KERN_INFO "VALUE READ: %s", c);
	}

	strcat(msg, "\n");
	len+=strlen("\n");	
	
	if (copy_to_user (buf, msg, len)) {
		rv = -EFAULT;
		goto wrap_up;
	}

	*f_pos+=len;
	up (&temp_dev->sem); //end of critical region
	return len;
	wrap_up:
	up (&temp_dev->sem);
	return rv;
}

static ssize_t sort_write( struct file *filp, const char *buf,size_t count, loff_t *pos) {  
	printk(KERN_DEBUG "[SORTLIST]: Executou o write");

	unsigned long long res;

	if (down_interruptible (&temp_dev->sem))//star critical region
		return -ERESTARTSYS;
	if(kstrtoull_from_user(buf, count, 10, &res))//READ A VALUE AS A INT 
		return -EINVAL;

	up (&temp_dev->sem);//end of critical region

	struct stlista *list = kmalloc(sizeof(struct stlista),GFP_KERNEL);
	struct stlista *listNEW = kmalloc(sizeof(struct stlista),GFP_KERNEL);

	listNEW->value = (int)res;

	char str[BUFFER_LENGTH];
	sprintf (str, "%d\0", (int)res);
	strcpy(listNEW->c, str);

	int i = 1;

	if (qtdNo == 0){
		INIT_LIST_HEAD(&listNEW->list_H);
		list_add(&listNEW->list_H,&my_list);
	}	
	else{
		list_for_each_entry(list, &my_list, list_H) {
			printk(KERN_INFO "VALOR DE I: %d e VALOR DE QTDNO: %d", i, qtdNo);
			//inserts at the beginning
			if (i == 1)
			{
				if ((int)res <= list->value)
				{
					list_add(&listNEW->list_H,&my_list);
					break;
				}
			}
			//inserts at the end
			else if (i == qtdNo)
			{
				if ((int)res >= list->value)
				{
					list_add_tail(&listNEW->list_H,&my_list);
					break;
				}
				else{
					list_add_tail(&listNEW->list_H,&list->list_H);
					break;
				}
			}
			//insert in the mid
			else if ((int)res <= list->value)
			{
				list_add_tail(&listNEW->list_H,&list->list_H);
				break;
			}
			i++;
		}
		list_for_each_entry(list, &my_list, list_H) {
			printk(KERN_INFO "val = %d\n", list->value);
			printk(KERN_INFO "SIZE = %d\n", strlen(list->c));
		}
	}	
	qtdNo++;
	SIZE+=strlen(listNEW->c);
	SIZE+=strlen(" ");
	return SIZE;
}