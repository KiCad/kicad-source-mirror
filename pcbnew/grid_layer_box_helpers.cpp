/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <grid_layer_box_helpers.h>

#include <board.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <settings/color_settings.h>
#include <widgets/layer_box_selector.h>
#include <wx/textctrl.h>


//-------- Custom wxGridCellRenderers --------------------------------------------------


GRID_CELL_LAYER_RENDERER::GRID_CELL_LAYER_RENDERER( PCB_BASE_FRAME* aFrame ) :
        m_frame( aFrame )
{
}


GRID_CELL_LAYER_RENDERER::~GRID_CELL_LAYER_RENDERER()
{
}


void GRID_CELL_LAYER_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                     const wxRect& aRect, int aRow, int aCol, bool isSelected )
{
    LAYER_NUM value = aGrid.GetTable()->GetValueAsLong( aRow, aCol );

    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    // draw the swatch
    wxBitmap bitmap( 14, 14 );
    COLOR_SETTINGS* cs = m_frame->GetColorSettings();
    LAYER_SELECTOR::DrawColorSwatch( bitmap,
                                     cs->GetColor( ToLAYER_ID( LAYER_PCB_BACKGROUND ) ),
                                     cs->GetColor( ToLAYER_ID( value ) ) );
    aDC.DrawBitmap( bitmap, rect.GetLeft() + 4, rect.GetTop() + 3, true );

    // draw the text
    wxString text = m_frame->GetBoard()->GetLayerName( ToLAYER_ID( value ) );
    rect.SetLeft( rect.GetLeft() + bitmap.GetWidth() + 8 );
    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, text, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}



//-------- Custom wxGridCellEditors ----------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor


GRID_CELL_LAYER_SELECTOR::GRID_CELL_LAYER_SELECTOR( PCB_BASE_FRAME* aFrame, LSET aMask ) :
        m_frame( aFrame ), m_mask( aMask ), m_value( 0 )
{
}


wxGridCellEditor* GRID_CELL_LAYER_SELECTOR::Clone() const
{
    return new GRID_CELL_LAYER_SELECTOR( m_frame, m_mask );
}


void GRID_CELL_LAYER_SELECTOR::Create( wxWindow* aParent, wxWindowID aId,
                                        wxEvtHandler* aEventHandler )
{
    m_control = new PCB_LAYER_BOX_SELECTOR( aParent, aId, wxEmptyString,
                    wxDefaultPosition, wxDefaultSize, 0, nullptr,
                    wxCB_READONLY | wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB | wxBORDER_NONE );

    LayerBox()->SetBoardFrame( m_frame );
    LayerBox()->SetNotAllowedLayerSet( m_mask );

    wxGridCellEditor::Create(aParent, aId, aEventHandler);
}


wxString GRID_CELL_LAYER_SELECTOR::GetValue() const
{
    if( LayerBox()->GetLayerSelection() != UNDEFINED_LAYER )
        return m_frame->GetBoard()->GetLayerName( ToLAYER_ID( LayerBox()->GetLayerSelection() ) );

    return wxEmptyString;
}


void GRID_CELL_LAYER_SELECTOR::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    rect.Inflate( -1 );

#if !defined( __WXMSW__ ) && !defined( __WXGTK20__ )
    // Only implemented in generic wxBitmapComboBox; MSW and GTK use native controls
    LayerBox()->SetButtonPosition( 0, 0, wxRIGHT, 0 );
#endif

#if defined( __WXMAC__ )
    rect.Inflate( 3 );      // no FOCUS_RING, even on Mac
#endif

    LayerBox()->SetSize( rect, wxSIZE_ALLOW_MINUS_ONE );
}


void GRID_CELL_LAYER_SELECTOR::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    auto* evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

    // Don't immediately end if we get a kill focus event within BeginEdit
    evtHandler->SetInSetFocus( true );

    // These event handlers are needed to properly dismiss the editor when the popup is closed
    m_control->Bind(wxEVT_COMBOBOX_DROPDOWN, &GRID_CELL_LAYER_SELECTOR::onComboDropDown, this);
    m_control->Bind(wxEVT_COMBOBOX_CLOSEUP,  &GRID_CELL_LAYER_SELECTOR::onComboCloseUp,  this);

    m_value = (LAYER_NUM) aGrid->GetTable()->GetValueAsLong( aRow, aCol );

    // Footprints are defined in a global context and may contain layers not enabled
    // on the current board.  Check and display all layers if so.
    bool currentLayerEnabled = m_frame->GetBoard()->IsLayerEnabled( ToLAYER_ID( m_value ) );
    LayerBox()->ShowNonActivatedLayers( !currentLayerEnabled );
    LayerBox()->Resync();

    LayerBox()->SetLayerSelection( m_value );
    LayerBox()->SetFocus();

#ifdef __WXOSX_COCOA__
    // This is a work around for the combobox being simply dismissed when a
    // choice is made in it under OS X. The bug is almost certainly due to a
    // problem in focus events generation logic but it's not obvious to fix and
    // for now this at least allows to use wxGrid.
    if( !LayerBox()->IsPopupShown() )
        LayerBox()->Popup();
#endif

    // When dropping down the menu, a kill focus event
    // happens after this point, so we can't reset the flag yet.
#if !defined(__WXGTK20__)
    evtHandler->SetInSetFocus( false );
#endif
}


bool GRID_CELL_LAYER_SELECTOR::EndEdit( int , int , const wxGrid* , const wxString& ,
                                        wxString *newval )
{
    const LAYER_NUM value = LayerBox()->GetLayerSelection();

    if ( value == m_value )
        return false;

    m_value = value;

    if ( newval )
        *newval = GetValue();

    return true;
}


void GRID_CELL_LAYER_SELECTOR::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValueAsLong( aRow, aCol, (long) m_value );
}


void GRID_CELL_LAYER_SELECTOR::Reset()
{
    LayerBox()->SetLayerSelection( m_value );
}


void GRID_CELL_LAYER_SELECTOR::onComboDropDown( wxCommandEvent& aEvent )
{
    // On other platforms this is done in BeginEdit()
#if defined(__WXGTK20__)
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

    // Once the combobox is dropped, reset the flag to allow the focus-loss handler
    // to function and close the editor.
    evtHandler->SetInSetFocus( false );
#endif
}


void GRID_CELL_LAYER_SELECTOR::onComboCloseUp( wxCommandEvent& aEvent )
{
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

    // Forward the combobox close up event to the cell event handler as a focus kill event
    // so that the grid editor is dismissed when the combox closes, otherwise it leaves the
    // dropdown arrow visible in the cell.
    wxFocusEvent event( wxEVT_KILL_FOCUS, m_control->GetId() );
    event.SetEventObject( m_control );
    evtHandler->ProcessEvent( event );
}
