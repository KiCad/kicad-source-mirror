/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 <author>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>
#include <widgets/wx_grid.h>
#include <sch_reference_list.h>
#include <schematic_settings.h>
#include "string_utils.h"

#include "fields_data_model.h"


const wxString FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE = wxS( "${QUANTITY}" );
const wxString FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE = wxS( "${ITEM_NUMBER}" );


void FIELDS_EDITOR_GRID_DATA_MODEL::AddColumn( const wxString& aFieldName, const wxString& aLabel,
                                               bool aAddedByUser )
{
    // Don't add a field twice
    if( GetFieldNameCol( aFieldName ) != -1 )
        return;

    m_cols.push_back( { aFieldName, aLabel, aAddedByUser, false, false } );

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        if( SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol() )
            updateDataStoreSymbolField( *symbol, aFieldName );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::updateDataStoreSymbolField( const SCH_SYMBOL& aSymbol,
                                                                const wxString&   aFieldName )
{
    if( isAttribute( aFieldName ) )
        m_dataStore[aSymbol.m_Uuid][aFieldName] = getAttributeValue( aSymbol, aFieldName );
    else if( const SCH_FIELD* field = aSymbol.GetFieldByName( aFieldName ) )
        m_dataStore[aSymbol.m_Uuid][aFieldName] = field->GetText();
    // Handle fields with variables as names that are not present in the symbol
    // by giving them the correct value
    else if( IsTextVar( aFieldName ) )
        m_dataStore[aSymbol.m_Uuid][aFieldName] = aFieldName;
    else
        m_dataStore[aSymbol.m_Uuid][aFieldName] = wxEmptyString;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RemoveColumn( int aCol )
{
    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        if( SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol() )
            m_dataStore[symbol->m_Uuid].erase( m_cols[aCol].m_fieldName );
    }

    m_cols.erase( m_cols.begin() + aCol );
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RenameColumn( int aCol, const wxString& newName )
{
    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol();

        // Careful; field may have already been renamed from another sheet instance
        if( auto node = m_dataStore[symbol->m_Uuid].extract( m_cols[aCol].m_fieldName ) )
        {
            node.key() = newName;
            m_dataStore[symbol->m_Uuid].insert( std::move( node ) );
        }
    }

    m_cols[aCol].m_fieldName = newName;
    m_cols[aCol].m_label = newName;
}


int FIELDS_EDITOR_GRID_DATA_MODEL::GetFieldNameCol( wxString aFieldName )
{
    for( size_t i = 0; i < m_cols.size(); i++ )
    {
        if( m_cols[i].m_fieldName == aFieldName )
            return (int) i;
    }

    return -1;
}


const std::vector<BOM_FIELD> FIELDS_EDITOR_GRID_DATA_MODEL::GetFieldsOrdered()
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
        for( size_t i = 0; i < m_cols.size(); i++ )
        {
            if( m_cols[i].m_fieldName == newField )
            {
                std::swap( m_cols[foundCount], m_cols[i] );
                foundCount++;
            }
        }
    }
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    if( ColIsReference( aCol ) )
    {
        // Poor-man's tree controls
        if( m_rows[aRow].m_Flag == GROUP_COLLAPSED )
            return wxT( ">  " ) + GetValue( m_rows[aRow], aCol );
        else if( m_rows[aRow].m_Flag == GROUP_EXPANDED )
            return wxT( "v  " ) + GetValue( m_rows[aRow], aCol );
        else if( m_rows[aRow].m_Flag == CHILD_ITEM )
            return wxT( "        " ) + GetValue( m_rows[aRow], aCol );
        else
            return wxT( "    " ) + GetValue( m_rows[aRow], aCol );
    }
    else
    {
        return GetValue( m_rows[aRow], aCol );
    }
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::GetValue( const DATA_MODEL_ROW& group, int aCol,
                                                  const wxString& refDelimiter,
                                                  const wxString& refRangeDelimiter,
                                                  bool            resolveVars )
{
    std::vector<SCH_REFERENCE> references;
    wxString                   fieldValue;

    for( const SCH_REFERENCE& ref : group.m_Refs )
    {
        if( ColIsReference( aCol ) || ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
        {
            references.push_back( ref );
        }
        else // Other columns are either a single value or ROW_MULTI_ITEMS
        {
            const KIID& symbolID = ref.GetSymbol()->m_Uuid;

            if( !m_dataStore.count( symbolID )
                || !m_dataStore[symbolID].count( m_cols[aCol].m_fieldName ) )
            {
                return INDETERMINATE_STATE;
            }

            wxString refFieldValue;

            // Only resolve vars on actual variables, otherwise we want to get
            // our values out of the datastore so we can show/export un-applied values
            if( resolveVars
                && ( IsTextVar( m_cols[aCol].m_fieldName )
                     || IsTextVar( m_dataStore[symbolID][m_cols[aCol].m_fieldName] ) ) )
            {
                refFieldValue = getFieldShownText( ref, m_cols[aCol].m_fieldName );
            }
            else
                refFieldValue = m_dataStore[symbolID][m_cols[aCol].m_fieldName];

            if( &ref == &group.m_Refs.front() )
                fieldValue = refFieldValue;
            else if( fieldValue != refFieldValue )
                return INDETERMINATE_STATE;
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
    wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), wxS( "Invalid column number" ) );

    // Can't modify references or text variables column, e.g. ${QUANTITY}
    if( ColIsReference( aCol )
        || ( IsTextVar( m_cols[aCol].m_fieldName ) && !ColIsAttribute( aCol ) ) )
    {
        return;
    }

    DATA_MODEL_ROW& rowGroup = m_rows[aRow];

    for( const SCH_REFERENCE& ref : rowGroup.m_Refs )
        m_dataStore[ref.GetSymbol()->m_Uuid][m_cols[aCol].m_fieldName] = aValue;

    m_edited = true;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsReference( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
    return m_cols[aCol].m_fieldName == GetCanonicalFieldName( REFERENCE_FIELD );
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsValue( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
    return m_cols[aCol].m_fieldName == GetCanonicalFieldName( VALUE_FIELD );
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsQuantity( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
    return m_cols[aCol].m_fieldName == QUANTITY_VARIABLE;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsItemNumber( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
    return m_cols[aCol].m_fieldName == ITEM_NUMBER_VARIABLE;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsAttribute( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
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

    wxString lhs = dataModel->GetValue( lhGroup, sortCol ).Trim( true ).Trim( false );
    wxString rhs = dataModel->GetValue( rhGroup, sortCol ).Trim( true ).Trim( false );

    if( lhs == rhs || sortCol == REFERENCE_FIELD )
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


bool FIELDS_EDITOR_GRID_DATA_MODEL::unitMatch( const SCH_REFERENCE& lhRef,
                                               const SCH_REFERENCE& rhRef )
{
    // If items are unannotated then we can't tell if they're units of the same symbol or not
    if( lhRef.GetRefNumber() == wxT( "?" ) )
        return false;

    return ( lhRef.GetRef() == rhRef.GetRef() && lhRef.GetRefNumber() == rhRef.GetRefNumber() );
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::groupMatch( const SCH_REFERENCE& lhRef,
                                                const SCH_REFERENCE& rhRef )

{
    int  refCol = GetFieldNameCol( GetCanonicalFieldName( REFERENCE_FIELD ) );
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

    const KIID& lhRefID = lhRef.GetSymbol()->m_Uuid;
    const KIID& rhRefID = rhRef.GetSymbol()->m_Uuid;

    // Now check all the other columns.
    for( size_t i = 0; i < m_cols.size(); ++i )
    {
        //Handled already
        if( (int) i == refCol )
            continue;

        if( !m_cols[i].m_group )
            continue;

        // If the field is a variable, we need to resolve it through the symbol
        // to get the actual current value, otherwise we need to pull it out of the
        // store so the refresh can regroup based on values that haven't been applied
        // to the schematic yet.
        wxString lh, rh;

        if( IsTextVar( m_cols[i].m_fieldName )
            || IsTextVar( m_dataStore[lhRefID][m_cols[i].m_fieldName] ) )
        {
            lh = getFieldShownText( lhRef, m_cols[i].m_fieldName );
        }
        else
            lh = m_dataStore[lhRefID][m_cols[i].m_fieldName];

        if( IsTextVar( m_cols[i].m_fieldName )
            || IsTextVar( m_dataStore[rhRefID][m_cols[i].m_fieldName] ) )
        {
            rh = getFieldShownText( rhRef, m_cols[i].m_fieldName );
        }
        else
            rh = m_dataStore[rhRefID][m_cols[i].m_fieldName];

        wxString fieldName = m_cols[i].m_fieldName;

        if( lh != rh )
            return false;

        matchFound = true;
    }

    return matchFound;
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::getFieldShownText( const SCH_REFERENCE& aRef,
                                                           const wxString&      aFieldName )
{
    SCH_FIELD* field = aRef.GetSymbol()->GetFieldByName( aFieldName );

    if( field )
        return field->GetShownText( &aRef.GetSheetPath(), false );

    // Handle fields with variables as names that are not present in the symbol
    // by giving them the correct value by resolving against the symbol
    if( IsTextVar( aFieldName ) )
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


wxString FIELDS_EDITOR_GRID_DATA_MODEL::getAttributeValue( const SCH_SYMBOL& aSymbol,
                                                           const wxString&   aAttributeName )
{
    if( aAttributeName == wxS( "${DNP}" ) )
        return aSymbol.GetDNP() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return aSymbol.GetExcludedFromBoard() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        return aSymbol.GetExcludedFromBOM() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        return aSymbol.GetExcludedFromSim() ? wxS( "1" ) : wxS( "0" );

    return wxS( "0" );
}

void FIELDS_EDITOR_GRID_DATA_MODEL::setAttributeValue( SCH_SYMBOL&     aSymbol,
                                                       const wxString& aAttributeName,
                                                       const wxString& aValue )
{
    if( aAttributeName == wxS( "${DNP}" ) )
        aSymbol.SetDNP( aValue == wxS( "1" ) );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        aSymbol.SetExcludedFromBoard( aValue == wxS( "1" ) );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        aSymbol.SetExcludedFromBOM( aValue == wxS( "1" ) );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        aSymbol.SetExcludedFromSim( aValue == wxS( "1" ) );
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

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_REFERENCE ref = m_symbolsList[i];

        if( !m_filter.IsEmpty() && !WildCompareString( m_filter, ref.GetFullRef(), false ) )
            continue;

        if( m_excludeDNP && ref.GetSymbol()->GetDNP() )
            continue;

        if( !m_includeExcluded && ref.GetSymbol()->GetExcludedFromBOM() )
            continue;

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


void FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData(
        std::function<void( SCH_SYMBOL&, SCH_SHEET_PATH& )> symbolChangeHandler )
{
    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_SYMBOL& symbol = *m_symbolsList[i].GetSymbol();

        symbolChangeHandler( symbol, m_symbolsList[i].GetSheetPath() );

        const std::map<wxString, wxString>& fieldStore = m_dataStore[symbol.m_Uuid];

        for( const std::pair<wxString, wxString> srcData : fieldStore )
        {
            const wxString& srcName = srcData.first;
            const wxString& srcValue = srcData.second;

            // Attributes bypass the field logic, so handle them first
            if( isAttribute( srcName ) )
            {
                setAttributeValue( *m_symbolsList[i].GetSymbol(), srcName, srcValue );
                continue;
            }

            // Skip special fields with variables as names (e.g. ${QUANTITY}),
            // they can't be edited
            if( IsTextVar( srcName ) )
                continue;

            SCH_FIELD*      destField = symbol.FindField( srcName );
            int             col = GetFieldNameCol( srcName );
            bool            userAdded = ( col != -1 && m_cols[col].m_userAdded );

            // Add a not existing field if it has a value for this symbol
            bool createField = !destField && ( !srcValue.IsEmpty() || userAdded );

            if( createField )
            {
                const VECTOR2I symbolPos = symbol.GetPosition();
                destField = symbol.AddField( SCH_FIELD( symbolPos, -1, &symbol, srcName ) );
            }

            if( !destField )
                continue;

            if( destField->GetId() == REFERENCE_FIELD )
            {
                // Reference is not editable from this dialog
            }
            else if( destField->GetId() == VALUE_FIELD )
            {
                // Value field cannot be empty
                if( !srcValue.IsEmpty() )
                    symbol.SetValueFieldText( srcValue );
            }
            else if( destField->GetId() == FOOTPRINT_FIELD )
            {
                symbol.SetFootprintFieldText( srcValue );
            }
            else
            {
                destField->SetText( srcValue );
            }
        }

        for( int ii = symbol.GetFields().size() - 1; ii >= MANDATORY_FIELDS; ii-- )
        {
            if( fieldStore.count( symbol.GetFields()[ii].GetName() ) == 0 )
                symbol.GetFields().erase( symbol.GetFields().begin() + ii );
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
            const KIID& symbolID = m_symbolsList[symbolRef].GetSymbol()->m_Uuid;
            wxString    text = m_dataStore[symbolID][fieldName];

            width = std::max( width, KIUI::GetTextSize( text, GetView() ).x );
        }
    }

    return width;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::ApplyBomPreset( const BOM_PRESET& aPreset )
{
    // Hide and un-group everything by default
    for( size_t i = 0; i < m_cols.size(); i++ )
    {
        SetShowColumn( i, false );
        SetGroupColumn( i, false );
    }

    std::vector<wxString> order;

    // Set columns that are present and shown
    for( BOM_FIELD field : aPreset.fieldsOrdered )
    {
        // Ignore empty fields
        if(!field.name)
            continue;

        order.emplace_back( field.name );

        int col = GetFieldNameCol( field.name );

        // Add any missing fields, if the user doesn't add any data
        // they won't be saved to the symbols anyway
        if( col == -1 )
        {
            AddColumn( field.name, field.label, true );
            col = GetFieldNameCol( field.name );
        }
        else
            SetColLabelValue( col, field.label );

        SetGroupColumn( col, field.groupBy );
        SetShowColumn( col, field.show );
    }

    // Set grouping columns
    SetGroupingEnabled( aPreset.groupSymbols );

    SetFieldsOrder( order );

    // Set our sorting
    int sortCol = GetFieldNameCol( aPreset.sortField );

    if( sortCol != -1 )
        SetSorting( sortCol, aPreset.sortAsc );
    else
        SetSorting( GetFieldNameCol( GetCanonicalFieldName( REFERENCE_FIELD ) ), aPreset.sortAsc );

    SetFilter( aPreset.filterString );
    SetExcludeDNP( aPreset.excludeDNP );
    SetIncludeExcludedFromBOM( aPreset.includeExcludedFromBOM );

    RebuildRows();
}


BOM_PRESET FIELDS_EDITOR_GRID_DATA_MODEL::GetBomSettings()
{
    BOM_PRESET current;
    current.readOnly = false;
    current.fieldsOrdered = GetFieldsOrdered();
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
            last_col = (int) col;
    }

    // No shown columns
    if( last_col == -1 )
        return out;

    auto formatField = [&]( wxString field, bool last ) -> wxString
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
                field.Replace( settings.stringDelimiter,
                               settings.stringDelimiter + settings.stringDelimiter );

            return settings.stringDelimiter + field + settings.stringDelimiter
                   + ( last ? wxString( wxS( "\n" ) ) : settings.fieldDelimiter );
        };

    // Column names
    for( size_t col = 0; col < m_cols.size(); col++ )
    {
        if( !m_cols[col].m_show )
            continue;

        out.Append( formatField( m_cols[col].m_label, col == (size_t) last_col ) );
    }

    // Data rows
    for( size_t row = 0; row < m_rows.size(); row++ )
    {
        // Don't output child rows
        if( GetRowFlags( (int) row ) == CHILD_ITEM )
            continue;

        for( size_t col = 0; col < m_cols.size(); col++ )
        {
            if( !m_cols[col].m_show )
                continue;

            // Get the unanottated version of the field, e.g. no ">   " or "v   " by
            out.Append( formatField( GetExportValue( (int) row, (int) col, settings.refDelimiter,
                                                     settings.refRangeDelimiter ),
                                     col == (size_t) last_col ) );
        }
    }

    return out;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::AddReferences( const SCH_REFERENCE_LIST& aRefs )
{
    for( const SCH_REFERENCE& ref : aRefs )
    {
        if( !m_symbolsList.Contains( ref ) )
        {
            m_symbolsList.AddItem( ref );

            // Update the fields of every reference
            for( const SCH_FIELD& field : ref.GetSymbol()->GetFields() )
                m_dataStore[ref.GetSymbol()->m_Uuid][field.GetCanonicalName()] = field.GetText();
        }
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::RemoveSymbol( const SCH_SYMBOL& aSymbol )
{
    // The schematic event listener passes us the symbol after it has been removed,
    // so we can't just work with a SCH_REFERENCE_LIST like the other handlers as the
    // references are already gone. Instead we need to prune our list.
    m_dataStore[aSymbol.m_Uuid].clear();

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
            m_symbolsList.RemoveItem( index );

            // If we're out of instances then remove the symbol, too
            if( ref.GetSymbol()->GetInstances().empty() )
                m_dataStore.erase( ref.GetSymbol()->m_Uuid );
        }
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::UpdateReferences( const SCH_REFERENCE_LIST& aRefs )
{
    for( const SCH_REFERENCE& ref : aRefs )
    {
        // Update the fields of every reference. Do this by iterating through the data model
        // columns; we must have all fields in the symbol added to the data model at this point,
        // and some of the data model columns may be variables that are not present in the symbol
        for( const DATA_MODEL_COL& col : m_cols )
            updateDataStoreSymbolField( *ref.GetSymbol(), col.m_fieldName );

        if( !m_symbolsList.Contains( ref ) )
            m_symbolsList.AddItem( ref );
    }
}
