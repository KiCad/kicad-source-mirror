/*
 * @file events_called_functions.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <general.h>
#include <kicad_device_context.h>
#include <wxEeschemaStruct.h>

#include <protos.h>
#include <sch_component.h>
#include <sch_text.h>


void SCH_EDIT_FRAME::OnCopySchematicItemRequest( wxCommandEvent& event )
{
    SCH_ITEM * curr_item = GetScreen()->GetCurItem();

    if( !curr_item || curr_item->GetFlags() )
        return;

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    switch( curr_item->Type() )
    {
    case SCH_COMPONENT_T:
    {
        SCH_COMPONENT* newitem;
        newitem = new SCH_COMPONENT( *( (SCH_COMPONENT*) curr_item ) );
        newitem->SetTimeStamp( GetNewTimeStamp() );
        newitem->ClearAnnotation( NULL );
        newitem->SetFlags( IS_NEW );
        MoveItem( (SCH_ITEM*) newitem, &dc );

        // Redraw the original part, because StartMovePart() erased it from screen.
        curr_item->Draw( m_canvas, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    }
    break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    {
        SCH_TEXT* newitem = (SCH_TEXT*) curr_item->Clone();
        newitem->SetFlags( IS_NEW );
        MoveItem( (SCH_ITEM*) newitem, &dc );

        /* Redraw the original part in XOR mode */
        curr_item->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode );
    }
        break;

    default:
        break;
    }
}
