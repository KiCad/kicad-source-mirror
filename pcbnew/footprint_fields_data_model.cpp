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
#include <memory>

#include <wx/string.h>
#include <wx/debug.h>
#include <wx/grid.h>
#include <wx/settings.h>
#include <common.h>
#include <core/kicad_algo.h>
#include <widgets/wx_grid.h>
#include <board.h>
#include <board_commit.h>
#include <eda_group.h>
#include <footprint.h>
#include <pcb_field.h>
#include <template_fieldnames.h>
#include <kiid.h>
#include "string_utils.h"

#include <footprint_fields_data_model.h>


const wxString LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_NAME = wxS( "${FOOTPRINT_NAME}" );
const wxString LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_KEYWORDS = wxS( "${FOOTPRINT_KEYWORDS}" );
const wxString LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_LIBRARY_DESCRIPTION =
        wxS( "${FOOTPRINT_LIBRARY_DESCRIPTION}" );


/**
 * Data store UUID for a footprint is just the footprint's UUID, since footprints are unique across the board.
 */
KIID_PATH FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getDataStoreKey( const FOOTPRINT_REF& aItem ) const
{
    KIID_PATH key;
    key.push_back( aItem.GetFootprint().m_Uuid );
    return key;
}


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getItemIdentifier( const FOOTPRINT_REF& aItem ) const
{
    return aItem.GetFootprint().GetReferenceAsString();
}


