/*
 *  linux/fs/proc/flog.c
 *
 *  Copyright (C) 1992  by Linus Torvalds
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/io.h>

extern wait_queue_head_t log_ftm_wait;
extern int do_ftmlog(int type, char __user *bug, int count);

static int flog_open(struct inode * inode, struct file * file)
{
	return do_ftmlog(1,NULL,0);
}

static int flog_release(struct inode * inode, struct file * file)
{
	(void) do_ftmlog(0,NULL,0);
	return 0;
}

static ssize_t flog_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	if ((file->f_flags & O_NONBLOCK) && !do_ftmlog(9, NULL, 0))
		return -EAGAIN;
	return do_ftmlog(2, buf, count);
}

static unsigned int flog_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &log_ftm_wait, wait);
	if (do_ftmlog(9, NULL, 0))
		return POLLIN | POLLRDNORM;
	return 0;
}


static const struct file_operations proc_flog_operations = {
	.read		= flog_read,
	.poll		= flog_poll,
	.open		= flog_open,
	.release	= flog_release,
};

static int __init proc_flog_init(void)
{
	proc_create("flog", S_IRUSR, NULL, &proc_flog_operations);
	return 0;
}
module_init(proc_flog_init);
