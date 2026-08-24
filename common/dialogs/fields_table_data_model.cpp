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
#include <utility>

#include <fields_table_data_model.h>
#include <settings/bom_settings.h>
#include <template_fieldnames.h>

#include <nlohmann/json.hpp>
#include <widgets/grid_striped_renderer.h>
#include <widgets/ui_common.h>
#include <wx/dc.h>


GRID_CELL_RESOLVED_TEXT_RENDERER::GRID_CELL_RESOLVED_TEXT_RENDERER() :
        wxGridCellStringRenderer()
{
}


void GRID_CELL_RESOLVED_TEXT_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect,
                                             int aRow, int aCol, bool isSelected )
{
    wxString value = aGrid.GetCellValue( aRow, aCol );

    if( auto* model = dynamic_cast<FIELDS_TABLE_DATA_MODEL_BASE*>( aGrid.GetTable() ) )
        value = model->GetResolvedValue( aRow, aCol );

    wxRect rect = aRect;
    rect.Inflate( -1 );

    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );
    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, value, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}


wxSize GRID_CELL_RESOLVED_TEXT_RENDERER::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow,
                                                      int aCol )
{
    wxString value = aGrid.GetCellValue( aRow, aCol );

    if( auto* model = dynamic_cast<FIELDS_TABLE_DATA_MODEL_BASE*>( aGrid.GetTable() ) )
        value = model->GetResolvedValue( aRow, aCol );

    return wxGridCellStringRenderer::DoGetBestSize( aAttr, aDC, value );
}


wxGridCellRenderer* GRID_CELL_RESOLVED_TEXT_RENDERER::Clone() const
{
    return new GRID_CELL_RESOLVED_TEXT_RENDERER();
}


const wxString FIELDS_TABLE_DATA_MODEL_BASE::QUANTITY_VARIABLE = wxS( "${QUANTITY}" );
const wxString FIELDS_TABLE_DATA_MODEL_BASE::ITEM_NUMBER_VARIABLE = wxS( "${ITEM_NUMBER}" );


FIELDS_TABLE_DATA_MODEL_BASE::FIELDS_TABLE_DATA_MODEL_BASE() :
        m_stripedRenderer( nullptr ),
        m_edited( false ),
        m_sortColumn( 0 ),
        m_sortAscending( false ),
        m_filterScope( BOM_FILTER_SCOPE::REFERENCE ),
        m_groupingEnabled( false ),
        m_excludeDNP( false ),
        m_includeExcluded( false ),
        m_rebuildsEnabled( true )
{
}


FIELDS_TABLE_DATA_MODEL_BASE::~FIELDS_TABLE_DATA_MODEL_BASE()
{
    wxSafeDecRef( m_stripedRenderer );
}


wxGridCellAttr* FIELDS_TABLE_DATA_MODEL_BASE::applyFieldPresenceRenderer( wxGridCellAttr* aAttr, int aRow, int aCol )
{
    if( !IsCellClear( aRow, aCol ) || ColIsAttribute( aCol ) )
        return aAttr;

    wxGridCellAttr* stripedAttr = aAttr ? aAttr->Clone() : new wxGridCellAttr;
    wxSafeDecRef( aAttr );

    if( !m_stripedRenderer )
    {
        m_stripedRenderer = new STRIPED_STRING_RENDERER( FIELDS_TABLE_COLOR::CLEARED_FIELD_STRIPE_ON_DARK_LIGHT_RED,
                                                         FIELDS_TABLE_COLOR::CLEARED_FIELD_STRIPE_ON_LIGHT_DARK_RED );
    }

    m_stripedRenderer->IncRef();
    stripedAttr->SetRenderer( m_stripedRenderer );

    if( !stripedAttr->HasBackgroundColour() )
        stripedAttr->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    return stripedAttr;
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


void FIELDS_TABLE_DATA_MODEL_BASE::RemoveColumn( int aCol )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );

    for( auto& [unused, fieldsStore] : m_dataStore )
    {
        fieldsStore.erase( m_cols[aCol].m_fieldName );
    }

    m_cols.erase( m_cols.begin() + aCol );

    if( m_sortColumn == aCol )
        m_sortColumn = 0;
    else if( m_sortColumn > aCol )
        m_sortColumn--;

    if( auto attrIt = m_colAttrs.find( aCol ); attrIt != m_colAttrs.end() )
    {
        wxSafeDecRef( attrIt->second );
        m_colAttrs.erase( attrIt );
    }

    std::map<int, wxGridCellAttr*> shiftedColAttrs;

    for( const auto& [col, attr] : m_colAttrs )
        shiftedColAttrs[col > aCol ? col - 1 : col] = attr;

    m_colAttrs.swap( shiftedColAttrs );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_COLS_DELETED, aCol, 1 );
        grid->ProcessTableMessage( msg );
    }

    m_edited = true;
}


