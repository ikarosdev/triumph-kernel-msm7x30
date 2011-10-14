/**+===========================================================================
  File: alog_ram_console.c

  Description:


  Note:


  Author:
        Bryan Hsieh May-30-2010
-------------------------------------------------------------------------------
** FIHSPEC CONFIDENTIAL
** Copyright(c) 2009 FIHSPEC Corporation. All Rights Reserved.
**^M
** The source code contained or described herein and all documents related
** to the source code (Material) are owned by FIHSPEC Technology Corporation.
** The Material is protected by worldwide copyright and trade secret laws and
** treaty provisions. No part of the Material may be used, copied, reproduced,
** modified, published, uploaded, posted, transmitted, distributed, or disclosed
** in any way without FIHSPEC prior express written permission.
============================================================================+*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/ctype.h>
#include <linux/time.h>
#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <asm/current.h>

#include "mach/alog_ram_console.h"

//#define BUFFER_DEBUG  // this flag can't be defined if klogd doesn't be filtered in logger
//#define OUTPUT_TEXT // this flag is used for outputing readable text directly, note: local time display issue is not fixed
//#define OUTPUT_TEXT_DEBUG // this flag is used for debugging output readable text

spinlock_t		bufflock;	/* spinlock for buffer */


/*
 * Android log priority values, in ascending priority order.
 */
typedef enum android_LogPriority {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} android_LogPriority;

#if 0
typedef enum {
    FORMAT_OFF = 0,
    FORMAT_BRIEF,
    FORMAT_PROCESS,
    FORMAT_TAG,
    FORMAT_THREAD,
    FORMAT_RAW,
    FORMAT_TIME,
    FORMAT_THREADTIME,
    FORMAT_LONG,
} AndroidLogPrintFormat;

typedef long pthread_t;

typedef struct FilterInfo_t {
    char *mTag;
    android_LogPriority mPri;
    struct FilterInfo_t *p_next;
} FilterInfo;

typedef struct AndroidLogFormat_t {
    android_LogPriority global_pri;
    FilterInfo *filters;
    AndroidLogPrintFormat format;
} AndroidLogFormat;

typedef struct AndroidLogEntry_t {
    time_t tv_sec;
    long tv_nsec;
    android_LogPriority priority;
    pid_t pid;
    pthread_t tid;
    const char * tag;
    size_t messageLen;
    const char * message;
} AndroidLogEntry;
#endif

typedef struct LoggerEntry_t {
	__u16		len;	/* length of the payload */
	__u16		__pad;	/* no matter what, we get 2 bytes of padding */
	__s32		pid;	/* generating process's pid */
	__s32		tid;	/* generating process's tid */
	__s32		sec;	/* seconds since Epoch */
	__s32		nsec;	/* nanoseconds */
	char		msg[0];	/* the entry's payload */
} LoggerEntry;

#define LOGGER_ENTRY_MAX_LEN		(4*1024)
#define LOGGER_ENTRY_MAX_PAYLOAD	(LOGGER_ENTRY_MAX_LEN - sizeof(LoggerEntry))

#define ALOG_RAM_CONSOLE_MAIN_SIG (0x4E49414D) /* MAIN */
#define ALOG_RAM_CONSOLE_RADIO_SIG (0x49444152) /* RADI */
#define ALOG_RAM_CONSOLE_EVENTS_SIG (0x4E455645) /* EVEN */
#define ALOG_RAM_CONSOLE_SYSTEM_SIG (0x54535953) /* SYST */	//SW2-5-2-MP-DbgCfgTool-02+

typedef struct alog_ram_console_buffer_t {
	uint32_t    sig;
	uint32_t    start;
	uint32_t    size;
	uint32_t    reader_pos;
	uint8_t     data[0];
}AlogRamConsoleBuffer;

static char *alog_ram_console_old_log[LOG_TYPE_NUM];
static size_t alog_ram_console_old_log_size[LOG_TYPE_NUM];

static AlogRamConsoleBuffer *alog_ram_console_buffer[LOG_TYPE_NUM]; // alog ram console structure identity
static size_t alog_ram_console_buffer_size[LOG_TYPE_NUM]; // maximum of  usable alog ram console buffer size
static uint32_t alog_ram_console_signature[LOG_TYPE_NUM] = {  // alog ram console signature
	ALOG_RAM_CONSOLE_MAIN_SIG,
	ALOG_RAM_CONSOLE_RADIO_SIG,
	ALOG_RAM_CONSOLE_EVENTS_SIG,
	ALOG_RAM_CONSOLE_SYSTEM_SIG
};
static char* alog_ram_console_proc_fn[LOG_TYPE_NUM] = {  // alog ram console proc file name
	"last_alog_main",
	"last_alog_radio",
	"last_alog_events",
	"last_alog_system"
};
static char* alog_ram_console_res_name[LOG_TYPE_NUM] = { // alog ram console resource name
	"alog_main_buffer",
	"alog_radio_buffer",
	"alog_events_buffer",
	"alog_system_buffer"
};
	
static int need_fix_reader[LOG_TYPE_NUM];
static char *  next_log_entry[LOG_TYPE_NUM];

