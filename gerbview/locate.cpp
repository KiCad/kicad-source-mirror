/************************************************/
/* Locate items at the current cursor position. */
/************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "gerbview.h"
#include "trigo.h"
#include "protos.h"


/* Display the character of the localized STRUCTURE and return a pointer
 * to it.
 */
BOARD_ITEM* WinEDA_GerberFrame::Locate( int typeloc )
{
    MsgPanel->EraseMsgBox();
    return NULL;
}

