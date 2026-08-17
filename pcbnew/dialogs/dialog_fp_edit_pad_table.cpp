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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_fp_edit_pad_table.h"

#include <wx/display.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include <column_formatter.h>
#include <pcb_shape.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <base_units.h>
#include <units_provider.h>
#include <board.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <grid_tricks.h>
#include <kiplatform/ui.h>
#include <pin_numbers.h>
#include <board_commit.h>
#include <reporter.h>
#include <table_io.h>
#include <wildcards_and_files_ext.h>


using COLS = DIALOG_FP_EDIT_PAD_TABLE::COLS;

// Helper to map shape string to PAD_SHAPE
static PAD_SHAPE ShapeFromString( const wxString& shape )
{
    if( shape == _( "Oval" ) )                return PAD_SHAPE::OVAL;
    if( shape == _( "Rectangle" ) )           return PAD_SHAPE::RECTANGLE;
    if( shape == _( "Trapezoid" ) )           return PAD_SHAPE::TRAPEZOID;
    if( shape == _( "Rounded rectangle" ) )   return PAD_SHAPE::ROUNDRECT;
    if( shape == _( "Chamfered rectangle" ) ) return PAD_SHAPE::CHAMFERED_RECT;
    if( shape == _( "Custom shape" ) )        return PAD_SHAPE::CUSTOM;

    return PAD_SHAPE::CIRCLE;
}


static wxString ShapeToString( PAD_SHAPE shape )
{
    switch( shape )
    {
    case PAD_SHAPE::CIRCLE:           return _( "Circle" );
    case PAD_SHAPE::OVAL:             return _( "Oval" );
    case PAD_SHAPE::RECTANGLE:        return _( "Rectangle" );
    case PAD_SHAPE::TRAPEZOID:        return _( "Trapezoid" );
    case PAD_SHAPE::ROUNDRECT:        return _( "Rounded rectangle" );
    case PAD_SHAPE::CHAMFERED_RECT:   return _( "Chamfered rectangle" );
    case PAD_SHAPE::CUSTOM:           return _( "Custom shape" );
    default:
        wxFAIL_MSG( wxT( "Invalid pad shape" ) );
        return wxEmptyString;
    }
}


static wxString GetPadTypeString( const PAD& aPad )
{
    if( aPad.IsAperturePad() )
        return _( "Aperture" );

    const PAD_ATTRIB attrib = aPad.GetAttribute();

    switch( attrib )
    {
    case PAD_ATTRIB::PTH:  return _( "Through-hole" );
    case PAD_ATTRIB::SMD:  return _( "SMD" );
    case PAD_ATTRIB::CONN: return _( "Connector" );
    case PAD_ATTRIB::NPTH: return _( "NPTH" );
    // No default - handle all cases
    }

    return wxEmptyString;
}


static void SetPadTypeFromString( PAD& aPad, const wxString& aType )
{
    const PAD_ATTRIB oldAttrib = aPad.GetAttribute();

    if( MatchTranslationOrNative( aType, _HKI( "Through-hole" ), false ) )
    {
        aPad.SetAttribute( PAD_ATTRIB::PTH );
    }
    else if( MatchTranslationOrNative( aType, _HKI( "SMD" ), false ) )
    {
        if( oldAttrib != PAD_ATTRIB::SMD )
        {
            aPad.SetAttribute( PAD_ATTRIB::SMD );
            aPad.SetLayerSet( LSET{ F_Cu } );
        }
    }
    else if( MatchTranslationOrNative( aType, _HKI( "Connector" ), false ) )
    {
        aPad.SetAttribute( PAD_ATTRIB::CONN );
    }
    else if( MatchTranslationOrNative( aType, _HKI( "NPTH" ), false ) )
    {
        aPad.SetAttribute( PAD_ATTRIB::NPTH );
    }
    else if( MatchTranslationOrNative( aType, _HKI( "Aperture" ), false ) )
    {
        if( !aPad.IsAperturePad() )
        {
            // Aperture pads are SMD pads with no copper
            aPad.SetAttribute( PAD_ATTRIB::SMD );

            // Unset layers except F.Paste
            LSET apertureLayers{ F_Paste };
            aPad.SetLayerSet( apertureLayers );
        }
    }

    // Note, bad strings can sneak in here, e.g. via pasting into a dropdown cell.
    // So don't assert or crash, it's not necessarily a programming error.
}


/**
 * Get the label for a given column in the pin table.
 *
 * This string is NOT translated.
 */
static wxString GetPadTableColLabel( COLS aCol )
{
    switch( aCol )
    {
    case COLS::COL_NUMBER:     return wxT( "Number" );
    case COLS::COL_TYPE:       return wxT( "Type" );
    case COLS::COL_SHAPE:      return wxT( "Shape" );
    case COLS::COL_POS_X:      return wxT( "Pos X" );
    case COLS::COL_POS_Y:      return wxT( "Pos Y" );
    case COLS::COL_SIZE_X:     return wxT( "Size X" );
    case COLS::COL_SIZE_Y:     return wxT( "Size Y" );
    case COLS::COL_DRILL_X:    return wxT( "Drill X" );
    case COLS::COL_DRILL_Y:    return wxT( "Drill Y" );
    case COLS::COL_P2D_LENGTH: return wxT( "Pad to die length" );
    case COLS::COL_P2D_DELAY:  return wxT( "Pad to die delay" );
    default:
        wxFAIL_MSG( wxT( "Invalid column index" ) );
        return wxEmptyString;
    }
}


static COLS GetColTypeForString( const wxString& aStr )
{
    for( int i = 0; i < static_cast<int>( COLS::COL_COUNT ); i++ )
    {
        COLS col = static_cast<COLS>( i );

        if( MatchTranslationOrNative( aStr, GetPadTableColLabel( col ), false ) )
            return col;
    }
    return COLS::COL_COUNT;
}


