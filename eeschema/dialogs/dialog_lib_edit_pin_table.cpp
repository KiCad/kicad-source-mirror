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

#include "dialog_lib_edit_pin_table.h"

#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>

#include "grid_tricks.h"
#include <richio.h>
#include <sch_pin.h>
#include <pin_numbers.h>
#include "pgm_base.h"
#include <base_units.h>
#include <bitmaps.h>
#include <clipboard.h>
#include <confirm.h>
#include <symbol_edit_frame.h>
#include <symbol_editor_settings.h>
#include <io/csv.h>
#include <kiplatform/ui.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <widgets/wx_grid.h>
#include <widgets/bitmap_button.h>
#include <widgets/std_bitmap_button.h>
#include <wildcards_and_files_ext.h>
#include <settings/settings_manager.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <string_utils.h>

#define BOOL_TRUE _HKI( "True" )
#define BOOL_FALSE _HKI( "False" )


/**
 * Get the label for a given column in the pin table.
 *
 * This string is NOT translated.
 */
static wxString GetPinTableColLabel( int aCol )
{
    switch( aCol )
    {
    case COL_PIN_COUNT:    return _HKI( "Count" );
    case COL_NUMBER:       return _HKI( "Number" );
    case COL_NAME:         return _HKI( "Name" );
    case COL_TYPE:         return _HKI( "Electrical Type" );
    case COL_SHAPE:        return _HKI( "Graphic Style" );
    case COL_ORIENTATION:  return _HKI( "Orientation" );
    case COL_NUMBER_SIZE:  return _HKI( "Number Text Size" );
    case COL_NAME_SIZE:    return _HKI( "Name Text Size" );
    case COL_LENGTH:       return _HKI( "Length" );
    case COL_POSX:         return _HKI( "X Position" );
    case COL_POSY:         return _HKI( "Y Position" );
    case COL_VISIBLE:      return _HKI( "Visible" );
    case COL_UNIT:         return _HKI( "Unit" );
    case COL_BODY_STYLE:   return _HKI( "Body Style" );
    default:               wxFAIL; return wxEmptyString;
    }
}


static bool MatchTranslationOrNative( const wxString& aStr, const wxString& aNativeLabel, bool aCaseSensitive )
{
    return wxGetTranslation( aNativeLabel ).IsSameAs( aStr, aCaseSensitive )
           || aStr.IsSameAs( aNativeLabel, aCaseSensitive );
}


static COL_ORDER GetColTypeForString( const wxString& aStr )
{
    for( int i = 0; i < COL_COUNT; i++ )
    {
        if( MatchTranslationOrNative( aStr, GetPinTableColLabel( i ), false ) )
            return (COL_ORDER) i;
    }
    return COL_COUNT;
}

/**
 * Class that handles conversion of various pin data fields into strings for display in the
 * UI or serialisation to formats like CSV.
 */
class PIN_INFO_FORMATTER
{
public:
    enum class BOOL_FORMAT
    {
        ZERO_ONE,
        TRUE_FALSE,
    };

    PIN_INFO_FORMATTER( UNITS_PROVIDER& aUnitsProvider, bool aIncludeUnits, BOOL_FORMAT aBoolFormat,
                        REPORTER& aReporter ) :
            m_unitsProvider( aUnitsProvider ),
            m_includeUnits( aIncludeUnits ),
            m_boolFormat( aBoolFormat ),
            m_reporter( aReporter )
    {
    }

    wxString Format( const SCH_PIN& aPin, int aFieldId ) const
    {
        switch( aFieldId )
        {
        case COL_NAME:
            return aPin.GetName();

        case COL_NUMBER:
            return aPin.GetNumber();

        case COL_TYPE:
            return PinTypeNames()[static_cast<int>( aPin.GetType() )];

        case COL_SHAPE:
            return PinShapeNames()[static_cast<int>( aPin.GetShape() )];

        case COL_ORIENTATION:
        {
            const int index = PinOrientationIndex( aPin.GetOrientation() );

            if( index >= 0)
                return PinOrientationNames()[ index ];

            return wxEmptyString;
        }

        case COL_NUMBER_SIZE:
            return m_unitsProvider.StringFromValue( aPin.GetNumberTextSize(), m_includeUnits );

        case COL_NAME_SIZE:
            return m_unitsProvider.StringFromValue( aPin.GetNameTextSize(), m_includeUnits );

        case COL_LENGTH:
            return m_unitsProvider.StringFromValue( aPin.GetLength(), m_includeUnits );

        case COL_POSX:
            return m_unitsProvider.StringFromValue( aPin.GetPosition().x, m_includeUnits );

        case COL_POSY:
            return m_unitsProvider.StringFromValue( aPin.GetPosition().y, m_includeUnits );

        case COL_VISIBLE:
            return stringFromBool( aPin.IsVisible() );

        case COL_UNIT:
            if( const SYMBOL* parent = aPin.GetParentSymbol() )
            {
                if( !parent->IsMultiUnit() )
                    return wxGetTranslation( UNITS_ALL );
            }

            if( aPin.GetUnit() == 0 )
                return wxGetTranslation( UNITS_ALL );
            else
                return aPin.GetUnitDisplayName( aPin.GetUnit(), true );

        case COL_BODY_STYLE:
            if( const SYMBOL* parent = aPin.GetParentSymbol() )
            {
                if( !parent->IsMultiBodyStyle() )
                    return wxGetTranslation( UNITS_ALL );
            }

            if( aPin.GetBodyStyle() == 0 )
                return wxGetTranslation( DEMORGAN_ALL );
            else
                return aPin.GetBodyStyleDescription( aPin.GetBodyStyle(), true );

        default:
            wxFAIL_MSG( wxString::Format( "Invalid field id %d", aFieldId ) );
            return wxEmptyString;
        }
    }