//CAUTION!!!this function have to be called within a block of logger buffer lock, otherwise it might disorder log entry.
void alog_ram_console_sync_time(const LogType log_type, const SyncType sync_type)
{
	struct timespec now;
	LoggerEntry header;
	unsigned long flags;
	char tbuf[50];
	unsigned tlen;
	unsigned long long t;
	unsigned long nanosec_rem;
	unsigned int prio = ANDROID_LOG_INFO;	
	int i; // Div6-PT2-Core-BH-AlogRamConsole-02+
			
	header.pid = current->tgid;
	header.tid = current->pid;

	spin_lock_irqsave(&bufflock, flags); // Div6-PT2-Core-BH-AlogRamConsole-02*
	
	now = current_kernel_time();
	header.sec = now.tv_sec;
	header.nsec = now.tv_nsec;

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	tlen = sprintf(tbuf, "[%5lu.%06lu] ", (unsigned long) t, nanosec_rem / 1000);
	
	header.len = tlen + 8;

	 // Div6-PT2-Core-BH-AlogRamConsole-02*[
	if (log_type == LOG_TYPE_ALL)
	{
		if (sync_type == SYNC_AFTER)
			printk(KERN_INFO "[LastAlog] alog_ram_console_sync_time %s for all log_type - After\n", tbuf);
		else
			printk(KERN_INFO "[LastAlog] alog_ram_console_sync_time %s for all log_type - Before\n", tbuf);
	
		for (i =LOG_TYPE_MAIN; i<LOG_TYPE_NUM; i++)
		{
			alog_ram_console_write_log(i, (char *)&header, (int)sizeof(LoggerEntry));
			alog_ram_console_write_log(i, (char *)&prio, 1);
			if (sync_type == SYNC_AFTER)
				alog_ram_console_write_log(i, "SYNCA", 6);
			else
				alog_ram_console_write_log(i, "SYNCB", 6);
			alog_ram_console_write_log(i, tbuf, tlen+1);
		}
	}
	else
	{
		alog_ram_console_write_log(log_type, (char *)&header, (int)sizeof(LoggerEntry));
		alog_ram_console_write_log(log_type, (char *)&prio, 1);
		if (sync_type == SYNC_AFTER)
		{
			printk(KERN_INFO "[LastAlog] alog_ram_console_sync_time %s for log_type:%d - After\n", tbuf, log_type);
			alog_ram_console_write_log(log_type, "SYNCA", 6);
		}
		else
		{
			printk(KERN_INFO "[LastAlog] alog_ram_console_sync_time %s for log_type:%d - Before\n", tbuf, log_type);
			alog_ram_console_write_log(log_type, "SYNCB", 6);
		}
		alog_ram_console_write_log(log_type, tbuf, tlen+1);
	}
	 // Div6-PT2-Core-BH-AlogRamConsole-02*]
	 
	spin_unlock_irqrestore(&bufflock, flags);

}	

