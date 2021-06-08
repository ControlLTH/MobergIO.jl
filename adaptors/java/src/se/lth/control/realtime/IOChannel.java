/**
 * se.lth.control.realtime.moberg.IOChannel.java
 *
 * Copyright (C) 2005-2019  Anders Blomdell <anders.blomdell@control.lth.se>
 * Copyright (C) 2014 Alfred Theorin <alfred.theorin@control.lth.se>
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

import java.util.HashMap;
import java.util.BitSet;

public abstract class IOChannel {

  private static class AllocationMap extends HashMap<String, BitSet> {}
  private static AllocationMap allocations = new AllocationMap();

  public final int index;
  private BitSet allocation;
  private boolean isAllocated = false;

  public IOChannel(int index) throws IOChannelException {
    this.index = index;
    open();
    allocate();
  }

  private BitSet getAllocation() {
    BitSet result = allocation;
    if (result == null) {
      result = (BitSet)allocations.get(this.getClass().getName());
      if (result == null) {
	result = new BitSet();
	allocations.put(this.getClass().getName(), result);
      }
    }
    return result;
  }

  private synchronized void allocate() throws IOChannelException {
    if (getAllocation().get(index)) {
      // Try to free unreferenced channels 
      System.gc();
      System.runFinalization();
    }
    if (getAllocation().get(index)) {
      try {
	close();
      } catch (IOChannelException e) {
      }
      throw new AlreadyAllocatedException(this.getClass().getName(), index);
    }
    getAllocation().set(index);
    isAllocated = true;
  }

  private synchronized void deallocate() {
    if (isAllocated) {
      getAllocation().clear(index);
      try {
        close();
      } catch (IOChannelException e) {
      }
      isAllocated = false;
    }
  }


  protected abstract void open() throws IOChannelException;
  protected abstract void close() throws IOChannelException;

  protected void finalize() throws IOChannelException {
    deallocate();
  }

}
