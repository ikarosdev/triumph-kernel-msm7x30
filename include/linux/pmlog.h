
#if defined(CONFIG_FIH_POWER_LOG)
#define __FIH_PM_STATISTICS__
//#define __FIH_DBG_PM_STATISTICS__
#define __FIH_PM_LOG__
#endif

struct pms {
	unsigned long time;
	unsigned long cnt;
	unsigned long pre;
};

struct hpms {
	unsigned long ntime;
	unsigned long time;
	unsigned long cnt;
	unsigned long npre;
	unsigned long pre;
};

extern struct pms g_pms_run;
extern struct pms g_pms_bkrun;
extern struct hpms g_pms_suspend;

extern int g_secupdatereq;

#ifdef __FIH_DBG_PM_STATISTICS__
extern struct hpms g_pms_resume;
extern int g_timelose;
#endif	// __FIH_DBG_PM_STATISTICS__

unsigned long get_nseconds(void);

#ifdef __FIH_PM_LOG__
int pmlog(const char *fmt, ...);
#else	// __FIH_PM_LOG__
#define pmlog(x, ...) {}
#endif	//__FIH_PM_LOG__
