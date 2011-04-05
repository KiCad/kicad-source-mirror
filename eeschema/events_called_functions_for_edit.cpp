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
#include "sch_component.h"
#include "sch_text.h"


/** Event function SCH_EDIT_FRAME::OnCopySchematicItemRequest
 * duplicate the current located item
 */
void SCH_EDIT_FRAME::OnCopySchematicItemRequest( wxCommandEvent& event )
{
    SCH_ITEM * curr_item = GetScreen()->GetCurItem();

    if( !curr_item || curr_item->m_Flags )
        return;

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

    switch( curr_item->Type() )
    {
    case SCH_COMPONENT_T:
    {
        SCH_COMPONENT* newitem;
        newitem = new SCH_COMPONENT( *( (SCH_COMPONENT*) curr_item ) );
        newitem->m_TimeStamp = GetTimeStamp();
        newitem->ClearAnnotation( NULL );
        newitem->m_Flags = IS_NEW;
        StartMovePart( newitem, &dc );

        /* Redraw the original part, because StartMovePart() erased
         * it from screen */
        curr_item->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    }
    break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    {
        SCH_TEXT* newitem = (SCH_TEXT*) curr_item->Clone();
        newitem->SetFlags( IS_NEW );
        MoveText( newitem, &dc );
        /* Redraw the original part in XOR mode */
        curr_item->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode );
    }
        break;

    default:
        break;
    }
}