// there is no spinlock inside tjis function, please take care the lock mechanism outside this function.
int alog_ram_console_write_log(const LogType log_type, char * s, int count){
	int rem,next_entry_len,skip_len,cut_len;
	AlogRamConsoleBuffer *buffer = alog_ram_console_buffer[log_type];
	LoggerEntry *temp_entry;
	char tempbuffer[20];
	int overrun = 0;
	char *orig_s = s;
#ifdef BUFFER_DEBUG
	int i = 0;

	printk(KERN_INFO "[LastAlog] write_log: <log_type>:%d <buffer->start>:%d <buffer->size>:%d <rem>:%d <count>:%d", log_type, buffer->start, buffer->size, alog_ram_console_buffer_size[log_type] - buffer->start, count);
#endif

	rem = alog_ram_console_buffer_size[log_type] - buffer->start;

	// if buffer remain length < message length, update ram console by remain part, then restart pointer to 0
	if (rem < count) 
	{
		s += rem;
		buffer->size = alog_ram_console_buffer_size[log_type];
		need_fix_reader[log_type] = 1; // That means the reader positon must be fixed once overrun happens
		overrun = 1; // overrun flag, default value is 0 in every log written
	}

	// fixe up the reader position to get the next readable entry when this data will overwrite next log entry
	if (need_fix_reader[log_type])
	{
		// init skip len between reader_pos and start
		if (buffer->reader_pos >=  buffer->start)
			skip_len=buffer->reader_pos - buffer->start; 
		else
			skip_len=buffer->size - buffer->start + buffer->reader_pos;
		
		if (count > skip_len)
		{		
			// fix reader position for overwite case
			do
			{
				// get next entry hader info
				if ((unsigned int)next_log_entry[log_type] + sizeof(LoggerEntry) > (unsigned int)buffer->data + buffer->size)
				{	
					// if entry header is over run, adjust it to get correct entry header
					cut_len = (unsigned int)buffer->data + buffer->size-(unsigned int)next_log_entry[log_type];
					memcpy(tempbuffer, next_log_entry[log_type], cut_len);
					memcpy(tempbuffer+(unsigned int)buffer->data + buffer->size-(unsigned int)next_log_entry[log_type], buffer->data, sizeof(LoggerEntry) - cut_len);
					temp_entry = (LoggerEntry *)tempbuffer;
#ifdef BUFFER_DEBUG				
					printk(" => get header over run!");					
#endif					
				}
				else
					temp_entry = (LoggerEntry *)next_log_entry[log_type];

				next_entry_len =  temp_entry->len + sizeof(LoggerEntry);

#ifdef BUFFER_DEBUG
				printk(" => find: <next_log_entry>:0x%X <len>:%d <pos>:%d", (unsigned int)next_log_entry[log_type], next_entry_len, (unsigned int)next_log_entry[log_type] - (unsigned int)buffer->data);
				printk(" [header: ");
				for (i=0 ;i<20; i++)
					printk("%02X ",*((char *)temp_entry+i));
				printk("]");				
#endif
				
				// find the first readable entry header after this log write
				skip_len += next_entry_len;
				// go to next log entry
				if ((unsigned int)next_log_entry[log_type] + next_entry_len > (unsigned int)buffer->data + buffer->size)
				{
					// if next log entry is over run, adjust it to turm around to begin
					next_log_entry[log_type] = buffer->data + (next_entry_len - (buffer->size - ((unsigned int)next_log_entry[log_type] - (unsigned int)buffer->data)));
				}
				else
					next_log_entry[log_type] = next_log_entry[log_type] + next_entry_len;
					
			} while (skip_len < count);

			buffer->reader_pos = next_log_entry[log_type] - (char *)buffer->data;

#ifdef BUFFER_DEBUG
			temp_entry = (LoggerEntry *)next_log_entry[log_type];
			next_entry_len =  temp_entry->len + sizeof(LoggerEntry);			
			printk(" => final: <next_log_entry>:0x%X <len>:%d <pos>:%d", (unsigned int)next_log_entry[log_type], next_entry_len, (unsigned int)next_log_entry[log_type] - (unsigned int)buffer->data);
			printk(" => fix up <reader_pos>:%d <offset>:%d", buffer->reader_pos, buffer->reader_pos - buffer->start - count);
#endif
		}
		else
		{
#ifdef BUFFER_DEBUG		
			printk(" => no fix up");
#endif			
		}
	}
	else
	{
		buffer->reader_pos = buffer->start + count;	
#ifdef BUFFER_DEBUG		
		printk(" => no fix up");
#endif		
	}
	
	// if overrun happens, copy the preceding string to start position
	if (overrun)
	{
		memcpy(buffer->data + buffer->start, orig_s, rem);
#ifdef BUFFER_DEBUG
		printk(" => write: [data over run!");
		printk(" pos:%d data1:", buffer->start);
		for (i=0 ;i<rem; i++)
			printk("%02X ",*(buffer->data + buffer->start + i));
#endif
		buffer->start = 0;
		count -= rem;
	}
	// copy whole or rest string to start position or 0 (overrun)
	memcpy(buffer->data + buffer->start, s, count);
#ifdef BUFFER_DEBUG
	if (overrun)
	{
		printk(" pos:%d data2:", buffer->start);
		for (i=0 ;i<count; i++)
			printk("%02X ",*(buffer->data + buffer->start + i));
		printk("]");		
	}
	else
	{
		printk(" => write: [");	
		printk(" pos:%d data:", buffer->start);
		for (i=0 ;i<count; i++)
			printk("%02X ",*(buffer->data + buffer->start + i));
		printk("]");		
	}
#endif
	buffer->start += count; // update start pointer
	if (buffer->size < alog_ram_console_buffer_size[log_type])
		buffer->size += count; // update used buffer size	
#ifdef BUFFER_DEBUG
	printk(" => <buffer->start>:%d\n", buffer->start);
#endif

	return overrun;
}

//CAUTION!!!this function is only for test usage, otherwise it might disorder log entry with logger.
void alog_ram_console_write_entry(const LogType log_type,
										char *priority,
										char * tag, int tag_bytes,
										char * msg, int msg_bytes)
{
	struct timespec now;
	LoggerEntry header;
	unsigned long flags;
	int overrun=0;

	header.pid = current->tgid;
	header.tid = current->pid;

	now = current_kernel_time();
	header.sec = now.tv_sec;
	header.nsec = now.tv_nsec;

	header.len = min_t(size_t, tag_bytes + msg_bytes + 1, LOGGER_ENTRY_MAX_PAYLOAD);

	spin_lock_irqsave(&bufflock, flags);

	overrun += alog_ram_console_write_log(log_type, (char *)&header, (int)sizeof(LoggerEntry));

	overrun += alog_ram_console_write_log(log_type, priority, 1);

	overrun += alog_ram_console_write_log(log_type, tag, tag_bytes);

	overrun += alog_ram_console_write_log(log_type, msg, msg_bytes);

	spin_unlock_irqrestore(&bufflock, flags);	

	if (overrun)
		alog_ram_console_sync_time(log_type, SYNC_AFTER);
		
}

