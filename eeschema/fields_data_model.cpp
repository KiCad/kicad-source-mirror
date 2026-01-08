/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 <author>
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
#include <wx/string.h>
#include <wx/debug.h>
#include <wx/grid.h>
#include <wx/settings.h>
#include <common.h>
#include <widgets/wx_grid.h>
#include <sch_reference_list.h>
#include <sch_commit.h>
#include <sch_screen.h>
#include "string_utils.h"

#include "fields_data_model.h"


/**
 * Create a unique key for the data store by combining the #KIID_PATH from the
 * #SCH_SHEET_PATH with the symbol's UUID.
 *
 * @param aSheetPath The sheet path containing the symbol
 * @param aSymbol The symbol to create a key for
 * @return A KIID_PATH representing the full #SCH_SHEET_PATH + symbol UUID.
 */
static KIID_PATH makeDataStoreKey( const SCH_SHEET_PATH& aSheetPath, const SCH_SYMBOL& aSymbol )
{
    KIID_PATH path = aSheetPath.Path();
    path.push_back( aSymbol.m_Uuid );
    return path;
}


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN: return _( "Field" );
    case LABEL_COLUMN:        return m_forBOM ? _( "BOM Name" ) : _( "Label" );
    case SHOW_FIELD_COLUMN:   return _( "Include" );
    case GROUP_BY_COLUMN:     return _( "Group By" );
    default:                  return wxT( "unknown column" );
    };
}


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), wxT( "bad row!" ) );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN:
        for( FIELD_T fieldId : MANDATORY_FIELDS )
        {
            if( GetDefaultFieldName( fieldId, !DO_TRANSLATE ) == rowData.name )
                return GetDefaultFieldName( fieldId, DO_TRANSLATE );
        }

        return rowData.name;

    case LABEL_COLUMN:
        return rowData.label;

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


bool VIEW_CONTROLS_GRID_DATA_MODEL::GetValueAsBool( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), false );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case SHOW_FIELD_COLUMN: return rowData.show;
    case GROUP_BY_COLUMN:   return rowData.groupBy;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString &aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN:
        // Not editable
        break;

    case LABEL_COLUMN:
        rowData.label = aValue;
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
    }

    GetView()->Refresh();
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case SHOW_FIELD_COLUMN: rowData.show = aValue;    break;
    case GROUP_BY_COLUMN:   rowData.groupBy = aValue; break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::AppendRow( const wxString& aFieldName, const wxString& aBOMName,
                                               bool aShow, bool aGroupBy )
{
    m_fields.emplace_back( BOM_FIELD{ aFieldName, aBOMName, aShow, aGroupBy } );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
        grid->ProcessTableMessage( msg );
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::DeleteRow( int aRow )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), /* void */ );

    m_fields.erase( m_fields.begin() + aRow );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
        grid->ProcessTableMessage( msg );
    }
}


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetCanonicalFieldName( int aRow )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), wxEmptyString );

    BOM_FIELD& rowData = m_fields[aRow];

    return rowData.name;
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetCanonicalFieldName( int aRow, const wxString& aName )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), /* void */ );

    BOM_FIELD& rowData = m_fields[aRow];

    rowData.name = aName;
}


const wxString FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE = wxS( "${QUANTITY}" );
const wxString FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE = wxS( "${ITEM_NUMBER}" );


void FIELDS_EDITOR_GRID_DATA_MODEL::AddColumn( const wxString& aFieldName, const wxString& aLabel,
                                               bool aAddedByUser, const wxString& aVariantName )
{
    // Don't add a field twice
    if( GetFieldNameCol( aFieldName ) != -1 )
        return;

    m_cols.push_back( { aFieldName, aLabel, aAddedByUser, false, false } );

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
        updateDataStoreSymbolField( m_symbolsList[i], aFieldName, aVariantName );
}


void FIELDS_EDITOR_GRID_DATA_MODEL::updateDataStoreSymbolField( const SCH_REFERENCE& aSymbolRef,
                                                                const wxString& aFieldName,
                                                                const wxString& aVariantName )
{
    const SCH_SYMBOL* symbol = aSymbolRef.GetSymbol();

    if( !symbol )
        return;

    KIID_PATH key = makeDataStoreKey( aSymbolRef.GetSheetPath(), *symbol );

    if( isAttribute( aFieldName ) )
    {
        m_dataStore[key][aFieldName] = getAttributeValue( aSymbolRef, aFieldName, aVariantName );
    }
    else if( const SCH_FIELD* field = symbol->GetField( aFieldName ) )
    {
        if( field->IsPrivate() )
        {
            m_dataStore[key][aFieldName] = wxEmptyString;
            return;
        }

        wxString value = symbol->Schematic()->ConvertKIIDsToRefs( field->GetText( &aSymbolRef.GetSheetPath(),
                                                                                  aVariantName ) );

        m_dataStore[key][aFieldName] = value;
    }
    else if( IsGeneratedField( aFieldName ) )
    {
        // Handle generated fields with variables as names (e.g. ${QUANTITY}) that are not present in
        // the symbol by giving them the correct value
        m_dataStore[key][aFieldName] = aFieldName;
    }
    else
    {
        m_dataStore[key][aFieldName] = wxEmptyString;
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RemoveColumn( int aCol )
{
    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        if( SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol() )
        {
            KIID_PATH key = makeDataStoreKey( m_symbolsList[i].GetSheetPath(), *symbol );
            m_dataStore[key].erase( m_cols[aCol].m_fieldName );
        }
    }

    m_cols.erase( m_cols.begin() + aCol );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_COLS_DELETED, aCol, 1 );
        grid->ProcessTableMessage( msg );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RenameColumn( int aCol, const wxString& newName )
{
    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol();
        KIID_PATH key = makeDataStoreKey( m_symbolsList[i].GetSheetPath(), *symbol );

        // Careful; field may have already been renamed from another sheet instance
        if( auto node = m_dataStore[key].extract( m_cols[aCol].m_fieldName ) )
        {
            node.key() = newName;
            m_dataStore[key].insert( std::move( node ) );
        }
    }

    m_cols[aCol].m_fieldName = newName;
    m_cols[aCol].m_label = newName;
}


