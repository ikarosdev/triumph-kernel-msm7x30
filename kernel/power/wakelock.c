/* kernel/power/wakelock.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/suspend.h>
#include <linux/syscalls.h> /* sys_sync */
#include <linux/wakelock.h>
#ifdef CONFIG_WAKELOCK_STAT
#include <linux/proc_fs.h>
#endif
#include "power.h"

enum {
	DEBUG_EXIT_SUSPEND = 1U << 0,
	DEBUG_WAKEUP = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_EXPIRE = 1U << 3,
	DEBUG_WAKE_LOCK = 1U << 4,
	DEBUG_POLLING_DUMP_WAKELOCK = 1U << 5,	//Div251-PK-Dump_Wakelock-00+
};

//Div251-PK-Dump_Wakelock-00+[
#ifdef CONFIG_FIH_DUMP_WAKELOCK
  static int debug_mask = DEBUG_EXIT_SUSPEND | DEBUG_WAKEUP | DEBUG_SUSPEND | DEBUG_POLLING_DUMP_WAKELOCK;
#else
  static int debug_mask = DEBUG_EXIT_SUSPEND | DEBUG_WAKEUP;
#endif /* CONFIG_FIH_DUMP_WAKELOCK */
//Div251-PK-Dump_Wakelock-00+]

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define WAKE_LOCK_TYPE_MASK              (0x0f)
#define WAKE_LOCK_INITIALIZED            (1U << 8)
#define WAKE_LOCK_ACTIVE                 (1U << 9)
#define WAKE_LOCK_AUTO_EXPIRE            (1U << 10)
#define WAKE_LOCK_PREVENTING_SUSPEND     (1U << 11)

#define POLLING_DUMP_WAKELOCK_SECS	(90)	//Div251-PK-Dump_Wakelock-00+

static DEFINE_SPINLOCK(list_lock);
static LIST_HEAD(inactive_locks);
static struct list_head active_wake_locks[WAKE_LOCK_TYPE_COUNT];
static int current_event_num;
struct workqueue_struct *suspend_work_queue;
struct wake_lock main_wake_lock;
suspend_state_t requested_suspend_state = PM_SUSPEND_MEM;
static struct wake_lock unknown_wakeup;

#ifdef CONFIG_WAKELOCK_STAT
static struct wake_lock deleted_wake_locks;
static ktime_t last_sleep_time_update;
static int wait_for_wakeup;

//Div2-SW2-BSP-pmlog, HenryMCWang +
#include "linux/pmlog.h"

#ifdef __FIH_PM_STATISTICS__
struct pms g_pms_run;
struct pms g_pms_bkrun;
struct hpms g_pms_suspend;

int g_secupdatereq;

#ifdef __FIH_DBG_PM_STATISTICS__
struct hpms g_pms_resume;
int g_timelose;
#endif		// __FIH_DBG_PM_STATISTICS__

