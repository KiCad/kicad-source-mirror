/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <symbol_preview_widget.h>

class ALT_PIN_DATA_MODEL : public WX_GRID_TABLE_BASE, public std::vector<SCH_PIN::ALT>
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


DIALOG_PIN_PROPERTIES::DIALOG_PIN_PROPERTIES( SYMBOL_EDIT_FRAME* parent, SCH_PIN* aPin,
                                              bool aFocusPinNumber ) :
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
    // Create a dummy symbol with a single pin for the preview widget:
    m_dummyParent = new LIB_SYMBOL( *static_cast<LIB_SYMBOL*>( m_pin->GetParentSymbol() ) );

    // Move everything in the copied symbol to unit 1; we'll use unit 2 for the dummy pin:
    m_dummyParent->SetUnitCount( 2, false );
    m_dummyParent->RunOnChildren( [&]( SCH_ITEM* child )
                                  {
                                      child->SetUnit( 1 );
                                  },
                                  RECURSE_MODE::NO_RECURSE );

    m_dummyPin = new SCH_PIN( *m_pin );
    m_dummyPin->ClearFlags( IS_NEW );       // Needed to display the dummy pin
    m_dummyPin->SetUnit( 2 );
    m_dummyParent->AddDrawItem( m_dummyPin, false );

    m_dummyParent->SetShowPinNames( true );
    m_dummyParent->SetShowPinNumbers( true );

    m_previewWidget = new SYMBOL_PREVIEW_WIDGET( m_panelShowPin, &m_frame->Kiway(), false,
                                                 m_frame->GetCanvas()->GetBackend() );

    m_previewWidget->SetLayoutDirection( wxLayout_LeftToRight );
    m_previewWidget->DisplayPart( m_dummyParent, m_dummyPin->GetUnit(), m_dummyPin->GetBodyStyle() );

    wxBoxSizer* previewSizer = new wxBoxSizer( wxHORIZONTAL );
    previewSizer->Add( m_previewWidget, 1, wxEXPAND, 5 );
    m_panelShowPin->SetSizer( previewSizer );

    m_previewWidget->GetRenderSettings()->m_ShowHiddenPins = true;

    const wxArrayString&        orientationNames = PinOrientationNames();
    const std::vector<BITMAPS>& orientationIcons = PinOrientationIcons();

    for ( unsigned ii = 0; ii < orientationNames.GetCount(); ii++ )
        m_choiceOrientation->Insert( orientationNames[ii], KiBitmapBundle( orientationIcons[ii] ), ii );

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

    // Default alternates turndown to whether or not alternates exist, or if we've had it open
    // before
    m_alternatesTurndown->Collapse( m_pin->GetAlternates().size() == 0 && !s_alternatesTurndownOpen );

    // wxwidgets doesn't call the OnCollapseChange even at init, so we update this value if
    // the alternates pane defaults to open
    if ( m_pin->GetAlternates().size() > 0 )
        s_alternatesTurndownOpen = true;

    m_alternatesDataModel = new ALT_PIN_DATA_MODEL( GetUserUnits() );

    // Save original columns widths so we can do proportional sizing.
    for( int i = 0; i < COL_COUNT; ++i )
        m_originalColWidths[ i ] = m_alternatesGrid->GetColSize( i );

    m_alternatesGrid->SetTable( m_alternatesDataModel );
    m_alternatesGrid->PushEventHandler( new GRID_TRICKS( m_alternatesGrid,
                                                         [this]( wxCommandEvent& aEvent )
                                                         {
                                                             OnAddAlternate( aEvent );
                                                         } ) );
    m_alternatesGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    if( aPin->GetParentSymbol()->IsMultiBodyStyle() )
    {
        m_alternatesTurndown->Collapse();
        m_alternatesTurndown->Disable();
        m_alternatesTurndown->SetToolTip( _( "Alternate pin assignments are not available for symbols with "
                                             "multiple body styles." ) );
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

    SetInitialFocus( aFocusPinNumber ? m_textPinNumber : m_textPinName );

    // We should call FinishDialogSettings() when all widgets have the size fixed.
    // However m_infoBar is not yet initialized, so it will called later
    // See TransferDataToWindow()

    // On some window managers (Unity, XFCE) the dialog is not always raised, depending on
    // how it is run.
    Raise();

    m_initialized = true;
}


