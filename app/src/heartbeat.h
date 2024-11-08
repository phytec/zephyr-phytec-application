#ifndef HEARTBEAT_H
#define HEARTBEAT_H

/* size of stack area used by each thread */
#define HEARTBEAT_STACKSIZE	1024

/* scheduling priority used by each thread */
#define HEARTBEAT_PRIORITY	5

void heartbeat(void);

#endif /* HEARTBEAT_H */