    /**
     * Update the pin from the given col/string.
     *
     * How much this should follow the format is debatable, but for now it's fairly permissive
     * (e.g. bools import as 0/1 and no/yes).
     */
    void UpdatePin( SCH_PIN& aPin, const wxString& aValue, int aFieldId, const LIB_SYMBOL& aSymbol ) const
    {
        switch( aFieldId )
        {
        case COL_NUMBER:
            aPin.SetNumber( aValue );
            break;

        case COL_NAME:
            aPin.SetName( aValue );
            break;

        case COL_TYPE:
            if( PinTypeNames().Index( aValue, false ) != wxNOT_FOUND )
                aPin.SetType( (ELECTRICAL_PINTYPE) PinTypeNames().Index( aValue ) );

            break;

        case COL_SHAPE:
            if( PinShapeNames().Index( aValue, false ) != wxNOT_FOUND )
                aPin.SetShape( (GRAPHIC_PINSHAPE) PinShapeNames().Index( aValue ) );

            break;

        case COL_ORIENTATION:
            if( PinOrientationNames().Index( aValue, false ) != wxNOT_FOUND )
                aPin.SetOrientation( (PIN_ORIENTATION) PinOrientationNames().Index( aValue ) );

            break;

        case COL_NUMBER_SIZE:
            aPin.SetNumberTextSize( m_unitsProvider.ValueFromString( aValue ) );
            break;

        case COL_NAME_SIZE:
            aPin.SetNameTextSize( m_unitsProvider.ValueFromString( aValue ) );
            break;

        case COL_LENGTH:
            aPin.ChangeLength( m_unitsProvider.ValueFromString( aValue ) );
            break;

        case COL_POSX:
            aPin.SetPosition( VECTOR2I( m_unitsProvider.ValueFromString( aValue ), aPin.GetPosition().y ) );
            break;

        case COL_POSY:
            aPin.SetPosition( VECTOR2I( aPin.GetPosition().x, m_unitsProvider.ValueFromString( aValue ) ) );
            break;

        case COL_VISIBLE:
            aPin.SetVisible(boolFromString( aValue, m_reporter ) );
            break;

        case COL_UNIT:
            if( const SYMBOL* parent = aPin.GetParentSymbol() )
            {
                if( !parent->IsMultiUnit() )
                    break;
            }

            if( MatchTranslationOrNative( aValue, UNITS_ALL, false ) )
            {
                aPin.SetUnit( 0 );
                break;
            }

            for( int i = 1; i <= aSymbol.GetUnitCount(); i++ )
            {
                if( aValue == aPin.GetBodyStyleDescription( i, true )
                        || aValue == aPin.GetBodyStyleDescription( i, false ) )
                {
                    aPin.SetUnit( i );
                    break;
                }
            }

            break;

        case COL_BODY_STYLE:
            if( const SYMBOL* parent = aPin.GetParentSymbol() )
            {
                if( !parent->IsMultiBodyStyle() )
                    break;
            }

            if( MatchTranslationOrNative( aValue, DEMORGAN_ALL, false ) )
            {
                aPin.SetBodyStyle( 0 );
                break;
            }

            for( int i = 1; i <= aSymbol.GetBodyStyleCount(); i++ )
            {
                if( aValue == aPin.GetBodyStyleDescription( i, true )
                        || aValue == aPin.GetBodyStyleDescription( i, false ) )
                {
                    aPin.SetBodyStyle( i );
                    break;
                }
            }

            break;

        default:
            wxFAIL_MSG( wxString::Format( "Invalid field id %d", aFieldId ) );
            break;
        }
    }

private:
    wxString stringFromBool( bool aValue ) const
    {
        switch( m_boolFormat )
        {
        case BOOL_FORMAT::ZERO_ONE:
            return aValue ? wxT( "1" ) : wxT( "0" );
        case BOOL_FORMAT::TRUE_FALSE:
            return wxGetTranslation( aValue ? BOOL_TRUE : BOOL_FALSE );
        default:
            wxFAIL_MSG( "Invalid BOOL_FORMAT" );
            return wxEmptyString;
        }

    }

    bool boolFromString( const wxString& aValue, REPORTER& aReporter ) const
    {
        if( aValue == wxS( "1" ) )
            return true;
        else if( aValue == wxS( "0" ) )
            return false;
        else if( MatchTranslationOrNative( aValue, BOOL_TRUE, false ) )
            return true;
        else if( MatchTranslationOrNative( aValue, BOOL_FALSE, false ) )
            return false;

        aReporter.Report( wxString::Format( _( "The value '%s' can't be converted to boolean correctly, "
                                               "it has been interpreted as 'False'" ),
                                            aValue ),
                          RPT_SEVERITY_ERROR );
        return false;
    }

    UNITS_PROVIDER& m_unitsProvider;
    bool            m_includeUnits;
    BOOL_FORMAT     m_boolFormat;
    REPORTER&       m_reporter;
};


void getSelectedArea( WX_GRID* aGrid, int* aRowStart, int* aRowCount )
{
    wxGridCellCoordsArray topLeft  = aGrid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray botRight = aGrid->GetSelectionBlockBottomRight();

    wxArrayInt  cols = aGrid->GetSelectedCols();
    wxArrayInt  rows = aGrid->GetSelectedRows();

    if( topLeft.Count() && botRight.Count() )
    {
        *aRowStart = topLeft[0].GetRow();
        *aRowCount = botRight[0].GetRow() - *aRowStart + 1;
    }
    else if( cols.Count() )
    {
        *aRowStart = 0;
        *aRowCount = aGrid->GetNumberRows();
    }
    else if( rows.Count() )
    {
        *aRowStart = rows[0];
        *aRowCount = rows.Count();
    }
    else
    {
        *aRowStart = aGrid->GetGridCursorRow();
        *aRowCount = *aRowStart >= 0 ? 1 : 0;
    }
}


