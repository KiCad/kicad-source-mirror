/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/panel_selection_filter.h>
#include <wx/settings.h>
#include <wx/dcbuffer.h>


wxDEFINE_EVENT( EVT_PCB_SELECTION_FILTER_FLASH, PCB_SELECTION_FILTER_EVENT );

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
    m_cbPoints->SetFont( font );
    m_cbOtherItems->SetFont( font );
    m_cbAllItems->SetFont( font );

    SetBorders( true, false, false, false );

    wxASSERT( m_frame );
    m_tool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    wxASSERT( m_tool );

    PCB_SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();
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
    m_cbPoints->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );
    m_cbOtherItems->Bind( wxEVT_RIGHT_DOWN, &PANEL_SELECTION_FILTER::onRightClick, this );

    m_frame->Bind( EDA_LANG_CHANGED, &PANEL_SELECTION_FILTER::OnLanguageChanged, this );

    SetMinSize( GetBestSize() );

    m_flashSteps = 10;
    m_defaultBg = GetBackgroundColour();
    m_flashTimer.SetOwner( this );
    Bind( wxEVT_TIMER, &PANEL_SELECTION_FILTER::onFlashTimer, this );
    aParent->Bind( EVT_PCB_SELECTION_FILTER_FLASH, &PANEL_SELECTION_FILTER::OnFlashEvent, this );
}


PANEL_SELECTION_FILTER::~PANEL_SELECTION_FILTER()
{
    // Stop any active flashing
    m_flashTimer.Stop();
    if( !m_flashCounters.empty() )
    {
        Unbind( wxEVT_PAINT, &PANEL_SELECTION_FILTER::onPanelPaint, this );
    }
    m_flashCounters.clear();

    m_frame->Unbind( EDA_LANG_CHANGED, &PANEL_SELECTION_FILTER::OnLanguageChanged, this );
    m_frame->Unbind( EVT_PCB_SELECTION_FILTER_FLASH, &PANEL_SELECTION_FILTER::OnFlashEvent, this );
}


void PANEL_SELECTION_FILTER::SetCheckboxesFromFilter( PCB_SELECTION_FILTER_OPTIONS& aOptions )
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
    m_cbPoints->SetValue( aOptions.points );
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
        m_cbPoints->SetValue( newState );
        m_cbOtherItems->SetValue( newState );
    }

    PCB_SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();

    // If any of the other checkboxes turned off, turn off the All Items checkbox
    bool allChecked = setFilterFromCheckboxes( opts );
    m_cbAllItems->SetValue( allChecked );
}


bool PANEL_SELECTION_FILTER::setFilterFromCheckboxes( PCB_SELECTION_FILTER_OPTIONS& aOptions )
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
    aOptions.points      = m_cbPoints->GetValue();
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
    m_cbPoints->SetValue( false );
    m_cbOtherItems->SetValue( false );

    m_onlyCheckbox->SetValue( true );
    m_onlyCheckbox = nullptr;

    wxCommandEvent dummy;
    OnFilterChanged( dummy );
}


void PANEL_SELECTION_FILTER::OnLanguageChanged( wxCommandEvent& aEvent )
{
    m_cbAllItems->SetLabel( _( "All items" ) );
    m_cbLockedItems->SetLabel( _( "Locked items" ) );
    m_cbLockedItems->SetToolTip( _( "Allow selection of locked items" ) );
    m_cbFootprints->SetLabel( _( "Footprints" ) );
    m_cbText->SetLabel( _( "Text" ) );
    m_cbTracks->SetLabel( _( "Tracks" ) );
    m_cbVias->SetLabel( _( "Vias" ) );
    m_cbPads->SetLabel( _( "Pads" ) );
    m_cbGraphics->SetLabel( _( "Graphics" ) );
    m_cbZones->SetLabel( _( "Zones" ) );
    m_cbKeepouts->SetLabel( _( "Rule Areas" ) );
    m_cbDimensions->SetLabel( _( "Dimensions" ) );
    m_cbPoints->SetLabel( _( "Points" ) );
    m_cbOtherItems->SetLabel( _( "Other items" ) );

    m_cbAllItems->GetParent()->Layout();

    aEvent.Skip();
}


void PANEL_SELECTION_FILTER::flashCheckbox( wxCheckBox* aBox )
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
        Bind( wxEVT_PAINT, &PANEL_SELECTION_FILTER::onPanelPaint, this );
    }

    Refresh();
}


void PANEL_SELECTION_FILTER::onFlashTimer( wxTimerEvent& aEvent )
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
        Unbind( wxEVT_PAINT, &PANEL_SELECTION_FILTER::onPanelPaint, this );
    }

    Refresh();
}


void PANEL_SELECTION_FILTER::onPanelPaint( wxPaintEvent& aEvent )
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


void PANEL_SELECTION_FILTER::OnFlashEvent( PCB_SELECTION_FILTER_EVENT& aEvent )
{
    const PCB_SELECTION_FILTER_OPTIONS& aOptions = aEvent.m_options;

    if( aOptions.lockedItems )
        flashCheckbox( m_cbLockedItems );

    if( aOptions.footprints )
        flashCheckbox( m_cbFootprints );

    if( aOptions.text )
        flashCheckbox( m_cbText );

    if( aOptions.tracks )
        flashCheckbox( m_cbTracks );

    if( aOptions.vias )
        flashCheckbox( m_cbVias );

    if( aOptions.pads )
        flashCheckbox( m_cbPads );

    if( aOptions.graphics )
        flashCheckbox( m_cbGraphics );

    if( aOptions.zones )
        flashCheckbox( m_cbZones );

    if( aOptions.keepouts )
        flashCheckbox( m_cbKeepouts );

    if( aOptions.dimensions )
        flashCheckbox( m_cbDimensions );

    if( aOptions.points )
        flashCheckbox( m_cbPoints );

    if( aOptions.otherItems )
        flashCheckbox( m_cbOtherItems );

    if( !m_flashCounters.empty() )
        m_flashTimer.Start( 50 );
}
