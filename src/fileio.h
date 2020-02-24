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
#ifndef _FILEIO_H
#define _FILEIO_H

#include<sys/uio.h>

/*#define updateFstats(X)  clock_gettime(CLOCK_MONOTONIC,X.start_time)*/

struct fstats{
	int update_count;
	long persec_bytes;
	long total_writes;
	long total_bytes;
	long interval_max_bytes_sec;
	long interval_min_bytes_sec;
	long interval_max_latency;
	long interval_min_latency;
	long max_bytes_sec;
	long min_bytes_sec;
	long avg_bytes_sec;
	long intervalTimer;
};

struct fileStore{
	int **files;
	int currentFile;
	int fs_blockSize;
	struct iovec writeVec[1]; 
	struct fstats stats;
	struct timer resultTimings;
	int thread;
};

struct fileStore *prepareFiles(int tnum);
struct fileStore *prepareStats(int tnum);
ssize_t writeToFile(struct fileStore *fs, int writeBlockSize);
void setBlockSize(struct fileStore *fs, int writeBlockSize);
void resetFiles(struct fileStore *fs);
void closeFiles(struct fileStore *fs);
void updateFileStats(struct fileStore *fs);
void printFileStats(struct fileStore *fs);
void csvFileStatsTitles(FILE *csvFile);
void csvFileStats(struct fileStore *fs, FILE *csvFile);

#endif
