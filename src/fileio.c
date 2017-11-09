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
#define _GNU_SOURCE /*Needed to support O_DIRECT*/

#include <sys/types.h>
#include <sys/stat.h>

#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<limits.h>
#include<unistd.h>

#include "util.h"
#include "fileio.h"
#include "opts.h"

#include <locale.h>

/*
 * prepareFiles
 *
 * Create the files to be written to (fully populated with seek positions set back to 0).
 * Create a fileStore structure to keep the handles, seek positions, file statistics and a pointer to 
 * the properly aligned string (of --blockSize length) to be use for subsequent writes.
 *
 * Also creates the memory aligned string to be used for write operations
 */
char *bigString;

struct fileStore *prepareFiles(){
   int **files;
   int currFileIndex = 0;
   int stemOffset = 0;
   char *currFileName = NULL;
   int i;
   int writeLen;
   
   struct fileStore *fs;
        
   struct iovec format_vec[1];  /*Use this to 'format' the files*/

   struct stat stat_buf;

   /*int openOptions = O_CREAT|O_RDWR|O_SYNC|O_DSYNC|O_DIRECT;*/
   int openOptions = O_CREAT|O_RDWR|O_DSYNC|O_DIRECT;
   int openOptions2 = O_RDWR|O_DSYNC;
   /*int openOptions = O_CREAT|O_RDWR|O_SYNC|O_DSYNC|O_DIRECT;*/

   int rc = -1;

   puts("Creating files...");
   umask(0); /*Change umask to allow us to set all file permissions*/
			
   fs = (struct fileStore*)malloc(sizeof(struct fileStore));	
   fs->stats.total_writes = 0;
   fs->stats.interval_max_bytes_sec = 0;
   fs->stats.interval_min_bytes_sec = LONG_MAX;
   fs->stats.interval_max_latency = 0;
   fs->stats.interval_min_latency = LONG_MAX;   
   fs->stats.update_count = 0;
   fs->stats.persec_bytes = 0;
   fs->stats.max_bytes_sec = 0;
   fs->stats.avg_bytes_sec = 0;
   fs->stats.min_bytes_sec = LONG_MAX;
   fs->stats.intervalTimer = 0;

   bigString=UtilMakeBigString(options.fileSize, options.alignment);
   
   fs->writeVec[0].iov_base = bigString;
   fs->writeVec[0].iov_len = 0;
   	
   /*Construct first filespec*/
   currFileName = (char*)malloc(strlen(options.directory)+strlen(options.filePrefix)+7);
   
   if(currFileName == NULL){

	   fprintf(stderr,"Error allocating filename\n");
	   exit;
   } else {
       strcpy(currFileName,options.directory);	
       strcat(currFileName,"/");
       strcat(currFileName,options.filePrefix);
	   stemOffset=strlen(currFileName)+1;
	   strcat(currFileName,".0000");
   }	
	
   /*Create 2d array of files with fd and seek position for each file*/			
   fs->files = malloc(options.numFiles * sizeof(int *));
	
   if(fs->files == NULL) {
      fprintf(stderr, "Error allocating file array\n");
      exit;
   }
   for(i = 0; i < options.numFiles; i++) {
      fs->files[i] = malloc(3 * sizeof(int));
      if(fs->files[i] == NULL){
         fprintf(stderr, "Error allocating file array\n");
         exit;
      }
   }
   
   
   format_vec[0].iov_base = bigString;
   format_vec[0].iov_len = options.fileSize;
   
   for(i = 0; i < options.numFiles;i++){
      rc = stat(currFileName, &stat_buf);
      if (rc == 0) {
         if (((long long)stat_buf.st_size) != options.fileSize) {
            rc = -1;
         }
      }
 	  fs->files[i][0]=open(currFileName, openOptions, 0660);
	  if(fs->files[i][0] == -1) {
		  printf("Error creating (or opening existing) test file %s: %s\n",currFileName,strerror(errno));
		  exit(8);
	  }
	  
	  fs->files[i][2]=open(currFileName, openOptions2);
	  if(fs->files[i][0] == -1) {
		  printf("Error creating 2nd handle on test file %s: %s\n",currFileName,strerror(errno));
		  exit(8);
	  }
	  
      if (rc == -1) {
	     writeLen = writev(fs->files[i][0],format_vec,1);
	     if(writeLen == -1 ){
  		    printf("Error populating test file %s: %s\n",currFileName,strerror(errno));
		    exit(8);
	     }
      }
	  fs->files[i][1]=0; /*set seek position to start of file*/
	  if(lseek(fs->files[i][0],0,SEEK_SET) == -1){
		 printf("Error setting seek position on test file %s to 0: %s\n",currFileName,strerror(errno));
		 exit(8);
	  }
	  
	  currFileIndex++;
	  sprintf(currFileName+stemOffset,"%04i",currFileIndex);
   }	
   umask(022); /*Change umask to allow us to set all file permissions*/	  	
   return fs;
}