static int pms_read_proc(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{
	//unsigned long irqflags;
	int len = 0;
	char *p = page;

	//spin_lock_irqsave(&list_lock, irqflags);

	p += sprintf(p, "   name   -    time    -    count\n");
	p += sprintf(p, "\"run    \" - %10d - %10d\n", (int)g_pms_run.time, (int)g_pms_run.cnt);
	p += sprintf(p, "\"bkrun  \" - %10d - %10d\n", (int)(g_pms_bkrun.time - g_pms_suspend.time), (int)g_pms_bkrun.cnt);
#ifdef __FIH_DBG_PM_STATISTICS__
	p += sprintf(p, "\"suspend\" - %10d - %10d - %d\n", (int)g_pms_suspend.time, (int)g_pms_suspend.cnt, (int)g_timelose);
	p += sprintf(p, "\"resume \" - %10d\n", (int)g_pms_resume.time);
#else		// __FIH_DBG_PM_STATISTICS__
	p += sprintf(p, "\"suspend\" - %10d - %10d\n", (int)g_pms_suspend.time, (int)g_pms_suspend.cnt);
#endif		// __FIH_DBG_PM_STATISTICS__

	//spin_unlock_irqrestore(&list_lock, irqflags);

	*start = page + off;

	len = p - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}
#endif	// __FIH_PM_STATISTICS__
//Div2-SW2-BSP-pmlog, HenryMCWang -

//Div2-SW2-BSP-pmlog, HenryMCWang +
#ifdef __FIH_PM_LOG__

#define PMLOG_LENGTH	(1 << CONFIG_PMLOG_SHIFT)
#define CONFIG_PMLOG_SHIFT	14

static char pmlog_buf[PMLOG_LENGTH];
static unsigned pmlog_start = 0, pmlog_end = 0;
static int pmlog_len = PMLOG_LENGTH;
static DEFINE_SPINLOCK(pmlog_lock);
DECLARE_WAIT_QUEUE_HEAD(pmlog_wait);

#define PMLOG_BUF_MASK (pmlog_len-1)
#define PMLOG_BUF(idx) (pmlog_buf[(idx) & PMLOG_BUF_MASK])


/*
static int getlog(char * p, int count)
{
	char *trg;
	int printed_chars = 0;
	trg = p;
	spin_lock(&pmlog_lock);
	while( (pmlog_start != pmlog_end) && (count != 0)){
	    *trg++ = PMLOG_BUF(pmlog_start);
	    pmlog_start++;
	    printed_chars++;
	    count--;
	}
    spin_unlock(&pmlog_lock);
	return printed_chars;
}
*/

static void put_char(char c)
{
	PMLOG_BUF(pmlog_end) = c;

	pmlog_end++;
	if (pmlog_end - pmlog_start > pmlog_len)
		pmlog_start = pmlog_end - pmlog_len;
}

static volatile unsigned int pmlog_cpu = UINT_MAX;
static int new_text_line = 1;
static int pmlog_time = 1;
static char pmlog_tbuf[512];
extern int __read_mostly timekeeping_suspended;

static int vpmlog(const char *fmt, va_list args)
{
	int printed_len = 0;
	char *p;
	struct timespec ts;
	struct rtc_time tm;

	spin_lock(&pmlog_lock);

    pmlog_cpu = smp_processor_id();
    
	/* Emit the output into the temporary buffer */
	printed_len += vscnprintf(pmlog_tbuf, sizeof(pmlog_tbuf), fmt, args);
    
    
	/* Copy the output into pmlog_buf. */
	for (p = pmlog_tbuf; *p; p++) {
		if (new_text_line) {

			new_text_line = 0;
			if (pmlog_time)
			{
				/* Follow the token with the time */
				char tbuf[50], *tp;
				unsigned tlen;
				if (timekeeping_suspended == 0)
				{
					/*
					unsigned long long t;
					unsigned long nanosec_rem;
					
					t = cpu_clock(pmlog_cpu);
					nanosec_rem = do_div(t, 1000000000);
					tlen = sprintf(tbuf, "[%5lu] ", (unsigned long) t);
					*/
					getnstimeofday(&ts);
					rtc_time_to_tm(ts.tv_sec, &tm);
					tlen = sprintf(tbuf, "[%02d/%02d %02d:%02d:%02d] ", 
					tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				}
				else
				{
					/*
					unsigned long long t;
					unsigned long nanosec_rem;
					
					t = cpu_clock(pmlog_cpu);
					nanosec_rem = do_div(t, 1000000000);
					tlen = sprintf(tbuf, "[%5lu] ", (unsigned long) t);
					*/
					tlen = sprintf(tbuf, "[xx/xx xx:xx:xx] ");
				}
				for (tp = tbuf; tp < tbuf + tlen; tp++)
					put_char(*tp);
				printed_len += tlen;
			}
			if (!*p)
				break;
		}

		put_char(*p);
		if (*p == '\n')
			new_text_line = 1;
	}

	spin_unlock(&pmlog_lock);
	
	if (waitqueue_active(&pmlog_wait))
		wake_up_interruptible(&pmlog_wait);
	
	return printed_len;
}

int pmlog(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vpmlog(fmt, args);
	va_end(args);

	return r;
}

/*
static int pmlog_read_proc(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{
	char * p = page;
	int len = 0;
	int offset;
	
	offset = off%(PAGE_SIZE);
    len = getlog( p+offset, count);
    
    *start = page + offset;
    if(len == 0)
        *eof = 1;
    else
        *eof = 0;
	return len;
}
*/

/*
 * Commands to do_pmlog:
 *
 * 	0 -- Close the log.  Currently a NOP.
 * 	1 -- Open the log. Currently a NOP.
 * 	2 -- Read from the log.
 *	9 -- Return number of unread characters in the log buffer
 */
int do_pmlog(int type, char __user *buf, int len)
{
	unsigned i;
	char c;
	int error = 0;

	switch (type) {
	case 0:		/* Close log */
		break;
	case 1:		/* Open log */
		break;
	case 2:		/* Read from log */
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}
		error = wait_event_interruptible(pmlog_wait,
							(pmlog_start - pmlog_end));
		if (error)
			goto out;
		i = 0;
		spin_lock_irq(&pmlog_lock);
		while (!error && (pmlog_start != pmlog_end) && i < len) {
			c = PMLOG_BUF(pmlog_start);
			pmlog_start++;
			spin_unlock_irq(&pmlog_lock);
			error = __put_user(c,buf);
			buf++;
			i++;
			cond_resched();
			spin_lock_irq(&pmlog_lock);
		}
		spin_unlock_irq(&pmlog_lock);
		if (!error)
			error = i;
		break;
	case 9:		/* Number of chars in the log buffer */
		error = pmlog_end - pmlog_start;
		break;
	default:
		error = -EINVAL;
		break;
	}
out:
	return error;
}

#endif	// __FIH_PM_LOG__
//Div2-SW2-BSP-pmlog, HenryMCWang -

int get_expired_time(struct wake_lock *lock, ktime_t *expire_time)
{
	struct timespec ts;
	struct timespec kt;
	struct timespec tomono;
	struct timespec delta;
	unsigned long seq;
	long timeout;

	if (!(lock->flags & WAKE_LOCK_AUTO_EXPIRE))
		return 0;
	do {
		seq = read_seqbegin(&xtime_lock);
		timeout = lock->expires - jiffies;
		if (timeout > 0)
			return 0;
		kt = current_kernel_time();
		tomono = wall_to_monotonic;
	} while (read_seqretry(&xtime_lock, seq));
	jiffies_to_timespec(-timeout, &delta);
	set_normalized_timespec(&ts, kt.tv_sec + tomono.tv_sec - delta.tv_sec,
				kt.tv_nsec + tomono.tv_nsec - delta.tv_nsec);
	*expire_time = timespec_to_ktime(ts);
	return 1;
}


static int print_lock_stat(struct seq_file *m, struct wake_lock *lock)
{
	int lock_count = lock->stat.count;
	int expire_count = lock->stat.expire_count;
	ktime_t active_time = ktime_set(0, 0);
	ktime_t total_time = lock->stat.total_time;
	ktime_t max_time = lock->stat.max_time;

	ktime_t prevent_suspend_time = lock->stat.prevent_suspend_time;
	if (lock->flags & WAKE_LOCK_ACTIVE) {
		ktime_t now, add_time;
		int expired = get_expired_time(lock, &now);
		if (!expired)
			now = ktime_get();
		add_time = ktime_sub(now, lock->stat.last_time);
		lock_count++;
		if (!expired)
			active_time = add_time;
		else
			expire_count++;
		total_time = ktime_add(total_time, add_time);
		if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND)
			prevent_suspend_time = ktime_add(prevent_suspend_time,
					ktime_sub(now, last_sleep_time_update));
		if (add_time.tv64 > max_time.tv64)
			max_time = add_time;
	}

	return seq_printf(m,
		     "\"%s\"\t%d\t%d\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\n",
		     lock->name, lock_count, expire_count,
		     lock->stat.wakeup_count, ktime_to_ns(active_time),
		     ktime_to_ns(total_time),
		     ktime_to_ns(prevent_suspend_time), ktime_to_ns(max_time),
		     ktime_to_ns(lock->stat.last_time));
}

