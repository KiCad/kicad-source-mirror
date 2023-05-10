/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <board.h>
#include <board_design_settings.h>
#include <panel_setup_formatting.h>
#include <widgets/paged_dialog.h>
#include <wx/treebook.h>


PANEL_SETUP_FORMATTING::PANEL_SETUP_FORMATTING( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_FORMATTING_BASE( aParentWindow ),
        m_frame( aFrame )
{
    wxSize minSize = m_dashLengthCtrl->GetMinSize();
    int    minWidth = m_dashLengthCtrl->GetTextExtent( wxT( "XXX.XXX" ) ).GetWidth();

    m_dashLengthCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
    m_gapLengthCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

    m_dashedLineHelp->SetFont( KIUI::GetInfoFont( this ).Italic() );
}


bool PANEL_SETUP_FORMATTING::TransferDataToWindow()
{
    const PCB_PLOT_PARAMS& settings = m_frame->GetBoard()->GetPlotOptions();

    m_dashLengthCtrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                                     settings.GetDashedLineDashRatio() ) );

    m_gapLengthCtrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                                    settings.GetDashedLineGapRatio() ) );

    return true;
}


bool PANEL_SETUP_FORMATTING::TransferDataFromWindow()
{
    PCB_PLOT_PARAMS settings = m_frame->GetBoard()->GetPlotOptions();

    settings.SetDashedLineDashRatio( EDA_UNIT_UTILS::UI::DoubleValueFromString( m_dashLengthCtrl->GetValue() ) );
    settings.SetDashedLineGapRatio( EDA_UNIT_UTILS::UI::DoubleValueFromString( m_gapLengthCtrl->GetValue() ) );

    m_frame->GetBoard()->SetPlotOptions( settings );

    KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();

    view->GetPainter()->GetSettings()->SetDashLengthRatio( settings.GetDashedLineDashRatio() );
    view->GetPainter()->GetSettings()->SetGapLengthRatio( settings.GetDashedLineGapRatio() );

    view->UpdateAllItemsConditionally( KIGFX::REPAINT,
            [] ( KIGFX::VIEW_ITEM* aItem ) -> bool
            {
                const EDA_ITEM* item = dynamic_cast<const EDA_ITEM*>( aItem );
                return item && item->Type() == PCB_SHAPE_T;
            } );
    m_frame->GetCanvas()->Refresh();

    return true;
}


void PANEL_SETUP_FORMATTING::ImportSettingsFrom( BOARD* aBoard )
{
    const PCB_PLOT_PARAMS& importedSettings = aBoard->GetPlotOptions();

    m_dashLengthCtrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                                     importedSettings.GetDashedLineDashRatio() ) );

    m_gapLengthCtrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                                    importedSettings.GetDashedLineGapRatio() ) );
}