#if 0
// following code segment is usaed for 
static android_LogPriority filterCharToPri (char c)
{
    android_LogPriority pri;

    c = tolower(c);

    if (c >= '0' && c <= '9') {
        if (c >= ('0'+ANDROID_LOG_SILENT)) {
            pri = ANDROID_LOG_VERBOSE;
        } else {
            pri = (android_LogPriority)(c - '0');
        }
    } else if (c == 'v') {
        pri = ANDROID_LOG_VERBOSE;
    } else if (c == 'd') {
        pri = ANDROID_LOG_DEBUG;
    } else if (c == 'i') {
        pri = ANDROID_LOG_INFO;
    } else if (c == 'w') {
        pri = ANDROID_LOG_WARN;
    } else if (c == 'e') {
        pri = ANDROID_LOG_ERROR;
    } else if (c == 'f') {
        pri = ANDROID_LOG_FATAL;
    } else if (c == 's') {
        pri = ANDROID_LOG_SILENT;
    } else if (c == '*') {
        pri = ANDROID_LOG_DEFAULT;
    } else {
        pri = ANDROID_LOG_UNKNOWN;
    }

    return pri;
}

static char filterPriToChar (android_LogPriority pri)
{
    switch (pri) {
        case ANDROID_LOG_VERBOSE:       return 'V';
        case ANDROID_LOG_DEBUG:         return 'D';
        case ANDROID_LOG_INFO:          return 'I';
        case ANDROID_LOG_WARN:          return 'W';
        case ANDROID_LOG_ERROR:         return 'E';
        case ANDROID_LOG_FATAL:         return 'F';
        case ANDROID_LOG_SILENT:        return 'S';

        case ANDROID_LOG_DEFAULT:
        case ANDROID_LOG_UNKNOWN:
        default:                        return '?';
    }
}

char *alog_formatLogLine (
    AndroidLogFormat *p_format,
    char *defaultBuffer,
    size_t defaultBufferSize,
    const AndroidLogEntry *entry,
    size_t *p_outLength)
{
#if defined(HAVE_LOCALTIME_R)
    //struct tm tmBuf;
#endif
    //struct tm* ptm;
    char timeBuf[32];
    char prefixBuf[128], suffixBuf[128];
    char priChar;
    int prefixSuffixIsHeaderFooter = 0;
    char * ret = NULL;
    size_t prefixLen, suffixLen;
    size_t numLines;
    char *p;
    size_t bufferSize;
    const char *pm;
	
    priChar = filterPriToChar(filterCharToPri(entry->priority));

#if defined(HAVE_LOCALTIME_R)
    //ptm = localtime_r(&(entry->tv_sec), &tmBuf);
#else
    //ptm = localtime(&(entry->tv_sec));
#endif
    // strftime(timeBuf, sizeof(timeBuf), "%m-%d %H:%M:%S", ptm);
	snprintf(timeBuf, sizeof(prefixBuf),"%d-%d %d:%d:%d", (unsigned int)entry->tv_sec%13, (unsigned int)entry->tv_sec%32, (unsigned int)entry->tv_sec%25, (unsigned int)entry->tv_sec%60, (unsigned int)entry->tv_sec%60);
	
    /*
     * Construct a buffer containing the log header and log message.
     */

    switch (p_format->format) {
        case FORMAT_TAG:
            prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
                "%c/%-8s: ", priChar, entry->tag);
            strcpy(suffixBuf, "\n"); suffixLen = 1;
            break;
        case FORMAT_PROCESS:
            prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
                "%c(%5d) ", priChar, entry->pid);
            suffixLen = snprintf(suffixBuf, sizeof(suffixBuf),
                "  (%s)\n", entry->tag);
            break;
        case FORMAT_THREAD:
            prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
                "%c(%5d:%p) ", priChar, entry->pid, (void*)entry->tid);
            strcpy(suffixBuf, "\n");
            suffixLen = 1;
            break;
        case FORMAT_RAW:
            prefixBuf[0] = 0;
            prefixLen = 0;
            strcpy(suffixBuf, "\n");
            suffixLen = 1;
            break;
        case FORMAT_TIME:
            prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
                "%s.%03ld %c/%-8s(%5d): ", timeBuf, entry->tv_nsec / 1000000,
                priChar, entry->tag, entry->pid);
            strcpy(suffixBuf, "\n");
            suffixLen = 1;
            break;
        case FORMAT_THREADTIME:
            prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
                "%s.%03ld %5d %5d %c %-8s: ", timeBuf, entry->tv_nsec / 1000000,
                (int)entry->pid, (int)entry->tid, priChar, entry->tag);
            strcpy(suffixBuf, "\n");
            suffixLen = 1;
            break;
        case FORMAT_LONG:
            prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
                "[ %s.%03ld %5d:%p %c/%-8s ]\n",
                timeBuf, entry->tv_nsec / 1000000, entry->pid,
                (void*)entry->tid, priChar, entry->tag);
            strcpy(suffixBuf, "\n\n");
            suffixLen = 2;
            prefixSuffixIsHeaderFooter = 1;
            break;
        case FORMAT_BRIEF:
        default:
            prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
                "%c/%-8s(%5d): ", priChar, entry->tag, entry->pid);
            strcpy(suffixBuf, "\n");
            suffixLen = 1;
            break;
    }

    /* the following code is tragically unreadable */

    if (prefixSuffixIsHeaderFooter) {
        // we're just wrapping message with a header/footer
        numLines = 1;
    } else {
        pm = entry->message;
        numLines = 0;

        // The line-end finding here must match the line-end finding
        // in for ( ... numLines...) loop below
        while (pm < (entry->message + entry->messageLen)) {
            if (*pm++ == '\n') numLines++;
        }
        // plus one line for anything not newline-terminated at the end
        if (pm > entry->message && *(pm-1) != '\n') numLines++;
    }

    // this is an upper bound--newlines in message may be counted
    // extraneously
    bufferSize = (numLines * (prefixLen + suffixLen)) + entry->messageLen + 1;

    if (defaultBufferSize >= bufferSize) {
        ret = defaultBuffer;
    } else {
        ret = (char *)kmalloc(bufferSize,GFP_KERNEL);

        if (ret == NULL) {
            return ret;
        }
    }

    ret[0] = '\0';       /* to start strcat off */

    p = ret;
    pm = entry->message;

    if (prefixSuffixIsHeaderFooter) {
        strcat(p, prefixBuf);
        p += prefixLen;
        strncat(p, entry->message, entry->messageLen);
        p += entry->messageLen;
        strcat(p, suffixBuf);
        p += suffixLen;
    } else {
        while(pm < (entry->message + entry->messageLen)) {
            const char *lineStart;
            size_t lineLen;

            lineStart = pm;

            // Find the next end-of-line in message
            while (pm < (entry->message + entry->messageLen)
                    && *pm != '\n') pm++;
            lineLen = pm - lineStart;

            strcat(p, prefixBuf);
            p += prefixLen;
            strncat(p, lineStart, lineLen);
            p += lineLen;
            strcat(p, suffixBuf);
            p += suffixLen;

            if (*pm == '\n') pm++;
        }
    }

    if (p_outLength != NULL) {
        *p_outLength = p - ret;
    }

    return ret;
}