class PIN_TABLE_DATA_MODEL : public WX_GRID_TABLE_BASE
{
public:
    PIN_TABLE_DATA_MODEL( SYMBOL_EDIT_FRAME* aFrame,
                          DIALOG_LIB_EDIT_PIN_TABLE* aPinTable,
                          LIB_SYMBOL* aSymbol,
                          const std::vector<SCH_PIN*>& aOrigSelectedPins ) :
            m_frame( aFrame ),
            m_unitFilter( -1 ),
            m_bodyStyleFilter( -1 ),
            m_filterBySelection( false ),
            m_edited( false ),
            m_pinTable( aPinTable ),
            m_symbol( aSymbol ),
            m_origSelectedPins( aOrigSelectedPins )
    {
        m_eval = std::make_unique<NUMERIC_EVALUATOR>( m_frame->GetUserUnits() );

        m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PIN_TABLE_DATA_MODEL::onUnitsChanged, this );
    }

    ~PIN_TABLE_DATA_MODEL()
    {
        m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &PIN_TABLE_DATA_MODEL::onUnitsChanged, this );
    }

    void onUnitsChanged( wxCommandEvent& aEvent )
    {
        if( GetView() )
            GetView()->ForceRefresh();

        aEvent.Skip();
    }

    void SetUnitFilter( int aFilter ) { m_unitFilter = aFilter; }
    void SetBodyStyleFilter( int aFilter ) { m_bodyStyleFilter = aFilter; }
    void SetFilterBySelection( bool aFilter ) { m_filterBySelection = aFilter; }

    int GetNumberRows() override { return (int) m_rows.size(); }
    int GetNumberCols() override { return COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override
    {
        return wxGetTranslation( GetPinTableColLabel( aCol ) );
    }

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        wxGrid*  grid = GetView();

        if( grid->GetGridCursorRow() == aRow && grid->GetGridCursorCol() == aCol && grid->IsCellEditControlShown() )
        {
            auto it = m_evalOriginal.find( { m_rows[ aRow ], aCol } );

            if( it != m_evalOriginal.end() )
                return it->second;
        }

        return GetValue( m_rows[ aRow ], aCol, m_frame );
    }

    static wxString GetValue( const std::vector<SCH_PIN*>& pins, int aCol, EDA_DRAW_FRAME* aParentFrame )
    {
        wxString fieldValue;

        if( pins.empty() )
            return fieldValue;

        NULL_REPORTER      reporter;
        PIN_INFO_FORMATTER formatter( *aParentFrame, true, PIN_INFO_FORMATTER::BOOL_FORMAT::ZERO_ONE, reporter );

        for( const SCH_PIN* pin : pins )
        {
            wxString val;
            switch( aCol )
            {
            case COL_PIN_COUNT:
                val << pins.size();
                break;
            default:
                val << formatter.Format( *pin, aCol );
                break;
            }

            if( aCol == COL_NUMBER )
            {
                if( fieldValue.length() )
                    fieldValue += wxT( ", " );

                fieldValue += val;
            }
            else
            {
                if( !fieldValue.Length() )
                    fieldValue = val;
                else if( val != fieldValue )
                    fieldValue = INDETERMINATE_STATE;
            }
        }

        return fieldValue;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aValue == INDETERMINATE_STATE )
            return;

        wxString value = aValue;

        switch( aCol )
        {
        case COL_NUMBER_SIZE:
        case COL_NAME_SIZE:
        case COL_LENGTH:
        case COL_POSX:
        case COL_POSY:
            m_eval->SetDefaultUnits( m_frame->GetUserUnits() );

            if( m_eval->Process( value ) )
            {
                m_evalOriginal[ { m_rows[ aRow ], aCol } ] = value;
                value = m_eval->Result();
            }

            break;

        default:
            break;
        }

        std::vector<SCH_PIN*> pins = m_rows[ aRow ];

        // If the NUMBER column is edited and the pins are grouped, renumber, and add or
        // remove pins based on the comma separated list of pins.
        if( aCol == COL_NUMBER && m_pinTable->IsDisplayGrouped() )
        {
            wxStringTokenizer tokenizer( value, "," );
            size_t            i = 0;

            while( tokenizer.HasMoreTokens() )
            {
                wxString pinName = tokenizer.GetNextToken();

                // Trim whitespace from both ends of the string
                pinName.Trim( true ).Trim( false );

                if( i < pins.size() )
                {
                    // Renumber the existing pins
                    pins.at( i )->SetNumber( pinName );
                }
                else
                {
                    // Create new pins
                    SCH_PIN* newPin = new SCH_PIN( this->m_symbol );
                    SCH_PIN* last = pins.back();

                    newPin->SetNumber( pinName );
                    newPin->SetName( last->GetName() );
                    newPin->SetOrientation( last->GetOrientation() );
                    newPin->SetType( last->GetType() );
                    newPin->SetShape( last->GetShape() );
                    newPin->SetUnit( last->GetUnit() );

                    VECTOR2I pos = last->GetPosition();

                    if( SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
                    {
                        if( last->GetOrientation() == PIN_ORIENTATION::PIN_LEFT
                            || last->GetOrientation() == PIN_ORIENTATION::PIN_RIGHT )
                        {
                            pos.y -= schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
                        }
                        else
                        {
                            pos.x += schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
                        }
                    }

                    newPin->SetPosition( pos );

                    pins.push_back( newPin );
                    m_pinTable->AddPin( newPin );
                }

                i++;
            }

            while( pins.size() > i )
            {
                m_pinTable->RemovePin( pins.back() );
                pins.pop_back();
            }

            m_rows[aRow] = pins;
            m_edited = true;

            return;
        }

        NULL_REPORTER      reporter;
        PIN_INFO_FORMATTER formatter( *m_frame, true, PIN_INFO_FORMATTER::BOOL_FORMAT::ZERO_ONE, reporter );

        for( SCH_PIN* pin : pins )
            formatter.UpdatePin( *pin, value, aCol, *m_symbol );

        m_edited = true;
    }

    static int findRow( const std::vector<std::vector<SCH_PIN*>>& aRowSet, const wxString& aName )
    {
        for( size_t i = 0; i < aRowSet.size(); ++i )
        {
            if( aRowSet[ i ][ 0 ] && aRowSet[ i ][ 0 ]->GetName() == aName )
                return i;
        }

        return -1;
    }

    static bool compare( const std::vector<SCH_PIN*>& lhs, const std::vector<SCH_PIN*>& rhs,
                         int sortCol, bool ascending, EDA_DRAW_FRAME* parentFrame )
    {
        wxString lhStr = GetValue( lhs, sortCol, parentFrame );
        wxString rhStr = GetValue( rhs, sortCol, parentFrame );

        if( lhStr == rhStr )
        {
            // Secondary sort key is always COL_NUMBER
            sortCol = COL_NUMBER;
            lhStr = GetValue( lhs, sortCol, parentFrame );
            rhStr = GetValue( rhs, sortCol, parentFrame );
        }

        bool res;

        // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
        // to get the opposite sort.  i.e. ~(a<b) != (a>b)
        auto cmp =
                [ ascending ]( const auto a, const auto b )
                {
                    if( ascending )
                        return a < b;
                    else
                        return b < a;
                };

        switch( sortCol )
        {
        case COL_NUMBER:
        case COL_NAME:
            res = cmp( PIN_NUMBERS::Compare( lhStr, rhStr ), 0 );
            break;

        case COL_NUMBER_SIZE:
        case COL_NAME_SIZE:
            res = cmp( parentFrame->ValueFromString( lhStr ), parentFrame->ValueFromString( rhStr ) );
            break;

        case COL_LENGTH:
        case COL_POSX:
        case COL_POSY:
            res = cmp( parentFrame->ValueFromString( lhStr ), parentFrame->ValueFromString( rhStr ) );
            break;

        case COL_VISIBLE:
        case COL_UNIT:
        case COL_BODY_STYLE:
        default:
            res = cmp( StrNumCmp( lhStr, rhStr ), 0 );
            break;
        }

        return res;
    }

    void RebuildRows( const std::vector<SCH_PIN*>& aPins, bool groupByName, bool groupBySelection )
    {
        WX_GRID* grid = dynamic_cast<WX_GRID*>( GetView() );
        std::vector<SCH_PIN*> clear_flags;

        clear_flags.reserve( aPins.size() );

        if( grid )
        {
            if( groupBySelection )
            {
                for( SCH_PIN* pin : aPins )
                    pin->ClearTempFlags();

                int firstSelectedRow;
                int selectedRowCount;

                getSelectedArea( grid, &firstSelectedRow, &selectedRowCount );

                for( int ii = 0; ii < selectedRowCount; ++ii )
                {
                    for( SCH_PIN* pin : m_rows[ firstSelectedRow + ii ] )
                    {
                        pin->SetFlags( CANDIDATE );
                        clear_flags.push_back( pin );
                    }
                }
            }

            // Commit any pending in-place edits before the row gets moved out from under
            // the editor.
            grid->CommitPendingChanges( true );

            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }

        m_rows.clear();

        if( groupBySelection )
            m_rows.emplace_back( std::vector<SCH_PIN*>() );

        std::set<wxString> selectedNumbers;

        for( SCH_PIN* pin : m_origSelectedPins )
            selectedNumbers.insert( pin->GetNumber() );

        const auto pinIsInEditorSelection =
                [&]( SCH_PIN* pin )
                {
                    // Quick check before we iterate the whole thing in N^2 time.
                    // (3000^2 = FPGAs causing issues down the road).
                    if( selectedNumbers.count( pin->GetNumber() ) == 0 )
                        return false;

                    for( SCH_PIN* selectedPin : m_origSelectedPins )
                    {
                        // The selected pin is in the editor, but the pins in the table
                        // are copies. We will mark the pin as selected if it's a match
                        // on the critical items.
                        if( selectedPin->GetNumber() == pin->GetNumber()
                            && selectedPin->GetName() == pin->GetName()
                            && selectedPin->GetUnit() == pin->GetUnit()
                            && selectedPin->GetBodyStyle() == pin->GetBodyStyle() )
                        {
                            return true;
                        }
                    }

                    return false;
                };

        for( SCH_PIN* pin : aPins )
        {
            const bool includedByUnit = m_unitFilter == -1 || pin->GetUnit() == 0 || pin->GetUnit() == m_unitFilter;
            const bool includedByBodyStyle = m_bodyStyleFilter == -1 || pin->GetBodyStyle() == m_bodyStyleFilter;
            const bool includedBySelection = !m_filterBySelection || pinIsInEditorSelection( pin );

            if( includedByUnit && includedByBodyStyle && includedBySelection )
            {
                int rowIndex = -1;

                if( groupByName )
                    rowIndex = findRow( m_rows, pin->GetName() );
                else if( groupBySelection && ( pin->GetFlags() & CANDIDATE ) )
                    rowIndex = 0;

                if( rowIndex < 0 )
                {
                    m_rows.emplace_back( std::vector<SCH_PIN*>() );
                    rowIndex = m_rows.size() - 1;
                }

                m_rows[ rowIndex ].push_back( pin );
            }
        }

        int sortCol = 0;
        bool ascending = true;

        if( GetView() && GetView()->GetSortingColumn() != wxNOT_FOUND )
        {
            sortCol = GetView()->GetSortingColumn();
            ascending = GetView()->IsSortOrderAscending();
        }

        for( std::vector<SCH_PIN*>& row : m_rows )
            SortPins( row );

        if( !groupBySelection )
            SortRows( sortCol, ascending );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int) m_rows.size() );
            GetView()->ProcessTableMessage( msg );

            if( groupBySelection )
                GetView()->SelectRow( 0 );
        }

        for( SCH_PIN* pin : clear_flags )
            pin->ClearFlags( CANDIDATE );
    }

    void SortRows( int aSortCol, bool ascending )
    {
        std::sort( m_rows.begin(), m_rows.end(),
                   [ aSortCol, ascending, this ]( const std::vector<SCH_PIN*>& lhs,
                                                  const std::vector<SCH_PIN*>& rhs ) -> bool
                   {
                       return compare( lhs, rhs, aSortCol, ascending, m_frame );
                   } );
    }

    void SortPins( std::vector<SCH_PIN*>& aRow )
    {
        std::sort( aRow.begin(), aRow.end(),
                   []( SCH_PIN* lhs, SCH_PIN* rhs ) -> bool
                   {
                       return PIN_NUMBERS::Compare( lhs->GetNumber(), rhs->GetNumber() ) < 0;
                   } );
    }

    void AppendRow( SCH_PIN* aPin )
    {
        std::vector<SCH_PIN*> row;
        row.push_back( aPin );
        m_rows.push_back( row );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            GetView()->ProcessTableMessage( msg );
        }
    }

    std::vector<SCH_PIN*> RemoveRow( int aRow )
    {
        std::vector<SCH_PIN*> removedRow = m_rows[ aRow ];

        m_rows.erase( m_rows.begin() + aRow );

        if( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
            GetView()->ProcessTableMessage( msg );
        }

        return removedRow;
    }

    std::vector<SCH_PIN*> GetRowPins( int aRow )
    {
        return m_rows[ aRow ];
    }

    bool IsEdited()
    {
        return m_edited;
    }

