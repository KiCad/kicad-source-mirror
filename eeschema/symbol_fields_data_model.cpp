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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <set>

#include <wx/string.h>
#include <wx/debug.h>
#include <wx/grid.h>
#include <wx/settings.h>
#include <common.h>
#include <widgets/wx_grid.h>
#include <sch_reference_list.h>
#include <sch_commit.h>
#include <sch_screen.h>
#include <template_fieldnames.h>
#include <sch_sheet_path.h>
#include "string_utils.h"

#include <symbol_fields_data_model.h>


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::AddColumn( const wxString& aFieldName, const wxString& aLabel,
                                                      bool aAddedByUser )
{
    // Don't add a field twice
    if( GetFieldNameCol( aFieldName ) != -1 )
        return;

    m_cols.push_back( { aFieldName, aLabel, aAddedByUser, false, false } );

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        if( m_symbolsList[i].GetSymbol() )
            updateDataStoreItemFieldFromLive( m_symbolsList[i], aFieldName );
    }
}


/**
 * Create a unique key for the data store by combining the #KIID_PATH from the
 * #SCH_SHEET_PATH with the symbol's UUID.
 *
 * @return A KIID_PATH representing the full #SCH_SHEET_PATH + symbol UUID.
 */
KIID_PATH SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getDataStoreKey( const SCH_REFERENCE& aItem ) const
{
    KIID_PATH path = aItem.GetSheetPath().Path();
    path.push_back( aItem.GetSymbol()->m_Uuid );
    return path;
}


wxString SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getItemIdentifier( const SCH_REFERENCE& aItem ) const
{
    return aItem.GetRef() + aItem.GetRefNumber();
}


