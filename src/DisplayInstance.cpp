#include "DisplayInstance.h"

// Instantiate the display here once.
// Adjust pins (CS, DC, RST, BUSY) to match your hardware
DisplayType display(GxEPD2_420c_GDEY042Z98(/*CS=*/ 5, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));


