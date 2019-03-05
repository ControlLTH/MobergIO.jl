/*
  realtimer.c, 
  a MEX file for syncing elapsed Simulink time to elapsed wall time.
  
  Copyright (C) 1999-2005 Anders Blomdell <anders.blomdell@control.lth.se>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#define S_FUNCTION_LEVEL 2
#define S_FUNCTION_NAME realtimer

#include "simstruc.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

static unsigned long long TIME2ULL(struct timeval t)
{
  unsigned long long result;
  result = (unsigned long long)t.tv_sec * 1000000 + t.tv_usec;
  return result;
}

#if 0
static struct timeval ULL2TIME(unsigned long long t)
{
  struct timeval result;
  result.tv_sec = t / 1000000;
  result.tv_usec = t % 1000000;
  return result;
}
#endif

static struct timespec ULL2TIMESPEC(unsigned long long t)
{
  struct timespec result;
  result.tv_sec = t / 1000000;
  result.tv_nsec = (t % 1000000) * 1000;
  return result;
}

static void mdlInitializeSizes(SimStruct *S)
{
  ssSetNumSFcnParams(S, 1);  /* Number of expected parameters */
  if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) { return; }

  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);

  if (!ssSetNumInputPorts(S, 0)) { return; }

  if (!ssSetNumOutputPorts(S, 1)) { return; } 
  ssSetOutputPortWidth(S, 0, 1);

  ssSetNumSampleTimes(S, 1);

  ssSetNumRWork(S, 0);
  ssSetNumIWork(S, 6); /* 0,1: long long h 
			  2,3: long long t 
			  4,5: long long diff */
  ssSetNumPWork(S, 0); 
  ssSetNumModes(S, 0);

  ssSetNumNonsampledZCs(S, 0);
  
  ssSetOptions(S, 0);
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
  ssSetSampleTime(S, 0, mxGetScalar(ssGetSFcnParam(S, 0)));
  ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START
static void mdlStart(SimStruct *S)
{
  struct timeval now;
  unsigned long long *work = (unsigned long long *)ssGetIWork(S);
  gettimeofday(&now, 0); 
  work[0] = mxGetScalar(ssGetSFcnParam(S, 0)) * 1000000;
  work[1] = TIME2ULL(now);
  work[2] = 0;
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
  unsigned long long *work = (unsigned long long *)ssGetIWork(S);
  real_T *y = ssGetOutputPortRealSignal(S,0);

  *y = 1.0 - ((real_T)work[2] / (real_T)work[0]); 
}

#define MDL_UPDATE
static void mdlUpdate(SimStruct *S, int_T tid)
{
  unsigned long long *work = (unsigned long long *)ssGetIWork(S);
  long long diff;
  struct timeval now;

  work[1] += work[0];
  gettimeofday(&now, 0);
  diff = work[1] - TIME2ULL(now);
  if ((diff) <= 0) {
    diff = 0;
    work[1] = TIME2ULL(now);
  } else {
    struct timespec delay = ULL2TIMESPEC(diff);
    nanosleep(&delay, NULL);
  }
  work[2] = diff;
}

static void mdlTerminate(SimStruct *S)
{
}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
