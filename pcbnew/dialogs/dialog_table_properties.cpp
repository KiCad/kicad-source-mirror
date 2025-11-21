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

#include "dialog_table_properties.h"

#include <wx/hyperlink.h>

#include <kiplatform/ui.h>
#include <widgets/font_choice.h>
#include <widgets/color_swatch.h>
#include <widgets/wx_grid.h>
#include <dialogs/html_message_box.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <grid_tricks.h>
#include <scintilla_tricks.h>
#include <string_utils.h>
#include <confirm.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_textbox.h>
#include <pcb_tablecell.h>
#include <pcb_table.h>
#include <project.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>


DIALOG_TABLE_PROPERTIES::DIALOG_TABLE_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame, PCB_TABLE* aTable ) :
        DIALOG_TABLE_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_table( aTable ),
        m_borderWidth( aFrame, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_separatorsWidth( aFrame, m_separatorsWidthLabel, m_separatorsWidthCtrl, m_separatorsWidthUnits )
{
    m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

    m_grid->CreateGrid( m_table->GetRowCount(), m_table->GetColCount() );
    m_grid->EnableEditing( true );
    m_grid->EnableGridLines( true );
    m_grid->EnableDragGridSize( false );
    m_grid->SetMargins( 0, 0 );
    m_grid->SetCellHighlightROPenWidth( 0 );

    m_grid->EnableDragColMove( false );
    m_grid->EnableDragColSize( false );
    m_grid->SetColLabelSize( 0 );
    m_grid->EnableDragRowMove( false );
    m_grid->EnableDragRowSize( false );
    m_grid->SetRowLabelSize( 0 );
    m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );

    m_gridSizer->Add( m_grid, 1, wxEXPAND, 5 );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    for( int row = 0; row < m_table->GetRowCount(); ++row )
    {
        for( int col = 0; col < m_table->GetColCount(); ++col )
        {
            PCB_TABLECELL*  cell = m_table->GetCell( row, col );
            wxGridCellAttr* attr = new wxGridCellAttr;

            if( cell->GetColSpan() == 0 || cell->GetRowSpan() == 0 )
            {
                attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( this ) );
                attr->SetReadOnly();
            }
            else
            {
                attr->SetEditor( new GRID_CELL_STC_EDITOR(
                        true, false,
                        [this, cell]( wxStyledTextEvent& aEvent, SCINTILLA_TRICKS* aScintillaTricks )
                        {
                            aScintillaTricks->DoTextVarAutocomplete(
                                    // getTokensFn
                                    [this, cell]( const wxString& xRef, wxArrayString* tokens )
                                    {
                                        m_frame->GetContextualTextVars( cell, xRef, tokens );
                                    } );
                        } ) );
            }

            m_grid->SetAttr( row, col, attr );
        }
    }

    if( m_table->GetParentFootprint() )
    {
        // Do not allow locking items in the footprint editor
        m_cbLocked->Show( false );
    }

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_frame->GetBoard()->IsLayerEnabled( m_table->GetLayer() ) )
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_frame );
    m_LayerSelectionCtrl->Resync();

    for( const auto& [lineStyle, lineStyleDesc] : lineTypeNames )
    {
        m_borderStyleCombo->Append( lineStyleDesc.name, KiBitmap( lineStyleDesc.bitmap ) );
        m_separatorsStyleCombo->Append( lineStyleDesc.name, KiBitmap( lineStyleDesc.bitmap ) );
    }

    SetupStandardButtons();
    Layout();

    // Add syntax help hyperlink
    m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _( "Syntax help" ), wxEmptyString, wxDefaultPosition,
                                        wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_syntaxHelp->SetToolTip( _( "Show syntax help window" ) );
    m_gridSizer->Add( m_syntaxHelp, 0, wxTOP | wxBOTTOM | wxRIGHT | wxLEFT, 3 );

    m_syntaxHelp->Bind( wxEVT_HYPERLINK, &DIALOG_TABLE_PROPERTIES::onSyntaxHelp, this );

    m_helpWindow = nullptr;

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_TABLE_PROPERTIES::~DIALOG_TABLE_PROPERTIES()
{
    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


bool DIALOG_TABLE_PROPERTIES::TransferDataToWindow()
{
    BOARD* board = m_frame->GetBoard();

    if( !wxDialog::TransferDataToWindow() )
        return false;

    //
    // Cell Contents
    //

    wxColour coveredColor = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );

    if( KIPLATFORM::UI::IsDarkTheme() )
        coveredColor = coveredColor.ChangeLightness( 140 );
    else
        coveredColor = coveredColor.ChangeLightness( 100 );

    for( int row = 0; row < m_table->GetRowCount(); ++row )
    {
        for( int col = 0; col < m_table->GetColCount(); ++col )
        {
            PCB_TABLECELL* tableCell;

            if( IsBackLayer( m_table->GetLayer() ) )
                tableCell = m_table->GetCell( row, m_table->GetColCount() - 1 - col );
            else
                tableCell = m_table->GetCell( row, col );

            if( tableCell->GetColSpan() == 0 || tableCell->GetRowSpan() == 0 )
            {
                m_grid->SetCellValue( row, col, coveredColor.GetAsString() );
                continue;
            }

            wxString text = tableCell->GetText();

            // show text variable cross-references in a human-readable format
            text = board->ConvertKIIDsToCrossReferences( text );

            m_grid->SetCellValue( row, col, text );
        }
    }

    CallAfter(
            [this]()
            {
                for( int row = 0; row < m_table->GetRowCount(); ++row )
                {
                    for( int col = 0; col < m_table->GetColCount(); ++col )
                    {
                        PCB_TABLECELL* tableCell = m_table->GetCell( row, col );

                        if( tableCell->IsSelected() )
                        {
                            m_grid->SetGridCursor( row, col );
                            m_grid->EnableCellEditControl();
                            m_grid->ShowCellEditControl();
                            return;
                        }
                    }
                }
            } );

    sizeGridToTable();

    //
    // Table Properties
    //

    m_LayerSelectionCtrl->SetLayerSelection( m_table->GetLayer() );
    m_cbLocked->SetValue( m_table->IsLocked() );

    m_borderCheckbox->SetValue( m_table->StrokeExternal() );
    m_headerBorder->SetValue( m_table->StrokeHeaderSeparator() );

    if( m_table->GetBorderStroke().GetWidth() >= 0 )
        m_borderWidth.SetValue( m_table->GetBorderStroke().GetWidth() );

    int style = static_cast<int>( m_table->GetBorderStroke().GetLineStyle() );

    if( style >= 0 && style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        m_borderStyleCombo->SetSelection( 0 );

    m_borderWidth.Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );
    m_borderStyleLabel->Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );
    m_borderStyleCombo->Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );

    bool rows = m_table->StrokeRows() && m_table->GetSeparatorsStroke().GetWidth() >= 0;
    bool cols = m_table->StrokeColumns() && m_table->GetSeparatorsStroke().GetWidth() >= 0;

    m_rowSeparators->SetValue( rows );
    m_colSeparators->SetValue( cols );

    if( m_table->GetSeparatorsStroke().GetWidth() >= 0 )
        m_separatorsWidth.SetValue( m_table->GetSeparatorsStroke().GetWidth() );

    style = static_cast<int>( m_table->GetSeparatorsStroke().GetLineStyle() );

    if( style >= 0 && style < (int) lineTypeNames.size() )
        m_separatorsStyleCombo->SetSelection( style );
    else
        m_separatorsStyleCombo->SetSelection( 0 );

    m_separatorsWidth.Enable( rows || cols );
    m_separatorsStyleLabel->Enable( rows || cols );
    m_separatorsStyleCombo->Enable( rows || cols );

    return true;
}