private:
    SYMBOL_EDIT_FRAME*                 m_frame;

    // Because the rows of the grid can either be a single pin or a group of pins, the
    // data model is a 2D vector.  If we're in the single pin case, each row's SCH_PINs
    // contains only a single pin.
    std::vector<std::vector<SCH_PIN*>> m_rows;
    int                                m_unitFilter;      // -1 to show pins for all units
    int                                m_bodyStyleFilter; // -1 to show all body styles
    bool                               m_filterBySelection;

    bool                               m_edited;

    DIALOG_LIB_EDIT_PIN_TABLE*         m_pinTable;
    LIB_SYMBOL*                        m_symbol;    // Parent symbol that the pins belong to.

    /// The pins in the symbol that are selected at dialog start
    const std::vector<SCH_PIN*>&       m_origSelectedPins;

    std::unique_ptr<NUMERIC_EVALUATOR> m_eval;
    std::map< std::pair<std::vector<SCH_PIN*>, int>, wxString > m_evalOriginal;
};


class PIN_TABLE_EXPORT
{
public:
    PIN_TABLE_EXPORT( UNITS_PROVIDER& aUnitsProvider ) :
            m_unitsProvider( aUnitsProvider )
    {
    }

    void ExportData( std::vector<SCH_PIN*>& aPins, const wxString& aToFile ) const
    {
        std::vector<int> exportCols {
            COL_NUMBER,
            COL_NAME,
            COL_TYPE,
            COL_SHAPE,
            COL_ORIENTATION,
            COL_NUMBER_SIZE,
            COL_NAME_SIZE,
            COL_LENGTH,
            COL_POSX,
            COL_POSY,
            COL_VISIBLE,
            COL_UNIT,
            COL_BODY_STYLE,
        };

        std::vector<std::vector<wxString>> exportTable;
        exportTable.reserve( aPins.size() + 1 );

        std::vector<wxString> headers;
        for( int col : exportCols )
        {
            headers.push_back( wxGetTranslation( GetPinTableColLabel( col ) ) );
        }
        exportTable.emplace_back( std::move( headers ) );

        NULL_REPORTER      reporter;
        PIN_INFO_FORMATTER formatter( m_unitsProvider, false, PIN_INFO_FORMATTER::BOOL_FORMAT::TRUE_FALSE, reporter );

        for( const SCH_PIN* pin : aPins )
        {
            std::vector<wxString>& cols = exportTable.emplace_back( 0 );
            cols.reserve( exportCols.size() );
            for( int col : exportCols )
            {
                cols.emplace_back( formatter.Format( *pin, col ) );
            }
        }

        if( !aToFile.IsEmpty() )
        {
            wxFileOutputStream os( aToFile );
            CSV_WRITER         writer( os );
            writer.WriteLines( exportTable );
        }
        else
        {
            SaveTabularDataToClipboard( exportTable );
        }
    }

private:
    UNITS_PROVIDER& m_unitsProvider;
};


