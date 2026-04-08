#include <stdio.h>
#include <semaphore.h>
#include "monitor.h"

#define CHAIRS 6

static sem_t entry_sem; //monitor lock and entry queue
static cond stylistAvailable;   
static cond customerAvailable;  


static int customer_count  = 0;  
static int stylist_count   = 0;  

static int haircuts_given    = 0;
static int salon_full_times  = 0;  
static int stylist_sleep_times = 0;


static int chairs[CHAIRS];  //1 is seat taken, 0 is empty


static void monitor_enter(void) { sem_wait(&entry_sem); }
static void monitor_exit(void)  { sem_post(&entry_sem); }

int cv_count(cond *cv)
{
    return cv->count;
}


void cv_signal(cond *cv) //Wake a blocked thread 
{
    if (cv->count > 0) {
        cv->count--;
        sem_post(&cv->sem);
    }
}

void cv_wait(cond *cv) //release monitor lock so others can enter, block CV sem
{
    cv->count++;
    monitor_exit();          
    sem_wait(&cv->sem);      
    monitor_enter();         
    printf("-cv wait- Thread resumed after signal — back inside the monitor "
           "(signal-and-continue confirmed).\n");
}

void initialize_monitor(void)
{
    sem_init(&entry_sem, 0, 1);

    stylistAvailable.count  = 0;
    sem_init(&stylistAvailable.sem, 0, 0);

    customerAvailable.count = 0;
    sem_init(&customerAvailable.sem, 0, 0);

    for (int i = 0; i < CHAIRS; i++) chairs[i] = 0;
}

void mon_checkCustomer(void)
{
    monitor_enter();
    stylist_count++;                      
    cv_signal(&stylistAvailable);         
    if (customer_count == 0) { 
        stylist_sleep_times++;
        printf("-stylist- No customers, so sleeping(sleep count: %d).\n",
               stylist_sleep_times);
        cv_wait(&customerAvailable);      
        printf("-stylist- Woke up bc customer arrived.\n");
    }
    customer_count--;
    haircuts_given++;

    for (int i = 0; i < CHAIRS; i++) {
        if (chairs[i] == 1) { chairs[i] = 0; break; }
    }

    printf("Total haircuts: %d\n",haircuts_given);
    monitor_exit();
}

int mon_checkStylist(void)
{
    monitor_enter();
    int status = 0;
    if (customer_count < CHAIRS) {
        customer_count++;
        for (int i = 0; i < CHAIRS; i++) {
            if (chairs[i] == 0) { chairs[i] = 1; break; }
        }
        if (customer_count == CHAIRS) {
            salon_full_times++;
            printf("[CUSTOMER] All %d chairs now full! (full count: %d)\n",
                   CHAIRS, salon_full_times);
        }
        cv_signal(&customerAvailable);    // wake stylist if sleeping
        if (stylist_count == 0) {
            printf("-customer- Stylist busy, will wait");
            printf("-customer- Stylist is now available\n");
        }
        stylist_count--;   // consume stylist's readiness
        status = 1;
    } else {
        salon_full_times++;
        printf("-customer- Salon full (%d waiting)."
               "(full count: %d)\n", customer_count, salon_full_times);
    }
    monitor_exit();
    return status;
}

void mon_debugPrint(void)
{
    monitor_enter();
    printf("| ");
    int occupied = 0;
    for (int i = 0; i < CHAIRS; i++) {
        printf("%d | ", chairs[i]);
        if (chairs[i]) occupied++;
    }
    printf("=> %d\n", occupied);
    printf("Given haircuts = %d\n", haircuts_given);
    printf("Salon full = %d times\n", salon_full_times);
    printf("Salon empty = %d times\n", stylist_sleep_times);
    printf("\n");

    monitor_exit();
}