/**
 * Class that handles conversion of various pin data fields into strings for display in the
 * UI or serialisation to formats like CSV.
 *
 * This is the footprint editor partner to @ref PIN_INFO_FORMATTER
 */
class PAD_INFO_FORMATTER : public COLUMN_FORMATTER
{
public:
    PAD_INFO_FORMATTER( UNITS_PROVIDER& aUnitsProvider, bool aIncludeUnits, BOOL_FORMAT aBoolFormat,
                        REPORTER& aReporter ) :
            COLUMN_FORMATTER( aUnitsProvider, aIncludeUnits, aBoolFormat, aReporter )
    {
    }

    wxString Format( const PAD& aPin, int aFieldId ) const
    {
        wxCHECK_MSG( aFieldId >= 0 && aFieldId < static_cast<int>( COLS::COL_COUNT ), wxEmptyString,
                     wxT( "Invalid column index" ) );

        const DIALOG_FP_EDIT_PAD_TABLE::COLS col = static_cast<DIALOG_FP_EDIT_PAD_TABLE::COLS>( aFieldId );

        switch( col )
        {
        case DIALOG_FP_EDIT_PAD_TABLE::COL_NUMBER:
            return aPin.GetNumber();

        case DIALOG_FP_EDIT_PAD_TABLE::COL_TYPE:
            return GetPadTypeString( aPin );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_SHAPE:
            return ShapeToString( aPin.GetShape( PADSTACK::ALL_LAYERS ) );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_POS_X:
            return m_unitsProvider.StringFromValue( aPin.GetCenter().x, m_includeUnits );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_POS_Y:
            return m_unitsProvider.StringFromValue( aPin.GetCenter().y, m_includeUnits );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_SIZE_X:
            return m_unitsProvider.StringFromValue( aPin.GetSize( PADSTACK::ALL_LAYERS ).x, m_includeUnits );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_SIZE_Y:
            return m_unitsProvider.StringFromValue( aPin.GetSize( PADSTACK::ALL_LAYERS ).y, m_includeUnits );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_DRILL_X:
            return m_unitsProvider.StringFromValue( aPin.GetDrillSize().x, m_includeUnits );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_DRILL_Y:
            return m_unitsProvider.StringFromValue( aPin.GetDrillSize().y, m_includeUnits );

        case DIALOG_FP_EDIT_PAD_TABLE::COL_P2D_LENGTH:
            if( aPin.GetPadToDieLength() )
                return m_unitsProvider.StringFromValue( aPin.GetPadToDieLength(), m_includeUnits );
            return wxEmptyString;

        case DIALOG_FP_EDIT_PAD_TABLE::COL_P2D_DELAY:
            if( aPin.GetPadToDieDelay() )
                return m_unitsProvider.StringFromValue( aPin.GetPadToDieDelay(), m_includeUnits, EDA_DATA_TYPE::TIME );
            return wxEmptyString;

        case DIALOG_FP_EDIT_PAD_TABLE::COL_COUNT:
            return wxEmptyString;
        }

        wxFAIL_MSG( "Invalid column index" );
        return wxEmptyString;
    }

    /**
     * Update the pad from the given col/string.
     *
     * How much this should follow the format is debatable, but for now it's fairly permissive
     * (e.g. bools import as 0/1 and no/yes).
     */
    void UpdatePad( PAD& aPad, const wxString& aValue, int aFieldId ) const
    {
        switch( aFieldId )
        {
        case DIALOG_FP_EDIT_PAD_TABLE::COL_NUMBER:
            aPad.SetNumber( aValue );
            break;

        case DIALOG_FP_EDIT_PAD_TABLE::COL_TYPE:
            SetPadTypeFromString( aPad, aValue );
            break;

        case DIALOG_FP_EDIT_PAD_TABLE::COL_SHAPE:
            aPad.SetShape( PADSTACK::ALL_LAYERS, ShapeFromString( aValue ) );
            break;

        case DIALOG_FP_EDIT_PAD_TABLE::COL_POS_X:
        {
            VECTOR2I pos = aPad.GetPosition();
            pos.x = m_unitsProvider.ValueFromString( aValue );
            aPad.SetPosition( pos );
            break;
        }

        case DIALOG_FP_EDIT_PAD_TABLE::COL_POS_Y:
        {
            VECTOR2I pos = aPad.GetPosition();
            pos.y = m_unitsProvider.ValueFromString( aValue );
            aPad.SetPosition( pos );
            break;
        }

        case DIALOG_FP_EDIT_PAD_TABLE::COL_SIZE_X:
        {
            VECTOR2I size = aPad.GetSize( PADSTACK::ALL_LAYERS );
            size.x = m_unitsProvider.ValueFromString( aValue );
            aPad.SetSize( PADSTACK::ALL_LAYERS, size );
            break;
        }

        case DIALOG_FP_EDIT_PAD_TABLE::COL_SIZE_Y:
        {
            VECTOR2I size = aPad.GetSize( PADSTACK::ALL_LAYERS );
            size.y = m_unitsProvider.ValueFromString( aValue );
            aPad.SetSize( PADSTACK::ALL_LAYERS, size );
            break;
        }

        case DIALOG_FP_EDIT_PAD_TABLE::COL_DRILL_X:
        {
            VECTOR2I drill = aPad.GetDrillSize();
            drill.x = m_unitsProvider.ValueFromString( aValue );
            aPad.SetDrillSize( drill );
            break;
        }

        case DIALOG_FP_EDIT_PAD_TABLE::COL_DRILL_Y:
        {
            VECTOR2I drill = aPad.GetDrillSize();
            drill.y = m_unitsProvider.ValueFromString( aValue );
            aPad.SetDrillSize( drill );
            break;
        }

        case DIALOG_FP_EDIT_PAD_TABLE::COL_P2D_LENGTH:
            if( aValue.empty() )
                aPad.SetPadToDieLength( 0 );
            else
                aPad.SetPadToDieLength( m_unitsProvider.ValueFromString( aValue ) );
            break;

        case DIALOG_FP_EDIT_PAD_TABLE::COL_P2D_DELAY:
            if( aValue.empty() )
                aPad.SetPadToDieDelay( 0 );
            else
                aPad.SetPadToDieDelay( m_unitsProvider.ValueFromString( aValue, EDA_DATA_TYPE::TIME ) );
            break;

        default:
            wxFAIL_MSG( "Invalid column index" );
            break;
        }
    }
};


