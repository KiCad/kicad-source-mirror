/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/panel_selection_filter.h>


PANEL_SELECTION_FILTER::PANEL_SELECTION_FILTER( wxWindow* aParent ) :
        PANEL_SELECTION_FILTER_BASE( aParent ),
        m_frame( dynamic_cast<PCB_BASE_EDIT_FRAME*>( aParent ) ),
        m_onlyCheckbox( nullptr )
{
    wxFont font = KIUI::GetInfoFont( this );
    m_cbLockedItems->SetFont( font );
    m_cbFootprints->SetFont( font );
    m_cbText->SetFont( font );
    m_cbTracks->SetFont( font );
    m_cbVias->SetFont( font );
    m_cbPads->SetFont( font );
    m_cbGraphics->SetFont( font );
    m_cbZones->SetFont( font );
    m_cbKeepouts->SetFont( font );
    m_cbDimensions->SetFont( font );
    m_cbOtherItems->SetFont( font );
    m_cbAllItems->SetFont( font );

    wxASSERT( m_frame );
    m_tool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    wxASSERT( m_tool );

    SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();
    SetCheckboxesFromFilter( opts );

    m_cbFootprints->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbText->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbTracks->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbVias->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbPads->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbGraphics->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbZones->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbKeepouts->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbDimensions->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbOtherItems->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
}


void PANEL_SELECTION_FILTER::SetCheckboxesFromFilter( SELECTION_FILTER_OPTIONS& aOptions )
{
    Freeze();

    m_cbLockedItems->SetValue( aOptions.lockedItems );
    m_cbFootprints->SetValue( aOptions.footprints );
    m_cbText->SetValue( aOptions.text );
    m_cbTracks->SetValue( aOptions.tracks );
    m_cbVias->SetValue( aOptions.vias );
    m_cbPads->SetValue( aOptions.pads );
    m_cbGraphics->SetValue( aOptions.graphics );
    m_cbZones->SetValue( aOptions.zones );
    m_cbKeepouts->SetValue( aOptions.keepouts );
    m_cbDimensions->SetValue( aOptions.dimensions );
    m_cbOtherItems->SetValue( aOptions.otherItems );

    m_cbAllItems->SetValue( aOptions.All() );

    Thaw();
}


void PANEL_SELECTION_FILTER::OnFilterChanged( wxCommandEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_cbAllItems )
    {
        bool newState = m_cbAllItems->GetValue();

        m_cbFootprints->SetValue( newState );
        m_cbText->SetValue( newState );
        m_cbTracks->SetValue( newState );
        m_cbVias->SetValue( newState );
        m_cbPads->SetValue( newState );
        m_cbGraphics->SetValue( newState );
        m_cbZones->SetValue( newState );
        m_cbKeepouts->SetValue( newState );
        m_cbDimensions->SetValue( newState );
        m_cbOtherItems->SetValue( newState );
    }

    SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();

    // If any of the other checkboxes turned off, turn off the All Items checkbox
    bool allChecked = setFilterFromCheckboxes( opts );
    m_cbAllItems->SetValue( allChecked );
}


bool PANEL_SELECTION_FILTER::setFilterFromCheckboxes( SELECTION_FILTER_OPTIONS& aOptions )
{
    aOptions.lockedItems = m_cbLockedItems->GetValue();
    aOptions.footprints  = m_cbFootprints->GetValue();
    aOptions.text        = m_cbText->GetValue();
    aOptions.tracks      = m_cbTracks->GetValue();
    aOptions.vias        = m_cbVias->GetValue();
    aOptions.pads        = m_cbPads->GetValue();
    aOptions.graphics    = m_cbGraphics->GetValue();
    aOptions.zones       = m_cbZones->GetValue();
    aOptions.keepouts    = m_cbKeepouts->GetValue();
    aOptions.dimensions  = m_cbDimensions->GetValue();
    aOptions.otherItems  = m_cbOtherItems->GetValue();

    return aOptions.All();
}


void PANEL_SELECTION_FILTER::onRightClick( wxMouseEvent& aEvent )
{
    wxMenu menu;

    wxCheckBox* cb = dynamic_cast<wxCheckBox*>( aEvent.GetEventObject() );

    if( !cb )
        return;

    m_onlyCheckbox = cb;

    wxString label;
    label.Printf( _( "Only %s" ),  cb->GetLabel().Lower() );

    menu.Append( new wxMenuItem( &menu, wxID_ANY, label, wxEmptyString, wxITEM_NORMAL ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_SELECTION_FILTER::onPopupSelection, this );

    PopupMenu( &menu );
}


void PANEL_SELECTION_FILTER::onPopupSelection( wxCommandEvent& aEvent )
{
    if( !m_onlyCheckbox )
        return;

    m_cbAllItems->SetValue( false );
    m_cbFootprints->SetValue( false );
    m_cbText->SetValue( false );
    m_cbTracks->SetValue( false );
    m_cbVias->SetValue( false );
    m_cbPads->SetValue( false );
    m_cbGraphics->SetValue( false );
    m_cbZones->SetValue( false );
    m_cbKeepouts->SetValue( false );
    m_cbDimensions->SetValue( false );
    m_cbOtherItems->SetValue( false );

    m_onlyCheckbox->SetValue( true );
    m_onlyCheckbox = nullptr;

    wxCommandEvent dummy;
    OnFilterChanged( dummy );
}