DIALOG_PIN_PROPERTIES::~DIALOG_PIN_PROPERTIES()
{
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
    m_posY.SetValue( m_origPos.y );
    m_textPinNumber->SetValue( m_pin->GetNumber() );
    m_numberSize.SetValue( m_pin->GetNumberTextSize() );
    m_pinLength.SetValue( m_pin->GetLength() );
    m_checkApplyToAllParts->Enable( m_pin->GetParentSymbol()->IsMultiUnit() );
    m_checkApplyToAllParts->SetValue( m_pin->GetParentSymbol()->IsMultiUnit() && m_pin->GetUnit() == 0 );
    m_checkApplyToAllBodyStyles->Enable( m_pin->GetParentSymbol()->IsMultiBodyStyle() );
    m_checkApplyToAllBodyStyles->SetValue( m_pin->GetBodyStyle() == 0 );
    m_checkShow->SetValue( m_pin->IsVisible() );

    m_dummyPin->SetVisible( m_pin->IsVisible() );

    wxString commonUnitsToolTip;

    if( m_frame->m_SyncPinEdit )
    {
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( m_infoBar, wxID_ANY, _( "Exit sync pins mode" ),
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

    if( !m_pin->GetParentSymbol()->IsMultiUnit() )
        commonUnitsToolTip = _( "This symbol only has one unit. This control has no effect." );

    m_checkApplyToAllParts->SetToolTip( commonUnitsToolTip );

    for( const std::pair<const wxString, SCH_PIN::ALT>& alt : m_pin->GetAlternates() )
        m_alternatesDataModel->AppendRow( alt.second );

    // We can call FinishDialogSettings() now all widgets have the size fixed.
    finishDialogSettings();

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

    VECTOR2I newPos( m_posX.GetIntValue(), m_posY.GetIntValue() );

    const int standard_grid = 50;

    // Only show the warning if the position has been changed
    if( ( m_origPos != newPos )
        && ( ( m_posX.GetValue() % standard_grid ) || ( m_posY.GetValue() % standard_grid ) ) )
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
    {
        m_infoBar->ShowMessage( getSyncPinsMessage() );
        m_infoBar->GetSizer()->Layout();
    }

    m_previewWidget->DisplayPart( m_dummyParent, m_dummyPin->GetUnit(), m_dummyPin->GetBodyStyle() );
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

    m_alternatesGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                SCH_PIN::ALT newAlt;
                newAlt.m_Name = wxEmptyString;
                newAlt.m_Type = m_pin->GetType();
                newAlt.m_Shape = m_pin->GetShape();

                m_alternatesDataModel->AppendRow( newAlt );
                return { m_alternatesGrid->GetNumberRows() - 1, COL_NAME };
            } );
}


void DIALOG_PIN_PROPERTIES::OnDeleteAlternate( wxCommandEvent& event )
{
    m_alternatesGrid->OnDeleteRows(
            [&]( int row )
            {
                m_alternatesDataModel->RemoveRow( row );
            } );
}


void DIALOG_PIN_PROPERTIES::adjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_alternatesGrid ).x;

    wxGridUpdateLocker deferRepaintsTillLeavingScope;

    m_alternatesGrid->SetColSize( COL_TYPE, m_originalColWidths[COL_TYPE] );
    m_alternatesGrid->SetColSize( COL_SHAPE, m_originalColWidths[COL_SHAPE] );

    m_alternatesGrid->SetColSize( COL_NAME, width - m_originalColWidths[COL_TYPE] - m_originalColWidths[COL_SHAPE] );
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
