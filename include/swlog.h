#ifndef SWLOG_H
#define SWLOG_H

// 日志已全部关闭: sw_log 为空操作, 不再写 sdmc:/ai5.log。
static inline void sw_log(const char *fmt, ...) { (void)fmt; }

#endif
