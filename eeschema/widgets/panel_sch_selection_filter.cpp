/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <project/sch_project_settings.h>
#include <sch_base_frame.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <widgets/panel_sch_selection_filter.h>


PANEL_SCH_SELECTION_FILTER::PANEL_SCH_SELECTION_FILTER( wxWindow* aParent ) :
        PANEL_SCH_SELECTION_FILTER_BASE( aParent ),
        m_frame( dynamic_cast<SCH_BASE_FRAME*>( aParent ) ),
        m_onlyCheckbox( nullptr )
{
    wxFont font = KIUI::GetInfoFont( this );
    m_cbLockedItems->SetFont( font );
    m_cbSymbols->SetFont( font );
    m_cbText->SetFont( font );
    m_cbWires->SetFont( font );
    m_cbLabels->SetFont( font );
    m_cbPins->SetFont( font );
    m_cbGraphics->SetFont( font );
    m_cbImages->SetFont( font );
    m_cbOtherItems->SetFont( font );
    m_cbAllItems->SetFont( font );

    SetBorders( true, false, false, false );

    wxASSERT( m_frame );
    m_tool = m_frame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    wxASSERT( m_tool );

    SCH_SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();
    SetCheckboxesFromFilter( opts );

    m_cbSymbols->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );
    m_cbText->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );
    m_cbWires->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );
    m_cbLabels->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );
    m_cbPins->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );
    m_cbGraphics->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );
    m_cbImages->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );
    m_cbOtherItems->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );

    if( m_frame->GetFrameType() == FRAME_SCH_SYMBOL_EDITOR )
    {
        Freeze();
        m_gridSizer->SetItemPosition( m_cbSymbols, wxGBPosition( 5, 0 ) );
        m_gridSizer->SetItemPosition( m_cbWires, wxGBPosition( 5, 1 ) );
        m_gridSizer->SetItemPosition( m_cbLabels, wxGBPosition( 6, 0 ) );
        m_gridSizer->SetItemPosition( m_cbImages, wxGBPosition( 6, 1 ) );
        m_cbSymbols->Hide();
        m_cbWires->Hide();
        m_cbLabels->Hide();
        m_cbImages->Hide();

        m_gridSizer->SetItemPosition( m_cbPins, wxGBPosition( 1, 0 ) );
        m_gridSizer->SetItemPosition( m_cbText, wxGBPosition( 1, 1 ) );
        m_gridSizer->SetItemPosition( m_cbGraphics, wxGBPosition( 2, 0 ) );
        m_gridSizer->SetItemPosition( m_cbOtherItems, wxGBPosition( 2, 1 ) );
        Thaw();
    }
}


void PANEL_SCH_SELECTION_FILTER::SetCheckboxesFromFilter( SCH_SELECTION_FILTER_OPTIONS& aOptions )
{
    Freeze();

    m_cbLockedItems->SetValue( aOptions.lockedItems );
    m_cbSymbols->SetValue( aOptions.symbols );
    m_cbText->SetValue( aOptions.text );
    m_cbWires->SetValue( aOptions.wires );
    m_cbLabels->SetValue( aOptions.labels );
    m_cbPins->SetValue( aOptions.pins );
    m_cbGraphics->SetValue( aOptions.graphics );
    m_cbImages->SetValue( aOptions.images );
    m_cbOtherItems->SetValue( aOptions.otherItems );

    m_cbAllItems->SetValue( aOptions.All() );

    Thaw();
}


void PANEL_SCH_SELECTION_FILTER::OnFilterChanged( wxCommandEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_cbAllItems )
    {
        bool newState = m_cbAllItems->GetValue();

        m_cbSymbols->SetValue( newState );
        m_cbText->SetValue( newState );
        m_cbWires->SetValue( newState );
        m_cbLabels->SetValue( newState );
        m_cbPins->SetValue( newState );
        m_cbGraphics->SetValue( newState );
        m_cbImages->SetValue( newState );
        m_cbOtherItems->SetValue( newState );
    }

    SCH_SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();

    // If any of the other checkboxes turned off, turn off the All Items checkbox
    bool allChecked = setFilterFromCheckboxes( opts );
    m_cbAllItems->SetValue( allChecked );
}


bool PANEL_SCH_SELECTION_FILTER::setFilterFromCheckboxes( SCH_SELECTION_FILTER_OPTIONS& aOptions )
{
    aOptions.lockedItems = m_cbLockedItems->GetValue();
    aOptions.symbols     = m_cbSymbols->GetValue();
    aOptions.text        = m_cbText->GetValue();
    aOptions.wires       = m_cbWires->GetValue();
    aOptions.labels      = m_cbLabels->GetValue();
    aOptions.pins        = m_cbPins->GetValue();
    aOptions.graphics    = m_cbGraphics->GetValue();
    aOptions.images      = m_cbImages->GetValue();
    aOptions.otherItems  = m_cbOtherItems->GetValue();

    return aOptions.All();
}


void PANEL_SCH_SELECTION_FILTER::onRightClick( wxMouseEvent& aEvent )
{
    wxMenu menu;

    wxCheckBox* cb = dynamic_cast<wxCheckBox*>( aEvent.GetEventObject() );

    if( !cb )
        return;

    m_onlyCheckbox = cb;

    wxString label;
    label.Printf( _( "Only %s" ),  cb->GetLabel().Lower() );

    menu.Append( new wxMenuItem( &menu, wxID_ANY, label, wxEmptyString, wxITEM_NORMAL ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_SCH_SELECTION_FILTER::onPopupSelection, this );

    PopupMenu( &menu );
}


void PANEL_SCH_SELECTION_FILTER::onPopupSelection( wxCommandEvent& aEvent )
{
    if( !m_onlyCheckbox )
        return;

    m_cbAllItems->SetValue( false );
    m_cbSymbols->SetValue( false );
    m_cbText->SetValue( false );
    m_cbWires->SetValue( false );
    m_cbLabels->SetValue( false );
    m_cbPins->SetValue( false );
    m_cbGraphics->SetValue( false );
    m_cbImages->SetValue( false );
    m_cbOtherItems->SetValue( false );

    m_onlyCheckbox->SetValue( true );
    m_onlyCheckbox = nullptr;

    wxCommandEvent dummy;
    OnFilterChanged( dummy );
}


void PANEL_SCH_SELECTION_FILTER::OnLanguageChanged()
{
    m_cbAllItems->SetLabel( _( "All items" ) );
    m_cbLockedItems->SetLabel( _( "Locked items" ) );
    m_cbLockedItems->SetToolTip( _( "Allow selection of locked items" ) );
    m_cbSymbols->SetLabel( _( "Symbols" ) );
    m_cbText->SetLabel( _( "Text" ) );
    m_cbWires->SetLabel( _( "Wires" ) );
    m_cbLabels->SetLabel( _( "Labels" ) );
    m_cbPins->SetLabel( _( "Pins" ) );
    m_cbGraphics->SetLabel( _( "Graphics" ) );
    m_cbImages->SetLabel( _( "Images" ) );
    m_cbOtherItems->SetLabel( _( "Other items" ) );

    m_cbAllItems->GetParent()->Layout();
}