class PIN_TABLE_IMPORT
{
public:
    PIN_TABLE_IMPORT( EDA_BASE_FRAME& aFrame, REPORTER& aReporter ) :
            m_frame( aFrame ),
            m_reporter( aReporter )
    {
    }

    std::vector<std::unique_ptr<SCH_PIN>> ImportData( bool aFromFile, LIB_SYMBOL& aSym ) const
    {
        wxString path;

        if( aFromFile )
        {
            path = promptForFile();
            if( path.IsEmpty() )
                return {};
        }

        std::vector<std::vector<wxString>> csvData;
        bool                               ok = false;

        if( !path.IsEmpty() )
        {
            // Read file content
            wxString csvFileContent = SafeReadFile( path, "r" );
            ok = AutoDecodeCSV( csvFileContent, csvData );
        }
        else
        {
            ok = GetTabularDataFromClipboard( csvData );
        }

        std::vector<std::unique_ptr<SCH_PIN>> pins;

        PIN_INFO_FORMATTER formatter( m_frame, false, PIN_INFO_FORMATTER::BOOL_FORMAT::TRUE_FALSE, m_reporter );

        if( ok )
        {
            // The first thing we need to do is map the CSV columns to the pin table columns
            // (in case the user reorders them)
            std::vector<COL_ORDER> headerCols = getColOrderFromCSV( csvData[0] );

            for( size_t i = 1; i < csvData.size(); ++i )
            {
                std::vector<wxString>& cols = csvData[i];

                auto pin = std::make_unique<SCH_PIN>( &aSym );

                // Ignore cells that stick out to the right of the headers
                size_t maxCol = std::min( headerCols.size(), cols.size() );

                for( size_t j = 0; j < maxCol; ++j )
                {
                    // Skip unrecognised columns
                    if( headerCols[j] == COL_COUNT )
                        continue;

                    formatter.UpdatePin( *pin, cols[j], headerCols[j], aSym );
                }

                pins.emplace_back( std::move( pin ) );
            }
        }
        return pins;
    }

private:
    wxString promptForFile() const
    {
        wxFileDialog dlg( &m_frame, _( "Select pin data file" ), "", "", FILEEXT::CsvTsvFileWildcard(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return wxEmptyString;

        return dlg.GetPath();
    }

    std::vector<COL_ORDER> getColOrderFromCSV( const std::vector<wxString>& aHeaderRow ) const
    {
        std::vector<COL_ORDER> colOrder;
        wxArrayString          unknownHeaders;

        for( size_t i = 0; i < aHeaderRow.size(); ++i )
        {
            COL_ORDER col = GetColTypeForString( aHeaderRow[i] );

            if( col >= COL_COUNT )
                unknownHeaders.push_back( aHeaderRow[i] );

            colOrder.push_back( col );
        }

        if( unknownHeaders.size() )
        {
            wxString msg = wxString::Format( _( "Unknown columns in data: %s. These columns will be ignored." ),
                                             AccumulateDescriptions( unknownHeaders ) );
            m_reporter.Report( msg, RPT_SEVERITY_ERROR );
        }

        return colOrder;
    }

    EDA_BASE_FRAME& m_frame;
    REPORTER&       m_reporter;
};


DIALOG_LIB_EDIT_PIN_TABLE::DIALOG_LIB_EDIT_PIN_TABLE( SYMBOL_EDIT_FRAME* parent, LIB_SYMBOL* aSymbol,
                                                      const std::vector<SCH_PIN*>& aSelectedPins ) :
        DIALOG_LIB_EDIT_PIN_TABLE_BASE( parent ),
        m_editFrame( parent ),
        m_symbol( aSymbol )
{
    m_dataModel = new PIN_TABLE_DATA_MODEL( m_editFrame, this, this->m_symbol, aSelectedPins );

    // Save original columns widths so we can do proportional sizing.
    for( int i = 0; i < COL_COUNT; ++i )
        m_originalColWidths[ i ] = m_grid->GetColSize( i );

    m_grid->SetTable( m_dataModel );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid, [this]( wxCommandEvent& aEvent )
                                                       {
                                                           OnAddRow( aEvent );
                                                       } ) );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_grid->ShowHideColumns( "0 1 2 3 4 5 9 10" );
    m_columnsShown = m_grid->GetShownColumns();

