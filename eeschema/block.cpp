/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <general.h>
#include <class_library.h>
#include <lib_pin.h>
#include <list_operations.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <list_operations.h>

#include <preview_items/selection_area.h>
#include <sch_view.h>
#include <view/view_group.h>


int SCH_EDIT_FRAME::BlockCommand( EDA_KEY key )
{
    int cmd = BLOCK_IDLE;

    switch( key )
    {
    default:
        cmd = key & 0xFF;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_DUPLICATE;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_DRAG;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


void SCH_EDIT_FRAME::InitBlockPasteInfos()
{
    wxFAIL_MSG( "How did we get here?  Should have gone through modern toolset..." );
    return;
}


void SCH_EDIT_FRAME::HandleBlockPlace( wxDC* DC )
{
    wxFAIL_MSG( "How did we get here?  Should have gone through modern toolset..." );
    return;
}


bool SCH_EDIT_FRAME::HandleBlockEnd( wxDC* aDC )
{
    wxFAIL_MSG( "How did we get here?  Should have gone through modern toolset..." );
    return false;
}


void DrawAndSizingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                 bool aErase )
{
    auto panel =static_cast<SCH_DRAW_PANEL*>(aPanel);
    auto area = panel->GetView()->GetSelectionArea();
    auto frame = static_cast<EDA_BASE_FRAME*>(aPanel->GetParent());

    BLOCK_SELECTOR* block;
    bool isLibEdit = frame->IsType( FRAME_SCH_LIB_EDITOR );

    block = &aPanel->GetScreen()->m_BlockLocate;
    block->SetMoveVector( wxPoint( 0, 0 ) );
    block->SetLastCursorPosition( aPanel->GetParent()->GetCrossHairPosition( isLibEdit ) );
    block->SetEnd( aPanel->GetParent()->GetCrossHairPosition() );

    panel->GetView()->ClearPreview();
    panel->GetView()->ClearHiddenFlags();

    area->SetOrigin( block->GetOrigin() );;
    area->SetEnd( block->GetEnd() );

    panel->GetView()->SetVisible( area );
    panel->GetView()->Hide( area, false );
    panel->GetView()->Update( area );

    if( block->GetState() == STATE_BLOCK_INIT )
    {
        if( block->GetWidth() || block->GetHeight() )
            // 2nd point exists: the rectangle is not surface anywhere
            block->SetState( STATE_BLOCK_END );
    }
}
