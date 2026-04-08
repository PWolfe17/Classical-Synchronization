#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "monitor.h"

#define NUM_CUSTOMERS 75
#define DELAY         500000  

void *stylist_thread(void *arg);
void *customer_thread(void *arg);

int main(void)
{
    pthread_t stylist_tid;
    pthread_t customer_tids[NUM_CUSTOMERS];
    int ids[NUM_CUSTOMERS];

    initialize_monitor();

    printf("Stylist Monitors\n");
    printf("Customers: %d and Chairs: %d\n\n", NUM_CUSTOMERS, 6);

    pthread_create(&stylist_tid, NULL, stylist_thread, NULL);

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        ids[i] = i + 1;
        pthread_create(&customer_tids[i], NULL, customer_thread, &ids[i]);
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++)
        pthread_join(customer_tids[i], NULL);

    pthread_cancel(stylist_tid);
    pthread_join(stylist_tid, NULL);

    printf("\n%d Customers have recieved haircuts. All done\n",
           NUM_CUSTOMERS);
    return 0;
}


void *stylist_thread(void *arg)
{
    (void)arg;
    int j;

    while (1) {
        mon_debugPrint();
        mon_checkCustomer();
        for (j = 0; j < DELAY; j++);           
    }
    return NULL;
}

void *customer_thread(void *arg)
{
    int id = *(int *)arg;
    int j;

    while (1) {
        mon_debugPrint();
        printf("-customer%d- trying to get in\n", id);

        if (mon_checkStylist()) {
            printf("-customer%d- done\n", id);
            break;
        }

        printf("-customer-%d- customer will enter \n", id);
        for (j = 0; j < DELAY; j++);
    }
    return NULL;
}
