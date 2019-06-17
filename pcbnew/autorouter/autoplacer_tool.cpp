/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see change_log.txt for contributors.
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


#include <board_commit.h>
#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <gal/graphics_abstraction_layer.h>
#include <preview_items/centreline_rect_item.h>
#include <preview_items/two_point_geom_manager.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <view/view_controls.h>

// For frame ToolID values
#include <pcbnew_id.h>

// For action icons
#include <bitmaps.h>

#include <class_board_item.h>
#include <class_module.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>
#include <tools/tool_event_utils.h>

#include <widgets/progress_reporter.h>

#include "ar_autoplacer.h"
#include "autoplacer_tool.h"


AUTOPLACE_TOOL::AUTOPLACE_TOOL() : PCB_TOOL_BASE( "pcbnew.Autoplacer" )
{
}


AUTOPLACE_TOOL::~AUTOPLACE_TOOL()
{
}


// A helper call back function used by autoplace.
// It is called by the autoplacer to update the view, when something must be displayed
// especially each time a footprint is autoplaced,
static PCB_BASE_EDIT_FRAME* fparent;
static int refreshCallback( MODULE* aModule )
{
    if( aModule )
        fparent->GetCanvas()->GetView()->Update( aModule );

    fparent->GetCanvas()->GetView()->MarkDirty();
    fparent->GetCanvas()->Refresh();
    wxSafeYield();  // Give a slice of time to refresh the display

    return 0;
}


int AUTOPLACE_TOOL::autoplace( std::vector<MODULE*>& aModules, bool aPlaceOffboard )
{
    auto overlay = view()->MakeOverlay();

    Activate();

    AR_AUTOPLACER autoplacer( board() );

    BOARD_COMMIT commit( frame() );

    autoplacer.SetOverlay( overlay );
    fparent = frame();
    std::function<int( MODULE* aModule )> callback = refreshCallback;
    autoplacer.SetRefreshCallback( callback );

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( frame(), _( "Autoplace Components" ), 1 ) );

    autoplacer.SetProgressReporter( progressReporter.get() );
    auto result = autoplacer.AutoplaceModules( aModules, &commit, aPlaceOffboard );

    if( result == AR_COMPLETED )
        commit.Push( _( "Autoplace components" ) );
    else
        commit.Revert();

    return 0;
}


int AUTOPLACE_TOOL::autoplaceSelected( const TOOL_EVENT& aEvent )
{
    std::vector<MODULE*> mods;

    for( auto item : selection() )
    {
        if( item->Type() == PCB_MODULE_T )
            mods.push_back( static_cast<MODULE*>( item ) );
    }

    return autoplace( mods, false );
}


int AUTOPLACE_TOOL::autoplaceOffboard( const TOOL_EVENT& aEvent )
{
    std::vector<MODULE*> mods;

    return autoplace( mods, true );
}


void AUTOPLACE_TOOL::setTransitions()
{
    Go( &AUTOPLACE_TOOL::autoplaceSelected, PCB_ACTIONS::autoplaceSelectedComponents.MakeEvent() );
    Go( &AUTOPLACE_TOOL::autoplaceOffboard, PCB_ACTIONS::autoplaceOffboardComponents.MakeEvent() );
}
