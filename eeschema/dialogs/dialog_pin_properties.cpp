/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_painter.h>
#include <symbol_edit_frame.h>
#include <sch_pin.h>
#include <dialog_pin_properties.h>
#include <confirm.h>
#include <kiplatform/ui.h>
#include <widgets/tab_traversal.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <wx/hyperlink.h>

class ALT_PIN_DATA_MODEL : public wxGridTableBase, public std::vector<SCH_PIN::ALT>
{
public:
    ALT_PIN_DATA_MODEL( EDA_UNITS aUserUnits )
    {
    }

    int GetNumberRows() override { return (int) size(); }
    int GetNumberCols() override { return COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case COL_NAME:  return _( "Alternate Pin Name" );
        case COL_TYPE:  return _( "Electrical Type" );
        case COL_SHAPE: return _( "Graphic Style" );
        default:        wxFAIL; return wxEmptyString;
        }
    }

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        switch( aCol )
        {
        case COL_NAME:  return at( aRow ).m_Name;
        case COL_TYPE:  return PinTypeNames()[static_cast<int>( at( aRow ).m_Type )];
        case COL_SHAPE: return PinShapeNames()[static_cast<int>( at( aRow ).m_Shape )];
        default:        wxFAIL; return wxEmptyString;
        }
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        switch( aCol )
        {
        case COL_NAME:
            at( aRow ).m_Name = aValue;
            break;

        case COL_TYPE:
            if( PinTypeNames().Index( aValue ) != wxNOT_FOUND )
                at( aRow ).m_Type = (ELECTRICAL_PINTYPE) PinTypeNames().Index( aValue );

            break;

        case COL_SHAPE:
            if( PinShapeNames().Index( aValue ) != wxNOT_FOUND )
                at( aRow ).m_Shape = (GRAPHIC_PINSHAPE) PinShapeNames().Index( aValue );

            break;

        default:
            wxFAIL;
            break;
        }
    }

    void AppendRow( const SCH_PIN::ALT& aAlt )
    {
        push_back( aAlt );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            GetView()->ProcessTableMessage( msg );
        }
    }

    void RemoveRow( int aRow )
    {
        erase( begin() + aRow );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
            GetView()->ProcessTableMessage( msg );
        }
    }
};


DIALOG_PIN_PROPERTIES::DIALOG_PIN_PROPERTIES( SYMBOL_EDIT_FRAME* parent, SCH_PIN* aPin ) :
    DIALOG_PIN_PROPERTIES_BASE( parent ),
    m_frame( parent ),
    m_pin( aPin ),
    m_posX( parent, m_posXLabel, m_posXCtrl, m_posXUnits ),
    m_posY( parent, m_posYLabel, m_posYCtrl, m_posYUnits ),
    m_pinLength( parent, m_pinLengthLabel, m_pinLengthCtrl, m_pinLengthUnits ),
    m_nameSize( parent, m_nameSizeLabel, m_nameSizeCtrl, m_nameSizeUnits ),
    m_numberSize( parent, m_numberSizeLabel, m_numberSizeCtrl, m_numberSizeUnits ),
    m_delayedFocusRow( -1 ),
    m_delayedFocusColumn( -1 ),
    m_initialized( false )
{
    // Creates a dummy pin to show on a panel, inside this dialog:
    m_dummyParent = new LIB_SYMBOL( *static_cast<LIB_SYMBOL*>( m_pin->GetParentSymbol() ) );
    m_dummyPin = new SCH_PIN( *m_pin );
    m_dummyPin->SetParent( m_dummyParent );
    m_dummyParent->SetShowPinNames( true );
    m_dummyParent->SetShowPinNumbers( true );

    COLOR4D bgColor = parent->GetRenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
    m_panelShowPin->SetBackgroundColour( bgColor.ToColour() );

    const wxArrayString&           orientationNames = PinOrientationNames();
    const std::vector<BITMAPS>& orientationIcons = PinOrientationIcons();

    for ( unsigned ii = 0; ii < orientationNames.GetCount(); ii++ )
        m_choiceOrientation->Insert( orientationNames[ii], KiBitmapBundle( orientationIcons[ii] ),
                                     ii );

    // We can't set the tab order through wxWidgets due to shortcomings in their mnemonics
    // implementation on MSW
    m_tabOrder = {
        m_textPinName,
        m_textPinNumber,
        m_choiceElectricalType,
        m_choiceStyle,
        m_posXCtrl,
        m_posYCtrl,
        m_choiceOrientation,
        m_pinLengthCtrl,
        m_nameSizeCtrl,
        m_numberSizeCtrl,
        m_checkApplyToAllParts,
      	m_checkApplyToAllBodyStyles,
      	m_checkShow,
      	m_sdbSizerButtonsOK,
        m_sdbSizerButtonsCancel
    };

    // Default alternates turndown to whether or not alternates exist, or if we've had it open before
    m_alternatesTurndown->Collapse( m_pin->GetAlternates().size() == 0 && !s_alternatesTurndownOpen);

    // wxwidgets doesn't call the OnCollapseChange even at init, so we update this value if
    // the alternates pane defaults to open
    if ( m_pin->GetAlternates().size() > 0 )
        s_alternatesTurndownOpen = true;

    m_alternatesDataModel = new ALT_PIN_DATA_MODEL( GetUserUnits() );

    // Save original columns widths so we can do proportional sizing.
    for( int i = 0; i < COL_COUNT; ++i )
        m_originalColWidths[ i ] = m_alternatesGrid->GetColSize( i );

    // Give a bit more room for combobox editors
    m_alternatesGrid->SetDefaultRowSize( m_alternatesGrid->GetDefaultRowSize() + 4 );

    m_alternatesGrid->SetTable( m_alternatesDataModel );
    m_alternatesGrid->PushEventHandler( new GRID_TRICKS( m_alternatesGrid,
                                                         [this]( wxCommandEvent& aEvent )
                                                         {
                                                             OnAddAlternate( aEvent );
                                                         } ) );

    if( aPin->GetParentSymbol()->HasAlternateBodyStyle() )
    {
        m_alternatesTurndown->Collapse();
        m_alternatesTurndown->Disable();
        m_alternatesTurndown->SetToolTip( _( "Alternate pin assignments are not available for "
                                             "De Morgan symbols." ) );
    }

    // Set special attributes
    wxGridCellAttr* attr;

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinTypeIcons(), PinTypeNames() ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinTypeIcons(), PinTypeNames() ) );
    m_alternatesGrid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinShapeIcons(), PinShapeNames() ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinShapeIcons(), PinShapeNames() ) );
    m_alternatesGrid->SetColAttr( COL_SHAPE, attr );

    m_addAlternate->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteAlternate->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_addAlternate->GetParent()->Layout();

    SetupStandardButtons();
    SetInitialFocus( m_textPinName );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    // On some window managers (Unity, XFCE) the dialog is not always raised, depending on
    // how it is run.
    Raise();

    m_initialized = true;
}


