/* dummy_functions.cpp
 *
 *  There are functions used in some classes.
 *  they are useful in pcbnew, but have no meaning or are never used
 *  in cvpcb or gerbview.
 *  but they must exist because they appear in some classes, and here, no nothing.
 */
#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

TRACK* Marque_Une_Piste( BOARD* aPcb,
                         TRACK* aStartSegm,
                         int*   aSegmCount,
                         int*   aTrackLen,
                         bool   aReorder )
{
    return NULL;
}

