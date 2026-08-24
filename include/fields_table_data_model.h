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

#pragma once

#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <map>

#include <widgets/wx_grid.h>
#include <widgets/ui_common.h>

#include <common.h>
#include <eda_pattern_match.h>
#include <kiid.h>
#include <refdes_utils.h>
#include <settings/bom_settings.h>
#include <string_utils.h>
#include <wx/debug.h>


namespace FIELDS_TABLE_COLOR
{
inline const wxColour VARIANT_SYMBOL_OVERRIDE_DARK_BLUE( 40, 60, 80 );
inline const wxColour VARIANT_SYMBOL_OVERRIDE_LIGHT_BLUE( 220, 235, 255 );
inline const wxColour VARIANT_FIELD_OVERRIDE_DARK_YELLOW( 80, 80, 40 );
inline const wxColour VARIANT_FIELD_OVERRIDE_LIGHT_YELLOW( 255, 255, 200 );
inline const wxColour TEXT_VARIABLE_DARK_AMBER( 80, 70, 30 );
inline const wxColour TEXT_VARIABLE_LIGHT_YELLOW( 255, 252, 200 );
inline const wxColour STRIPED_CLEARED_EMPTY_FIELD_LIGHT_GREEN( 180, 220, 180 );
inline const wxColour STRIPED_CLEARED_NONEMPTY_FIELD_LIGHT_RED( 220, 180, 180 );
inline const wxColour STRIPED_EDITED_EMPTY_FIELD_MUTED_GREEN( 180, 200, 180 );
inline const wxColour STRIPED_EDITED_NONEMPTY_FIELD_MUTED_RED( 200, 180, 180 );
inline const wxColour EDITED_EMPTY_FIELD_BRIGHT_GREEN( 192, 255, 192 );
inline const wxColour CLEARED_FIELD_STRIPE_ON_DARK_LIGHT_RED( 220, 180, 180 );
inline const wxColour CLEARED_FIELD_STRIPE_ON_LIGHT_DARK_RED( 100, 10, 10 );
} // namespace FIELDS_TABLE_COLOR


/**
 * The point of the data model classes is fundamentally to represent three things:
 *
 * 1. The list of live "items" (symbols, footprints, etc). These are often pointers to the actual item,
 * or instances in the case of symbols. This is where we get live data from on initial load, and when
 * schematic/board changes come in in real-time, and where we write data back to when the user is done
 * editing.
 *
 * 2. The data store (m_dataStore), this is the map of the user's edited values. This lets us keep
 * edits separate from the live values, especially since in some editors, this dialog is not modal,
 * and the user can change the live items while the dialog is open and contains edits.
 *
 * 3. The wx grid view of the table, which is the actual row of grids and columns that we
 * generate from the data store. This is what the user sees and interacts with. This changes
 * with filtering/scoping/grouping, etc. and is rebuilt from the data store as needed.
 */

/**
 * DATA_MODEL_COL and DATA_MODEL_ROW are used together in vectors of both (m_cols and m_rows)
 * to represent the current view of the fields table.
 *
 * This is not where the fields data itself is stored. See the data model for that.
 *
 * The columns are rebuilt as the user changes the display options, presets, etc.
 *
 * The rows are rebuilt according to grouping, filtering, sorting, etc. as well
 * as user actions like expanding and collapsing rows with groupable fields.
 *
 * A row can contain multiple items because they are grouped together because of the
 * user's grouping settings. Or, a row may contain multiple references to a multi-unit
 * symbol, etc. So, it does not necessarily represent a single symbol in the schematic.
 */

struct DATA_MODEL_COL
{
    wxString m_fieldName;
    wxString m_label;
    bool     m_userAdded;
    bool     m_show;
    bool     m_group;
};


template <typename ITEM_TYPE>
struct DATA_MODEL_ROW
{
    DATA_MODEL_ROW( const ITEM_TYPE& aFirstItem, ROW_STATE aGroupingState )
    {
        m_itemNumber = 0;
        m_items.push_back( aFirstItem );
        m_state = aGroupingState;
    }

    int                    m_itemNumber;
    ROW_STATE              m_state;
    std::vector<ITEM_TYPE> m_items;
};


/**
 * Contains everything completly generic to fields tables data models,
 * as well as column functionality that doesn't touch the internal fields data store.
 */
class FIELDS_TABLE_DATA_MODEL_BASE : public WX_GRID_TABLE_BASE
{
public:
    FIELDS_TABLE_DATA_MODEL_BASE();
    ~FIELDS_TABLE_DATA_MODEL_BASE() override;

    static const wxString QUANTITY_VARIABLE;
    static const wxString ITEM_NUMBER_VARIABLE;

    bool IsEdited() { return m_edited; }

