/**
 * se_lth_control_realtime_moberg_Moberg.c
 *
 * Copyright (C) 2005-2019  Anders Blomdell <anders.blomdell@control.lth.se>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jni.h>
#include "se_lth_control_realtime_moberg_Moberg.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <moberg.h>

static inline int OK(struct moberg_status status)
{
  return moberg_OK(status);
}

static void throwMoberg(JNIEnv *env, int chan, char *exceptionName)
{
  jclass exceptionClass = 0;
  jmethodID exceptionClassInit = 0;
  jobject exception = 0;
  jint result = -1;

  exceptionClass = 
    (*env)->FindClass(env, exceptionName);
  if (exceptionClass) {
     exceptionClassInit = (*env)->GetMethodID(env, exceptionClass,
					      "<init>", "(I)V");
  }
  if (exceptionClassInit) {
    exception = (*env)->NewObject(env, exceptionClass, exceptionClassInit, 
				  chan);
  }
  if (exception) {
    result = (*env)->Throw(env, exception);
  }
  if (result != 0) {
    fprintf(stderr, "%s: %d\n", exceptionName, chan);
  }
}

static void throwMobergNotOpenException(JNIEnv *env, int chan)
{
  throwMoberg(env, chan, "se/lth/control/realtime/moberg/MobergNotOpenException");		      
}

static void throwMobergDeviceDoesNotExistException(JNIEnv *env, int chan)
{
  throwMoberg(env, chan, "se/lth/control/realtime/moberg/MobergDeviceDoesNotExistException");		      
}

#if 0
static void throwMobergDeviceReadException(JNIEnv *env, int chan)
{
  throwMoberg(env, chan, "se/lth/control/realtime/moberg/MobergDeviceReadException");		      
}

static void throwMobergDeviceWriteException(JNIEnv *env, int chan)
{
  throwMoberg(env, chan, "se/lth/control/realtime/moberg/MobergDeviceWriteException");		      
}

static void throwMobergRangeNotFoundException(JNIEnv *env, int chan)
{
  throwMoberg(env, chan, "se/lth/control/realtime/moberg/MobergRangeNotFoundException");		      
}

static void throwMobergConfigurationFileNotFoundError(JNIEnv *env)
{
  throwMoberg(env, 0, "se/lth/control/realtime/moberg/MobergConfigurationFileNotFoundError");		      
}
#endif

static struct list {
  int capacity;
  struct channel {
    int count;
    union {
      struct moberg_analog_in analog_in;
      struct moberg_analog_out analog_out;
      struct moberg_digital_in digital_in;
      struct moberg_digital_out digital_out;
      struct moberg_encoder_in encoder_in;
    };
  } *channel;
} analog_in = { .capacity=0, .channel=NULL },
  analog_out = { .capacity=0, .channel=NULL },
  digital_in = { .capacity=0, .channel=NULL },
  digital_out = { .capacity=0, .channel=NULL },
  encoder_in = { .capacity=0, .channel=NULL };

struct {
  int count;
  struct moberg *moberg;
} g_moberg = { 0, NULL };

static int up()
{
  if (g_moberg.count <= 0) {
    g_moberg.moberg = moberg_new(NULL);
  }
  g_moberg.count++;
  return 0;
}

static int down()
{
  g_moberg.count--;
  if (g_moberg.count <= 0) {
    moberg_free(g_moberg.moberg);
    g_moberg.moberg = NULL;
  }
  return 0;
}

static struct channel *channel_get(struct list *list, int index)
{
  if (index < list->capacity && list->channel[index].count > 0) {
    return &list->channel[index];
  }
  return NULL;
}

static int channel_set(struct list *list, int index, struct channel channel)
{
  if (list->capacity <= index) {
    int capacity = index + 1;
    void *new = realloc(list->channel, capacity * sizeof(*list->channel));
    if (new) {
      void *p = new + list->capacity * sizeof(*list->channel);
      memset(p, 0, (capacity - list->capacity) * sizeof(*list->channel));
      list->channel = new;
      list->capacity = capacity;
    }
  }
  if (0 <= index && index < list->capacity) {
    list->channel[index] = channel;
    return 1;
  } else {
    return 0;
  }
}

static int channel_up(struct list *list, int index)
{
  struct channel *channel = channel_get(list, index);
  if (channel) {
    up();
    channel->count++;
    return 1;
  }
  return 0;
}

static struct channel *channel_down(struct list *list, int index)
{
  struct channel *channel = channel_get(list, index);
  if (channel) {
    down();
    channel->count--;
    return channel;
  }
  return NULL;
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_analogInOpen(
  JNIEnv *env, jclass obj, jint index
)
{
  if (! channel_up(&analog_in, index)) {
    struct channel channel;
    up();
    if (! OK(moberg_analog_in_open(g_moberg.moberg, index,
                                   &channel.analog_in))) {
      down();
      throwMobergDeviceDoesNotExistException(env, index);      
    } else {
      channel.count = 1;
      channel_set(&analog_in, index, channel);
    }
  }
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_analogInClose(
  JNIEnv *env, jclass obj, jint index
)
{
  struct channel *channel = channel_down(&analog_in, index);
  if (! channel) {
    throwMobergDeviceDoesNotExistException(env, index);
  } else if (channel->count == 0) {
    moberg_analog_in_close(g_moberg.moberg, index, channel->analog_in);
  }
}


JNIEXPORT jdouble JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_analogIn(
  JNIEnv *env, jobject obj, jint index
) 
{
  double result = 0.0;

  struct channel *channel = channel_get(&analog_in, index);
  if (! channel) {
    throwMobergNotOpenException(env, index);
  } else {
    channel->analog_in.read(channel->analog_in.context, &result);
  }
  return result;
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_analogOutOpen(
  JNIEnv *env, jclass obj, jint index
)
{
  if (! channel_up(&analog_out, index)) {
    struct channel channel;
    up();
    if (! OK(moberg_analog_out_open(g_moberg.moberg, index,
                                    &channel.analog_out))) {
      down();
      throwMobergDeviceDoesNotExistException(env, index);      
    } else {
      channel.count = 1;
      channel_set(&analog_out, index, channel);
    }
  }
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_analogOutClose(
  JNIEnv *env, jclass obj, jint index
)
{
  struct channel *channel = channel_down(&analog_out, index);
  if (! channel) {
    throwMobergDeviceDoesNotExistException(env, index);
  } else if (channel->count == 0) {
    moberg_analog_out_close(g_moberg.moberg, index, channel->analog_out);
  }
}


JNIEXPORT double JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_analogOut(
  JNIEnv *env, jobject obj, jint index, jdouble desired
) 
{
  struct channel *channel = channel_get(&analog_out, index);
  if (! channel) {
    throwMobergNotOpenException(env, index);
    return 0.0;
  } else {
    double actual;
    channel->analog_out.write(channel->analog_out.context, desired, &actual);
    return actual;
  }
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_digitalInOpen(
  JNIEnv *env, jclass obj, jint index
)
{
  if (! channel_up(&digital_in, index)) {
    struct channel channel;
    up();
    if (! OK(moberg_digital_in_open(g_moberg.moberg, index,
                                    &channel.digital_in))) {
      down();
      throwMobergDeviceDoesNotExistException(env, index);      
    } else {
      channel.count = 1;
      channel_set(&digital_in, index, channel);
    }
  }
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_digitalInClose(
  JNIEnv *env, jclass obj, jint index
)
{
  struct channel *channel = channel_down(&digital_in, index);
  if (! channel) {
    throwMobergDeviceDoesNotExistException(env, index);
  } else if (channel->count == 0) {
    moberg_digital_in_close(g_moberg.moberg, index, channel->digital_in);
  }
}

JNIEXPORT jboolean JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_digitalIn(
  JNIEnv *env, jobject obj, jint index
) 
{
  int result = 0;

  struct channel *channel = channel_get(&digital_in, index);
  if (! channel) {
    throwMobergNotOpenException(env, index);
  } else {
    channel->digital_in.read(channel->digital_in.context, &result);
  }
  return result;
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_digitalOutOpen(
  JNIEnv *env, jclass obj, jint index
)
{
  if (! channel_up(&digital_out, index)) {
    struct channel channel;
    up();
    if (! OK(moberg_digital_out_open(g_moberg.moberg, index,
                                     &channel.digital_out))) {
      down();
      throwMobergDeviceDoesNotExistException(env, index);      
    } else {
      channel.count = 1;
      channel_set(&digital_out, index, channel);
    }
  }
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_digitalOutClose(
  JNIEnv *env, jclass obj, jint index
)
{
  struct channel *channel = channel_down(&digital_out, index);
  if (! channel) {
    throwMobergDeviceDoesNotExistException(env, index);
  } else if (channel->count == 0) {
    moberg_digital_out_close(g_moberg.moberg, index, channel->digital_out);
  }
}

JNIEXPORT jboolean JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_digitalOut(
  JNIEnv *env, jobject obj, jint index, jboolean desired
) 
{
  struct channel *channel = channel_get(&digital_out, index);
  if (! channel) {
    throwMobergNotOpenException(env, index);
    return 0;
  } else {
    int actual;
    channel->digital_out.write(channel->digital_out.context, desired, &actual);
    return actual;
  }
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_encoderInOpen(
  JNIEnv *env, jclass obj, jint index
)
{
  if (! channel_up(&encoder_in, index)) {
    struct channel channel;
    up();
    if (! OK(moberg_encoder_in_open(g_moberg.moberg, index,
                                    &channel.encoder_in))) {
      down();
      throwMobergDeviceDoesNotExistException(env, index);      
    } else {
      channel.count = 1;
      channel_set(&encoder_in, index, channel);
    }
  }
}

JNIEXPORT void JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_encoderInClose(
  JNIEnv *env, jclass obj, jint index
)
{
  struct channel *channel = channel_down(&encoder_in, index);
  if (! channel) {
    throwMobergDeviceDoesNotExistException(env, index);
  } else if (channel->count == 0) {
    moberg_encoder_in_close(g_moberg.moberg, index, channel->encoder_in);
  }
}

JNIEXPORT jlong JNICALL 
Java_se_lth_control_realtime_moberg_Moberg_encoderIn(
  JNIEnv *env, jobject obj, jint index
) 
{
  long result = 0;

  struct channel *channel = channel_get(&encoder_in, index);
  if (! channel) {
    throwMobergNotOpenException(env, index);
  } else {
    channel->encoder_in.read(channel->encoder_in.context, &result);
  }
  return result;
}

JNIEXPORT void JNICALL
Java_se_lth_control_realtime_moberg_Moberg_Init(
  JNIEnv *env, jobject obj
)
{
}
