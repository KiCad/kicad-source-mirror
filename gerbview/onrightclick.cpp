/********************/
/* onrightclick.cpp */
/********************/

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "gerbview.h"


/* Prepare the right-click pullup menu.
 * The menu already has a list of zoom commands.
 */
bool GERBVIEW_FRAME::OnRightClick( const wxPoint& aPosition, wxMenu* PopMenu )
{
    BOARD_ITEM* DrawStruct = GetScreen()->GetCurItem();
    wxString    msg;
    bool        BlockActive =
        (GetScreen()->m_BlockLocate.m_Command !=  BLOCK_IDLE);

    // Do not initiate a start block validation on menu.
    DrawPanel->m_CanStartBlock = -1;

    // Simple location of elements where possible.
    if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
    {
        DrawStruct = Locate( aPosition, CURSEUR_OFF_GRILLE );
    }

    // If command in progress, end command.
    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( DrawStruct && DrawStruct->m_Flags )
            AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                         _( "Cancel" ), cancel_xpm  );
        else
            AddMenuItem( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                         _( "End Tool" ), cancel_tool_xpm );

        PopMenu->AppendSeparator();
    }
    else
    {
        if( (DrawStruct && DrawStruct->m_Flags) || BlockActive )
        {
            if( BlockActive )
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel Block" ), cancel_xpm );
                PopMenu->AppendSeparator();
                AddMenuItem( PopMenu, ID_POPUP_PLACE_BLOCK,
                             _( "Place Block" ), apply_xpm );
                AddMenuItem( PopMenu, ID_POPUP_DELETE_BLOCK,
                             _( "Delete Block (ctrl + drag mouse)" ), delete_xpm );
            }
            else
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel" ), cancel_xpm );
            }

            PopMenu->AppendSeparator();
        }
    }

    if( BlockActive )
        return true;

    if( DrawStruct == NULL )
        return true;

    GetScreen()->SetCurItem( DrawStruct );

    return true;
}
