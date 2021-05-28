#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <net/arp.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/proc_fs.h>

#define PROC_NAME "lab3"

static char* link = "lo"; // имя виртуального интерфейса
module_param(link, charp, 0);

static char* ifname = "lab3";

/*
net_device_stats - структура для накопления статистики 
происходящих в сетевом интерфейсе процессов
*/
static struct net_device_stats stats;

//  net_device - основная структура, описывающая сетевой интерфейс (устройство)
static struct net_device *child = NULL;

/*
есть приватная структура данных, в которых пользователь размещает произвольные данные любой длины, ассоциированные с интерфейсом
netdev_priv(net_device) - функция доступа к этой структуре
*/
struct priv {
    struct net_device *parent;
};


static struct proc_dir_entry* entry;
int passed_packets = 0;
int dropped_packets = 0;


static ssize_t proc_read(struct file *file, char __user * ubuf, size_t count, loff_t* ppos) 
{
  	char sarr[512];
  	int written = 0;
  	size_t len;

    written += snprintf(&sarr[written], 512 - written, "Passed: %d; Dropped: %d\n", passed_packets, dropped_packets);
  	sarr[written] = 0;

    len = strlen(sarr);
  	if (*ppos > 0 || count < len) return 0;
  	if (copy_to_user(ubuf, sarr, len) != 0) return -EFAULT;
  	*ppos = len;
  	return len;
}


static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = proc_read,
};

//проверка пакета на длину
static char check_frame(struct sk_buff *skb) {
    //iphdr - структура для работы с ip-заголовком
    //skb_network_header позволяет получить указатель на эту структуру
    //sk_buff - буфер для работы с пакетами, сюда помещается пакет при принятии/отправке с информацией откуда/куда,для чего.
    struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
    int data_len = 0;
    //ntohs преобазует сетевой порядок расположения байтов положительного короткого целого в узловой порядок
    data_len = ntohs(ip->tot_len) - sizeof(struct iphdr);

   if (data_len > 70) {
     dropped_packets++;
     return 0;
   }
   
   passed_packets++;
   printk(KERN_INFO "-----------");
   //ntohl - как ntohs, но просто положительного целого
   printk(KERN_INFO "Source address: %d.%d.%d.%d\n",
         ntohl(ip->saddr) >> 24, (ntohl(ip->saddr) >> 16) & 0x00FF,
         (ntohl(ip->saddr) >> 8) & 0x0000FF, (ntohl(ip->saddr)) & 0x000000FF);
   printk(KERN_INFO "Destination address: %d.%d.%d.%d\n",
         ntohl(ip->daddr) >> 24, (ntohl(ip->daddr) >> 16) & 0x00FF,
         (ntohl(ip->daddr) >> 8) & 0x0000FF, (ntohl(ip->daddr)) & 0x000000FF);

   printk(KERN_INFO "Size: %d.", data_len);
   printk(KERN_INFO "-----------");
   return 1;
}

/*
установка обработчика входящих пакетов для созданного интерфейса
перехват входящего трафика родительского интерфейса
прием кадра
*/
static rx_handler_result_t handle_frame(struct sk_buff **pskb) {
	if (check_frame(*pskb)) {
        //rx_packets = total packets received
        //rx_bytes = total bytes received
        stats.rx_packets++;
        stats.rx_bytes += (*pskb)->len;
    }
    (*pskb)->dev = child;
    return RX_HANDLER_ANOTHER;
} 

static int open(struct net_device *dev) {
    /*
    разрешает передачу пакетов (start_xmit)
    разрешает верхним уровням вызывать start_xmit
    */
    netif_start_queue(dev);
    printk(KERN_INFO "%s: device opened", dev->name);
    return 0; 
} 

static int stop(struct net_device *dev) {
    //останавливает передачу пакетов
    netif_stop_queue(dev);
    printk(KERN_INFO "%s: device closed", dev->name);
    return 0; 
} 