void DIALOG_TABLE_PROPERTIES::onHeaderChecked( wxCommandEvent& aEvent )
{
    BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();
    PCB_LAYER_ID           currentLayer = ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() );
    int                    defaultLineThickness = bds.GetLineThickness( currentLayer );

    bool border = m_borderCheckbox->GetValue();
    bool header = m_headerBorder->GetValue();

    if( ( border || header ) && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( defaultLineThickness );

    m_borderWidth.Enable( border || header );
    m_borderStyleLabel->Enable( border || header );
    m_borderStyleCombo->Enable( border || header );

    bool row = m_rowSeparators->GetValue();
    bool col = m_colSeparators->GetValue();

    if( ( row || col ) && m_separatorsWidth.GetValue() < 0 )
        m_separatorsWidth.SetValue( defaultLineThickness );

    m_separatorsWidth.Enable( row || col );
    m_separatorsStyleLabel->Enable( row || col );
    m_separatorsStyleCombo->Enable( row || col );
}

void DIALOG_TABLE_PROPERTIES::onBorderChecked( wxCommandEvent& aEvent )
{
    BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();
    PCB_LAYER_ID           currentLayer = ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() );
    int                    defaultLineThickness = bds.GetLineThickness( currentLayer );

    bool border = m_borderCheckbox->GetValue();
    bool header = m_headerBorder->GetValue();

    if( ( border || header ) && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( defaultLineThickness );

    m_borderWidth.Enable( border || header );
    m_borderStyleLabel->Enable( border || header );
    m_borderStyleCombo->Enable( border || header );

    bool row = m_rowSeparators->GetValue();
    bool col = m_colSeparators->GetValue();

    if( ( row || col ) && m_separatorsWidth.GetValue() < 0 )
        m_separatorsWidth.SetValue( defaultLineThickness );

    m_separatorsWidth.Enable( row || col );
    m_separatorsStyleLabel->Enable( row || col );
    m_separatorsStyleCombo->Enable( row || col );
}


