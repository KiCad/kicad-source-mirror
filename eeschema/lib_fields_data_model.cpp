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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <template_fieldnames.h>
#include "string_utils.h"
#include <trace_helpers.h>

#include "lib_fields_data_model.h"


const wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME = wxS( "${SYMBOL_NAME}" );
const wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_KEYWORDS = wxS( "${SYMBOL_KEYWORDS}" );
const wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_IS_POWER = wxS( "${SYMBOL_IS_POWER}" );
const wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_IS_LOCAL_POWER = wxS( "${SYMBOL_IS_LOCAL_POWER}" );


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::AddColumn( const wxString& aFieldName, const wxString& aLabel,
                                                   bool aAddedByUser )
{
    // Don't add a field twice
    if( GetFieldNameCol( aFieldName ) != -1 )
        return;

    m_cols.push_back( { aFieldName, aLabel, aAddedByUser, false, false } );

    for( LIB_SYMBOL* symbol : m_symbolsList )
        updateDataStoreItemFieldFromLive( symbol, aFieldName );
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getLiveFieldValue( LIB_SYMBOL* const& aSymbol,
                                                           const wxString& aFieldName,
                                                           wxString& aValue )
{
    int col = GetFieldNameCol( aFieldName );
    aValue.clear();

    if( col != -1 && ColIsAttribute( col ) )
    {
        aValue = getAttributeValue( aSymbol, aFieldName );
        return true;
    }
    else if( const SCH_FIELD* field = aSymbol->GetField( aFieldName ) )
    {
        aValue = field->GetText();
        return true;
    }
    else if( aFieldName == SYMBOL_KEYWORDS )
    {
        aValue = aSymbol->GetKeyWords();
        return true;
    }
    else if( aFieldName == SYMBOL_NAME )
    {
        aValue = aSymbol->GetName();
        return true;
    }

    return false;
}


std::vector<LIB_SYMBOL*> LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getAllItems() const
{
    return m_symbolsList;
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxCHECK_RET( aRow >= 0 && aRow < (int) m_rows.size(), wxS( "Invalid row number" ) );
    wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), wxS( "Invalid column number" ) );

    if( ColIsItemIdentifier( aCol ) )
        return;

    LIB_FIELDS_TABLE_DATA_MODEL_ROW& rowGroup = m_rows[aRow];
    const wxString&                  fieldName = m_cols[aCol].m_fieldName;

    for( LIB_SYMBOL* symbol : rowGroup.m_items )
    {
        std::map<wxString, wxString>& fields = m_dataStore[getDataStoreKey( symbol )];

        if( fieldName == SYMBOL_IS_POWER )
        {
            fields[fieldName] = aValue;

            if( aValue == wxS( "1" ) )
                setPowerSymbolDefaults( symbol );
        }
        else if( fieldName == SYMBOL_IS_LOCAL_POWER )
        {
            // Local Power is disabled in the symbol properties dialog until Power is enabled.
            if( getStoredPowerSymbolValue( symbol ) )
            {
                fields[fieldName] = aValue;
                setPowerSymbolDefaults( symbol );
            }
        }
        else if( getStoredPowerSymbolValue( symbol ) && isPowerSymbolControlledField( fieldName ) )
        {
            if( fieldName == GetCanonicalFieldName( FIELD_T::VALUE ) )
                fields[fieldName] = symbol->GetName();
            else
                fields[fieldName] = wxS( "1" );
        }
        else
        {
            fields[fieldName] = aValue;
        }
    }

    m_edited = true;
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ColIsItemIdentifier( int aCol )
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME;
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::fieldIsAttribute( const wxString& aFieldName ) const
{
    return FIELDS_TABLE_DATA_MODEL_BASE::fieldIsAttribute( aFieldName ) || aFieldName == SYMBOL_IS_POWER
           || aFieldName == SYMBOL_IS_LOCAL_POWER;
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::IsCellReadOnly( int aRow, int aCol )
{
    if( FIELDS_TABLE_DATA_MODEL<LIB_SYMBOL*>::IsCellReadOnly( aRow, aCol ) )
        return true;

    if( m_cols[aCol].m_fieldName == SYMBOL_IS_LOCAL_POWER )
    {
        for( LIB_SYMBOL* symbol : m_rows[aRow].m_items )
        {
            if( getStoredPowerSymbolValue( symbol ) )
                return false;
        }

        return true;
    }

    if( !isPowerSymbolControlledField( m_cols[aCol].m_fieldName ) )
        return false;

    for( LIB_SYMBOL* symbol : m_rows[aRow].m_items )
    {
        if( !getStoredPowerSymbolValue( symbol ) )
            return false;
    }

    return true;
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

    if( IsCellReadOnly( aRow, aCol ) )
        attr->SetReadOnly();

    bool rowModified = false;
    bool cellModified = false;
    bool cellEmpty = true;
    bool blankModified = false;

    const wxString& fieldName = m_cols[aCol].m_fieldName;

    for( LIB_SYMBOL* symbol : m_rows[aRow].m_items )
    {
        wxString originalValue;
        wxString currentValue;
        bool     originallyEmpty = !getLiveFieldValue( symbol, fieldName, originalValue );
        bool     currentlyEmpty = !getStoredFieldValue( symbol, fieldName, currentValue );
        bool     modified = originallyEmpty != currentlyEmpty || originalValue != currentValue;

        if( modified )
            cellModified = true;

        bool elementEmpty = currentlyEmpty || ( originallyEmpty && !modified );

        if( !elementEmpty )
            cellEmpty = false;

        if( currentValue.IsEmpty() && modified )
            blankModified = true;

        if( !rowModified )
        {
            for( const DATA_MODEL_COL& col : m_cols )
            {
                if( fieldIsModified( symbol, col.m_fieldName ) )
                {
                    rowModified = true;
                    break;
                }
            }
        }

        if( cellModified && rowModified && !cellEmpty )
            break;
    }

    if( cellEmpty && !ColIsAttribute( aCol ) )
    {
        attr->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

        for( LIB_SYMBOL* symbol : m_rows[aRow].m_items )
        {
            wxString originalValue;
            wxString currentValue;
            bool     originallyEmpty = !getLiveFieldValue( symbol, fieldName, originalValue );
            bool     currentlyEmpty = !getStoredFieldValue( symbol, fieldName, currentValue );
            bool     modified = originallyEmpty != currentlyEmpty || originalValue != currentValue;

            if( modified )
            {
                if( currentlyEmpty )
                {
                    if( originallyEmpty )
                    {
                        attr->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
                    }
                    else if( originalValue.empty() )
                    {
                        attr->SetBackgroundColour( wxColour( 180, 220, 180 ) );
                    }
                    else
                    {
                        attr->SetBackgroundColour( wxColour( 220, 180, 180 ) );
                    }
                }
                else if( currentValue.IsEmpty() )
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
    else
    {
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

    return applyFieldPresenceRenderer( attr, aRow, aCol );
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::CreateDerivedSymbolImmediate( int aRow, int aCol,
                                                                      wxString& aNewSymbolName )
{
    wxCHECK_RET( aRow >= 0 && aRow < (int) m_rows.size(), "Invalid Row Number" );
    wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );

    const LIB_SYMBOL* parentSymbol = m_rows[aRow].m_items[0];

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
            updateDataStoreItemFieldFromLive( newSymbol, col.m_fieldName );
        }

        wxLogTrace( traceLibFieldTable, "createActualDerivedSymbol: Initialized field data for new symbol" );
    }

    // Note: Not adding to symbolLibrary directly - this will be handled by the dialog's library manager integration
    wxString libraryName = aParentSymbol->GetLibId().GetLibNickname();
    m_createdDerivedSymbols.emplace_back( newSymbol, libraryName );

    wxLogTrace( traceLibFieldTable, "Created derived symbol '%s' for library '%s', total tracked: %zu",
                aNewSymbolName, libraryName, m_createdDerivedSymbols.size() );
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

    if( aAttributeName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
        return aSymbol->GetExcludedFromPosFiles() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == SYMBOL_IS_POWER )
        return aSymbol->IsPower() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == SYMBOL_IS_LOCAL_POWER )
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
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
        aSymbol->SetExcludedFromPosFiles( aValue == wxS( "1" ) );
    else
        wxLogDebug( "Unknown attribute name: %s", aAttributeName );
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::isPowerSymbolControlledField( const wxString& aFieldName ) const
{
    return aFieldName == GetCanonicalFieldName( FIELD_T::VALUE )
           || aFieldName == wxS( "${EXCLUDE_FROM_BOARD}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_BOM}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_POS_FILES}" )
           || aFieldName == wxS( "${EXCLUDE_FROM_SIM}" );
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getStoredPowerSymbolValue( LIB_SYMBOL* aSymbol ) const
{
    wxString value;

    if( getStoredFieldValue( aSymbol, SYMBOL_IS_POWER, value ) )
        return value == wxS( "1" );

    return GetFieldNameCol( SYMBOL_IS_POWER ) == -1 && aSymbol->IsPower();
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::setPowerSymbolDefaults( LIB_SYMBOL* aSymbol )
{
    std::map<wxString, wxString>& fields = m_dataStore[getDataStoreKey( aSymbol )];

    fields[GetCanonicalFieldName( FIELD_T::VALUE )] = aSymbol->GetName();

    static const std::vector<wxString> exclusionFields = {
        wxS( "${EXCLUDE_FROM_BOARD}" ),
        wxS( "${EXCLUDE_FROM_BOM}" ),
        wxS( "${EXCLUDE_FROM_POS_FILES}" ),
        wxS( "${EXCLUDE_FROM_SIM}" ),
    };

    for( const wxString& fieldName : exclusionFields )
    {
        if( GetFieldNameCol( fieldName ) != -1 )
            fields[fieldName] = wxS( "1" );
    }
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getAttributeResolvedValue( const wxString& aFieldName, bool aValue ) const
{
    if( !aValue )
        return wxEmptyString;

    if( aFieldName == SYMBOL_IS_POWER )
        return wxS( "Power Symbol" );
    else if( aFieldName == SYMBOL_IS_LOCAL_POWER )
        return wxS( "Local Power Symbol" );

    return FIELDS_TABLE_DATA_MODEL_BASE::getAttributeResolvedValue( aFieldName, aValue );
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::RebuildRows()
{
    if( !m_rebuildsEnabled )
        return;

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

    EDA_COMBINED_MATCHER matcher( m_filter.Lower(), CTX_SEARCH );

    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        wxLogTrace( traceLibFieldTable, "RebuildRows: Processing symbol '%s' (UUID: %s)",
                    symbol->GetName(), symbol->m_Uuid.AsString() );

        if( m_scope == SCOPE::SCOPE_RELATED_SYMBOLS )
        {
            std::shared_ptr<LIB_SYMBOL> root = symbol->GetRootSymbol();

            if( !root || root->GetName() != m_relatedSymbolRoot )
                continue;
        }

        if( !MatchesFilter( symbol, symbol->GetName(), matcher ) )
            continue;

        if( m_excludeDNP && symbol->GetDNP() )
            continue;

        if( !m_includeExcluded && symbol->GetExcludedFromBOM() )
            continue;

        bool matchFound = false;

        // Performance optimization for ungrouped case to skip the N^2 for loop
        if( !m_groupingEnabled )
        {
            m_rows.emplace_back( LIB_FIELDS_TABLE_DATA_MODEL_ROW( symbol, ROW_STATE::NON_EXPANDABLE ) );
            continue;
        }

        // See if we already have a row which this symbol fits into
        for( LIB_FIELDS_TABLE_DATA_MODEL_ROW& row : m_rows )
        {
            // all group members must have identical refs so just use the first one
            LIB_SYMBOL* rowSymbol = row.m_items[0];

            if( groupMatch( symbol, rowSymbol ) )
            {
                matchFound = true;
                row.m_items.push_back( symbol );
                row.m_state = ROW_STATE::COLLAPSED;
                break;
            }
        }

        if( !matchFound )
            m_rows.emplace_back( LIB_FIELDS_TABLE_DATA_MODEL_ROW( symbol, ROW_STATE::NON_EXPANDABLE ) );
    }

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int) m_rows.size() );
        GetView()->ProcessTableMessage( msg );
    }

    wxLogTrace( traceLibFieldTable, "RebuildRows: Completed rebuild with %zu rows created", m_rows.size() );
    Sort();
}


bool LIB_FIELDS_EDITOR_GRID_DATA_MODEL::unitMatch( LIB_SYMBOL* const& lhItem, LIB_SYMBOL* const& rhItem )
{
    return lhItem == rhItem;
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getFieldResolvedLiveValue( LIB_SYMBOL* const& aRef,
                                                                       const wxString&    aFieldName )
{
    SCH_FIELD* field = aRef->GetField( aFieldName );

    if( field )
    {
        if( field->IsPrivate() )
            return wxEmptyString;
        else
            return field->GetShownText( nullptr, false, 0 );
    }

    // Handle generated fields with variables as names (e.g. ${QUANTITY}) that are not present in
    // the symbol by giving them the correct value by resolving against the symbol
    if( IsGeneratedField( aFieldName ) )
    {
        int depth = 0;

        std::function<bool( wxString* )> libSymbolResolver = [&]( wxString* token ) -> bool
        {
            return aRef->ResolveTextVar( token, depth + 1 );
        };

        return ResolveTextVars( aFieldName, &libSymbolResolver, depth );
    }

    return wxEmptyString;
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::resolveTextVars( LIB_SYMBOL* const& aLibSymbol, const wxString& aText )
{
    // TODO: this isn't technically correct, this should resolve against the
    // data store's copy of variables whenever whenever possible,
    // but currently it is resolving against the symbol's current values.
    // For instance, if you have "My value is ${VALUE}" in the description field,
    // ${VALUE} will be resolved against the symbol's live value, not the Value field
    // stored in the data store.
    std::function<bool( wxString* )> libSymbolResolver = [&]( wxString* token ) -> bool
    {
        return aLibSymbol->ResolveTextVar( token );
    };

    int depth = 0;
    return ResolveTextVars( aText, &libSymbolResolver, depth );
}


void LIB_FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData( std::function<void( LIB_SYMBOL* )> symbolChangeHandler,
                                                   std::function<void()> postApplyHandler )
{
    int powerCol = GetFieldNameCol( SYMBOL_IS_POWER );
    int localPowerCol = GetFieldNameCol( SYMBOL_IS_LOCAL_POWER );

    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        bool symbolModified = false;
        bool symbolIsPower = symbol->IsPower();
        bool symbolIsLocalPower = symbol->IsLocalPower();
        wxString value;

        if( powerCol != -1 )
            symbolIsPower = getStoredFieldValue( symbol, SYMBOL_IS_POWER, value ) && value == wxS( "1" );

        if( localPowerCol != -1 )
        {
            symbolIsLocalPower = getStoredFieldValue( symbol, SYMBOL_IS_LOCAL_POWER, value )
                                 && value == wxS( "1" );
        }

        // Local power is a subtype of power, so apply the two checkbox columns as one
        // three-state property rather than allowing column order to determine the result.
        if( powerCol != -1 && !symbolIsPower )
            symbolIsLocalPower = false;
        else if( symbolIsLocalPower )
            symbolIsPower = true;

        if( symbol->IsPower() != symbolIsPower || symbol->IsLocalPower() != symbolIsLocalPower )
        {
            if( symbolIsLocalPower )
                symbol->SetLocalPower();
            else if( symbolIsPower )
                symbol->SetGlobalPower();
            else
                symbol->SetNormal();

            symbolModified = true;
        }

        for( size_t i = 0; i < m_cols.size(); ++i )
        {
            const DATA_MODEL_COL& col = m_cols[i];
            const wxString&       srcName = col.m_fieldName;
            wxString              srcValue;
            bool                  currentlyPresent = getStoredFieldValue( symbol, srcName, srcValue );

            if( srcName == SYMBOL_NAME )
                continue;

            if( srcName == SYMBOL_IS_POWER || srcName == SYMBOL_IS_LOCAL_POWER )
                continue;

            // Attributes bypass the field logic, so handle them first
            if( ColIsAttribute( static_cast<int>( i ) ) )
            {
                wxString newValue = currentlyPresent ? srcValue : wxS( "0" );

                if( getAttributeValue( symbol, srcName ) != newValue )
                {
                    setAttributeValue( symbol, srcName, newValue );
                    symbolModified = true;
                }

                continue;
            }

            if( srcName == SYMBOL_KEYWORDS )
            {
                if( symbol->GetKeyWords() != srcValue )
                {
                    symbol->SetKeyWords( srcValue );
                    symbolModified = true;
                }

                continue;
            }

            // Skip special fields with variables as names (e.g. ${QUANTITY}),
            // they can't be edited
            if( IsGeneratedField( srcName ) )
                continue;

            SCH_FIELD* destField = symbol->GetField( srcName );

            if( !currentlyPresent && destField )
            {
                if( destField->IsMandatory() )
                {
                    // Value cannot be empty, but the other mandatory fields can be cleared.
                    if( destField->GetId() != FIELD_T::VALUE && !destField->GetText().IsEmpty() )
                    {
                        destField->SetText( wxEmptyString );
                        symbolModified = true;
                    }
                }
                else if( !destField->IsPrivate() )
                {
                    symbol->RemoveField( destField );
                    symbolModified = true;
                }

                continue;
            }

            // User-added columns are instantiated on every symbol.  For other columns, preserve
            // an explicitly edited empty value as a present-but-empty field.
            bool createField = !destField && ( currentlyPresent || col.m_userAdded );

            if( createField )
            {
                const VECTOR2I symbolPos = symbol->GetPosition();
                destField = new SCH_FIELD( symbol, FIELD_T::USER, srcName );
                destField->SetPosition( symbolPos );
                symbol->AddField( destField );
                symbolModified = true;
            }

            if( !destField )
                continue;

            if( destField->GetId() == FIELD_T::VALUE )
            {
                // Value field cannot be empty
                wxString newValue = symbolIsPower ? symbol->GetName() : srcValue;

                if( !newValue.IsEmpty() && destField->GetText() != newValue )
                {
                    symbol->GetField( FIELD_T::VALUE )->SetText( newValue );
                    symbolModified = true;
                }
            }
            else if( destField->GetText() != srcValue )
            {
                destField->SetText( srcValue );
                symbolModified = true;
            }
        }

        std::vector<SCH_FIELD*> symbolFields;
        symbol->GetFields( symbolFields );

        // Remove any fields that are not mandatory
        for( SCH_FIELD* field : symbolFields )
        {
            if( field->IsMandatory() || field->IsPrivate() )
                continue;

            const wxString& existingName = field->GetName();

            // Entries for fields whose columns still exist were handled above.  Anything left here
            // has had its column removed from the table.
            if( GetFieldNameCol( existingName ) == -1 )
            {
                symbol->RemoveField( field );
                symbolModified = true;
            }
        }

        if( symbolModified )
            symbolChangeHandler( symbol );

        for( const DATA_MODEL_COL& col : m_cols )
            updateDataStoreItemFieldFromLive( symbol, col.m_fieldName );
    }

    m_edited = false;

    // Call post-apply handler if provided (for library operations and tree refresh)
    if( postApplyHandler )
        postApplyHandler();
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::GetTypeName( int row, int col )
{
    if( ColIsAttribute( col ) )
        return wxGRID_VALUE_BOOL;

    return wxGridTableBase::GetTypeName( row, col );
}


KIID_PATH LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getDataStoreKey( LIB_SYMBOL* const& aItem ) const
{
    KIID_PATH key;
    key.push_back( aItem->m_Uuid );
    return key;
}


wxString LIB_FIELDS_EDITOR_GRID_DATA_MODEL::getItemIdentifier( LIB_SYMBOL* const& aItem ) const
{
    return aItem->GetName();
}
