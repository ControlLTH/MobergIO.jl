/**
 * se.lth.control.realtime.moberg.Moberg.java
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

package se.lth.control.realtime.moberg;

import java.io.File;

public class Moberg {
  public static native void analogInOpen(int index) throws MobergException;
  public static native void analogInClose(int index) throws MobergException;
  public static native double analogIn(int index) throws MobergException;

  public static native void analogOutOpen(int index) throws MobergException;
  public static native void analogOutClose(int index) throws MobergException;
  public static native double analogOut(int index, double value) throws MobergException;

  public static native void digitalInOpen(int index) throws MobergException;
  public static native void digitalInClose(int index) throws MobergException;
  public static native boolean digitalIn(int index) throws MobergException;

  public static native void digitalOutOpen(int index) throws MobergException;
  public static native void digitalOutClose(int index) throws MobergException;
  public static native boolean digitalOut(int index, boolean value) throws MobergException;

  public static native void encoderInOpen(int index) throws MobergException;
  public static native void encoderInClose(int index) throws MobergException;
  public static native long encoderIn(int index) throws MobergException;

  private static native void Init();

  static  {
    System.loadLibrary("se_lth_control_realtime_moberg_Moberg");
    Init();
  }
}