bool DIALOG_TABLE_PROPERTIES::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    BOARD*       board = m_frame->GetBoard();
    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_table );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_table->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same text if is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_table->SetFlags( IN_EDIT );

    for( int row = 0; row < m_table->GetRowCount(); ++row )
    {
        for( int col = 0; col < m_table->GetColCount(); ++col )
        {
            PCB_TABLECELL* tableCell;

            if( IsBackLayer( m_table->GetLayer() ) )
                tableCell = m_table->GetCell( row, m_table->GetColCount() - 1 - col );
            else
                tableCell = m_table->GetCell( row, col );

            wxString txt = m_grid->GetCellValue( row, col );

            // Don't insert grey colour value back in to table cell
            if( tableCell->GetColSpan() == 0 || tableCell->GetRowSpan() == 0 )
                txt = wxEmptyString;

            // convert any text variable cross-references to their UUIDs
            txt = board->ConvertCrossReferencesToKIIDs( txt );

#ifdef __WXMAC__
            // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting.
            // Replace it now.
            txt.Replace( "\r", "\n" );
#elif defined( __WINDOWS__ )
            // On Windows, a new line is coded as \r\n.  We use only \n in kicad files and in
            // drawing routines so strip the \r char.
            txt.Replace( "\r", "" );
#endif

            tableCell->SetText( txt );
            tableCell->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
        }
    }

    m_table->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
    m_table->SetLocked( m_cbLocked->GetValue() );

    m_table->SetStrokeExternal( m_borderCheckbox->GetValue() );
    m_table->SetStrokeHeaderSeparator( m_headerBorder->GetValue() );
    {
        STROKE_PARAMS stroke = m_table->GetBorderStroke();

        if( m_borderCheckbox->GetValue() || m_headerBorder->GetValue() )
            stroke.SetWidth( std::max( 0, m_borderWidth.GetIntValue() ) );
        else
            stroke.SetWidth( -1 );

        auto it = lineTypeNames.begin();
        std::advance( it, m_borderStyleCombo->GetSelection() );

        if( it == lineTypeNames.end() )
            stroke.SetLineStyle( LINE_STYLE::SOLID );
        else
            stroke.SetLineStyle( it->first );

        m_table->SetBorderStroke( stroke );
    }

    m_table->SetStrokeRows( m_rowSeparators->GetValue() );
    m_table->SetStrokeColumns( m_colSeparators->GetValue() );
    {
        STROKE_PARAMS stroke = m_table->GetSeparatorsStroke();

        if( m_rowSeparators->GetValue() || m_colSeparators->GetValue() )
            stroke.SetWidth( std::max( 0, m_separatorsWidth.GetIntValue() ) );
        else
            stroke.SetWidth( -1 );

        auto it = lineTypeNames.begin();
        std::advance( it, m_separatorsStyleCombo->GetSelection() );

        if( it == lineTypeNames.end() )
            stroke.SetLineStyle( LINE_STYLE::SOLID );
        else
            stroke.SetLineStyle( it->first );

        m_table->SetSeparatorsStroke( stroke );
    }

    if( pushCommit )
        commit.Push( _( "Edit Table" ), SKIP_CONNECTIVITY );

    return true;
}


void DIALOG_TABLE_PROPERTIES::sizeGridToTable()
{
    Layout(); // Make sure we get the current client size for the grid

    wxSize availableGridSize = m_grid->GetClientSize();

    if( availableGridSize.x == 0 || availableGridSize.y == 0 )
        return;

    BOX2I  tableBBox = m_table->GetBoundingBox();
    double scalerX = static_cast<double>( availableGridSize.x ) / tableBBox.GetWidth();
    double scalerY = static_cast<double>( availableGridSize.y ) / tableBBox.GetHeight();

    for( int row = 0; row < m_table->GetRowCount(); ++row )
        m_grid->SetRowSize( row, std::floor( m_table->GetRowHeight( row ) * scalerY ) );

    for( int col = 0; col < m_table->GetColCount(); ++col )
        m_grid->SetColSize( col, std::floor( m_table->GetColWidth( col ) * scalerX ) );
}


void DIALOG_TABLE_PROPERTIES::onSize( wxSizeEvent& aEvent )
{
    if( m_table )
        sizeGridToTable();

    aEvent.Skip();
}


void DIALOG_TABLE_PROPERTIES::onSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = PCB_TEXT::ShowSyntaxHelp( this );
}