    // Set special attributes
    wxGridCellAttr* attr;

    attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_grid->SetColAttr( COL_PIN_COUNT, attr );

    attr = new wxGridCellAttr;
    wxArrayString typeNames = PinTypeNames();
    typeNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinTypeIcons(), typeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinTypeIcons(), typeNames ) );
    m_grid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString shapeNames = PinShapeNames();
    shapeNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinShapeIcons(), shapeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinShapeIcons(), shapeNames ) );
    m_grid->SetColAttr( COL_SHAPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString orientationNames = PinOrientationNames();
    orientationNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinOrientationIcons(), orientationNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinOrientationIcons(), orientationNames ) );
    m_grid->SetColAttr( COL_ORIENTATION, attr );

    attr = new wxGridCellAttr;
    wxArrayString unitNames;
    unitNames.push_back( wxGetTranslation( UNITS_ALL ) );

    for( int i = 1; i <= aSymbol->GetUnitCount(); i++ )
        unitNames.push_back( LIB_SYMBOL::LetterSubReference( i, 'A' ) );

    attr->SetEditor( new GRID_CELL_COMBOBOX( unitNames ) );
    m_grid->SetColAttr( COL_UNIT, attr );

    attr = new wxGridCellAttr;
    wxArrayString bodyStyleNames;
    bodyStyleNames.push_back( wxGetTranslation( DEMORGAN_ALL ) );

    if( aSymbol->HasDeMorganBodyStyles() )
    {
        bodyStyleNames.push_back( wxGetTranslation( DEMORGAN_STD ) );
        bodyStyleNames.push_back( wxGetTranslation( DEMORGAN_ALT ) );
    }
    else
    {
        for( const wxString& body_style_name : aSymbol->GetBodyStyleNames() )
            bodyStyleNames.push_back( body_style_name );
    }

    attr->SetEditor( new GRID_CELL_COMBOBOX( bodyStyleNames ) );
    m_grid->SetColAttr( COL_BODY_STYLE, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetEditor( new wxGridCellBoolEditor() );
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_grid->SetColAttr( COL_VISIBLE, attr );

    /* Right-aligned position values look much better, but only MSW and GTK2+
     * currently support right-aligned textEditCtrls, so the text jumps on all
     * the other platforms when you edit it.
    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSX, attr );

    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSY, attr );
    */

    m_addButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_refreshButton->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );

    m_divider1->SetIsSeparator();

    GetSizer()->SetSizeHints(this);
    Centre();

    if( aSymbol->IsMultiUnit() )
    {
        m_unitFilter->Append( wxGetTranslation( UNITS_ALL ) );

        for( int ii = 0; ii < aSymbol->GetUnitCount(); ++ii )
            m_unitFilter->Append( LIB_SYMBOL::LetterSubReference( ii + 1, 'A' ) );

        m_unitFilter->SetSelection( -1 );
    }
    else
    {
        m_cbFilterByUnit->Enable( false );
        m_unitFilter->Enable( false );
    }

    if( aSymbol->HasDeMorganBodyStyles() )
    {
        m_bodyStyleFilter->Append( wxGetTranslation( DEMORGAN_ALL ) );
        m_bodyStyleFilter->Append( wxGetTranslation( DEMORGAN_STD ) );
        m_bodyStyleFilter->Append( wxGetTranslation( DEMORGAN_ALT ) );

        m_bodyStyleFilter->SetSelection( -1 );
    }
    else if( aSymbol->IsMultiBodyStyle() )
    {
        m_bodyStyleFilter->Append( wxGetTranslation( DEMORGAN_ALL ) );

        for( const wxString& bodyStyle : aSymbol->GetBodyStyleNames() )
            m_bodyStyleFilter->Append( bodyStyle );

        m_bodyStyleFilter->SetSelection( -1 );
    }
    else
    {
        m_cbFilterByBodyStyle->Enable( false );
        m_bodyStyleFilter->Enable( false );
    }

    SetupStandardButtons();

    if( !parent->IsSymbolEditable() || parent->IsSymbolAlias() )
    {
        m_ButtonsCancel->SetDefault();
        m_ButtonsOK->SetLabel( _( "Read Only" ) );
        m_ButtonsOK->Enable( false );
    }

    m_initialized = true;
    m_modified = false;

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr,
                     this );
}


