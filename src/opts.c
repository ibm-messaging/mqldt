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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "opts.h"
#include "util.h"

void display_usage(void) {
    puts("mqldt - MQ log file tester");
    puts("Usage: mqldt --dir=<directory> --bsize=<list of csv blocksizes i.e 32K,64K> [ --filePrefix=<prefix> --fileSize=<fs> --numFiles=<n> --duration=<s> --csvFile=<csvFileName> --qm=<#qm>]");
    exit(8);
}

int alignmentSpecified = 0;
static const char *optString = "h?b:d:f:s:t:n:c:a:p:q:";

static const struct option longOpts[] = {
    {"bsize", required_argument, NULL, 'b'},
    {"dir", required_argument, NULL, 'd'},
    {"filePrefix", optional_argument, NULL, 'f'},
    {"fileSize", optional_argument, NULL, 's'},
    {"duration", optional_argument, NULL, 't'},
    {"numFiles", required_argument, NULL, 'n'},
    {"csvFile", optional_argument, NULL, 'c'},
    {"alignment", optional_argument, NULL, 'a'},
    {"backgroundThreads", optional_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {"qm", optional_argument, NULL, 'q'},
    {NULL, no_argument, NULL, 0}};

struct Options options;

void parseOptions(int argc, char *argv[]) {
    int longIndex;
    int opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    int i = 0;
    char *bSizeValue;

    /*Set defaults*/
    options.directory = ".";
    options.filePrefix = "mqldt_testfile";
    options.fileSize = 1638400;
    options.alignment = 4096; /*MQ Sets log page alignment to 4K*/
    options.duration = 10;
    options.blockSizeCount = 0;
    options.csvFile = NULL;
    options.backgroundThreads = 0;
    options.numFiles = 8;
    options.qm = 1;

    if (opt == -1) {
        display_usage();
        return;
    }

    while (opt != -1) {
        //It's an undocumented 'feature' of getopt_long that optional parameers need to be specified with the equals sign
        //This isn't trapped by the normal error processing (indicated by an opt value of '?')
        if(!optarg && opt !='?') {
            printf("No value specified for parameter --%s. Hint: long form parms must be specified as <parm>=<value>.\n",longOpts[longIndex].name,opt);
            display_usage();
        }
    
        switch (opt) {
        case 'b':
            options.blockSizeStr = malloc(strlen(optarg) + 1);
            strcpy(options.blockSizeStr, optarg);

            for (i = 0; i < strlen(optarg); i++) {
                if (optarg[i] == ',')
                    options.blockSizeCount++;
            }
            options.blockSizeCount++;
            options.blockSize = malloc(options.blockSizeCount * sizeof(int));

            bSizeValue = strtok(optarg, ",");
            for (i = 0; i < options.blockSizeCount; i++) {
                if (bSizeValue[strlen(bSizeValue) - 1] != 'k' && bSizeValue[strlen(bSizeValue) - 1] != 'K') {
                    fprintf(stderr, "Error: --bsize must be specified as <n>k \n");
                    display_usage();
                } else {
                    options.blockSize[i] = atoi(bSizeValue) * 1024;
                }
                bSizeValue = strtok(NULL, ",");
            }
            break;

        case 'c':
            options.csvFile = optarg;
            break;

        case 'd':
            options.directory = optarg;
            break;

        case 'f':
            options.filePrefix = optarg;
            break;

        case 's':
            options.fileSize = atoi(optarg);
            break;

        case 't':
            options.duration = atoi(optarg);
            if (options.duration < 1) {
                fprintf(stderr, "Error: value of duration is too small\n");
                display_usage();
            }
            break;

        case 'n':
            options.numFiles = atoi(optarg);
            if (options.numFiles > 10000) {
                fprintf(stderr, "Error: value of numFiles is too big\n");
                display_usage();
            }
            if (options.numFiles < 1) {
                fprintf(stderr, "Error: value of numFiles is too small\n");
                display_usage();
            }
            break;

        case 'a':
            options.alignment = atoi(optarg);
            alignmentSpecified = 1;
            break;

        case 'p':
            options.backgroundThreads = atoi(optarg);
            break;

        case 'q':
            options.qm = atoi(optarg);
            if (options.qm > 10) {
                puts("A maximum of 10 simulated queue managers is supported\n");
                display_usage();
            }
            break;

        case 'h': /* fall-through is intentional */
        case '?':
            display_usage();
            break;

        default:
            /* Shouldn't get here. */
            break;
        }
        opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    }
    if (options.alignment == 0) {
        options.alignment = file_GetPhysicalBlockSize(options.directory);
    }
    if ((options.qm > 1) && (options.blockSizeCount > 1)) {
        puts("MQLDT currently only supports a single block size with more than 1 QM");
        display_usage();
    }
}

void printOptions() {
    puts("Options (specified or defaulted to)");
    puts("=====================================================================================");
    printf("Write blocksize             (--bsize)              : %s\n", options.blockSizeStr);
    printf("Directory to write to       (--dir)                : %s\n", options.directory);
    printf("Test file prefix            (--filePrefix)         : %s\n", options.filePrefix);
    printf("Number of files to write to (--numFiles)           : %i\n", options.numFiles);
    printf("Size of test files          (--fileSize)           : %i\n", options.fileSize);
    /*printf("Simulate linear logging?    (--linear)     : %i\n",options.linearLogging);*/
    printf("Test duration               (--duration)           : %i\n", options.duration);
    if (alignmentSpecified)
        printf("Record alignment            (--alignment)          : %ld\n", options.alignment);
    if (options.backgroundThreads > 0)
        printf("Background Threads          (--backgroundThreads)  : %i\n", options.backgroundThreads);
    if (options.qm > 1)
        printf("Queue Managers              (--qm)                 : %i\n", options.qm);
    puts("");
}
