/*----------------------------------------------------------------------------*/
/* File: tem.c                                                             */
/* Date: 13/03/2006                                                           */
/* Author: Zhiyi Huang                                                       */
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
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/aio.h>
#include <linux/uaccess.h>

#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
//Macros utilizadas para especificar informações relacionadas ao modulo
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhiyi Huang");
MODULE_DESCRIPTION("A template module");


/* Valor major igual a zero quando for alocar um major dinamicamente */
int major=0;
//Especifica parâmetros do módulo. variável, tipo e permissões usadas no sysfs
//S_IRUGO - Permissão de leitura para todos os usuários
module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "device major number");

#define MAX_DSIZE	3071
struct my_dev {
        char data[MAX_DSIZE+1];
        size_t size;              
        struct semaphore sem;     /* Para implementar exclusão mútua */
        struct cdev cdev;
	struct class *class;
	struct device *device;
} *temp_dev;
//Definição de um nó da lista ligada
struct stlista {
        char data;
        struct list_head milista;
};
//Definicao da lista duplamente ligada
LIST_HEAD(listaDupla) ;

int temp_open (struct inode *inode, struct file *filp)
{
	return 0;
}

int temp_release (struct inode *inode, struct file *filp)
{
	return 0;
}
// função para leitura do arquivo de device
//filp - referencia para o arquivo de device
//buf - area no espaço do usuário onde o dado lido será colocado
//count - quantidade de dados lidos
//f_pos - posição atual de leitura
ssize_t temp_read (struct file *filp, char __user *buf, size_t count,loff_t *f_pos)
{
	int rv=0;
        struct stlista *ptr=NULL;
        printk("aqui\n");
        list_for_each_entry(ptr,&listaDupla,milista){
                printk("dado=%c\n",ptr->data);
        }
	printk(KERN_WARNING " f_pos: %lld; count: %zu;\n", *f_pos, count);
	if (down_interruptible (&temp_dev->sem))//Inicio da regiao critica
		return -ERESTARTSYS;
	if (*f_pos > MAX_DSIZE)//verifica se já leu tudo do buffer do device  
		goto wrap_up;     
	if (*f_pos + count > MAX_DSIZE)//Verifica se o que foi solicitado(count) mais a posição atual(*f_pos) ultrapassa a capacidade do driver(MAX_DSIZE).
		count = MAX_DSIZE - *f_pos;//Ajusta count para o que tem disponivel
    //Função para copiar dados do espaço do kernel(temp_dev->data) para
	//o espaço do usuário(buf). Count  é a quantidade de dados a ser copiado.
	if (copy_to_user (buf, temp_dev->data+*f_pos, count)) {
		rv = -EFAULT;
		goto wrap_up;
	}
	up (&temp_dev->sem);
	*f_pos += count;//Avança a posição de leitura para a próxima vez
	return count;
wrap_up:
	up (&temp_dev->sem);
	return rv;
}

ssize_t temp_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int count1=count,rv=count;
        struct stlista *no;
	no=kmalloc(sizeof(struct stlista),GFP_KERNEL);
	no->data=*buf;
        INIT_LIST_HEAD(&no->milista);
	list_add_tail(&no->milista,&listaDupla);
	if (down_interruptible (&temp_dev->sem))
		return -ERESTARTSYS;
	if (*f_pos > MAX_DSIZE)
		goto wrap_up;
	if (*f_pos + count > MAX_DSIZE)
		count1 = MAX_DSIZE - *f_pos;
	if (copy_from_user (temp_dev->data+*f_pos, buf, count)) {
		rv = -EFAULT;
		goto wrap_up;
	}
	up (&temp_dev->sem);
	*f_pos += count1;
	return count;
wrap_up:
	up (&temp_dev->sem);
	return rv;
}

struct file_operations temp_fops = {
        .owner =     THIS_MODULE,
        .read =      temp_read,
        .write =     temp_write,
        .open =      temp_open,
        .release =   temp_release,
};


/**
 * Inicializa o módulo e cria o dispositivo
 */
int __init temp_init_module(void){
	int rv;
	//Macro para converter major e minor number do tipo dev_t
	dev_t devno = MKDEV(major, 0);

	if(major) {
		//Registra o device com major devno 
		//e 1 é a quantidade de devices, 
		//temp é o nome do device
		rv = register_chrdev_region(devno, 1, "temp");
		if(rv < 0){
			printk(KERN_WARNING "Can't use the major number %d; try atomatic allocation...\n", major);
			rv = alloc_chrdev_region(&devno, 0, 1, "temp");
			major = MAJOR(devno);
		}
	}
	else {
        //Aloca um major dinamicamente para o device e registra-o. 
		//0 - minor number 1 - numero de devices
		rv = alloc_chrdev_region(&devno, 0, 1, "temp");
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
	if (rv) printk(KERN_WARNING "Error %d adding device temp", rv);
    //UDEV- Deamon responsável por criar o arquivo de device 
	//a partir das informações fornecidas
	
	//Cria uma classe para uso em device_create
	temp_dev->class = class_create(THIS_MODULE, "temp");
	if(IS_ERR(temp_dev->class)) {
		cdev_del(&temp_dev->cdev);
		unregister_chrdev_region(devno, 1);
		printk(KERN_WARNING "%s: can't create udev class\n", "temp");
		rv = -ENOMEM;
		return rv;
	}
    //Cria arquivo de device em /dev automaticamente
	temp_dev->device = device_create(temp_dev->class, NULL,
					MKDEV(major, 0), "%s", "temp");
	if(IS_ERR(temp_dev->device)){
		class_destroy(temp_dev->class);
		cdev_del(&temp_dev->cdev);
		unregister_chrdev_region(devno, 1);
		printk(KERN_WARNING "%s: can't create udev device\n", "temp");
		rv = -ENOMEM;
		return rv;
	}

	printk(KERN_WARNING "Hello world from Template Module\n");
	printk(KERN_WARNING "temp device MAJOR is %d, dev addr: %lx\n", major, (unsigned long)temp_dev);

  return 0;
}


/**
 * Finaliza o módulo. Desfaz tudo que foi feito na inicalização.
 */
void __exit temp_exit_module(void){
	device_destroy(temp_dev->class, MKDEV(major, 0));
	class_destroy(temp_dev->class);
	cdev_del(&temp_dev->cdev); 
	kfree(temp_dev);
	unregister_chrdev_region(MKDEV(major, 0), 1);
	printk(KERN_WARNING "Good bye from Template Module\n");
}

//Funções de callback chamadas na inicialização e no desligamento do módulo.
module_init(temp_init_module);
module_exit(temp_exit_module);

