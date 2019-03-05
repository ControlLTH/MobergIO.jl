#ifndef __MOBERG4SIMULINK_H__
#define __MOBERG4SIMULINK_H__

#include <moberg.h>

struct moberg_analog_in *moberg4simulink_analog_in_open(int index);

void moberg4simulink_analog_in_close(int index,
                                     struct moberg_analog_in *analog_in);

struct moberg_analog_out *moberg4simulink_analog_out_open(int index);

void moberg4simulink_analog_out_close(int index,
                                      struct moberg_analog_out *analog_out);

struct moberg_digital_in *moberg4simulink_digital_in_open(int index);

void moberg4simulink_digital_in_close(int index,
                                      struct moberg_digital_in *digital_in);

struct moberg_digital_out *moberg4simulink_digital_out_open(int index);

void moberg4simulink_digital_out_close(int index,
                                       struct moberg_digital_out *digital_out);

struct moberg_encoder_in *moberg4simulink_encoder_in_open(int index);

void moberg4simulink_encoder_in_close(int index,
                                      struct moberg_encoder_in *encoder_in);

#endif