int FIELDS_EDITOR_GRID_DATA_MODEL::GetFieldNameCol( const wxString& aFieldName ) const
{
    for( size_t i = 0; i < m_cols.size(); i++ )
    {
        if( m_cols[i].m_fieldName == aFieldName )
            return static_cast<int>( i );
    }

    return -1;
}


std::vector<BOM_FIELD> FIELDS_EDITOR_GRID_DATA_MODEL::GetFieldsOrdered()
{
    std::vector<BOM_FIELD> fields;

    for( const DATA_MODEL_COL& col : m_cols )
        fields.push_back( { col.m_fieldName, col.m_label, col.m_show, col.m_group } );

    return fields;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::SetFieldsOrder( const std::vector<wxString>& aNewOrder )
{
    size_t foundCount = 0;

    for( const wxString& newField : aNewOrder )
    {
        if( foundCount >= m_cols.size() )
            break;

        for( DATA_MODEL_COL& col : m_cols )
        {
            if( col.m_fieldName == newField )
            {
                std::swap( m_cols[foundCount], col );
                foundCount++;
                break;
            }
        }
    }
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::IsExpanderColumn( int aCol ) const
{
    // Check if aCol is the first visible column
    for( int col = 0; col < aCol; ++col )
    {
        if( m_cols[col].m_show )
            return false;
    }

    return true;
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    GetView()->SetReadOnly( aRow, aCol, IsExpanderColumn( aCol ) );
    return GetValue( m_rows[aRow], aCol );
}


wxGridCellAttr* FIELDS_EDITOR_GRID_DATA_MODEL::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    wxGridCellAttr* attr = nullptr;

    if( GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::DATASHEET )
            || IsURL( GetValue( m_rows[aRow], aCol ) ) )
    {
        if( m_urlEditor )
        {
            m_urlEditor->IncRef();
            attr = m_urlEditor;
        }
    }

    // Highlight cells that differ from default when viewing a variant
    if( !m_currentVariant.IsEmpty() && aRow >= 0 && aRow < (int) m_rows.size()
            && aCol >= 0 && aCol < (int) m_cols.size() )
    {
        const wxString& fieldName = m_cols[aCol].m_fieldName;

        // Skip Reference and generated fields (like ${QUANTITY}) for highlighting
        if( !ColIsReference( aCol ) && !ColIsQuantity( aCol ) && !ColIsItemNumber( aCol ) )
        {
            const DATA_MODEL_ROW& row = m_rows[aRow];

            // Check if any symbol in this row has a variant-specific value
            bool hasVariantDifference = false;

            for( const SCH_REFERENCE& ref : row.m_Refs )
            {
                wxString defaultValue = getDefaultFieldValue( ref, fieldName );

                KIID_PATH symbolKey = KIID_PATH();

                if( const SCH_SYMBOL* symbol = ref.GetSymbol() )
                {
                    symbolKey = ref.GetSheetPath().Path();
                    symbolKey.push_back( symbol->m_Uuid );
                }

                // Get the current value from the data store
                wxString currentValue;

                if( m_dataStore.contains( symbolKey ) && m_dataStore[symbolKey].contains( fieldName ) )
                    currentValue = m_dataStore[symbolKey][fieldName];

                if( currentValue != defaultValue )
                {
                    hasVariantDifference = true;
                    break;
                }
            }

            if( hasVariantDifference )
            {
                if( !attr )
                {
                    attr = new wxGridCellAttr();
                }
                else
                {
                    // Clone so we don't modify the shared URL editor attr
                    wxGridCellAttr* newAttr = attr->Clone();
                    attr->DecRef();
                    attr = newAttr;
                }

                // Use a subtle highlight color that works in both light and dark themes
                // Light yellow in light mode, darker yellow-ish in dark mode
                wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
                bool isDark = ( bg.Red() + bg.Green() + bg.Blue() ) < 384;

                if( isDark )
                    attr->SetBackgroundColour( wxColour( 80, 80, 40 ) );  // Dark gold/brown
                else
                    attr->SetBackgroundColour( wxColour( 255, 255, 200 ) );  // Light yellow
            }
        }
    }

    if( attr )
        return enhanceAttr( attr, aRow, aCol, aKind );

    return WX_GRID_TABLE_BASE::GetAttr( aRow, aCol, aKind );
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::GetValue( const DATA_MODEL_ROW& group, int aCol,
                                                  const wxString& refDelimiter,
                                                  const wxString& refRangeDelimiter,
                                                  bool            resolveVars,
                                                  bool            listMixedValues )
{
    std::vector<SCH_REFERENCE> references;
    std::set<wxString>         mixedValues;
    wxString                   fieldValue;

    for( const SCH_REFERENCE& ref : group.m_Refs )
    {
        if( ColIsReference( aCol ) || ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
        {
            references.push_back( ref );
        }
        else // Other columns are either a single value or ROW_MULTI_ITEMS
        {
            KIID_PATH symbolKey = makeDataStoreKey( ref.GetSheetPath(), *ref.GetSymbol() );

            if( !m_dataStore.contains( symbolKey ) || !m_dataStore[symbolKey].contains( m_cols[aCol].m_fieldName ) )
                return INDETERMINATE_STATE;

            wxString refFieldValue = m_dataStore[symbolKey][m_cols[aCol].m_fieldName];

            if( resolveVars )
            {
                if( IsGeneratedField( m_cols[aCol].m_fieldName ) )
                {
                    // Generated fields (e.g. ${QUANTITY}) can't have un-applied values as they're
                    // read-only.  Resolve them against the field.
                    refFieldValue = getFieldShownText( ref, m_cols[aCol].m_fieldName );
                }
                else if( refFieldValue.Contains( wxT( "${" ) ) )
                {
                    // Resolve variables in the un-applied value using the parent symbol and instance
                    // data.
                    std::function<bool( wxString* )> symbolResolver =
                            [&]( wxString* token ) -> bool
                            {
                                return ref.GetSymbol()->ResolveTextVar( &ref.GetSheetPath(), token );
                            };

                    refFieldValue = ExpandTextVars( refFieldValue, & symbolResolver );
                }
            }

            if( listMixedValues )
                mixedValues.insert( refFieldValue );
            else if( &ref == &group.m_Refs.front() )
                fieldValue = refFieldValue;
            else if( fieldValue != refFieldValue )
                return INDETERMINATE_STATE;
        }
    }

    if( listMixedValues )
    {
        fieldValue = wxEmptyString;

        for( const wxString& value : mixedValues )
        {
            if( value.IsEmpty() )
                continue;
            else if( fieldValue.IsEmpty() )
                fieldValue = value;
            else
                fieldValue += "," + value;
        }
    }

    if( ColIsReference( aCol ) || ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
    {
        // Remove duplicates (other units of multi-unit parts)
        std::sort( references.begin(), references.end(),
                []( const SCH_REFERENCE& l, const SCH_REFERENCE& r ) -> bool
                {
                    wxString l_ref( l.GetRef() << l.GetRefNumber() );
                    wxString r_ref( r.GetRef() << r.GetRefNumber() );
                    return StrNumCmp( l_ref, r_ref, true ) < 0;
                } );

        auto logicalEnd = std::unique( references.begin(), references.end(),
                []( const SCH_REFERENCE& l, const SCH_REFERENCE& r ) -> bool
                {
                    // If unannotated then we can't tell what units belong together
                    // so we have to leave them all
                    if( l.GetRefNumber() == wxT( "?" ) )
                        return false;

                    wxString l_ref( l.GetRef() << l.GetRefNumber() );
                    wxString r_ref( r.GetRef() << r.GetRefNumber() );
                    return l_ref == r_ref;
                } );

        references.erase( logicalEnd, references.end() );
    }

    if( ColIsReference( aCol ) )
        fieldValue = SCH_REFERENCE_LIST::Shorthand( references, refDelimiter, refRangeDelimiter );
    else if( ColIsQuantity( aCol ) )
        fieldValue = wxString::Format( wxT( "%d" ), (int) references.size() );
    else if( ColIsItemNumber( aCol ) && group.m_Flag != CHILD_ITEM )
        fieldValue = wxString::Format( wxT( "%d" ), group.m_ItemNumber );

    return fieldValue;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxS( "Invalid column number" ) );

    // Can't modify references or generated fields (e.g. ${QUANTITY})
    if( ColIsReference( aCol )
        || ( IsGeneratedField( m_cols[aCol].m_fieldName ) && !ColIsAttribute( aCol ) ) )
    {
        return;
    }

    DATA_MODEL_ROW& rowGroup = m_rows[aRow];

    const SCH_SYMBOL* sharedSymbol = nullptr;
    bool isSharedInstance = false;

    for( const SCH_REFERENCE& ref : rowGroup.m_Refs )
    {
        const SCH_SCREEN* screen = nullptr;

        // Check to see if the symbol associated with this row has more than one instance.
        if( const SCH_SYMBOL* symbol = ref.GetSymbol() )
        {
            screen = static_cast<const SCH_SCREEN*>( symbol->GetParent() );

            isSharedInstance = ( screen && ( screen->GetRefCount() > 1 ) );
            sharedSymbol = symbol;
        }

        KIID_PATH key = makeDataStoreKey( ref.GetSheetPath(), *ref.GetSymbol() );
        m_dataStore[key][m_cols[aCol].m_fieldName] = aValue;
    }

    // Update all of the other instances for the shared symbol as required.
    if( isSharedInstance
      && ( ( rowGroup.m_Flag == GROUP_SINGLETON ) || ( rowGroup.m_Flag == CHILD_ITEM ) ) )
    {
        for( DATA_MODEL_ROW& row : m_rows )
        {
            if( row.m_ItemNumber == aRow + 1 )
                continue;

            for( const SCH_REFERENCE& ref : row.m_Refs )
            {
                if( ref.GetSymbol() != sharedSymbol )
                    continue;

                KIID_PATH key = makeDataStoreKey( ref.GetSheetPath(), *ref.GetSymbol() );
                m_dataStore[key][m_cols[aCol].m_fieldName] = aValue;
            }
        }
    }

    m_edited = true;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsReference( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == GetCanonicalFieldName( FIELD_T::REFERENCE );
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsValue( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == GetCanonicalFieldName( FIELD_T::VALUE );
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsQuantity( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == QUANTITY_VARIABLE;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsItemNumber( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == ITEM_NUMBER_VARIABLE;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsAttribute( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return isAttribute( m_cols[aCol].m_fieldName );
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::cmp( const DATA_MODEL_ROW&          lhGroup,
                                         const DATA_MODEL_ROW&          rhGroup,
                                         FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol,
                                         bool ascending )
{
    // Empty rows always go to the bottom, whether ascending or descending
    if( lhGroup.m_Refs.size() == 0 )
        return true;
    else if( rhGroup.m_Refs.size() == 0 )
        return false;

    // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
    // to get the opposite sort.  i.e. ~(a<b) != (a>b)
    auto local_cmp =
            [ ascending ]( const auto a, const auto b )
            {
                if( ascending )
                    return a < b;
                else
                    return a > b;
            };

    // Primary sort key is sortCol; secondary is always REFERENCE (column 0)
    if( sortCol < 0 || sortCol >= dataModel->GetNumberCols() )
        sortCol = 0;

    wxString lhs = dataModel->GetValue( lhGroup, sortCol ).Trim( true ).Trim( false );
    wxString rhs = dataModel->GetValue( rhGroup, sortCol ).Trim( true ).Trim( false );

    if( lhs == rhs || dataModel->ColIsReference( sortCol ) )
    {
        wxString lhRef = lhGroup.m_Refs[0].GetRef() + lhGroup.m_Refs[0].GetRefNumber();
        wxString rhRef = rhGroup.m_Refs[0].GetRef() + rhGroup.m_Refs[0].GetRefNumber();
        return local_cmp( StrNumCmp( lhRef, rhRef, true ), 0 );
    }
    else
    {
        return local_cmp( ValueStringCompare( lhs, rhs ), 0 );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::Sort()
{
    CollapseForSort();

    // We're going to sort the rows based on their first reference, so the first reference
    // had better be the lowest one.
    for( DATA_MODEL_ROW& row : m_rows )
    {
        std::sort( row.m_Refs.begin(), row.m_Refs.end(),
                   []( const SCH_REFERENCE& lhs, const SCH_REFERENCE& rhs )
                   {
                       wxString lhs_ref( lhs.GetRef() << lhs.GetRefNumber() );
                       wxString rhs_ref( rhs.GetRef() << rhs.GetRefNumber() );
                       return StrNumCmp( lhs_ref, rhs_ref, true ) < 0;
                   } );
    }

    std::sort( m_rows.begin(), m_rows.end(),
               [this]( const DATA_MODEL_ROW& lhs, const DATA_MODEL_ROW& rhs ) -> bool
               {
                   return cmp( lhs, rhs, this, m_sortColumn, m_sortAscending );
               } );

    // Time to renumber the item numbers
    int itemNumber = 1;

    for( DATA_MODEL_ROW& row : m_rows )
    {
        row.m_ItemNumber = itemNumber++;
    }

    ExpandAfterSort();
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef )
{
    // If items are unannotated then we can't tell if they're units of the same symbol or not
    if( lhRef.GetRefNumber() == wxT( "?" ) )
        return false;

    return ( lhRef.GetRef() == rhRef.GetRef() && lhRef.GetRefNumber() == rhRef.GetRefNumber() );
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef )
{
    int  refCol = GetFieldNameCol( GetCanonicalFieldName( FIELD_T::REFERENCE ) );
    bool matchFound = false;

    if( refCol == -1 )
        return false;

    // First check the reference column.  This can be done directly out of the
    // SCH_REFERENCEs as the references can't be edited in the grid.
    if( m_cols[refCol].m_group )
    {
        // if we're grouping by reference, then only the prefix must match
        if( lhRef.GetRef() != rhRef.GetRef() )
            return false;

        matchFound = true;
    }

    KIID_PATH lhRefKey = makeDataStoreKey( lhRef.GetSheetPath(), *lhRef.GetSymbol() );
    KIID_PATH rhRefKey = makeDataStoreKey( rhRef.GetSheetPath(), *rhRef.GetSymbol() );

    // Now check all the other columns.
    for( size_t i = 0; i < m_cols.size(); ++i )
    {
        //Handled already
        if( static_cast<int>( i ) == refCol )
            continue;

        if( !m_cols[i].m_group )
            continue;

        // If the field is generated (e.g. ${QUANTITY}), we need to resolve it through the symbol
        // to get the actual current value; otherwise we need to pull it out of the store so the
        // refresh can regroup based on values that haven't been applied to the schematic yet.
        wxString lh, rh;

        if( IsGeneratedField( m_cols[i].m_fieldName )
            || IsGeneratedField( m_dataStore[lhRefKey][m_cols[i].m_fieldName] ) )
        {
            lh = getFieldShownText( lhRef, m_cols[i].m_fieldName );
        }
        else
        {
            lh = m_dataStore[lhRefKey][m_cols[i].m_fieldName];
        }

        if( IsGeneratedField( m_cols[i].m_fieldName )
            || IsGeneratedField( m_dataStore[rhRefKey][m_cols[i].m_fieldName] ) )
        {
            rh = getFieldShownText( rhRef, m_cols[i].m_fieldName );
        }
        else
        {
            rh = m_dataStore[rhRefKey][m_cols[i].m_fieldName];
        }

        if( lh != rh )
            return false;

        matchFound = true;
    }

    return matchFound;
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::getFieldShownText( const SCH_REFERENCE& aRef,
                                                           const wxString&      aFieldName )
{
    SCH_FIELD* field = aRef.GetSymbol()->GetField( aFieldName );

    if( field )
    {
        if( field->IsPrivate() )
            return wxEmptyString;
        else
            return field->GetShownText( &aRef.GetSheetPath(), false );
    }

    // Handle generated fields with variables as names (e.g. ${QUANTITY}) that are not present in
    // the symbol by giving them the correct value by resolving against the symbol
    if( IsGeneratedField( aFieldName ) )
    {
        int                   depth = 0;
        const SCH_SHEET_PATH& path = aRef.GetSheetPath();

        std::function<bool( wxString* )> symbolResolver =
                [&]( wxString* token ) -> bool
                {
                    return aRef.GetSymbol()->ResolveTextVar( &path, token, depth + 1 );
                };

        return ExpandTextVars( aFieldName, &symbolResolver );
    }

    return wxEmptyString;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::isAttribute( const wxString& aFieldName )
{
    return aFieldName == wxS( "${DNP}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_BOARD}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_BOM}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_SIM}" );
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::getAttributeValue( const SCH_REFERENCE& aRef, const wxString& aAttributeName,
                                                           const wxString& aVariantName )
{
    if( aAttributeName == wxS( "${DNP}" ) )
        return aRef.GetSymbolDNP( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return aRef.GetSymbolExcludedFromBoard() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        return aRef.GetSymbolExcludedFromBOM( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        return aRef.GetSymbolExcludedFromSim( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    return wxS( "0" );
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::getDefaultFieldValue( const SCH_REFERENCE& aRef,
                                                               const wxString& aFieldName )
{
    const SCH_SYMBOL* symbol = aRef.GetSymbol();

    if( !symbol )
        return wxEmptyString;

    // For attributes, get the default (non-variant) value
    if( isAttribute( aFieldName ) )
        return getAttributeValue( aRef, aFieldName, wxEmptyString );

    // For regular fields, get the text without variant override
    if( const SCH_FIELD* field = symbol->GetField( aFieldName ) )
    {
        if( field->IsPrivate() )
            return wxEmptyString;

        // Get the field text with empty variant name (default value)
        wxString value = symbol->Schematic()->ConvertKIIDsToRefs(
                field->GetText( &aRef.GetSheetPath(), wxEmptyString ) );
        return value;
    }

    // For generated fields, return the field name itself
    if( IsGeneratedField( aFieldName ) )
        return aFieldName;

    return wxEmptyString;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::setAttributeValue( SCH_REFERENCE&  aRef,
                                                       const wxString& aAttributeName,
                                                       const wxString& aValue,
                                                       const wxString& aVariantName )
{
    bool attrChanged = false;
    bool newValue = aValue == wxS( "1" );

    if( aAttributeName == wxS( "${DNP}" ) )
    {
        attrChanged = aRef.GetSymbolDNP( aVariantName ) != newValue;
        aRef.SetSymbolDNP( newValue, aVariantName );
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
    {
        attrChanged = aRef.GetSymbolExcludedFromBoard() != newValue;
        aRef.SetSymbolExcludedFromBoard( newValue );
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
    {
        attrChanged = aRef.GetSymbolExcludedFromBOM( aVariantName ) != newValue;
        aRef.SetSymbolExcludedFromBOM( newValue, aVariantName );
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
    {
        attrChanged = aRef.GetSymbolExcludedFromSim( aVariantName ) != newValue;
        aRef.SetSymbolExcludedFromSim( newValue, aVariantName );
    }

    return attrChanged;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::EnableRebuilds()
{
    m_rebuildsEnabled = true;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::DisableRebuilds()
{
    m_rebuildsEnabled = false;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RebuildRows()
{
    if( !m_rebuildsEnabled )
        return;

    if( GetView() )
    {
        // Commit any pending in-place edits before the row gets moved out from under
        // the editor.
        static_cast<WX_GRID*>( GetView() )->CommitPendingChanges( true );

        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, m_rows.size() );
        GetView()->ProcessTableMessage( msg );
    }

    m_rows.clear();

    EDA_COMBINED_MATCHER matcher( m_filter.Lower(), CTX_SEARCH );

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_REFERENCE ref = m_symbolsList[i];

        if( !m_filter.IsEmpty() && !matcher.Find( ref.GetFullRef().Lower() ) )
            continue;

        if( m_excludeDNP )
        {
            bool isDNP = false;

            if( !m_variantNames.empty() )
            {
                for( const wxString& variantName : m_variantNames )
                {
                    if( ref.GetSymbol()->GetDNP( &ref.GetSheetPath(), variantName )
                        || ref.GetSheetPath().GetDNP( variantName ) )
                    {
                        isDNP = true;
                        break;
                    }
                }
            }
            else
            {
                isDNP = ref.GetSymbol()->GetDNP( &ref.GetSheetPath(), m_currentVariant )
                        || ref.GetSheetPath().GetDNP( m_currentVariant );
            }

            if( isDNP )
                continue;
        }

        if( !m_includeExcluded )
        {
            bool isExcluded = false;

            if( !m_variantNames.empty() )
            {
                for( const wxString& variantName : m_variantNames )
                {
                    if( ref.GetSymbol()->GetExcludedFromBOM( &ref.GetSheetPath(), variantName )
                        || ref.GetSheetPath().GetExcludedFromBOM( variantName ) )
                    {
                        isExcluded = true;
                        break;
                    }
                }
            }
            else
            {
                isExcluded = ref.GetSymbol()->GetExcludedFromBOM( &ref.GetSheetPath(), m_currentVariant )
                             || ref.GetSheetPath().GetExcludedFromBOM( m_currentVariant );
            }

            if( isExcluded )
                continue;
        }

        // Check if the symbol if on the current sheet or, in the sheet path somewhere
        // depending on scope
        if( ( m_scope == SCOPE::SCOPE_SHEET && ref.GetSheetPath() != m_path )
            || ( m_scope == SCOPE::SCOPE_SHEET_RECURSIVE
                 && !ref.GetSheetPath().IsContainedWithin( m_path ) ) )
        {
            continue;
        }

        bool matchFound = false;

        // Performance optimization for ungrouped case to skip the N^2 for loop
        if( !m_groupingEnabled && !ref.IsMultiUnit() )
        {
            m_rows.emplace_back( DATA_MODEL_ROW( ref, GROUP_SINGLETON ) );
            continue;
        }

        // See if we already have a row which this symbol fits into
        for( DATA_MODEL_ROW& row : m_rows )
        {
            // all group members must have identical refs so just use the first one
            SCH_REFERENCE rowRef = row.m_Refs[0];

            if( unitMatch( ref, rowRef ) )
            {
                matchFound = true;
                row.m_Refs.push_back( ref );
                break;
            }
            else if( m_groupingEnabled && groupMatch( ref, rowRef ) )
            {
                matchFound = true;
                row.m_Refs.push_back( ref );
                row.m_Flag = GROUP_COLLAPSED;
                break;
            }
        }

        if( !matchFound )
            m_rows.emplace_back( DATA_MODEL_ROW( ref, GROUP_SINGLETON ) );
    }

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
        GetView()->ProcessTableMessage( msg );
    }

    Sort();
}


void FIELDS_EDITOR_GRID_DATA_MODEL::ExpandRow( int aRow )
{
    std::vector<DATA_MODEL_ROW> children;

    for( SCH_REFERENCE& ref : m_rows[aRow].m_Refs )
    {
        bool matchFound = false;

        // See if we already have a child group which this symbol fits into
        for( DATA_MODEL_ROW& child : children )
        {
            // group members are by definition all matching, so just check
            // against the first member
            if( unitMatch( ref, child.m_Refs[0] ) )
            {
                matchFound = true;
                child.m_Refs.push_back( ref );
                break;
            }
        }

        if( !matchFound )
            children.emplace_back( DATA_MODEL_ROW( ref, CHILD_ITEM ) );
    }

    if( children.size() < 2 )
        return;

    std::sort( children.begin(), children.end(),
               [this]( const DATA_MODEL_ROW& lhs, const DATA_MODEL_ROW& rhs ) -> bool
               {
                   return cmp( lhs, rhs, this, m_sortColumn, m_sortAscending );
               } );

    m_rows[aRow].m_Flag = GROUP_EXPANDED;
    m_rows.insert( m_rows.begin() + aRow + 1, children.begin(), children.end() );

    wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, aRow, children.size() );
    GetView()->ProcessTableMessage( msg );
}


void FIELDS_EDITOR_GRID_DATA_MODEL::CollapseRow( int aRow )
{
    auto firstChild = m_rows.begin() + aRow + 1;
    auto afterLastChild = firstChild;
    int  deleted = 0;

    while( afterLastChild != m_rows.end() && afterLastChild->m_Flag == CHILD_ITEM )
    {
        deleted++;
        afterLastChild++;
    }

    m_rows[aRow].m_Flag = GROUP_COLLAPSED;
    m_rows.erase( firstChild, afterLastChild );

    wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow + 1, deleted );
    GetView()->ProcessTableMessage( msg );
}


void FIELDS_EDITOR_GRID_DATA_MODEL::ExpandCollapseRow( int aRow )
{
    DATA_MODEL_ROW& group = m_rows[aRow];

    if( group.m_Flag == GROUP_COLLAPSED )
        ExpandRow( aRow );
    else if( group.m_Flag == GROUP_EXPANDED )
        CollapseRow( aRow );
}


void FIELDS_EDITOR_GRID_DATA_MODEL::CollapseForSort()
{
    for( size_t i = 0; i < m_rows.size(); ++i )
    {
        if( m_rows[i].m_Flag == GROUP_EXPANDED )
        {
            CollapseRow( i );
            m_rows[i].m_Flag = GROUP_COLLAPSED_DURING_SORT;
        }
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::ExpandAfterSort()
{
    for( size_t i = 0; i < m_rows.size(); ++i )
    {
        if( m_rows[i].m_Flag == GROUP_COLLAPSED_DURING_SORT )
            ExpandRow( i );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData( SCH_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames,
                                               const wxString& aVariantName )
{
    bool symbolModified = false;
    std::unique_ptr<SCH_SYMBOL> symbolCopy;

    for( size_t i = 0; i < m_symbolsList.GetCount(); i++ )
    {
        SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol();
        SCH_SYMBOL* nextSymbol = nullptr;

        if( ( i + 1 ) < m_symbolsList.GetCount() )
            nextSymbol = m_symbolsList[i + 1].GetSymbol();

        if( i == 0 )
            symbolCopy = std::make_unique<SCH_SYMBOL>( *symbol );

        KIID_PATH key = makeDataStoreKey( m_symbolsList[i].GetSheetPath(), *symbol );
        const std::map<wxString, wxString>& fieldStore = m_dataStore[key];

        for( const auto& [srcName, srcValue] : fieldStore )
        {
            // Attributes bypass the field logic, so handle them first
            if( isAttribute( srcName ) )
            {
                symbolModified |= setAttributeValue( m_symbolsList[i], srcName, srcValue, aVariantName );
                continue;
            }

            // Skip generated fields with variables as names (e.g. ${QUANTITY});
            // they can't be edited
            if( IsGeneratedField( srcName ) )
                continue;

            SCH_FIELD* destField = symbol->GetField( srcName );

            if( destField && destField->IsPrivate() )
            {
                if( srcValue.IsEmpty() )
                    continue;
                else
                    destField->SetPrivate( false );
            }

            int  col = GetFieldNameCol( srcName );
            bool userAdded = ( col != -1 && m_cols[col].m_userAdded );

            // Add a not existing field if it has a value for this symbol
            bool createField = !destField && ( !srcValue.IsEmpty() || userAdded );

            if( createField )
            {
                destField = symbol->AddField( SCH_FIELD( symbol, FIELD_T::USER, srcName ) );
                destField->SetTextAngle( symbol->GetField( FIELD_T::REFERENCE )->GetTextAngle() );

                if( const TEMPLATE_FIELDNAME* srcTemplate = aTemplateFieldnames.GetFieldName( srcName ) )
                    destField->SetVisible( srcTemplate->m_Visible );
                else
                    destField->SetVisible( false );

                destField->SetTextPos( symbol->GetPosition() );
                symbolModified = true;
            }

            if( !destField )
                continue;

            // Reference is not editable from this dialog
            if( destField->GetId() == FIELD_T::REFERENCE )
                continue;

            wxString previousValue = destField->GetText( &m_symbolsList[i].GetSheetPath(), aVariantName );

            destField->SetText( symbol->Schematic()->ConvertRefsToKIIDs( srcValue ), &m_symbolsList[i].GetSheetPath(),
                                aVariantName );

            if( !createField && ( previousValue != srcValue ) )
                symbolModified = true;
        }

        for( int ii = static_cast<int>( symbol->GetFields().size() ) - 1; ii >= 0; ii-- )
        {
            if( symbol->GetFields()[ii].IsMandatory() || symbol->GetFields()[ii].IsPrivate() )
                continue;

            if( fieldStore.count( symbol->GetFields()[ii].GetName() ) == 0 )
            {
                symbol->GetFields().erase( symbol->GetFields().begin() + ii );
                symbolModified = true;
            }
        }

        if( symbolModified && ( symbol != nextSymbol ) )
            aCommit.Modified( symbol, symbolCopy.release(), m_symbolsList[i].GetSheetPath().LastScreen() );

        // Only reset the modified flag and next symbol copy if the next symbol is different from the current one.
        if( symbol != nextSymbol )
        {
            if( nextSymbol )
                symbolCopy = std::make_unique<SCH_SYMBOL>( *nextSymbol );
            else
                symbolCopy.reset( nullptr );

            symbolModified = false;
        }
    }

    m_edited = false;
}


int FIELDS_EDITOR_GRID_DATA_MODEL::GetDataWidth( int aCol )
{
    int width = 0;

    if( ColIsReference( aCol ) )
    {
        for( int row = 0; row < GetNumberRows(); ++row )
            width = std::max( width, KIUI::GetTextSize( GetValue( row, aCol ), GetView() ).x );
    }
    else
    {
        wxString fieldName = GetColFieldName( aCol ); // symbol fieldName or Qty string

        for( unsigned symbolRef = 0; symbolRef < m_symbolsList.GetCount(); ++symbolRef )
        {
            KIID_PATH key = makeDataStoreKey( m_symbolsList[symbolRef].GetSheetPath(),
                                              *m_symbolsList[symbolRef].GetSymbol() );
            wxString text = m_dataStore[key][fieldName];

            width = std::max( width, KIUI::GetTextSize( text, GetView() ).x );
        }
    }

    return width;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::ApplyBomPreset( const BOM_PRESET& aPreset, const wxString& aVariantName )
{
    // Hide and un-group everything by default
    for( size_t i = 0; i < m_cols.size(); i++ )
    {
        SetShowColumn( i, false );
        SetGroupColumn( i, false );
    }

    std::set<wxString> seen;
    std::vector<wxString> order;

    // Set columns that are present and shown
    for( const BOM_FIELD& field : aPreset.fieldsOrdered )
    {
        // Ignore empty fields
        if( !field.name || seen.count( field.name ) )
            continue;

        seen.insert( field.name );
        order.emplace_back( field.name );

        int col = GetFieldNameCol( field.name );

        // Add any missing fields, if the user doesn't add any data
        // they won't be saved to the symbols anyway
        if( col == -1 )
        {
            AddColumn( field.name, field.label, true, aVariantName );
            col = GetFieldNameCol( field.name );
        }
        else
        {
            SetColLabelValue( col, field.label );
        }

        SetGroupColumn( col, field.groupBy );
        SetShowColumn( col, field.show );
    }

    // Set grouping columns
    SetGroupingEnabled( aPreset.groupSymbols );

    SetFieldsOrder( order );

    // Set our sorting
    int sortCol = GetFieldNameCol( aPreset.sortField );

    if( sortCol == -1 )
        sortCol = GetFieldNameCol( GetCanonicalFieldName( FIELD_T::REFERENCE ) );

    SetSorting( sortCol, aPreset.sortAsc );

    SetFilter( aPreset.filterString );
    SetExcludeDNP( aPreset.excludeDNP );
    SetIncludeExcludedFromBOM( aPreset.includeExcludedFromBOM );
    SetCurrentVariant( aVariantName );

    RebuildRows();
}


BOM_PRESET FIELDS_EDITOR_GRID_DATA_MODEL::GetBomSettings()
{
    BOM_PRESET current;
    current.readOnly = false;
    current.fieldsOrdered = GetFieldsOrdered();

    if( GetSortCol() >= 0 && GetSortCol() < GetNumberCols() )
        current.sortField = GetColFieldName( GetSortCol() );

    current.sortAsc = GetSortAsc();
    current.filterString = GetFilter();
    current.groupSymbols = GetGroupingEnabled();
    current.excludeDNP = GetExcludeDNP();
    current.includeExcludedFromBOM = GetIncludeExcludedFromBOM();

    return current;
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::Export( const BOM_FMT_PRESET& settings )
{
    wxString out;

    if( m_cols.empty() )
        return out;

    int last_col = -1;

    // Find the location for the line terminator
    for( size_t col = 0; col < m_cols.size(); col++ )
    {
        if( m_cols[col].m_show )
            last_col = static_cast<int>( col );
    }

    // No shown columns
    if( last_col == -1 )
        return out;

    auto formatField =
            [&]( wxString field, bool last ) -> wxString
            {
                if( !settings.keepLineBreaks )
                {
                    field.Replace( wxS( "\r" ), wxS( "" ) );
                    field.Replace( wxS( "\n" ), wxS( "" ) );
                }

                if( !settings.keepTabs )
                {
                    field.Replace( wxS( "\t" ), wxS( "" ) );
                }

                if( !settings.stringDelimiter.IsEmpty() )
                {
                    field.Replace( settings.stringDelimiter,
                                   settings.stringDelimiter + settings.stringDelimiter );
                }

                return settings.stringDelimiter + field + settings.stringDelimiter
                       + ( last ? wxString( wxS( "\n" ) ) : settings.fieldDelimiter );
            };

    // Column names
    for( size_t col = 0; col < m_cols.size(); col++ )
    {
        if( !m_cols[col].m_show )
            continue;

        out.Append( formatField( m_cols[col].m_label, col == static_cast<size_t>( last_col ) ) );
    }

    // Data rows
    for( size_t row = 0; row < m_rows.size(); row++ )
    {
        // Don't output child rows
        if( GetRowFlags( static_cast<int>( row ) ) == CHILD_ITEM )
            continue;

        for( size_t col = 0; col < m_cols.size(); col++ )
        {
            if( !m_cols[col].m_show )
                continue;

            // Get the unannotated version of the field, e.g. no ">   " or "v   " by
            out.Append( formatField( GetExportValue( static_cast<int>( row ), static_cast<int>( col ),
                                                     settings.refDelimiter, settings.refRangeDelimiter ),
                                     col == static_cast<size_t>( last_col ) ) );
        }
    }

    return out;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::AddReferences( const SCH_REFERENCE_LIST& aRefs )
{
    bool refListChanged = false;

    for( const SCH_REFERENCE& ref : aRefs )
    {
        if( !m_symbolsList.Contains( ref ) )
        {
            SCH_SYMBOL* symbol = ref.GetSymbol();

            m_symbolsList.AddItem( ref );

            KIID_PATH key = makeDataStoreKey( ref.GetSheetPath(), *symbol );

            // Update the fields of every reference
            for( const SCH_FIELD& field : symbol->GetFields() )
            {
                if( !field.IsPrivate() )
                {
                    wxString name = field.GetCanonicalName();
                    wxString value = symbol->Schematic()->ConvertKIIDsToRefs( field.GetText() );

                    m_dataStore[key][name] = value;
                }
            }

            refListChanged = true;
        }
    }

    if( refListChanged )
        m_symbolsList.SortBySymbolPtr();
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RemoveSymbol( const SCH_SYMBOL& aSymbol )
{
    // The schematic event listener passes us the symbol after it has been removed,
    // so we can't just work with a SCH_REFERENCE_LIST like the other handlers as the
    // references are already gone. Instead we need to prune our list.

    // Since we now use full KIID_PATH as keys, we need to find and remove all entries
    // that correspond to this symbol (their keys end with the symbol's UUID)
    KIID symbolUuid = aSymbol.m_Uuid;
    std::vector<KIID_PATH> keysToRemove;

    for( const auto& [key, value] : m_dataStore )
    {
        if( !key.empty() && ( key.back() ==  symbolUuid ) )
            keysToRemove.push_back( key );
    }

    for( const KIID_PATH& key : keysToRemove )
        m_dataStore.erase( key );

    // Remove all refs that match this symbol using remove_if
    m_symbolsList.erase( std::remove_if( m_symbolsList.begin(), m_symbolsList.end(),
                                         [&aSymbol]( const SCH_REFERENCE& ref ) -> bool
                                         {
                                             return ref.GetSymbol()->m_Uuid == aSymbol.m_Uuid;
                                         } ),
                         m_symbolsList.end() );
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RemoveReferences( const SCH_REFERENCE_LIST& aRefs )
{
    for( const SCH_REFERENCE& ref : aRefs )
    {
        int index = m_symbolsList.FindRefByFullPath( ref.GetFullPath() );

        if( index != -1 )
        {
            KIID_PATH key = makeDataStoreKey( ref.GetSheetPath(), *ref.GetSymbol() );
            m_dataStore.erase( key );
            m_symbolsList.RemoveItem( index );
        }
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::UpdateReferences( const SCH_REFERENCE_LIST& aRefs,
                                                      const wxString& aVariantName )
{
    bool refListChanged = false;

    for( const SCH_REFERENCE& ref : aRefs )
    {
        // Update the fields of every reference. Do this by iterating through the data model
        // columns; we must have all fields in the symbol added to the data model at this point,
        // and some of the data model columns may be variables that are not present in the symbol
        for( const DATA_MODEL_COL& col : m_cols )
            updateDataStoreSymbolField( ref, col.m_fieldName, aVariantName );

        if( SCH_REFERENCE* listRef = m_symbolsList.FindItem( ref ) )
        {
            *listRef = ref;
        }
        else
        {
            m_symbolsList.AddItem( ref );
            refListChanged = true;
        }
    }

    if( refListChanged )
        m_symbolsList.SortBySymbolPtr();
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::DeleteRows( size_t aPosition, size_t aNumRows )
{
    size_t curNumRows = m_rows.size();

    if( aPosition >= curNumRows )
    {
       wxFAIL_MSG( wxString::Format( wxT( "Called FIELDS_EDITOR_GRID_DATA_MODEL::DeleteRows(aPosition=%lu, "
                                          "aNumRows=%lu)\nPosition value is invalid for present table with %lu rows" ),
                                      (unsigned long) aPosition, (unsigned long) aNumRows,
                                      (unsigned long) curNumRows ) );

        return false;
    }

    if( aNumRows > curNumRows - aPosition )
    {
        aNumRows = curNumRows - aPosition;
    }

    if( aNumRows >= curNumRows )
    {
        m_rows.clear();
        m_dataStore.clear();
    }
    else
    {
        const auto first = m_rows.begin() + aPosition;
        std::vector<SCH_REFERENCE> dataMapRefs = first->m_Refs;
        m_rows.erase( first, first + aNumRows );

        for( const SCH_REFERENCE& ref : dataMapRefs )
            m_dataStore.erase( ref.GetSheetPath().Path() );
    }

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aPosition, aNumRows );
        GetView()->ProcessTableMessage( msg );
    }

    return true;
}