static android_LogPriority filterPriForTag(AndroidLogFormat *p_format, const char *tag)
{
    FilterInfo *p_curFilter;

    for (p_curFilter = p_format->filters
            ; p_curFilter != NULL
            ; p_curFilter = p_curFilter->p_next
    ) {
        if (0 == strcmp(tag, p_curFilter->mTag)) {
            if (p_curFilter->mPri == ANDROID_LOG_DEFAULT) {
                return p_format->global_pri;
            } else {
                return p_curFilter->mPri;
            }
        }
    }

    return p_format->global_pri;
}

int android_log_shouldPrintLine (AndroidLogFormat *p_format, const char *tag, android_LogPriority pri)
{
    return pri >= filterPriForTag(p_format, tag);
}

int alog_filterAndPrintLogLine(AndroidLogFormat *p_format, const AndroidLogEntry *entry,char * dest)
{
    char defaultBuffer[512];
    char *outBuffer = NULL;
    size_t totalLen;

    if (0 == android_log_shouldPrintLine(p_format, entry->tag, entry->priority)) {
        return 0;
    }

    outBuffer = alog_formatLogLine(p_format,defaultBuffer,sizeof(defaultBuffer), entry, &totalLen);

    printk(KERN_INFO "[LastAlog] alog_filterAndPrintLogLine: <outBuffer>:%s", outBuffer);

    if (!outBuffer)
        return -1;

   memcpy(dest, outBuffer,strlen(outBuffer)+1);

    if (outBuffer != defaultBuffer) {
        kfree(outBuffer);
    }
    return strlen(outBuffer)+1;
}

int alog_processLogBuffer(LoggerEntry *buf, AndroidLogEntry *entry)
{
    size_t tag_len;

    entry->tv_sec = buf->sec;
    entry->tv_nsec = buf->nsec;
    entry->priority = buf->msg[0];
    entry->pid = buf->pid;
    entry->tid = buf->tid;
    entry->tag = buf->msg + 1;
    tag_len = strlen(entry->tag);
    entry->messageLen = buf->len - tag_len - 3;
    entry->message = entry->tag + tag_len + 1;

    return 0;
}

LoggerEntry *get_next_entry(const LogType log_type, LoggerEntry *now_entry)
{
	char *next_entry;
	LoggerEntry *temp_entry;
	
	if (now_entry == NULL)
		next_entry = alog_ram_console_old_log[log_type];
	else
		next_entry = (char *)now_entry + now_entry->len + sizeof(LoggerEntry);

	temp_entry = (LoggerEntry *)next_entry;	
	if ( ((next_entry+ temp_entry->len - alog_ram_console_old_log[log_type]) > alog_ram_console_old_log_size[log_type]) || (temp_entry->len == 0))
		return NULL;
	else
		return (LoggerEntry *)next_entry;
}

