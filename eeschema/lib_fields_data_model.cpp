/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
#include <wx/settings.h>
#include <wx/brush.h>
#include <common.h>
#include <widgets/wx_grid.h>
#include <sch_reference_list.h>
#include <schematic_settings.h>
#include "string_utils.h"
#include <trace_helpers.h>

#include "lib_fields_data_model.h"


const wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE = wxS( "${ITEM_NUMBER}" );
const wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME = wxS( "Symbol Name" );


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::AddColumn( const wxString& aFieldName, const wxString& aLabel,
                                                   bool aAddedByUser, bool aIsCheckbox )
{
    // Don't add a field twice
    if( GetFieldNameCol( aFieldName ) != -1 )
        return;

    m_cols.push_back( { aFieldName, aLabel, aAddedByUser, false, false, aIsCheckbox } );

    for( LIB_SYMBOL* symbol : m_symbolsList )
        updateDataStoreSymbolField( symbol, aFieldName );
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::updateDataStoreSymbolField( const LIB_SYMBOL* aSymbol,
                                                                    const wxString&   aFieldName )
{
    int col = GetFieldNameCol( aFieldName );
    LIB_DATA_ELEMENT& dataElement = m_dataStore[aSymbol->m_Uuid][aFieldName];

    if( col != -1 && ColIsCheck( col ) )
    {
        dataElement.m_originalData = getAttributeValue( aSymbol, aFieldName );
        dataElement.m_currentData = getAttributeValue( aSymbol, aFieldName );
        dataElement.m_originallyEmpty = false;
        dataElement.m_currentlyEmpty = false;
        dataElement.m_isModified = false;
    }
    else if( const SCH_FIELD* field = aSymbol->GetField( aFieldName ) )
    {
        dataElement.m_originalData = field->GetText();
        dataElement.m_currentData = field->GetText();
        dataElement.m_originallyEmpty = false;
        dataElement.m_currentlyEmpty = false;
        dataElement.m_isModified = false;
    }
    else if( aFieldName == wxS( "Keywords" ) )
    {
        dataElement.m_originalData = aSymbol->GetKeyWords();
        dataElement.m_currentData = aSymbol->GetKeyWords();
        dataElement.m_originallyEmpty = false;
        dataElement.m_currentlyEmpty = false;
        dataElement.m_isModified = false;
    }
    else if( ColIsSymbolName( col ) )
    {
        dataElement.m_originalData = aSymbol->GetName();
        dataElement.m_currentData = aSymbol->GetName();
        dataElement.m_originallyEmpty = false;
        dataElement.m_currentlyEmpty = false;
        dataElement.m_isModified = false;
    }
    else
    {
        m_dataStore[aSymbol->m_Uuid][aFieldName].m_originalData = wxEmptyString;
        m_dataStore[aSymbol->m_Uuid][aFieldName].m_currentData = wxEmptyString;
        m_dataStore[aSymbol->m_Uuid][aFieldName].m_originallyEmpty = true;
        m_dataStore[aSymbol->m_Uuid][aFieldName].m_currentlyEmpty = true;
        m_dataStore[aSymbol->m_Uuid][aFieldName].m_isModified = false;
    }
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::RemoveColumn( int aCol )
{
    const wxString fieldName = m_cols[aCol].m_fieldName;

    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        std::map<wxString, LIB_DATA_ELEMENT>& fieldStore = m_dataStore[symbol->m_Uuid];
        auto it = fieldStore.find( fieldName );

        if( it != fieldStore.end() )
        {
            it->second.m_currentData = wxEmptyString;
            it->second.m_currentlyEmpty = true;
            it->second.m_isModified = true;
            fieldStore.erase( it );
        }
    }

    m_cols.erase( m_cols.begin() + aCol );
    m_edited = true;
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::RenameColumn( int aCol, const wxString& newName )
{
    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        // Careful; field may have already been renamed from another sheet instance
        if( auto node = m_dataStore[symbol->m_Uuid].extract( m_cols[aCol].m_fieldName ) )
        {
            node.key() = newName;
            node.mapped().m_isModified = true;
            m_dataStore[symbol->m_Uuid].insert( std::move( node ) );
        }
    }

    m_cols[aCol].m_fieldName = newName;
    m_cols[aCol].m_label = newName;
    m_edited = true;
}


int LIB_FIELDS_EDITOR_GRID_DATA_MODEL::GetFieldNameCol( const wxString& aFieldName )
{
    for( size_t i = 0; i < m_cols.size(); i++ )
    {
        if( m_cols[i].m_fieldName == aFieldName )
            return (int) i;
    }

    return -1;
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SetFieldsOrder( const std::vector<wxString>& aNewOrder )
{
    size_t foundCount = 0;

    if( aNewOrder.size() > m_cols.size() )
        wxLogDebug( "New order contains more fields than existing columns." );

    for( const wxString& newField : aNewOrder )
    {
        bool found = false;
        for( size_t i = 0; i < m_cols.size() && foundCount < m_cols.size(); i++ )
        {
            if( m_cols[i].m_fieldName == newField )
            {
                std::swap( m_cols[foundCount], m_cols[i] );
                foundCount++;
                found = true;
                break;
            }
        }

        if( !found )
            wxLogDebug( "Field '%s' not found in existing columns.", newField );
    }

    if( foundCount != m_cols.size() && foundCount != aNewOrder.size() )
    {
        wxLogDebug( "Not all fields in the new order were found in the existing columns." );
    }
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::IsExpanderColumn( int aCol ) const
{
    // Check if aCol is the first visible column
    for( int col = 0; col < aCol; ++col )
    {
        if( m_cols[col].m_show )
            return false;
    }

    return true;
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    GetView()->SetReadOnly( aRow, aCol, IsExpanderColumn( aCol ) );
    return GetValue( m_rows[aRow], aCol );
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::GetValue( const LIB_DATA_MODEL_ROW& group, int aCol )
{
    std::set<wxString> mixedValues;
    bool               listMixedValues = ColIsSymbolName( aCol );
    wxString           fieldValue = INDETERMINATE_STATE;
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), fieldValue );

    LIB_DATA_MODEL_COL& col = m_cols[aCol];

    for( const LIB_SYMBOL* ref : group.m_Refs )
    {
        const KIID& symbolID = ref->m_Uuid;

        if( !m_dataStore.contains( symbolID ) || !m_dataStore[symbolID].contains( col.m_fieldName ) )
            return INDETERMINATE_STATE;

        wxString refFieldValue = m_dataStore[symbolID][col.m_fieldName].m_currentData;

        if( listMixedValues )
            mixedValues.insert( refFieldValue );
        else if( ref == group.m_Refs.front() )
            fieldValue = refFieldValue;
        else if( fieldValue != refFieldValue )
            return INDETERMINATE_STATE;
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

    return fieldValue;
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), wxS( "Invalid column number" ) );

    if( ColIsSymbolName( aCol ) )
        return;

    LIB_DATA_MODEL_ROW& rowGroup = m_rows[aRow];

    for( const LIB_SYMBOL* ref : rowGroup.m_Refs )
    {
        LIB_DATA_ELEMENT& dataElement = m_dataStore[ref->m_Uuid][m_cols[aCol].m_fieldName];
        dataElement.m_currentData = aValue;
        dataElement.m_isModified = ( dataElement.m_currentData != dataElement.m_originalData );
        dataElement.m_currentlyEmpty = false;
    }

    m_edited = true;
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ColIsSymbolName( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME;
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ColIsCheck( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_isCheckbox;
}


wxGridCellAttr* LIB_FIELDS_EDITOR_GRID_DATA_MODEL::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    wxGridCellAttr* attr = wxGridTableBase::GetAttr( aRow, aCol, aKind );

    // Check for column-specific attributes first
    if( m_colAttrs.find( aCol ) != m_colAttrs.end() && m_colAttrs[aCol] )
    {
        if( attr )
        {
            // Merge with existing attributes
            wxGridCellAttr* newAttr = m_colAttrs[aCol]->Clone();

            // Copy any existing attributes that aren't overridden
            if( attr->HasBackgroundColour() && !newAttr->HasBackgroundColour() )
                newAttr->SetBackgroundColour( attr->GetBackgroundColour() );
            if( attr->HasTextColour() && !newAttr->HasTextColour() )
                newAttr->SetTextColour( attr->GetTextColour() );
            if( attr->HasFont() && !newAttr->HasFont() )
                newAttr->SetFont( attr->GetFont() );

            attr->DecRef();
            attr = newAttr;
        }
        else
        {
            attr = m_colAttrs[aCol]->Clone();
        }
    }
    else if( !attr )
    {
        attr = new wxGridCellAttr;
    }

    bool rowModified = false;
    bool cellModified = false;
    bool cellEmpty = true;
    bool blankModified = false;

    const wxString& fieldName = m_cols[aCol].m_fieldName;

    for( const LIB_SYMBOL* ref : m_rows[aRow].m_Refs )
    {
        LIB_DATA_ELEMENT& element = m_dataStore[ref->m_Uuid][fieldName];

        if( element.m_isModified )
            cellModified = true;

        bool elementEmpty = element.m_currentlyEmpty
                             || ( element.m_originallyEmpty && !element.m_isModified );

        if( !elementEmpty )
            cellEmpty = false;

        if( element.m_currentData.IsEmpty() && element.m_isModified )
            blankModified = true;

        if( !rowModified )
        {
            for( const LIB_DATA_MODEL_COL& col : m_cols )
            {
                if( m_dataStore[ref->m_Uuid][col.m_fieldName].m_isModified )
                {
                    rowModified = true;
                    break;
                }
            }
        }

        if( cellModified && rowModified && !cellEmpty )
            break;
    }

    // Apply striped renderer for appropriate empty cells
    if( cellEmpty && isStripeableField( aCol ) )
    {
        wxGridCellRenderer* stripedRenderer = getStripedRenderer( aCol );

        if( stripedRenderer )
        {
            attr->SetRenderer( stripedRenderer );
            attr->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

            for( const LIB_SYMBOL* ref : m_rows[aRow].m_Refs )
            {
                if( m_dataStore[ref->m_Uuid][fieldName].m_isModified )
                {
                    m_dataStore[ref->m_Uuid][fieldName].m_isStriped = true;

                    if( m_dataStore[ref->m_Uuid][fieldName].m_currentlyEmpty )
                    {
                        if( m_dataStore[ref->m_Uuid][fieldName].m_originallyEmpty )
                        {
                            attr->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
                        }
                        else if( m_dataStore[ref->m_Uuid][fieldName].m_originalData.empty() )
                        {
                            attr->SetBackgroundColour( wxColour( 180, 220, 180 ) );
                        }
                        else
                        {
                            attr->SetBackgroundColour( wxColour( 220, 180, 180 ) );
                        }
                    }
                    else if( m_dataStore[ref->m_Uuid][fieldName].m_currentData.IsEmpty() )
                    {
                        attr->SetBackgroundColour( wxColour( 180, 200, 180 ) );
                    }
                    else
                    {
                        attr->SetBackgroundColour( wxColour( 200, 180, 180 ) );
                    }
                }
            }
        }
    }
    else
    {
        bool wasStriped = false;

        for( const LIB_SYMBOL* ref : m_rows[aRow].m_Refs )
        {
            if( m_dataStore[ref->m_Uuid][fieldName].m_isStriped )
            {
                wasStriped = true;
                m_dataStore[ref->m_Uuid][fieldName].m_isStriped = false;
            }
        }

        // If the cell was previously striped, we need to reset the attribute
        if( wasStriped )
            attr = new wxGridCellAttr;

        if( rowModified )
            attr->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );

        if( blankModified )
            attr->SetBackgroundColour( wxColour( 192, 255, 192 ) );
    }

    if( cellModified )
    {
        wxFont font;

        if( attr->HasFont() )
            font = attr->GetFont();
        else if( GetView() )
            font = GetView()->GetDefaultCellFont();
        else
            font = wxFont();

        if( font.IsOk() )
        {
            font.MakeBold();
            attr->SetFont( font );
        }
    }

    return attr;
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::CreateDerivedSymbol( int aRow, int aCol, wxString& aNewSymbolName )
{
    wxCHECK_RET( aRow >= 0 && aRow < (int) m_rows.size(), "Invalid Row Number" );
    wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );

    const LIB_SYMBOL* parentSymbol = m_rows[aRow].m_Refs[0];
    const wxString& fieldName = m_cols[aCol].m_fieldName;

    std::map<wxString, LIB_DATA_ELEMENT>& parentFieldStore = m_dataStore[parentSymbol->m_Uuid];

    // Use a special field name that won't conflict with real fields
    wxString derivedSymbolFieldName = "__DERIVED_SYMBOL_" + fieldName + "__";

    // Store derived symbol creation data under special field name so ApplyData can find it
    LIB_DATA_ELEMENT& targetElement = parentFieldStore[derivedSymbolFieldName];
    targetElement.m_createDerivedSymbol = true;
    targetElement.m_derivedSymbolName = aNewSymbolName;
    targetElement.m_currentData = aNewSymbolName;
    targetElement.m_isModified = true;
    targetElement.m_originalData = parentSymbol->m_Uuid.AsString();

    wxLogTrace( traceLibFieldTable, "CreateDerivedSymbol: Parent symbol name='%s', UUID='%s'",
                parentSymbol->GetName(), parentSymbol->m_Uuid.AsString() );
    wxLogTrace( traceLibFieldTable, "CreateDerivedSymbol: Stored creation request for symbol '%s' under parent UUID %s, special field '%s'",
                aNewSymbolName, parentSymbol->m_Uuid.AsString(), derivedSymbolFieldName );

    m_edited = true;
}

void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::CreateDerivedSymbolImmediate( int aRow, int aCol, wxString& aNewSymbolName )
{
    wxCHECK_RET( aRow >= 0 && aRow < (int) m_rows.size(), "Invalid Row Number" );
    wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );

    const LIB_SYMBOL* parentSymbol = m_rows[aRow].m_Refs[0];

    wxLogTrace( traceLibFieldTable, "CreateDerivedSymbolImmediate: Creating '%s' from parent '%s' immediately",
                aNewSymbolName, parentSymbol->GetName() );

    // Generate a fresh UUID for the new derived symbol
    KIID newDerivedSymbolUuid;

    // Create the symbol immediately
    createActualDerivedSymbol( parentSymbol, aNewSymbolName, newDerivedSymbolUuid );

    // Rebuild the grid to show the new symbol
    RebuildRows();

    m_edited = true;
}

