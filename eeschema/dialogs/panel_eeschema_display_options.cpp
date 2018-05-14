/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_screen.h>
#include <sch_edit_frame.h>
#include <class_libentry.h>
#include <panel_eeschema_display_options.h>


PANEL_EESCHEMA_DISPLAY_OPTIONS::PANEL_EESCHEMA_DISPLAY_OPTIONS( SCH_EDIT_FRAME* aFrame,
                                                                wxWindow* aWindow ) :
        PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( aWindow ),
        m_frame( aFrame )
{}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataToWindow()
{
    const GRIDS& gridSizes = m_frame->GetScreen()->GetGrids();

    for( size_t i = 0; i < gridSizes.size(); i++ )
    {
        m_choiceGridSize->Append( wxString::Format( wxT( "%0.1f" ), gridSizes[i].m_Size.x ) );

        if( gridSizes[i].m_CmdId == m_frame->GetScreen()->GetGridCmdId() )
            m_choiceGridSize->SetSelection( (int) i );
    }

    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    int refStyleSelection;

    switch( LIB_PART::GetSubpartIdSeparator() )
    {
    default:
    case 0:   refStyleSelection = 0; break;
    case '.': refStyleSelection = LIB_PART::GetSubpartFirstId() == '1' ? 4 : 1; break;
    case '-': refStyleSelection = LIB_PART::GetSubpartFirstId() == '1' ? 5 : 2; break;
    case '_': refStyleSelection = LIB_PART::GetSubpartFirstId() == '1' ? 6 : 3; break;
    }

    m_choiceSeparatorRefId->SetSelection( refStyleSelection );

    m_busWidthCtrl->SetValue( StringFromValue( INCHES, GetDefaultBusThickness(), false, true ) );
    m_lineWidthCtrl->SetValue( StringFromValue( INCHES, GetDefaultLineThickness(), false, true ) );
    m_checkShowGrid->SetValue( m_frame->IsGridVisible() );
    m_checkShowHiddenPins->SetValue( m_frame->GetShowAllPins() );
    m_checkPageLimits->SetValue( m_frame->ShowPageLimits() );
    m_footprintPreview->SetValue( m_frame->GetFootprintPreview() );

    return true;
}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    const GRIDS& gridSizes = m_frame->GetScreen()->GetGrids();
    wxRealPoint gridsize = gridSizes[ (size_t) m_choiceGridSize->GetSelection() ].m_Size;
    m_frame->SetLastGridSizeId( m_frame->GetScreen()->SetGrid( gridsize ) );
    m_frame->SetGridVisibility( m_checkShowGrid->GetValue() );

    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    int firstRefId, refSeparator;

    switch( m_choiceSeparatorRefId->GetSelection() )
    {
    default:
    case 0: firstRefId = 'A'; refSeparator = 0; break;
    case 1: firstRefId = 'A'; refSeparator = '.'; break;
    case 2: firstRefId = 'A'; refSeparator = '-'; break;
    case 3: firstRefId = 'A'; refSeparator = '_'; break;
    case 4: firstRefId = '1'; refSeparator = '.'; break;
    case 5: firstRefId = '1'; refSeparator = '-'; break;
    case 6: firstRefId = '1'; refSeparator = '_'; break;
    }

    if( refSeparator != LIB_PART::GetSubpartIdSeparator() ||
        firstRefId != LIB_PART::GetSubpartFirstId() )
    {
        LIB_PART::SetSubpartIdNotation( refSeparator, firstRefId );
        m_frame->SaveProjectSettings( false );
    }

    SetDefaultBusThickness( ValueFromString( INCHES, m_busWidthCtrl->GetValue(), true ) );
    SetDefaultLineThickness( ValueFromString( INCHES, m_lineWidthCtrl->GetValue(), true ) );
    m_frame->SetShowAllPins( m_checkShowHiddenPins->GetValue() );
    m_frame->SetShowPageLimits( m_checkPageLimits->GetValue() );
    m_frame->SetFootprintPreview( m_footprintPreview->GetValue() );

    return true;
}