char * allocate_print_buffer(const LogType log_type)
{
	unsigned int entryNum=0;
	char *next_entry;
	LoggerEntry *temp_entry;	
	unsigned int sizeCount=0,allocSize=0;
	int next_entry_len;
	int i;

	next_entry = alog_ram_console_old_log[log_type];
	do
	{
		temp_entry = (LoggerEntry *)next_entry;	
		next_entry_len = temp_entry->len + sizeof(LoggerEntry);
		if ((sizeCount + next_entry_len) > alog_ram_console_old_log_size[log_type])
			break;
		sizeCount += next_entry_len;
		entryNum++;
		allocSize += MAX_PRINT_HEAD_SIZE + temp_entry->len;
#ifdef OUTPUT_TEXT_DEBUG		
		printk(KERN_INFO "[LastAlog] allocate_print_buffer: find <next_entry>:0x%X <entryNum>:%d <allocSize>:%d <sizeCount>:%d ", (unsigned int)next_entry, entryNum, allocSize, sizeCount);
		printk(" [header: ");
		for (i=0 ;i<20; i++)
			printk("%02X ",*(next_entry+i));
		printk("]");
		printk(" [data: ");
		for (i=0 ;i<temp_entry->len; i++)
			printk("%02X ",*(next_entry+20+i));
		printk("]\n");
#endif		
		next_entry += next_entry_len;
	} while (sizeCount < alog_ram_console_old_log_size[log_type]);
	
	return kmalloc(allocSize, GFP_KERNEL);
}

char * filter_and_format_log(const LogType log_type, int * len)
{

	AndroidLogEntry alog_entry;
	LoggerEntry *p_logger_entry;
	char *print_buffer;
	char *print_buffer_pos;
	int i;

	print_buffer=allocate_print_buffer(log_type);
	if (print_buffer == NULL)
	{
		printk(KERN_ERR "[LastAlog] filter_and_format_log allocate print buffer failed\n");	
		*len =0;		
		return NULL;
	}

	print_buffer_pos = print_buffer;
	p_logger_entry = NULL;
	while ((p_logger_entry = get_next_entry( log_type, p_logger_entry)) != NULL)
	{
#ifdef OUTPUT_TEXT_DEBUG	
		printk(KERN_INFO "<p_logger_entry>:0x%X ", (unsigned int)p_logger_entry);
		printk(" [header: ");
		for (i=0 ;i<20; i++)
			printk("%02X ",*((char *)p_logger_entry+i));
		printk("]");
		printk(" [data: ");
		for (i=0 ;i<p_logger_entry->len; i++)
			printk("%02X ",*((char *)p_logger_entry+20+i));
		printk("]\n");
#endif		
		alog_processLogBuffer(p_logger_entry, &alog_entry);
		print_buffer_pos += alog_filterAndPrintLogLine(&g_format, (const AndroidLogEntry *)&alog_entry, print_buffer_pos);
	}

	*len = (unsigned int)print_buffer_pos - (unsigned int)print_buffer;
	return print_buffer;

}
#endif

static void __init alog_ram_console_save_old(const LogType log_type, char *dest)
{
	size_t old_log_size = alog_ram_console_buffer[log_type]->size;
	int reader_pos =  alog_ram_console_buffer[log_type]->reader_pos;
	int start_pos = alog_ram_console_buffer[log_type]->start;

	if (dest == NULL) {
		dest = kmalloc(old_log_size, GFP_KERNEL);
		if (dest == NULL) {
			printk(KERN_ERR "[LastAlog] save_old: failed to allocate buffer\n");
			return;
		}
	}

	// copy old log by sequence
	alog_ram_console_old_log[log_type] = dest;
	
	if (old_log_size < alog_ram_console_buffer_size[log_type])
	{
		memcpy(alog_ram_console_old_log[log_type], alog_ram_console_buffer[log_type]->data, old_log_size);
		alog_ram_console_old_log_size[log_type] = old_log_size;
	}
	else
	{
		memcpy(alog_ram_console_old_log[log_type], &alog_ram_console_buffer[log_type]->data[reader_pos], old_log_size - reader_pos);
		memcpy(alog_ram_console_old_log[log_type] + old_log_size - reader_pos, alog_ram_console_buffer[log_type]->data, start_pos);
		alog_ram_console_old_log_size[log_type] = old_log_size - reader_pos + start_pos;
	}

	printk(KERN_ERR "[LastAlog] save_old: alog_ram_console_old_log[%d]=0x%x alog_ram_console_old_log_size[%d]=%d reader_pos=%d start_pos=%d old_log_size=%d\n", log_type, (unsigned int)alog_ram_console_old_log[log_type], log_type, alog_ram_console_old_log_size[log_type], reader_pos, start_pos, old_log_size);

}

