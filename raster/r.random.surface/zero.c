/* zero.c                                                               */

#undef TRACE
#undef DEBUG

#include "ransurf.h"


void ZeroMapCells(void)
{
    int Row, Col;

    FUNCTION(ZeroMapCells);

    for (Row = 0; Row < Rs; Row++) {
	for (Col = 0; Col < Cs; Col++)
	    Surface[Row][Col] = 0.0;
    }
}
