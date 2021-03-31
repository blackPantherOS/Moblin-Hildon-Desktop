/* sample "Hello world" kernel module source
 * Written by alek.du@intel.com
 * This file is released under the GPLv2
 *
 * This is a very simple kernel module, only shows how to add a driver to kernel.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

#define MAJOR_NR 121
#define DEVICE_NAME "Hello World Char Device"

char content='0';

static ssize_t hello_read(struct file *filp,char *buf, size_t count,loff_t *f_pos)
{
    if (count>0){
       return copy_to_user(buf,&content,1);
    }
    return 0;
}
static ssize_t hello_write(struct file *filp,const char *buf,size_t count,loff_t *f_pos)
{
    if (count>0){
       return copy_from_user(&content,buf,1);
    }
    return 0;
}
static int hello_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int hello_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations hello_fops= {
    read:hello_read,
    write:hello_write,
    open: hello_open,
    release:hello_release,
};

int helloworld_init(void)
{
    int result;

    /* register a char dev */
    result=register_chrdev(MAJOR_NR,DEVICE_NAME,&hello_fops);
    if (result<0){
       printk(KERN_ERR DEVICE_NAME":get major %d wrong\n",MAJOR_NR);
       return(result);
    }
    printk("Hello world char driver initialized!\n");
    // A non 0 return means init_module failed; module can't be loaded.
    return 0;
}

void helloworld_cleanup(void)
{
    printk("Hello world char driver de-initialized!\n");
}  

module_init(helloworld_init);
module_exit(helloworld_cleanup);
MODULE_AUTHOR("alek.du"); 
MODULE_DESCRIPTION("Hello World Char Device Driver");
MODULE_LICENSE("GPL");
