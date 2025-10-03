#include "Arduino.h"
#include "PressButton.h"

// Initializer for buttin. Required the pin value.
PressButton::PressButton(int pin){
    _IoPin = pin;
    pinMode(_IoPin, INPUT_PULLUP);
}

// returns the IO pin the button is configured to use
int PressButton::GetIoPin(){return _IoPin;}     

// does a debounced check to see if the specified button is in the down state
boolean PressButton::IsDown(){return digitalRead(_IoPin) == LOW && digitalRead(_IoPin) == LOW;}     

// does a debounced check to see if the specified button is in the up state
boolean PressButton::IsUp(){return digitalRead(_IoPin) == HIGH && digitalRead(_IoPin) == HIGH;}            

// will set the WasDOwn flag true if was pressed when checked - also returns WasDown state at the same time
boolean PressButton::CaptureDownState(){if (IsDown()) {WasDown = true;} return WasDown;}

// clears the was down state, returns true if clear was done
boolean PressButton::ClearWasDown(){if (WasDown){WasDown = false; return true;} return false;}    

// provides a trigger action if the button was pressed but is now released > Clears WasDown State when trigger sent
boolean PressButton::PressReleased(){if (WasDown && IsUp()) {RepeatCnt = 0; WasDown = false; RepeatCnt = 0; return true;} return false;}

// provides a trigger for long press case
boolean PressButton::LongPressed(){return (Repeated() && RepeatCnt == 2);}     

// provieds and action trigger at a increasingly higher frequecy for as long as a key is held down > Clears WasDown state when trigger sent
boolean PressButton::Repeated(){
    // snapshot the current time
  uint32_t currMs = millis();

  // check if repeat signal should be sent
  if (WasDown &&  (                                                          // button must be down for any cases to be considered > AND
                      RepeatCnt == 0 ||                                        // if this is the first time to repeat > OR
                      (RepeatCnt > 5 && currMs >= (LastRepeatMs + 1000)) ||       // if more than the 5th repeat then lock to 200 ms intervals > OR
                      currMs >= (LastRepeatMs + 250 + (50 * (5 - RepeatCnt))) // otherwise calculate a proportional interval to check against
  )) {
    if (RepeatCnt < 999) {RepeatCnt += 1;} // increase the repeat count limiting to max value 99 to avoid roll over
    WasDown = false;                       // clear the was down state to get the next repeat
    LastRepeatMs = currMs;                  // take note of the last repeat time to be used in next comparison
    return true;                             // send back a positive trigger

  } else {
    // has repeated and button is now up, then clear the repeat count and was down state
   
    if (RepeatCnt > 0 && IsUp()) { RepeatCnt = 0; WasDown = false;}
    // Send back a negative trigger
    return false;
  }
}          