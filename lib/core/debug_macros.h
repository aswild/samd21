#ifndef DEBUG_MACROS_H
#define DEBUG_MACROS_H

#if defined(DEBUG_PORT) && defined(DEBUG_PIN)
#define DBGINIT() do { PORT->Group[DEBUG_PORT].DIRSET.reg = 1UL << (DEBUG_PIN); } while (0)
#define DBGHIGH() do { PORT->Group[DEBUG_PORT].OUTSET.reg = 1UL << (DEBUG_PIN); } while (0)
#define DBGLOW()  do { PORT->Group[DEBUG_PORT].OUTCLR.reg = 1UL << (DEBUG_PIN); } while (0)
#else
#define DBGINIT() do { } while(0)
#define DBGHIGH() do { } while(0)
#define DBGLOW()  do { } while(0)
#endif

#endif
