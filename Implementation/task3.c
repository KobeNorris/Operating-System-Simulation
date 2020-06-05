//sSync                 used to enforce mutual exclusion for the buffer
//sDelayProducer        used to make the producer go to sleep
//sCountItem            used to block the consumer
//iBufferSizeCounter    keeps track of the number of full buffers

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "coursework.h"
#include "linkedlist.h"

int iBufferSizeCounter, iProduced, iConsumed;
sem_t sCountItem, sDelayProducer, sSync;
struct element * oProductListHead, * oProductListTail;

void * consumer(void * iConsumerID);
void * producer(void * iProducerID);
void consumeBuffer();
void produceBuffer();
char * generateStar();
void showItems(struct element * oProductListHead);

int main(int argc, char * argv[]){
    pthread_t oConThread, oProThread;
    int iConsumerID = 1, iProducerID = 1, iTempSemaValue;

    iBufferSizeCounter = iProduced = iConsumed = 0;
    oProductListHead = NULL;
    sem_init(&sDelayProducer, 0, 0);
    sem_init(&sCountItem, 0, 0);
    sem_init(&sSync, 0, 1);

    pthread_create(&oProThread, NULL, producer, &iProducerID);
    pthread_create(&oConThread, NULL, consumer, &iConsumerID);

    pthread_join(oConThread, NULL);
    pthread_join(oProThread, NULL);

    sem_getvalue(&sSync, &iTempSemaValue);
    printf("sSync = %d, ", iTempSemaValue);
    sem_getvalue(&sDelayProducer, &iTempSemaValue);
    printf("sDelayProducer = %d, ", iTempSemaValue);
    sem_getvalue(&sCountItem, &iTempSemaValue);
    printf("sCountItem = %d\n", iTempSemaValue);
    
    sem_destroy(&sDelayProducer);
    sem_destroy(&sCountItem);
    sem_destroy(&sSync);

    return 0;
}

void * consumer(void * iConsumerID){
    int iTemp;
    while(iConsumed < NUMBER_OF_JOBS){
        sem_wait(&sCountItem);

        sem_wait(&sSync);
        iTemp = iBufferSizeCounter--;
        consumeBuffer(iConsumerID);
        sem_post(&sSync);

        if(iTemp == MAX_BUFFER_SIZE)
            sem_post(&sDelayProducer);
    }

    return NULL;
}

void * producer(void * iProducerID){
    int iTemp;
    while(iProduced < NUMBER_OF_JOBS){

        sem_wait(&sSync);
        sem_post(&sCountItem);
        iTemp = ++iBufferSizeCounter;
        produceBuffer(iProducerID);
        sem_post(&sSync);

        if(iTemp == MAX_BUFFER_SIZE)
            sem_wait(&sDelayProducer);
    }

    return NULL;
}

void consumeBuffer(void * iConsumerID){
    struct element * oProductListTemp;

    iConsumed++;
    printf("Consumer %d, Produced = %d, Consumed = %d: ", 
    *((int *)iConsumerID), 
    iProduced, 
    iConsumed);
    free(removeFirst(&oProductListHead, &oProductListTail));
    showItems(oProductListHead);

    return;
}

void produceBuffer(void * iProducerID){
    struct element * oProductListTemp;

    iProduced++;
    printf("Producer %d, Produced = %d, Consumed = %d: ", 
    *((int *)iProducerID), 
    iProduced, 
    iConsumed);
    addLast(generateStar(), &oProductListHead, &oProductListTail);
    showItems(oProductListHead);

    return;
}

char * generateStar(){
    char * cStar = (char *) malloc (sizeof(char));
    * cStar = '*';

    return cStar;
}

//A visualisation function that displays the exact number of elements currently in the buffer
void showItems(struct element * oProductListHead){
    struct element * oProductListTemp;
    
    oProductListTemp = oProductListHead;
    while(oProductListTemp != NULL){
        printf("%c", *((char *)oProductListTemp->pData));
        oProductListTemp = oProductListTemp->pNext;
    }
    printf("\n");

    return;
}