DIALOG_FP_EDIT_PAD_TABLE::DIALOG_FP_EDIT_PAD_TABLE( PCB_BASE_FRAME* aParent, FOOTPRINT* aFootprint ) :
        DIALOG_FP_EDIT_PAD_TABLE_BASE( (wxWindow*) aParent ),
        m_frame( aParent ),
        m_footprint( aFootprint ),
        m_unitsProvider( std::make_unique<UNITS_PROVIDER>( pcbIUScale, GetUserUnits() ) ),
        m_summaryDirty( true )
{
    CaptureOriginalPadState();

    // The base class created a single placeholder row; resize the grid to fit the pads.
    if( m_grid->GetNumberRows() > 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

    if( !m_originalPads.empty() )
        m_grid->AppendRows( static_cast<int>( m_originalPads.size() ) );

    // Constrain summary label widths so they ellipsize rather than push the layout around
    // when long pin-number summaries (or duplicate lists) are produced.
    const int summaryW = m_pin_numbers_summary->GetCharWidth() * 30;

    m_pin_numbers_summary->SetMinSize( wxSize( summaryW, -1 ) );
    m_pin_numbers_summary->SetMaxSize( wxSize( summaryW, -1 ) );

    m_duplicate_pins->SetMinSize( wxSize( summaryW, -1 ) );
    m_duplicate_pins->SetMaxSize( wxSize( summaryW, -1 ) );

    wxGridCellAttr* attr = nullptr;

    // Type column editor (attribute)
    attr = new wxGridCellAttr;
    wxArrayString typeNames;
    typeNames.push_back( _( "Through-hole" ) ); // PTH
    typeNames.push_back( _( "SMD" ) );          // SMD
    typeNames.push_back( _( "Connector" ) );    // CONN SMD? (use CONN?)
    typeNames.push_back( _( "NPTH" ) );         // NPTH
    typeNames.push_back( _( "Aperture" ) );     // inferred copper-less
    attr->SetEditor( new GRID_CELL_COMBOBOX( typeNames ) );
    m_grid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString shapeNames;
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CIRCLE ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::OVAL ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::RECTANGLE ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::TRAPEZOID ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::ROUNDRECT ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CHAMFERED_RECT ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CUSTOM ) );
    attr->SetEditor( new GRID_CELL_COMBOBOX( shapeNames ) );
    m_grid->SetColAttr( COL_SHAPE, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_POS_X, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_POS_Y, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_SIZE_X, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_SIZE_Y, attr );

    // Drill X
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_DRILL_X, attr );

    // Drill Y
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_DRILL_Y, attr );

    // Pad->Die Length
    m_grid->SetAutoEvalColUnits( COL_P2D_LENGTH, m_unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::DISTANCE ) );

    // Pad->Die Delay
    m_grid->SetAutoEvalColUnits( COL_P2D_DELAY, m_unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::TIME ) );

    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_POS_X );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_POS_Y );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_SIZE_X );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_SIZE_Y );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_DRILL_X );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_DRILL_Y );
    m_grid->SetAutoEvalCols( { COL_POS_X, COL_POS_Y,
                               COL_SIZE_X, COL_SIZE_Y,
                               COL_DRILL_X, COL_DRILL_Y,
                               COL_P2D_LENGTH,
                               COL_P2D_DELAY } );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    SetupStandardButtons();

    Layout();
    finishDialogSettings();

    // Cap the initial height so the dialog does not grow off-screen for footprints
    // with many pads. The grid grows to fill the available space via wxEXPAND.
    // Use the parent window to find the display since this dialog isn't shown yet.
    int displayIdx = wxDisplay::GetFromWindow( aParent );

    if( displayIdx == wxNOT_FOUND )
        displayIdx = 0;

    wxRect displayArea = wxDisplay( (unsigned int) displayIdx ).GetClientArea();
    wxSize dlgSize = GetSize();
    int    maxH = ( displayArea.height * 4 ) / 5;

    if( dlgSize.y > maxH )
    {
        dlgSize.y = maxH;
        SetSize( dlgSize );

        // Reset minimum height so the user can resize the capped dialog freely.
        // The minimum width from finishDialogSettings() is still honoured.
        wxSize minSz = GetMinSize();
        minSz.y = -1;
        SetMinSize( minSz );

        Centre();
    }
}


DIALOG_FP_EDIT_PAD_TABLE::~DIALOG_FP_EDIT_PAD_TABLE()
{
    // Roll back any session changes unless the dialog was accepted. This means
    // rollback happens on both title-bar 'X'/Esc and Cancel.
    if( !m_accepted )
        RestoreOriginalPadState();

    // destroy GRID_TRICKS before m_grid.
    m_grid->PopEventHandler( true );
}