DIALOG_LIB_EDIT_PIN_TABLE::~DIALOG_LIB_EDIT_PIN_TABLE()
{
    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr,
                        this );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_dataModel );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // This is our copy of the pins.  If they were transferred to the part on an OK, then
    // m_pins will already be empty.
    for( SCH_PIN* pin : m_pins )
        delete pin;

    WINDOW_THAWER thawer( m_editFrame );

    m_editFrame->ClearFocus();
    m_editFrame->GetCanvas()->Refresh();
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataToWindow()
{
    // Make a copy of the pins for editing
    std::vector<SCH_PIN*> pins = m_symbol->GetPins();

    for( SCH_PIN* pin : pins )
        m_pins.push_back( new SCH_PIN( *pin ) );

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue(), false );

    updateSummary();

    return true;
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Delete the part's pins
    std::vector<SCH_PIN*> pins = m_symbol->GetPins();

    for( SCH_PIN* pin : pins )
        m_symbol->RemoveDrawItem( pin );

    // Transfer our pins to the part
    for( SCH_PIN* pin : m_pins )
        m_symbol->AddDrawItem( pin );

    m_pins.clear();

    return true;
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnColSort( wxGridEvent& aEvent )
{
    int sortCol = aEvent.GetCol();
    bool ascending;

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the
    // event, and if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    else
        // different column; start with ascending
        ascending = true;

    m_dataModel->SortRows( sortCol, ascending );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnAddRow( wxCommandEvent& event )
{
    m_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                SCH_PIN* newPin = new SCH_PIN( this->m_symbol );

                // Copy the settings of the last pin onto the new pin.
                if( m_pins.size() > 0 )
                {
                    SCH_PIN* last = m_pins.back();

                    newPin->SetOrientation( last->GetOrientation() );
                    newPin->SetType( last->GetType() );
                    newPin->SetShape( last->GetShape() );
                    newPin->SetUnit( last->GetUnit() );
                    newPin->SetBodyStyle( last->GetBodyStyle() );

                    VECTOR2I pos = last->GetPosition();

                    SYMBOL_EDITOR_SETTINGS* cfg = m_editFrame->GetSettings();

                    if( last->GetOrientation() == PIN_ORIENTATION::PIN_LEFT
                        || last->GetOrientation() == PIN_ORIENTATION::PIN_RIGHT )
                    {
                        pos.y -= schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
                    }
                    else
                    {
                        pos.x += schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
                    }

                    newPin->SetPosition( pos );
                }

                m_pins.push_back( newPin );

                m_dataModel->AppendRow( m_pins[ m_pins.size() - 1 ] );
                updateSummary();

                return { m_dataModel->GetNumberRows() - 1, COL_NUMBER };
            } );
}


