// ==== monitor.h ====
#ifndef MONITOR_H
#define MONITOR_H

#include <semaphore.h>

// -------------------------------------------------------
// Custom Condition Variable (CV) type.
// Follows signal-and-continue discipline:
//   - signaler stays in the monitor.
//   - signaled thread is moved to the entry queue (entry_sem)
//     and must re-compete to re-enter the monitor.
// -------------------------------------------------------
typedef struct {
    int     count;      // number of threads blocked on this CV
    sem_t   sem;        // semaphore used to suspend waiting threads
} cond;

// -------------------------------------------------------
// CV operations
// -------------------------------------------------------

// Returns number of threads blocked on cv.
int  cv_count(cond *cv);

// Relinquishes monitor lock and suspends calling thread on cv.
// On wake-up the thread re-enters via the entry queue (re-acquires
// the monitor lock) before returning — ensuring signal-and-continue.
void cv_wait(cond *cv);

// Unblocks one thread suspended on cv (if any).
// The signaler keeps the monitor lock (signal-and-continue).
// The unblocked thread is moved to the entry queue.
void cv_signal(cond *cv);

// -------------------------------------------------------
// Monitor functions
// -------------------------------------------------------

// Called once to initialise all monitor state.
void mon_init(void);

// Called by the stylist thread.
// Signals stylistAvailable, then waits on customerAvailable if
// no customer is present.
void mon_checkCustomer(void);

// Called by a customer thread.
// Returns 1 if the customer got into a chair (haircut will follow),
// 0 if the salon was full (customer must go shopping and retry).
int  mon_checkStylist(void);

// Prints internal monitor state (must be called inside the monitor).
void mon_debugPrint(void);

#endif // MONITOR_H
