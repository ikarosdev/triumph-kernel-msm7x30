#if !defined(NETLINK_H_20081202)
#define NETLINK_H_20081202
#include <linux/netdevice.h>
#include <net/sock.h>

struct sock * netlink_init(int unit,
				void (*cb)(struct net_device *dev, u16 type, void *msg, int len));
void netlink_exit(struct sock *sock);
int netlink_send(struct sock *sock, int group, u16 type, void *msg, int len);

#endif