void DIALOG_LIB_EDIT_PIN_TABLE::AddPin( SCH_PIN* pin )
{
    m_pins.push_back( pin );
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnDeleteRow( wxCommandEvent& event )
{
    m_grid->OnDeleteRows(
            [&]( int row )
            {
                std::vector<SCH_PIN*> removedRow = m_dataModel->RemoveRow( row );

                for( SCH_PIN* pin : removedRow )
                    m_pins.erase( std::find( m_pins.begin(), m_pins.end(), pin ) );
            } );

    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::RemovePin( SCH_PIN* pin )
{
    m_pins.erase( std::find( m_pins.begin(), m_pins.end(), pin ) );
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCellEdited( wxGridEvent& event )
{
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCellSelected( wxGridEvent& event )
{
    SCH_PIN* pin = nullptr;

    if( event.GetRow() >= 0 && event.GetRow() < m_dataModel->GetNumberRows() )
    {
        const std::vector<SCH_PIN*>& pins = m_dataModel->GetRowPins( event.GetRow() );

        if( pins.size() == 1 && m_editFrame->GetCurSymbol() )
        {
            for( SCH_PIN* candidate : m_editFrame->GetCurSymbol()->GetPins() )
            {
                if( candidate->GetNumber() == pins.at( 0 )->GetNumber() )
                {
                    pin = candidate;
                    break;
                }
            }
        }
    }

    WINDOW_THAWER thawer( m_editFrame );

    m_editFrame->FocusOnItem( pin );
    m_editFrame->GetCanvas()->Refresh();
}


bool DIALOG_LIB_EDIT_PIN_TABLE::IsDisplayGrouped()
{
    return m_cbGroup->GetValue();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnGroupSelected( wxCommandEvent& event )
{
    m_cbGroup->SetValue( false );

    m_dataModel->RebuildRows( m_pins, false, true );

    m_grid->ShowCol( COL_PIN_COUNT );
    m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    adjustGridColumns();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnRebuildRows( wxCommandEvent&  )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue(), false );

    if( m_cbGroup->GetValue() )
    {
        m_grid->ShowCol( COL_PIN_COUNT );
        m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    }

    adjustGridColumns();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnFilterCheckBox( wxCommandEvent& event )
{
    if( event.GetEventObject() == m_cbFilterByUnit )
    {
        if( event.IsChecked() )
        {
            if( m_unitFilter->GetSelection() == -1 )
                m_unitFilter->SetSelection( 0 );

            m_dataModel->SetUnitFilter( m_unitFilter->GetSelection() );
        }
        else
        {
            m_dataModel->SetUnitFilter( -1 );
            m_unitFilter->SetSelection( -1 );
        }
    }
    else if( event.GetEventObject() == m_cbFilterByBodyStyle )
    {
        if( event.IsChecked() )
        {
            if( m_bodyStyleFilter->GetSelection() == -1 )
                m_bodyStyleFilter->SetSelection( 0 );

            m_dataModel->SetBodyStyleFilter( m_bodyStyleFilter->GetSelection() );
        }
        else
        {
            m_dataModel->SetBodyStyleFilter( -1 );
            m_bodyStyleFilter->SetSelection( -1 );
        }
    }
    else if( event.GetEventObject() == m_cbFilterSelected )
    {
        m_dataModel->SetFilterBySelection( event.IsChecked() );
    }

    OnRebuildRows( event );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnFilterChoice( wxCommandEvent& event )
{
    if( event.GetEventObject() == m_unitFilter )
    {
        m_cbFilterByUnit->SetValue( true );
        m_dataModel->SetUnitFilter( m_unitFilter->GetSelection() );
    }
    else if( event.GetEventObject() == m_bodyStyleFilter )
    {
        m_cbFilterByBodyStyle->SetValue( true );
        m_dataModel->SetBodyStyleFilter( m_bodyStyleFilter->GetSelection() );
    }

    OnRebuildRows( event );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnImportButtonClick( wxCommandEvent& event )
{
    bool fromFile = event.GetEventObject() == m_btnImportFromFile;
    bool replaceAll = m_rbReplaceAll->GetValue();

    WX_STRING_REPORTER reporter;
    PIN_TABLE_IMPORT   importer( *m_editFrame, reporter );

    std::vector<std::unique_ptr<SCH_PIN>> newPins = importer.ImportData( fromFile, *m_symbol );

    if( reporter.HasMessage() )
    {
        int ret = wxMessageBox( reporter.GetMessages(), _( "Errors" ), wxOK | wxCANCEL | wxICON_ERROR, this );

        // Give the user the option to cancel the import on errors.
        if( ret == wxCANCEL )
            return;
    }

    if( !newPins.size() )
        return;

    if( replaceAll )
    {
        // This is quite a dance with a segfault without smart pointers
        for( SCH_PIN* pin : m_pins )
            delete pin;

        m_pins.clear();
    }

    for( auto& newPin : newPins )
        m_pins.push_back( newPin.release() );

    m_cbGroup->SetValue( false );
    m_dataModel->RebuildRows( m_pins, false, false );

    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnExportButtonClick( wxCommandEvent& event )
{
    bool toFile = event.GetEventObject() == m_btnExportToFile;
    bool onlyShown = m_rbExportOnlyShownPins->GetValue();

    wxString filePath = wxEmptyString;

    if( toFile )
    {
        wxFileName fn( m_symbol->GetName() );
        fn.SetExt( FILEEXT::CsvFileExtension );
        wxFileDialog dlg( this, _( "Select pin data file" ), "", fn.GetFullName(), FILEEXT::CsvFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        filePath = dlg.GetPath();
    }

    std::vector<SCH_PIN*> pinsToExport;

    if( onlyShown )
    {
        for( int i = 0; i < m_dataModel->GetNumberRows(); ++i )
        {
            for( SCH_PIN* pin : m_dataModel->GetRowPins( i ) )
                pinsToExport.push_back( pin );
        }
    }
    else
    {
        pinsToExport = m_pins;
    }

    PIN_TABLE_EXPORT exporter( *m_editFrame );
    exporter.ExportData( pinsToExport, filePath );
}


void DIALOG_LIB_EDIT_PIN_TABLE::adjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_grid ).x;

    wxGridUpdateLocker deferRepaintsTillLeavingScope;

    // The Number and Name columns must be at least wide enough to hold their contents, but
    // no less wide than their original widths.
    m_grid->AutoSizeColumn( COL_NUMBER );

    if( m_grid->GetColSize( COL_NUMBER ) < m_originalColWidths[ COL_NUMBER ] )
        m_grid->SetColSize( COL_NUMBER, m_originalColWidths[ COL_NUMBER ] );

    m_grid->AutoSizeColumn( COL_NAME );

    if( m_grid->GetColSize( COL_NAME ) < m_originalColWidths[ COL_NAME ] )
        m_grid->SetColSize( COL_NAME, m_originalColWidths[ COL_NAME ] );

    // If the grid is still wider than the columns, then stretch the Number and Name columns
    // to fit.
    for( int i = 0; i < COL_COUNT; ++i )
        width -= m_grid->GetColSize( i );

    if( width > 0 )
    {
        m_grid->SetColSize( COL_NUMBER, m_grid->GetColSize( COL_NUMBER ) + width / 2 );
        m_grid->SetColSize( COL_NAME, m_grid->GetColSize( COL_NAME ) + width / 2 );
    }
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnSize( wxSizeEvent& event )
{
    wxSize new_size = event.GetSize();

    if( m_initialized && m_size != new_size )
    {
        m_size = new_size;

        adjustGridColumns();
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnUpdateUI( wxUpdateUIEvent& event )
{
    std::bitset<64> columnsShown = m_grid->GetShownColumns();

    if( columnsShown != m_columnsShown )
    {
        m_columnsShown = columnsShown;

        if( !m_grid->IsCellEditControlShown() )
            adjustGridColumns();
    }

    int firstSelectedRow;
    int selectedRowCount;

    getSelectedArea( m_grid, &firstSelectedRow, &selectedRowCount );

    if( ( selectedRowCount > 1 ) != m_groupSelected->IsEnabled() )
        m_groupSelected->Enable( selectedRowCount > 1 );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCancel( wxCommandEvent& event )
{
    Close();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnClose( wxCloseEvent& event )
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
    m_grid->CommitPendingChanges( true );

    int retval = wxID_CANCEL;

    if( m_dataModel->IsEdited() )
    {
        if( HandleUnsavedChanges( this, _( "Save changes?" ),
                                  [&]() -> bool
                                  {
                                      if( TransferDataFromWindow() )
                                      {
                                          retval = wxID_OK;
                                          return true;
                                      }

                                      return false;
                                  } ) )
        {
            if( IsQuasiModal() )
                EndQuasiModal( retval );
            else
                EndDialog( retval );

            return;
        }
        else
        {
            event.Veto();
            return;
        }
    }

    // No change in dialog: we can close it
    if( IsQuasiModal() )
        EndQuasiModal( retval );
    else
        EndDialog( retval );
}


void DIALOG_LIB_EDIT_PIN_TABLE::updateSummary()
{
    PIN_NUMBERS pinNumbers;

    for( SCH_PIN* pin : m_pins )
    {
        if( pin->GetNumber().Length() )
            pinNumbers.insert( pin->GetNumber() );
    }

    m_pin_numbers_summary->SetLabel( pinNumbers.GetSummary() );
    m_pin_count->SetLabel( wxString::Format( wxT( "%u" ), (unsigned) m_pins.size() ) );
    m_duplicate_pins->SetLabel( pinNumbers.GetDuplicates() );

    Layout();
}
