package se.lth.control.realtime.moberg;


public class MobergRangeNotFoundException extends MobergException {
  
  private static final long serialVersionUID = 1L;
  
  public MobergRangeNotFoundException(int index) {
    super(index, "has no range information");
  }
  
}