DIALOG_PIN_PROPERTIES::~DIALOG_PIN_PROPERTIES()
{
    delete m_dummyPin;
    delete m_dummyParent;

    // Prevents crash bug in wxGrid's d'tor
    m_alternatesGrid->DestroyTable( m_alternatesDataModel );

    // Delete the GRID_TRICKS.
    m_alternatesGrid->PopEventHandler( true );
}


bool DIALOG_PIN_PROPERTIES::TransferDataToWindow()
{
    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

    m_origPos = m_pin->GetPosition();

    m_choiceOrientation->SetSelection( PinOrientationIndex( m_pin->GetOrientation() ) );
    m_choiceStyle->SetSelection( m_pin->GetShape() );
    m_choiceElectricalType->SetSelection( m_pin->GetType() );
    m_textPinName->SetValue( m_pin->GetName() );
    m_nameSize.SetValue( m_pin->GetNameTextSize() );
    m_posX.SetValue( m_origPos.x );
    m_posY.SetValue( -m_origPos.y );
    m_textPinNumber->SetValue( m_pin->GetNumber() );
    m_numberSize.SetValue( m_pin->GetNumberTextSize() );
    m_pinLength.SetValue( m_pin->GetLength() );
    m_checkApplyToAllParts->Enable( m_pin->GetParentSymbol()->IsMulti() );
    m_checkApplyToAllParts->SetValue( m_pin->GetParentSymbol()->IsMulti() && m_pin->GetUnit() == 0 );
    m_checkApplyToAllBodyStyles->SetValue( m_pin->GetBodyStyle() == 0 );
    m_checkShow->SetValue( m_pin->IsVisible() );

    m_dummyPin->SetVisible( m_pin->IsVisible() );

    wxString commonUnitsToolTip;

    if( m_frame->m_SyncPinEdit )
    {
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( m_infoBar, wxID_ANY,
                                                       _( "Exit sync pins mode" ),
                                                       wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK,
                      std::function<void( wxHyperlinkEvent& aEvent )>(
                      [&]( wxHyperlinkEvent& aEvent )
                      {
                          m_frame->m_SyncPinEdit = false;
                          m_infoBar->Dismiss();
                      } ) );

        m_infoBar->RemoveAllButtons();
        m_infoBar->AddButton( button );
        m_infoBar->ShowMessage( getSyncPinsMessage() );

        commonUnitsToolTip = _( "Synchronized pins mode is enabled.\n"
                                "Similar pins will be edited regardless of this option." );
    }
    else
    {
        commonUnitsToolTip = _( "If checked, this pin will exist in all units." );
    }

    if( !m_pin->GetParentSymbol()->IsMulti() )
        commonUnitsToolTip = _( "This symbol only has one unit. This control has no effect." );

    m_checkApplyToAllParts->SetToolTip( commonUnitsToolTip );

    for( const std::pair<const wxString, SCH_PIN::ALT>& alt : m_pin->GetAlternates() )
        m_alternatesDataModel->AppendRow( alt.second );

    return true;
}


