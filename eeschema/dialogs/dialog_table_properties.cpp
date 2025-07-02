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

#include <kiplatform/ui.h>
#include <sch_edit_frame.h>
#include <widgets/color_swatch.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_helpers.h>
#include <grid_tricks.h>
#include <settings/color_settings.h>
#include <sch_table.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <dialog_table_properties.h>


DIALOG_TABLE_PROPERTIES::DIALOG_TABLE_PROPERTIES( SCH_EDIT_FRAME* aFrame, SCH_TABLE* aTable ) :
        DIALOG_TABLE_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_table( aTable ),
        m_borderWidth( aFrame, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_separatorsWidth( aFrame, m_separatorsWidthLabel, m_separatorsWidthCtrl,
                           m_separatorsWidthUnits )
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
            const SCH_TABLECELL* cell = m_table->GetCell( row, col );
            wxGridCellAttr*      attr = new wxGridCellAttr;

            if( cell->GetColSpan() == 0 || cell->GetRowSpan() == 0 )
            {
                attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( this ) );
                attr->SetReadOnly();
            }
            else
            {
                attr->SetEditor( new GRID_CELL_STC_EDITOR( true, false,
                        // onCharFn
                        [this]( wxStyledTextEvent& aEvent, SCINTILLA_TRICKS* aScintillaTricks )
                        {
                            aScintillaTricks->DoTextVarAutocomplete(
                                    // getTokensFn
                                    [this]( const wxString& xRef, wxArrayString* tokens )
                                    {
                                        getContextualTextVars( xRef, tokens );
                                    } );
                        } ) );
            }

            m_grid->SetAttr( row, col, attr );
        }
    }

    for( const auto& [lineStyle, lineStyleDesc] : lineTypeNames )
    {
        m_borderStyleCombo->Append( lineStyleDesc.name, KiBitmap( lineStyleDesc.bitmap ) );
        m_separatorsStyleCombo->Append( lineStyleDesc.name, KiBitmap( lineStyleDesc.bitmap ) );
    }

    KIGFX::COLOR4D canvas = aFrame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_borderColorSwatch->SetSwatchBackground( canvas );
    m_separatorsColorSwatch->SetSwatchBackground( canvas );

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    SetupStandardButtons();
    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_TABLE_PROPERTIES::~DIALOG_TABLE_PROPERTIES()
{
    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_TABLE_PROPERTIES::TransferDataToWindow()
{
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
            SCH_TABLECELL* tableCell = m_table->GetCell( row, col );

            if( tableCell->GetColSpan() == 0 || tableCell->GetRowSpan() == 0 )
            {
                m_grid->SetCellValue( row, col, coveredColor.GetAsString() );
                continue;
            }

            wxString text = tableCell->GetText();

            // show text variable cross-references in a human-readable format
            if( SCHEMATIC* schematic = tableCell->Schematic() )
                text = schematic->ConvertKIIDsToRefs( text );

            m_grid->SetCellValue( row, col, text );
        }
    }

    CallAfter( [this]()
               {
                   for( int row = 0; row < m_table->GetRowCount(); ++row )
                   {
                       for( int col = 0; col < m_table->GetColCount(); ++col )
                       {
                           SCH_TABLECELL* tableCell = m_table->GetCell( row, col );

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

    m_borderCheckbox->SetValue( m_table->StrokeExternal() );
    m_headerBorder->SetValue( m_table->StrokeHeaderSeparator() );

    if( m_table->GetBorderStroke().GetWidth() >= 0 )
        m_borderWidth.SetValue( m_table->GetBorderStroke().GetWidth() );

    m_borderColorSwatch->SetSwatchColor( m_table->GetBorderStroke().GetColor(), false );

    int style = static_cast<int>( m_table->GetBorderStroke().GetLineStyle() );

    if( style >= 0 && style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        m_borderStyleCombo->SetSelection( 0 );

    m_borderWidth.Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );
    m_borderColorLabel->Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );
    m_borderColorSwatch->Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );
    m_borderStyleLabel->Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );
    m_borderStyleCombo->Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );

    bool rows = m_table->StrokeRows() && m_table->GetSeparatorsStroke().GetWidth() >= 0;
    bool cols = m_table->StrokeColumns() && m_table->GetSeparatorsStroke().GetWidth() >= 0;

    m_rowSeparators->SetValue( rows );
    m_colSeparators->SetValue( cols );

    if( m_table->GetSeparatorsStroke().GetWidth() >= 0 )
        m_separatorsWidth.SetValue( m_table->GetSeparatorsStroke().GetWidth() );

    m_separatorsColorSwatch->SetSwatchColor( m_table->GetSeparatorsStroke().GetColor(), false );

    style = static_cast<int>( m_table->GetSeparatorsStroke().GetLineStyle() );

    if( style >= 0 && style < (int) lineTypeNames.size() )
        m_separatorsStyleCombo->SetSelection( style );
    else
        m_separatorsStyleCombo->SetSelection( 0 );

    m_separatorsWidth.Enable( rows || cols );
    m_separatorsColorLabel->Enable( rows || cols );
    m_separatorsColorSwatch->Enable( rows || cols );
    m_separatorsStyleLabel->Enable( rows || cols );
    m_separatorsStyleCombo->Enable( rows || cols );

    return true;
}