wxGridCellAttr* SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    wxGridCellAttr* attr = nullptr;
    wxString        rawValue = GetGroupedValue( m_rows[aRow], aCol );
    bool            needsReadOnly = isCellReadOnly( aRow, aCol );
    bool            needsUrlEditor = false;
    bool            needsVariantHighlight = false;
    bool            needsTextVarRenderer = false;
    wxColour        highlightColor;

    // Check if we need URL editor
    if( GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::DATASHEET )
        || IsURL( rawValue ) )
    {
        if( m_urlEditor )
            needsUrlEditor = true;
    }

    // Check if the raw value contains a text variable that should be resolved for display
    if( aRow >= 0 && aRow < (int) m_rows.size() && aCol >= 0 && aCol < (int) m_cols.size() && !ColIsReference( aCol )
        && !ColIsQuantity( aCol ) && !ColIsItemNumber( aCol ) )
    {
        if( IsGeneratedValue( rawValue ) )
            needsTextVarRenderer = true;
    }

    // Check if we need variant highlighting
    if( !m_currentVariant.IsEmpty() && aRow >= 0 && aRow < (int) m_rows.size() && aCol >= 0
        && aCol < (int) m_cols.size() )
    {
        const wxString& fieldName = m_cols[aCol].m_fieldName;

        // Skip Reference and generated fields (like ${QUANTITY}) for highlighting
        if( !ColIsReference( aCol ) && !ColIsQuantity( aCol ) && !ColIsItemNumber( aCol ) )
        {
            const SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& row = m_rows[aRow];

            // Check if any symbol in this row has a variant-specific value
            for( const SCH_REFERENCE& ref : row.m_items )
            {
                wxString defaultValue = getDefaultFieldValue( ref, fieldName );

                // Get the current value from the data store
                wxString currentValue;

                if( ref.GetSymbol() )
                {
                    KIID_PATH key = getDataStoreKey( ref );

                    if( m_dataStore.contains( key ) && m_dataStore[key].contains( fieldName ) )
                        currentValue = m_dataStore[key][fieldName];
                }

                if( currentValue != defaultValue )
                {
                    needsVariantHighlight = true;

                    bool isPriority2 = false;

                    if( const SCH_SYMBOL* sym = ref.GetSymbol() )
                    {
                        auto variantData = sym->GetVariant( ref.GetSheetPath(),
                                                            m_currentVariant );

                        if( variantData
                            && variantData->m_SymbolOverride
                            && !variantData->m_Fields.count( fieldName ) )
                        {
                            isPriority2 = true;
                        }
                    }

                    wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
                    bool     isDark = ( bg.Red() + bg.Green() + bg.Blue() ) < 384;

                    if( isPriority2 )
                    {
                        highlightColor = isDark ? wxColour( 40, 60, 80 )
                                                : wxColour( 220, 235, 255 );
                    }
                    else
                    {
                        highlightColor = isDark ? wxColour( 80, 80, 40 )
                                                : wxColour( 255, 255, 200 );
                    }

                    break;
                }
            }
        }
    }

    // If we don't need any custom attributes, use the base class behavior
    if( !needsReadOnly && !needsUrlEditor && !needsVariantHighlight && !needsTextVarRenderer )
        return WX_GRID_TABLE_BASE::GetAttr( aRow, aCol, aKind );

    // URL cells: use m_urlEditor as base, potentially with read-only or variant overlays
    if( needsUrlEditor )
    {
        if( needsReadOnly || needsVariantHighlight )
        {
            attr = m_urlEditor->Clone();

            if( needsReadOnly )
                attr->SetReadOnly();

            if( needsVariantHighlight )
                attr->SetBackgroundColour( highlightColor );
        }
        else
        {
            // Just use the URL editor attribute directly
            m_urlEditor->IncRef();
            attr = m_urlEditor;
        }

        return enhanceAttr( attr, aRow, aCol, aKind );
    }

    // Non-URL cells: start with column attributes if they exist.
    // This preserves checkbox renderers and other column-specific settings.
    if( m_colAttrs.find( aCol ) != m_colAttrs.end() && m_colAttrs[aCol] )
    {
        attr = m_colAttrs[aCol]->Clone();
    }
    else
    {
        attr = new wxGridCellAttr();
    }

    if( needsReadOnly )
        attr->SetReadOnly();

    if( needsVariantHighlight )
        attr->SetBackgroundColour( highlightColor );

    if( needsTextVarRenderer )
    {
        if( !m_textVarRenderer )
            m_textVarRenderer = new GRID_CELL_RESOLVED_TEXT_RENDERER();

        m_textVarRenderer->IncRef();
        attr->SetRenderer( m_textVarRenderer );

        // Tint text-var cells if not already highlighted by variant
        if( !needsVariantHighlight )
        {
            wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
            bool     isDark = ( bg.Red() + bg.Green() + bg.Blue() ) < 384;

            attr->SetBackgroundColour( isDark ? wxColour( 80, 70, 30 )       // Dark amber
                                              : wxColour( 255, 252, 200 ) ); // Light yellow
        }
    }

    return enhanceAttr( attr, aRow, aCol, aKind );
}


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxS( "Invalid column number" ) );

    // Can't modify references or generated fields (e.g. ${QUANTITY})
    if( ColIsReference( aCol )
        || ( IsGeneratedField( m_cols[aCol].m_fieldName ) && !ColIsAttribute( aCol ) ) )
    {
        return;
    }

    if( aValue == INDETERMINATE_STATE )
        return;

    const SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& row = m_rows[aRow];
    const wxString&                           fieldName = m_cols[aCol].m_fieldName;

    std::set<const SCH_SYMBOL*> editedSymbols;

    for( const SCH_REFERENCE& ref : row.m_items )
    {
        editedSymbols.insert( ref.GetSymbol() );
        m_dataStore[getDataStoreKey( ref )][fieldName] = aValue;
    }

    // ApplyData walks every path a symbol is reachable through, so an edit to storage those
    // paths have in common must also reach the ones the current scope and filter hide
    if( storageIsSharedAcrossPaths( fieldName ) )
    {
        for( unsigned ii = 0; ii < m_symbolsList.GetCount(); ++ii )
        {
            const SCH_REFERENCE& ref = m_symbolsList[ii];

            if( !editedSymbols.contains( ref.GetSymbol() ) )
                continue;

            m_dataStore[getDataStoreKey( ref )][fieldName] = aValue;
        }
    }

    m_edited = true;
}


bool SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::unitMatch( const SCH_REFERENCE& lhItem, const SCH_REFERENCE& rhItem )
{
    // If items are unannotated then we can't tell if they're units of the same symbol or not
    if( lhItem.GetRefNumber() == wxT( "?" ) )
        return false;

    return ( lhItem.GetRef() == rhItem.GetRef() && lhItem.GetRefNumber() == rhItem.GetRefNumber() );
}


bool SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getLiveFieldValue( const SCH_REFERENCE& aRef,
                                                              const wxString& aFieldName,
                                                              wxString& aValue )
{
    aValue.clear();

    if( !aRef.GetSymbol() )
        return false;

    aValue = getFieldValueForVariant( aRef, aFieldName, m_currentVariant );
    return true;
}


std::vector<SCH_REFERENCE> SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getAllItems() const
{
    std::vector<SCH_REFERENCE> items;

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        if( m_symbolsList[i].GetSymbol() )
            items.push_back( m_symbolsList[i] );
    }

    return items;
}


