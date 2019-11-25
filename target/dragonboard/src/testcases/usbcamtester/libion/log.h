
#ifndef __LOG_H__
#define __LOG_H__

#define log_error(fmt, args...)		printf("[ION] "fmt, ##args)
#define log_warning(fmt, args...)	printf("[ION] "fmt, ##args)

#ifdef __DEBUG__
#define log_debug(fmt, args...)		printf("[ION] "fmt, ##args)
#else
#define log_debug(fmt, args...)
#endif

#endif

