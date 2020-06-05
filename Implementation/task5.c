#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "coursework.h"
#include "linkedlist.h"
#include <unistd.h>

int iItemsProduced, iItemsConsumed, iConsumerCounter;
sem_t sCountItem, sDelayProducer, sGlobalSync, sBufferSync, sConsumerOutSync, sProcessListSync;
struct element * oProductListHeads[MAX_PRIORITY], * oProductListTails[MAX_PRIORITY], * oProcessListHead, * oProcessListTail;

long int iResponseTime = 0, iTurnAroundTime = 0;
double dAverageResponseTime, dAverageTurnAroundTime;

void initialize();
void * consumer(void * iConsumerID);
void * producer(void * iProducerID);
int consumeBuffer(void * iConsumerID);
void produceBuffer(void * iProducerID);
void increaseProcessList(struct process * oTargetProcess);
void decreaseProcessList(struct process * oTargetProcess);
void checkPreemptive(struct process * oNewProcess);
void * booster();
struct process * processJob(int iConsumerId, struct process * pProcess, struct timeval oStartTime, struct timeval oEndTime);

int main(int argc, char * argv[]){
    pthread_t oConThread[NUMBER_OF_CONSUMERS], oProThread, oBoostThread;
    int iConsumerID[NUMBER_OF_CONSUMERS], iProducerID = 0, iTempSemaValue;

    initialize();

    //Create and join producer thread, a booster thread and all consumer threads
    pthread_create(&oProThread, NULL, producer, &iProducerID);
    pthread_create(&oBoostThread, NULL, booster, NULL);  
    for(int iTemp = 0; iTemp < NUMBER_OF_CONSUMERS; iTemp++){
        iConsumerCounter++;
        iConsumerID[iTemp] = iTemp;
        pthread_create(&oConThread[iTemp], NULL, consumer, &iConsumerID[iTemp]);
    }

    for(int iTemp = 0; iTemp < NUMBER_OF_CONSUMERS; iTemp++){
        pthread_join(oConThread[iTemp], NULL);
    }
    pthread_join(oBoostThread, NULL);
    pthread_join(oProThread, NULL);

    printf(
        "Average response time = %.6f\nAverage turn around time = %.6f\n", 
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
    iItemsProduced = iItemsConsumed = 0;
    dAverageResponseTime = dAverageTurnAroundTime = 0.0;
    sem_init(&sDelayProducer, 0, 0);
    sem_init(&sCountItem, 0, 0);
    sem_init(&sGlobalSync, 0, 1);
    sem_init(&sBufferSync, 0, 1);
    sem_init(&sConsumerOutSync, 0, 1);
    sem_init(&sProcessListSync, 0, 1);
    oProcessListHead = NULL;
    oProcessListTail = NULL;
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

    while(iItemsConsumed < NUMBER_OF_JOBS){
        sem_wait(&sGlobalSync);
        iTemp = iItemsProduced - iItemsConsumed;
        sem_post(&sGlobalSync);

        flag = consumeBuffer(iConsumerID);

        if(flag){
            sem_wait(&sGlobalSync);
            iItemsConsumed++;
            iTemp = iItemsProduced - iItemsConsumed;
            sem_post(&sGlobalSync);
            if(iTemp == MAX_BUFFER_SIZE - 1){
                sem_post(&sDelayProducer);
            }
        }
        
        if(iItemsConsumed >= NUMBER_OF_JOBS){
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
    while(iItemsProduced < NUMBER_OF_JOBS){

        produceBuffer(iProducerID);

        sem_wait(&sGlobalSync);
        iItemsProduced++;
        iTemp = iItemsProduced - iItemsConsumed;
        sem_post(&sCountItem);
        sem_post(&sGlobalSync);

        if(iTemp == MAX_BUFFER_SIZE)
            sem_wait(&sDelayProducer);
    }

    return NULL;
}

//Select out and consume next process, 
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

    sem_wait(&sProcessListSync);
    increaseProcessList(oTempProcess);
    sem_post(&sProcessListSync);

    runJob(oTempProcess, &oTempStartTime, &oTempEndTime);

    sem_wait(&sProcessListSync);
    decreaseProcessList(oTempProcess);
    sem_post(&sProcessListSync);

    sem_wait(&sGlobalSync);
    oTempProcess = processJob((*(int *)iConsumerID), oTempProcess, oTempStartTime, oTempEndTime);
    if(oTempProcess == NULL)
        flag = 1;
    sem_post(&sGlobalSync);

    //If the process is not finished
    //      1. FCFS:    Add to the head of the queue
    //      2. RR:      Add to the rear of the queue
    if(!flag){
        if(oTempProcess->iPriority < MAX_PRIORITY / 2){
            sem_wait(&sBufferSync);
            addFirst(oTempProcess, &oProductListHeads[iPriority], &oProductListTails[iPriority]);
            sem_post(&sBufferSync);
        }else{
            sem_wait(&sBufferSync);
            addLast(oTempProcess, &oProductListHeads[iPriority], &oProductListTails[iPriority]);
            sem_post(&sBufferSync);
        }
    }

    return flag;
}

//Put new process into the link list of FCFS jobs in priority order
void increaseProcessList(struct process * oTargetProcess){
    struct element * oTempListNode, * oPrevListNode;

    if(oProcessListHead == NULL){
        addFirst(oTargetProcess, &oProcessListHead, &oProcessListTail);
    }else if(((struct process *)oProcessListHead->pData)->iPriority < oTargetProcess->iPriority){
        addFirst(oTargetProcess, &oProcessListHead, &oProcessListTail);
    }else{
        oPrevListNode = oProcessListHead;
        oTempListNode = oProcessListHead->pNext;
        while(oTempListNode != NULL && ((struct process *)oTempListNode->pData)->iPriority >= oTargetProcess->iPriority){
            oPrevListNode = oPrevListNode->pNext;
            oTempListNode = oTempListNode->pNext;
        }

        if(oTempListNode == NULL){
            addLast(oTargetProcess, &oProcessListHead, &oProcessListTail);
        }
        else{
            struct element * pNewElement = (struct element *) malloc (sizeof(struct element));
            oPrevListNode->pNext = pNewElement;
            pNewElement->pData = oTargetProcess;
            pNewElement->pNext = oTempListNode;
        }
    }
}

//Take completed process out of the link list of FCFS jobs and ensure
//the rest processes are still in priority order
void decreaseProcessList(struct process * oTargetProcess){
    struct element * oTempListNode, * oPrevListNode;

    oPrevListNode = NULL;
    oTempListNode = oProcessListHead;
    while(oTempListNode != NULL){
        if(((struct process *)oTempListNode->pData)->iProcessId == oTargetProcess->iProcessId){
            if(oPrevListNode == NULL){
                oProcessListHead = oTempListNode->pNext;
                free(oTempListNode);
            }else{
                oPrevListNode->pNext = oTempListNode->pNext;
                free(oTempListNode);
                if(oPrevListNode->pNext == NULL){
                    oProcessListTail = oPrevListNode;
                }
            }

            return;
        }
        oPrevListNode = oTempListNode;
        oTempListNode = oTempListNode->pNext;
    }

    return;
}

//Create and print out a new process. If the job is FCFS, check whether it should be preempted
void produceBuffer(void * iProducerID){
    struct process * oNewProcess = generateProcess();
    int iPriority = oNewProcess->iPriority;

    sem_wait(&sBufferSync);
    addLast(oNewProcess, &oProductListHeads[iPriority], &oProductListTails[iPriority]);
    printf("Producer %d, Process Id = %d (%s), Priority = %d, Initial Burst Time %d\n", 
        *((int *)iProducerID), 
        oNewProcess->iProcessId,
        oNewProcess->iPriority < MAX_PRIORITY / 2 ? "FCFS" : "RR",
        oNewProcess->iPriority,
        oNewProcess->iInitialBurstTime);

    if(oNewProcess->iPriority < MAX_PRIORITY/2){
        sem_wait(&sProcessListSync);
        checkPreemptive(oNewProcess);
        sem_post(&sProcessListSync);
    }
    sem_post(&sBufferSync);

    return;
}

//Check whether target process's priority is high enough to be Pre-empted
void checkPreemptive(struct process * oNewProcess){
    if(oProcessListHead == NULL){
        return;
    }else if(((struct process *)oProcessListHead->pData)->iPriority > oNewProcess->iPriority){
        preemptJob((struct process *)oProcessListHead->pData);
        printf("Pre-empted job: Pre-empted Process Id = %d, Pre-empted Priority %d, New Process Id %d, New priority %d\n"
        , ((struct process *)oProcessListHead->pData)->iProcessId
        , ((struct process *)oProcessListHead->pData)->iPriority
        , oNewProcess->iProcessId
        , oNewProcess->iPriority);
    }

    return;
}

//This function is opertate to chcek the entire process buffer space to ensure all processes without
//highest priority (MAX_PRIORITY / 2) could operate at lease once every 100 ms
void * booster(){
    int iPriority, iMaxPriority = MAX_PRIORITY / 2;
    struct process * oTempProcess;
    struct timeval now;

    while(iItemsConsumed < MAX_NUMBER_OF_JOBS){
        sem_wait(&sBufferSync);
        for(iPriority = iMaxPriority + 1; iPriority < MAX_PRIORITY; iPriority++){
            if(oProductListHeads[iPriority] != NULL){
                oTempProcess = (struct process *)(oProductListHeads[iPriority]->pData);
                gettimeofday(&now, NULL);
                if((oTempProcess->iInitialBurstTime == oTempProcess->iRemainingBurstTime)){
                    if(getDifferenceInMilliSeconds(oTempProcess->oTimeCreated, now) > BOOST_INTERVAL){
                        printf("Boost priority: Process Id = %d, Priority = %d, New Priority = %d\n"
                        , oTempProcess->iProcessId
                        , oTempProcess->iPriority
                        , iMaxPriority);
                        removeFirst(&oProductListHeads[iPriority], &oProductListTails[iPriority]);
                        addFirst(oTempProcess, &oProductListHeads[iMaxPriority], &oProductListTails[iMaxPriority]);
                    }
                }else{
                    if(getDifferenceInMilliSeconds(oTempProcess->oMostRecentTime, now) > BOOST_INTERVAL){
                        printf("Boost priority: Process Id = %d, Priority = %d, New Priority = %d\n"
                        , oTempProcess->iProcessId
                        , oTempProcess->iPriority
                        , iMaxPriority);
                        removeFirst(&oProductListHeads[iPriority], &oProductListTails[iPriority]);
                        addFirst(oTempProcess, &oProductListHeads[iMaxPriority], &oProductListTails[iMaxPriority]);
                    }
                }
            }
        }
        sem_post(&sBufferSync);
    }

    return 0;
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