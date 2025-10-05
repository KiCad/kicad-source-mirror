/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <sch_reference_list.h>
#include <wx/grid.h>
#include <wx/arrstr.h>
#include <widgets/grid_striped_renderer.h>
#include <fields_data_model.h>


struct LIB_DATA_MODEL_ROW
{
    LIB_DATA_MODEL_ROW( const LIB_SYMBOL* aFirstReference, GROUP_TYPE aType ) :
            m_ItemNumber( 0 ),
            m_Flag( aType ),
            m_Refs( { aFirstReference } )
    {
    }

    int                            m_ItemNumber;
    GROUP_TYPE                     m_Flag;
    std::vector<const LIB_SYMBOL*> m_Refs;
};


struct LIB_DATA_MODEL_COL
{
    wxString m_fieldName;
    wxString m_label;
    bool     m_userAdded;
    bool     m_show;
    bool     m_group;
    bool     m_isCheckbox;
};


struct LIB_DATA_ELEMENT
{
    LIB_DATA_ELEMENT()
    {
        m_originalData = wxEmptyString;
        m_currentData = wxEmptyString;
        m_originallyEmpty = false;
        m_currentlyEmpty = false;
        m_isModified = false;
        m_isStriped = false;
        m_createDerivedSymbol = false;
        m_derivedSymbolName = wxEmptyString;
    }

    wxString m_originalData;
    wxString m_currentData;
    bool     m_originallyEmpty;
    bool     m_currentlyEmpty;
    bool     m_isModified;
    bool     m_isStriped;
    bool     m_createDerivedSymbol;
    wxString m_derivedSymbolName;
};


class LIB_FIELDS_EDITOR_GRID_DATA_MODEL : public WX_GRID_TABLE_BASE
{
public:
    LIB_FIELDS_EDITOR_GRID_DATA_MODEL() :
            m_edited( false ),
            m_sortColumn( 0 ),
            m_sortAscending( false ),
            m_filter( wxEmptyString ),
            m_groupingEnabled( false ),
            m_stripedStringRenderer( nullptr )
    {
    }

    ~LIB_FIELDS_EDITOR_GRID_DATA_MODEL() override
    {
        for( auto& pair : m_stripedRenderers )
            pair.second->DecRef();

        m_stripedRenderers.clear();
    }

    static const wxString ITEM_NUMBER_VARIABLE;
    static const wxString SYMBOL_NAME;