void DIALOG_TABLE_PROPERTIES::getContextualTextVars( const wxString& aCrossRef,
                                                     wxArrayString*  aTokens )
{
    if( !aCrossRef.IsEmpty() )
    {
        SCH_REFERENCE_LIST refs;
        SCH_SYMBOL*        refSymbol = nullptr;

        m_frame->Schematic().Hierarchy().GetSymbols( refs );

        for( int jj = 0; jj < (int) refs.GetCount(); jj++ )
        {
            SCH_REFERENCE& ref = refs[jj];

            if( ref.GetSymbol()->GetRef( &ref.GetSheetPath(), true ) == aCrossRef )
            {
                refSymbol = ref.GetSymbol();
                break;
            }
        }

        if( refSymbol )
            refSymbol->GetContextualTextVars( aTokens );
    }
    else
    {
        SCHEMATIC* schematic = m_table->Schematic();

        if( schematic && schematic->CurrentSheet().Last() )
        {
            schematic->CurrentSheet().Last()->GetContextualTextVars( aTokens );
        }
        else
        {
            for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
                aTokens->push_back( entry.first );
        }
    }
}


void DIALOG_TABLE_PROPERTIES::onBorderChecked( wxCommandEvent& aEvent )
{
    bool border = m_borderCheckbox->GetValue();
    bool headerSeparator = m_headerBorder->GetValue();

    if( ( border || headerSeparator ) && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( m_frame->eeconfig()->m_Drawing.default_line_thickness );

    m_borderWidth.Enable( border || headerSeparator );
    m_borderColorLabel->Enable( border || headerSeparator );
    m_borderColorSwatch->Enable( border || headerSeparator );
    m_borderStyleLabel->Enable( border || headerSeparator );
    m_borderStyleCombo->Enable( border || headerSeparator );

    bool row = m_rowSeparators->GetValue();
    bool col = m_colSeparators->GetValue();

    if( ( row || col ) && m_separatorsWidth.GetValue() < 0 )
        m_separatorsWidth.SetValue( m_frame->eeconfig()->m_Drawing.default_line_thickness );

    m_separatorsWidth.Enable( row || col );
    m_separatorsColorLabel->Enable( row || col );
    m_separatorsColorSwatch->Enable( row || col );
    m_separatorsStyleLabel->Enable( row || col );
    m_separatorsStyleCombo->Enable( row || col );
}


bool DIALOG_TABLE_PROPERTIES::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_COMMIT commit( m_frame );

    /* save table in undo list if not already in edit */
    if( m_table->GetEditFlags() == 0 )
        commit.Modify( m_table, m_frame->GetScreen() );

    for( int row = 0; row < m_table->GetRowCount(); ++row )
    {
        for( int col = 0; col < m_table->GetColCount(); ++col )
        {
            SCH_TABLECELL* tableCell = m_table->GetCell( row, col );
            wxString       txt = m_grid->GetCellValue( row, col );

            // convert any text variable cross-references to their UUIDs
            if( SCHEMATIC* schematic = tableCell->Schematic() )
                txt = schematic->ConvertRefsToKIIDs( txt );

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
        }
    }

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

        stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

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

        stroke.SetColor( m_separatorsColorSwatch->GetSwatchColor() );

        m_table->SetSeparatorsStroke( stroke );
    }

    if( !commit.Empty() )
        commit.Push( _( "Edit Table" ) );

    return true;
}


void DIALOG_TABLE_PROPERTIES::sizeGridToTable()
{
    Layout();   // Make sure we get the current client size for the grid

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


