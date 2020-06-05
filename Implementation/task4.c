#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "coursework.h"
#include "linkedlist.h"

int iProduced, iConsumed, iConsumerCounter, iTimeCounter;
sem_t sCountItem, sDelayProducer, sGlobalSync, sBufferSync, sConsumerOutSync;
struct element * oProductListHeads[MAX_PRIORITY], * oProductListTails[MAX_PRIORITY];

long int iResponseTime = 0, iTurnAroundTime = 0, iTimeInterval = 0;
double dAverageResponseTime, dAverageTurnAroundTime;


void initialize();
void * consumer(void * iConsumerID);
void * producer(void * iProducerID);
int consumeBuffer(void * iConsumerID);
void produceBuffer(void * iProducerID);
struct process * processJob(int iConsumerId, struct process * pProcess, struct timeval oStartTime, struct timeval oEndTime);


int main(int argc, char * argv[]){
    pthread_t oConThread[NUMBER_OF_CONSUMERS], oProThread;
    int iConsumerID[NUMBER_OF_CONSUMERS], iProducerID = 0, iTempSemaValue;
    
    initialize();

    //Create and join producer thread and all consumer threads
    pthread_create(&oProThread, NULL, producer, &iProducerID);
    for(int iTemp = 0; iTemp < NUMBER_OF_CONSUMERS; iTemp++){
        iConsumerCounter++;
        iConsumerID[iTemp] = iTemp + 1;
        pthread_create(&oConThread[iTemp], NULL, consumer, &iConsumerID[iTemp]);
    }

    for(int iTemp = 0; iTemp < NUMBER_OF_CONSUMERS; iTemp++){
        pthread_join(oConThread[iTemp], NULL);
    }
    pthread_join(oProThread, NULL);

    printf(
        "Average Response Time = %.6f\nAverage Turn Around Time = %.6f\n", 
        dAverageResponseTime / NUMBER_OF_JOBS, 
        dAverageTurnAroundTime / NUMBER_OF_JOBS
    );
   
    sem_destroy(&sDelayProducer);
    sem_destroy(&sCountItem);
    sem_destroy(&sBufferSync);
    sem_destroy(&sGlobalSync);

    return 0;
}

//Initialize all variables, semaphores and the product buffer
void initialize(){
    iProduced = iConsumed = iTimeCounter = 0;
    dAverageResponseTime = dAverageTurnAroundTime = 0.0;
    sem_init(&sDelayProducer, 0, 0);
    sem_init(&sCountItem, 0, 0);
    sem_init(&sGlobalSync, 0, 1);
    sem_init(&sBufferSync, 0, 1);
    sem_init(&sConsumerOutSync, 0, 1);
    for(int iTemp = 0; iTemp < MAX_PRIORITY; iTemp++){
        oProductListHeads[iTemp] = oProductListTails[iTemp] = NULL;
    }

    return;
}

//Comsume next process and update all semaphores
void * consumer(void * iConsumerID){
    //flag == 1 means the process job has been finished running
    //flag == 0 means the process job needs running next time
    int iTemp, flag; 
    sem_wait(&sCountItem);

    while(iConsumed < NUMBER_OF_JOBS){
        sem_wait(&sGlobalSync);
        iTemp = iProduced - iConsumed;
        sem_post(&sGlobalSync);

        flag = consumeBuffer(iConsumerID);

        if(flag){
            sem_wait(&sGlobalSync);
            iConsumed++;
            iTemp = iProduced - iConsumed;
            sem_post(&sGlobalSync);
            if(iTemp == MAX_BUFFER_SIZE - 1){
                sem_post(&sDelayProducer);
            }
        }
        
        if(iConsumed >= NUMBER_OF_JOBS){
            break;
        }

        if(flag){
            sem_wait(&sCountItem);
        }
    }

    sem_wait(&sConsumerOutSync);
    iConsumerCounter--;
    sem_getvalue(&sCountItem, &iTemp);
    if(iConsumerCounter == 0){
        for(;iTemp > 0; iTemp--)
            sem_wait(&sCountItem);
    }else
        sem_post(&sCountItem);
    sem_post(&sConsumerOutSync);

    return NULL;
}

