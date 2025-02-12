#ifndef UPTIME_COUNTER_H
#define UPTIME_COUNTER_H

/* size of stack area used by each thread */
#define UPTIME_COUNTER_STACKSIZE	1024

/* scheduling priority used by each thread */
#define UPTIME_COUNTER_PRIORITY		5

void uptime_counter(void);

#endif /* UPTIME_COUNTER_H */