void DIALOG_FP_EDIT_PAD_TABLE::fillGridRow( int aRowId, PAD* aPad )
{
    m_grid->SetCellValue( aRowId, COL_NUMBER, aPad->GetNumber() );

    wxString attrStr = GetPadTypeString( *aPad );
    int      size_x = aPad->GetSize( PADSTACK::ALL_LAYERS ).x;
    int      size_y = aPad->GetSize( PADSTACK::ALL_LAYERS ).y;
    wxString padShape = aPad->ShowPadShape( PADSTACK::ALL_LAYERS );

    aPad->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                if( aPad->GetSize( aLayer ).x != size_x )
                    size_x = -1;

                if( aPad->GetSize( aLayer ).y != size_y )
                    size_y = -1;

                if( aPad->ShowPadShape( aLayer ) != padShape )
                    padShape = INDETERMINATE_STATE;
            } );

    m_grid->SetCellValue( aRowId, COL_TYPE, attrStr );
    m_grid->SetCellValue( aRowId, COL_SHAPE, padShape );
    m_grid->SetCellValue( aRowId, COL_POS_X, m_unitsProvider->StringFromValue( aPad->GetPosition().x, true ) );
    m_grid->SetCellValue( aRowId, COL_POS_Y, m_unitsProvider->StringFromValue( aPad->GetPosition().y, true ) );
    m_grid->SetCellValue( aRowId, COL_SIZE_X,
                          size_x >= 0 ? m_unitsProvider->StringFromValue( size_x, true ) : INDETERMINATE_STATE );
    m_grid->SetCellValue( aRowId, COL_SIZE_Y,
                          size_y >= 0 ? m_unitsProvider->StringFromValue( size_y, true ) : INDETERMINATE_STATE );

    // Drill values (only meaningful for PTH or NPTH). Leave empty otherwise.
    if( aPad->GetAttribute() == PAD_ATTRIB::PTH || aPad->GetAttribute() == PAD_ATTRIB::NPTH )
    {
        VECTOR2I drill = aPad->GetDrillSize();

        if( drill.x > 0 )
            m_grid->SetCellValue( aRowId, COL_DRILL_X, m_unitsProvider->StringFromValue( drill.x, true ) );

        if( drill.y > 0 )
            m_grid->SetCellValue( aRowId, COL_DRILL_Y, m_unitsProvider->StringFromValue( drill.y, true ) );
    }
    else
    {
        // For non-PTH pads, drill columns are not applicable.
        m_grid->SetReadOnly( aRowId, COL_DRILL_X, true );
        m_grid->SetReadOnly( aRowId, COL_DRILL_Y, true );
    }

    // Pad to die metrics
    if( aPad->GetPadToDieLength() )
        m_grid->SetUnitValue( aRowId, COL_P2D_LENGTH, aPad->GetPadToDieLength() );

    if( aPad->GetPadToDieDelay() )
        m_grid->SetUnitValue( aRowId, COL_P2D_DELAY, aPad->GetPadToDieDelay() );

    setRowNullableEditors( aRowId );
}


bool DIALOG_FP_EDIT_PAD_TABLE::TransferDataToWindow()
{
    if( !m_footprint )
        return false;

    int row = 0;

    for( PAD* pad : m_rowPads )
    {
        fillGridRow( row, pad );
        row++;
    }

    // Auto size the data columns first to get reasonable initial widths
    m_grid->AutoSizeColumns();

    // Ensure the Shape column (index 1) is wide enough for the longest translated
    // shape text plus the dropdown arrow / padding. We compute a max text width
    // using a device context and add a platform neutral padding.
    {
        wxClientDC dc( m_grid );
        dc.SetFont( m_grid->GetFont() );

        wxArrayString shapeNames;
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CIRCLE ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::OVAL ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::RECTANGLE ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::TRAPEZOID ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::ROUNDRECT ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CHAMFERED_RECT ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CUSTOM ) );

        int maxWidth = 0;

        for( const wxString& str : shapeNames )
        {
            int w, h;
            dc.GetTextExtent( str, &w, &h );
            maxWidth = std::max( maxWidth, w );
        }

        // Add padding for internal cell margins + dropdown control.
        int padding = FromDIP( 30 ); // heuristic: 2*margin + arrow button
        m_grid->SetColSize( COL_SHAPE, maxWidth + padding );
    }

    // Record initial proportions for proportional resizing later.
    InitColumnProportions();

    // Run an initial proportional resize using current client size so columns
    // respect proportions immediately.
    wxSizeEvent sizeEvt( GetSize(), GetId() );
    CallAfter(
            [this, sizeEvt]
            {
                wxSizeEvent evt( sizeEvt );
                this->OnSize( evt );
            } );

    // If pads exist, select the first row to show initial highlight
    if( m_grid->GetNumberRows() > 0 )
    {
        m_grid->SetGridCursor( 0, 0 );

        // Construct event with required parameters (id, type, obj, row, col,...)
        wxGridEvent ev( m_grid->GetId(), wxEVT_GRID_SELECT_CELL, m_grid, 0, 0, -1, -1, true );
        OnSelectCell( ev );
    }

    return true;
}


void DIALOG_FP_EDIT_PAD_TABLE::setRowNullableEditors( int aRowId ) const
{
    // Set nullable editors
    auto setCellEditor =
            [this, aRowId]( int aCol )
            {
                GRID_CELL_MARK_AS_NULLABLE* cellEditor = new GRID_CELL_MARK_AS_NULLABLE( true );
                wxGridCellAttr*             attr = m_grid->GetOrCreateCellAttr( aRowId, aCol );
                attr->SetEditor( cellEditor );
                attr->DecRef();
            };

    setCellEditor( COL_P2D_LENGTH );
    setCellEditor( COL_P2D_DELAY );
}


void DIALOG_FP_EDIT_PAD_TABLE::CaptureOriginalPadState()
{
    m_originalPads.clear();
    m_rowPads.clear();
    m_removedPads.clear();

    if( !m_footprint )
        return;

    for( PAD* pad : m_footprint->Pads() )
    {
        PAD_SNAPSHOT snap( pad );
        snap.number        = pad->GetNumber();
        snap.position      = pad->GetPosition();
        snap.padstack      = pad->Padstack();
        snap.attribute     = pad->GetAttribute();
        snap.padToDieLength= pad->GetPadToDieLength();
        snap.padToDieDelay = pad->GetPadToDieDelay();

        m_originalPads.try_emplace( pad, std::move( snap ) );
        m_rowPads.push_back( pad );
    }

    std::sort( m_rowPads.begin(), m_rowPads.end(), PAD_SNAPSHOT_COMPARE() );
}


