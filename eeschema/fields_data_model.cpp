#include <wx/string.h>
#include <wx/debug.h>
#include <wx/grid.h>
#include <common.h>
#include <widgets/wx_grid.h>
#include <sch_reference_list.h>
#include <schematic_settings.h>
#include "string_utils.h"

#include "fields_data_model.h"


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
        {
            if( SCH_FIELD* field = symbol->GetFieldByName( aFieldName ) )
                m_dataStore[symbol->m_Uuid][aFieldName] = field->GetText();
            // Handle fields with variables as names that are not present in the symbol
            // by giving them the correct value
            else if( aFieldName.StartsWith( wxT( "${" ) ) )
                m_dataStore[symbol->m_Uuid][aFieldName] = aFieldName;
            else
                m_dataStore[symbol->m_Uuid][aFieldName] = wxEmptyString;
        }
    }
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

        auto node = m_dataStore[symbol->m_Uuid].extract( m_cols[aCol].m_fieldName );
        node.key() = newName;
        m_dataStore[symbol->m_Uuid].insert( std::move( node ) );
    }

    m_cols[aCol].m_fieldName = newName;
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

            if( resolveVars )
                refFieldValue = getFieldShownText( ref, m_cols[aCol].m_fieldName );
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
    if( ColIsReference( aCol ) || ColIsQuantity( aCol ) )
        return; // Can't modify references or quantity

    DATA_MODEL_ROW& rowGroup = m_rows[aRow];

    for( const SCH_REFERENCE& ref : rowGroup.m_Refs )
        m_dataStore[ref.GetSymbol()->m_Uuid][m_cols[aCol].m_fieldName] = aValue;

    m_edited = true;
}


bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsReference( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
    return m_cols[aCol].m_fieldName == TEMPLATE_FIELDNAME::GetDefaultFieldName( REFERENCE_FIELD );
}

bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsQuantity( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
    return m_cols[aCol].m_fieldName == wxS( "Quantity" );
}

bool FIELDS_EDITOR_GRID_DATA_MODEL::ColIsItemNumber( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
    return m_cols[aCol].m_fieldName == wxS( "Item Number" );
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

    wxString lhs = dataModel->GetValue( (DATA_MODEL_ROW&) lhGroup, sortCol );
    wxString rhs = dataModel->GetValue( (DATA_MODEL_ROW&) rhGroup, sortCol );

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
    int  refCol = GetFieldNameCol( TEMPLATE_FIELDNAME::GetDefaultFieldName( REFERENCE_FIELD ) );
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

    // Now check all the other columns.  This must be done out of the dataStore
    // for the refresh button to work after editing.
    for( size_t i = 0; i < m_cols.size(); ++i )
    {
        //Handled already
        if( (int) i == refCol )
            continue;

        if( !m_cols[i].m_group )
            continue;

        if( getFieldShownText( lhRef, m_cols[i].m_fieldName )
            != getFieldShownText( rhRef, m_cols[i].m_fieldName ) )
        {
            return false;
        }

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
    if( aFieldName.StartsWith( wxT( "${" ) ) )
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
            if( srcData.first == _( "Quantity" ) )
                continue;

            const wxString& srcName = srcData.first;
            const wxString& srcValue = srcData.second;
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
        order.emplace_back( field.name );

        int col = GetFieldNameCol( field.name );

        // Add any missing fields, if the user doesn't add any data
        // they won't be saved to the symbols anywa
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
        SetSorting( GetFieldNameCol( TEMPLATE_FIELDNAME::GetDefaultFieldName( REFERENCE_FIELD ) ),
                    aPreset.sortAsc );

    SetFilter( aPreset.filterString );
    SetExcludeDNP( aPreset.excludeDNP );

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
                   + ( last ? wxString( wxS( "\r\n" ) ) : settings.fieldDelimiter );
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
