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
#include <vector>
#include <algorithm>
#include <map>

#include <widgets/wx_grid.h>
#include <widgets/ui_common.h>

#include <common.h>
#include <kiid.h>
#include <refdes_utils.h>
#include <string_utils.h>
#include <wx/debug.h>


struct BOM_FIELD;
struct BOM_PRESET;
struct BOM_FMT_PRESET;


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

    bool IsExpanderColumn( int aCol ) const override;

    void SetSorting( int aCol, bool aAscending );
    int  GetSortCol() { return m_sortColumn; }
    bool GetSortAsc() { return m_sortAscending; }

    // These are used to disable RebuildRows() while generating batches of UI events, e.g.
    // applying a BOM preset, that would otherwise thrash the grid.
    void         EnableRebuilds();
    void         DisableRebuilds();
    virtual void RebuildRows() = 0;

    void            SetFilter( const wxString& aFilter ) { m_filter = aFilter; }
    const wxString& GetFilter() { return m_filter; }

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
    // Helper functions to deal with translating wxGrid values to and from
    // named field values like ${DNP}
    bool isAttribute( const wxString& aFieldName );

    virtual bool isCellReadOnly( int aRow, int aCol );

protected:
    bool     m_edited;
    int      m_sortColumn;
    bool     m_sortAscending;
    wxString m_filter;
    bool     m_groupingEnabled;
    bool     m_excludeDNP;
    bool     m_includeExcluded;
    bool     m_rebuildsEnabled;

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
    int GetNumberRows() override { return (int) m_rows.size(); }

    ROW_STATE GetRowState( int aRow ) const override { return m_rows[aRow].m_state; }

    std::vector<ITEM_TYPE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), std::vector<ITEM_TYPE>() );
        return m_rows[aRow].m_items;
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
            if( ColIsReference( aCol ) || ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
            {
                items.push_back( item );
            }
            else // Other columns are either a single value or ROW_MULTI_ITEMS
            {
                KIID_PATH key = getDataStoreKey( item );

                if( !m_dataStore.contains( key ) || !m_dataStore[key].contains( m_cols[aCol].m_fieldName ) )
                    return INDETERMINATE_STATE;

                wxString itemFieldValue = m_dataStore[key][m_cols[aCol].m_fieldName];

                // Show the effective state when a sheet forces it on, but do not change
                // the stored value so the symbol is never stamped on apply.
                if( ColIsAttribute( aCol ) && attributeInheritedFromSheet( item, m_cols[aCol].m_fieldName ) )
                    itemFieldValue = wxS( "1" );

                if( resolveVars )
                {
                    // Generated fields (e.g. ${FOOTPRINT_LIBRARY}) can't have un-applied values as they're
                    // read-only.  Resolve them against the field.
                    if( IsGeneratedField( m_cols[aCol].m_fieldName ) )
                    {
                        itemFieldValue = getFieldResolvedLiveValue( item, m_cols[aCol].m_fieldName );
                    }
                    // We have a field that contains both non-variable text and a variable
                    else if( itemFieldValue.Contains( wxT( "${" ) ) )
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

        if( ColIsReference( aCol ) || ColIsQuantity( aCol ) || ColIsItemNumber( aCol ) )
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

        if( ColIsReference( aCol ) )
        {
            std::vector<wxString> references;

            for( const ITEM_TYPE& item : items )
                references.push_back( getItemReference( item ) );

            fieldValue = UTIL::FormatRefDesRanges( references, refDelimiter, refRangeDelimiter );
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


protected:
    virtual bool attributeInheritedFromSheet( const ITEM_TYPE& aItem, const wxString& aAttributeName ) const
    {
        return false;
    }


    bool rowAttributeInheritedFromSheet( const DATA_MODEL_ROW<ITEM_TYPE>& aRow, int aCol )
    {
        if( !ColIsAttribute( aCol ) || aRow.m_items.empty() )
            return false;

        // Lock the cell only when every symbol in the row inherits it, so a mixed group
        // stays editable and shows the indeterminate state.
        for( const ITEM_TYPE& item : aRow.m_items )
        {
            if( !attributeInheritedFromSheet( item, m_cols[aCol].m_fieldName ) )
                return false;
        }

        return true;
    }

    bool isCellReadOnly( int aRow, int aCol ) override
    {
        return FIELDS_TABLE_DATA_MODEL_BASE::isCellReadOnly( aRow, aCol )
               || rowAttributeInheritedFromSheet( m_rows[aRow], aCol );
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

        // Primary sort key is sortCol; secondary is always REFERENCE (column 0)
        if( aSortCol < 0 || aSortCol >= this->GetNumberCols() )
            aSortCol = 0;

        wxString lhs = this->GetGroupedValue( lhRow, aSortCol, wxT( ", " ), wxT( "-" ), true )
                               .Trim( true )
                               .Trim( false );
        wxString rhs = this->GetGroupedValue( rhRow, aSortCol, wxT( ", " ), wxT( "-" ), true )
                               .Trim( true )
                               .Trim( false );

        if( lhs == rhs || this->ColIsReference( aSortCol ) )
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

    // Used for sorting row items that are grouped with a single row, e.g. the references
    virtual bool cmpRowItems( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem )
    {
        return StrNumCmp( getItemReference( lhItem ), getItemReference( rhItem ), true ) < 0;
    }

    virtual bool unitMatch( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem ) = 0;

    bool groupMatch( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem )
    {
        int  refCol = -1;
        bool matchFound = false;

        for( size_t i = 0; i < m_cols.size(); ++i )
        {
            if( ColIsReference( static_cast<int>( i ) ) )
            {
                refCol = static_cast<int>( i );
                break;
            }
        }

        if( refCol == -1 )
            return false;

        // First check the reference column.  This can be done directly from the items as
        // references can't be edited in the grid.
        if( m_cols[refCol].m_group )
        {
            // If we're grouping by reference, then only the prefix must match.
            if( UTIL::GetRefDesPrefix( getItemReference( lhItem ) )
                != UTIL::GetRefDesPrefix( getItemReference( rhItem ) ) )
            {
                return false;
            }

            matchFound = true;
        }

        KIID_PATH lhItemKey = getDataStoreKey( lhItem );
        KIID_PATH rhItemKey = getDataStoreKey( rhItem );

        // Now check all the other columns.
        for( size_t i = 0; i < m_cols.size(); ++i )
        {
            // Handled already
            if( static_cast<int>( i ) == refCol )
                continue;

            if( !m_cols[i].m_group )
                continue;

            wxString fieldName = m_cols[i].m_fieldName;

            wxString lh = m_dataStore[lhItemKey][fieldName];
            wxString rh = m_dataStore[rhItemKey][fieldName];

            // If the field is generated (e.g. ${QUANTITY}), we need to resolve it through the
            // item to get the actual current value; otherwise we need to pull it out of the store
            // so the refresh can regroup based on values that haven't been applied yet.
            if( IsGeneratedField( fieldName ) )
            {
                lh = getFieldResolvedLiveValue( lhItem, fieldName );
                rh = getFieldResolvedLiveValue( rhItem, fieldName );
            }
            // If we're not generated, we might still have a variable reference in the value,
            // e.g. "10K ${TOLERANCE}", which still needs to be resolved
            else
            {
                if( lh.Contains( wxT( "${" ) ) )
                    lh = resolveTextVars( lhItem, lh );
                if( rh.Contains( wxT( "${" ) ) )
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

        // We're going to sort the rows based on their first reference, so the first reference
        // had better be the lowest one.
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

    virtual KIID_PATH getDataStoreKey( const ITEM_TYPE& aItem ) const = 0;
    virtual wxString  getItemReference( const ITEM_TYPE& aItem ) const = 0;

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
