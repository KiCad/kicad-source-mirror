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


/**
 * Cell renderer that shows the expanded result of text variables (e.g. "${VALUE}" is
 * displayed as "10K").  The actual cell still stores the raw variable so it can be
 * edited directly.
 */
class GRID_CELL_RESOLVED_TEXT_RENDERER : public wxGridCellStringRenderer
{
public:
    GRID_CELL_RESOLVED_TEXT_RENDERER() :
            wxGridCellStringRenderer()
    {
    }

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow, int aCol,
               bool isSelected ) override
    {
        wxString value = aGrid.GetCellValue( aRow, aCol );

        if( auto* model = dynamic_cast<FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL*>( aGrid.GetTable() ) )
            value = model->GetResolvedValue( aRow, aCol );

        wxRect rect = aRect;
        rect.Inflate( -1 );

        wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );
        SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
        aGrid.DrawTextRectangle( aDC, value, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
    }

    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow, int aCol ) override
    {
        wxString value = aGrid.GetCellValue( aRow, aCol );

        if( auto* model = dynamic_cast<FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL*>( aGrid.GetTable() ) )
            value = model->GetResolvedValue( aRow, aCol );

        return wxGridCellStringRenderer::DoGetBestSize( aAttr, aDC, value );
    }

    wxGridCellRenderer* Clone() const override { return new GRID_CELL_RESOLVED_TEXT_RENDERER(); }
};


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::AddColumn( const wxString& aFieldName, const wxString& aLabel,
                                                         bool aAddedByUser )
{
    // Don't add a field twice
    if( GetFieldNameCol( aFieldName ) != -1 )
        return;

    m_cols.push_back( { aFieldName, aLabel, aAddedByUser, false, false } );

    for( unsigned i = 0; i < m_footprintsList.size(); ++i )
        updateDataStoreFootprintField( m_footprintsList[i], aFieldName );
}


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::updateDataStoreFootprintField( const FOOTPRINT_REF& aFootprintRef,
                                                                             const wxString&      aFieldName )
{
    KIID_PATH key = getDataStoreKey( aFootprintRef );
    m_dataStore[key][aFieldName] = getFieldValueForVariant( aFootprintRef, aFieldName, m_currentVariant );
}


/**
 * Data store UUID for a footprint is just the footprint's UUID, since footprints are unique across the board.
 */
KIID_PATH FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getDataStoreKey( const FOOTPRINT_REF& aItem ) const
{
    KIID_PATH key;
    key.push_back( aItem.GetFootprint().m_Uuid );
    return key;
}


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getItemReference( const FOOTPRINT_REF& aItem ) const
{
    return aItem.GetFootprint().GetReferenceAsString();
}