static int __init alog_ram_console_init(const LogType log_type, AlogRamConsoleBuffer *buffer, size_t buffer_size, char *old_buf)
{
	alog_ram_console_buffer[log_type] = buffer;

#if 0
	// init default filter and format
	g_format.format=FORMAT_TIME;
	g_format.global_pri=ANDROID_LOG_DEFAULT;
	g_format.filters=NULL;
#endif

	// set maximum of  usable alog ram console buffer size
	alog_ram_console_buffer_size[log_type] = buffer_size - sizeof(AlogRamConsoleBuffer);
	
	printk(KERN_INFO "[LastAlog] init for log type:%d <alog_ram_console_buffer>:0x%x <alog_ram_console_buffer_size>:%d(0x%x) <buffer->data>:0x%x\n", log_type, (unsigned int)alog_ram_console_buffer[log_type], alog_ram_console_buffer_size[log_type], alog_ram_console_buffer_size[log_type], (unsigned int)buffer->data);

	if (alog_ram_console_buffer_size[log_type] > buffer_size) {
		printk(KERN_ERR "[LastAlog] init for log type:%d buffer %p, invalid size %zu, datasize %zu\n", log_type, buffer, buffer_size, alog_ram_console_buffer_size[log_type]);
		return 0;
	}

	// if alog ram console has correct signature, that means there are logs in ram console
	if (buffer->sig == alog_ram_console_signature[log_type]) 
	{
		if (buffer->size > alog_ram_console_buffer_size[log_type] || buffer->start > buffer->size)
			printk(KERN_INFO "[LastAlog] init for log type:%d found existing invalid buffer, size %d, start %d\n", log_type, buffer->size, buffer->start);
		else 
		{
			printk(KERN_INFO "[LastAlog] init for log type:%d found existing buffer, size %d, start %d\n", log_type, buffer->size, buffer->start);
			alog_ram_console_save_old(log_type, old_buf); // save old log
		}
	} 
	else 
	{
		printk(KERN_INFO "[LastAlog] init for log type:%d no valid data in buffer (sig = 0x%08x)\n", log_type, buffer->sig);
		// init alog_ram_console_old_log[]
		alog_ram_console_old_log[log_type]=NULL;	
		alog_ram_console_old_log_size[log_type]=0;		
	}

	// reset alog ram console
	buffer->sig = alog_ram_console_signature[log_type]; // mark alog ram console signature
	buffer->start = 0; // reset start pointer to 0
	buffer->size = 0; // reset used buffer size to 0
	buffer->reader_pos = 0; // reset reader position
	need_fix_reader[log_type] = 0; // reset fixed reader flag
	next_log_entry[log_type] = buffer->data; // reset next_log_entry to beginning of buffer
	alog_ram_console_sync_time(log_type, SYNC_AFTER); // sync time in beginning
	return 0;
}

static int alog_ram_console_driver_probe(struct platform_device *pdev)
{
	struct resource *res;
	size_t start;
	size_t whole_buffer_size;
	void * whole_buffer;
	int i,ret=0;	

	for (i=0; i<LOG_TYPE_NUM; i++)
	{
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,alog_ram_console_res_name[i]);
		if (res == NULL) 
		{
			printk(KERN_ERR "[LastAlog] driver_probe: invalid resource -%s\n",alog_ram_console_res_name[i]);
			return -ENXIO;
		}
		else
		{
			whole_buffer_size = res->end - res->start + 1;
			start = res->start;
			printk(KERN_INFO "[LastAlog] driver_probe: got physical %s at 0x%zx, size %d(0x%zx)\n", alog_ram_console_res_name[i], start, whole_buffer_size,whole_buffer_size);
			whole_buffer = ioremap(res->start, whole_buffer_size);
			if (whole_buffer == NULL)
			{
				printk(KERN_ERR "[LastAlog] driver_probe: failed to map memory for %s!\n", alog_ram_console_res_name[i]);
				return -ENOMEM;
			}
			ret += alog_ram_console_init(i, (AlogRamConsoleBuffer *)whole_buffer, whole_buffer_size, NULL/* allocate */);
		}
	}

	spin_lock_init(&bufflock);

	return ret;
}

static struct platform_driver alog_ram_console_driver = {
	.probe = alog_ram_console_driver_probe,
	.driver		= {
		.name	= "alog_ram_console",
	},
};

static int __init alog_ram_console_module_init(void)
{
	int err;

	err = platform_driver_register(&alog_ram_console_driver);
	if (err)
		printk(KERN_ERR "[LastAlog] alog_ram_console_module_init ret:%d\n", err);
	
	return err;
}

static ssize_t alog_ram_console_read_old_main(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	LoggerEntry *pLogEntry;

	//printk(KERN_ERR "[LastAlog] alog_ram_console_read_old_main: len:%d offset:%d ", len, (int)pos);

	if (pos >= alog_ram_console_old_log_size[LOG_TYPE_MAIN])
		return 0;

	pLogEntry = (LoggerEntry *)(alog_ram_console_old_log[LOG_TYPE_MAIN] + pos);
	if ( (pos + sizeof(LoggerEntry) + pLogEntry->len) > alog_ram_console_old_log_size[LOG_TYPE_MAIN])
		return 0;
	count = min(len, pLogEntry->len + sizeof(LoggerEntry));
/*
	// following code segment is used for reading whole raw data into user-space code directly
	count = min(len, (size_t)(alog_ram_console_old_log_size[LOG_TYPE_MAIN] - pos));
*/	
	if (copy_to_user(buf, alog_ram_console_old_log[LOG_TYPE_MAIN] + pos, count))
		return -EFAULT;
	
#ifdef OUTPUT_TEXT
	// following code segment is used for converting whole raw data to readable text for user-space code	
	outBuf = filter_and_format_log(LOG_TYPE_MAIN, &count);
	if (copy_to_user(buf, outBuf, count))
		return -EFAULT;
		
	if (outBuf != NULL)
		kfree(outBuf);
#endif

	//printk("count:%d\n", count);

	*offset += count;
	return count;
}

