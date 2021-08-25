/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


PANEL_SETUP_FORMATTING::PANEL_SETUP_FORMATTING( wxWindow* aWindow, PCB_EDIT_FRAME* aFrame  ) :
        PANEL_SETUP_FORMATTING_BASE( aWindow ),
        m_frame( aFrame )
{
}

bool PANEL_SETUP_FORMATTING::TransferDataToWindow()
{
    const PCB_PLOT_PARAMS& settings = m_frame->GetBoard()->GetPlotOptions();

    m_dashLengthCtrl->SetValue( StringFromValue( EDA_UNITS::UNSCALED,
                                                 settings.GetDashedLineDashRatio() ) );

    m_gapLengthCtrl->SetValue( StringFromValue( EDA_UNITS::UNSCALED,
                                                settings.GetDashedLineGapRatio() ) );

    return true;
}


bool PANEL_SETUP_FORMATTING::TransferDataFromWindow()
{
    PCB_PLOT_PARAMS settings = m_frame->GetBoard()->GetPlotOptions();

    settings.SetDashedLineDashRatio( DoubleValueFromString( EDA_UNITS::UNSCALED,
                                                            m_dashLengthCtrl->GetValue() ) );

    settings.SetDashedLineGapRatio( DoubleValueFromString( EDA_UNITS::UNSCALED,
                                                           m_gapLengthCtrl->GetValue() ) );

    m_frame->GetBoard()->SetPlotOptions( settings );

    KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();

    view->GetPainter()->GetSettings()->SetDashLengthRatio( settings.GetDashedLineDashRatio() );
    view->GetPainter()->GetSettings()->SetGapLengthRatio( settings.GetDashedLineGapRatio() );
    view->MarkDirty();
    view->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return true;
}


void PANEL_SETUP_FORMATTING::ImportSettingsFrom( BOARD* aBoard )
{
    const PCB_PLOT_PARAMS& importedSettings = aBoard->GetPlotOptions();

    m_dashLengthCtrl->SetValue( StringFromValue( EDA_UNITS::UNSCALED,
                                                 importedSettings.GetDashedLineDashRatio() ) );

    m_gapLengthCtrl->SetValue( StringFromValue( EDA_UNITS::UNSCALED,
                                                importedSettings.GetDashedLineGapRatio() ) );
}
