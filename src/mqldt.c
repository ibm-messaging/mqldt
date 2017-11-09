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
#include<stdio.h>
#include<stdlib.h> 
#include "opts.h"
#include "util.h"
#include "fileio.h"

#include<pthread.h>
#include<time.h>
#include<math.h>

void* consume_cpu(void *arg){
    clock_t start, end;
    double runTime;
    start = clock();
    int i, num = 1, primes = 0;
	//puts("Thread started");	
    while (num <= 1000000000) { 
        i = 2; 
        while (i <= num) { 
            if(num % i == 0)
                break;
            i++; 
        }
        if (i == num)
            primes++;
        num++;
    }

    end = clock();
    runTime = (end - start) / (double) CLOCKS_PER_SEC;
    //printf("This machine calculated all %d prime numbers under 1000 in %g seconds\n", primes, runTime);
	puts("Thread ended");	
}

int main(int argc, char *argv[]) {
   struct fileStore *files;   
   struct timer t1;
   int elapsed=0;
   
   int thread_err, testCount;
   pthread_t tid[100];
   int i2=0;
   int elapsedStringDigits=0;
   int elapsedStringTailChars=0;
   
   FILE *csvFile;

   parseOptions(argc, argv);
   puts("");
   printOptions();
   
   if(options.csvFile != NULL){
	  csvFile = fopen(options.csvFile,"w");
	  if(csvFile == NULL) {
		  perror("Error opening csv file for writing");
		  exit(8);
	  }
	  fprintf(csvFile,"Blocksize,");
	  csvFileStatsTitles(files,csvFile);
	  fprintf(csvFile,",");
	  csvTimerStatsTitles(&t1,csvFile);
	  fprintf(csvFile,"\n");
   }
 
   files = prepareFiles();

   for(i2=0;i2<options.backgroundThreads;i2++){	
	  //printf("Starting thread %i\n",i2);
      thread_err = pthread_create(&(tid[i2]), NULL, &consume_cpu, NULL);
   }
   if(options.backgroundThreads > 0)printf("%i backgound threads started\n",options.backgroundThreads);
   
   elapsedStringTailChars=floor(log10(options.duration))+2; 
   /*We report secs elapsed as 'elapsed/duration', e.g 34/100, this value is how many chars the back half of that string takes (e.g. '/100')*/
   
   tInit(&t1); /*Construct the timer and set the start time of the test*/
   for(testCount = 0 ; testCount < options.blockSizeCount; testCount++){
	   setBlockSize(files, options.blockSize[testCount]);  
	   fprintf(stderr,"Executing test for write blocksize %i (%ik). Seconds elapsed -> ",options.blockSize[testCount],options.blockSize[testCount]/1024);  
	   tReset(&t1);
	   do{ 
	      tStart(t1);	    
	      writeToFile(files, options.blockSize[testCount]);
	      files->stats.intervalTimer+=tCheck(&t1);
	      if(files->stats.intervalTimer>=1000000000){
			 elapsed++;
			 elapsedStringDigits=floor(log10(elapsed))+1;
			 fprintf(stderr,"%i/%i",elapsed,options.duration);
			 fflush(stdout);
			 /*if(elapsed % 10 == 0) fprintf(stderr,"%i ",elapsed);*/
			 /*Backspace the cursor the right number of digits*/
			 fprintf(stderr,"%.*s", (elapsedStringDigits+elapsedStringTailChars), "\b\b\b\b\b\b\b\b\b\b\b");
			 
		     updateFileStats(files);
	      }
	  /*Note the not operator. Read the line below as 'until')*/	  
	  } while (!((t1.check_time1->tv_sec - t1.start_time->tv_sec >= options.duration) && (t1.check_time1->tv_nsec > t1.start_time->tv_nsec)));
	  
	  fprintf(stderr,"%i/%i",options.duration,options.duration);
	  puts("");
	  puts("");

	  updateFileStats(files);
	  printFileStats(files);
	  puts("");
	  printTimerStats(&t1, options.blockSize[testCount]);
	  puts("");
	  elapsed = 0;
	  if(options.csvFile != NULL){
		  fprintf(csvFile,"%i,",options.blockSize[testCount]);
		  csvFileStats(files,csvFile);
		  fprintf(csvFile,",");
		  csvTimerStats(&t1,csvFile, options.blockSize[testCount]);
		  fprintf(csvFile,"\n");			  
	  }
	  resetFiles(files); /*Reset SEEK pointers to start of files */  
   }

  closeFiles(files);
  if(options.csvFile != NULL) close(csvFile);
}