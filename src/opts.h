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
#ifndef _OPTS_H
#define _OPTS_H

void parseOptions(int, char** const);
void printOptions();

struct Options {
	char *blockSizeStr;			/* -b, --bsize option */
    int *blockSize;             /* Parsed blocksizes */
	int blockSizeCount;         /* Number of elements in blockSize */
	long alignment;             /* Memory alignment of buffer */
    char *directory;            /* -d, --dir option   */
	char *filePrefix;           /* -f, --filePrefix   */
	int numFiles;               /* -n, --numFiles     */
	int fileSize;               /* -s, --fileSize     */
	int duration;               /* -t, --duration     */
	int linearLogging;          /* -l, --linear       */
	char * csvFile;             /* -c, --csvFile      */
	int backgroundThreads;      /* -p, --pthreads     */
};

extern struct Options options;

#endif