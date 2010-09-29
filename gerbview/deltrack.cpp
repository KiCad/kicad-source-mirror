/*********************************************/
/* Edit Track: Erase functions               */
/*********************************************/

#include "fctsys.h"
#include "common.h"
//#include "class_drawpanel.h"

#include "gerbview.h"
#include "class_gerber_draw_item.h"


void WinEDA_GerberFrame::Delete_DCode_Items( wxDC* DC,
                                             int   dcode_value,
                                             int   layer_number )
{
    if( dcode_value < FIRST_DCODE )  // No tool selected
        return;

    BOARD_ITEM* item = GetBoard()->m_Drawings;
    BOARD_ITEM * next;
    for( ; item; item = next )
    {
        next = item->Next();
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;

        if( dcode_value != gerb_item->m_DCode )
            continue;

        if( layer_number >= 0 && layer_number != gerb_item->GetLayer() )
            continue;

//   TODO:     Delete_Item( DC, item );
    }

    GetScreen()->SetCurItem( NULL );
}

