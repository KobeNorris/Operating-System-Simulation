//generateProcessList():    used to generate a pre-defined NUMBER_OF_JOBS and stores them in a linked list

#include <stdio.h>
#include <stdlib.h>
#include "./coursework.h"
#include "./linkedlist.h"

void generateProcessList(struct element ** pReadyQueueHead, struct element ** pReadyQueueTail);

int main(int argc, char* argv[]){
    long int iResponseTime = 0, iTurnAroundTime = 0, iTimeInterval = 0;
    double dSumResponseTime = 0, dSumTurnAroundTime = 0;
    struct process* oTempProcess;
    struct element* oReadyQueueHead = NULL, * oReadyQueueTail, * oReadyQueueTemp;

    generateProcessList(&oReadyQueueHead, &oReadyQueueTail);

    oReadyQueueTemp = oReadyQueueHead;
    do{
        oTempProcess = (struct process*)oReadyQueueTemp->pData;
        runNonPreemptiveJob(oTempProcess, &(oTempProcess->oTimeCreated), &(oTempProcess->oMostRecentTime));
        iTimeInterval = getDifferenceInMilliSeconds(oTempProcess->oTimeCreated, oTempProcess->oMostRecentTime);
        iTurnAroundTime += iTimeInterval;
        dSumTurnAroundTime += iTurnAroundTime;
        printf(
            "Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %d, Turn Around Time = %d\n", 
            oTempProcess->iProcessId, 
            oTempProcess->iPreviousBurstTime, 
            oTempProcess->iRemainingBurstTime, 
            iResponseTime, iTurnAroundTime
        );
        dSumResponseTime += iResponseTime;
        iResponseTime += iTimeInterval;
        removeFirst(&oReadyQueueTemp, &oReadyQueueTail);
        free(oTempProcess);
    } while(oReadyQueueTemp != NULL);
    printf(
        "Average response time = %.6f\nAverage turn around time = %.6f\n", 
        dSumResponseTime / NUMBER_OF_JOBS, 
        dSumTurnAroundTime / NUMBER_OF_JOBS
    );
    
    return 0;
}

void generateProcessList(struct element ** pReadyQueueHead, struct element ** pReadyQueueTail){
    for(int iTempCounter = 0; iTempCounter < NUMBER_OF_JOBS; iTempCounter++){
        addLast(generateProcess(), pReadyQueueHead, pReadyQueueTail);
    }

    return;
}