wxGridCellAttr* FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    wxGridCellAttr* attr = nullptr;
    bool            needsReadOnly = IsCellReadOnly( aRow, aCol );
    bool            needsUrlEditor = cellUsesUrlEditor( aRow, aCol );
    bool            needsVariantHighlight = false;
    bool            needsResolvedTextRenderer = cellUsesResolvedTextRenderer( aRow, aCol );
    wxColour        highlightColor;

    // Check if we need variant highlighting
    if( !m_currentVariant.IsEmpty() && aRow >= 0 && aRow < (int) m_rows.size() && aCol >= 0
        && aCol < (int) m_cols.size() )
    {
        const wxString& fieldName = m_cols[aCol].m_fieldName;

        // Skip Reference and generated fields (like ${QUANTITY}) for highlighting
        if( !ColIsReference( aCol ) && !ColIsQuantity( aCol ) && !ColIsItemNumber( aCol ) )
        {
            const FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW& row = m_rows[aRow];

            // Check if any footprint in this row has a variant-specific value
            for( const FOOTPRINT_REF& ref : row.m_items )
            {
                wxString defaultValue = getDefaultFieldValue( ref, fieldName );

                // Get the current value from the data store
                wxString currentValue;

                getStoredFieldValue( ref, fieldName, currentValue );

                if( currentValue != defaultValue )
                {
                    needsVariantHighlight = true;

                    wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
                    bool     isDark = ( bg.Red() + bg.Green() + bg.Blue() ) < 384;

                    highlightColor = isDark ? FIELDS_TABLE_COLOR::VARIANT_FIELD_OVERRIDE_DARK_YELLOW
                                            : FIELDS_TABLE_COLOR::VARIANT_FIELD_OVERRIDE_LIGHT_YELLOW;

                    break;
                }
            }
        }
    }

    // If we don't need any custom attributes, use the base class behavior
    if( !needsReadOnly && !needsUrlEditor && !needsVariantHighlight && !needsResolvedTextRenderer )
        return applyCellDecorations( WX_GRID_TABLE_BASE::GetAttr( aRow, aCol, aKind ), aRow, aCol );

    // URL cells use the Datasheet column's editor.  Other cells use their own column attributes.
    if( needsUrlEditor )
    {
        attr = cloneUrlEditorAttr();
    }
    else if( m_colAttrs.find( aCol ) != m_colAttrs.end() && m_colAttrs[aCol] )
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

    if( needsResolvedTextRenderer )
        applyResolvedTextRenderer( attr, !needsVariantHighlight );

    return applyCellDecorations( enhanceAttr( attr, aRow, aCol, aKind ), aRow, aCol );
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxS( "Invalid column number" ) );

    if( IsCellReadOnly( aRow, aCol ) )
        return;

    if( aValue == INDETERMINATE_STATE )
        return;

    const FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW& row = m_rows[aRow];
    const wxString&                              fieldName = m_cols[aCol].m_fieldName;

    for( const FOOTPRINT_REF& ref : row.m_items )
        setStoredFieldValue( ref, fieldName, aValue );

    m_edited = true;
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::ColIsReadOnly( int aCol ) const
{
    return FIELDS_TABLE_DATA_MODEL<FOOTPRINT_REF>::ColIsReadOnly( aCol )
           || ColIsFootprint( aCol )
           || m_cols[aCol].m_fieldName == wxS( "${EXCLUDE_FROM_BOARD}" );
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::unitMatch( const FOOTPRINT_REF& lhItem, const FOOTPRINT_REF& rhItem )
{
    // Footprints are just pointers and never have multiple units unlike symbols
    // so just compare
    return lhItem == rhItem;
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getLiveFieldValue( const FOOTPRINT_REF& aRef,
                                                                 const wxString& aFieldName,
                                                                 wxString& aValue )
{
    return getLiveFieldValueForVariant( aRef, aFieldName, m_currentVariant, aValue );
}


std::vector<FOOTPRINT_REF> FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getAllItems() const
{
    return m_footprintsList;
}


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getFieldResolvedLiveValue( const FOOTPRINT_REF& aRef,
                                                                             const wxString&      aFieldName )
{
    FOOTPRINT& footprint = aRef.GetFootprint();
    PCB_FIELD* field = footprint.GetField( aFieldName );

    if( field )
    {
        if( field->IsPrivate() )
            return wxEmptyString;
        else
            return field->GetShownText( false, 0 );
    }

    // Handle generated fields with variables as names (e.g. ${QUANTITY}) that are not present in
    // the footprint by giving them the correct value by resolving against the footprint
    if( IsGeneratedField( aFieldName ) )
    {
        int depth = 0;

        std::function<bool( wxString* )> footprintResolver = [&]( wxString* token ) -> bool
        {
            return footprint.ResolveTextVar( token, m_currentVariant, depth + 1 );
        };

        return ResolveTextVars( aFieldName, &footprintResolver, depth );
    }

    return wxEmptyString;
}


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::resolveTextVars( const FOOTPRINT_REF& aRef, const wxString& aText )
{
    // TODO: this isn't technically correct, this should resolve against the
    // data store's copy of variables whenever whenever possible,
    // but currently it is resolving against the footprint's current values.
    // For instance, if you have "My value is ${VALUE}" in the description field,
    // ${VALUE} will be resolved against the footprint's live value, not the Value field
    // stored in the data store.
    std::function<bool( wxString* )> footprintResolver = [&]( wxString* token ) -> bool
    {
        return aRef.GetFootprint().ResolveTextVar( token, m_currentVariant );
    };

    int depth = 0;
    return ResolveTextVars( aText, &footprintResolver, depth );
}


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getAttributeValue( const FOOTPRINT_REF& aRef,
                                                                     const wxString&      aAttributeName,
                                                                     const wxString&      aVariantName )
{
    if( aAttributeName == wxS( "${DNP}" ) )
        return aRef.GetFootprint().GetDNPForVariant( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
        return wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
        return aRef.GetFootprint().GetExcludedFromBOMForVariant( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
        return aRef.GetFootprint().GetExcludedFromSimForVariant( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
        return aRef.GetFootprint().GetExcludedFromPosFilesForVariant( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    return wxS( "0" );
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::attributeForcedOnBySheet( const FOOTPRINT_REF&, const wxString& ) const
{
    // TODO: So if a symbol has DNP set to false, and the sheet is in has DNP set to true,
    // the symbol will be effectively DNP true. This will propogate to the footprint correctly.
    //
    // The symbol fields table dialog shows the overridden sheet-inherited value and does not let
    // the user modify the symbol DNP value.
    //
    // The footprint fields table WILL allow the user to modify the footprint DNP value,
    // which will be overridden whenever they sync-to-board.
    //
    // To have similar behavior we will eventually need to track whether these attributes on
    // footprints came from the symbol or the sheet.
    return false;
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getLiveFieldValueForVariant( const FOOTPRINT_REF& aRef,
                                                                           const wxString&      aFieldName,
                                                                           const wxString&      aVariantName,
                                                                           wxString&            aValue )
{
    aValue.clear();

    const FOOTPRINT& footprint = aRef.GetFootprint();

    if( fieldIsAttribute( aFieldName ) )
    {
        aValue = getAttributeValue( aRef, aFieldName, aVariantName );
        return true;
    }

    if( aFieldName == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
    {
        aValue = footprint.GetFPIDAsString();
        return true;
    }

    if( const PCB_FIELD* field = footprint.GetField( aFieldName ) )
    {
        if( field->IsPrivate() )
            return false;

        aValue = footprint.GetFieldValueForVariant( aVariantName, aFieldName );

        if( footprint.GetBoard() )
            // Cross part references e.g. ${U2:MyField} stored in U1 are converted
            // to KIIDs transparently e.g. ${KIID:MyField} so reannotating U2->U3 doesn't
            // break the the variable resolution
            aValue = footprint.GetBoard()->ConvertKIIDsToCrossReferences( aValue );

        return true;
    }

    // For generated fields, return the field name itself
    if( IsGeneratedField( aFieldName ) )
    {
        aValue = aFieldName;
        return true;
    }

    return false;
}


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getDefaultFieldValue( const FOOTPRINT_REF& aRef,
                                                                        const wxString& aFieldName )
{
    wxString value;
    getLiveFieldValueForVariant( aRef, aFieldName, wxEmptyString, value );
    return value;
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::setAttributeValue( const FOOTPRINT_REF& aRef,
                                                                 const wxString& aAttributeName,
                                                                 const wxString& aValue,
                                                                 const wxString& aVariantName )
{
    bool attrChanged = false;
    bool newValue = aValue == wxS( "1" );
    bool defaultVariant = aVariantName.IsEmpty()
                          || aVariantName.CmpNoCase( GetDefaultVariantName() ) == 0;

    if( aAttributeName == wxS( "${DNP}" ) )
    {
        attrChanged = aRef.GetFootprint().GetDNPForVariant( aVariantName ) != newValue;

        if( attrChanged )
        {
            // TODO: fix footprint API to match symbol
            if( defaultVariant )
                aRef.GetFootprint().SetDNP( newValue );
            else if( FOOTPRINT_VARIANT* variant = aRef.GetFootprint().AddVariant( aVariantName ) )
                variant->SetDNP( newValue );
        }
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOARD}" ) )
    {
        attrChanged = false;
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_BOM}" ) )
    {
        attrChanged = aRef.GetFootprint().GetExcludedFromBOMForVariant( aVariantName ) != newValue;

        if( attrChanged )
        {
            // TODO: fix footprint API to match symbol
            if( defaultVariant )
                aRef.GetFootprint().SetExcludedFromBOM( newValue );
            else if( FOOTPRINT_VARIANT* variant = aRef.GetFootprint().AddVariant( aVariantName ) )
                variant->SetExcludedFromBOM( newValue );
        }
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_SIM}" ) )
    {
        attrChanged = aRef.GetFootprint().GetExcludedFromSimForVariant( aVariantName ) != newValue;

        if( attrChanged )
        {
            // TODO: fix footprint API to match symbol
            if( defaultVariant )
                aRef.GetFootprint().SetExcludedFromSim( newValue );
            else if( FOOTPRINT_VARIANT* variant = aRef.GetFootprint().AddVariant( aVariantName ) )
                variant->SetExcludedFromSim( newValue );
        }
    }
    else if( aAttributeName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
    {
        attrChanged = aRef.GetFootprint().GetExcludedFromPosFilesForVariant( aVariantName ) != newValue;

        if( attrChanged )
        {
            // TODO: fix footprint API to match symbol
            if( defaultVariant )
                aRef.GetFootprint().SetExcludedFromPosFiles( newValue );
            else if( FOOTPRINT_VARIANT* variant = aRef.GetFootprint().AddVariant( aVariantName ) )
                variant->SetExcludedFromPosFiles( newValue );
        }
    }

    return attrChanged;
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::RebuildRows()
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

    for( const FOOTPRINT_REF& ref : m_footprintsList )
    {
        const FOOTPRINT& footprint = ref.GetFootprint();

        if( m_scope == SCOPE::SCOPE_SELECTION
            && !m_selectionItems.contains( getDataStoreKey( ref ) ) )
        {
            continue;
        }

        if( !MatchesFilter( ref, getItemIdentifier( ref ), matcher ) )
            continue;

        if( m_excludeDNP )
        {
            bool isDNP = false;

            if( !m_variantNames.empty() )
            {
                for( const wxString& variantName : m_variantNames )
                {
                    if( footprint.GetDNPForVariant( variantName ) )
                    {
                        isDNP = true;
                        break;
                    }
                }
            }
            else
            {
                isDNP = footprint.GetDNPForVariant( m_currentVariant );
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
                    if( footprint.GetExcludedFromBOMForVariant( variantName ) )
                    {
                        isExcluded = true;
                        break;
                    }
                }
            }
            else
            {
                isExcluded = footprint.GetExcludedFromBOMForVariant( m_currentVariant );
            }

            if( isExcluded )
                continue;
        }

        KIID_PATH footprintSheetPath = footprint.GetPath();

        if( !footprintSheetPath.empty() )
            footprintSheetPath.pop_back();

        if( ( m_scope == SCOPE::SCOPE_SHEET && footprintSheetPath != m_path )
            || ( m_scope == SCOPE::SCOPE_SHEET_RECURSIVE
                 && !footprintSheetPath.IsContainedWithin( m_path ) ) )
        {
            continue;
        }

        bool matchFound = false;

        // Performance optimization for ungrouped case to skip the N^2 for loop
        if( !m_groupingEnabled )
        {
            m_rows.emplace_back( FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW( ref, ROW_STATE::NON_EXPANDABLE ) );
            continue;
        }

        // See if we already have a row which this footprint fits into
        for( FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW& row : m_rows )
        {
            // all group members must have identical refs so just use the first one
            const FOOTPRINT_REF& rowRef = row.m_items[0];

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
            m_rows.emplace_back( FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW( ref, ROW_STATE::NON_EXPANDABLE ) );
    }

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
        GetView()->ProcessTableMessage( msg );
    }

    Sort();
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::applyDataToFootprint( const FOOTPRINT_REF& aSourceRef,
                                                                    FOOTPRINT&           aDestFootprint,
                                                                    TEMPLATES*           aTemplateFieldnames,
                                                                    const wxString&      aVariantName )
{
    bool defaultVariant = aVariantName.IsEmpty()
                          || aVariantName.CmpNoCase( GetDefaultVariantName() ) == 0;
    bool footprintModified = false;

    FOOTPRINT_REF destRef( aDestFootprint );

    const std::map<wxString, wxString>& fieldStore = getStoredFields( aSourceRef );

    for( const auto& [srcName, srcValue] : fieldStore )
    {
        // Attributes bypass the field logic, so handle them first
        if( fieldIsAttribute( srcName ) )
        {
            footprintModified |= setAttributeValue( destRef, srcName, srcValue, aVariantName );
            continue;
        }

        // Lib footprint fields models exposes extra footprint properties like lib description
        // that aren't fields
        if( fieldIsItemProperty( srcName ) )
            continue;

        // Skip generated fields with variables as names (e.g. ${QUANTITY});
        // they can't be edited
        if( IsGeneratedField( srcName ) )
            continue;

        // Don't apply footprint fields to footprints
        if( srcName == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
            continue;

        int col = GetFieldNameCol( srcName );

        // Footprint names are not editable (from the fields table dialogs)
        if( col != -1 && ColIsItemIdentifier( col ) )
            continue;

        PCB_FIELD* destField = aDestFootprint.GetField( srcName );

        if( destField && destField->IsPrivate() )
        {
            if( srcValue.IsEmpty() )
                continue;
            else
            {
                destField->SetPrivate( false );
                footprintModified = true;
            }
        }

        // Reaching this point means the data store field is at least marked present,
        // so add the field to the footprint even when its stored value is empty.
        bool createField = !destField;

        if( createField )
        {
            destField = new PCB_FIELD( &aDestFootprint, FIELD_T::USER, srcName );
            destField->SetLayer( aDestFootprint.GetLayer() == F_Cu ? F_Fab : B_Fab );
            destField->SetFPRelativePosition( { 0, 0 } );

            if( BOARD* board = aDestFootprint.GetBoard() )
                destField->StyleFromSettings( board->GetDesignSettings(), true );

            // TODO: Fixup when this is implemented on the PCB side of things
            if( aTemplateFieldnames )
            {
                if( const TEMPLATE_FIELDNAME* srcTemplate = aTemplateFieldnames->GetFieldName( srcName ) )
                    destField->SetVisible( srcTemplate->m_Visible );
                else
                    destField->SetVisible( false );
            }
            else
                destField->SetVisible( false );

            aDestFootprint.Add( destField );
            footprintModified = true;
        }

        if( !destField )
            continue;

        wxString previousValue = aDestFootprint.GetFieldValueForVariant( aVariantName, srcName );
        wxString newValue = srcValue;

        // Board work is optional, not preset for lib fp fields table
        if( BOARD* board = aDestFootprint.GetBoard() )
            newValue = board->ConvertCrossReferencesToKIIDs( srcValue );

        if( previousValue != newValue )
        {
            // Lib footprints pass a wxEmptyString variant and always apply straight to the field
            if( defaultVariant )
            {
                destField->SetText( newValue );
                footprintModified = true;
            }
            else if( FOOTPRINT_VARIANT* variant = aDestFootprint.AddVariant( aVariantName ) )
            {
                variant->SetFieldValue( srcName, newValue );
                footprintModified = true;
            }
        }
    }

    for( int ii = static_cast<int>( aDestFootprint.GetFields().size() ) - 1; ii >= 0; ii-- )
    {
        PCB_FIELD* field = aDestFootprint.GetFields()[ii];

        if( field->IsMandatory() || field->IsPrivate() )
            continue;

        if( !fieldStore.contains( field->GetCanonicalName() ) )
        {
            // TODO: unlike symbols/SCH_FIELD, footprint PCB_FIELD
            // can be grouped so we need to remove it from the group before deleting it
            // In general, I'm not sure letting PCB_FIELDs associated with a footprint
            // be grouped separately from the footprint is a good idea.
            if( EDA_GROUP* parentGroup = field->GetParentGroup() )
                parentGroup->RemoveItem( field );

            aDestFootprint.Remove( field );
            delete field;
            footprintModified = true;
        }
    }

    return footprintModified;
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData( BOARD_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames,
                                                         const wxString& aVariantName )
{
    for( const FOOTPRINT_REF& ref : m_footprintsList )
    {
        FOOTPRINT& footprint = ref.GetFootprint();

        // commit will delete the copy properly as needed, and we will delete it when
        // we go out of scope if we fail to apply the data
        std::unique_ptr<FOOTPRINT> footprintCopy = std::make_unique<FOOTPRINT>( footprint );
        footprintCopy->SetParentGroup( nullptr );

        // Only commit if the footprint was actually modified
        if( applyDataToFootprint( ref, footprint, &aTemplateFieldnames, aVariantName ) )
            aCommit.Modified( &footprint, footprintCopy.release() );
    }

    m_edited = false;
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::AddReferences( const FOOTPRINT_REFERENCE_LIST& aRefs )
{
    for( const FOOTPRINT_REF& ref : aRefs )
    {
        if( !alg::contains( m_footprintsList, ref ) )
        {
            m_footprintsList.push_back( ref );

            initializeDataStoreItem( ref );
        }
    }
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::RemoveFootprint( const FOOTPRINT_REF& aFootprint )
{
    m_dataStore.erase( getDataStoreKey( aFootprint ) );

    m_footprintsList.erase( std::remove_if( m_footprintsList.begin(), m_footprintsList.end(),
                                            [&aFootprint]( const FOOTPRINT_REF& ref ) -> bool
                                            {
                                                return ref == aFootprint;
                                            } ),
                            m_footprintsList.end() );
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::RemoveReferences( const FOOTPRINT_REFERENCE_LIST& aRefs )
{
    for( const FOOTPRINT_REF& ref : aRefs )
        RemoveFootprint( ref );
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::UpdateReferences( const FOOTPRINT_REFERENCE_LIST& aRefs )
{
    for( const FOOTPRINT_REF& ref : aRefs )
    {
        // Update the fields of every reference. Do this by iterating through the data model
        // columns; we must have all fields in the footprint added to the data model at this point,
        // and some of the data model columns may be variables that are not present in the footprint
        initializeDataStoreItem( ref );

        if( !alg::contains( m_footprintsList, ref ) )
            m_footprintsList.push_back( ref );
    }
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::DeleteRows( size_t aPosition, size_t aNumRows )
{
    size_t curNumRows = m_rows.size();

    if( aPosition >= curNumRows )
    {
        wxFAIL_MSG( wxString::Format( wxT( "Called FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::DeleteRows(aPosition=%lu, "
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
        m_footprintsList.clear();

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

        FOOTPRINT_REFERENCE_LIST refsToDelete;
        for( auto it = first; it != last && it != m_rows.end(); ++it )
        {
            for( const FOOTPRINT_REF& ref : it->m_items )
                refsToDelete.push_back( ref );
        }

        RemoveReferences( refsToDelete );
        // This will also notify the view
        RebuildRows();
    }

    return true;
}


LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL(
        const FOOTPRINT_REFERENCE_LIST& aFootprints ) :
        FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL( aFootprints )
{
    m_includeExcluded = true;
}


bool LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::ColIsItemIdentifier( int aCol ) const
{
    wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false );
    return m_cols[aCol].m_fieldName == FOOTPRINT_NAME;
}


bool LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::fieldIsItemProperty( const wxString& aFieldName ) const
{
    return aFieldName == FOOTPRINT_NAME || aFieldName == FOOTPRINT_KEYWORDS
           || aFieldName == FOOTPRINT_LIBRARY_DESCRIPTION
           || FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::fieldIsItemProperty( aFieldName );
}


bool LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getLiveFieldValue( const FOOTPRINT_REF& aRef,
                                                                     const wxString& aFieldName, wxString& aValue )
{
    const FOOTPRINT& footprint = aRef.GetFootprint();

    if( aFieldName == FOOTPRINT_NAME )
    {
        aValue = footprint.GetName();
        return true;
    }
    else if( aFieldName == FOOTPRINT_KEYWORDS )
    {
        aValue = footprint.GetKeywords();
        return true;
    }
    else if( aFieldName == FOOTPRINT_LIBRARY_DESCRIPTION )
    {
        aValue = footprint.GetLibDescription();
        return true;
    }

    return FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getLiveFieldValue( aRef, aFieldName, aValue );
}


wxString LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getItemIdentifier( const FOOTPRINT_REF& aRef ) const
{
    return aRef.GetFootprint().GetName();
}


bool LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::applyDataToFootprint( const FOOTPRINT_REF& aRef,
                                                                        FOOTPRINT&           aFootprint )
{
    bool     footprintModified = false;
    wxString value;

    if( getStoredFieldValue( aRef, FOOTPRINT_KEYWORDS, value )
        && aFootprint.GetKeywords() != value )
    {
        aFootprint.SetKeywords( value );
        footprintModified = true;
    }

    if( getStoredFieldValue( aRef, FOOTPRINT_LIBRARY_DESCRIPTION, value )
        && aFootprint.GetLibDescription() != value )
    {
        aFootprint.SetLibDescription( value );
        footprintModified = true;
    }

    footprintModified |= FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::applyDataToFootprint(
            aRef, aFootprint, nullptr, wxEmptyString );

    return footprintModified;
}


bool LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData( std::function<bool( FOOTPRINT& )> aChangeHandler )
{
    bool allChangesApplied = true;

    for( const FOOTPRINT_REF& ref : m_footprintsList )
    {
        // This works the opposite of the non-lib footprint fields table,
        // here we just make a copy on the stack and let it pop off regardless of what
        // happens; the actual footprint in source ref gets overwritten when we've
        // determined that the change were applied
        FOOTPRINT changedFootprint( ref.GetFootprint() );

        if( !applyDataToFootprint( ref, changedFootprint ) )
        {
            // An empty staged public field may collide with an existing private field and
            // be ignored. Re-sync from live so the no-op does not leave the model edited.
            // A non-empty public field that collides with a private field is taken to be
            // an explicit request to make it non-private, so that case isn't what we're
            // checking for here, only the empty public/existing private mismatch.
            for( const DATA_MODEL_COL& col : m_cols )
                updateDataStoreItemFieldFromLive( ref, col.m_fieldName );

            continue;
        }

        if( !aChangeHandler( changedFootprint ) )
        {
            allChangesApplied = false;
            break;
        }

        ref.GetFootprint() = changedFootprint;

        // Update the data store with the new live values after applying changes
        for( const DATA_MODEL_COL& col : m_cols )
            updateDataStoreItemFieldFromLive( ref, col.m_fieldName );
    }

    m_edited = false;

    for( const FOOTPRINT_REF& ref : m_footprintsList )
    {
        for( const DATA_MODEL_COL& col : m_cols )
        {
            if( fieldIsModified( ref, col.m_fieldName ) )
            {
                m_edited = true;
                return allChangesApplied;
            }
        }
    }

    return allChangesApplied;
}
