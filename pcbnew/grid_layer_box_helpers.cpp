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

#include <grid_layer_box_helpers.h>

#include <wx/textctrl.h>

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <footprint_editor_settings.h>
#include <board.h>
#include <lset.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <settings/color_settings.h>
#include <widgets/layer_presentation.h>


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
    int             value = aGrid.GetTable()->GetValueAsLong( aRow, aCol );
    COLOR_SETTINGS* cs = ::GetColorSettings( DEFAULT_THEME );

    if( m_frame )
        cs = m_frame->GetColorSettings();
    else if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) )
        cs = ::GetColorSettings( cfg->m_ColorTheme );

    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    // draw the swatch
#ifdef __WXMAC__
    int      size = 14;
#else
    int      size = KiROUND( 14 * aDC.GetContentScaleFactor() );
#endif
    if( !m_bitmap.IsOk() || m_bitmap.GetWidth() != size || m_bitmap.GetHeight() != size )
        m_bitmap = wxBitmap( size, size );

    LAYER_PRESENTATION::DrawColorSwatch( m_bitmap,
                                         cs->GetColor( ToLAYER_ID( LAYER_PCB_BACKGROUND ) ),
                                         cs->GetColor( ToLAYER_ID( value ) ) );

    aDC.DrawBitmap( m_bitmap, rect.GetLeft() + 4,
                    rect.GetTop() + ( rect.GetHeight() - m_bitmap.GetHeight() ) / 2, true );

    // draw the text
    PCB_LAYER_ID layer = ToLAYER_ID( value );
    wxString     layerName;

    if( m_frame )
        layerName = m_frame->GetBoard()->GetLayerName( layer );
    else
        layerName = BOARD::GetStandardLayerName( layer );

    rect.SetLeft( rect.GetLeft() + m_bitmap.GetWidth() + 8 );
    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, layerName, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}



//-------- Custom wxGridCellEditors ----------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor


GRID_CELL_LAYER_SELECTOR::GRID_CELL_LAYER_SELECTOR( PCB_BASE_FRAME* aFrame, const LSET& aMask,
                                                    bool aShowNonActivated ) :
        m_frame( aFrame ),
        m_mask( aMask ),
        m_showNonActivated( aShowNonActivated ),
        m_value( 0 )
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

    LayerBox()->SetLayersHotkeys( false );
    LayerBox()->SetBoardFrame( m_frame );
    LayerBox()->SetNotAllowedLayerSet( m_mask );
    LayerBox()->ShowNonActivatedLayers( m_showNonActivated );

    wxGridCellEditor::Create(aParent, aId, aEventHandler);
}


wxString GRID_CELL_LAYER_SELECTOR::GetValue() const
{
    if( LayerBox()->GetLayerSelection() != UNDEFINED_LAYER )
    {
        PCB_LAYER_ID layer = ToLAYER_ID( LayerBox()->GetLayerSelection() );

        if( m_frame )
            return m_frame->GetBoard()->GetLayerName( layer );
        else
            return BOARD::GetStandardLayerName( layer );
    }

    return wxEmptyString;
}


void GRID_CELL_LAYER_SELECTOR::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    rect.Inflate( -1 );

#if !defined( __WXMSW__ ) && !defined( __WXGTK__ )
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

    m_value = aGrid->GetTable()->GetValueAsLong( aRow, aCol );

    // Footprints are defined in a global context and may contain layers not enabled
    // on the current board.  Check and display all layers if so.
    if( m_frame && !m_frame->GetBoard()->IsLayerEnabled( ToLAYER_ID( m_value ) ) )
        LayerBox()->ShowNonActivatedLayers( true );

    LayerBox()->SetNotAllowedLayerSet( m_mask );
    LayerBox()->Resync();
    LayerBox()->SetLayerSelection( m_value );
    LayerBox()->SetFocus();

#ifdef __WXOSX_COCOA__
    // This is a work around for the combobox being simply dismissed when a
    // choice is made in it under OS X. The bug is almost certainly due to a
    // problem in focus events generation logic but it's not obvious to fix and
    // for now this at least allows one to use wxGrid.
    if( !LayerBox()->IsPopupShown() )
        LayerBox()->Popup();
#endif

    // When dropping down the menu, a kill focus event
    // happens after this point, so we can't reset the flag yet.
#if !defined(__WXGTK__)
    evtHandler->SetInSetFocus( false );
#endif
}


bool GRID_CELL_LAYER_SELECTOR::EndEdit( int , int , const wxGrid* , const wxString& ,
                                        wxString *newval )
{
    const int value = LayerBox()->GetLayerSelection();

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
#if defined(__WXGTK__)
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
