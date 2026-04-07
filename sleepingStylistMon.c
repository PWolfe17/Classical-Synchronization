// ==== sleepingStylistMon.c ====
// Sleeping Stylist solved with a custom monitor (signal-and-continue).
// Uses monitor.c / monitor.h for all synchronisation.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "monitor.h"

#define NUM_CUSTOMERS 75
#define DELAY         500000  // Adjust for steady customer stream

// ---- Forward declarations ----
void *stylist_thread(void *arg);
void *customer_thread(void *arg);

// -------------------------------------------------------
int main(void)
{
    pthread_t stylist_tid;
    pthread_t customer_tids[NUM_CUSTOMERS];
    int ids[NUM_CUSTOMERS];

    mon_init();

    printf("=== Sleeping Stylist (Monitor / signal-and-continue) ===\n");
    printf("Customers: %d  |  Chairs: %d\n\n", NUM_CUSTOMERS, 6);

    pthread_create(&stylist_tid, NULL, stylist_thread, NULL);

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        ids[i] = i + 1;
        pthread_create(&customer_tids[i], NULL, customer_thread, &ids[i]);
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++)
        pthread_join(customer_tids[i], NULL);

    pthread_cancel(stylist_tid);
    pthread_join(stylist_tid, NULL);

    printf("\n=== All %d customers have received haircuts. Done. ===\n",
           NUM_CUSTOMERS);
    return 0;
}

// -------------------------------------------------------
void *stylist_thread(void *arg)
{
    (void)arg;
    int j;

    while (1) {
        mon_debugPrint();
        mon_checkCustomer();                    // sleep if no customer; take next
        for (j = 0; j < DELAY; j++);           // cut hair
    }
    return NULL;
}

// -------------------------------------------------------
void *customer_thread(void *arg)
{
    int id = *(int *)arg;
    int j;

    while (1) {
        mon_debugPrint();
        printf("[CUSTOMER %d] Trying to enter salon...\n", id);

        if (mon_checkStylist()) {
            // Got a seat and the stylist took us
            printf("[CUSTOMER %d] Haircut complete!\n", id);
            break;
        }

        // Salon was full — go shopping
        printf("[CUSTOMER %d] Going shopping...\n", id);
        for (j = 0; j < DELAY; j++);
    }
    return NULL;
}
