/*
  analogout.c, 
  a MEX file for writing analog output via Moberg.
  
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
#define S_FUNCTION_NAME  analogout

#include <uchar.h>  /* for CHAR16_T typedef in tmwtypes.h */
#include "simstruc.h"
#include <moberg4simulink.h>

/*
  Usage of work vectors:

  PWork:    0           moberg_analog_out pointer[0]
            1           moberg_analog_out pointer[1]
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
      ssSetErrorStatus(S, "output channels must be a scalar or a vector");
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

  if (!ssSetNumInputPorts(S, 2)) return;
  ssSetInputPortWidth(S, 0, 1);
  ssSetInputPortDirectFeedThrough(S, 0, 1);
  ssSetInputPortWidth(S, 1, channelCount);
  ssSetInputPortDirectFeedThrough(S, 1, 1);
  
  if (!ssSetNumOutputPorts(S, 1)) return;
  ssSetOutputPortWidth(S, 0, 1);

  ssSetNumSampleTimes(S, 1);
  ssSetNumPWork(S, channelCount);     /* 0:      moberg_analog_out pointer[0]
                                         1:      moberg_analog_out pointer[1]
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
    pwork[i] = moberg4simulink_analog_out_open(channel[i]);
    if (! pwork[i]) {
      static char error[256];
      sprintf(error, "Failed to open analogout #%d", (int)channel[i]);
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
    InputRealPtrsType up = ssGetInputPortRealSignalPtrs(S,1);

    for (i = 0 ; i < ssGetNumPWork(S) ; i++) {
      struct moberg_analog_out *aout = (struct moberg_analog_out*)pwork[i];
      if (! moberg_OK(aout->write(aout->context, *up[i], NULL))) {
        static char error[256];
        double *channel = mxGetPr(ssGetSFcnParam(S,1));
        sprintf(error, "Failed to write analogout #%d", (int)channel[i]);
        ssSetErrorStatus(S, error);
      }
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
      struct moberg_analog_out *aout = (struct moberg_analog_out*)pwork[i];
      moberg4simulink_analog_out_close(channel[i], aout);
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