static int wakelock_stats_show(struct seq_file *m, void *unused)
{
	unsigned long irqflags;
	struct wake_lock *lock;
	int ret;
	int type;

	spin_lock_irqsave(&list_lock, irqflags);

	ret = seq_puts(m, "name\tcount\texpire_count\twake_count\tactive_since"
			"\ttotal_time\tsleep_time\tmax_time\tlast_change\n");
	//Div2-SW2-BSP-pmlog, HenryMCWang +
	ret += seq_puts(m, "<< *inactive_locks >> :\n");
	
	//Div2-SW2-BSP-pmlog, HenryMCWang -
	list_for_each_entry(lock, &inactive_locks, link)
		ret = print_lock_stat(m, lock);
	for (type = 0; type < WAKE_LOCK_TYPE_COUNT; type++) {
		//Div2-SW2-BSP-pmlog, HenryMCWang +
		if(type == 0)
		{
			ret += seq_puts(m, "<< *active_wake_locks[WAKE_LOCK_SUSPEND] >> :\n");
		}
		else
		{
			ret += seq_puts(m, "<< *active_wake_locks[WAKE_LOCK_IDLE] >> :\n");
		}
		//Div2-SW2-BSP-pmlog, HenryMCWang -
		list_for_each_entry(lock, &active_wake_locks[type], link)
			ret = print_lock_stat(m, lock);
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
	return 0;
}

static void wake_unlock_stat_locked(struct wake_lock *lock, int expired)
{
	ktime_t duration;
	ktime_t now;
	if (!(lock->flags & WAKE_LOCK_ACTIVE))
		return;
	if (get_expired_time(lock, &now))
		expired = 1;
	else
		now = ktime_get();
	lock->stat.count++;
	if (expired)
		lock->stat.expire_count++;
	duration = ktime_sub(now, lock->stat.last_time);
	lock->stat.total_time = ktime_add(lock->stat.total_time, duration);
	if (ktime_to_ns(duration) > ktime_to_ns(lock->stat.max_time))
		lock->stat.max_time = duration;
	lock->stat.last_time = ktime_get();
	if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND) {
		duration = ktime_sub(now, last_sleep_time_update);
		lock->stat.prevent_suspend_time = ktime_add(
			lock->stat.prevent_suspend_time, duration);
		lock->flags &= ~WAKE_LOCK_PREVENTING_SUSPEND;
	}
}