wxString SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getFieldResolvedLiveValue( const SCH_REFERENCE& aRef,
                                                                          const wxString&      aFieldName )
{
    SCH_FIELD* field = aRef.GetSymbol()->GetField( aFieldName );

    if( field )
    {
        if( field->IsPrivate() )
            return wxEmptyString;
        else
            return field->GetShownText( &aRef.GetSheetPath(), false, 0, m_currentVariant );
    }

    // Handle generated fields with variables as names (e.g. ${QUANTITY}) that are not present in
    // the symbol by giving them the correct value by resolving against the symbol
    if( IsGeneratedField( aFieldName ) )
    {
        int                   depth = 0;
        const SCH_SHEET_PATH& path = aRef.GetSheetPath();

        std::function<bool( wxString* )> symbolResolver = [&]( wxString* token ) -> bool
        {
            return aRef.GetSymbol()->ResolveTextVar( &path, token, m_currentVariant, depth + 1 );
        };

        return ResolveTextVars( aFieldName, &symbolResolver, depth );
    }

    return wxEmptyString;
}


wxString SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::resolveTextVars( const SCH_REFERENCE& aRef, const wxString& aText )
{
    // TODO: this isn't technically correct, this should resolve against the
    // data store's copy of variables whenever whenever possible,
    // but currently it is resolving against the symbol's current values.
    // For instance, if you have "My value is ${VALUE}" in the description field,
    // ${VALUE} will be resolved against the symbol's live value, not the Value field
    // stored in the data store.
    std::function<bool( wxString* )> symbolResolver = [&]( wxString* token ) -> bool
    {
        return aRef.GetSymbol()->ResolveTextVar( &aRef.GetSheetPath(), token, m_currentVariant );
    };

    int depth = 0;
    return ResolveTextVars( aText, &symbolResolver, depth );
}


