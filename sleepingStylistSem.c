// ==== sleepingStylistSem.c ====
// Sleeping Stylist problem solved using POSIX semaphores.
// 75 customer threads, 1 stylist thread, 6 waiting chairs.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define CHAIRS      6
#define NUM_CUSTOMERS 75
#define DELAY       500000  // Adjust to get steady stream of customers

// ---- Semaphores and shared state ----
sem_t mutex;       // protects 'waiting' counter
sem_t customers;   // signals stylist that a customer is ready
sem_t stylist;     // signals customer that stylist is ready

int waiting = 0;           // customers currently in waiting chairs
int haircuts_given = 0;    // total completed haircuts
int salon_full_count = 0;  // times a customer found the salon full
int stylist_sleep_count = 0; // times stylist went to sleep

// ---- Forward declarations ----
void *stylist_thread(void *arg);
void *customer_thread(void *arg);

// -------------------------------------------------------
int main(void)
{
    pthread_t stylist_tid;
    pthread_t customer_tids[NUM_CUSTOMERS];
    int ids[NUM_CUSTOMERS];

    sem_init(&mutex,     0, 1);
    sem_init(&customers, 0, 0);
    sem_init(&stylist,   0, 0);

    printf("=== Sleeping Stylist (Semaphores) ===\n");
    printf("Customers: %d  |  Chairs: %d\n\n", NUM_CUSTOMERS, CHAIRS);

    pthread_create(&stylist_tid, NULL, stylist_thread, NULL);

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        ids[i] = i + 1;
        pthread_create(&customer_tids[i], NULL, customer_thread, &ids[i]);
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++)
        pthread_join(customer_tids[i], NULL);

    // All customers done — cancel stylist thread
    pthread_cancel(stylist_tid);
    pthread_join(stylist_tid, NULL);

    printf("\n=== All %d customers have received haircuts. Done. ===\n",
           NUM_CUSTOMERS);

    sem_destroy(&mutex);
    sem_destroy(&customers);
    sem_destroy(&stylist);
    return 0;
}

// -------------------------------------------------------
// Stylist thread: sleeps when no customers; wakes up to cut hair.
void *stylist_thread(void *arg)
{
    (void)arg;
    int j;

    while (1) {
        // Block until a customer signals
        sem_wait(&customers);       // wait for a customer

        sem_wait(&mutex);
        waiting--;
        printf("[STYLIST] Taking a customer. Waiting now: %d\n", waiting);
        if (waiting == 0) {
            stylist_sleep_count++;
            printf("[STYLIST] No more customers waiting — will sleep after this haircut.\n");
        }
        sem_post(&stylist);         // signal customer: stylist ready
        sem_post(&mutex);

        // Cut hair
        for (j = 0; j < DELAY; j++);

        sem_wait(&mutex);
        haircuts_given++;
        printf("[STYLIST] Haircut done. Total haircuts given: %d\n", haircuts_given);
        sem_post(&mutex);
    }
    return NULL;
}

// -------------------------------------------------------
// Customer thread: tries to get a haircut; leaves to shop if salon full.
void *customer_thread(void *arg)
{
    int id = *(int *)arg;
    int j;

    while (1) {
        sem_wait(&mutex);

        if (waiting < CHAIRS) {
            // Seat available — take a chair
            waiting++;
            printf("[CUSTOMER %d] Entered salon. Waiting: %d/%d\n",
                   id, waiting, CHAIRS);
            sem_post(&customers);   // wake up stylist
            sem_post(&mutex);

            sem_wait(&stylist);     // wait until stylist is ready for me
            printf("[CUSTOMER %d] Getting haircut!\n", id);
            break;                  // haircut obtained — exit loop

        } else {
            // Salon full — leave and come back later
            salon_full_count++;
            printf("[CUSTOMER %d] Salon FULL (%d waiting). Going shopping. "
                   "(Full count: %d)\n", id, waiting, salon_full_count);
            sem_post(&mutex);

            for (j = 0; j < DELAY; j++);   // go shopping
        }
    }
    return NULL;
}