bool DIALOG_PIN_PROPERTIES::TransferDataFromWindow()
{
    if( !m_alternatesGrid->CommitPendingChanges() )
        return false;

    // Check for missing alternate names.
    for( size_t i = 0;  i < m_alternatesDataModel->size(); ++i )
    {
        if( m_alternatesDataModel->at( i ).m_Name.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "Alternate pin definitions must have a name." ) );

            m_delayedFocusColumn = COL_NAME;
            m_delayedFocusRow = i;

            return false;
        }
    }

    if( !DIALOG_SHIM::TransferDataFromWindow() )
        return false;

    VECTOR2I newPos( m_posX.GetIntValue(), -m_posY.GetIntValue() );

    const int standard_grid = 50;

    // Only show the warning if the position has been changed
    if( ( m_origPos != newPos )
        && (( m_posX.GetValue() % standard_grid ) || ( m_posY.GetValue() % standard_grid ) ) )
    {
        wxString msg = wxString::Format( _( "This pin is not on a %d mils grid which will make it "
                                            "difficult to connect to in the schematic.\n"
                                            "Do you wish to continue?" ),
                                         standard_grid );
        if( !IsOK( this, msg ) )
            return false;
    }

    m_pin->SetName( m_textPinName->GetValue() );
    m_pin->SetNumber( m_textPinNumber->GetValue() );
    m_pin->SetNameTextSize( m_nameSize.GetIntValue() );
    m_pin->SetNumberTextSize( m_numberSize.GetIntValue() );
    m_pin->SetOrientation( PinOrientationCode( m_choiceOrientation->GetSelection() ) );
    m_pin->SetPosition( newPos );
    m_pin->ChangeLength( m_pinLength.GetIntValue() );
    m_pin->SetType( m_choiceElectricalType->GetPinTypeSelection() );
    m_pin->SetShape( m_choiceStyle->GetPinShapeSelection() );
    m_pin->SetBodyStyle( m_checkApplyToAllBodyStyles->GetValue() ? 0 : m_frame->GetBodyStyle() );
    m_pin->SetUnit( m_checkApplyToAllParts->GetValue() ? 0 : m_frame->GetUnit() );
    m_pin->SetVisible( m_checkShow->GetValue() );

    std::map<wxString, SCH_PIN::ALT>& alternates = m_pin->GetAlternates();
    alternates.clear();

    for( const SCH_PIN::ALT& alt : *m_alternatesDataModel )
        alternates[ alt.m_Name ] = alt;

    return true;
}


/*
 * Draw (on m_panelShowPin) the pin according to current settings in dialog
 */
void DIALOG_PIN_PROPERTIES::OnPaintShowPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelShowPin );
    wxSize    dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Give a parent to m_dummyPin for draw purposes.
    // In fact m_dummyPin should not have a parent, but draw functions need a parent
    // to know some options, about pin texts
    SYMBOL_EDIT_FRAME* symbolEditor = (SYMBOL_EDIT_FRAME*) GetParent();

    // Calculate a suitable scale to fit the available draw area
    BOX2I  bBox = m_dummyPin->GetBoundingBox( true, true, false );
    bBox.Inflate( schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE ) );

    double xscale = (double) dc_size.x / bBox.GetWidth();
    double yscale = (double) dc_size.y / bBox.GetHeight();
    double scale = std::min( xscale, yscale );

    // Give a 7% margin (each side) and limit to no more than 100% zoom
    scale = std::min( scale * 0.85, 1.0 );
    dc.SetUserScale( scale, scale );
    GRResetPenAndBrush( &dc );

    SCH_RENDER_SETTINGS renderSettings( *symbolEditor->GetRenderSettings() );
    renderSettings.m_ShowPinNumbers = true;
    renderSettings.m_ShowPinNames = true;
    renderSettings.m_ShowHiddenFields = true;
    renderSettings.m_ShowConnectionPoints = true;
    renderSettings.m_Transform = TRANSFORM();
    renderSettings.SetPrintDC( &dc );

    m_dummyPin->Print( &renderSettings, 0, 0, -bBox.Centre(), false, false );

    event.Skip();
}


