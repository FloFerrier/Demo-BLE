#ifndef RTC_H
#define RTC_H

#include <time.h>

void rtc_setup(void);
void rtc_calendar_set(struct tm time);
struct tm rtc_calendar_get(void);

#endif /* RTC_H */