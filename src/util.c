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
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>

#include "util.h"

/*
** Method: UtilMakeBigString
**
** Function to build a string of a given size.
** The string is allocated with malloc and needs to be disposed of by the caller.
**
** Input Parameters: size - the length of the string to build
**
** Returns: a pointer to the built character string
**
** The string start with ascii 65 ("A") and increment up to ascii 122 ("z")
** before wrapping round again.
*/
char *UtilMakeBigString(int size, long alignment) {
    char *str = NULL;
	
	posix_memalign((void**)&str, alignment, size);
	if (NULL != str ){
        int i;
        char c = 65;
        for (i=0; i < size; i++) {
           str[i] = c++;
           if (c > 122) c = 65;
        }

    /* Null terminate the string for trace etc */
    str[size - 1] = '\0';
    }
	/*printf("Buffer address %p\n", (void *) str);*/
    return(char*)(str);
}

long file_GetPhysicalBlockSize(char *path){
	struct stat sb;
	if (stat(path, &sb) == -1) {
	   printf("Can't stat %s\n",path);
	        exit(EXIT_FAILURE);
	}
	return (long)sb.st_blksize;
}

void tInit(struct timer *timerIn){
	timerIn->start_time  = (struct timespec*) malloc(sizeof(struct timespec));
	timerIn->check_time1  = (struct timespec*) malloc(sizeof(struct timespec));
	timerIn->check_time2  = (struct timespec*) malloc(sizeof(struct timespec));
	timerIn->max_time    = 0;
	timerIn->min_time    = LONG_MAX;
	timerIn->avg_time    = 0;
	timerIn->check_count = 0;
	clock_gettime(CLOCK_MONOTONIC,timerIn->start_time);
}

void tReset(struct timer *timerIn){
	timerIn->max_time    = 0;
	timerIn->min_time    = LONG_MAX;
	timerIn->avg_time    = 0;
	timerIn->check_count = 0;
	clock_gettime(CLOCK_MONOTONIC,timerIn->start_time);
}

const char maxLatencyDesc[] = "Max latency of write (ns)";
const char minLatencyDesc[] = "Min latency of write (ns)";
const char avgLatencyDesc[] = "Avg latency of write (ns)";
const char maxBytesSecDesc[]= "Max bytes/sec (fastest write)";
const char minBytesSecDesc[]= "Min bytes/sec (slowest write)";

void printTimerStats(struct timer *timerIn, int blockSize){
	printf("%s      : %'15li (#%'i)\n",maxLatencyDesc, timerIn->max_time, timerIn->max_time_instance);
	printf("%s  : %'15li\n",minBytesSecDesc, (long)(blockSize/((float)timerIn->max_time/1000000000)));
	printf("%s      : %'15li (#%'i)\n",minLatencyDesc, timerIn->min_time, timerIn->min_time_instance);
	printf("%s  : %'15li\n",maxBytesSecDesc, (long)(blockSize/((float)timerIn->min_time/1000000000)));
	printf("%s      : %'15li\n",avgLatencyDesc, timerIn->avg_time);
}

void csvTimerStatsTitles(struct timer *timerIn, FILE *csvFile){
	fprintf(csvFile,"%s,%s,%s,%s,%s",maxLatencyDesc,minLatencyDesc,avgLatencyDesc,minBytesSecDesc,maxBytesSecDesc);
}

void csvTimerStats(struct timer *timerIn, FILE *csvFile, int blockSize){
	fprintf(csvFile,"%li,%li,%li,%li,%li",timerIn->max_time,timerIn->min_time,timerIn->avg_time,(long)(blockSize/((float)timerIn->max_time/1000000000)),(long)(blockSize/((float)timerIn->min_time/1000000000)));
}


