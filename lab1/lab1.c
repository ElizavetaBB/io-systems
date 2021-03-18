#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>

#define DEVICE_NAME "var5"
#define FIRST_MINOR 0
#define DEV_CNT 1
#define WRITE_BUF_SIZE 100
#define READ_BUF_SIZE 8192

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Borisenko Elizaveta");
MODULE_DESCRIPTION("Lab №1 IO Systems");
MODULE_VERSION("0.1");

static dev_t first;
static struct cdev c_dev;
static struct class *c_dev_class;
static struct proc_dir_entry *entry;
static size_t read_letters;

static ssize_t proc_read(struct file *filp, char __user *ubuf,size_t count, loff_t *offp){
  printk(KERN_INFO "%s:proc_read\n",DEVICE_NAME);
  char buf[WRITE_BUF_SIZE];
	ssize_t len=sprintf(buf,"%zu\n",read_letters);
	if (len<0) return -ENOMEM;
  if (*offp>=len) return 0;
  if (count>len-*offp) count=len-*offp;
  if(copy_to_user(ubuf,buf+*offp,count)) return -EFAULT;
	*off+=len;
	return len;
}

static ssize_t proc_write(struct file *f, const char __user *buf,
  size_t len, loff_t *off){
  printk(KERN_INFO "%s: proc_read\n",DEVICE_NAME);
  return -EINVAL;
}

static int file_open(struct inode *i,struct file *f){
  printk(KERN_INFO "%s: open()\n",DEVICE_NAME);
  return 0;
}

static int file_close(struct inode *i, struct file *f){
  printk(KERN_INFO "%s: close()\n",DEVICE_NAME);
  return 0;
}

static ssize_t file_read(struct file *f, char __user *buf, size_t
  len, loff_t *off){
  printk(KERN_INFO "%s: read()\n",DEVICE_NAME);
  printk(KERN_INFO "The total amount of the read letters:%zu\n",read_letters);
  return 0;
}

static ssize_t file_write(struct file *f, const char __user *ubuf,
  size_t count, loff_t *off){
  printk(KERN_INFO "%s: write()\n",DEVICE_NAME);
  char buf[READ_BUF_SIZE];
  if (count>READ_BUF_SIZE) count=READ_BUF_SIZE;
  if (copy_from_user(buf,ubuf,count)) return -EFAULT;
  for (int i=0;i<READ_BUF_SIZE;i++){
    if ((buf[i]>='A' && buf[i]<='Z')||(buf[i]>='a' && buf[i]<='z')){
        read_letters++;
    }
  }
  *off+=count;
  printk(KERN_INFO "The total amount of the read letters:%zu\n",read_letters);
  return count;
}

static struct file_operations fops={
    .owner=THIS_MODULE,
    .open=file_open,
    .release=file_close,
    .read=file_read,
    .write=file_write,
};

static struct file_operations proc_fops={
  .owner=THIS_MODULE,
  .read=proc_read,
  .write=proc_write,
};

static int __init lab1_init(void){
    read_letters=0;
    if((entry=proc_create(DEVICE_NAME,0444,NULL,&proc_fops))==NULL){
      printk(KERN_INFO "Failed the proc_dir_entry creation\n");
      return -1;
    }
    printk(KERF_INFO "Trying to initialize\n");
    if (alloc_chrdev_region(&first,FIRST_MINOR,1,DRIVER_NAME)<0){//1-cnt
      proc_remove(entry);
      printk(KERN_INFO "Failed the registration of device files\n");
      return -1;
    }
    if ((c_dev_class=class_create(THIS_MODULE,DRIVER_NAME))==NULL){
      unregister_chardev_region(first,1);//1-cnt
      proc_remove(entry);
      printk(KERN_INFO "Failed the class creation\n");
      return -1;
    }
    if (device_create(c_dev_class,NULL,first,NULL,DRIVER_NAME)==NULL){
      class_destroy(c_dev_class);
      unregister_chardev_region(first,1);
      proc_remove(entry);
      printk(KERN_INFO "Failed the device creation\n");
      return -1;
    }
    cdev_init(&c_dev,&fops);
    if (cdev_add(&c_dev,first,1)<0){
    device_destroy(c_dev,first);
    class_destroy(c_dev_class);
    unregister_chardev_region(first,1);
    proc_remove(entry);
    printk(KERN_INFO "Failed the creation of files\n");
    return -1;
  }
  printk(KERN_INFO "Initialized!\n");
  return 0;
}

static void __exit lab1_exit(void){
  cdev_del(&c_dev);
  device_destroy(c_dev,first);
  class_destroy(c_dev);
  unregister_chrdev_region(first,DEV_CNT);
  proc_remove(entry);
  printk(KERN_INFO "Goodbye\n");
}

module_init(lab1_init);
module_exit(lab1_exit);