static void update_sleep_wait_stats_locked(int done)
{
	struct wake_lock *lock;
	ktime_t now, etime, elapsed, add;
	int expired;

	now = ktime_get();
	elapsed = ktime_sub(now, last_sleep_time_update);
	list_for_each_entry(lock, &active_wake_locks[WAKE_LOCK_SUSPEND], link) {
		expired = get_expired_time(lock, &etime);
		if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND) {
			if (expired)
				add = ktime_sub(etime, last_sleep_time_update);
			else
				add = elapsed;
			lock->stat.prevent_suspend_time = ktime_add(
				lock->stat.prevent_suspend_time, add);
		}
		if (done || expired)
			lock->flags &= ~WAKE_LOCK_PREVENTING_SUSPEND;
		else
			lock->flags |= WAKE_LOCK_PREVENTING_SUSPEND;
	}
	last_sleep_time_update = now;
}
#endif


static void expire_wake_lock(struct wake_lock *lock)
{
#ifdef CONFIG_WAKELOCK_STAT
	wake_unlock_stat_locked(lock, 1);
#endif
	lock->flags &= ~(WAKE_LOCK_ACTIVE | WAKE_LOCK_AUTO_EXPIRE);
	list_del(&lock->link);
	list_add(&lock->link, &inactive_locks);
	if (debug_mask & (DEBUG_WAKE_LOCK | DEBUG_EXPIRE))
		pr_info("expired wake lock %s\n", lock->name);
}

/* Caller must acquire the list_lock spinlock */
static void print_active_locks(int type)
{
	struct wake_lock *lock;
	bool print_expired = true;

	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	list_for_each_entry(lock, &active_wake_locks[type], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout > 0)
				pr_info("active wake lock %s, time left %ld\n",
					lock->name, timeout);
			else if (print_expired)
				pr_info("wake lock %s, expired\n", lock->name);
		} else {
//Div251-PK-Dump_Wakelock-00+[
#ifdef CONFIG_FIH_DUMP_WAKELOCK
			ktime_t now = ktime_get();
			ktime_t active_time = ktime_sub(now, lock->stat.last_time);
			s64 ns = ktime_to_ns(active_time);
			s64 s = ns;			
			ns = do_div(s, NSEC_PER_SEC);
			pr_info("active wake lock %s %lld.%lld secs\n", lock->name, s, ns);
#else
			pr_info("active wake lock %s\n", lock->name);
#endif /* CONFIG_FIH_DUMP_WAKELOCK */
//Div251-PK-Dump_Wakelock-00+]
			if ((!debug_mask) & DEBUG_EXPIRE)
				print_expired = false;
		}
	}
}