void DIALOG_FP_EDIT_PAD_TABLE::restorePadFromSnapshot( PAD& aPad, const PAD_SNAPSHOT& aSnap ) const
{
    aPad.SetNumber( aSnap.number );
    aPad.SetPosition( aSnap.position );
    aPad.SetPadstack( aSnap.padstack );
    aPad.SetAttribute( aSnap.attribute );
    aPad.SetPadToDieLength( aSnap.padToDieLength );
    aPad.SetPadToDieDelay( aSnap.padToDieDelay );
}


void DIALOG_FP_EDIT_PAD_TABLE::restoreOriginalPadData()
{
    if( !m_footprint )
        return;

    const PCB_BASE_FRAME* base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );
    PCB_DRAW_PANEL_GAL* canvas = base ? base->GetCanvas() : nullptr;

    for( PAD* pad : m_footprint->Pads() )
    {
        if( !m_originalPads.contains( pad ) )
            continue;

        restorePadFromSnapshot( *pad, m_originalPads.at( pad ) );
        pad->ClearBrightened();

        if( canvas )
            canvas->GetView()->Update( pad, KIGFX::REPAINT );
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::RestoreOriginalPadState()
{
    if( !m_footprint )
        return;

    restoreOriginalPadData();

    PCB_BASE_FRAME*     base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );
    PCB_DRAW_PANEL_GAL* canvas = base ? base->GetCanvas() : nullptr;
    KIGFX::PCB_VIEW*    view = canvas ? static_cast<KIGFX::PCB_VIEW*>( canvas->GetView() ) : nullptr;

    // Remove pads added during the session; they have no snapshot entry.
    std::vector<PAD*> livePads( m_footprint->Pads().begin(), m_footprint->Pads().end() );

    for( PAD* pad : livePads )
    {
        if( m_originalPads.contains( pad ) )
            continue;

        m_footprint->Remove( pad );

        if( view )
            view->Remove( pad );

        delete pad;
    }

    // Re-add pads removed during the session and restore their original data.
    // Pads with no snapshot were added and removed again, so just drop them.
    for( PAD* pad : m_removedPads )
    {
        auto snapIt = m_originalPads.find( pad );

        if( snapIt == m_originalPads.end() )
        {
            delete pad;
            continue;
        }

        restorePadFromSnapshot( *pad, snapIt->second );
        m_footprint->Add( pad );

        if( view )
            view->Add( pad );
    }

    m_removedPads.clear();

    // Rebuild the row mapping so no pointers to freed pads survive for any UI
    // events still pending while the dialog closes.
    m_rowPads.clear();

    for( PAD* pad : m_footprint->Pads() )
        m_rowPads.push_back( pad );

    std::sort( m_rowPads.begin(), m_rowPads.end(), PAD_SNAPSHOT_COMPARE() );

    if( canvas )
    {
        canvas->GetView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
        canvas->ForceRefresh();
    }

    m_summaryDirty = true;
}


void DIALOG_FP_EDIT_PAD_TABLE::setPadFromGridCell( PAD& aPad, int aRowId, COLS aCol )
{
    switch( aCol )
    {
    case COL_NUMBER:
        aPad.SetNumber( m_grid->GetCellValue( aRowId, aCol ) );
        break;

    case COL_TYPE:
        SetPadTypeFromString( aPad, m_grid->GetCellValue( aRowId, aCol ) );
        break;

    case COL_SHAPE:
    {
        const wxString shape = m_grid->GetCellValue( aRowId, aCol );

        if( shape == INDETERMINATE_STATE )
            break;

        const PAD_SHAPE newShape = ShapeFromString( shape );

        aPad.Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID aLayer )
                {
                    aPad.SetShape( aLayer, newShape );
                } );
        break;
    }

    case COL_POS_X:
    case COL_POS_Y:
    {
        VECTOR2I pos = aPad.GetPosition();

        if( aCol == COL_POS_X )
            pos.x = m_grid->GetUnitValue( aRowId, aCol );
        else
            pos.y = m_grid->GetUnitValue( aRowId, aCol );

        aPad.SetPosition( pos );
        break;
    }

    case COL_SIZE_X:
    case COL_SIZE_Y:
    {
        const wxString sizeValue = m_grid->GetCellValue( aRowId, aCol );

        if( sizeValue == INDETERMINATE_STATE )
            break;

        const int size = m_grid->GetUnitValue( aRowId, aCol );

        aPad.Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID aLayer )
                {
                    VECTOR2I layerSize = aPad.GetSize( aLayer );

                    if( aCol == COL_SIZE_X )
                        layerSize.x = size;
                    else
                        layerSize.y = size;

                    aPad.SetSize( aLayer, layerSize );
                } );
        break;
    }

    case COL_DRILL_X:
    case COL_DRILL_Y:
    {
        // Drill sizes (only if attribute allows)
        if( aPad.GetAttribute() == PAD_ATTRIB::PTH || aPad.GetAttribute() == PAD_ATTRIB::NPTH )
        {
            int drillX = m_grid->GetUnitValue( aRowId, COL_DRILL_X );
            int drillY = m_grid->GetUnitValue( aRowId, COL_DRILL_Y );

            if( drillX > 0 || drillY > 0 )
            {
                if( drillX <= 0 )
                    drillX = drillY;

                if( drillY <= 0 )
                    drillY = drillX;

                aPad.SetDrillSize( { drillX, drillY } );
            }
        }

        break;
    }

    case COL_P2D_LENGTH:
    {
        const wxString lenStr = m_grid->GetCellValue( aRowId, aCol );

        if( lenStr.IsEmpty() )
            aPad.SetPadToDieLength( 0 );
        else
            aPad.SetPadToDieLength( m_grid->GetUnitValue( aRowId, aCol ) );

        break;
    }

    case COL_P2D_DELAY:
    {
        const wxString delayStr = m_grid->GetCellValue( aRowId, aCol );

        if( delayStr.IsEmpty() )
            aPad.SetPadToDieDelay( 0 );
        else
            aPad.SetPadToDieDelay( m_grid->GetUnitValue( aRowId, aCol ) );

        break;
    }

    default:
        wxFAIL_MSG( wxT( "Invalid column index" ) );
        break;
    }
}