    void CreateDerivedSymbol( int aRow, int aCol, wxString& aNewSymbolName );
    void CreateDerivedSymbolImmediate( int aRow, int aCol, wxString& aNewSymbolName );

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser, bool aIsCheckbox );
    void RemoveColumn( int aCol );
    void RenameColumn( int aCol, const wxString& newName );

    void MoveColumn( int aCol, int aNewPos )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );

        if( aCol == aNewPos )
        {
            return;
        }
        else if( aCol < aNewPos )
        {
            std::rotate( std::begin( m_cols ) + aCol, std::begin( m_cols ) + aCol + 1,
                         std::begin( m_cols ) + aNewPos + 1 );
        }
        else
        {
            std::rotate( std::begin( m_cols ) + aNewPos, std::begin( m_cols ) + aCol,
                         std::begin( m_cols ) + aCol + 1 );
        }
    }

    int GetNumberRows() override { return (int) m_rows.size(); }
    int GetNumberCols() override { return (int) m_cols.size(); }

    void SetColLabelValue( int aCol, const wxString& aLabel ) override
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_cols[aCol].m_label = aLabel;
    }

    wxString GetColLabelValue( int aCol ) override
    {
        wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), wxString() );
        return m_cols[aCol].m_label;
    }

    wxString GetColFieldName( int aCol )
    {
        wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), wxString() );
        return m_cols[aCol].m_fieldName;
    }

    int GetFieldNameCol( const wxString& aFieldName );

    void SetFieldsOrder( const std::vector<wxString>& aNewOrder );

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override;
    wxString GetTypeName( int row, int col ) override;
    wxString GetValue( const LIB_DATA_MODEL_ROW& group, int aCol );
    void     SetValue( int aRow, int aCol, const wxString& aValue ) override;

    wxGridCellAttr* GetAttr( int row, int col, wxGridCellAttr::wxAttrKind kind ) override;
    void            RevertRow( int aRow );
    void            ClearCell( int aRow, int aCol );

    bool ColIsSymbolName( int aCol );
    bool ColIsCheck( int aCol );

    bool IsExpanderColumn( int aCol ) const override;
    GROUP_TYPE GetGroupType( int aRow ) const override
    {
        return m_rows[aRow].m_Flag;
    }

    void SetSorting( int aCol, bool ascending )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_sortColumn = aCol;
        m_sortAscending = ascending;
    }

    int  GetSortCol() { return m_sortColumn; }
    bool GetSortAsc() { return m_sortAscending; }

    const LIB_SYMBOL* GetSymbolForRow( int aRow )
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), nullptr );
        return m_rows[aRow].m_Refs[0];
    }

    void GetSymbolNames( wxArrayString& aList )
    {
        aList.Clear();

        for( const LIB_SYMBOL* symbol : m_symbolsList )
            aList.Add( symbol->GetName() );
    }

    void SetSymbols( const std::vector<LIB_SYMBOL*>& aSymbolsList ) { m_symbolsList = aSymbolsList; }
    void RebuildRows();

    void ExpandRow( int aRow );
    void CollapseRow( int aRow );
    void ExpandCollapseRow( int aRow );
    void CollapseForSort();
    void ExpandAfterSort();

    void ApplyData( std::function<void( LIB_SYMBOL* )> symbolChangeHandler,
                    std::function<void()> postApplyHandler = nullptr );

    /// Get and clear the list of newly created derived symbols for library manager processing
    std::vector<std::pair<LIB_SYMBOL*, wxString>> GetAndClearCreatedDerivedSymbols()
    {
        auto result = std::move( m_createdDerivedSymbols );
        m_createdDerivedSymbols.clear();
        return result;
    }

    bool IsEdited() { return m_edited; }

    int GetDataWidth( int aCol );

    void SetFilter( const wxString& aFilter ) { m_filter = aFilter; }
    const wxString& GetFilter() { return m_filter; }

    void SetGroupingEnabled( bool group ) { m_groupingEnabled = group; }
    bool GetGroupingEnabled() { return m_groupingEnabled; }

    void SetGroupColumn( int aCol, bool group )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_cols[aCol].m_group = group;
    }

    bool GetGroupColumn( int aCol )
    {
        wxCHECK_MSG( aCol >= 0 && aCol < (int) m_cols.size(), false, "Invalid Column Number" );
        return m_cols[aCol].m_group;
    }

    void SetShowColumn( int aCol, bool show )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_cols[aCol].m_show = show;
    }

    bool GetShowColumn( int aCol )
    {
        wxCHECK_MSG( aCol >= 0 && aCol < (int) m_cols.size(), false, "Invalid Column Number" );
        return m_cols[aCol].m_show;
    }

    bool IsRowEditable( int aRow )
    {
        wxCHECK_MSG( aRow >= 0 && aRow < (int) m_rows.size(), false, "Invalid Row Number" );
        return m_rows[aRow].m_Flag == GROUP_SINGLETON || m_rows[aRow].m_Flag == GROUP_SINGLETON;
    }

    bool IsCellEdited( int aRow, int aCol )
    {
        wxCHECK_MSG( aRow >= 0 && aRow < (int) m_rows.size(), false, "Invalid Row Number" );
        wxCHECK_MSG( aCol >= 0 && aCol < (int) m_cols.size(), false, "Invalid Column Number" );
        return m_dataStore[m_rows[aRow].m_Refs[0]->m_Uuid][m_cols[aCol].m_fieldName].m_isModified;
    }

    bool IsCellClear( int aRow, int aCol )
    {
        wxCHECK_MSG( aRow >= 0 && aRow < (int) m_rows.size(), false, "Invalid Row Number" );
        wxCHECK_MSG( aCol >= 0 && aCol < (int) m_cols.size(), false, "Invalid Column Number" );
        return m_dataStore[m_rows[aRow].m_Refs[0]->m_Uuid][m_cols[aCol].m_fieldName].m_currentlyEmpty;
    }

    bool IsRowSingleSymbol( int aRow )
    {
        wxCHECK_MSG( aRow >= 0 && aRow < (int) m_rows.size(), false, "Invalid Row Number" );
        return m_rows[aRow].m_Flag == GROUP_SINGLETON || m_rows[aRow].m_Flag == CHILD_ITEM;
    }

private:
    static bool cmp( const LIB_DATA_MODEL_ROW& lhGroup, const LIB_DATA_MODEL_ROW& rhGroup,
                     LIB_FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol, bool ascending );

    bool groupMatch( const LIB_SYMBOL* lhRef, const LIB_SYMBOL* rhRef );
    wxString getAttributeValue( const LIB_SYMBOL*, const wxString& aAttributeName );
    void setAttributeValue( LIB_SYMBOL* aSymbol, const wxString& aAttributeName, const wxString& aValue );

    void createActualDerivedSymbol( const LIB_SYMBOL* aParentSymbol, const wxString& aNewSymbolName,
                                    const KIID& aNewSymbolUuid );

    void Sort();

    void updateDataStoreSymbolField( const LIB_SYMBOL* aSymbol, const wxString& aFieldName );

    bool isStripeableField( int aCol );
    wxGridCellRenderer* getStripedRenderer( int aCol ) const;

protected:
    std::vector<LIB_SYMBOL*> m_symbolsList;
    bool                     m_edited;
    int                      m_sortColumn;
    bool                     m_sortAscending;
    wxString                 m_filter;
    bool                     m_groupingEnabled;

    std::vector<LIB_DATA_MODEL_COL> m_cols;
    std::vector<LIB_DATA_MODEL_ROW> m_rows;

    // Data store
    // The data model is fundamentally symbols X fieldNames.
    // A map of symbolID : fieldSet, where fieldSet is a map of fieldName : LIB_DATA_ELEMENT
    std::map<KIID, std::map<wxString, LIB_DATA_ELEMENT>> m_dataStore;

    // Track newly created derived symbols for library manager integration
    std::vector<std::pair<LIB_SYMBOL*, wxString>> m_createdDerivedSymbols; // symbol, library name

    // stripe bitmap support
    mutable STRIPED_STRING_RENDERER* m_stripedStringRenderer;
    mutable std::map<wxString, wxGridCellRenderer*> m_stripedRenderers;
};
