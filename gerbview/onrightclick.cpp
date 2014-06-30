/********************/
/* onrightclick.cpp */
/********************/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <id.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <menus_helpers.h>


/* Prepare the right-click pullup menu.
 * The menu already has a list of zoom commands.
 */
bool GERBVIEW_FRAME::OnRightClick( const wxPoint& aPosition, wxMenu* PopMenu )
{
    GERBER_DRAW_ITEM* DrawStruct = (GERBER_DRAW_ITEM*) GetScreen()->GetCurItem();
    wxString    msg;
    bool        BlockActive = !GetScreen()->m_BlockLocate.IsIdle();
    bool        busy = DrawStruct && DrawStruct->GetFlags();

    // Do not initiate a start block validation on menu.
    m_canvas->SetCanStartBlock( -1 );

    // Simple location of elements where possible.
    if( !busy )
    {
        DrawStruct = Locate( aPosition, CURSEUR_OFF_GRILLE );
        busy = DrawStruct && DrawStruct->GetFlags();
    }

    // If command in progress, end command.
    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( busy )
            AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                         _( "Cancel" ), KiBitmap( cancel_xpm )  );
        else
            AddMenuItem( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                         _( "End Tool" ), KiBitmap( cursor_xpm ) );

        PopMenu->AppendSeparator();
    }
    else
    {
        if( busy || BlockActive )
        {
            if( BlockActive )
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel Block" ), KiBitmap( cancel_xpm ) );
                PopMenu->AppendSeparator();
                AddMenuItem( PopMenu, ID_POPUP_PLACE_BLOCK,
                             _( "Place Block" ), KiBitmap( checked_ok_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_DELETE_BLOCK,
                             _( "Delete Block (ctrl + drag mouse)" ), KiBitmap( delete_xpm ) );
            }
            else
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel" ), KiBitmap( cancel_xpm ) );
            }

            PopMenu->AppendSeparator();
        }
    }

    if( BlockActive )
        return true;

    if( DrawStruct )
        GetScreen()->SetCurItem( DrawStruct );

    return true;
}
