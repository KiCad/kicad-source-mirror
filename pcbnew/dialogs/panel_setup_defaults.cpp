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

#include "panel_setup_defaults.h"

#include <board_design_settings.h>
#include <dialogs/panel_setup_text_and_graphics.h>
#include <dialogs/panel_setup_dimensions.h>
#include <dialogs/panel_setup_zones.h>
#include <pcb_edit_frame.h>

#include <kidialog.h>


PANEL_SETUP_DEFAULTS::PANEL_SETUP_DEFAULTS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_DEFAULTS_BASE( aParentWindow ),
        m_Frame( aFrame ),
        m_BrdSettings( &m_Frame->GetBoard()->GetDesignSettings() ),
        m_textAndGraphicsPanel( new PANEL_SETUP_TEXT_AND_GRAPHICS( m_scrolledWindow, aFrame, m_BrdSettings ) ),
        m_dimensionsPanel( new PANEL_SETUP_DIMENSIONS( m_scrolledWindow, *aFrame, *m_BrdSettings ) ),
        m_zonesPanel( new PANEL_SETUP_ZONES( m_scrolledWindow, aFrame, *m_BrdSettings ) )
{
    m_scrollSizer->Add( m_textAndGraphicsPanel, 0, wxEXPAND, 5 );
    m_scrollSizer->AddSpacer( 10 );
    m_scrollSizer->Add( m_dimensionsPanel, 0, wxEXPAND, 5 );
    m_scrollSizer->AddSpacer( 10 );
    m_scrollSizer->Add( m_zonesPanel, 0, wxEXPAND, 5 );

	Layout();
	GetSizer()->Fit( this );
}


bool PANEL_SETUP_DEFAULTS::TransferDataToWindow()
{
    m_textAndGraphicsPanel->TransferDataToWindow();
    m_dimensionsPanel->TransferDataToWindow();
    m_zonesPanel->TransferDataToWindow();

    Layout();

    return true;
}


bool PANEL_SETUP_DEFAULTS::TransferDataFromWindow()
{
    if( !m_textAndGraphicsPanel->TransferDataFromWindow() )
        return false;

    if( !m_dimensionsPanel->TransferDataFromWindow() )
        return false;

    if( !m_zonesPanel->TransferDataFromWindow() )
        return false;

    return true;
}


void PANEL_SETUP_DEFAULTS::ImportSettingsFrom( BOARD* aBoard )
{
    if( !m_textAndGraphicsPanel->CommitPendingChanges() )
        return;

    if( !m_dimensionsPanel->CommitPendingChanges() )
        return;

    if( !m_zonesPanel->CommitPendingChanges() )
        return;

    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}