//Div2-SW2-BSP-pmlog, HenryMCWang +
#if defined(CONFIG_FIH_POWER_LOG)
static void pmlog_active_locks(int type)
{
	unsigned long irqflags;
	struct wake_lock *lock;

	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_wake_locks[type], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout <= 0)
				pmlog("wake lock %s, expired\n", lock->name);
			else
				pmlog("active wake lock %s, time left %ld\n",
					lock->name, timeout);
		} else
			pmlog("active wake lock %s\n", lock->name);
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}
#endif
//Div2-SW2-BSP-pmlog, HenryMCWang -

static long has_wake_lock_locked(int type)
{
	struct wake_lock *lock, *n;
	long max_timeout = 0;

	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	list_for_each_entry_safe(lock, n, &active_wake_locks[type], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout <= 0)
				expire_wake_lock(lock);
			else if (timeout > max_timeout)
				max_timeout = timeout;
		} else
			return -1;
	}
	return max_timeout;
}

long has_wake_lock(int type)
{
	long ret;
	unsigned long irqflags;
	spin_lock_irqsave(&list_lock, irqflags);
	ret = has_wake_lock_locked(type);
	if (ret && (debug_mask & DEBUG_SUSPEND) && type == WAKE_LOCK_SUSPEND)
		print_active_locks(type);
	spin_unlock_irqrestore(&list_lock, irqflags);
	return ret;
}

//Div251-PK-Dump_Wakelock-00+[
#ifdef CONFIG_FIH_DUMP_WAKELOCK
static void dump_wakelocks(unsigned long data);
static DEFINE_TIMER(dump_wakelock_timer, dump_wakelocks, 0, 0);

static void dump_wakelocks(unsigned long data)
{
	unsigned long irqflags;

	if (debug_mask & DEBUG_POLLING_DUMP_WAKELOCK) {
		pr_info("[PM]--- dump_wakelocks ---\n");

		spin_lock_irqsave(&list_lock, irqflags);
			print_active_locks(WAKE_LOCK_SUSPEND);
		spin_unlock_irqrestore(&list_lock, irqflags);
	}

	mod_timer(&dump_wakelock_timer, jiffies + POLLING_DUMP_WAKELOCK_SECS*HZ);
}
#endif /* CONFIG_FIH_DUMP_WAKELOCK */
//Div251-PK-Dump_Wakelock-00+]

static void suspend(struct work_struct *work)
{
	int ret;
	int entry_event_num;

	if (has_wake_lock(WAKE_LOCK_SUSPEND)) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("suspend: abort suspend\n");
			
//Div251-PK-Dump_Wakelock-00+[
#ifdef CONFIG_FIH_DUMP_WAKELOCK
		pr_info("requested_suspend_state = %d\n", requested_suspend_state);
		if (requested_suspend_state != PM_SUSPEND_ON)
			mod_timer(&dump_wakelock_timer, jiffies + POLLING_DUMP_WAKELOCK_SECS*HZ);
#endif /* CONFIG_FIH_DUMP_WAKELOCK */
//Div251-PK-Dump_Wakelock-00+]
		return;
	}

	entry_event_num = current_event_num;
	sys_sync();
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("suspend: enter suspend\n");

//Div251-PK-Dump_Wakelock-00+[
#ifdef CONFIG_FIH_DUMP_WAKELOCK
	if (del_timer(&dump_wakelock_timer))
		pr_info("suspend: del dump_wakelock_timer\n");
#endif /* CONFIG_FIH_DUMP_WAKELOCK */
//Div251-PK-Dump_Wakelock-00+]

	//Div2-SW2-BSP-pmlog, HenryMCWang +
	#if defined(CONFIG_FIH_POWER_LOG)
	pmlog("suspend(): enter suspend\n");
	#endif
	//Div2-SW2-BSP-pmlog, HenryMCWang -

	ret = pm_suspend(requested_suspend_state);
	if (debug_mask & DEBUG_EXIT_SUSPEND) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		pr_info("suspend: exit suspend, ret = %d "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", ret,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
	if (current_event_num == entry_event_num) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("suspend: pm_suspend returned with no event\n");
		wake_lock_timeout(&unknown_wakeup, HZ / 2);
	}
}
static DECLARE_WORK(suspend_work, suspend);

