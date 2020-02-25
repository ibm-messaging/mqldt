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
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fileio.h"
#include "opts.h"
#include "util.h"

void *consume_cpu(void *arg) {
    clock_t start, end;
    double runTime;
    start = clock();
    int i, num = 1, primes = 0;
    //puts("Thread started");
    while (num <= 1000000000) {
        i = 2;
        while (i <= num) {
            if (num % i == 0)
                break;
            i++;
        }
        if (i == num)
            primes++;
        num++;
    }

    end = clock();
    runTime = (end - start) / (double)CLOCKS_PER_SEC;
    //printf("This machine calculated all %d prime numbers under 1000 in %g seconds\n", primes, runTime);
    puts("Thread ended");
}

void openCSVFile() {
    if (options.csvFile != NULL) {
		if (options.qm == 1) {
        	csvFile = fopen(options.csvFile, "w");
		} else {
        	csvFile = fopen(options.csvFile, "a");
        	fprintf(csvFile, "Running MQLDT over %d Queue Managers\n", options.qm);
		}
        if (csvFile == NULL) {
            perror("Error opening csv file for writing");
            exit(8);
        }
        fprintf(csvFile, "Blocksize,");
		if (options.qm == 1) {
        	csvFileStatsTitles(csvFile);
		} else {
        	csvQMFileStatsTitles(csvFile);
		}
        fprintf(csvFile, ",");
        csvTimerStatsTitles(csvFile);
        fprintf(csvFile, "\n");
    }
    return;
}

void CSVStats(struct fileStore *files, struct timer *t1, int blockSize) {
    if (options.csvFile != NULL) {
        fprintf(csvFile, "%i,", blockSize);
		if (options.qm == 1) {
	        csvFileStats(files, csvFile);
		} else {
	        csvQMFileStats(files, csvFile);
		}
        fprintf(csvFile, ",");
        csvTimerStats(t1, csvFile, blockSize);
        fprintf(csvFile, "\n");
    }
}

void CSVQMStats(struct fileStore *files, struct timer *t1, int blockSize) {
    if (options.csvFile != NULL) {
        fprintf(csvFile, "%i,", blockSize);
        csvQMFileStats(files, csvFile);
        fprintf(csvFile, ",");
        csvTimerStats(t1, csvFile, blockSize);
        fprintf(csvFile, "\n");
    }
}


void *runTest(void *arg) {
    struct timer t1;
    int elapsed = 0;
    int thread_err, testCount;
    int elapsedStringDigits = 0;
    int elapsedStringTailChars = 0;

    struct fileStore *files;
    files = (struct fileStore *)arg;

    elapsedStringTailChars = floor(log10(options.duration)) + 2;
    /*We report secs elapsed as 'elapsed/duration', e.g 34/100, this value is how many chars the back half of that string takes (e.g. '/100')*/

    //Wait for sync to ensure all threads start at the same time (after file creation has finished)
    if (options.qm > 1) {
        pthread_mutex_lock(&mutex);
        while (started == 0) {
            pthread_cond_wait(&condition, &mutex);
        }
        pthread_mutex_unlock(&mutex);
    }

    //Construct the timer and set the start time of the test
    tInit(&t1);
    for (testCount = 0; testCount < options.blockSizeCount; testCount++) {
        setBlockSize(files, options.blockSize[testCount]);

        //Only display on first thread (0 for single thread, 1 for multi-thread)
        if (files->thread <= 1)
            fprintf(stderr, "Executing test for write blocksize %i (%ik). Seconds elapsed -> ", options.blockSize[testCount], options.blockSize[testCount] / 1024);

        //Stores current time in start time
        tReset(&t1);

        //Stores current time in check_time1
        tStart(t1);
        do {
            writeToFile(files, options.blockSize[testCount]);

            //tCheck stores current time in check_time2 and calculates difference between each check_time, flips time2->time1
            files->stats.intervalTimer += tCheck(&t1);

            if (files->stats.intervalTimer >= ONE_SEC_IN_NS) {
                if (files->thread <= 1) {
                    elapsed++;
                    elapsedStringDigits = floor(log10(elapsed)) + 1;
                    fprintf(stderr, "%i/%i", elapsed, options.duration);
                    fflush(stderr);
                    /*Backspace the cursor the right number of digits*/
                    fprintf(stderr, "%.*s", (elapsedStringDigits + elapsedStringTailChars), "\b\b\b\b\b\b\b\b\b\b\b");
                }
                updateFileStats(files);
            }
            /*Note the not operator. Read the line below as 'until')*/
            //Second clause required if start time nsec was at or close to 999999999
        } while (!(((t1.check_time1->tv_sec - t1.start_time->tv_sec == options.duration) && (t1.check_time1->tv_nsec > t1.start_time->tv_nsec)) ||
                   (t1.check_time1->tv_sec - t1.start_time->tv_sec > options.duration)));

        updateFileStats(files);

        if (files->thread <= 1) {
            fprintf(stdout, "%i/%i", options.duration, options.duration);
            puts("");
            puts("");
        }

        //For single QM test, we can dump out stats as we iterate through the block sizes
        if (options.qm == 1) {
            printFileStats(files);
            puts("");
            printTimerStats(&t1, options.blockSize[testCount]);
            puts("");
            elapsed = 0;
            CSVStats(files, &t1, options.blockSize[testCount]);
            resetFiles(files); /*Reset SEEK pointers to start of files if running further tests */
        }
    }
    // If we have multi QM, we have only a single block size, store timings and return
    if (options.qm > 1) {
        memcpy(&files->resultTimings, &t1, sizeof(struct timer));
    }

    closeFiles(files);
    return;
}

