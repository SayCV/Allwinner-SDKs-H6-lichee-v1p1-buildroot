
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define loginfo(fmt, args...) fprintf(stdout, "car_reverse: "fmt, ##args)
#define logerr(fmt, args...)  fprintf(stderr, "car_reverse: "fmt, ##args)

#endif