ssize_t writeToFile(struct fileStore *fs, int writeBlockSize){
	ssize_t writeLen;
	
	if(fs->files[fs->currentFile][1] + writeBlockSize > options.fileSize) {
		/*Next write will take us beyond end of file size, so reset seek posion to 0, and move on to next file*/
		fs->files[fs->currentFile][1]=0;
		if(lseek(fs->files[fs->currentFile][0],0,SEEK_SET) == -1){
			perror("Error resetting seek position on test file to 0");
			exit(8);	
		}

		fs->currentFile++;
		if(fs->currentFile >= options.numFiles)fs->currentFile=0;
	}
	 
    if((writeLen = writev(fs->files[fs->currentFile][0],fs->writeVec,1))== -1){
		perror("Error writing to test files");
		exit(8);
	} else {
		fs->stats.total_writes++;
		fs->stats.persec_bytes+=writeLen;
	}
		
	fs->files[fs->currentFile][1]+=writeBlockSize;
	if(lseek(fs->files[fs->currentFile][0],fs->files[fs->currentFile][1],SEEK_SET) == -1){
		perror("Error setting seek position on test file");
		exit(8);	
	}
	return writeLen;
}

void closeFiles(struct fileStore *fs){
	int i;
	for(i=0;i<options.numFiles;i++){
		close(fs->files[i][0]);
		close(fs->files[i][2]);
	}
}

void resetFiles(struct fileStore *fs){
	int i;
	for(i=0;i<options.numFiles;i++){
		lseek(fs->files[i][0],0,SEEK_SET);
	}
    fs->stats.total_writes = 0;
    fs->stats.interval_max_bytes_sec = 0;
    fs->stats.interval_min_bytes_sec = LONG_MAX;
    fs->stats.interval_max_latency = 0;
    fs->stats.interval_min_latency = LONG_MAX;   
    fs->stats.update_count = 0;
    fs->stats.persec_bytes = 0;
    fs->stats.max_bytes_sec = 0;
    fs->stats.avg_bytes_sec = 0;
    fs->stats.min_bytes_sec = LONG_MAX;
    fs->stats.intervalTimer = 0;
}

void setBlockSize(struct fileStore *fs, int writeBlockSize){
    fs->writeVec[0].iov_len = writeBlockSize;
}

void updateFileStats(struct fileStore *fs){
	if(fs->stats.intervalTimer < 1000000000  && fs->stats.intervalTimer > 500000000){
		fs->stats.persec_bytes = fs->stats.persec_bytes * 1/((float)fs->stats.intervalTimer/1000000000);
	}
	if(fs->stats.persec_bytes > fs->stats.interval_max_bytes_sec) fs->stats.interval_max_bytes_sec = fs->stats.persec_bytes;
	if(fs->stats.persec_bytes < fs->stats.interval_min_bytes_sec) fs->stats.interval_min_bytes_sec = fs->stats.persec_bytes;
	
	fs->stats.avg_bytes_sec = ((fs->stats.avg_bytes_sec * fs->stats.update_count) + fs->stats.persec_bytes) / (fs->stats.update_count+1);
	fs->stats.update_count++;
	fs->stats.persec_bytes = 0;
	fs->stats.intervalTimer = 0;
}

const char totalBytesDesc[]="Total bytes written to files";
const char intervalMaxBytesSecDesc[]="Max bytes/sec written to files (over 1 sec interval)";
const char intervalMinBytesSecDesc[]="Min bytes/sec written to files (over 1 sec interval)";
const char avgBytesSecDesc[]="Avg bytes/sec written to files";
const char totalWritesDesc[]="Total writes to files";

void printFileStats(struct fileStore *fs){
	setlocale(LC_ALL, "");
	printf("%s                                : %'15li\n",totalWritesDesc,fs->stats.total_writes);
	printf("%s                         : %'15li\n",totalBytesDesc,(fs->stats.total_writes * fs->writeVec[0].iov_len) );
	printf("%s : %'15li\n",intervalMaxBytesSecDesc,fs->stats.interval_max_bytes_sec);
	printf("%s : %'15li\n",intervalMinBytesSecDesc,fs->stats.interval_min_bytes_sec);
	printf("%s                       : %'15li\n",avgBytesSecDesc,fs->stats.avg_bytes_sec);
}

void csvFileStatsTitles(struct fileStore *fs, FILE *csvFile){
	if(	fprintf(csvFile,"%s,%s,%s,%s,%s",totalWritesDesc,totalBytesDesc,intervalMaxBytesSecDesc,intervalMinBytesSecDesc,avgBytesSecDesc) < 0) {
		perror("Error writing to csvFile");
		exit(8);
	}
}

void csvFileStats(struct fileStore *fs, FILE *csvFile){
	if(fprintf(csvFile,"%li,%li,%li,%li,%li",fs->stats.total_writes,(fs->stats.total_writes * fs->writeVec[0].iov_len),fs->stats.interval_max_bytes_sec,fs->stats.interval_min_bytes_sec,fs->stats.avg_bytes_sec) < 0){
		perror("Error writing to csvFile");
		exit(8);
	}
}

