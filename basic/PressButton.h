#ifndef PressButton_h
#define PressButton_h

#include <Arduino.h>

/*!
* @brief Multi-function and debounce button object
*/

class PressButton
{
private:
    int _IoPin;                // internal value - IO pin
    
public:
    PressButton(int pin);      // Initializer for buttin. Required the pin value.
    int GetIoPin();            // returns the IO pin the button is configured to use
    boolean WasDown = false;   // stores was down state as set by CaptureDownState()
    uint32_t RepeatCnt = 0;    // maintains a count of number of times repeat trigger sent
    uint32_t LastRepeatMs = 0; // keeps a recoreds of when last repeat trigger was sent
    boolean IsDown();          // does a dobounced check to see if the specified button is in the bown state
    boolean IsUp();            // does a debounced check to see if the specified button is in the up state
    boolean CaptureDownState();// will set the WasDOwn flag true if was pressed when checked - also returns WasDown state at the same time
    boolean ClearWasDown();    // clears the was down state, returns true if clear was done
    boolean PressReleased();   // provides a trigger action if the button was pressed but is now released > Clears WasDown State when trigger sent
    boolean LongPressed();     // provides a trigger for long press case
    boolean Repeated();        // provieds and action trigger at a increasingly higher frequecy for as long as a key is held down > Clears WasDown state when trigger sent        
};

#endif