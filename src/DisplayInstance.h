#ifndef DISPLAY_INSTANCE_H
#define DISPLAY_INSTANCE_H

#include <GxEPD2_3C.h>

// 1. Set the page height here. 
// Changing HEIGHT to (HEIGHT / 4) saves ~22KB of RAM and fixes your crash!
#define MAX_DISPLAY_BUFFER_HEIGHT (GxEPD2_420c_GDEY042Z98::HEIGHT /4)


// 2. Create a Type Alias to keep things clean
typedef GxEPD2_3C<GxEPD2_420c_GDEY042Z98, MAX_DISPLAY_BUFFER_HEIGHT> DisplayType;

// 3. Declare the display as extern
extern DisplayType display;


#endif // DISPLAY_INSTANCE_H