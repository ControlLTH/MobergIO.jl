package se.lth.control.realtime.moberg;


public class MobergNotOpenException extends MobergException {
  
  private static final long serialVersionUID = 1L;
  
  public MobergNotOpenException(int index) {
    super(index, "is not open");
  }
  
}
