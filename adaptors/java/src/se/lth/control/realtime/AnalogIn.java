/**
 * se.lth.control.realtime.AnalogIn.java
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

import se.lth.control.realtime.moberg.Moberg;

public class AnalogIn extends IOChannel {

  public AnalogIn(final int index) throws IOChannelException {
    super(index);
  }

  protected void open() throws IOChannelException {
    Moberg.analogInOpen(index);
  }

  protected void close() throws IOChannelException {
    Moberg.analogInClose(index);
  }

  @Deprecated
  public double get() throws IOChannelException {
    return Moberg.analogIn(index);
  }

  public double read() throws IOChannelException {
    return Moberg.analogIn(index);
  }

}
