/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/sch_selection_tool.h>
#include <widgets/panel_sch_selection_filter.h>
#include <wx/settings.h>
#include <wx/dcbuffer.h>


wxDEFINE_EVENT( EVT_SCH_SELECTION_FILTER_FLASH, SCH_SELECTION_FILTER_EVENT );

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
    m_cbRuleAreas->SetFont( font );

    SetBorders( true, false, false, false );

    wxASSERT( m_frame );
    m_tool = m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
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
    m_cbRuleAreas->Bind( wxEVT_RIGHT_DOWN, &PANEL_SCH_SELECTION_FILTER::onRightClick, this );

    if( m_frame->GetFrameType() == FRAME_SCH_SYMBOL_EDITOR )
    {
        Freeze();
        m_gridSizer->SetItemPosition( m_cbSymbols, wxGBPosition( 6, 0 ) );
        m_gridSizer->SetItemPosition( m_cbWires, wxGBPosition( 6, 1 ) );
        m_gridSizer->SetItemPosition( m_cbLabels, wxGBPosition( 7, 0 ) );
        m_gridSizer->SetItemPosition( m_cbImages, wxGBPosition( 7, 1 ) );
        m_cbSymbols->Hide();
        m_cbWires->Hide();
        m_cbLabels->Hide();
        m_cbImages->Hide();
        m_cbRuleAreas->Hide();

        m_gridSizer->SetItemPosition( m_cbPins, wxGBPosition( 1, 0 ) );
        m_gridSizer->SetItemPosition( m_cbText, wxGBPosition( 1, 1 ) );
        m_gridSizer->SetItemPosition( m_cbGraphics, wxGBPosition( 2, 0 ) );
        m_gridSizer->SetItemPosition( m_cbOtherItems, wxGBPosition( 2, 1 ) );
        Thaw();
    }

    m_frame->Bind( EDA_LANG_CHANGED, &PANEL_SCH_SELECTION_FILTER::OnLanguageChanged, this );

    m_flashSteps = 10;
    m_defaultBg = GetBackgroundColour();
    m_flashTimer.SetOwner( this );
    Bind( wxEVT_TIMER, &PANEL_SCH_SELECTION_FILTER::onFlashTimer, this );
    aParent->Bind( EVT_SCH_SELECTION_FILTER_FLASH, &PANEL_SCH_SELECTION_FILTER::OnFlashEvent, this );
}


PANEL_SCH_SELECTION_FILTER::~PANEL_SCH_SELECTION_FILTER()
{
    // Stop any active flashing
    m_flashTimer.Stop();
    if( !m_flashCounters.empty() )
    {
        Unbind( wxEVT_PAINT, &PANEL_SCH_SELECTION_FILTER::onPanelPaint, this );
    }
    m_flashCounters.clear();

    m_frame->Unbind( EDA_LANG_CHANGED, &PANEL_SCH_SELECTION_FILTER::OnLanguageChanged, this );
    m_frame->Unbind( EVT_SCH_SELECTION_FILTER_FLASH, &PANEL_SCH_SELECTION_FILTER::OnFlashEvent, this );
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
    m_cbRuleAreas->SetValue( aOptions.ruleAreas );
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
        m_cbRuleAreas->SetValue( newState );
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
    aOptions.ruleAreas   = m_cbRuleAreas->GetValue();
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
    m_cbRuleAreas->SetValue( false );

    m_onlyCheckbox->SetValue( true );
    m_onlyCheckbox = nullptr;

    wxCommandEvent dummy;
    OnFilterChanged( dummy );
}


void PANEL_SCH_SELECTION_FILTER::OnLanguageChanged( wxCommandEvent& aEvent )
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
    m_cbRuleAreas->SetLabel( _( "Rule Areas" ) );
    m_cbOtherItems->SetLabel( _( "Other items" ) );

    m_cbAllItems->GetParent()->Layout();

    aEvent.Skip();
}


