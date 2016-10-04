#include "main.h"
#include <stdbool.h>

extern int pre_touch_y;
extern int pre_touch_x;
extern float slide_value_hold;
extern float slide_value;
extern SceTouchData touch;
extern bool touch_nothing;

// State_Touch is global value to certain function TOUCH_FRONT alway return absolute one value for one time.
extern int State_Touch;

extern bool have_touch_hold;
extern bool have_touch;

extern bool disable_touch;

// Output:
//	0 : Touch down select
//	1 : Touch up select
//	2 : Double touch (temporary disable)
//  3 : Hold Touch
//  4 : Empty slot
//	5 : Empty slot
//	6 : Touch slide up
//	7 : Touch slide down
//  8 : Touch slide left
//  9 : Touch slide right
// 10 : Two finger slide right
// 11 : Empty slot
// 12 : Empty slot
// 13 : Empty slot
// 14 : Three finger slide left
// 15 : Three finger slide right
int TOUCH_FRONT();