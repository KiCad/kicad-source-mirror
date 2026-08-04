/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <set>

#include <fields_table_data_model.h>
#include <settings/bom_settings.h>
#include <template_fieldnames.h>


const wxString FIELDS_TABLE_DATA_MODEL_BASE::QUANTITY_VARIABLE = wxS( "${QUANTITY}" );
const wxString FIELDS_TABLE_DATA_MODEL_BASE::ITEM_NUMBER_VARIABLE = wxS( "${ITEM_NUMBER}" );


FIELDS_TABLE_DATA_MODEL_BASE::FIELDS_TABLE_DATA_MODEL_BASE() :
        m_sortColumn( 0 ),
        m_sortAscending( false ),
        m_groupingEnabled( false ),
        m_excludeDNP( false ),
        m_includeExcluded( false ),
        m_rebuildsEnabled( true )
{
}


void FIELDS_TABLE_DATA_MODEL_BASE::MoveColumn( int aCol, int aNewPos )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );

    if( aCol == aNewPos )
    {
        return;
    }
    else if( aCol < aNewPos )
    {
        std::rotate( std::begin( m_cols ) + aCol, std::begin( m_cols ) + aCol + 1, std::begin( m_cols ) + aNewPos + 1 );
    }
    else
    {
        std::rotate( std::begin( m_cols ) + aNewPos, std::begin( m_cols ) + aCol, std::begin( m_cols ) + aCol + 1 );
    }
}


void FIELDS_TABLE_DATA_MODEL_BASE::SetColLabelValue( int aCol, const wxString& aLabel )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );
    m_cols[aCol].m_label = aLabel;
}


wxString FIELDS_TABLE_DATA_MODEL_BASE::GetColLabelValue( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxString() );
    return m_cols[aCol].m_label;
}


wxString FIELDS_TABLE_DATA_MODEL_BASE::GetColFieldName( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxString() );
    return m_cols[aCol].m_fieldName;
}


int FIELDS_TABLE_DATA_MODEL_BASE::GetFieldNameCol( const wxString& aFieldName ) const
{
    for( size_t i = 0; i < m_cols.size(); i++ )
    {
        if( FieldNamesAreDuplicates( m_cols[i].m_fieldName, aFieldName ) )
            return static_cast<int>( i );
    }

    return -1;
}


std::vector<BOM_FIELD> FIELDS_TABLE_DATA_MODEL_BASE::GetFieldsOrdered()
{
    std::vector<BOM_FIELD> fields;

    for( const DATA_MODEL_COL& col : m_cols )
        fields.push_back( { col.m_fieldName, col.m_label, col.m_show, col.m_group } );

    return fields;
}


void FIELDS_TABLE_DATA_MODEL_BASE::SetFieldsOrder( const std::vector<wxString>& aNewOrder )
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


bool FIELDS_TABLE_DATA_MODEL_BASE::ColIsReference( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == GetCanonicalFieldName( FIELD_T::REFERENCE );
}


bool FIELDS_TABLE_DATA_MODEL_BASE::ColIsQuantity( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == QUANTITY_VARIABLE;
}


bool FIELDS_TABLE_DATA_MODEL_BASE::ColIsItemNumber( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == ITEM_NUMBER_VARIABLE;
}


bool FIELDS_TABLE_DATA_MODEL_BASE::IsExpanderColumn( int aCol ) const
{
    // Check if aCol is the first visible column
    for( int col = 0; col < aCol; ++col )
    {
        if( m_cols[col].m_show )
            return false;
    }

    return true;
}


void FIELDS_TABLE_DATA_MODEL_BASE::SetSorting( int aCol, bool aAscending )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );
    m_sortColumn = aCol;
    m_sortAscending = aAscending;
}


void FIELDS_TABLE_DATA_MODEL_BASE::EnableRebuilds()
{
    m_rebuildsEnabled = true;
}


void FIELDS_TABLE_DATA_MODEL_BASE::DisableRebuilds()
{
    m_rebuildsEnabled = false;
}


void FIELDS_TABLE_DATA_MODEL_BASE::SetGroupColumn( int aCol, bool aGroup )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );
    m_cols[aCol].m_group = aGroup;
}