bool SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::storageIsSharedAcrossPaths( const wxString& aFieldName ) const
{
    // Variant edits are kept on the symbol instance, but SCH_REFERENCE has no variant form of
    // the board exclusion so that one always lands on the symbol
    if( aFieldName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return true;

    return m_currentVariant.IsEmpty();
}


wxString SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getAttributeValue( const SCH_REFERENCE& aRef,
                                                                  const wxString&      aAttributeName,
                                                                  const wxString&      aVariantName )
{
    if( aAttributeName == wxS( "${DNP}" ) )
        return aRef.GetSymbolDNP( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return aRef.GetSymbolExcludedFromBoard() ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        return aRef.GetSymbolExcludedFromBOM( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        return aRef.GetSymbolExcludedFromSim( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
        return aRef.GetSymbolExcludedFromPosFiles( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    return wxS( "0" );
}


bool SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::attributeForcedOnBySheet( const SCH_REFERENCE& aRef,
                                                                     const wxString&      aAttributeName ) const
{
    const SCH_SHEET_PATH& path = aRef.GetSheetPath();

    if( aAttributeName == wxS( "${DNP}" ) )
        return path.GetDNP( m_currentVariant );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return path.GetExcludedFromBoard( m_currentVariant );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        return path.GetExcludedFromBOM( m_currentVariant );
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        return path.GetExcludedFromSim( m_currentVariant );

    return false;
}


wxString SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getFieldValueForVariant( const SCH_REFERENCE& aRef,
                                                                        const wxString&      aFieldName,
                                                                        const wxString&      aVariantName )
{
    const SCH_SYMBOL* symbol = aRef.GetSymbol();

    if( !symbol )
        return wxEmptyString;

    if( fieldIsAttribute( aFieldName ) )
        return getAttributeValue( aRef, aFieldName, aVariantName );

    if( const SCH_FIELD* field = symbol->GetField( aFieldName ) )
    {
        if( field->IsPrivate() )
            return wxEmptyString;

        return symbol->Schematic()->ConvertKIIDsToRefs( field->GetText( &aRef.GetSheetPath(), aVariantName ) );
    }

    // For generated fields, return the field name itself
    if( IsGeneratedField( aFieldName ) )
        return aFieldName;

    return wxEmptyString;
}


wxString SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::getDefaultFieldValue( const SCH_REFERENCE& aRef,
                                                                     const wxString&      aFieldName )
{
    return getFieldValueForVariant( aRef, aFieldName, wxEmptyString );
}


bool SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::setAttributeValue( SCH_REFERENCE&  aRef,
                                                       const wxString& aAttributeName,
                                                       const wxString& aValue,
                                                       const wxString& aVariantName )
{
    bool attrChanged = false;
    bool newValue = aValue == wxS( "1" );

    if( aAttributeName == wxS( "${DNP}" ) )
    {
        attrChanged = aRef.GetSymbolDNP( aVariantName ) != newValue;

        if( attrChanged )
            aRef.SetSymbolDNP( newValue, aVariantName );
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
    {
        attrChanged = aRef.GetSymbolExcludedFromBoard() != newValue;

        if( attrChanged )
            aRef.SetSymbolExcludedFromBoard( newValue );
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
    {
        attrChanged = aRef.GetSymbolExcludedFromBOM( aVariantName ) != newValue;

        if( attrChanged )
            aRef.SetSymbolExcludedFromBOM( newValue, aVariantName );
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
    {
        attrChanged = aRef.GetSymbolExcludedFromSim( aVariantName ) != newValue;

        if( attrChanged )
            aRef.SetSymbolExcludedFromSim( newValue, aVariantName );
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
    {
        attrChanged = aRef.GetSymbolExcludedFromPosFiles( aVariantName ) != newValue;

        if( attrChanged )
            aRef.SetSymbolExcludedFromPosFiles( newValue, aVariantName );
    }

    return attrChanged;
}


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::RebuildRows()
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

        if( m_scope == SCOPE::SCOPE_SELECTION
            && !m_selectionItems.contains( getDataStoreKey( ref ) ) )
        {
            continue;
        }

        if( !MatchesFilter( ref, ref.GetFullRef(), matcher ) )
            continue;

        if( m_excludeDNP )
        {
            bool isDNP = false;

            if( !m_variantNames.empty() )
            {
                for( const wxString& variantName : m_variantNames )
                {
                    if( ref.GetSymbol()->ResolveDNP( &ref.GetSheetPath(), variantName )
                        || ref.GetSheetPath().GetDNP( variantName ) )
                    {
                        isDNP = true;
                        break;
                    }
                }
            }
            else
            {
                isDNP = ref.GetSymbol()->ResolveDNP( &ref.GetSheetPath(), m_currentVariant )
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
                    if( ref.GetSymbol()->ResolveExcludedFromBOM( &ref.GetSheetPath(), variantName )
                        || ref.GetSheetPath().GetExcludedFromBOM( variantName ) )
                    {
                        isExcluded = true;
                        break;
                    }
                }
            }
            else
            {
                isExcluded = ref.GetSymbol()->ResolveExcludedFromBOM( &ref.GetSheetPath(), m_currentVariant )
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
            m_rows.emplace_back( SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW( ref, ROW_STATE::NON_EXPANDABLE ) );
            continue;
        }

        // See if we already have a row which this symbol fits into
        for( SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& row : m_rows )
        {
            // all group members must have identical refs so just use the first one
            SCH_REFERENCE rowRef = row.m_items[0];

            if( unitMatch( ref, rowRef ) )
            {
                matchFound = true;
                row.m_items.push_back( ref );
                break;
            }
            else if( m_groupingEnabled && groupMatch( ref, rowRef ) )
            {
                matchFound = true;
                row.m_items.push_back( ref );
                row.m_state = ROW_STATE::COLLAPSED;
                break;
            }
        }

        if( !matchFound )
            m_rows.emplace_back( SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW( ref, ROW_STATE::NON_EXPANDABLE ) );
    }

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
        GetView()->ProcessTableMessage( msg );
    }

    Sort();
}


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData( SCH_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames,
                                                      const wxString& aVariantName )
{
    bool                        symbolModified = false;
    std::unique_ptr<SCH_SYMBOL> symbolCopy;

    for( size_t i = 0; i < m_symbolsList.GetCount(); i++ )
    {
        SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol();
        SCH_SYMBOL* nextSymbol = nullptr;

        if( ( i + 1 ) < m_symbolsList.GetCount() )
            nextSymbol = m_symbolsList[i + 1].GetSymbol();

        if( i == 0 )
            symbolCopy = std::make_unique<SCH_SYMBOL>( *symbol );

        KIID_PATH                           key = getDataStoreKey( m_symbolsList[i] );
        const std::map<wxString, wxString>& fieldStore = m_dataStore[key];

        for( const auto& [srcName, srcValue] : fieldStore )
        {
            // Attributes bypass the field logic, so handle them first
            if( fieldIsAttribute( srcName ) )
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

            const wxString& existingName = symbol->GetFields()[ii].GetName();

            bool stillTracked = std::any_of( fieldStore.begin(), fieldStore.end(),
                                             [&]( const auto& kv )
                                             {
                                                 return kv.first == existingName;
                                             } );

            if( !stillTracked )
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


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::AddReferences( const SCH_REFERENCE_LIST& aRefs )
{
    bool refListChanged = false;

    for( const SCH_REFERENCE& ref : aRefs )
    {
        if( !m_symbolsList.Contains( ref ) )
        {
            SCH_SYMBOL* symbol = ref.GetSymbol();

            m_symbolsList.AddItem( ref );

            KIID_PATH key = getDataStoreKey( ref );

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

            for( const DATA_MODEL_COL& col : m_cols )
                m_dataStore[key].try_emplace( col.m_fieldName, wxEmptyString );

            refListChanged = true;
        }
    }

    if( refListChanged )
        m_symbolsList.SortBySymbolPtr();
}


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::RemoveSymbol( const SCH_SYMBOL& aSymbol )
{
    // The schematic event listener passes us the symbol after it has been removed,
    // so we can't just work with a SCH_REFERENCE_LIST like the other handlers as the
    // references are already gone. Instead we need to prune our list.

    // Since we now use full KIID_PATH as keys, we need to find and remove all entries
    // that correspond to this symbol (their keys end with the symbol's UUID)
    KIID                   symbolUuid = aSymbol.m_Uuid;
    std::vector<KIID_PATH> keysToRemove;

    for( const auto& [key, value] : m_dataStore )
    {
        if( !key.empty() && ( key.back() == symbolUuid ) )
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


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::RemoveReferences( const SCH_REFERENCE_LIST& aRefs )
{
    for( const SCH_REFERENCE& ref : aRefs )
    {
        int index = m_symbolsList.FindRefByFullPath( ref.GetFullPath() );

        if( index != -1 )
        {
            m_dataStore.erase( getDataStoreKey( ref ) );
            m_symbolsList.RemoveItem( index );
        }
    }
}


void SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::UpdateReferences( const SCH_REFERENCE_LIST& aRefs )
{
    bool refListChanged = false;

    for( const SCH_REFERENCE& ref : aRefs )
    {
        // Update the fields of every reference. Do this by iterating through the data model
        // columns; we must have all fields in the symbol added to the data model at this point,
        // and some of the data model columns may be variables that are not present in the symbol
        if( ref.GetSymbol() )
        {
            for( const DATA_MODEL_COL& col : m_cols )
                updateDataStoreItemFieldFromLive( ref, col.m_fieldName );
        }

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


bool SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::DeleteRows( size_t aPosition, size_t aNumRows )
{
    size_t curNumRows = m_rows.size();

    if( aPosition >= curNumRows )
    {
        wxFAIL_MSG( wxString::Format( wxT( "Called SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::DeleteRows(aPosition=%lu, "
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
        m_symbolsList.Clear();

        if( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aPosition, aNumRows );
            GetView()->ProcessTableMessage( msg );
        }
    }
    else
    {
        // Note: this code is currently dead, as all current usage calls the clear path above,
        // which is left because it is faster. This code *should* be correct but is untested.
        auto first = m_rows.begin() + aPosition;
        auto last = first + aNumRows;

        SCH_REFERENCE_LIST refsToDelete;
        for( auto it = first; it != last && it != m_rows.end(); ++it )
        {
            for( const SCH_REFERENCE& ref : it->m_items )
                refsToDelete.AddItem( ref );
        }

        RemoveReferences( refsToDelete );
        // This will also notify the view
        RebuildRows();
    }

    return true;
}


std::vector<FIELD_CASE_CONFLICT> DetectFieldCaseConflicts( const SCH_REFERENCE_LIST& aSymbols )
{
    std::vector<FIELD_CASE_CONFLICT> conflicts;

    for( unsigned i = 0; i < aSymbols.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = aSymbols[i].GetSymbol();

        if( !symbol )
            continue;

        std::map<wxString, std::vector<std::pair<wxString, wxString>>> groups;

        for( const SCH_FIELD& field : symbol->GetFields() )
        {
            if( field.IsMandatory() || field.IsPrivate() )
                continue;

            groups[field.GetName().Lower()].emplace_back( field.GetName(), field.GetText() );
        }

        for( const auto& [key, members] : groups )
        {
            if( members.size() < 2 )
                continue;

            FIELD_CASE_CONFLICT c;
            c.symbol = symbol;
            c.sheetPath = aSymbols[i].GetSheetPath();
            c.reference = symbol->GetRef( &c.sheetPath );
            c.caseFoldedKey = key;
            c.variants = members;
            conflicts.push_back( std::move( c ) );
        }
    }

    return conflicts;
}
