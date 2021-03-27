/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board_design_settings.h>
#include <panel_edit_options.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_view.h>
#include <pcbnew_settings.h>
#include <ratsnest/ratsnest_view_item.h>
#include <widgets/paged_dialog.h>
#include <footprint_edit_frame.h>

PANEL_EDIT_OPTIONS::PANEL_EDIT_OPTIONS( PCB_BASE_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent ) :
        PANEL_EDIT_OPTIONS_BASE( aParent->GetTreebook() ), m_frame( aFrame )
{
    m_magneticPads->Show( dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame ) != nullptr );
    m_magneticGraphics->Show( dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame ) != nullptr );
    m_flipLeftRight->Show( dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) != nullptr );

#ifdef __WXOSX_MAC__
    m_mouseCmdsOSX->Show( true );
    m_mouseCmdsWinLin->Show( false );
#else
    m_mouseCmdsWinLin->Show( true );
    m_mouseCmdsOSX->Show( false );
#endif

    m_optionsBook->SetSelection( dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) ? 1 : 0 );
}


bool PANEL_EDIT_OPTIONS::TransferDataToWindow()
{
    const PCB_DISPLAY_OPTIONS& displ_opts = m_frame->GetDisplayOptions();
    const PCBNEW_SETTINGS&     general_opts = m_frame->Settings();

    m_segments45OnlyCtrl->SetValue( general_opts.m_Use45DegreeGraphicSegments );

    wxString rotationAngle;
    rotationAngle = AngleToStringDegrees( (double) m_frame->GetRotationAngle() );
    m_rotationAngle->SetValue( rotationAngle );

    if( dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        /* Set display options */
        m_OptDisplayCurvedRatsnestLines->SetValue( displ_opts.m_DisplayRatsnestLinesCurved );
        m_showSelectedRatsnest->SetValue( displ_opts.m_ShowModuleRatsnest );

        m_magneticPadChoice->SetSelection( static_cast<int>( general_opts.m_MagneticItems.pads ) );
        m_magneticTrackChoice->SetSelection( static_cast<int>( general_opts.m_MagneticItems.tracks ) );
        m_magneticGraphicsChoice->SetSelection( !general_opts.m_MagneticItems.graphics );
        m_flipLeftRight->SetValue( general_opts.m_FlipLeftRight );

        switch( general_opts.m_TrackDragAction )
        {
        case TRACK_DRAG_ACTION::MOVE:            m_rbTrackDragMove->SetValue( true ); break;
        case TRACK_DRAG_ACTION::DRAG:            m_rbTrackDrag45->SetValue( true );   break;
        case TRACK_DRAG_ACTION::DRAG_FREE_ANGLE: m_rbTrackDragFree->SetValue( true ); break;
        }

        m_Show_Page_Limits->SetValue( m_frame->ShowPageLimits() );
        m_Auto_Refill_Zones->SetValue( general_opts.m_AutoRefillZones );
        m_Allow_Free_Pads->SetValue( general_opts.m_AllowFreePads );
    }
    else if( dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame ) )
    {
        m_magneticPads->SetValue( m_frame->GetMagneticItemsSettings()->pads
                                                            == MAGNETIC_OPTIONS::CAPTURE_ALWAYS );
        m_magneticGraphics->SetValue( m_frame->GetMagneticItemsSettings()->graphics );
    }

    return true;
}


bool PANEL_EDIT_OPTIONS::TransferDataFromWindow()
{
    PCB_DISPLAY_OPTIONS displ_opts = m_frame->GetDisplayOptions();

    m_frame->SetRotationAngle( wxRound( 10.0 * wxAtof( m_rotationAngle->GetValue() ) ) );

    m_frame->Settings().m_Use45DegreeGraphicSegments = m_segments45OnlyCtrl->GetValue();

    if( dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        PCBNEW_SETTINGS& pcbnewSettings = m_frame->Settings();

        displ_opts.m_DisplayRatsnestLinesCurved = m_OptDisplayCurvedRatsnestLines->GetValue();
        displ_opts.m_ShowModuleRatsnest = m_showSelectedRatsnest->GetValue();

        m_frame->Settings().m_MagneticItems.pads =
                static_cast<MAGNETIC_OPTIONS>( m_magneticPadChoice->GetSelection() );
        m_frame->Settings().m_MagneticItems.tracks =
                static_cast<MAGNETIC_OPTIONS>( m_magneticTrackChoice->GetSelection() );
        m_frame->Settings().m_MagneticItems.graphics = !m_magneticGraphicsChoice->GetSelection();

        m_frame->Settings().m_FlipLeftRight = m_flipLeftRight->GetValue();
        m_frame->SetShowPageLimits( m_Show_Page_Limits->GetValue() );
        m_frame->Settings().m_AutoRefillZones = m_Auto_Refill_Zones->GetValue();
        m_frame->Settings().m_AllowFreePads = m_Allow_Free_Pads->GetValue();

        if( m_rbTrackDragMove->GetValue() )
            pcbnewSettings.m_TrackDragAction = TRACK_DRAG_ACTION::MOVE;
        else if( m_rbTrackDrag45->GetValue() )
            pcbnewSettings.m_TrackDragAction = TRACK_DRAG_ACTION::DRAG;
        else if( m_rbTrackDragFree->GetValue() )
            pcbnewSettings.m_TrackDragAction = TRACK_DRAG_ACTION::DRAG_FREE_ANGLE;
    }
    else if( dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame ) )
    {
        m_frame->GetMagneticItemsSettings()->pads = m_magneticPads->GetValue()
                                                            ? MAGNETIC_OPTIONS::CAPTURE_ALWAYS
                                                            : MAGNETIC_OPTIONS::NO_EFFECT;

        m_frame->GetMagneticItemsSettings()->graphics = m_magneticGraphics->GetValue();
    }

    // Apply changes to the GAL
    KIGFX::VIEW*                view = m_frame->GetCanvas()->GetView();
    KIGFX::PCB_PAINTER*         painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

    m_frame->SetDisplayOptions( displ_opts, false );
    settings->LoadDisplayOptions( displ_opts, m_frame->ShowPageLimits() );

    view->UpdateAllItemsConditionally( KIGFX::REPAINT,
                                       []( KIGFX::VIEW_ITEM* aItem ) -> bool
                                       {
                                           return dynamic_cast<RATSNEST_VIEW_ITEM*>( aItem );
                                       } );
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    m_frame->GetCanvas()->Refresh();

    return true;
}


