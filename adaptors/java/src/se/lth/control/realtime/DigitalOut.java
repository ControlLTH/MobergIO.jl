/**
 * se.lth.control.realtime.DigitalOut.java
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

public class DigitalOut extends IOChannel {

  public DigitalOut(int index) throws IOChannelException {
    super(index);
  }
   
  public void open() throws IOChannelException {
    Moberg.digitalOutOpen(index);
  }
  public void close() throws IOChannelException {
    Moberg.digitalOutClose(index);
  }

  public void set(boolean value) throws IOChannelException {
    Moberg.digitalOut(index, value);
  }

}
