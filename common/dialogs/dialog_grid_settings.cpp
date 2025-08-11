/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <wx/msgdlg.h>
#include <dialogs/dialog_grid_settings.h>
#include <widgets/std_bitmap_button.h>
#include <common.h>
#include <settings/app_settings.h>
#include <eda_draw_frame.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tool/grid_menu.h>
#include <tool/common_tools.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

DIALOG_GRID_SETTINGS::DIALOG_GRID_SETTINGS( wxWindow* aParent, wxWindow* aEventSource,
                                            UNITS_PROVIDER* aProvider, GRID& aGrid ) :
        DIALOG_GRID_SETTINGS_BASE( aParent ),
        m_unitsProvider( aProvider ), m_grid( aGrid ),
        m_gridSizeX( aProvider, aEventSource, m_staticTextX, m_textX, m_staticTextXUnits, true ),
        m_gridSizeY( aProvider, aEventSource, m_staticTextY, m_textY, m_staticTextYUnits, true )
{
    // Properties dialogs don't really want state-saving/restoring
    OptOut( this );

    SetupStandardButtons();
    SetInitialFocus( m_textName );

    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_GRID_SETTINGS::TransferDataToWindow()
{
    if( !m_grid.x.IsEmpty() )
    {
        bool     linked = ( m_grid.x == m_grid.y );
        VECTOR2D grid = m_grid.ToDouble( m_unitsProvider->GetIuScale() );

        m_textName->SetValue( m_grid.name );
        m_checkLinked->SetValue( linked );
        m_gridSizeX.SetDoubleValue( grid.x );

        if( !linked )
            m_gridSizeY.SetDoubleValue( grid.y );

        m_textY->Enable( !linked );
    }

    return true;
}


bool DIALOG_GRID_SETTINGS::TransferDataFromWindow()
{
    double gridX = m_gridSizeX.GetDoubleValue();

    if( !m_gridSizeX.Validate( 0.001, 1000.0, EDA_UNITS::MM ) )
    {
        wxMessageBox( _( "Grid size X out of range." ), _( "Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    if( !m_checkLinked->IsChecked() && !m_gridSizeY.Validate( 0.001, 1000.0, EDA_UNITS::MM ) )
    {
        wxMessageBox( _( "Grid size Y out of range." ), _( "Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    double gridY = m_checkLinked->IsChecked() ? gridX : m_gridSizeY.GetDoubleValue();

    m_grid.name = m_textName->GetValue();
    // Grid X/Y are always stored in millimeters so we can compare them easily
    m_grid.x = EDA_UNIT_UTILS::UI::StringFromValue( m_unitsProvider->GetIuScale(), EDA_UNITS::MM, gridX );
    m_grid.y = EDA_UNIT_UTILS::UI::StringFromValue( m_unitsProvider->GetIuScale(), EDA_UNITS::MM, gridY );

    return true;
}


void DIALOG_GRID_SETTINGS::OnLinkedChecked( wxCommandEvent& event )
{
    m_textY->Enable( !m_checkLinked->IsChecked() );
}
