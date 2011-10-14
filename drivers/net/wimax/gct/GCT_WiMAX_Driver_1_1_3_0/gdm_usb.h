#ifndef __GDM_USB_H__
#define __GDM_USB_H__

#include <linux/types.h>
#include <linux/usb.h>
#include <linux/list.h>

#define B_DIFF_DL_DRV		(1<<4)
#define B_DOWNLOAD			(1 << 5)
#define MAX_NR_SDU_BUF		64

struct usb_tx {
	struct list_head	list;
#if defined(CONFIG_GDM_USB_PM) || defined(CONFIG_K_MODE)
	struct list_head	p_list;
#endif
	struct tx_cxt		*tx_cxt;

	struct urb	*urb;
	u8			*buf;

	void (*callback)(void *cb_data);
	void *cb_data;
};

struct tx_cxt {
	struct list_head	free_list;
	struct list_head	sdu_list;
	struct list_head	hci_list;
#if defined(CONFIG_GDM_USB_PM) || defined(CONFIG_K_MODE)
	struct list_head	pending_list;
#endif

	spinlock_t			lock;
};

struct usb_rx {
	struct list_head	list;
	struct rx_cxt		*rx_cxt;

	struct urb	*urb;
	u8			*buf;

	void (*callback)(void *cb_data, void *data, int len);
	void *cb_data;
};

struct rx_cxt {
	struct list_head	free_list;
	struct list_head	used_list;
	spinlock_t			lock;
};

struct usbwm_dev {
	struct usb_device	*usbdev;
#ifdef CONFIG_GDM_USB_PM
	struct workqueue_struct	*pm_wq;
	struct work_struct	pm_ws;

	struct usb_interface	*intf;
#endif
#ifdef CONFIG_K_MODE
	int bw_switch;
	struct list_head	list;
#endif

	struct tx_cxt	tx;
	struct rx_cxt	rx;

	int padding;
};

#endif // __GDM_USB_H__
