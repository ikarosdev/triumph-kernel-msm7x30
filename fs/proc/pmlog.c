/*
 *  linux/fs/proc/pmlog.c
 *
 *  Copyright (C) 2009  by FIHTDC Kenny
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include "internal.h"

extern wait_queue_head_t pmlog_wait;

extern int do_pmlog(int type, char __user *bug, int count);

static int pmlog_open(struct inode * inode, struct file * file)
{
	return do_pmlog(1,NULL,0);
}

static int pmlog_release(struct inode * inode, struct file * file)
{
	(void) do_pmlog(0,NULL,0);
	return 0;
}

static ssize_t pmlog_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	if ((file->f_flags & O_NONBLOCK) && !do_pmlog(9, NULL, 0))
		return -EAGAIN;
	return do_pmlog(2, buf, count);
}

static unsigned int pmlog_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &pmlog_wait, wait);
	if (do_pmlog(9, NULL, 0))
		return POLLIN | POLLRDNORM;
	return 0;
}

const struct file_operations proc_pmlog_operations = {
	.read		= pmlog_read,
	.poll		= pmlog_poll,
	.open		= pmlog_open,
	.release	= pmlog_release,
};

static int __init proc_pmlog_init(void)
{
	proc_create("pmlog", S_IRUSR, NULL, &proc_pmlog_operations);
	return 0;
}
module_init(proc_pmlog_init);