void PANEL_SCH_SELECTION_FILTER::flashCheckbox( wxCheckBox* aBox )
{
    if( !aBox )
        return;

    // If already flashing, just reset the counter
    if( m_flashCounters.find( aBox ) != m_flashCounters.end() )
    {
        m_flashCounters[aBox] = m_flashSteps;
        return;
    }

    m_flashCounters[aBox] = m_flashSteps;
    m_defaultBg = aBox->GetBackgroundColour();

    // Bind paint event to this panel if not already bound
    if( m_flashCounters.size() == 1 )
    {
        Bind( wxEVT_PAINT, &PANEL_SCH_SELECTION_FILTER::onPanelPaint, this );
    }

    Refresh();
}


void PANEL_SCH_SELECTION_FILTER::onFlashTimer( wxTimerEvent& aEvent )
{
    for( auto it = m_flashCounters.begin(); it != m_flashCounters.end(); )
    {
        int step = --( it->second );

        if( step <= 0 )
            it = m_flashCounters.erase( it );
        else
            ++it;
    }

    if( m_flashCounters.empty() )
    {
        m_flashTimer.Stop();
        // Unbind paint event when no more flashing
        Unbind( wxEVT_PAINT, &PANEL_SCH_SELECTION_FILTER::onPanelPaint, this );
    }

    Refresh();
}


void PANEL_SCH_SELECTION_FILTER::onPanelPaint( wxPaintEvent& aEvent )
{
    wxPaintDC dc( this );

    // First, let the default painting happen
    aEvent.Skip();

    // Then draw our highlights on top
    for( auto& pair : m_flashCounters )
    {
        wxCheckBox* checkbox = pair.first;
        int step = pair.second;

        if( step > 0 )
        {
            // Get the checkbox position relative to this panel
            wxPoint checkboxPos = checkbox->GetPosition();
            wxSize checkboxSize = checkbox->GetSize();
            wxRect checkboxRect( checkboxPos, checkboxSize );

            // Calculate blended color based on current flash step
            wxColour highlight = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
            wxColour blend(
                ( highlight.Red() * step + m_defaultBg.Red() * ( m_flashSteps - step ) ) / m_flashSteps,
                ( highlight.Green() * step + m_defaultBg.Green() * ( m_flashSteps - step ) ) / m_flashSteps,
                ( highlight.Blue() * step + m_defaultBg.Blue() * ( m_flashSteps - step ) ) / m_flashSteps );

            // Draw semi-transparent overlay
            wxColour overlayColor( blend.Red(), blend.Green(), blend.Blue(), 128 );
            dc.SetBrush( wxBrush( overlayColor ) );
            dc.SetPen( wxPen( overlayColor ) );
            dc.DrawRectangle( checkboxRect );
        }
    }
}


void PANEL_SCH_SELECTION_FILTER::OnFlashEvent( SCH_SELECTION_FILTER_EVENT& aEvent )
{
    const SCH_SELECTION_FILTER_OPTIONS& aOptions = aEvent.m_options;

    if( aOptions.lockedItems )
        flashCheckbox( m_cbLockedItems );

    if( aOptions.symbols )
        flashCheckbox( m_cbSymbols );

    if( aOptions.text )
        flashCheckbox( m_cbText );

    if( aOptions.wires )
        flashCheckbox( m_cbWires );

    if( aOptions.labels )
        flashCheckbox( m_cbLabels );

    if( aOptions.pins )
        flashCheckbox( m_cbPins );

    if( aOptions.graphics )
        flashCheckbox( m_cbGraphics );

    if( aOptions.images )
        flashCheckbox( m_cbImages );

    if( aOptions.ruleAreas )
        flashCheckbox( m_cbRuleAreas );

    if( aOptions.otherItems )
        flashCheckbox( m_cbOtherItems );

    if( !m_flashCounters.empty() )
        m_flashTimer.Start( 50 );
}