bool DIALOG_FP_EDIT_PAD_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !m_footprint )
        return true;

    PCB_BASE_FRAME*     base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );
    PCB_DRAW_PANEL_GAL* canvas = base ? base->GetCanvas() : nullptr;
    KIGFX::PCB_VIEW*    view = canvas ? static_cast<KIGFX::PCB_VIEW*>( canvas->GetView() ) : nullptr;

    restoreOriginalPadData();

    BOARD_COMMIT commit( m_frame );

    // Pads removed during the session are re-inserted momentarily so their
    // removal can be staged in this commit (Push performs the actual removal).
    for( PAD* pad : m_removedPads )
    {
        auto snapIt = m_originalPads.find( pad );

        if( snapIt == m_originalPads.end() )
        {
            // A pad added and removed again during the session: it never needs
            // to appear in the commit.
            delete pad;
            continue;
        }

        restorePadFromSnapshot( *pad, snapIt->second );
        m_footprint->Add( pad );

        if( view )
            view->Add( pad );

        commit.Remove( pad );
    }

    m_removedPads.clear();

    int row = 0;

    const auto applyRowDataToPad = [&]( PAD& aPad, int aRowId )
    {
        for( int col = 0; col < m_grid->GetNumberCols(); ++col )
            setPadFromGridCell( aPad, aRowId, static_cast<COLS>( col ) );
    };

    for( PAD* pad : m_rowPads )
    {
        if( m_originalPads.contains( pad ) )
        {
            // Existing pad: its data was already restored to the dialog-open
            // state above, so the commit's undo image is correct.
            commit.Modify( pad );
            applyRowDataToPad( *pad, row );
        }
        else
        {
            // Imported pad: it was added to the footprint at import time for the
            // canvas preview. Take it out again so the commit can stage a clean
            // addition.
            applyRowDataToPad( *pad, row );

            m_footprint->Remove( pad );

            if( view )
                view->Remove( pad );

            commit.Add( pad );
        }

        row++;
    }

    commit.Push( _( "Edit Pads" ) );
    m_frame->Refresh();

    m_accepted = true;

    return true;
}


