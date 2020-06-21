/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_board.h>
#include <fctsys.h>
#include <panel_edit_options.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_view.h>
#include <pcbnew.h>
#include <pcbnew_settings.h>
#include <widgets/paged_dialog.h>
#include <footprint_edit_frame.h>

PANEL_EDIT_OPTIONS::PANEL_EDIT_OPTIONS( PCB_BASE_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent ) :
        PANEL_EDIT_OPTIONS_BASE( aParent->GetTreebook() ),
        m_Frame( aFrame )
{
    m_MagneticPads->Show( dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_Frame ) != nullptr );
    m_FlipLeftRight->Show( dynamic_cast<PCB_EDIT_FRAME*>( m_Frame ) != nullptr );\

    m_optionsBook->SetSelection( dynamic_cast<PCB_EDIT_FRAME*>( m_Frame ) ? 1 : 0 );
}


bool PANEL_EDIT_OPTIONS::TransferDataToWindow()
{
    const PCB_DISPLAY_OPTIONS& displ_opts = m_Frame->GetDisplayOptions();
    const PCBNEW_SETTINGS&     general_opts = m_Frame->Settings();

    m_PolarDisplay->SetSelection( m_Frame->GetShowPolarCoords() ? 1 : 0 );
    m_UnitsSelection->SetSelection( m_Frame->GetUserUnits() == EDA_UNITS::INCHES ? 0 : 1 );

    m_Segments_45_Only_Ctrl->SetValue( general_opts.m_Use45DegreeGraphicSegments );

    wxString rotationAngle;
    rotationAngle = AngleToStringDegrees( (double)m_Frame->GetRotationAngle() );
    m_RotationAngle->SetValue( rotationAngle );

    if( dynamic_cast<PCB_EDIT_FRAME*>( m_Frame ) )
    {
        /* Set display options */
        m_OptDisplayCurvedRatsnestLines->SetValue( displ_opts.m_DisplayRatsnestLinesCurved );
        m_showGlobalRatsnest->SetValue( displ_opts.m_ShowGlobalRatsnest );
        m_showSelectedRatsnest->SetValue( displ_opts.m_ShowModuleRatsnest );
        m_OptDisplayCurvedRatsnestLines->SetValue( displ_opts.m_DisplayRatsnestLinesCurved );

        m_magneticPadChoice->SetSelection( static_cast<int>( general_opts.m_MagneticItems.pads ) );
        m_magneticTrackChoice->SetSelection( static_cast<int>( general_opts.m_MagneticItems.tracks ) );
        m_magneticGraphicsChoice->SetSelection( !general_opts.m_MagneticItems.graphics );
        m_FlipLeftRight->SetValue( general_opts.m_FlipLeftRight );

        m_Show_Page_Limits->SetValue( m_Frame->ShowPageLimits() );

        switch( general_opts.m_TrackDragAction )
        {
        case TRACK_DRAG_ACTION::MOVE:            m_rbTrackDragMove->SetValue( true ); break;
        case TRACK_DRAG_ACTION::DRAG:            m_rbTrackDrag45->SetValue( true );   break;
        case TRACK_DRAG_ACTION::DRAG_FREE_ANGLE: m_rbTrackDragFree->SetValue( true ); break;
        }
    }
    else if( dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_Frame ) )
    {
        m_MagneticPads->SetValue(
                m_Frame->GetMagneticItemsSettings()->pads == MAGNETIC_OPTIONS::CAPTURE_ALWAYS );
    }

    return true;
}


bool PANEL_EDIT_OPTIONS::TransferDataFromWindow()
{
    m_Frame->SetShowPolarCoords( m_PolarDisplay->GetSelection() != 0 );
    m_Frame->SetUserUnits(
            m_UnitsSelection->GetSelection() == 0 ? EDA_UNITS::INCHES : EDA_UNITS::MILLIMETRES );

    m_Frame->SetRotationAngle( wxRound( 10.0 * wxAtof( m_RotationAngle->GetValue() ) ) );

    m_Frame->Settings().m_Use45DegreeGraphicSegments = m_Segments_45_Only_Ctrl->GetValue();

    m_Frame->Settings().m_MagneticItems.pads =
            static_cast<MAGNETIC_OPTIONS>( m_magneticPadChoice->GetSelection() );
    m_Frame->Settings().m_MagneticItems.tracks =
            static_cast<MAGNETIC_OPTIONS>( m_magneticTrackChoice->GetSelection() );
    m_Frame->Settings().m_MagneticItems.graphics = !m_magneticGraphicsChoice->GetSelection();

    m_Frame->Settings().m_FlipLeftRight = m_FlipLeftRight->GetValue();

    m_Frame->SetShowPageLimits( m_Show_Page_Limits->GetValue() );

    if( dynamic_cast<PCB_EDIT_FRAME*>( m_Frame ) )
    {
        PCBNEW_SETTINGS& settings = m_Frame->Settings();

        if( m_rbTrackDragMove->GetValue() )
            settings.m_TrackDragAction = TRACK_DRAG_ACTION::MOVE;
        else if( m_rbTrackDrag45->GetValue() )
            settings.m_TrackDragAction = TRACK_DRAG_ACTION::DRAG;
        else if( m_rbTrackDragFree->GetValue() )
            settings.m_TrackDragAction = TRACK_DRAG_ACTION::DRAG_FREE_ANGLE;
    }

    // Apply changes to the GAL
    PCB_DISPLAY_OPTIONS         displ_opts = m_Frame->GetDisplayOptions();
    KIGFX::VIEW*                view = m_Frame->GetCanvas()->GetView();
    KIGFX::PCB_PAINTER*         painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

    displ_opts.m_DisplayRatsnestLinesCurved = m_OptDisplayCurvedRatsnestLines->GetValue();
    displ_opts.m_ShowGlobalRatsnest = m_showGlobalRatsnest->GetValue();
    displ_opts.m_ShowModuleRatsnest = m_showSelectedRatsnest->GetValue();

    m_Frame->SetDisplayOptions( displ_opts );
    settings->LoadDisplayOptions( displ_opts, m_Frame->ShowPageLimits() );
    view->RecacheAllItems();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    m_Frame->GetCanvas()->Refresh();

    return true;
}