    virtual void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser ) = 0;

    void MoveColumn( int aCol, int aNewPos );
    void RemoveColumn( int aCol );
    void RenameColumn( int aCol, const wxString& newName );

    int GetNumberCols() override { return static_cast<int>( m_cols.size() ); }

    void     SetColLabelValue( int aCol, const wxString& aLabel ) override;
    wxString GetColLabelValue( int aCol ) override;
    wxString GetColFieldName( int aCol );
    int      GetColDataWidth( int aCol );
    int      GetFieldNameCol( const wxString& aFieldName ) const;

    std::vector<BOM_FIELD> GetFieldsOrdered();
    void                   SetFieldsOrder( const std::vector<wxString>& aNewOrder );

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    bool ColIsReference( int aCol );
    bool ColIsQuantity( int aCol );
    bool ColIsItemNumber( int aCol );
    bool ColIsValue( int aCol );
    bool ColIsFootprint( int aCol );
    bool ColIsAttribute( int aCol );

    /**
     * Reference for symbol/fields tables,
     * lib_id for lib tables.
     */
    virtual bool ColIsItemIdentifier( int aCol ) { return ColIsReference( aCol ); }

    bool IsExpanderColumn( int aCol ) const override;
    virtual bool IsCellReadOnly( int aRow, int aCol );

    void SetSorting( int aCol, bool aAscending );
    int  GetSortCol() { return m_sortColumn; }
    bool GetSortAsc() { return m_sortAscending; }

    // These are used to disable RebuildRows() while generating batches of UI events, e.g.
    // applying a BOM preset, that would otherwise thrash the grid.
    void         EnableRebuilds();
    void         DisableRebuilds();
    virtual void RebuildRows() = 0;

    void             SetFilter( const wxString& aFilter ) { m_filter = aFilter; }
    const wxString&  GetFilter() { return m_filter; }
    void             SetFilterScope( BOM_FILTER_SCOPE aScope ) { m_filterScope = aScope; }
    BOM_FILTER_SCOPE GetFilterScope() const { return m_filterScope; }

    void SetSelectionItems( const std::unordered_set<KIID_PATH>& aItems ) { m_selectionItems = aItems; }
    void ClearSelectionItems() { m_selectionItems.clear(); }

    void SetGroupingEnabled( bool aGroup ) { m_groupingEnabled = aGroup; }
    bool GetGroupingEnabled() { return m_groupingEnabled; }

    /* These contradictorily named functions force including items that have the Exclude from
     * BOM flag set.  This is needed so they can be viewed in a fields table while still being
     * excluded from BOM export.
     */
    void SetIncludeExcludedFromBOM( bool aInclude ) { m_includeExcluded = aInclude; }
    bool GetIncludeExcludedFromBOM() { return m_includeExcluded; }

    void SetExcludeDNP( bool aExclude ) { m_excludeDNP = aExclude; }
    bool GetExcludeDNP() { return m_excludeDNP; }

    void SetGroupColumn( int aCol, bool aGroup );
    bool GetGroupColumn( int aCol );

    void SetShowColumn( int aCol, bool aShow );
    bool GetShowColumn( int aCol );

    void       ApplyBomPreset( const BOM_PRESET& aPreset );
    BOM_PRESET GetBomSettings();
    wxString   Export( const BOM_FMT_PRESET& aSettings );

    virtual wxString GetResolvedValue( int aRow, int aCol ) = 0;
    virtual wxString GetExportValue( int aRow, int aCol, const wxString& aRefDelimiter,
                                     const wxString& aRefRangeDelimiter ) = 0;

    virtual void ClearCell( int aRow, int aCol ) = 0;
    bool         CanClearCell( int aRow, int aCol );
    virtual bool IsCellClear( int aRow, int aCol ) = 0;
    virtual bool IsCellEdited( int aRow, int aCol ) = 0;
    /// Return true if any cell in the row has been edited.
    bool         IsRowEdited( int aRow );
    virtual void RevertRow( int aRow ) = 0;

    /// Return the stable data-store keys of every item represented by a row.
    virtual std::vector<KIID_PATH> GetRowItemKeys( int aRow ) const = 0;

    /**
     * Set the current variant name for highlighting purposes.
     *
     * When a variant is set, cells that differ from the default (non-variant) value
     * will be highlighted.
     *
     * @param aVariantName The name of the current variant, or empty string for default.
     */
    void            SetCurrentVariant( const wxString& aVariantName ) { m_currentVariant = aVariantName; }
    const wxString& GetCurrentVariant() const { return m_currentVariant; }

    void SetVariantNames( const std::vector<wxString>& aVariantNames ) { m_variantNames = aVariantNames; }
    const std::vector<wxString>& GetVariantNames() const { return m_variantNames; }

    // Identity-based undo serialization (keyed by symbol, not row position) for the dialog's
    // Ctrl+Z, so it stays correct as rows are grouped/sorted/reordered.
    bool     HasUndoStateSerialization() const override { return true; }
    wxString SerializeUndoState() const override;
    void     RestoreUndoState( const wxString& aState ) override;


    // Stuff this class knows nothing about, but needs to guarantee that the templated class implements
    virtual void ExpandCollapseRow( int aRow ) = 0;