void DIALOG_PIN_PROPERTIES::OnPropertiesChange( wxCommandEvent& event )
{
    if( !IsShownOnScreen() )   // do nothing at init time
        return;

    m_dummyPin->SetName( m_textPinName->GetValue() );
    m_dummyPin->SetNumber( m_textPinNumber->GetValue() );
    m_dummyPin->SetNameTextSize( m_nameSize.GetIntValue() );
    m_dummyPin->SetNumberTextSize( m_numberSize.GetIntValue() );
    m_dummyPin->SetOrientation( PinOrientationCode( m_choiceOrientation->GetSelection() ) );
    m_dummyPin->SetLength( m_pinLength.GetIntValue() );
    m_dummyPin->SetType( m_choiceElectricalType->GetPinTypeSelection() );
    m_dummyPin->SetShape( m_choiceStyle->GetPinShapeSelection() );
    m_dummyPin->SetVisible( m_checkShow->GetValue() );

    if( event.GetEventObject() == m_checkApplyToAllParts && m_frame->m_SyncPinEdit )
        m_infoBar->ShowMessage( getSyncPinsMessage() );

    m_panelShowPin->Refresh();
}

wxString DIALOG_PIN_PROPERTIES::getSyncPinsMessage()
{
    if( m_checkApplyToAllParts->GetValue() )
        return _( "Synchronized Pins Mode." );
    else if( m_pin->IsNew() )
        return _( "Synchronized Pins Mode.  New pin will be added to all units." );
    else
        return _( "Synchronized Pins Mode.  Matching pins in other units will be updated." );
}


void DIALOG_PIN_PROPERTIES::OnAddAlternate( wxCommandEvent& event )
{
    if( !m_alternatesGrid->CommitPendingChanges() )
        return;

    SCH_PIN::ALT newAlt;
    newAlt.m_Name = wxEmptyString;
    newAlt.m_Type = m_pin->GetType();
    newAlt.m_Shape = m_pin->GetShape();

    m_alternatesDataModel->AppendRow( newAlt );

    m_alternatesGrid->MakeCellVisible( m_alternatesGrid->GetNumberRows() - 1, 0 );
    m_alternatesGrid->SetGridCursor( m_alternatesGrid->GetNumberRows() - 1, 0 );

    m_alternatesGrid->EnableCellEditControl( true );
    m_alternatesGrid->ShowCellEditControl();
}


void DIALOG_PIN_PROPERTIES::OnDeleteAlternate( wxCommandEvent& event )
{
    if( !m_alternatesGrid->CommitPendingChanges() )
        return;

    if( m_alternatesDataModel->size() == 0 )   // empty table
        return;

    int curRow = m_alternatesGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    m_alternatesDataModel->RemoveRow( curRow );

    curRow = std::max( 0, curRow - 1 );
    m_alternatesGrid->MakeCellVisible( curRow, m_alternatesGrid->GetGridCursorCol() );
    m_alternatesGrid->SetGridCursor( curRow, m_alternatesGrid->GetGridCursorCol() );
}


void DIALOG_PIN_PROPERTIES::adjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_alternatesGrid ).x;

    wxGridUpdateLocker deferRepaintsTillLeavingScope;

    m_alternatesGrid->SetColSize( COL_TYPE, m_originalColWidths[COL_TYPE] );
    m_alternatesGrid->SetColSize( COL_SHAPE, m_originalColWidths[COL_SHAPE] );

    m_alternatesGrid->SetColSize( COL_NAME, width - m_originalColWidths[COL_TYPE]
                                                    - m_originalColWidths[COL_SHAPE] );
}


void DIALOG_PIN_PROPERTIES::OnSize( wxSizeEvent& event )
{
    auto new_size = event.GetSize();

    if( m_initialized && m_size != new_size )
    {
        m_size = new_size;

        adjustGridColumns();
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_PIN_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    // Handle a delayed focus
    if( m_delayedFocusRow >= 0 )
    {
        m_alternatesTurndown->Collapse( false );

        m_alternatesGrid->SetFocus();
        m_alternatesGrid->MakeCellVisible( m_delayedFocusRow, m_delayedFocusColumn );
        m_alternatesGrid->SetGridCursor( m_delayedFocusRow, m_delayedFocusColumn );

        m_alternatesGrid->EnableCellEditControl( true );
        m_alternatesGrid->ShowCellEditControl();

        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
    }
}
void DIALOG_PIN_PROPERTIES::OnCollapsiblePaneChange( wxCollapsiblePaneEvent& event )
{
    if( !event.GetCollapsed() )
    {
        wxTopLevelWindow* tlw = dynamic_cast<wxTopLevelWindow*>( wxGetTopLevelParent( this ) );

        if( tlw )
        {
            tlw->InvalidateBestSize();
            wxSize bestSize = tlw->GetBestSize();
            wxSize currentSize = tlw->GetSize();
            tlw->SetSize( wxMax( bestSize.GetWidth(), currentSize.GetWidth() ),
                          wxMax( bestSize.GetHeight(), currentSize.GetHeight() ) );
        }
    }
}
