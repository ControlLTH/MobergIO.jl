#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <moberg.h>
#include <moberg4simulink.h>

static struct channel {
  struct channel *next;
  struct channel *prev;
  union {
    struct moberg_analog_in analog_in;
    struct moberg_analog_out analog_out;
    struct moberg_digital_in digital_in;
    struct moberg_digital_out digital_out;
    struct moberg_encoder_in encoder_in;
  };
} analog_in_list={.next=&analog_in_list,
                  .prev=&analog_in_list},
  analog_out_list={.next=&analog_out_list,
                   .prev=&analog_out_list},
  digital_in_list={.next=&digital_in_list,
                   .prev=&digital_in_list},
  digital_out_list={.next=&digital_out_list,
                    .prev=&digital_out_list},
  encoder_in_list={.next=&encoder_in_list,
                   .prev=&encoder_in_list};

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

void list_insert(struct channel *list,
                 struct channel *element)
{
  element->next = list;
  element->prev = list->prev;
  element->prev->next = element;
  element->next->prev = element;
}

void list_remove(struct channel *element)
{
  element->prev->next = element->next;
  element->next->prev = element->prev;
}

struct moberg_analog_in *moberg4simulink_analog_in_open(int index)
{
  up();
  struct channel *result = malloc(sizeof(*result));
  if (result && moberg_analog_in_open(g_moberg.moberg, index,
                                      &result->analog_in)) {
    list_insert(&analog_in_list, result);
    return &result->analog_in;
  } else {
    down();
    return NULL;
  }
}

void moberg4simulink_analog_in_close(int index,
                                     struct moberg_analog_in *analog_in)
{
  struct channel *channel =
    (void*)analog_in - offsetof(struct channel, analog_in);
  moberg_analog_in_close(g_moberg.moberg, index, channel->analog_in);
  list_remove(channel);
  free(channel);
  down();
}

struct moberg_analog_out *moberg4simulink_analog_out_open(int index)
  {
  up();
  struct channel *result = malloc(sizeof(*result));
  if (result && moberg_analog_out_open(g_moberg.moberg, index,
                                      &result->analog_out)) {
    list_insert(&analog_out_list, result);
    return &result->analog_out;
  } else {
    down();
    return NULL;
  }
}

void moberg4simulink_analog_out_close(int index,
                                      struct moberg_analog_out *analog_out)
{
  struct channel *channel =
    (void*)analog_out - offsetof(struct channel, analog_out);
  moberg_analog_out_close(g_moberg.moberg, index, channel->analog_out);
  list_remove(channel);
  free(channel);
  down();
}

struct moberg_digital_in *moberg4simulink_digital_in_open(int index)
{
  up();
  struct channel *result = malloc(sizeof(*result));
  if (result && moberg_digital_in_open(g_moberg.moberg, index,
                                      &result->digital_in)) {
    list_insert(&digital_in_list, result);
    return &result->digital_in;
  } else {
    down();
    return NULL;
  }
}


void moberg4simulink_digital_in_close(int index,
                                      struct moberg_digital_in *digital_in)
{
  struct channel *channel =
    (void*)digital_in - offsetof(struct channel, digital_in);
  moberg_digital_in_close(g_moberg.moberg, index, channel->digital_in);
  list_remove(channel);
  free(channel);
  down();
}

struct moberg_digital_out *moberg4simulink_digital_out_open(int index)
{
  up();
  struct channel *result = malloc(sizeof(*result));
  if (result && moberg_digital_out_open(g_moberg.moberg, index,
                                      &result->digital_out)) {
    list_insert(&digital_out_list, result);
    return &result->digital_out;
  } else {
    down();
    return NULL;
  }
}


void moberg4simulink_digital_out_close(int index,
                                       struct moberg_digital_out *digital_out)
{
  struct channel *channel =
    (void*)digital_out - offsetof(struct channel, digital_out);
  moberg_digital_out_close(g_moberg.moberg, index, channel->digital_out);
  list_remove(channel);
  free(channel);
  down();
}

struct moberg_encoder_in *moberg4simulink_encoder_in_open(int index)
{
  up();
  struct channel *result = malloc(sizeof(*result));
  if (result && moberg_encoder_in_open(g_moberg.moberg, index,
                                       &result->encoder_in)) {
    list_insert(&encoder_in_list, result);
    return &result->encoder_in;
  } else {
    down();
    return NULL;
  }
}


void moberg4simulink_encoder_in_close(int index,
                                      struct moberg_encoder_in *encoder_in)
{
  struct channel *channel =
    (void*)encoder_in - offsetof(struct channel, encoder_in);
  moberg_encoder_in_close(g_moberg.moberg, index, channel->encoder_in);
  list_remove(channel);
  free(channel);
  down();
}