void processStats(struct fileStore *array[]) {
    int k;
    for (k = 1; k <= options.qm; k++) {
        array[0]->stats.total_writes += array[k]->stats.total_writes;
        array[0]->stats.total_bytes += array[k]->stats.total_writes * options.blockSize[0];

        if (array[k]->stats.interval_max_bytes_sec > array[0]->stats.interval_max_bytes_sec) {
            array[0]->stats.interval_max_bytes_sec = array[k]->stats.interval_max_bytes_sec;
        }
        if (array[k]->stats.interval_min_bytes_sec < array[0]->stats.interval_min_bytes_sec) {
            array[0]->stats.interval_min_bytes_sec = array[k]->stats.interval_min_bytes_sec;
        }
        array[0]->stats.avg_bytes_sec += array[k]->stats.avg_bytes_sec;

        if (array[k]->resultTimings.max_time > array[0]->resultTimings.max_time) {
            array[0]->resultTimings.max_time = array[k]->resultTimings.max_time;
            array[0]->resultTimings.max_time_instance = array[k]->resultTimings.max_time_instance;
        }
        if (array[k]->resultTimings.min_time < array[0]->resultTimings.min_time) {
            array[0]->resultTimings.min_time = array[k]->resultTimings.min_time;
            array[0]->resultTimings.min_time_instance = array[k]->resultTimings.min_time_instance;
        }
        array[0]->resultTimings.avg_time += array[k]->resultTimings.avg_time;
    }
    array[0]->resultTimings.avg_time /= options.qm;
}

int main(int argc, char *argv[]) {
    struct fileStore *files;
    struct fileStore *filesArray[11];
    int thread = 1;
    int thread_err;
    pthread_t tid[11];
    int k;
    pthread_t bgtid[100];
    int bgCount;

    parseOptions(argc, argv);
    puts("");
    printOptions();
    openCSVFile();

    //Start background threads if required
    for (bgCount = 0; bgCount < options.backgroundThreads; bgCount++) {
        thread_err = pthread_create(&(bgtid[bgCount]), NULL, &consume_cpu, NULL);
        if (thread_err) {
            puts("Error creating background thread\n");
            exit(8);
        }
    }
    if (options.backgroundThreads > 0)
        printf("%i backgound threads started\n", options.backgroundThreads);

    if (options.qm > 1) {
        printf("Running simulation of %d Queue Managers\n", options.qm);
        files = prepareStats(0);
        filesArray[0] = files;

        //Create conditional flag which can be used on each thread, so that they are notified when file creation is finished
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&condition, NULL);
        pthread_mutex_lock(&mutex);
        started = 0;
        pthread_mutex_unlock(&mutex);

        while (thread <= options.qm) {

            files = prepareFiles(thread);
            filesArray[thread] = files;

            //Spawn onto thread
            thread_err = pthread_create(&tid[thread], NULL, &runTest, (void *)filesArray[thread]);
            if (thread_err) {
                puts("Error creating writing thread\n");
                exit(8);
            }
            thread++;
        }
        pthread_mutex_lock(&mutex);
        started = 1;
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&mutex);

        //Wait for all threads to finish.
        for (k = 1; k <= options.qm; k++) {
            pthread_join(tid[k], NULL);
        }
        processStats(filesArray);
        //Process results
        printQMFileStats(filesArray[0]);
        puts("");
        printTimerStats(&filesArray[0]->resultTimings, options.blockSize[0]);
        puts("");
        CSVStats(filesArray[0], &filesArray[0]->resultTimings, options.blockSize[0]);
    } else {
        //Single qm
        files = prepareFiles(0);
        runTest(files);
    }

    if (options.csvFile != NULL)
        fclose(csvFile);
}
