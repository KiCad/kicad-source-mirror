/******************************************************************/
/* onleftclick.cpp: functions called on left or double left click */
/******************************************************************/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <common.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <class_GERBER.h>
#include <dialog_helpers.h>
#include <class_DCodeSelectionbox.h>

/* Process the command triggered by the left button of the mouse when a tool
 * is already selected.
 */
void GERBVIEW_FRAME::OnLeftClick( wxDC* DC, const wxPoint& aPosition )
{
    GERBER_DRAW_ITEM* DrawStruct = (GERBER_DRAW_ITEM*) GetScreen()->GetCurItem();
    wxString    msg;

    if( GetToolId() == ID_NO_TOOL_SELECTED )
    {
        if( DrawStruct && DrawStruct->GetFlags() )
        {
            msg.Printf( wxT( "GERBVIEW_FRAME::OnLeftClick err: Struct %u, m_Flags = %X" ),
                        (unsigned) DrawStruct->Type(),
                        (unsigned) DrawStruct->GetFlags() );
            wxFAIL_MSG( msg );
        }
        else
        {
            DrawStruct = Locate( aPosition, CURSEUR_OFF_GRILLE );
            GetScreen()->SetCurItem( DrawStruct );
            if( DrawStruct == NULL )
            {
                GERBER_IMAGE* gerber = g_GERBER_List[getActiveLayer() ];
                if( gerber )
                    gerber->DisplayImageInfo( );
            }
        }
    }

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        break;

        if( DrawStruct == NULL )
            break;

    default:
        wxFAIL_MSG( wxT( "GERBVIEW_FRAME::ProcessCommand error" ) );
        break;
    }
}


/* Called on a double click of left mouse button.
 */
void GERBVIEW_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& aPosition )
{
    // Currently: no nothing
}
