/*<copyright notice="lm-source" pids="" years="2017">*/
/*******************************************************************************
 * Copyright (c) 2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Contributors:
 *    Paul Harris - Initial implementation
 *******************************************************************************/
/*</copyright>*/
/*******************************************************************************/
/*                                                                             */
/* IBM MQ Log Disk Tester                                                      */
/*                                                                             */
/*******************************************************************************/
#ifndef _UTIL_H
#define _UTIL_H

#include <sys/time.h>
#include <time.h>

#define tStart(X) clock_gettime(CLOCK_MONOTONIC, X.check_time1);
#define ONE_SEC_IN_NS 1000000000
#define HALF_SEC_IN_NS 500000000

extern FILE *csvFile;
extern pthread_mutex_t mutex;
extern pthread_cond_t condition;
extern volatile int started;

struct timer {
    struct timespec *start_time; /*Make these pointers so we can flip them easily in tCheck*/
    struct timespec *check_time1;
    struct timespec *check_time2;
    int check_count;
    long min_time;
    int min_time_instance;
    long avg_time;
    long max_time;
    int max_time_instance;
};

//Calculates time difference between the current time and the last time this method was called, updates min/max/avg time values
static inline long tCheck(struct timer *timerIn) {
    long duration;
    struct timespec *temp;

    clock_gettime(CLOCK_MONOTONIC, timerIn->check_time2);
    timerIn->check_count++;

    if (timerIn->check_time2->tv_nsec > timerIn->check_time1->tv_nsec) {
        duration = ((timerIn->check_time2->tv_sec - timerIn->check_time1->tv_sec) * ONE_SEC_IN_NS - (timerIn->check_time1->tv_nsec - timerIn->check_time2->tv_nsec));
    } else {
        duration = ((timerIn->check_time2->tv_sec - timerIn->check_time1->tv_sec) * ONE_SEC_IN_NS + (timerIn->check_time2->tv_nsec - timerIn->check_time1->tv_nsec));
    }

    if (duration > timerIn->max_time) {
        timerIn->max_time = duration;
        timerIn->max_time_instance = timerIn->check_count;
    }
    if (duration < timerIn->min_time) {
        timerIn->min_time = duration;
        timerIn->min_time_instance = timerIn->check_count;
    }

    timerIn->avg_time = ((timerIn->avg_time * (timerIn->check_count - 1)) + duration) / (timerIn->check_count);

    /*flip the timespecs*/
    temp = timerIn->check_time1;
    timerIn->check_time1 = timerIn->check_time2;
    timerIn->check_time2 = temp;

    return duration;
}

void tInit(struct timer *timerIn);
void tReset(struct timer *timerIn);

void printTimerStats(struct timer *timerIn, int blockSize);
void csvTimerStatsTitles(FILE *csvFile);
void csvTimerStats(struct timer *timerIn, FILE *csvFile, int blockSize);

long file_GetPhysicalBlockSize(char *path);
char *UtilMakeBigString(int size, long alignment);

void *runTest(void *arg);
void openCSVFile();
#endif