static ssize_t alog_ram_console_read_old_radio(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	LoggerEntry *pLogEntry;

	//printk(KERN_ERR "[LastAlog] alog_ram_console_read_old_radio: len:%d offset:%d ", len, (int)pos);

	if (pos >= alog_ram_console_old_log_size[LOG_TYPE_RADIO])
		return 0;

	pLogEntry = (LoggerEntry *)(alog_ram_console_old_log[LOG_TYPE_RADIO] + pos);
	if ( (pos + sizeof(LoggerEntry) + pLogEntry->len) > alog_ram_console_old_log_size[LOG_TYPE_RADIO])
		return 0;
	count = min(len, pLogEntry->len + sizeof(LoggerEntry));

	if (copy_to_user(buf, alog_ram_console_old_log[LOG_TYPE_RADIO] + pos, count))
		return -EFAULT;
	
	//printk("count:%d\n", count);
	
	*offset += count;
	return count;
}

static ssize_t alog_ram_console_read_old_events(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	LoggerEntry *pLogEntry;

	//printk(KERN_ERR "[LastAlog] alog_ram_console_read_old_events: len:%d offset:%d ", len, (int)pos);

	if (pos >= alog_ram_console_old_log_size[LOG_TYPE_EVENTS])
		return 0;

	pLogEntry = (LoggerEntry *)(alog_ram_console_old_log[LOG_TYPE_EVENTS] + pos);
	if ( (pos + sizeof(LoggerEntry) + pLogEntry->len) > alog_ram_console_old_log_size[LOG_TYPE_EVENTS])
		return 0;
	count = min(len, pLogEntry->len + sizeof(LoggerEntry));

	if (copy_to_user(buf, alog_ram_console_old_log[LOG_TYPE_EVENTS] + pos, count))
		return -EFAULT;

	//printk("count:%d\n", count);

	*offset += count;
	return count;
}

static ssize_t alog_ram_console_read_old_system(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	LoggerEntry *pLogEntry;

	//printk(KERN_ERR "[LastAlog] alog_ram_console_read_old_system: len:%d offset:%d ", len, (int)pos);

	if (pos >= alog_ram_console_old_log_size[LOG_TYPE_SYSTEM])
		return 0;

	pLogEntry = (LoggerEntry *)(alog_ram_console_old_log[LOG_TYPE_SYSTEM] + pos);
	if ( (pos + sizeof(LoggerEntry) + pLogEntry->len) > alog_ram_console_old_log_size[LOG_TYPE_SYSTEM])
		return 0;
	count = min(len, pLogEntry->len + sizeof(LoggerEntry));
/*
	// following code segment is used for reading whole raw data into user-space code directly
	count = min(len, (size_t)(alog_ram_console_old_log_size[LOG_TYPE_SYSTEM] - pos));
*/	
	if (copy_to_user(buf, alog_ram_console_old_log[LOG_TYPE_SYSTEM] + pos, count))
		return -EFAULT;
	
#ifdef OUTPUT_TEXT
	// following code segment is used for converting whole raw data to readable text for user-space code	
	outBuf = filter_and_format_log(LOG_TYPE_SYSTEM, &count);
	if (copy_to_user(buf, outBuf, count))
		return -EFAULT;
		
	if (outBuf != NULL)
		kfree(outBuf);
#endif

	//printk("count:%d\n", count);

	*offset += count;
	return count;
}

static struct file_operations alog_ram_console_file_ops[LOG_TYPE_NUM] = {
	{
	.owner = THIS_MODULE,
	.read = alog_ram_console_read_old_main,
	},
	{
	.owner = THIS_MODULE,
	.read = alog_ram_console_read_old_radio,
	},
	{
	.owner = THIS_MODULE,
	.read = alog_ram_console_read_old_events,
	},
	{
	.owner = THIS_MODULE,
	.read = alog_ram_console_read_old_system,
	}
};

static int __init alog_ram_console_late_init(void)
{
	struct proc_dir_entry *entry;
	int i;

	// create a proc file to read old log for alog ram console
	for (i=0; i<LOG_TYPE_NUM; i++)
		if (alog_ram_console_old_log[i] != NULL)
		{
			entry = create_proc_entry(alog_ram_console_proc_fn[i], S_IFREG | S_IRUGO, NULL);
			if (!entry) 
			{
				printk(KERN_ERR "[LastAlog] late_init: failed to create proc entry - /proc/%s\n", alog_ram_console_proc_fn[i]);
				kfree(alog_ram_console_old_log[i]);
				alog_ram_console_old_log[i] = NULL;
				return -EFAULT;
			}

			entry->proc_fops = &alog_ram_console_file_ops[i];
			entry->size = alog_ram_console_old_log_size[i];
		}
		
	return 0;
}

module_init(alog_ram_console_module_init);
late_initcall(alog_ram_console_late_init);