protected:
    virtual bool fieldIsAttribute( const wxString& aFieldName ) const;

    // Helper function to translate named attribute values like ${DNP}.
    virtual wxString getAttributeResolvedValue( const wxString& aFieldName, bool aValue ) const;

    bool cellUsesResolvedTextRenderer( int aRow, int aCol );
    void applyResolvedTextRenderer( wxGridCellAttr* aAttr, bool aApplyTint );

    wxGridCellAttr* applyFieldPresenceRenderer( wxGridCellAttr* aAttr, int aRow, int aCol );

protected:
    wxGridCellRenderer* m_stripedRenderer;
    wxGridCellRenderer* m_resolvedTextRenderer;

    bool             m_edited;
    int              m_sortColumn;
    bool             m_sortAscending;
    wxString         m_filter;
    BOM_FILTER_SCOPE m_filterScope;
    bool             m_groupingEnabled;
    bool             m_excludeDNP;
    bool             m_includeExcluded;
    bool             m_rebuildsEnabled;

    ///< Items included by the user selection scope
    std::unordered_set<KIID_PATH> m_selectionItems;

    wxString              m_currentVariant;  ///< Current variant name for highlighting
    std::vector<wxString> m_variantNames;    ///< Variant names for multi-variant DNP filtering


    std::vector<DATA_MODEL_COL> m_cols;

    // Data store
    //
    // This is the storage of the dialog's currently edited field values for each item in the table.
    // Having a store separate from the live values in the symbols/footprints objects
    // allows us to keep our edits separate from the actual sch/pcb until the user explicitly
    // applies them.
    //
    // Items (symbols/footprints) are both identified by KIID_PATH, which for a symbol
    // is the sheet path + symbol UUID (symbols have multiple instances),
    // and for a footprint is the footprint UUID.
    //
    // m_rows and m_cols are just a generated view based on the data store,
    // and are rebuilt as the user changes grouping, sorting, filtering, etc.
    //
    // NOTE: be very careful about how you "read" this data store, you should
    // use getDataStoreFieldValue() to read values from the data store.
    //
    // The map is used to distinguish between present-but-empty vs. not-present.
    //
    // Use the get/set/clear/update/initialize functions to access the data store,
    // rather than accessing it directly, as using [] can unintentionally create
    // a present-but-empty field when you just want to check if it is present.
    std::map<KIID_PATH, std::map<wxString, wxString>> m_dataStore;
};


/**
 * Cell renderer that shows the expanded result of text variables (e.g. "${VALUE}" is
 * displayed as "10K").  The actual cell still stores the raw variable so it can be
 * edited directly.
 */
class GRID_CELL_RESOLVED_TEXT_RENDERER : public wxGridCellStringRenderer
{
public:
    GRID_CELL_RESOLVED_TEXT_RENDERER();

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow, int aCol,
               bool isSelected ) override;

    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow, int aCol ) override;

    wxGridCellRenderer* Clone() const override;
};


/**
 * The rest of the generic fields data model stuff, primarily around row functionality.
 *
 * Templated because rows differ per model, e.g. SCH_REFERENCE for the schematic fields table.
 */
template <typename ITEM_TYPE>
class FIELDS_TABLE_DATA_MODEL : public FIELDS_TABLE_DATA_MODEL_BASE
{
public:
    void AddColumn( const wxString& aFieldName, const wxString& aLabel,
                    bool aAddedByUser ) override
    {
        // Don't add a field twice
        if( GetFieldNameCol( aFieldName ) != -1 )
            return;

        m_cols.push_back( { aFieldName, aLabel, aAddedByUser, false, false } );

        for( const ITEM_TYPE& item : getAllItems() )
            initializeDataStoreItemField( item, m_cols.back() );

        if( aAddedByUser )
            m_edited = true;
    }


    /**
     * Clears the field from the data store, rather than setting its value to an empty string.
     */
    void ClearCell( int aRow, int aCol ) override
    {
        wxCHECK_RET( aRow >= 0 && aRow < static_cast<int>( m_rows.size() ), "Invalid Row Number" );
        wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );

        if( !CanClearCell( aRow, aCol ) )
            return;

        const wxString& fieldName = m_cols[aCol].m_fieldName;

        for( const ITEM_TYPE& item : m_rows[aRow].m_items )
            clearStoredField( item, fieldName );

