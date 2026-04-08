#ifndef MONITOR_H
#define MONITOR_H
#include <semaphore.h>

typedef struct {
    int     count;      // number of threads blocked on this CV
    sem_t   sem;        // semaphore used to suspend waiting threads
} cond;


// CV operations
void cv_signal(cond *cv);
int  cv_count(cond *cv);
void cv_wait(cond *cv);

//Monitor fucncs
void initialize_monitor(void);
void mon_debugPrint(void);
void mon_checkCustomer(void);
int  mon_checkStylist(void);


#endif 
