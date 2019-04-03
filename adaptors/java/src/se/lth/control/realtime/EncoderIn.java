/**
 * se.lth.control.realtime.EncoderIn.java
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

package se.lth.control.realtime;

import java.util.BitSet;

import se.lth.control.realtime.moberg.Moberg;

public class EncoderIn extends IOChannel {
  
  public EncoderIn(int index) throws IOChannelException {
    super(index);
  }

  protected void open() throws IOChannelException {
     Moberg.encoderInOpen(index);
  }

  protected void close() throws IOChannelException {
    Moberg.encoderInClose(index);
  }

  @Deprecated
  public long get() throws IOChannelException {
    return Moberg.encoderIn(index);
  }
    
  public long read() throws IOChannelException {
    return Moberg.encoderIn(index);
  }

}