void DIALOG_FP_EDIT_PAD_TABLE::InitColumnProportions()
{
    m_colProportions.clear();
    m_minColWidths.clear();

    if( !m_grid )
        return;

    // Only consider the actual data columns (all of them since row labels are hidden)
    int cols = m_grid->GetNumberCols();
    int total = 0;
    std::vector<int> widths;
    widths.reserve( cols );

    for( int c = 0; c < cols; ++c )
    {
        int w = m_grid->GetColSize( c );
        widths.push_back( w );
        total += w;
    }

    if( total <= 0 )
        return;

    for( int w : widths )
    {
        m_colProportions.push_back( (double) w / (double) total );
        m_minColWidths.push_back( w );
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnSize( wxSizeEvent& aEvent )
{
    if( m_colProportions.empty() )
    {
        aEvent.Skip();
        return;
    }

    // Compute available total width for columns and resize keeping proportions.
    int cols = m_grid->GetNumberCols();
    int available = 0;

    for( int c = 0; c < cols; ++c )
        available += m_grid->GetColSize( c );

    // Use client size of grid minus scrollbar estimate to better distribute.
    int clientW = m_grid->GetClientSize().x;

    if( clientW > 0 )
        available = clientW; // prefer actual client width

    int used = 0;

    for( int c = 0; c < cols; ++c )
    {
        int target = (int) std::round( m_colProportions[c] * available );
        target = std::max( target, m_minColWidths[c] );

        // Defer last column to absorb rounding diff.
        if( c == cols - 1 )
            target = std::max( available - used, m_minColWidths[c] );

        m_grid->SetColSize( c, target );
        used += target;
    }

    aEvent.Skip();
}


void DIALOG_FP_EDIT_PAD_TABLE::OnCharHook( wxKeyEvent& aEvent )
{
    if( m_grid->IsCellEditControlShown() && m_grid->GetGridCursorCol() == COL_NUMBER )
        m_summaryDirty = true;

    DIALOG_SHIM::OnCharHook( aEvent );
}


void DIALOG_FP_EDIT_PAD_TABLE::OnCellChanged( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( !m_footprint )
        return;

    PAD* target = getPadForRow( row );

    if( !target )
        return;

    bool needCanvasRefresh = true;

    setPadFromGridCell( *target, row, static_cast<COLS>( col ) );

    if( col == COL_TYPE )
    {
        // Toggle drill columns read-only state dynamically.
        const bool drillsEditable =
                ( target->GetAttribute() == PAD_ATTRIB::PTH || target->GetAttribute() == PAD_ATTRIB::NPTH );
        m_grid->SetReadOnly( row, COL_DRILL_X, !drillsEditable );
        m_grid->SetReadOnly( row, COL_DRILL_Y, !drillsEditable );
    }
    else if( col == COL_P2D_LENGTH || col == COL_P2D_DELAY )
    {
        // Pad-to-die values are not drawn on the canvas.
        needCanvasRefresh = false;
    }

    if( col == COL_NUMBER )
        m_summaryDirty = true;

    // Request redraw (simple approach)
    target->SetDirty();

    if( needCanvasRefresh )
    {
        if( PCB_BASE_FRAME* base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() ) )
        {
            if( KIGFX::PCB_VIEW* view = static_cast<KIGFX::PCB_VIEW*>( base->GetCanvas()->GetView() ) )
                view->Update( target, KIGFX::REPAINT );

            base->GetCanvas()->ForceRefresh();
        }
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnSelectCell( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    if( !m_footprint )
        return;

    PCB_BASE_FRAME* base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );
    PCB_DRAW_PANEL_GAL* canvas = base ? base->GetCanvas() : nullptr;

    // Clear existing pad selections
    for( PAD* pad : m_footprint->Pads() )
    {
        if( pad->IsBrightened() )
        {
            pad->ClearBrightened();

            if( canvas )
                canvas->GetView()->Update( pad, KIGFX::REPAINT );
        }
    }

    PAD* pad = getPadForRow( row );

    if( !pad )
        return;

    pad->SetBrightened();

    if( canvas )
    {
        canvas->GetView()->Update( pad, KIGFX::REPAINT );
        canvas->ForceRefresh();
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnUpdateUI( wxUpdateUIEvent& aEvent )
{
    if( m_summaryDirty )
    {
        if( m_grid->IsCellEditControlShown() && m_grid->GetGridCursorCol() == COL_NUMBER )
        {
            int  row = m_grid->GetGridCursorRow();
            int  col = m_grid->GetGridCursorCol();

            PAD* target = getPadForRow( row );

            if( !target )
                return;

            wxGridCellEditor* editor = m_grid->GetCellEditor( row, col );

            if( editor )
            {
                target->SetNumber( editor->GetValue() );
                editor->DecRef();
            }
        }

        updateSummary();
        m_summaryDirty = false;
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnCancel( wxCommandEvent& aEvent )
{
    // The destructor rolls back everything that was not accepted.
    m_accepted = false;
    aEvent.Skip();
}


void DIALOG_FP_EDIT_PAD_TABLE::updateSummary()
{
    PIN_NUMBERS pinNumbers;

    for( PAD* pad : m_footprint->Pads() )
    {
        if( pad->GetNumber().Length() )
            pinNumbers.insert( pad->GetNumber() );
    }

    const wxString summary = pinNumbers.GetSummary();
    const wxString duplicates = pinNumbers.GetDuplicates();

    m_pin_numbers_summary->SetLabel( summary );
    m_pin_numbers_summary->SetToolTip( summary );
    m_pin_count->SetLabel( wxString::Format( wxT( "%u" ), (unsigned) m_footprint->Pads().size() ) );
    m_duplicate_pins->SetLabel( duplicates );
    m_duplicate_pins->SetToolTip( duplicates );

    Layout();
}


PAD* DIALOG_FP_EDIT_PAD_TABLE::getPadForRow( const int aRowId ) const
{
    if( aRowId < 0 || static_cast<size_t>( aRowId ) >= m_rowPads.size() )
        return nullptr;

    return m_rowPads[aRowId];
}


void DIALOG_FP_EDIT_PAD_TABLE::OnExportButtonClick( wxCommandEvent& aEvent )
{
    bool toFile = aEvent.GetEventObject() == m_btnExportToFile;

    wxString filePath;

    if( toFile )
    {
        wxFileName fn( m_footprint->GetFPID().GetLibItemName() );
        fn.SetExt( FILEEXT::CsvFileExtension );

        wxFileDialog dlg( this, _( "Select pad data file" ), "", fn.GetFullName(), FILEEXT::CsvTsvFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        filePath = dlg.GetPath();
    }

    std::vector<PAD*> padsToExport;

    for( PAD* pad : m_rowPads )
        padsToExport.push_back( pad );

    static const std::vector<COLS> exportCols {
        COLS::COL_NUMBER,
        COLS::COL_TYPE,
        COLS::COL_SHAPE,
        COLS::COL_POS_X,
        COLS::COL_POS_Y,
        COLS::COL_SIZE_X,
        COLS::COL_SIZE_Y,
        COLS::COL_DRILL_X,
        COLS::COL_DRILL_Y,
        COLS::COL_P2D_LENGTH,
        COLS::COL_P2D_DELAY,
    };

    NULL_REPORTER      reporter;
    PAD_INFO_FORMATTER fmt( *m_unitsProvider, false, COLUMN_FORMATTER::BOOL_FORMAT::TRUE_FALSE, reporter );

    std::vector<std::vector<wxString>> table;
    table.reserve( padsToExport.size() + 1 );

    std::vector<wxString> header;
    header.reserve( exportCols.size() );

    for( COLS col : exportCols )
        header.emplace_back( wxGetTranslation( GetPadTableColLabel( col ) ) );

    table.emplace_back( std::move( header ) );

    for( PAD* pad : padsToExport )
    {
        std::vector<wxString>& row = table.emplace_back();
        row.reserve( exportCols.size() );

        for( COLS col : exportCols )
            row.emplace_back( fmt.Format( *pad, col ) );
    }

    WriteTableToFileOrClipboard( filePath, table );
}


void DIALOG_FP_EDIT_PAD_TABLE::OnImportButtonClick( wxCommandEvent& aEvent )
{
    bool fromFile = aEvent.GetEventObject() == m_btnImportFromFile;
    bool replaceAll = m_rbReplaceExisting->GetValue();

    WX_STRING_REPORTER reporter;

    PAD_INFO_FORMATTER fmt( *m_frame, false, COLUMN_FORMATTER::BOOL_FORMAT::TRUE_FALSE, reporter );

    std::optional<std::vector<std::vector<wxString>>> csvData = ReadTableFromFileOrClipboard( *m_frame, fromFile );

    // The pad table does not and cannot capture the full glory of a PAD object
    // (for example a custom pad's custom shape). So, when we are re-importing
    // pads and "replacing existing", we attempt to rematch the imported pads
    // to the existing ones by pad number. If a match is found, we update the
    // existing pad with the imported data. If no match is found, we add the
    // imported pad as a new pad. If "replace existing" is not selected, we
    // simply add the imported pads as new pads.

    // Group the current pads by case-insensitive pad number so each imported
    // row can be rematched with a single map lookup rather than a linear scan.
    // Buckets keep the grid order, so this is stable as we'll match in grid order.
    std::map<wxString, std::vector<PAD*>> padsByNumber;

    for( PAD* pad : m_rowPads )
        padsByNumber[pad->GetNumber()].push_back( pad );

    PCB_BASE_FRAME*     base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );
    PCB_DRAW_PANEL_GAL* canvas = base ? base->GetCanvas() : nullptr;
    KIGFX::PCB_VIEW*    view = canvas ? static_cast<KIGFX::PCB_VIEW*>( canvas->GetView() ) : nullptr;

    std::vector<std::unique_ptr<PAD>> createdPads;

    if( csvData && csvData->size() >= 2 )
    {
        std::vector<COLS>     headerCols;
        wxArrayString         unknownHeaders;
        std::optional<size_t> numberColIdx;

        for( const wxString& label : ( *csvData )[0] )
        {
            COLS col = GetColTypeForString( label );

            if( col >= COLS::COL_COUNT )
                unknownHeaders.push_back( label );

            if( col == COLS::COL_NUMBER )
                numberColIdx = headerCols.size();

            headerCols.push_back( col );
        }

        if( replaceAll && !numberColIdx.has_value() )
        {
            wxString msg = _( "Imported pad data must include pad numbers." );
            wxMessageBox( msg, _( "Import error" ), wxOK | wxICON_ERROR, this );
            return;
        }

        if( !unknownHeaders.IsEmpty() )
        {
            wxString msg = wxString::Format( _( "Unknown columns in data: %s. These columns will be ignored." ),
                                             AccumulateDescriptions( unknownHeaders ) );
            reporter.Report( msg, RPT_SEVERITY_WARNING );
        }

        if( reporter.HasMessage() )
        {
            int ret = wxMessageBox( reporter.GetMessages(), _( "Errors" ), wxOK | wxCANCEL | wxICON_ERROR, this );

            if( ret == wxCANCEL )
                return;
        }

        for( size_t i = 1; i < csvData->size(); ++i )
        {
            const std::vector<wxString>& cols = ( *csvData )[i];

            // Decide whether to create a new pad or update an existing one based on the
            // "replace existing" option and the pad number.
            std::unique_ptr<PAD> newPad;
            PAD*                 padToUpdate = nullptr;

            const size_t numberCol = numberColIdx.value_or( cols.size() );

            if( replaceAll && numberCol < cols.size() && !cols[numberCol].IsEmpty() )
            {
                auto bucketIt = padsByNumber.find( cols[numberCol] );

                if( bucketIt != padsByNumber.end() && !bucketIt->second.empty() )
                {
                    padToUpdate = bucketIt->second.front();
                    bucketIt->second.erase( bucketIt->second.begin() );
                }
            }

            if( !padToUpdate )
            {
                // We are adding a new pad, not updating an existing one.
                newPad = std::make_unique<PAD>( m_footprint );
                padToUpdate = newPad.get();
            }

            size_t maxCol = std::min( headerCols.size(), cols.size() );

            for( size_t j = 0; j < maxCol; ++j )
            {
                if( headerCols[j] == COLS::COL_COUNT )
                    continue;

                fmt.UpdatePad( *padToUpdate, cols[j], headerCols[j] );
            }

            // Invalidate the pad's cached draw data and mark it for re-render
            // so the canvas refresh below shows the imported values.
            padToUpdate->SetDirty();

            if( view )
                view->Update( padToUpdate, KIGFX::REPAINT );

            if( newPad )
            {
                createdPads.push_back( std::move( newPad ) );
            }
        }

        // Bulk-load the imported pads into the view.
        if( !createdPads.empty() )
        {
            if( view )
            {
                std::vector<KIGFX::VIEW_ITEM*> viewPads;
                for( const auto& pad : createdPads )
                    viewPads.push_back( pad.get() );

                view->AddBatch( viewPads );
            }

            for( auto& pad : createdPads )
            {
                m_footprint->Add( pad.release(), ADD_MODE::BULK_APPEND, true );
            }
        }

        // Any pads not matched to an imported pad are removed from the footprint.
        // They are kept alive so they can be restored on cancel and staged as
        // removals when the dialog is accepted.
        if( replaceAll )
        {
            for( const auto& [number, pads] : padsByNumber )
            {
                for( PAD* pad : pads )
                {
                    m_footprint->Remove( pad );

                    if( view )
                        view->Remove( pad );

                    m_removedPads.push_back( pad );
                }
            }
        }
    }

    // Commit any in-progress cell edits so the grid rebuild below does not
    // discard them.
    m_grid->CommitPendingChanges();

    // Rebuild the row mapping from the pads now in the footprint.
    m_rowPads.clear();

    for( PAD* pad : m_footprint->Pads() )
        m_rowPads.push_back( pad );

    std::sort( m_rowPads.begin(), m_rowPads.end(), PAD_SNAPSHOT_COMPARE() );

    // Rebuild the grid to match the new pad count.
    int currentRows = m_grid->GetNumberRows();
    int neededRows = static_cast<int>( m_rowPads.size() );

    if( neededRows > currentRows )
        m_grid->AppendRows( neededRows - currentRows );
    else if( neededRows < currentRows )
        m_grid->DeleteRows( neededRows, currentRows - neededRows );

    TransferDataToWindow();
    updateSummary();

    if( canvas )
        canvas->ForceRefresh();
}