void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::createActualDerivedSymbol( const LIB_SYMBOL* aParentSymbol, const wxString& aNewSymbolName, const KIID& aNewSymbolUuid )
{
    wxLogTrace( traceLibFieldTable, "createActualDerivedSymbol: Creating '%s' from parent '%s', symbol list size before: %zu",
                aNewSymbolName, aParentSymbol->GetName(), m_symbolsList.size() );

    LIB_SYMBOL* newSymbol = nullptr;

    for( LIB_SYMBOL* sym : m_symbolsList )
    {
        if( sym->m_Uuid == aNewSymbolUuid )
        {
            newSymbol = sym;
            break;
        }
    }

    if( !newSymbol )
    {
        newSymbol = new LIB_SYMBOL( *aParentSymbol );
        newSymbol->SetName( aNewSymbolName );

        // Also update the VALUE field to reflect the new name for derived symbols
        newSymbol->GetValueField().SetText( aNewSymbolName );

        newSymbol->SetParent( const_cast<LIB_SYMBOL*>( aParentSymbol ) );
        // Note: SetLib() not called here - library association handled by dialog's library manager
        const_cast<KIID&>( newSymbol->m_Uuid ) = aNewSymbolUuid;
        m_symbolsList.push_back( newSymbol );

        wxLogTrace( traceLibFieldTable, "createActualDerivedSymbol: Added new symbol to list, size now: %zu",
                    m_symbolsList.size() );

        // Initialize field data for the new symbol in the data store
        for( const auto& col : m_cols )
        {
            updateDataStoreSymbolField( newSymbol, col.m_fieldName );
        }

        wxLogTrace( traceLibFieldTable, "createActualDerivedSymbol: Initialized field data for new symbol" );
    }

    // Note: Not adding to symbolLibrary directly - this will be handled by the dialog's library manager integration
    wxString libraryName = aParentSymbol->GetLibId().GetLibNickname();
    m_createdDerivedSymbols.emplace_back( newSymbol, libraryName );

    wxLogTrace( traceLibFieldTable, "Created derived symbol '%s' for library '%s', total tracked: %zu",
                aNewSymbolName, libraryName, m_createdDerivedSymbols.size() );
}

