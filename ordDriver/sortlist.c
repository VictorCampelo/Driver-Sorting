/*----------------------------------------------------------------------------*/
/* File: tem.c                                                             */
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
//#include <asm/uaccess.h> /* copy_from/to_user */
#include <linux/sort.h> /* sort */
#include <linux/security.h>

#include <linux/uaccess.h>

#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
//Macros utilizadas para especificar informações relacionadas ao modulo
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joao Victor Campelo do Vale");
MODULE_DESCRIPTION("A module to ordenate a list of number");

struct task_struct;

#define SUCCESS 0
#define DEVICE_NAME "sortlist"
#define BUFFER_LENGTH 10
#define MAX_DSIZE	3071


/* GLOBAL */
static int msg[BUFFER_LENGTH];
static int *msg_p;
int memory_major = 0;
int bytes_operated = 0;
/* Valor major igual a zero quando for alocar um major dinamicamente */
int major=0;
//Especifica parâmetros do módulo. variável, tipo e permissões usadas no sysfs
//S_IRUGO - Permissão de leitura para todos os usuários
module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "device major number");

struct my_dev {
        char data[MAX_DSIZE+1];
        size_t size;              
        struct semaphore sem;     /* Para implementar exclusão mútua */
        struct cdev cdev;
	struct class *class;
	struct device *device;
} *temp_dev;

//----------------------------------------------------------------------------------------------
//LINKED LIST
struct stlista {
        int value;
        struct list_head list_H;
};
//INITIALIZATION
LIST_HEAD(my_list);

//----------------------------------------------------------------------------------------------

/* Declaração das funções SORTLIST.c  */
static int temp_open(struct inode *inode, struct file *filp);
static int temp_release(struct inode *inode, struct file *filp);
static ssize_t temp_read(struct file *filp,  char *buf, size_t count, loff_t *f_pos);
static ssize_t temp_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int temp_init(void);
void temp_exit(void);

static int compare(const void *a, const void *b); /* Funcao para comparar no sort */

/* Declaração das funções init and exit */
module_init(temp_init);
module_exit(temp_exit);

static struct file_operations temp_fops = {
  .owner = THIS_MODULE,
  .read = temp_read,
  .write = temp_write,
  .open = temp_open,
  .release = temp_release
};

int temp_init(void) {
	int rv;
	//Macro para converter major e minor number do tipo dev_t
	dev_t devno = MKDEV(major, 0);

	if(major) {
		//Registra o device com major devno 
		//e 1 é a quantidade de devices, 
		//sortlist é o nome do device
		rv = register_chrdev_region(devno, 1, DEVICE_NAME);
		if(rv < 0){
			printk(KERN_WARNING "Can't use the major number %d; try atomatic allocation...\n", major);
			rv = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
			major = MAJOR(devno);
		}
	}
	else {
        //Aloca um major dinamicamente para o device e registra-o. 
		//0 - minor number 1 - numero de devices
		rv = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
		major = MAJOR(devno);
	}

	if(rv < 0) return rv;
    //Alocação de memória fisica para colocar a estrutura do device. 
	//GFP_KERNEL bloqueia o processo se a memória 
	//não está imediatamente disponível
	temp_dev = kmalloc(sizeof(struct my_dev), GFP_KERNEL);
	if(temp_dev == NULL){
		rv = -ENOMEM;
		unregister_chrdev_region(devno, 1);
		return rv;
	}
    //Inicializa a memória alocada com kmalloc
	memset(temp_dev, 0, sizeof(struct my_dev));
	//cdev - Estrutura para dispositivos de caractere
	//Inicializa a estrutura cdev do dispositivo
	//temp_fops - especifica as funções que implementam as
	//operações sobre o arquivo de device a serem registradas no VFS
	cdev_init(&temp_dev->cdev, &temp_fops);
	temp_dev->cdev.owner = THIS_MODULE;
	temp_dev->size = MAX_DSIZE;
	sema_init (&temp_dev->sem, 1);
	//Entrega a estrutura cdev ao VFS
	rv = cdev_add (&temp_dev->cdev, devno, 1);
	if (rv) printk(KERN_WARNING "Error %d adding device sortlist", rv);
    //UDEV- Deamon responsável por criar o arquivo de device 
	//a partir das informações fornecidas
	
	//Cria uma classe para uso em device_create
	temp_dev->class = class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(temp_dev->class)) {
		cdev_del(&temp_dev->cdev);
		unregister_chrdev_region(devno, 1);
		printk(KERN_WARNING "%s: can't create udev class\n", DEVICE_NAME);
		rv = -ENOMEM;
		return rv;
	}
    //Cria arquivo de device em /dev automaticamente
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

void temp_exit(void) {
	device_destroy(temp_dev->class, MKDEV(major, 0));
	class_destroy(temp_dev->class);
	cdev_del(&temp_dev->cdev); 
	kfree(temp_dev);
	unregister_chrdev_region(MKDEV(major, 0), 1);
	printk(KERN_WARNING "Good bye from Template Module\n");
}

static int temp_open(struct inode *inode, struct file *filp) {
  printk(KERN_DEBUG "[SORTLIST]: Aberto");
  msg_p = msg;
    return SUCCESS;
}
static int temp_release(struct inode *inode, struct file *filp) {
  printk(KERN_DEBUG "[SORTLIST]: Fechado");
    return SUCCESS;
}

static ssize_t temp_read(struct file *filp,  char *buf, size_t count, loff_t *f_pos) { 
  //printk(KERN_DEBUG "[SORTLIST]: Executou o read %n\n\n", msg_p);

  if(*msg_p == '\0') return 0;
  bytes_operated = 0;
  while(count && *msg_p){
    // Escrever um valor no espaco do usuario
	struct stlista *list;
	list = kmalloc(sizeof(struct stlista *),GFP_KERNEL);

	list_for_each_entry(list, &my_list, list_H) {
		put_user(list->value, buf++);
    	count--;
	    bytes_operated++;
		printk(KERN_INFO "READ = %d\n", list->value);
	}
  }
  
  return bytes_operated;
}

static ssize_t temp_write( struct file *filp, const char *buf,size_t count, loff_t *posicao) {  
  printk(KERN_DEBUG "[SORTLIST]: Executou o write");

  bytes_operated = 0;
  
  memset(msg_p, 0, sizeof(char)* BUFFER_LENGTH);
  if(copy_from_user(msg_p, buf, count)){
    return -EINVAL;
  }else{
    //sort(msg_p, count, sizeof(char), &compare, NULL);

	struct stlista *list;
	list = kmalloc(sizeof(struct stlista *),GFP_KERNEL);

	list->value = *msg_p;

	list_add(&list->list_H,&my_list);

	list_for_each_entry(list, &my_list, list_H) {
		printk(KERN_INFO "val = %d\n", list->value);
	}
    
    //printk(KERN_DEBUG "[SORTLIST]: ordenacao,  msg: %s\n", msg_p); 
    return count;
  }
}

static int compare(const void *a, const void *b){
  char ca = *(const char*)(a);
  char cb = *(const char*)(b);
  if(ca < cb) return -1;
  if(ca > cb) return 1;
  return 0;
}