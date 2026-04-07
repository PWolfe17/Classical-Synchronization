#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define CHAIRS      6
#define NUM_CUSTOMERS 75
#define DELAY       500000  


sem_t mutex;       
sem_t customers;   
sem_t stylist;     

int waiting = 0;           
int haircuts_given = 0;    
int salon_full_count = 0;  
int stylist_sleep_count = 0; 


void *stylist_thread(void *arg);
void *customer_thread(void *arg);

int main(void)
{
    pthread_t stylist_tid;
    pthread_t customer_tids[NUM_CUSTOMERS];
    int ids[NUM_CUSTOMERS];

    sem_init(&mutex,     0, 1);
    sem_init(&customers, 0, 0);
    sem_init(&stylist,   0, 0);

    printf("Stylist Semaphores\n");
    printf("Customers: %d and Chairs: %d\n\n", NUM_CUSTOMERS, CHAIRS);

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

    printf("\n%d customers have gotten their hair cut. All done\n",
           NUM_CUSTOMERS);

    sem_destroy(&mutex);
    sem_destroy(&customers);
    sem_destroy(&stylist);
    return 0;
}

void *stylist_thread(void *arg)
{
    (void)arg;
    int j;

    while (1) {
        sem_wait(&customers);  // wait for a customer

        sem_wait(&mutex);
        waiting--;
        printf("-stylist- is getting a customer waiting now: %d\n", waiting);
        if (waiting == 0) {
            stylist_sleep_count++;
            printf("-stylist- No more customers waiting\n");
        }
        sem_post(&stylist);        
        sem_post(&mutex);
        for (j = 0; j < DELAY; j++);

        sem_wait(&mutex);
        haircuts_given++;
        printf("-stylist- Haircut done w/ total haircuts given: %d\n", haircuts_given);
        sem_post(&mutex);
    }
    return NULL;
}

void *customer_thread(void *arg)
{
    int id = *(int *)arg;
    int j;

    while (1) {
        sem_wait(&mutex);

        if (waiting < CHAIRS) {
            waiting++;
            printf("-customer%d- in the salon now waiting: %d/%d\n",
                   id, waiting, CHAIRS);
            sem_post(&customers);   // get stylist ready
            sem_post(&mutex);

            sem_wait(&stylist);     
            printf("-customer%d- is getting a haircut\n", id);
            break;                 

        } else {
            salon_full_count++;
            printf("-customer %d- salon is full (%d waiting)"
                   "(Full count: %d)\n", id, waiting, salon_full_count);
            sem_post(&mutex);

            for (j = 0; j < DELAY; j++);
        }
    }
    return NULL;
}