wxGridCellAttr* FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
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
        if( rawValue.Contains( wxT( "${" ) ) )
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
            const FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW& row = m_rows[aRow];

            // Check if any footprint in this row has a variant-specific value
            for( const FOOTPRINT_REF& ref : row.m_items )
            {
                wxString defaultValue = getDefaultFieldValue( ref, fieldName );

                // Get the current value from the data store
                wxString currentValue;

                KIID_PATH key = getDataStoreKey( ref );

                if( m_dataStore.contains( key ) && m_dataStore[key].contains( fieldName ) )
                    currentValue = m_dataStore[key][fieldName];

                if( currentValue != defaultValue )
                {
                    needsVariantHighlight = true;

                    wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
                    bool     isDark = ( bg.Red() + bg.Green() + bg.Blue() ) < 384;

                    highlightColor = isDark ? wxColour( 80, 80, 40 )
                                            : wxColour( 255, 255, 200 );

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


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
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

    const FOOTPRINT_FIELDS_TABLE_DATA_MODEL_ROW& row = m_rows[aRow];
    const wxString&                              fieldName = m_cols[aCol].m_fieldName;

    for( const FOOTPRINT_REF& ref : row.m_items )
        m_dataStore[getDataStoreKey( ref )][fieldName] = aValue;

    m_edited = true;
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::isCellReadOnly( int aRow, int aCol )
{
    return FIELDS_TABLE_DATA_MODEL<FOOTPRINT_REF>::isCellReadOnly( aRow, aCol )
           || ColIsFootprint( aCol )
           || GetColFieldName( aCol ) == wxS( "${EXCLUDE_FROM_BOARD}" )
           || GetColFieldName( aCol ) == wxS( "${EXCLUDE_FROM_SIM}" );
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::unitMatch( const FOOTPRINT_REF& lhItem, const FOOTPRINT_REF& rhItem )
{
    // Footprints are just pointers and never have multiple units unlike symbols
    // so just compare
    return lhItem == rhItem;
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
            return footprint.ResolveTextVar( token, depth + 1 );
        };

        return ExpandTextVars( aFieldName, &footprintResolver );
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
        return aRef.GetFootprint().ResolveTextVar( token );
    };

    return ExpandTextVars( aText, &footprintResolver );
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
        return wxS( "0" );

    if( aAttributeName == wxS( "${EXCLUDE_FROM_POS_FILES}" ) )
        return aRef.GetFootprint().GetExcludedFromPosFilesForVariant( aVariantName ) ? wxS( "1" ) : wxS( "0" );

    return wxS( "0" );
}


bool FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::attributeInheritedFromSheet( const FOOTPRINT_REF&,
                                                                           const wxString& ) const
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


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getFieldValueForVariant( const FOOTPRINT_REF& aRef,
                                                                           const wxString&      aFieldName,
                                                                           const wxString&      aVariantName )
{
    const FOOTPRINT& footprint = aRef.GetFootprint();

    if( isAttribute( aFieldName ) )
        return getAttributeValue( aRef, aFieldName, aVariantName );

    if( const PCB_FIELD* field = footprint.GetField( aFieldName ) )
    {
        if( field->IsPrivate() )
            return wxEmptyString;

        wxString value = footprint.GetFieldValueForVariant( aVariantName, aFieldName );

        if( footprint.GetBoard() )
            // Cross part references e.g. ${U2:MyField} stored in U1 are converted
            // to KIIDs transparently e.g. ${KIID:MyField} so reannotating U2->U3 doesn't
            // break the the variable resolution
            value = footprint.GetBoard()->ConvertKIIDsToCrossReferences( value );

        return value;
    }

    // For generated fields, return the field name itself
    if( IsGeneratedField( aFieldName ) )
        return aFieldName;

    return wxEmptyString;
}


wxString FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::getDefaultFieldValue( const FOOTPRINT_REF& aRef,
                                                                        const wxString& aFieldName )
{
    return getFieldValueForVariant( aRef, aFieldName, wxEmptyString );
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
        attrChanged = false;
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

        if( !m_filter.IsEmpty() && !matcher.Find( footprint.GetReferenceAsString().Lower() ) )
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


void FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::ApplyData( BOARD_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames,
                                                         const wxString& aVariantName )
{
    bool defaultVariant = aVariantName.IsEmpty()
                          || aVariantName.CmpNoCase( GetDefaultVariantName() ) == 0;

    for( const FOOTPRINT_REF& ref : m_footprintsList )
    {
        FOOTPRINT& footprint = ref.GetFootprint();
        bool       footprintModified = false;

        std::unique_ptr<FOOTPRINT> footprintCopy = std::make_unique<FOOTPRINT>( footprint );
        footprintCopy->SetParentGroup( nullptr );

        KIID_PATH                           key = getDataStoreKey( ref );
        const std::map<wxString, wxString>& fieldStore = m_dataStore[key];

        for( const auto& [srcName, srcValue] : fieldStore )
        {
            // Attributes bypass the field logic, so handle them first
            if( isAttribute( srcName ) )
            {
                footprintModified |= setAttributeValue( ref, srcName, srcValue, aVariantName );
                continue;
            }

            // Skip generated fields with variables as names (e.g. ${QUANTITY});
            // they can't be edited
            if( IsGeneratedField( srcName ) )
                continue;

            // Don't apply footprint fields to footprints
            if( srcName == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
                continue;

            PCB_FIELD* destField = footprint.GetField( srcName );

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

            int  col = GetFieldNameCol( srcName );
            bool userAdded = ( col != -1 && m_cols[col].m_userAdded );

            // Add a not existing field if it has a value for this footprint
            bool createField = !destField && ( !srcValue.IsEmpty() || userAdded );

            if( createField )
            {
                destField = new PCB_FIELD( &footprint, FIELD_T::USER, srcName );
                destField->SetLayer( footprint.GetLayer() == F_Cu ? F_Fab : B_Fab );
                destField->SetFPRelativePosition( { 0, 0 } );

                if( BOARD* board = footprint.GetBoard() )
                    destField->StyleFromSettings( board->GetDesignSettings(), true );

                if( const TEMPLATE_FIELDNAME* srcTemplate = aTemplateFieldnames.GetFieldName( srcName ) )
                    destField->SetVisible( srcTemplate->m_Visible );
                else
                    destField->SetVisible( false );

                footprint.Add( destField );
                footprintModified = true;
            }

            if( !destField )
                continue;

            // Reference is not editable from this dialog
            if( destField->GetId() == FIELD_T::REFERENCE )
                continue;

            wxString previousValue = footprint.GetFieldValueForVariant( aVariantName, srcName );
            wxString newValue = aCommit.GetBoard()->ConvertCrossReferencesToKIIDs( srcValue );

            if( previousValue != newValue )
            {
                if( defaultVariant )
                {
                    destField->SetText( newValue );
                    footprintModified = true;
                }
                else if( FOOTPRINT_VARIANT* variant = footprint.AddVariant( aVariantName ) )
                {
                    variant->SetFieldValue( srcName, newValue );
                    footprintModified = true;
                }
            }
        }

        for( int ii = static_cast<int>( footprint.GetFields().size() ) - 1; ii >= 0; ii-- )
        {
            PCB_FIELD* field = footprint.GetFields()[ii];

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

                footprint.Remove( field );
                delete field;
                footprintModified = true;
            }
        }

        if( footprintModified )
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
            FOOTPRINT& footprint = ref.GetFootprint();
            m_footprintsList.push_back( ref );

            KIID_PATH key = getDataStoreKey( ref );

            // Update the fields of every reference
            for( const PCB_FIELD* field : footprint.GetFields() )
            {
                if( !field->IsPrivate() )
                {
                    wxString name = field->GetCanonicalName();
                    wxString value = getFieldValueForVariant( ref, name, m_currentVariant );
                    m_dataStore[key][name] = value;
                }
            }

            for( const DATA_MODEL_COL& col : m_cols )
                m_dataStore[key].try_emplace( col.m_fieldName, wxEmptyString );
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
        for( const DATA_MODEL_COL& col : m_cols )
            updateDataStoreFootprintField( ref, col.m_fieldName );

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
