/*
 *  events_called_functions.cpp
 *  some events functions
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "general.h"
#include "kicad_device_context.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "protos.h"
#include "class_sch_component.h"
#include "class_text-label.h"


/** Event function WinEDA_SchematicFrame::OnCopySchematicItemRequest
 * duplicate the current located item
 */
void WinEDA_SchematicFrame::OnCopySchematicItemRequest( wxCommandEvent& event )
{
    SCH_ITEM * curr_item = GetScreen()->GetCurItem();

    if( !curr_item || curr_item->m_Flags )
        return;

    INSTALL_DC( dc, DrawPanel );

    switch( curr_item->Type() )
    {
    case TYPE_SCH_COMPONENT:
    {
        SCH_COMPONENT* newitem;
        newitem = ((SCH_COMPONENT*) curr_item)->GenCopy();
        newitem->m_TimeStamp = GetTimeStamp();
        newitem->ClearAnnotation( NULL );
        newitem->m_Flags = IS_NEW;
        StartMovePart( newitem, &dc );

        /* Redraw the original part, because StartMovePart() erased
         * it from screen */
        RedrawOneStruct( DrawPanel, &dc, curr_item, GR_DEFAULT_DRAWMODE );
    }
    break;

    case TYPE_SCH_TEXT:
    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    {
        SCH_TEXT* newitem = ((SCH_TEXT*) curr_item)->GenCopy();
        newitem->m_Flags = IS_NEW;
        StartMoveTexte( newitem, &dc );
        /* Redraw the original part in XOR mode */
        RedrawOneStruct( DrawPanel, &dc, curr_item, g_XorMode );
    }
        break;

    default:
        break;
    }
}
