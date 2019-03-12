/*
  encoderin.c, 
  a MEX file for reading encoder input via Moberg
  
  Copyright (C) 2019 Anders Blomdell <anders.blomdell@control.lth.se>
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#define S_FUNCTION_LEVEL 2
#define S_FUNCTION_NAME  encoderin

#include <uchar.h>  /* for CHAR16_T typedef in tmwtypes.h */
#include "simstruc.h"
#include <moberg4simulink.h>

/*
  Usage of work vectors:

  PWork:    0           moberg_encoder_in pointer[0]
            1           moberg_encoder_in pointer[1]
            ...

 */

#define MDL_CHECK_PARAMETERS
static void mdlCheckParameters(SimStruct *S)
{
  /* 1st parameter: sampling interval */
  {
    if (!mxIsDouble(ssGetSFcnParam(S,0)) ||
	mxGetNumberOfElements(ssGetSFcnParam(S,0)) != 1) {
      ssSetErrorStatus(S, "sampling time must be a scalar");
      return;
    }
  }
 
  /* 2nd parameter: input channels */
  {
    int number_of_dims = mxGetNumberOfDimensions(ssGetSFcnParam(S,1)); 
    
    if (!mxIsDouble(ssGetSFcnParam(S,1)) ||
	number_of_dims != 2 || mxGetM(ssGetSFcnParam(S,1)) != 1) {
      ssSetErrorStatus(S, "input channels must be a scalar or a vector");
      return;
    }
  }
}

static void mdlInitializeSizes(SimStruct *S)
{
  int channelCount;

  ssSetNumSFcnParams(S, 2);
  if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
    mdlCheckParameters(S);
    if (ssGetErrorStatus(S) != NULL) { return; }
  } else {
    return;
  }

  channelCount = mxGetN(ssGetSFcnParam(S,1));

  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);

  if (!ssSetNumInputPorts(S, 1)) { return; }
  ssSetInputPortWidth(S, 0, 1);
  ssSetInputPortDirectFeedThrough(S, 0, 1);

  if (!ssSetNumOutputPorts(S, 2)) { return; }
  ssSetOutputPortWidth(S, 0, 1);
  ssSetOutputPortWidth(S, 1, channelCount); 

  ssSetNumSampleTimes(S, 1);
  ssSetNumPWork(S, channelCount);     /* 0:      moberg_encoder_in pointer[0]
                                         1:      moberg_encoder_in pointer[1]
					 ...
				      */
  ssSetNumModes(S, 0);
  ssSetNumNonsampledZCs(S, 0);
  
  ssSetOptions(S, 0);
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
  ssSetSampleTime(S, 0, mxGetScalar(ssGetSFcnParam(S, 0)));
  ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_INITIALIZE_CONDITIONS
static void mdlInitializeConditions(SimStruct *S)
{
  void **pwork = ssGetPWork(S);
  double *channel = mxGetPr(ssGetSFcnParam(S,1));
  int channel_count = mxGetN(ssGetSFcnParam(S,1));
  int i;

  for (i = 0 ; i < channel_count ; i++) {
    pwork[i] = moberg4simulink_encoder_in_open(channel[i]);
    if (! pwork[i]) {
      static char error[256];
      sprintf(error, "Failed to open encoderin #%d", (int)channel[i]);
      ssSetErrorStatus(S, error);
    }
  }
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
  void **pwork = ssGetPWork(S);

  {
    /* Propagate the dummy sorting signal */
    InputRealPtrsType up = ssGetInputPortRealSignalPtrs(S,0);
    real_T *y = ssGetOutputPortRealSignal(S, 0);
    y[0] = *up[0]+1;
  }
  {
    int i;
    real_T *y = ssGetOutputPortRealSignal(S, 1);

    for (i = 0 ; i < ssGetNumPWork(S) ; i++) {
      struct moberg_encoder_in *ein = (struct moberg_encoder_in*)pwork[i];
      long value;
      if (! moberg_OK(ein->read(ein->context, &value))) {
        static char error[256];
        double *channel = mxGetPr(ssGetSFcnParam(S,1));
        sprintf(error, "Failed to read encoderin #%d", (int)channel[i]);
        ssSetErrorStatus(S, error);
      }
      y[i] = value;
    }
  }
}

static void mdlTerminate(SimStruct *S)
{
  int i;
  void **pwork = ssGetPWork(S);
  double *channel = mxGetPr(ssGetSFcnParam(S,1));
  int channel_count = mxGetN(ssGetSFcnParam(S,1));

  for (i = 0 ; i < channel_count ; i++) {
    if (pwork[i]) {
      struct moberg_encoder_in *ein = (struct moberg_encoder_in*)pwork[i];
      moberg4simulink_encoder_in_close(channel[i], ein);
    }
  }
}

/*======================================================*
 * See sfuntmpl.doc for the optional S-function methods *
 *======================================================*/

/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
