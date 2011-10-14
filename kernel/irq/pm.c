/*
 * linux/kernel/irq/pm.c
 *
 * Copyright (C) 2009 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 *
 * This file contains power management functions related to interrupts.
 */

#include <linux/irq.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#include "internals.h"

/**
 * suspend_device_irqs - disable all currently enabled interrupt lines
 *
 * During system-wide suspend or hibernation device drivers need to be prevented
 * from receiving interrupts and this function is provided for this purpose.
 * It marks all interrupt lines in use, except for the timer ones, as disabled
 * and sets the IRQ_SUSPENDED flag for each of them.
 */
void suspend_device_irqs(void)
{
	struct irq_desc *desc;
	int irq;

	for_each_irq_desc(irq, desc) {
		unsigned long flags;

		spin_lock_irqsave(&desc->lock, flags);
		__disable_irq(desc, irq, true);
		spin_unlock_irqrestore(&desc->lock, flags);
	}

	for_each_irq_desc(irq, desc)
		if (desc->status & IRQ_SUSPENDED)
			synchronize_irq(irq);
}
EXPORT_SYMBOL_GPL(suspend_device_irqs);

/**
 * resume_device_irqs - enable interrupt lines disabled by suspend_device_irqs()
 *
 * Enable all interrupt lines previously disabled by suspend_device_irqs() that
 * have the IRQ_SUSPENDED flag set.
 */
void resume_device_irqs(void)
{
	struct irq_desc *desc;
	int irq;

	for_each_irq_desc(irq, desc) {
		unsigned long flags;

		if (!(desc->status & IRQ_SUSPENDED))
			continue;

		spin_lock_irqsave(&desc->lock, flags);
		__enable_irq(desc, irq, true);
		spin_unlock_irqrestore(&desc->lock, flags);
	}
}
EXPORT_SYMBOL_GPL(resume_device_irqs);

/**
 * check_wakeup_irqs - check if any wake-up interrupts are pending
 */
//Div2-SW2-BSP-SuspendLog, VinceCCTsai+[
#define MSM_PM_DEBUG_FIH_MODULE (1U << 8)
extern int msm_pm_debug_mask;
//Div2-SW2-BSP-SuspendLog, VinceCCTsai-]
int check_wakeup_irqs(void)
{
	struct irq_desc *desc;
	int irq;

	for_each_irq_desc(irq, desc)
//Div2-SW2-BSP-SuspendLog, VinceCCTsai+[
#ifdef CONFIG_FIH_POWER_LOG
		if (desc->status & IRQ_WAKEUP) {
			if (msm_pm_debug_mask & MSM_PM_DEBUG_FIH_MODULE)
				printk(KERN_INFO "GPIO%03d set WAKEUP : %x\n", irq, desc->status);
			if (desc->status & IRQ_PENDING) {
				printk(KERN_INFO "Suspend aborted by IRQ_WAKEUP(%d) IRQ_PENDING(%x).\n", irq, desc->status);
				return -EBUSY;
			}
		}
#else	// CONFIG_FIH_POWER_LOG
		if ((desc->status & IRQ_WAKEUP) && (desc->status & IRQ_PENDING))
			return -EBUSY;
#endif	// CONFIG_FIH_POWER_LOG
//Div2-SW2-BSP-SuspendLog, VinceCCTsai-]

	return 0;
}
