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

#include <vector>
#include <algorithm>

#include <widgets/wx_grid.h>


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


class FIELDS_TABLE_DATA_MODEL_BASE : public WX_GRID_TABLE_BASE
{
public:
    FIELDS_TABLE_DATA_MODEL_BASE();

    static const wxString QUANTITY_VARIABLE;
    static const wxString ITEM_NUMBER_VARIABLE;

    virtual void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser ) = 0;

    void MoveColumn( int aCol, int aNewPos );

    int GetNumberCols() override { return static_cast<int>( m_cols.size() ); }

    void     SetColLabelValue( int aCol, const wxString& aLabel ) override;
    wxString GetColLabelValue( int aCol ) override;
    wxString GetColFieldName( int aCol );
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

    virtual wxString GetExportValue( int aRow, int aCol, const wxString& aRefDelimiter,
                                     const wxString& aRefRangeDelimiter ) = 0;

protected:
    int      m_sortColumn;
    bool     m_sortAscending;
    wxString m_filter;
    bool     m_groupingEnabled;
    bool     m_excludeDNP;
    bool     m_includeExcluded;
    bool     m_rebuildsEnabled;

    std::vector<DATA_MODEL_COL> m_cols;
};


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


    void ExpandCollapseRow( int aRow )
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


protected:
    virtual bool cmpRows( const DATA_MODEL_ROW<ITEM_TYPE>& lhRow, const DATA_MODEL_ROW<ITEM_TYPE>& rhRow,
                     int aSortCol, bool aAscending ) = 0;
    // Used for sorting row items that are grouped with a single row, e.g. the references
    virtual bool cmpRowItems( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem ) = 0;
    virtual bool unitMatch( const ITEM_TYPE& lhItem, const ITEM_TYPE& rhItem ) = 0;

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


protected:
    std::vector<DATA_MODEL_ROW<ITEM_TYPE>> m_rows;
};
