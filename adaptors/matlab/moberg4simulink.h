/*
    moberg4simulink.h -- moberg interface for simulink MEX functions

    Copyright (C) 2019 Anders Blomdell <anders.blomdell@gmail.com>

    This file is part of Moberg.

    Moberg is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
