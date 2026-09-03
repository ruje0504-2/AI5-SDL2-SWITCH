#ifndef SWLOG_H
#define SWLOG_H

// Logging is fully disabled: sw_log is a no-op and no longer writes sdmc:/ai5.log.
static inline void sw_log(const char *fmt, ...) { (void)fmt; }

#endif