//Produce a new process and update all semaphores
void * producer(void * iProducerID){
    int iTemp;
    while(iProduced < NUMBER_OF_JOBS){

        produceBuffer(iProducerID);

        sem_wait(&sGlobalSync);
        iProduced++;
        iTemp = iProduced - iConsumed;
        sem_post(&sCountItem);
        sem_post(&sGlobalSync);

        if(iTemp == MAX_BUFFER_SIZE)
            sem_wait(&sDelayProducer);
    }

    return NULL;
}

//Select out and consume next process
int consumeBuffer(void * iConsumerID){
    struct element * oProductListTemp;
    struct process * oTempProcess;
    struct timeval oTempStartTime, oTempEndTime;
    int iPriority, flag = 0;

    sem_wait(&sBufferSync);
    for(iPriority = 0; iPriority < MAX_PRIORITY; iPriority++){
        if(oProductListHeads[iPriority] != NULL)
            break;
    }
    oTempProcess = (struct process*)oProductListHeads[iPriority]->pData;
    removeFirst(&oProductListHeads[iPriority], &oProductListTails[iPriority]);
    sem_post(&sBufferSync);

    runJob(oTempProcess, &oTempStartTime, &oTempEndTime);

    //Realize CPU parallel operation
    sem_wait(&sGlobalSync);
    oTempProcess = processJob((*(int *)iConsumerID), oTempProcess, oTempStartTime, oTempEndTime);
    if(oTempProcess == NULL)
        flag = 1; 
    sem_post(&sGlobalSync);

    if(!flag){
        sem_wait(&sBufferSync);
        addLast(oTempProcess, &oProductListHeads[iPriority], &oProductListTails[iPriority]);
        sem_post(&sBufferSync);
    }

    return flag;
}

//Create and print out a new process
void produceBuffer(void * iProducerID){
    struct process * oTempProcess = generateProcess();
    int iPriority = oTempProcess->iPriority;

    sem_wait(&sBufferSync);
    addLast(oTempProcess, &oProductListHeads[iPriority], &oProductListTails[iPriority]);
    printf("Producer %d, Process Id = %d (%s), Priority = %d, Initial Burst Time %d\n", 
        *((int *)iProducerID), 
        oTempProcess->iProcessId,
        oTempProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR",
        oTempProcess->iPriority,
        oTempProcess->iInitialBurstTime); 
    sem_post(&sBufferSync);

    return;
}

//The origin funtion of processJob
struct process * processJob(int iConsumerId, struct process * pProcess, struct timeval oStartTime, struct timeval oEndTime)
{
	int iResponseTime;
	int iTurnAroundTime;
	if(pProcess->iPreviousBurstTime == pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime > 0)
	{
		iResponseTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oStartTime);	
		dAverageResponseTime += iResponseTime;
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2	 ? "FCFS" : "RR",pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime, iResponseTime);
		return pProcess;
	} else if(pProcess->iPreviousBurstTime == pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime == 0)
	{
		iResponseTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oStartTime);	
		dAverageResponseTime += iResponseTime;
		iTurnAroundTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oEndTime);
		dAverageTurnAroundTime += iTurnAroundTime;
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d, Turnaround Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime, iResponseTime, iTurnAroundTime);
		free(pProcess);
		return NULL;
	} else if(pProcess->iPreviousBurstTime != pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime > 0)
	{
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime);
		return pProcess;
	} else if(pProcess->iPreviousBurstTime != pProcess->iInitialBurstTime && pProcess->iRemainingBurstTime == 0)
	{
		iTurnAroundTime = getDifferenceInMilliSeconds(pProcess->oTimeCreated, oEndTime);
		dAverageTurnAroundTime += iTurnAroundTime;
		printf("Consumer %d, Process Id = %d (%s), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Turnaround Time = %d\n", iConsumerId, pProcess->iProcessId, pProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR", pProcess->iPriority, pProcess->iPreviousBurstTime, pProcess->iRemainingBurstTime, iTurnAroundTime);
		free(pProcess);
		return NULL;
	}
}