void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::RevertRow( int aRow )
{
    LIB_DATA_MODEL_ROW& rowGroup = m_rows[aRow];

    for( const LIB_SYMBOL* ref : rowGroup.m_Refs )
    {
        auto& fieldStore = m_dataStore[ref->m_Uuid];

        for( auto& [name, element] : fieldStore )
        {
            element.m_currentData = element.m_originalData;
            element.m_isModified = false;
            element.m_currentlyEmpty = false;
        }
    }

    m_edited = false;

    for( const auto& [symId, fieldStore] : m_dataStore )
    {
        for( const auto& [name, element] : fieldStore )
        {
            if( element.m_isModified )
            {
                m_edited = true;
                return;
            }
        }
    }
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ClearCell( int aRow, int aCol )
{
    wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), wxS( "Invalid column number" ) );

    LIB_DATA_MODEL_ROW& rowGroup = m_rows[aRow];

    for( const LIB_SYMBOL* ref : rowGroup.m_Refs )
    {
        LIB_DATA_ELEMENT& dataElement = m_dataStore[ref->m_Uuid][m_cols[aCol].m_fieldName];
        if( m_cols[aCol].m_fieldName == wxS( "Keywords" ) )
        {
            dataElement.m_currentData = wxEmptyString;
            dataElement.m_currentlyEmpty = false;
            dataElement.m_isModified = ( dataElement.m_currentData != dataElement.m_originalData );
        }
        else
        {
            dataElement.m_currentData = wxEmptyString;
            dataElement.m_currentlyEmpty = true;
            dataElement.m_isModified = ( dataElement.m_currentData != dataElement.m_originalData )
                                       || ( dataElement.m_currentlyEmpty != dataElement.m_originallyEmpty );
        }
    }

    m_edited = true;
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::cmp( const LIB_DATA_MODEL_ROW&          lhGroup,
                                             const LIB_DATA_MODEL_ROW&          rhGroup,
                                             LIB_FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol,
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
            [ ascending ]( const wxString& a, const wxString& b )
            {
                if( ascending )
                    return a < b;
                else
                    return a > b;
            };

    // Primary sort key is sortCol; secondary is always the symbol name (column 0)
    wxString lhs = dataModel->GetValue( lhGroup, sortCol ).Trim( true ).Trim( false );
    wxString rhs = dataModel->GetValue( rhGroup, sortCol ).Trim( true ).Trim( false );

    if( lhs == rhs && lhGroup.m_Refs.size() > 1 && rhGroup.m_Refs.size() > 1 )
    {
        wxString lhRef = lhGroup.m_Refs[1]->GetName();
        wxString rhRef = rhGroup.m_Refs[1]->GetName();
        return local_cmp( lhRef, rhRef );
    }
    else
    {
        return local_cmp( lhs, rhs );
    }
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::Sort()
{
    CollapseForSort();

    // We're going to sort the rows based on their first reference, so the first reference
    // had better be the lowest one.
    for( LIB_DATA_MODEL_ROW& row : m_rows )
    {
        std::sort( row.m_Refs.begin(), row.m_Refs.end(),
                   []( const LIB_SYMBOL* lhs, const LIB_SYMBOL* rhs )
                   {
                       wxString lhs_ref( lhs->GetRef( nullptr ) );
                       wxString rhs_ref( rhs->GetRef( nullptr ) );
                       return StrNumCmp( lhs_ref, rhs_ref, true ) < 0;
                   } );
    }

    std::sort( m_rows.begin(), m_rows.end(),
               [this]( const LIB_DATA_MODEL_ROW& lhs, const LIB_DATA_MODEL_ROW& rhs ) -> bool
               {
                   return cmp( lhs, rhs, this, m_sortColumn, m_sortAscending );
               } );

    // Time to renumber the item numbers
    int itemNumber = 1;

    for( LIB_DATA_MODEL_ROW& row : m_rows )
    {
        row.m_ItemNumber = itemNumber++;
    }

    ExpandAfterSort();
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::groupMatch( const LIB_SYMBOL* lhRef, const LIB_SYMBOL* rhRef )

{
    bool        matchFound = false;
    const KIID& lhRefID = lhRef->m_Uuid;
    const KIID& rhRefID = rhRef->m_Uuid;

    // Now check all the other columns.
    for( size_t i = 0; i < m_cols.size(); ++i )
    {
        const LIB_DATA_MODEL_COL& col = m_cols[i];

        if( !col.m_group )
            continue;

        if( m_dataStore[lhRefID][col.m_fieldName].m_currentData != m_dataStore[rhRefID][col.m_fieldName].m_currentData )
            return false;

        matchFound = true;
    }

    return matchFound;
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getAttributeValue( const LIB_SYMBOL* aSymbol,
                                                               const wxString&   aAttributeName )
{
    if( aAttributeName == wxS( "${DNP}" ) )
        return aSymbol->GetDNP() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return aSymbol->GetExcludedFromBoard() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        return aSymbol->GetExcludedFromBOM() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        return aSymbol->GetExcludedFromSim() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "Power" ) )
        return aSymbol->IsPower() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "LocalPower" ) )
        return aSymbol->IsLocalPower() ? wxS( "1" ) : wxS( "0" );


    return wxS( "0" );
}

