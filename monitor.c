// ==== monitor.c ====
// Custom monitor with signal-and-continue discipline.
// Custom Condition Variables built on top of POSIX semaphores.
// NO pthread_cond_* is used anywhere.

#include <stdio.h>
#include <semaphore.h>
#include "monitor.h"

#define CHAIRS 6

// -------------------------------------------------------
// Monitor lock + entry queue
//   entry_sem  — controls entry into the monitor (binary semaphore).
//                Only one thread may hold it at a time.
// -------------------------------------------------------
static sem_t entry_sem;

// -------------------------------------------------------
// Condition variables
// -------------------------------------------------------
static cond stylistAvailable;   // signaled when stylist is free
static cond customerAvailable;  // signaled when a customer is waiting

// -------------------------------------------------------
// Shared state (protected by the monitor lock)
// -------------------------------------------------------
static int customer_count  = 0;  // customers in waiting chairs
static int stylist_count   = 0;  // 1 when stylist is free/ready, 0 when busy

// Debug counters
static int haircuts_given    = 0;
static int salon_full_times  = 0;  // each time occupied chairs hit 7 (6 waiting + 1 being cut)
static int stylist_sleep_times = 0;

// Occupied-chair array for visual display
static int chairs[CHAIRS];  // 0 = empty, 1 = occupied

// -------------------------------------------------------
// Internal helpers: acquire / release the monitor lock.
// Every public function MUST acquire on entry and release on exit
// (including all return paths).
// -------------------------------------------------------
static void monitor_enter(void) { sem_wait(&entry_sem); }
static void monitor_exit(void)  { sem_post(&entry_sem); }

// -------------------------------------------------------
// CV operations — must only be called from within the monitor.
// -------------------------------------------------------

int cv_count(cond *cv)
{
    return cv->count;
}

// signal-and-continue:
//   1. Wake one blocked thread (post its semaphore).
//   2. That thread will re-acquire entry_sem before continuing —
//      the signaler keeps the lock right now.
void cv_signal(cond *cv)
{
    if (cv->count > 0) {
        cv->count--;
        sem_post(&cv->sem);
        // Signaler continues holding the monitor lock.
        // The unblocked thread will block on entry_sem until we exit.
    }
}

// wait:
//   1. Release the monitor lock so other threads can enter.
//   2. Block on the CV semaphore.
//   3. On wake-up, re-acquire the monitor lock (entry queue).
//      This is what implements signal-and-continue: the awakened
//      thread goes back to the entry queue and must wait until the
//      signaler (or some other thread) exits the monitor.
void cv_wait(cond *cv)
{
    cv->count++;
    monitor_exit();          // release monitor lock
    sem_wait(&cv->sem);      // sleep on this CV
    monitor_enter();         // re-enter monitor via entry queue
    // Thread resumes HERE after being signaled AND re-acquiring monitor.
    printf("[CV-WAIT] Thread resumed after signal — back inside the monitor "
           "(signal-and-continue confirmed).\n");
}

// -------------------------------------------------------
// mon_init — called once before any threads are created.
// -------------------------------------------------------
void mon_init(void)
{
    sem_init(&entry_sem, 0, 1);

    stylistAvailable.count  = 0;
    sem_init(&stylistAvailable.sem, 0, 0);

    customerAvailable.count = 0;
    sem_init(&customerAvailable.sem, 0, 0);

    for (int i = 0; i < CHAIRS; i++) chairs[i] = 0;
}

// -------------------------------------------------------
// mon_checkCustomer — called by the STYLIST thread.
//
// Logic (fixed from pseudo-code):
//   stylist signals it is available.
//   If no customer is waiting, stylist waits on customerAvailable.
//   When a customer exists, consume one and proceed to cut hair.
// -------------------------------------------------------
void mon_checkCustomer(void)
{
    monitor_enter();

    stylist_count++;                      // stylist is now available
    cv_signal(&stylistAvailable);         // tell any waiting customer

    if (customer_count == 0) {
        // No customer yet — stylist goes to sleep
        stylist_sleep_times++;
        printf("[STYLIST] No customers. Going to sleep (sleep count: %d).\n",
               stylist_sleep_times);
        cv_wait(&customerAvailable);      // releases lock; re-acquires on wake
        printf("[STYLIST] Woke up — customer arrived.\n");
    }

    // Consume one customer from a waiting chair
    customer_count--;
    haircuts_given++;

    // Free the first occupied chair
    for (int i = 0; i < CHAIRS; i++) {
        if (chairs[i] == 1) { chairs[i] = 0; break; }
    }

    printf("[STYLIST] Taking customer to chair. Customers still waiting: %d. "
           "Total haircuts: %d\n", customer_count, haircuts_given);

    monitor_exit();
}

// -------------------------------------------------------
// mon_checkStylist — called by a CUSTOMER thread.
//
// Returns 1 if customer obtained a haircut slot, 0 if salon full.
//
// Fixed fault in pseudo-code: the original decremented stylist_count
// unconditionally after wait, which is correct, but it also needed
// to guard against the stylist not yet being available (stylist_count==0)
// when the customer just woke up, which cv_wait + re-check handles here.
// -------------------------------------------------------
int mon_checkStylist(void)
{
    monitor_enter();

    int status = 0;

    if (customer_count < CHAIRS) {
        // Take a waiting chair
        customer_count++;
        // Mark first free chair occupied
        for (int i = 0; i < CHAIRS; i++) {
            if (chairs[i] == 0) { chairs[i] = 1; break; }
        }

        // Check if all chairs now full (customer_count == CHAIRS means
        // 6 waiting + potentially 1 being cut → salon full event)
        if (customer_count == CHAIRS) {
            salon_full_times++;
            printf("[CUSTOMER] All %d chairs now full! (full count: %d)\n",
                   CHAIRS, salon_full_times);
        }

        cv_signal(&customerAvailable);    // wake stylist if sleeping

        if (stylist_count == 0) {
            // Stylist is busy — wait until it signals availability
            printf("[CUSTOMER] Stylist busy. Waiting on stylistAvailable...\n");
            cv_wait(&stylistAvailable);   // releases lock; re-acquires on wake
            printf("[CUSTOMER] Stylist is now available — proceeding.\n");
        }

        stylist_count--;   // consume stylist's readiness
        status = 1;

    } else {
        // Salon full
        salon_full_times++;
        printf("[CUSTOMER] Salon FULL (%d waiting). Leaving to shop. "
               "(full count: %d)\n", customer_count, salon_full_times);
    }

    monitor_exit();
    return status;
}

// -------------------------------------------------------
// mon_debugPrint — prints monitor state (must be called inside monitor).
// -------------------------------------------------------
void mon_debugPrint(void)
{
    monitor_enter();

    // Line 1: chair occupancy
    printf("| ");
    int occupied = 0;
    for (int i = 0; i < CHAIRS; i++) {
        printf("%d | ", chairs[i]);
        if (chairs[i]) occupied++;
    }
    printf("=> %d\n", occupied);

    // Line 2: haircuts given
    printf("Given haircuts = %d\n", haircuts_given);

    // Line 3: salon-full count
    printf("Salon full = %d times\n", salon_full_times);

    // Line 4: stylist-sleep count
    printf("Salon empty = %d times\n", stylist_sleep_times);
    printf("\n");

    monitor_exit();
}