        m_edited = true;
    }


    /**
     * Returns true if the cell is not present in the data store for any item in the row,
     * not just empty.
     */
    bool IsCellClear( int aRow, int aCol ) override
    {
        wxCHECK_MSG( aRow >= 0 && aRow < static_cast<int>( m_rows.size() ), false, "Invalid Row Number" );
        wxCHECK_MSG( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false, "Invalid Column Number" );

        for( const ITEM_TYPE& item : m_rows[aRow].m_items )
        {
            wxString unused;

            if( getStoredFieldValue( item, m_cols[aCol].m_fieldName, unused ) )
                return false;
        }

        return true;
    }


    /**
     * Returns true if the cell has been modified from the live value in any item in the row
     * (symbol/footprint/etc).
     */
    bool IsCellEdited( int aRow, int aCol ) override
    {
        wxCHECK_MSG( aRow >= 0 && aRow < static_cast<int>( m_rows.size() ), false, "Invalid Row Number" );
        wxCHECK_MSG( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), false, "Invalid Column Number" );

        for( const ITEM_TYPE& item : m_rows[aRow].m_items )
        {
            if( fieldIsModified( item, m_cols[aCol].m_fieldName ) )
                return true;
        }

        return false;
    }


    int       GetNumberRows() override { return (int) m_rows.size(); }
    ROW_STATE GetRowState( int aRow ) const override { return m_rows[aRow].m_state; }


    std::vector<KIID_PATH> GetRowItemKeys( int aRow ) const override
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), std::vector<KIID_PATH>() );

        std::vector<KIID_PATH> keys;

        for( const ITEM_TYPE& item : m_rows[aRow].m_items )
            keys.push_back( getDataStoreKey( item ) );

        return keys;
    }


    std::vector<ITEM_TYPE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), std::vector<ITEM_TYPE>() );
        return m_rows[aRow].m_items;
    }


    /**
     * Go through and revert all the fields in the row to their live values from the item (symbol/footprint/etc).
     *
     * Then recheck if any fields in the entire table are modified, and set edited state accordingly.
     */
    void RevertRow( int aRow ) override
    {
        wxCHECK_RET( aRow >= 0 && aRow < static_cast<int>( m_rows.size() ), "Invalid Row Number" );

        for( const ITEM_TYPE& item : m_rows[aRow].m_items )
        {
            for( const DATA_MODEL_COL& col : m_cols )
                updateDataStoreItemFieldFromLive( item, col.m_fieldName );
        }

        m_edited = false;

        for( const ITEM_TYPE& item : getAllItems() )
        {
            for( const DATA_MODEL_COL& col : m_cols )
            {
                if( fieldIsModified( item, col.m_fieldName ) )
                {
                    m_edited = true;
                    return;
                }
            }
        }
    }


    void ExpandRow( int aRow )
    {
        std::vector<DATA_MODEL_ROW<ITEM_TYPE>> children;

        for( ITEM_TYPE& ref : m_rows[aRow].m_items )
        {
            bool matchFound = false;

            // See if we already have a child group which this symbol fits into
            for( DATA_MODEL_ROW<ITEM_TYPE>& child : children )
            {
                // group members are by definition all matching, so just check
                // against the first member
                if( unitMatch( ref, child.m_items[0] ) )
                {
                    matchFound = true;
                    child.m_items.push_back( ref );
                    break;
                }
            }

            if( !matchFound )
                children.emplace_back( ref, ROW_STATE::EXPANDED_CHILD );
        }

        if( children.size() < 2 )
            return;

        std::sort( children.begin(), children.end(),
                   [this]( const DATA_MODEL_ROW<ITEM_TYPE>& lhs,
                           const DATA_MODEL_ROW<ITEM_TYPE>& rhs ) -> bool
                   {
                       return cmpRows( lhs, rhs, m_sortColumn, m_sortAscending );
                   } );

        m_rows[aRow].m_state = ROW_STATE::EXPANDED_PARENT;
        m_rows.insert( m_rows.begin() + aRow + 1, children.begin(), children.end() );

        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, aRow, children.size() );
        GetView()->ProcessTableMessage( msg );
    }


    void CollapseRow( int aRow )
    {
        auto firstChild = m_rows.begin() + aRow + 1;
        auto afterLastChild = firstChild;
        int  deleted = 0;

        while( afterLastChild != m_rows.end() && afterLastChild->m_state == ROW_STATE::EXPANDED_CHILD )
        {
            deleted++;
            afterLastChild++;
        }

        m_rows[aRow].m_state = ROW_STATE::COLLAPSED;
        m_rows.erase( firstChild, afterLastChild );

        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow + 1, deleted );
        GetView()->ProcessTableMessage( msg );
    }


    void ExpandCollapseRow( int aRow ) override
    {
        if( m_rows[aRow].m_state == ROW_STATE::COLLAPSED )
            ExpandRow( aRow );
        else if( m_rows[aRow].m_state == ROW_STATE::EXPANDED_PARENT )
            CollapseRow( aRow );
    }


    void CollapseForSort()
    {
        for( size_t i = 0; i < m_rows.size(); ++i )
        {
            if( m_rows[i].m_state == ROW_STATE::EXPANDED_PARENT )
            {
                CollapseRow( i );
                m_rows[i].m_state = ROW_STATE::COLLAPSED_DURING_SORT;
            }
        }
    }


    void ExpandAfterSort()
    {
        for( size_t i = 0; i < m_rows.size(); ++i )
        {
            if( m_rows[i].m_state == ROW_STATE::COLLAPSED_DURING_SORT )
                ExpandRow( i );
        }
    }

    wxString GetGroupedValue( const DATA_MODEL_ROW<ITEM_TYPE>& aRow, int aCol,
                              const wxString& refDelimiter = wxT( ", " ),
                              const wxString& refRangeDelimiter = wxT( "-" ),
                              bool resolveVars = false, bool listMixedValues = false )
    {
        std::vector<ITEM_TYPE> items;
        std::set<wxString>     mixedValues;
        wxString               fieldValue;

        for( const ITEM_TYPE& item : aRow.m_items )
        {
            if( ColIsItemIdentifier( aCol ) || ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
            {
                items.push_back( item );
            }
            else // Other columns are either a single value or ROW_MULTI_ITEMS
            {
                wxString itemFieldValue;
                getStoredFieldValue( item, m_cols[aCol].m_fieldName, itemFieldValue );

                // Show the effective state when a sheet forces it on, but do not change
                // the stored value so the symbol is never stamped on apply.
                if( ColIsAttribute( aCol ) && attributeForcedOnBySheet( item, m_cols[aCol].m_fieldName ) )
                    itemFieldValue = wxS( "1" );

                if( resolveVars )
                {
                    if( ColIsAttribute( aCol ) )
                    {
                        itemFieldValue =
                                getAttributeResolvedValue( m_cols[aCol].m_fieldName, itemFieldValue == wxS( "1" ) );
                    }
                    // Generated fields (e.g. ${FOOTPRINT_LIBRARY}) can't have un-applied values as they're
                    // read-only.  Resolve them against the field.
                    else if( IsGeneratedField( m_cols[aCol].m_fieldName ) )
                    {
                        itemFieldValue = getFieldResolvedLiveValue( item, m_cols[aCol].m_fieldName );
                    }
                    // We have a field that contains both non-variable text and a variable
                    else if( IsGeneratedValue( itemFieldValue ) )
                    {
                        // Resolve variables in the un-applied value using the parent symbol and instance
                        // data.
                        itemFieldValue = resolveTextVars( item, itemFieldValue );
                    }
                }

                if( listMixedValues )
                    mixedValues.insert( itemFieldValue );
                else if( &item == &aRow.m_items.front() )
                    fieldValue = itemFieldValue;
                else if( fieldValue != itemFieldValue )
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

        if( ColIsItemIdentifier( aCol ) || ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
        {
            // Remove duplicates (other units of multi-unit parts)
            std::sort( items.begin(), items.end(),
                    [this]( const ITEM_TYPE& lhs, const ITEM_TYPE& rhs ) -> bool
                    {
                        return cmpRowItems( lhs, rhs );
                    } );

            auto logicalEnd = std::unique( items.begin(), items.end(),
                    [this]( const ITEM_TYPE& lhs, const ITEM_TYPE& rhs ) -> bool
                    {
                        return unitMatch( lhs, rhs );
                    } );

            items.erase( logicalEnd, items.end() );
        }

        // References are the item-identifier column for schematic symbols and footprints and have
        // special formatting for representing reference ranges.  Library item identifiers are joined
        // without reference-range formatting.
        if( ColIsItemIdentifier( aCol ) )
        {
            std::vector<wxString> itemIdentifiers;

            for( const ITEM_TYPE& item : items )
                itemIdentifiers.push_back( getItemIdentifier( item ) );

            // C1-15, C17 kind of formatting
            if( ColIsReference( aCol ) )
            {
                fieldValue = UTIL::FormatRefDesRanges( itemIdentifiers, refDelimiter, refRangeDelimiter );
            }
            else
            {
                fieldValue.clear();

                for( const wxString& itemIdentifier : itemIdentifiers )
                {
                    if( !fieldValue.IsEmpty() )
                        fieldValue += refDelimiter;

                    fieldValue += itemIdentifier;
                }
            }
        }
        else if( ColIsQuantity( aCol ) )
            fieldValue = wxString::Format( wxT( "%d" ), (int) items.size() );
        else if( ColIsItemNumber( aCol ) && aRow.m_state != ROW_STATE::EXPANDED_CHILD )
            fieldValue = wxString::Format( wxT( "%d" ), aRow.m_itemNumber );

        return fieldValue;
    }


    wxString GetValue( int aRow, int aCol ) override
    {
        return GetGroupedValue( m_rows[aRow], aCol );
    }

    wxString GetResolvedValue( int aRow, int aCol ) override
    {
        return GetGroupedValue( m_rows[aRow], aCol, wxT( ", " ), wxT( "-" ), true, false );
    }

    wxString GetExportValue( int aRow, int aCol, const wxString& refDelimiter,
                             const wxString& refRangeDelimiter ) override
    {
        return GetGroupedValue( m_rows[aRow], aCol, refDelimiter, refRangeDelimiter, true, true );
    }

    bool IsCellReadOnly( int aRow, int aCol ) override
    {
        return FIELDS_TABLE_DATA_MODEL_BASE::IsCellReadOnly( aRow, aCol )
               || allRowItemsHaveAttributeForcedOnBySheet( m_rows[aRow], aCol );
    }


protected:
    bool MatchesFilter( const ITEM_TYPE& aItem, const wxString& aReference,
                        EDA_COMBINED_MATCHER& aMatcher )
    {
        if( m_filter.IsEmpty() )
            return true;

        // There is a slight difference between how the reference field is matched between
        // 'reference' mode and all other modes that only applies to multiunit symbols in the
        // symbol fields table.
        //
        // For compatibility, a filter like U1A will match a multiunit U1 that has a unit A
        // in 'reference' mode, but not in any other mode. It won't match a C1A to a C1 that
        // isn't mulitunit.
        //
        // In non-reference mode, because multiunit symbols are all grouped together,
        // e.g. U1A and U1B are in the same row as U1, the filter will only match if the
        // reference is exactly U1.
        //
        // None of this matters for the footprint fields table / other future tables.
        if( m_filterScope == BOM_FILTER_SCOPE::REFERENCE )
            return aMatcher.Find( aReference.Lower() );

        for( size_t i = 0; i < m_cols.size(); ++i )
        {
            const DATA_MODEL_COL& col = m_cols[i];

            if( m_filterScope == BOM_FILTER_SCOPE::VISIBLE && !col.m_show )
                continue;

            wxString value;
            getStoredFieldValue( aItem, col.m_fieldName, value );

            // We want to match on things like DNP and Excluded from BOM when the
            // checkbox is checked
            if( ColIsAttribute( static_cast<int>( i ) ) )
            {
                bool effectiveValue = value == wxS( "1" ) || attributeForcedOnBySheet( aItem, col.m_fieldName );
                value = getAttributeResolvedValue( col.m_fieldName, effectiveValue );
            }
            // For generated fields, we always want to match on the resolved value,
            // not the stored variable
            else if( IsGeneratedField( col.m_fieldName ) )
            {
                value = getFieldResolvedLiveValue( aItem, col.m_fieldName );
            }
            // Same as above, but for field values that contain a mix of text and variables, e.g. "Value: ${VALUE}"
            // The point is to match on what the user can see
            else if( IsGeneratedValue( value ) )
            {
                value = resolveTextVars( aItem, value );
            }

            if( aMatcher.Find( value.Lower() ) )
                return true;
        }

        return false;
    }


    /**
     * Sheets can force all the symbols in them to have certain attributes on, like DNP. They can't force them off.
     *
     * Returns true if the attribute is forced on by a sheet
     */
    virtual bool attributeForcedOnBySheet( const ITEM_TYPE& aItem, const wxString& aAttributeName ) const
    {
        return false;
    }


    bool allRowItemsHaveAttributeForcedOnBySheet( const DATA_MODEL_ROW<ITEM_TYPE>& aRow, int aCol )
    {
        if( !ColIsAttribute( aCol ) || aRow.m_items.empty() )
            return false;

        // Lock the cell only when every symbol in the row inherits it, so a mixed group
        // stays editable and shows the indeterminate state.
        for( const ITEM_TYPE& item : aRow.m_items )
        {
            if( !attributeForcedOnBySheet( item, m_cols[aCol].m_fieldName ) )
                return false;
        }

        return true;
    }

    bool cmpRows( const DATA_MODEL_ROW<ITEM_TYPE>& lhRow, const DATA_MODEL_ROW<ITEM_TYPE>& rhRow,
                  int aSortCol, bool aAscending )
    {
        // Empty rows always go to the bottom, whether ascending or descending
        if( lhRow.m_items.empty() )
            return false;
        else if( rhRow.m_items.empty() )
            return true;

        // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
        // to get the opposite sort.  i.e. ~(a<b) != (a>b)
        auto local_cmp =
                [aAscending]( const auto a, const auto b )
                {
                    if( aAscending )
                        return a < b;
                    else
                        return a > b;
                };

        // Primary sort key is sortCol; secondary is always the item identifier.
        if( aSortCol < 0 || aSortCol >= this->GetNumberCols() )
            aSortCol = 0;

        wxString lhs = this->GetGroupedValue( lhRow, aSortCol, wxT( ", " ), wxT( "-" ), true )
                               .Trim( true )
                               .Trim( false );
        wxString rhs = this->GetGroupedValue( rhRow, aSortCol, wxT( ", " ), wxT( "-" ), true )
                               .Trim( true )
                               .Trim( false );

        if( lhs == rhs || this->ColIsItemIdentifier( aSortCol ) )
        {
            if( aAscending )
                return cmpRowItems( lhRow.m_items[0], rhRow.m_items[0] );
            else
                return cmpRowItems( rhRow.m_items[0], lhRow.m_items[0] );
        }
        else
        {
            return local_cmp( ValueStringCompare( lhs, rhs ), 0 );
        }
    }

    // Used for sorting items that are grouped within a single row by their item identifiers.
    virtual bool cmpRowItems( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem )
    {
        return StrNumCmp( getItemIdentifier( lhItem ), getItemIdentifier( rhItem ), true ) < 0;
    }

    virtual bool unitMatch( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem ) = 0;

    bool groupMatch( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem )
    {
        bool matchFound = false;

        for( size_t i = 0; i < m_cols.size(); ++i )
        {
            if( !m_cols[i].m_group )
                continue;

            int col = static_cast<int>( i );

            // Schematic and PCB references are item identifiers and group by reference prefix.
            // Library item identifiers and library references use ordinary exact matching below.
            if( ColIsItemIdentifier( col ) && ColIsReference( col ) )
            {
                if( UTIL::GetRefDesPrefix( getItemIdentifier( lhItem ) )
                    != UTIL::GetRefDesPrefix( getItemIdentifier( rhItem ) ) )
                {
                    return false;
                }

                matchFound = true;
                continue;
            }

            const wxString& fieldName = m_cols[i].m_fieldName;

            wxString lh;
            wxString rh;
            getStoredFieldValue( lhItem, fieldName, lh );
            getStoredFieldValue( rhItem, fieldName, rh );

            // Normalize attributes so absent and explicitly false values group together, while
            // still honoring edits in the data store and effective values inherited from sheets.
            if( ColIsAttribute( col ) )
            {
                // Sheets can force values like DNP on, but it can't force them off
                bool lhEffectiveValue = lh == wxS( "1" ) || attributeForcedOnBySheet( lhItem, fieldName );
                bool rhEffectiveValue = rh == wxS( "1" ) || attributeForcedOnBySheet( rhItem, fieldName );

                if( lhEffectiveValue != rhEffectiveValue )
                    return false;

                matchFound = true;
                continue;
            }
            // If the field is generated (e.g. ${QUANTITY}), we need to resolve it through the
            // item to get the actual current value; otherwise we need to pull it out of the store
            // so the refresh can regroup based on values that haven't been applied yet.
            else if( IsGeneratedField( fieldName ) )
            {
                lh = getFieldResolvedLiveValue( lhItem, fieldName );
                rh = getFieldResolvedLiveValue( rhItem, fieldName );
            }
            // If we're not generated, we might still have a variable reference in the value,
            // e.g. "10K ${TOLERANCE}", which still needs to be resolved
            else
            {
                if( IsGeneratedValue( lh ) )
                    lh = resolveTextVars( lhItem, lh );
                if( IsGeneratedValue( rh ) )
                    rh = resolveTextVars( rhItem, rh );
            }

            if( lh != rh )
                return false;

            matchFound = true;
        }

        return matchFound;
    }

    void Sort()
    {
        CollapseForSort();

        // We're going to sort the rows based on their first item, so the first item identifier had
        // better be the lowest one.
        for( DATA_MODEL_ROW<ITEM_TYPE>& row : m_rows )
        {
            std::sort( row.m_items.begin(), row.m_items.end(),
                    [this]( const ITEM_TYPE& lhs, const ITEM_TYPE& rhs ) -> bool
                    {
                        return cmpRowItems( lhs, rhs );
                    } );
        }

        std::sort( m_rows.begin(), m_rows.end(),
                   [this]( const DATA_MODEL_ROW<ITEM_TYPE>& lhs, const DATA_MODEL_ROW<ITEM_TYPE>& rhs ) -> bool
                   {
                       return cmpRows( lhs, rhs, m_sortColumn, m_sortAscending );
                   } );

        // Time to renumber the item numbers
        int itemNumber = 1;

        for( DATA_MODEL_ROW<ITEM_TYPE>& row : m_rows )
        {
            row.m_itemNumber = itemNumber++;
        }

        ExpandAfterSort();
    }


    /**
     * Gets the current value of the field from the live item, e.g. from the symbol on the schematic.
     *
     * @return True if the field is present in the live item, false if it is not present (and
     *         aValue will be empty).
     */
    virtual bool getLiveFieldValue( const ITEM_TYPE& aItem, const wxString& aFieldName, wxString& aValue ) = 0;

    /**
     * Returns all stored fields for an item without creating a data-store entry.
     */
    const std::map<wxString, wxString>& getStoredFields( const ITEM_TYPE& aItem ) const
    {
        static const std::map<wxString, wxString> emptyFields;

        auto itemIt = m_dataStore.find( getDataStoreKey( aItem ) );

        if( itemIt == m_dataStore.end() )
            return emptyFields;

        return itemIt->second;
    }

    /**
     * Gets the stored value of the field from the data store, not from the live item,
     * e.g. from the symbol on the schematic.
     *
     * @return True if the field is present in the data store, false if it is not present (and aValue will be empty).
     */
    bool getStoredFieldValue( const ITEM_TYPE& aItem, const wxString& aFieldName, wxString& aValue ) const
    {
        aValue.clear();

        const std::map<wxString, wxString>& fields = getStoredFields( aItem );
        auto                                fieldIt = fields.find( aFieldName );

        if( fieldIt == fields.end() )
            return false;

        aValue = fieldIt->second;
        return true;
    }

    /**
     * Creates or updates a field in the data store. Always marks the field present as a result.
     */
    void setStoredFieldValue( const ITEM_TYPE& aItem, const wxString& aFieldName,
                              const wxString& aValue )
    {
        m_dataStore[getDataStoreKey( aItem )][aFieldName] = aValue;
    }

    /**
     * Makes sure a field is at least marked present-but-empty without changing its value
     * if it has one already
     */
    void ensureStoredFieldPresent( const ITEM_TYPE& aItem, const wxString& aFieldName )
    {
        m_dataStore[getDataStoreKey( aItem )].try_emplace( aFieldName, wxEmptyString );
    }

    /**
     * Clears a field, e.g. marks in not-present (as opposed to an empty string)
     */
    void clearStoredField( const ITEM_TYPE& aItem, const wxString& aFieldName )
    {
        auto itemIt = m_dataStore.find( getDataStoreKey( aItem ) );

        if( itemIt != m_dataStore.end() )
            itemIt->second.erase( aFieldName );
    }

    /**
     * Compares the live value of the field to the stored value in the data store.
     *
     * If they differ in presence or value then the field has been modified
     */
    bool fieldIsModified( const ITEM_TYPE& aItem, const wxString& aFieldName )
    {
        wxString liveValue;
        wxString storedValue;
        bool     liveFieldPresent = getLiveFieldValue( aItem, aFieldName, liveValue );
        bool     storedFieldPresent = getStoredFieldValue( aItem, aFieldName, storedValue );

        return liveFieldPresent != storedFieldPresent || liveValue != storedValue;
    }


    /**
     * Updates the data store with the current value of the field from the live item.
     *
     * If the field is not present on the item, it will be cleared from the data store rather than
     * set to empty.
     */
    void updateDataStoreItemFieldFromLive( const ITEM_TYPE& aItem, const wxString& aFieldName )
    {
        wxString value;

        if( getLiveFieldValue( aItem, aFieldName, value ) )
            setStoredFieldValue( aItem, aFieldName, value );
        else
            clearStoredField( aItem, aFieldName );
    }

    /**
     * Puts the live value of the field in the data store, correctly handling a missing
     * field in the souce item as empty or not-present based on whether the col was added
     * by the user
     */
    void initializeDataStoreItemField( const ITEM_TYPE& aItem, const DATA_MODEL_COL& aCol )
    {
        updateDataStoreItemFieldFromLive( aItem, aCol.m_fieldName );

        if( aCol.m_userAdded )
            ensureStoredFieldPresent( aItem, aCol.m_fieldName );
    }

    /**
     * Initializes every column in the data store for a newly added (or refreshed) item.
     *
     * Like the field initializer, this will correctly handle missing fields in the source item
     * as empty or not-present based on the column user-added status
     */
    void initializeDataStoreItem( const ITEM_TYPE& aItem )
    {
        for( const DATA_MODEL_COL& col : m_cols )
            initializeDataStoreItemField( aItem, col );
    }

    virtual std::vector<ITEM_TYPE> getAllItems() const = 0;

public:
    virtual KIID_PATH getDataStoreKey( const ITEM_TYPE& aItem ) const = 0;
    virtual wxString  getItemIdentifier( const ITEM_TYPE& aItem ) const = 0;

    /**
     * Explicitly bypasses the data store's field values and retries them from
     * the item's current field value on the editing canvas. Will also handle generated
     * fields as necessary. Returns empty string for private/missing fields.
     *
     * Example: BOM template provides ${DNP} as a field, but they symbol doesn't have the field.
     *
     * @return Resolved display text for the field's live value from the canvas.
     */
    virtual wxString getFieldResolvedLiveValue( const ITEM_TYPE& aItem, const wxString& aFieldName ) = 0;

    virtual wxString resolveTextVars( const ITEM_TYPE& aItem, const wxString& aText ) = 0;

protected:
    std::vector<DATA_MODEL_ROW<ITEM_TYPE>> m_rows;
};
