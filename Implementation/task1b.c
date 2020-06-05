//generateProcessList():    used to generate a pre-defined NUMBER_OF_JOBS and stores them in a linked list

#include <stdio.h>
#include <stdlib.h>
#include "./coursework.h"
#include "./linkedlist.h"

void removeKnot(struct element* oReadyQueueKnot);
void generateProcessList(struct element ** pReadyQueueHead, struct element ** pReadyQueueTail);

int main(int argc, char* argv[]){
    long int iTimeCounter = 0;
    double dSumResponseTime = 0, dSumTurnAroundTime = 0;
    struct element* oReadyQueueHead = NULL, * oReadyQueueTail = NULL, * oReadyQueuePre = NULL, * oReadyQueueTemp;
    struct process* oTempProcess;

    generateProcessList(&oReadyQueueHead, &oReadyQueueTail);
    oReadyQueueTail->pNext = oReadyQueueHead;
    oReadyQueuePre = oReadyQueueTail;

    oReadyQueueTemp = oReadyQueueHead;
    for(int iTemp = NUMBER_OF_JOBS; iTemp > 0;){
        oTempProcess = (struct process*)oReadyQueueTemp->pData;
        runPreemptiveJob(oTempProcess, &(oTempProcess->oTimeCreated), &(oTempProcess->oMostRecentTime));
        printf(
            "\nProcess Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d", 
            oTempProcess->iProcessId, 
            oTempProcess->iPriority,
            oTempProcess->iPreviousBurstTime, 
            oTempProcess->iRemainingBurstTime
        );
        if(oTempProcess->iPreviousBurstTime == oTempProcess->iInitialBurstTime){
            printf(", Response Time = %d", iTimeCounter);
            dSumResponseTime += iTimeCounter;
        }
        iTimeCounter += getDifferenceInMilliSeconds(oTempProcess->oTimeCreated, oTempProcess->oMostRecentTime);
        if(oTempProcess->iRemainingBurstTime == 0){
            printf(", Turnaround Time = %d", iTimeCounter);
            dSumTurnAroundTime += iTimeCounter;
            oReadyQueuePre->pNext = oReadyQueueTemp->pNext;
            removeKnot(oReadyQueueTemp);
            iTemp--;
            if(iTemp != 0){
                oReadyQueueTemp = oReadyQueuePre->pNext;
            }else{
                break;
            }
            continue;
        }
        oReadyQueuePre = oReadyQueueTemp;
        oReadyQueueTemp = oReadyQueueTemp->pNext;
    }
    printf(
        "\nAverage response time = %.6f\nAverage turn around time = %.6f", 
        dSumResponseTime / NUMBER_OF_JOBS, 
        dSumTurnAroundTime / NUMBER_OF_JOBS
    );

    return 0;
}

void generateProcessList(struct element ** pReadyQueueHead, struct element ** pReadyQueueTail){
    struct process * oTempProcess;

    printf("PROCESS LIST:\n");
    for(int iTempCounter = 0; iTempCounter < NUMBER_OF_JOBS; iTempCounter++){
        addLast(generateProcess(), pReadyQueueHead, pReadyQueueTail);
        oTempProcess = (struct process* )(*pReadyQueueTail)->pData;
        printf(
            "	 Process Id = %d, Priority = %d, Initial Burst Time = %d, Remaining Burst Time = %d\n", 
            oTempProcess->iProcessId, 
            oTempProcess->iPriority,
            oTempProcess->iInitialBurstTime, 
            oTempProcess->iRemainingBurstTime
        );
    }
    printf("END\n");
}

void removeKnot(struct element* oReadyQueueKnot){
    free(oReadyQueueKnot->pData);
    oReadyQueueKnot->pData = NULL;
    free(oReadyQueueKnot);

    return;
}