/*
Передача кадра
Без этого метода модуль выйдет из строя при удалении или использовании.
Всякий раз при попытке отправить пакет происходит утечка памяти.
Операция передачи сокетного буфера в физическую среду. 
Принятые пакеты тут же передаются в очередь (ядра) принимаемых пакетов, 
далее обрабатываются сетевым стеком, потому операция принятия не нужна.
этот метод обязателен из-за, как минимум, вызова API ядра dev_kfree_skb, 
который утилизирует сокетный буфер после операции передачи пакета. 
*/
static netdev_tx_t start_xmit(struct sk_buff *skb, struct net_device *dev) {
    //netdev_priv - приватная структура данных, в которой пользователь может размещать произвольные собственные данные, ассоциированные с интерфейсом
    struct priv *priv = netdev_priv(dev);
    stats.tx_packets++;
    stats.tx_bytes += skb->len;
    if (priv->parent) {
        skb->dev = priv->parent;
        skb->priority = 1;
        //Ставит буфер в очередь для передачи на сетевое устройство
        dev_queue_xmit(skb);
        return 0;
    }
    return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev) {
    return &stats;
} 

// набор обменных операций
static struct net_device_ops crypto_net_device_ops = {
    .ndo_open = open,
    .ndo_stop = stop,
    .ndo_get_stats = get_stats,
    .ndo_start_xmit = start_xmit
};

static void setup(struct net_device *dev) {
    int i;
    //Заполняет поля структуры устройства общими значениями Ethernet
    ether_setup(dev);
    memset(netdev_priv(dev), 0, sizeof(struct priv));
    dev->netdev_ops = &crypto_net_device_ops;

    //fill in the MAC address with a phoney
    for (i = 0; i < ETH_ALEN; i++)
        dev->dev_addr[i] = (char)i;
} 

int __init vni_init(void) {
    int err = 0;
    struct priv *priv;
    //NET_NAME_UNKNOWN - константа, определяющая порядок нумерации создаваемых интерфейсов
    //  = 0, unknown origin, не открыт для пользовательского пространства 
    if (!(entry = proc_create(PROC_NAME, 0444, NULL, &fops))){
        printk(KERN_ERR "%s: failed to create a proc entry %s\n",THIS_MODULE->name,PROC_NAME);
        return -EIO;
    }
    child = alloc_netdev(sizeof(struct priv), ifname, NET_NAME_UNKNOWN, setup);
    if (child == NULL) {
        printk(KERN_ERR "%s: allocate error", THIS_MODULE->name);
        return -ENOMEM;
    }
    priv = netdev_priv(child);
    priv->parent = __dev_get_by_name(&init_net, link); //parent interface
    if (!priv->parent) {
        printk(KERN_ERR "%s: no such net: %s", THIS_MODULE->name, link);
        free_netdev(child);
        return -ENODEV;
    }
    if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
        printk(KERN_ERR "%s: illegal net type", THIS_MODULE->name); 
        free_netdev(child);
        return -EINVAL;
    }

    //copy IP, MAC and other information
    memcpy(child->dev_addr, priv->parent->dev_addr, ETH_ALEN);
    memcpy(child->broadcast, priv->parent->broadcast, ETH_ALEN);
    if ((err = dev_alloc_name(child, child->name))) {
        printk(KERN_ERR "%s: allocate name, error %i", THIS_MODULE->name, err);
        free_netdev(child);
        return -EIO;
    }
    register_netdev(child);
    rtnl_lock();
    //регистрируется обработчик приема
    //вызывающий должен удерживать rntl_mutex
    netdev_rx_handler_register(priv->parent, &handle_frame, NULL);
    rtnl_unlock();
    printk(KERN_INFO "Module %s loaded", THIS_MODULE->name);
    printk(KERN_INFO "%s: create link %s", THIS_MODULE->name, child->name);
    printk(KERN_INFO "%s: registered rx handler for %s", THIS_MODULE->name, priv->parent->name);
    return 0; 
}

void __exit vni_exit(void) {
    struct priv *priv = netdev_priv(child);
    if (priv->parent) {
        rtnl_lock();
        netdev_rx_handler_unregister(priv->parent);
        rtnl_unlock();
        printk(KERN_INFO "%s: unregister rx handler for %s", THIS_MODULE->name, priv->parent->name);
    }
    unregister_netdev(child);
    free_netdev(child);
    proc_remove(entry);
    printk(KERN_INFO "Module %s unloaded", THIS_MODULE->name); 
} 

module_init(vni_init);
module_exit(vni_exit);

MODULE_AUTHOR("Elizaveta Borisenko");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Lab #3");