void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::setAttributeValue( LIB_SYMBOL* aSymbol,
                                                           const wxString& aAttributeName,
                                                           const wxString& aValue )
{
    if( aAttributeName == wxS( "${DNP}" ) )
        aSymbol->SetDNP( aValue == wxS( "1" ) );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        aSymbol->SetExcludedFromBoard( aValue == wxS( "1" ) );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        aSymbol->SetExcludedFromBOM( aValue == wxS( "1" ) );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        aSymbol->SetExcludedFromSim( aValue == wxS( "1" ) );
    else if( aAttributeName == wxS( "LocalPower" ) )
    {
        // Turning off local power still leaves the global flag set
        if( aValue == wxS( "0" ) )
            aSymbol->SetGlobalPower();
        else
            aSymbol->SetLocalPower();
    }
    else if( aAttributeName == wxS( "Power" ) )
    {
        if( aValue == wxS( "0" ) )
            aSymbol->SetNormal();
        else
            aSymbol->SetGlobalPower();
    }
    else
        wxLogDebug( "Unknown attribute name: %s", aAttributeName );
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::RebuildRows()
{
    wxLogTrace( traceLibFieldTable, "RebuildRows: Starting rebuild with %zu symbols in list", m_symbolsList.size() );

    if( GetView() )
    {
        // Commit any pending in-place edits before the row gets moved out from under
        // the editor.
        static_cast<WX_GRID*>( GetView() )->CommitPendingChanges( true );

        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, (int) m_rows.size() );
        GetView()->ProcessTableMessage( msg );
    }

    m_rows.clear();

    wxLogTrace( traceLibFieldTable, "RebuildRows: About to process %zu symbols", m_symbolsList.size() );

    for( LIB_SYMBOL* ref : m_symbolsList )
    {
        wxLogTrace( traceLibFieldTable, "RebuildRows: Processing symbol '%s' (UUID: %s)",
                    ref->GetName(), ref->m_Uuid.AsString() );

        if( !m_filter.IsEmpty() )
        {
            bool match = false;

            std::map<wxString, LIB_DATA_ELEMENT>& fieldStore = m_dataStore[ref->m_Uuid];

            for( const LIB_DATA_MODEL_COL& col : m_cols )
            {
                auto it = fieldStore.find( col.m_fieldName );

                if( it != fieldStore.end() && WildCompareString( m_filter, it->second.m_currentData, false ) )
                {
                    match = true;
                    break;
                }
            }

            if( !match )
                continue;
        }

        bool matchFound = false;

        // Performance optimization for ungrouped case to skip the N^2 for loop
        if( !m_groupingEnabled )
        {
            m_rows.emplace_back( LIB_DATA_MODEL_ROW( ref, GROUP_SINGLETON ) );
            continue;
        }

        // See if we already have a row which this symbol fits into
        for( LIB_DATA_MODEL_ROW& row : m_rows )
        {
            // all group members must have identical refs so just use the first one
            const LIB_SYMBOL* rowRef = row.m_Refs[0];

            if( m_groupingEnabled && groupMatch( ref, rowRef ) )
            {
                matchFound = true;
                row.m_Refs.push_back( ref );
                row.m_Flag = GROUP_COLLAPSED;
                break;
            }
        }

        if( !matchFound )
            m_rows.emplace_back( LIB_DATA_MODEL_ROW( ref, GROUP_SINGLETON ) );
    }

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int) m_rows.size() );
        GetView()->ProcessTableMessage( msg );
    }

    wxLogTrace( traceLibFieldTable, "RebuildRows: Completed rebuild with %zu rows created", m_rows.size() );
    Sort();
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ExpandRow( int aRow )
{
    std::vector<LIB_DATA_MODEL_ROW> children;

    for( const LIB_SYMBOL* ref : m_rows[aRow].m_Refs )
    {
        bool matchFound = false;

        if( !matchFound )
            children.emplace_back( LIB_DATA_MODEL_ROW( ref, CHILD_ITEM ) );
    }

    if( children.size() < 2 )
        return;

    std::sort( children.begin(), children.end(),
               [this]( const LIB_DATA_MODEL_ROW& lhs, const LIB_DATA_MODEL_ROW& rhs ) -> bool
               {
                   return cmp( lhs, rhs, this, m_sortColumn, m_sortAscending );
               } );

    m_rows[aRow].m_Flag = GROUP_EXPANDED;
    m_rows.insert( m_rows.begin() + aRow + 1, children.begin(), children.end() );

    wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, aRow, (int) children.size() );
    GetView()->ProcessTableMessage( msg );
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::CollapseRow( int aRow )
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


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ExpandCollapseRow( int aRow )
{
    LIB_DATA_MODEL_ROW& group = m_rows[aRow];

    if( group.m_Flag == GROUP_COLLAPSED )
        ExpandRow( aRow );
    else if( group.m_Flag == GROUP_EXPANDED )
        CollapseRow( aRow );
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::CollapseForSort()
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


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ExpandAfterSort()
{
    for( size_t i = 0; i < m_rows.size(); ++i )
    {
        if( m_rows[i].m_Flag == GROUP_COLLAPSED_DURING_SORT )
            ExpandRow( i );
    }
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData( std::function<void( LIB_SYMBOL* )> symbolChangeHandler,
                                                   std::function<void()> postApplyHandler )
{
    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        std::map<wxString, LIB_DATA_ELEMENT>& fieldStore = m_dataStore[symbol->m_Uuid];

        for( auto& srcData : fieldStore )
        {
            const wxString&   srcName = srcData.first;
            LIB_DATA_ELEMENT& dataElement = srcData.second;
            const wxString&   srcValue = dataElement.m_currentData;
            int               col = GetFieldNameCol( srcName );

            if( dataElement.m_isModified )
                symbolChangeHandler( symbol );

            // Attributes bypass the field logic, so handle them first
            if( col != -1 && ColIsCheck( col ) )
            {
                setAttributeValue( symbol, srcName, srcValue );
                continue;
            }

            // Skip special fields with variables as names (e.g. ${QUANTITY}),
            // they can't be edited
            if( IsGeneratedField( srcName ) )
                continue;

            // Skip special derived symbol creation fields - these are handled separately
            if( srcName.StartsWith( "__DERIVED_SYMBOL_" ) && srcName.EndsWith( "__" ) )
                continue;

            if( srcName == wxS( "Keywords" ) )
            {
                symbol->SetKeyWords( srcValue );
                dataElement.m_originalData = dataElement.m_currentData;
                dataElement.m_isModified = false;
                dataElement.m_currentlyEmpty = false;
                dataElement.m_originallyEmpty = false;
                continue;
            }

            SCH_FIELD* destField = symbol->GetField( srcName );
            bool       userAdded = ( col != -1 && m_cols[col].m_userAdded );

            // Add a not existing field if it has a value for this symbol
            bool createField = !destField && ( !srcValue.IsEmpty() || userAdded );

            if( createField )
            {
                const VECTOR2I symbolPos = symbol->GetPosition();
                destField = new SCH_FIELD( symbol, FIELD_T::USER, srcName );
                destField->SetPosition( symbolPos );
                symbol->AddField( destField );
            }

            if( !destField )
                continue;

            if( destField->GetId() == FIELD_T::REFERENCE )
            {
                // Reference is not editable from this dialog
            }
            else if( destField->GetId() == FIELD_T::VALUE )
            {
                // Value field cannot be empty
                if( !srcValue.IsEmpty() )
                    symbol->GetField( FIELD_T::VALUE )->SetText( srcValue );
            }
            else if( destField->GetId() == FIELD_T::FOOTPRINT )
            {
                symbol->GetField(FIELD_T::FOOTPRINT)->SetText( srcValue );
            }
            else
            {
                destField->SetText( srcValue );
            }

            dataElement.m_originalData = dataElement.m_currentData;
            dataElement.m_isModified = false;
            dataElement.m_currentlyEmpty = false;
            dataElement.m_originallyEmpty = dataElement.m_currentlyEmpty;
        }

        std::vector<SCH_FIELD*> symbolFields;
        symbol->GetFields( symbolFields );

        // Remove any fields that are not mandatory
        for( SCH_FIELD* field : symbolFields )
        {
            if( field->IsMandatory() )
                continue;

            // Remove any fields that are not in the fieldStore
            if( !fieldStore.contains( field->GetName() ) )
            {
                symbolChangeHandler( symbol );
                symbol->RemoveField( field );
                delete field;
            }
        }
    }

    // Process derived symbol creation requests
    for( auto& [symId, fieldStore] : m_dataStore )
    {
        for( auto& [fieldName, element] : fieldStore )
        {
            if( element.m_createDerivedSymbol )
            {
                const LIB_SYMBOL* parentSymbol = nullptr;

                // First try to interpret as UUID
                try
                {
                    KIID parentUuid( element.m_originalData );

                    for( const LIB_SYMBOL* sym : m_symbolsList )
                    {
                        if( sym->m_Uuid == parentUuid )
                        {
                            parentSymbol = sym;
                            break;
                        }
                    }
                }
                catch( ... )
                {
                    // Not a valid UUID, try looking up by symbol name
                }

                // If UUID lookup failed, try looking up by symbol name
                if( !parentSymbol )
                {
                    for( const LIB_SYMBOL* sym : m_symbolsList )
                    {
                        if( sym->GetName() == element.m_originalData )
                        {
                            parentSymbol = sym;
                            break;
                        }
                    }
                }

                if( parentSymbol )
                {
                    wxString actualDerivedName = element.m_derivedSymbolName;

                    // If the derived name is the same as the parent name, auto-generate a unique name
                    if( actualDerivedName == parentSymbol->GetName() )
                    {
                        // Try common variant patterns first
                        actualDerivedName = parentSymbol->GetName() + "_1";

                        // If that exists, try incrementing the number
                        int  variant = 2;
                        bool nameExists = true;

                        while( nameExists && variant < 100 )
                        {
                            nameExists = false;

                            for( const LIB_SYMBOL* sym : m_symbolsList )
                            {
                                if( sym->GetName() == actualDerivedName )
                                {
                                    nameExists = true;
                                    break;
                                }
                            }

                            if( nameExists )
                            {
                                actualDerivedName = parentSymbol->GetName() + "_" + wxString::Format( "%d", variant );
                                variant++;
                            }
                        }
                    }

                    // Generate a fresh UUID for the new derived symbol (don't reuse symId which is for an existing symbol)
                    KIID newDerivedSymbolUuid;

                    createActualDerivedSymbol( parentSymbol, actualDerivedName, newDerivedSymbolUuid );
                }

                element.m_createDerivedSymbol = false;
                break;
            }
        }
    }

    m_edited = false;

    // Call post-apply handler if provided (for library operations and tree refresh)
    if( postApplyHandler )
        postApplyHandler();
}


int LIB_FIELDS_EDITOR_GRID_DATA_MODEL::GetDataWidth( int aCol )
{
    int      width = 0;
    wxString fieldName = GetColFieldName( aCol ); // symbol fieldName or Qty string

    for( const LIB_SYMBOL* symbol : m_symbolsList )
    {
        LIB_DATA_ELEMENT& text = m_dataStore[symbol->m_Uuid][fieldName];

        width = std::max( width, KIUI::GetTextSize( text.m_currentData, GetView() ).x );
    }

    return width;
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::GetTypeName( int row, int col )
{
    if( ColIsCheck( col ) )
        return wxGRID_VALUE_BOOL;

    return wxGridTableBase::GetTypeName( row, col );
}


wxGridCellRenderer* LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getStripedRenderer( int aCol ) const
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), nullptr );

    const wxString& fieldName = m_cols[aCol].m_fieldName;

    // Check if we already have a striped renderer for this field type
    auto it = m_stripedRenderers.find( fieldName );
    if( it != m_stripedRenderers.end() )
    {
        it->second->IncRef();
        return it->second;
    }

    wxGridCellRenderer* stripedRenderer = nullptr;
    // Default to striped string renderer
    stripedRenderer = new STRIPED_STRING_RENDERER();

    // Cache the renderer for future use - the cache owns one reference
    stripedRenderer->IncRef();
    m_stripedRenderers[fieldName] = stripedRenderer;

    // Return with IncRef for the caller (SetRenderer will consume this reference)
    stripedRenderer->IncRef();
    return stripedRenderer;
}


// lib_fields_data_model.cpp - Add the isStripeableField method
bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::isStripeableField( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );

    // Don't apply stripes to checkbox fields
    return !ColIsCheck( aCol );
}