static void expire_wake_locks(unsigned long data)
{
	long has_lock;
	unsigned long irqflags;
	if (debug_mask & DEBUG_EXPIRE)
		pr_info("expire_wake_locks: start\n");
	spin_lock_irqsave(&list_lock, irqflags);
	if (debug_mask & DEBUG_SUSPEND)
		print_active_locks(WAKE_LOCK_SUSPEND);
	has_lock = has_wake_lock_locked(WAKE_LOCK_SUSPEND);
	if (debug_mask & DEBUG_EXPIRE)
		pr_info("expire_wake_locks: done, has_lock %ld\n", has_lock);
	if (has_lock == 0)
		queue_work(suspend_work_queue, &suspend_work);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
static DEFINE_TIMER(expire_timer, expire_wake_locks, 0, 0);

static int power_suspend_late(struct device *dev)
{
	int ret = has_wake_lock(WAKE_LOCK_SUSPEND) ? -EAGAIN : 0;
#ifdef CONFIG_WAKELOCK_STAT
	wait_for_wakeup = 1;
#endif
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("power_suspend_late return %d\n", ret);
	return ret;
}

static struct dev_pm_ops power_driver_pm_ops = {
	.suspend_noirq = power_suspend_late,
};

static struct platform_driver power_driver = {
	.driver.name = "power",
	.driver.pm = &power_driver_pm_ops,
};
static struct platform_device power_device = {
	.name = "power",
};

void wake_lock_init(struct wake_lock *lock, int type, const char *name)
{
	unsigned long irqflags = 0;

	if (name)
		lock->name = name;
	BUG_ON(!lock->name);

	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_lock_init name=%s\n", lock->name);
#ifdef CONFIG_WAKELOCK_STAT
	lock->stat.count = 0;
	lock->stat.expire_count = 0;
	lock->stat.wakeup_count = 0;
	lock->stat.total_time = ktime_set(0, 0);
	lock->stat.prevent_suspend_time = ktime_set(0, 0);
	lock->stat.max_time = ktime_set(0, 0);
	lock->stat.last_time = ktime_set(0, 0);
#endif
	lock->flags = (type & WAKE_LOCK_TYPE_MASK) | WAKE_LOCK_INITIALIZED;

	INIT_LIST_HEAD(&lock->link);
	spin_lock_irqsave(&list_lock, irqflags);
	list_add(&lock->link, &inactive_locks);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_lock_init);

void wake_lock_destroy(struct wake_lock *lock)
{
	unsigned long irqflags;
	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_lock_destroy name=%s\n", lock->name);
	spin_lock_irqsave(&list_lock, irqflags);
	lock->flags &= ~WAKE_LOCK_INITIALIZED;
#ifdef CONFIG_WAKELOCK_STAT
	if (lock->stat.count) {
		deleted_wake_locks.stat.count += lock->stat.count;
		deleted_wake_locks.stat.expire_count += lock->stat.expire_count;
		deleted_wake_locks.stat.total_time =
			ktime_add(deleted_wake_locks.stat.total_time,
				  lock->stat.total_time);
		deleted_wake_locks.stat.prevent_suspend_time =
			ktime_add(deleted_wake_locks.stat.prevent_suspend_time,
				  lock->stat.prevent_suspend_time);
		deleted_wake_locks.stat.max_time =
			ktime_add(deleted_wake_locks.stat.max_time,
				  lock->stat.max_time);
	}
#endif
	list_del(&lock->link);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_lock_destroy);

static void wake_lock_internal(
	struct wake_lock *lock, long timeout, int has_timeout)
{
	int type;
	unsigned long irqflags;
	long expire_in;

	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & WAKE_LOCK_TYPE_MASK;
	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	BUG_ON(!(lock->flags & WAKE_LOCK_INITIALIZED));
#ifdef CONFIG_WAKELOCK_STAT
	if (type == WAKE_LOCK_SUSPEND && wait_for_wakeup) {
		if (debug_mask & DEBUG_WAKEUP)
			pr_info("wakeup wake lock: %s\n", lock->name);
		wait_for_wakeup = 0;
		lock->stat.wakeup_count++;
	}
	if ((lock->flags & WAKE_LOCK_AUTO_EXPIRE) &&
	    (long)(lock->expires - jiffies) <= 0) {
		wake_unlock_stat_locked(lock, 0);
		lock->stat.last_time = ktime_get();
	}
#endif
	if (!(lock->flags & WAKE_LOCK_ACTIVE)) {
		lock->flags |= WAKE_LOCK_ACTIVE;
#ifdef CONFIG_WAKELOCK_STAT
		lock->stat.last_time = ktime_get();
#endif
	}
	list_del(&lock->link);
	if (has_timeout) {
		if (debug_mask & DEBUG_WAKE_LOCK)
			pr_info("wake_lock: %s, type %d, timeout %ld.%03lu\n",
				lock->name, type, timeout / HZ,
				(timeout % HZ) * MSEC_PER_SEC / HZ);
		lock->expires = jiffies + timeout;
		lock->flags |= WAKE_LOCK_AUTO_EXPIRE;
		list_add_tail(&lock->link, &active_wake_locks[type]);
	} else {
		if (debug_mask & DEBUG_WAKE_LOCK)
			pr_info("wake_lock: %s, type %d\n", lock->name, type);
		lock->expires = LONG_MAX;
		lock->flags &= ~WAKE_LOCK_AUTO_EXPIRE;
		list_add(&lock->link, &active_wake_locks[type]);
	}
	if (type == WAKE_LOCK_SUSPEND) {
		current_event_num++;
//Div251-PK-Dump_Wakelock-00+[
#ifdef CONFIG_FIH_DUMP_WAKELOCK
		if (lock == &main_wake_lock) {
			if (del_timer(&dump_wakelock_timer))
				pr_info("main_wake_lock: del dump_wakelock_timer\n");
		}
#endif /* CONFIG_FIH_DUMP_WAKELOCK */
//Div251-PK-Dump_Wakelock-00+]
#ifdef CONFIG_WAKELOCK_STAT
		if (lock == &main_wake_lock)
			update_sleep_wait_stats_locked(1);
		else if (!wake_lock_active(&main_wake_lock))
			update_sleep_wait_stats_locked(0);
#endif
		if (has_timeout)
			expire_in = has_wake_lock_locked(type);
		else
			expire_in = -1;
		if (expire_in > 0) {
			if (debug_mask & DEBUG_EXPIRE)
				pr_info("wake_lock: %s, start expire timer, "
					"%ld\n", lock->name, expire_in);
			mod_timer(&expire_timer, jiffies + expire_in);
		} else {
			if (del_timer(&expire_timer))
				if (debug_mask & DEBUG_EXPIRE)
					pr_info("wake_lock: %s, stop expire timer\n",
						lock->name);
			if (expire_in == 0)
				queue_work(suspend_work_queue, &suspend_work);
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void wake_lock(struct wake_lock *lock)
{
	wake_lock_internal(lock, 0, 0);
}
EXPORT_SYMBOL(wake_lock);

void wake_lock_timeout(struct wake_lock *lock, long timeout)
{
	wake_lock_internal(lock, timeout, 1);
}
EXPORT_SYMBOL(wake_lock_timeout);

void wake_unlock(struct wake_lock *lock)
{
	int type;
	unsigned long irqflags;
	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & WAKE_LOCK_TYPE_MASK;
#ifdef CONFIG_WAKELOCK_STAT
	wake_unlock_stat_locked(lock, 0);
#endif
	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_unlock: %s\n", lock->name);
	lock->flags &= ~(WAKE_LOCK_ACTIVE | WAKE_LOCK_AUTO_EXPIRE);
	list_del(&lock->link);
	list_add(&lock->link, &inactive_locks);
	if (type == WAKE_LOCK_SUSPEND) {
		long has_lock = has_wake_lock_locked(type);
		if (has_lock > 0) {
			if (debug_mask & DEBUG_EXPIRE)
				pr_info("wake_unlock: %s, start expire timer, "
					"%ld\n", lock->name, has_lock);
			mod_timer(&expire_timer, jiffies + has_lock);
		} else {
			if (del_timer(&expire_timer))
				if (debug_mask & DEBUG_EXPIRE)
					pr_info("wake_unlock: %s, stop expire "
						"timer\n", lock->name);
			if (has_lock == 0)
				queue_work(suspend_work_queue, &suspend_work);
		}
		if (lock == &main_wake_lock) {
			if (debug_mask & DEBUG_SUSPEND)
				print_active_locks(WAKE_LOCK_SUSPEND);
			//Div2-SW2-BSP-pmlog, HenryMCWang +
			#if defined(CONFIG_FIH_POWER_LOG)
			pmlog_active_locks(WAKE_LOCK_SUSPEND);
			#endif
			//Div2-SW2-BSP-pmlog, HenryMCWang -
//Div251-PK-Dump_Wakelock-00+[
#ifdef CONFIG_FIH_DUMP_WAKELOCK
			if (has_lock){
				pr_info("but still have lock.\n");
				mod_timer(&dump_wakelock_timer, jiffies + POLLING_DUMP_WAKELOCK_SECS*HZ);
			}
#endif /* CONFIG_FIH_DUMP_WAKELOCK */
//Div251-PK-Dump_Wakelock-00+]
#ifdef CONFIG_WAKELOCK_STAT
			update_sleep_wait_stats_locked(0);
#endif
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_unlock);

int wake_lock_active(struct wake_lock *lock)
{
	return !!(lock->flags & WAKE_LOCK_ACTIVE);
}
EXPORT_SYMBOL(wake_lock_active);

static int wakelock_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, wakelock_stats_show, NULL);
}

static const struct file_operations wakelock_stats_fops = {
	.owner = THIS_MODULE,
	.open = wakelock_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init wakelocks_init(void)
{
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(active_wake_locks); i++)
		INIT_LIST_HEAD(&active_wake_locks[i]);

#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_init(&deleted_wake_locks, WAKE_LOCK_SUSPEND,
			"deleted_wake_locks");
#endif
	wake_lock_init(&main_wake_lock, WAKE_LOCK_SUSPEND, "main");
	wake_lock(&main_wake_lock);
	wake_lock_init(&unknown_wakeup, WAKE_LOCK_SUSPEND, "unknown_wakeups");

	ret = platform_device_register(&power_device);
	if (ret) {
		pr_err("wakelocks_init: platform_device_register failed\n");
		goto err_platform_device_register;
	}
	ret = platform_driver_register(&power_driver);
	if (ret) {
		pr_err("wakelocks_init: platform_driver_register failed\n");
		goto err_platform_driver_register;
	}

	suspend_work_queue = create_singlethread_workqueue("suspend");
	if (suspend_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_suspend_work_queue;
	}

#ifdef CONFIG_WAKELOCK_STAT
	proc_create("wakelocks", S_IRUGO, NULL, &wakelock_stats_fops);
#endif

//Div2-SW2-BSP-pmlog, HenryMCWang +
#ifdef __FIH_PM_STATISTICS__
	create_proc_read_entry("pms", S_IRUGO, NULL,
				pms_read_proc, NULL);
#endif	// __FIH_PM_STATISTICS__
///-FIH_ADQ

#ifdef __FIH_PM_LOG__
	//create_proc_read_entry("pmlog", S_IRUGO, NULL,
	//			pmlog_read_proc, NULL);
#endif	//__FIH_PM_LOG__
//Div2-SW2-BSP-pmlog, HenryMCWang -

	return 0;

err_suspend_work_queue:
	platform_driver_unregister(&power_driver);
err_platform_driver_register:
	platform_device_unregister(&power_device);
err_platform_device_register:
	wake_lock_destroy(&unknown_wakeup);
	wake_lock_destroy(&main_wake_lock);
#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_destroy(&deleted_wake_locks);
#endif
	return ret;
}

static void  __exit wakelocks_exit(void)
{
#ifdef CONFIG_WAKELOCK_STAT
	remove_proc_entry("wakelocks", NULL);
#endif
	destroy_workqueue(suspend_work_queue);
	platform_driver_unregister(&power_driver);
	platform_device_unregister(&power_device);
	wake_lock_destroy(&unknown_wakeup);
	wake_lock_destroy(&main_wake_lock);
#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_destroy(&deleted_wake_locks);
#endif
}

core_initcall(wakelocks_init);
module_exit(wakelocks_exit);
