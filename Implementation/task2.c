//sSync             synchronises access to the buffer
//sDelayConsumer    ensures the consumer goes to sleep when there are no items available

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#define NUM_OF_JOBS 1000

int iTotalConsumeNumber = 0, iTotalProduceNumber = 0, items;
sem_t sDelayConsumer, sSync;

void * consumer();
void * producer();
void showItems(int items);

int main(int argc, char * argv[]){
    pthread_t oConThread, oProThread;
    int iTempSemaValue;

    sem_init(&sDelayConsumer, 0, 0);
    sem_init(&sSync, 0, 1);
    pthread_create(&oConThread, NULL, (void *)consumer, NULL);
    pthread_create(&oProThread, NULL, (void *)producer, NULL);

    pthread_join(oProThread, NULL);
    pthread_join(oConThread, NULL);

    sem_getvalue(&sSync, &iTempSemaValue);
    printf("sSync = %d, ", iTempSemaValue);
    sem_getvalue(&sDelayConsumer, &iTempSemaValue);
    printf("sDelayConsumer = %d\n", iTempSemaValue);

    sem_destroy(&sDelayConsumer);
    sem_destroy(&sSync);

    return 0;
}

void * consumer(){
    int temp;
    sem_wait(&sDelayConsumer);
    while(1){
        sem_wait(&sSync);

        temp = --items;
        iTotalConsumeNumber++;
        printf("Consumer, Produced = %d, Consumed = %d:", iTotalProduceNumber, iTotalConsumeNumber);
        showItems(items);

        sem_post(&sSync);
        if(iTotalConsumeNumber >= NUM_OF_JOBS)
            break;
        if(temp == 0){
            sem_wait(&sDelayConsumer);
        }
    }

    return NULL;
}

void * producer(){
    while(iTotalProduceNumber < NUM_OF_JOBS){
        sem_wait(&sSync);

        items++;
        iTotalProduceNumber++;
        printf("Producer, Produced = %d, Consumed = %d:", iTotalProduceNumber, iTotalConsumeNumber);
        showItems(items);

        if(items == 1){
            sem_post(&sDelayConsumer);
        }
        sem_post(&sSync);
    }

    return NULL;
}

//A visualisation function that displays the exact number of elements currently in the buffer
void showItems(int items){
    for(int iTemp = 0; iTemp < items; iTemp++){
        printf("*");
    }
    printf("\n");
    return;
}