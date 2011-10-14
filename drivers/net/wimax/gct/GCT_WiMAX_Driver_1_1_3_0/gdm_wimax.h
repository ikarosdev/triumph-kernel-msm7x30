#ifndef __GDM_WIMAX_H__
#define __GDM_WIMAX_H__

#include <linux/netdevice.h>
#include <linux/version.h>
#include <linux/types.h>
#include "wm_ioctl.h"
#if defined(CONFIG_GDM_QOS)
#include "gdm_qos.h"
#endif

#define DRIVER_VERSION		"3.2.0"

//#define ETH_P_IP	0x0800
//#define ETH_P_ARP	0x0806
//#define ETH_P_IPV6	0x86DD

#define H2L(x)		__cpu_to_le16(x)
#define L2H(x)		__le16_to_cpu(x)
#define DH2L(x)		__cpu_to_le32(x)
#define DL2H(x)		__le32_to_cpu(x)

#define H2B(x)		__cpu_to_be16(x)
#define B2H(x)		__be16_to_cpu(x)
#define DH2B(x)		__cpu_to_be32(x)
#define DB2H(x)		__be32_to_cpu(x)

#ifndef HAVE_NETDEV_PRIV
#define netdev_priv(dev)	(dev)->priv
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif

struct phy_dev {
	void	*priv_dev;
	struct net_device	*netdev;

	int	(*send_func)(void *priv_dev, void *data, int len,
			void (*cb)(void *cb_data), void *cb_data);
	int	(*rcv_func)(void *priv_dev, void (*cb)(void *cb_data, void *data, int len),
			void *cb_data);
};

struct nic {
	struct net_device	*netdev;
	struct phy_dev		*phy_dev;

	struct net_device_stats	stats;

	data_t	sdk_data[SIOC_DATA_MAX];

#if defined(CONFIG_GDM_QOS)
	qos_cb_t	qos;
#endif

};


#if 0
#define dprintk(fmt, args ...)	printk(" [GDM] " fmt, ## args)
#else
#define dprintk(...)
#endif

//#define DEBUG_SDU
#if defined(DEBUG_SDU)
#define DUMP_SDU_ALL		(1<<0)
#define DUMP_SDU_ARP		(1<<1)
#define DUMP_SDU_IP			(1<<2)
#define DUMP_SDU_IP_TCP		(1<<8)
#define DUMP_SDU_IP_UDP		(1<<9)
#define DUMP_SDU_IP_ICMP	(1<<10)
#define DUMP_PACKET			(DUMP_SDU_ALL)
#endif

//#define DEBUG_HCI

extern int register_wimax_device(struct phy_dev *phy_dev);
extern int gdm_wimax_send_tx(struct sk_buff *skb, struct net_device *dev);
extern void unregister_wimax_device(struct phy_dev *phy_dev);

#endif
