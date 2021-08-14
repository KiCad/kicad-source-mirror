/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2020 Kicad Developers, see change_log.txt for contributors.
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
#include <tools/pcb_actions.h>
#include <widgets/infobar.h>
#include <widgets/wx_progress_reporters.h>

#include "ar_autoplacer.h"
#include "autoplace_tool.h"


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

static int refreshCallback( FOOTPRINT* aFootprint )
{
    if( aFootprint )
        fparent->GetCanvas()->GetView()->Update( aFootprint );

    fparent->GetCanvas()->GetView()->MarkDirty();
    fparent->GetCanvas()->Refresh();
    wxSafeYield();  // Give a slice of time to refresh the display

    return 0;
}


int AUTOPLACE_TOOL::autoplace( std::vector<FOOTPRINT*>& aFootprints, bool aPlaceOffboard )
{
    EDA_RECT bbox = board()->GetBoardEdgesBoundingBox();

    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
    {
        wxString msg = wxString::Format( _( "Board edges must be defined on the %s layer." ),
                                         LayerName( Edge_Cuts ) );

        frame()->GetInfoBar()->RemoveAllButtons();
        frame()->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_ERROR );
        return 0;
    }

    Activate();

    AR_AUTOPLACER autoplacer( board() );
    BOARD_COMMIT  commit( frame() );

    std::shared_ptr<KIGFX::VIEW_OVERLAY> overlay = view()->MakeOverlay();
    autoplacer.SetOverlay( overlay );

    fparent = frame();
    std::function<int( FOOTPRINT* aFootprint )> callback = refreshCallback;
    autoplacer.SetRefreshCallback( callback );

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( frame(), _( "Autoplace Components" ), 1 ) );

    autoplacer.SetProgressReporter( progressReporter.get() );
    auto result = autoplacer.AutoplaceFootprints( aFootprints, &commit, aPlaceOffboard );

    if( result == AR_COMPLETED )
        commit.Push( _( "Autoplace components" ) );
    else
        commit.Revert();

    return 0;
}


int AUTOPLACE_TOOL::autoplaceSelected( const TOOL_EVENT& aEvent )
{
    std::vector<FOOTPRINT*> footprints;

    for( EDA_ITEM* item : selection() )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            footprints.push_back( static_cast<FOOTPRINT*>( item ) );
    }

    return autoplace( footprints, false );
}


int AUTOPLACE_TOOL::autoplaceOffboard( const TOOL_EVENT& aEvent )
{
    std::vector<FOOTPRINT*> footprints;

    return autoplace( footprints, true );
}


void AUTOPLACE_TOOL::setTransitions()
{
    Go( &AUTOPLACE_TOOL::autoplaceSelected, PCB_ACTIONS::autoplaceSelectedComponents.MakeEvent() );
    Go( &AUTOPLACE_TOOL::autoplaceOffboard, PCB_ACTIONS::autoplaceOffboardComponents.MakeEvent() );
}