void FIELDS_TABLE_DATA_MODEL_BASE::RenameColumn( int aCol, const wxString& newName )
{
    for( auto& [unused, fieldsStore] : m_dataStore )
    {
        auto node = fieldsStore.extract( m_cols[aCol].m_fieldName );

        if( !node.empty() )
        {
            node.key() = newName;
            fieldsStore.insert( std::move( node ) );
        }
    }

    m_cols[aCol].m_fieldName = newName;
    m_cols[aCol].m_label = newName;
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


int FIELDS_TABLE_DATA_MODEL_BASE::GetColDataWidth( int aCol )
{
    int width = KIUI::GetTextSize( GetColLabelValue( aCol ), GetView() ).x;

    for( int row = 0; row < GetNumberRows(); ++row )
        width = std::max( width, KIUI::GetTextSize( GetResolvedValue( row, aCol ), GetView() ).x );

    return width;
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


bool FIELDS_TABLE_DATA_MODEL_BASE::ColIsValue( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == GetCanonicalFieldName( FIELD_T::VALUE );
}


bool FIELDS_TABLE_DATA_MODEL_BASE::ColIsFootprint( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == GetCanonicalFieldName( FIELD_T::FOOTPRINT );
}


bool FIELDS_TABLE_DATA_MODEL_BASE::ColIsAttribute( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );

    return fieldIsAttribute( m_cols[aCol].m_fieldName );
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


bool FIELDS_TABLE_DATA_MODEL_BASE::IsCellReadOnly( int, int aCol )
{
    return IsExpanderColumn( aCol );
}


bool FIELDS_TABLE_DATA_MODEL_BASE::CanClearCell( int aRow, int aCol )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), false );
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );

    if( IsCellReadOnly( aRow, aCol ) || IsCellClear( aRow, aCol )
        || IsGeneratedField( m_cols[aCol].m_fieldName ) )
    {
        return false;
    }

    for( FIELD_T fieldId : MANDATORY_FIELDS )
    {
        if( m_cols[aCol].m_fieldName == GetCanonicalFieldName( fieldId ) )
            return false;
    }

    // Template fields are added by default to symbols, but it's unclear whether or not
    // that means they should be mandatory. For now, allow them to be cleared.

    return true;
}


bool FIELDS_TABLE_DATA_MODEL_BASE::IsRowEdited( int aRow )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), false );

    for( int col = 0; col < GetNumberCols(); ++col )
    {
        if( IsCellEdited( aRow, col ) )
            return true;
    }

    return false;
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

    if( ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
        aGroup = false;

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
            AddColumn( field.name, field.label, false );
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
    {
        for( int col = 0; col < GetNumberCols(); ++col )
        {
            if( ColIsItemIdentifier( col ) )
            {
                sortCol = col;
                break;
            }
        }
    }

    SetSorting( sortCol, aPreset.sortAsc );

    SetFilter( aPreset.filterString );
    SetFilterScope( aPreset.filterScope );
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
    current.filterScope = GetFilterScope();
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
        if( GetRowState( row ) == ROW_STATE::EXPANDED_CHILD )
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


bool FIELDS_TABLE_DATA_MODEL_BASE::fieldIsAttribute( const wxString& aFieldName ) const
{
    return aFieldName == wxS( "${DNP}" ) || aFieldName == wxS( "${EXCLUDE_FROM_BOARD}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_BOM}" ) || aFieldName == wxS( "${EXCLUDE_FROM_POS_FILES}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_SIM}" );
}


wxString FIELDS_TABLE_DATA_MODEL_BASE::getAttributeResolvedValue( const wxString& aFieldName, bool aValue ) const
{
    if( !aValue )
        return wxEmptyString;

    if( aFieldName == wxS( "${DNP}" ) )
        return wxS( "DNP" );
    else if( aFieldName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return wxS( "Excluded from board" );
    else if( aFieldName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        return wxS( "Excluded from BOM" );
    else if( aFieldName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
        return wxS( "Excluded from position files" );
    else if( aFieldName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        return wxS( "Excluded from simulation" );

    return wxEmptyString;
}


wxString FIELDS_TABLE_DATA_MODEL_BASE::SerializeUndoState() const
{
    // Serialize the un-applied edit store keyed by symbol identity (sheet path + UUID), so that
    // restoring it is independent of the current row grouping/order.
    nlohmann::json j = nlohmann::json::object();

    for( const auto& [key, fields] : m_dataStore )
    {
        nlohmann::json jfields = nlohmann::json::object();

        for( const auto& [name, value] : fields )
            jfields[std::string( name.ToUTF8() )] = std::string( value.ToUTF8() );

        j[std::string( key.AsString().ToUTF8() )] = jfields;
    }

    return wxString( j.dump() );
}


void FIELDS_TABLE_DATA_MODEL_BASE::RestoreUndoState( const wxString& aState )
{
    nlohmann::json j = nlohmann::json::parse( aState.ToStdString(), nullptr, false );

    if( !j.is_object() )
        return;

    // We want to wipe out key/value presence so we can properly test for
    // empty vs. not-present
    m_dataStore.clear();

    for( auto it = j.begin(); it != j.end(); ++it )
    {
        KIID_PATH                     key( wxString::FromUTF8( it.key().c_str() ) );
        std::map<wxString, wxString>& fields = m_dataStore[key];

        for( auto fit = it.value().begin(); fit != it.value().end(); ++fit )
            fields[wxString::FromUTF8( fit.key().c_str() )] =
                    wxString::FromUTF8( fit.value().get<std::string>().c_str() );
    }

    m_edited = true;
    RebuildRows();

    if( GetView() )
        GetView()->ForceRefresh();
}