bool FIELDS_TABLE_DATA_MODEL_BASE::GetGroupColumn( int aCol )
{
    wxCHECK_MSG( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false, "Invalid Column Number" );
    return m_cols[aCol].m_group;
}


void FIELDS_TABLE_DATA_MODEL_BASE::SetShowColumn( int aCol, bool aShow )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );
    m_cols[aCol].m_show = aShow;
}


bool FIELDS_TABLE_DATA_MODEL_BASE::GetShowColumn( int aCol )
{
    wxCHECK_MSG( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false, "Invalid Column Number" );
    return m_cols[aCol].m_show;
}


void FIELDS_TABLE_DATA_MODEL_BASE::ApplyBomPreset( const BOM_PRESET& aPreset )
{
    // Hide and un-group everything by default
    for( size_t i = 0; i < m_cols.size(); i++ )
    {
        SetShowColumn( i, false );
        SetGroupColumn( i, false );
    }

    std::set<wxString>    seen;
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

        // Add any missing fields; if the user doesn't add any data they won't be saved to the
        // design anyway.
        if( col == -1 )
        {
            AddColumn( field.name, field.label, true );
            col = GetFieldNameCol( field.name );
        }
        else
        {
            SetColLabelValue( col, field.label );
        }

        SetGroupColumn( col, field.groupBy );
        SetShowColumn( col, field.show );
    }

    SetGroupingEnabled( aPreset.groupSymbols );
    SetFieldsOrder( order );

    int sortCol = GetFieldNameCol( aPreset.sortField );

    if( sortCol == -1 )
        sortCol = GetFieldNameCol( GetCanonicalFieldName( FIELD_T::REFERENCE ) );

    SetSorting( sortCol, aPreset.sortAsc );

    SetFilter( aPreset.filterString );
    SetExcludeDNP( aPreset.excludeDNP );
    SetIncludeExcludedFromBOM( aPreset.includeExcludedFromBOM );

    RebuildRows();
}


BOM_PRESET FIELDS_TABLE_DATA_MODEL_BASE::GetBomSettings()
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


wxString FIELDS_TABLE_DATA_MODEL_BASE::Export( const BOM_FMT_PRESET& aSettings )
{
    wxString out;

    if( m_cols.empty() )
        return out;

    int lastCol = -1;

    // Find the location for the line terminator
    for( size_t col = 0; col < m_cols.size(); col++ )
    {
        if( m_cols[col].m_show )
            lastCol = static_cast<int>( col );
    }

    // No shown columns
    if( lastCol == -1 )
        return out;

    if( aSettings.includeByteOrderMark )
        out.Append( wxString::FromUTF8( "\xEF\xBB\xBF" ) );

    auto formatField = [&]( wxString aField, bool aLast ) -> wxString
    {
        if( !aSettings.keepLineBreaks )
        {
            aField.Replace( wxS( "\r" ), wxS( "" ) );
            aField.Replace( wxS( "\n" ), wxS( "" ) );
        }

        if( !aSettings.keepTabs )
            aField.Replace( wxS( "\t" ), wxS( "" ) );

        if( !aSettings.stringDelimiter.IsEmpty() )
        {
            aField.Replace( aSettings.stringDelimiter, aSettings.stringDelimiter + aSettings.stringDelimiter );
        }

        return aSettings.stringDelimiter + aField + aSettings.stringDelimiter
               + ( aLast ? wxString( wxS( "\n" ) ) : aSettings.fieldDelimiter );
    };

    // Column names
    for( size_t col = 0; col < m_cols.size(); col++ )
    {
        if( !m_cols[col].m_show )
            continue;

        out.Append( formatField( m_cols[col].m_label, col == static_cast<size_t>( lastCol ) ) );
    }

    // Data rows
    for( int row = 0; row < GetNumberRows(); row++ )
    {
        // Don't output child rows
        if( GetGroupType( row ) == CHILD_ITEM )
            continue;

        for( size_t col = 0; col < m_cols.size(); col++ )
        {
            if( !m_cols[col].m_show )
                continue;

            out.Append( formatField(
                    GetExportValue( row, static_cast<int>( col ), aSettings.refDelimiter, aSettings.refRangeDelimiter ),
                    col == static_cast<size_t>( lastCol ) ) );
        }
    }

    return out;
}
