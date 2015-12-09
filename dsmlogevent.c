/*
 * Copyright (c) 2015 IPnett AS. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dsmapitd.h"
#include "dsmapifp.h"
#include "dapitype.h"
#include "dapiproc.h"
#include "dapidata.h"
//#include "dapiutil.h" /* seems unneeded now */
#include "dsmrc.h"
#include <sys/stat.h>

// #define DEBUG 1

char **globalArgv;

void printdsmerror(dsUint32_t,dsInt16_t);

int
main(int argc,char **argv) {
  
  int fdesc, usingfile=0, done=0;
  struct stat statbuf;
  FILE* stream;
  
  dsInt16_t dsmresult;
  dsUint32_t myhandle;

  optStruct *my_opts;

  dsmInitExIn_t *init_in;
  dsmInitExOut_t *init_out;

  dsmLogExIn_t *log_in;
  dsmLogExOut_t *log_out;

  char strbuf[1024];

  if (((2 == argc)&&(strncmp(argv[1],"-f",2))) ||	\
      ((3 == argc)&&(strncmp(argv[1],"-f",2)==0))) {
    
    /* handle -f case first */
    if (3 == argc) {
      usingfile=1;
      fdesc=open(argv[2],O_RDONLY);
      if (fdesc < 0) {
	printf("Could not open file %s\n", argv[2]);
	exit(1);
      }

      if (0==fstat(fdesc, &statbuf)) {
	if ((statbuf.st_size > 5*1024) || \
	    (statbuf.st_size < 2)) {
	  printf("Input file %s too small/large to send: %lld\n",
		 argv[2], statbuf.st_size);
	  exit(1);
	}
#ifdef DEBUG
	else {
	  printf("Size of file: %lld\n",statbuf.st_size);
	}
#endif /* DEBUG */
      } else {
	perror("fstat()");
	exit(1);
      }

      stream=fdopen(fdesc,"r");
      if (NULL==stream) {
	perror("fdopen failed");
	exit(1);
      }
    } 
    
    if ((my_opts=(optStruct*)calloc(1,sizeof(*my_opts))) == NULL) {
      printf("No mem for calloc\n");
      exit(1);
    }
#ifdef DEBUG
    else {
      printf("Sizeof optstruct: %d\n",sizeof(*my_opts));
    }
#endif /* DEBUG */
    
    dsmresult=dsmQueryCliOptions(my_opts);
#ifdef DEBUG
    printf("dsmQuery returned: %d\n",dsmresult);
#endif /* DEBUG */
    
    if ((init_in=(dsmInitExIn_t*)calloc(1,sizeof(*init_in))) == NULL) {
      printf("No mem for init_in\n");
      exit(1);
    }    
#ifdef DEBUG
    else {
      printf("sizeof: %d\n",sizeof(*init_in));
    }
#endif /* DEBUG */
    
    if ((init_out=(dsmInitExOut_t*)calloc(1,sizeof(*init_out))) == NULL) {
      printf("No mem for init_out\n");
      exit(1);
    }
#ifdef DEBUG
    else {
      printf("sizeof: %d\n",sizeof(*init_out));
    }
#endif /* DEBUG */
    
    if ((log_in=(dsmLogExIn_t*)calloc(1,sizeof(*log_in))) == NULL) {
      printf("No mem for log_in\n");
      exit(1);
    }    
#ifdef DEBUG
    else {
      printf("sizeof: %d\n",sizeof(*log_in));
    }
#endif /* DEBUG */
    
    if ((log_out=(dsmLogExOut_t*)calloc(1,sizeof(*log_out))) == NULL) {
      printf("No mem for log_out\n");
      exit(1);
    }
#ifdef DEBUG
    else {
      printf("Sizeof: %d\n",sizeof(*log_out));
    }
#endif /* DEBUG */
    
    /* Preparing the initial settings */
    init_in->stVersion        = dsmInitExInVersion;
    init_in->apiVersionExP    = &apiApplVer;
    init_in->clientNodeNameP  = NULL;
    init_in->clientOwnerNameP = NULL;
    init_in->clientPasswordP  = NULL;
    init_in->applicationTypeP = NULL;
    init_in->configfile       = NULL;
    init_in->options          = NULL;
    init_in->userNameP        = NULL;
    init_in->userPasswordP    = NULL;
    
    dsmresult=dsmInitEx(&myhandle,init_in,init_out);
    if (DSM_RC_OK != dsmresult) {
      printdsmerror(myhandle,dsmresult);
      printf("dsmInitEx failed: %d\n",dsmresult);
      exit(1);
    }
#ifdef DEBUG
    printf("dsmInitEx returned: %d\n",dsmresult);
#endif /* DEBUG */


    /* set common settings for both cmdline and file log shipping */
    log_in->severity = logSevInfo; /* ANE4990 */
    strncpy(log_in->appMsgID, "IPN4711",8);

    if(0==usingfile) {
      log_in->message        = argv[1];
      log_in->logType        = logBoth;
      done=1;
    } else {
      log_in->logType        = logServer;
      log_in->message        = fgets(strbuf,1010,stream);
    }

    do {

#ifdef DEBUG
      printf("would send string: %s", log_in->message);
#else     
      dsmresult=dsmLogEventEx(myhandle,log_in,log_out);
      if (DSM_RC_OK != dsmresult) {
	printdsmerror(myhandle, dsmresult);
	if (dsmresult == DSM_RC_STRING_TOO_LONG) {
	  printf("(max ~1000 chars)\n");
	}
	/* wont stop, we still want to run the exit cleanup */
      } else {
	printf("Message sent ok.\n");
      }
#endif /* DEBUG */

      if (usingfile) {
	log_in->message        = fgets(strbuf,1010,stream);
	if (log_in->message == NULL) {
	  done=1; /* break out of loop in case fgets stops
		   reading useful data out of the file */
	}
      }

    } while (!done);
    



    dsmresult=dsmTerminate(myhandle);
    if (DSM_RC_OK != dsmresult) {
      printdsmerror(myhandle, dsmresult);
    } else {
      printf("Exiting\n");
    }
      
    exit(0);
  } else {
    printf("Usage #1: %s <string in quotes to send>\n", argv[0]);
    printf("Usage #2: %s -f <filename to send>\n", argv[0]);
    exit(1);
  }
}

void
printdsmerror(dsUint32_t handle, dsInt16_t dserror) {

  char rcmess[DSM_MAX_RC_MSG_LENGTH];
  
  if (0 == dsmRCMsg(handle, dserror,rcmess)) {
    printf("Error: %d\n%s", dserror, rcmess);
  } else {
    printf("Can't translate error");
  }
  return;
}
