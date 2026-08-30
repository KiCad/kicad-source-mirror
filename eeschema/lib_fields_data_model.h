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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <sch_reference_list.h>
#include <wx/grid.h>
#include <wx/arrstr.h>
#include <widgets/wx_grid.h>

#include <fields_table_data_model.h>

#include <symbol_library_manager.h>


using LIB_FIELDS_TABLE_DATA_MODEL_ROW = DATA_MODEL_ROW<LIB_SYMBOL*>;


class LIB_FIELDS_EDITOR_GRID_DATA_MODEL : public FIELDS_TABLE_DATA_MODEL<LIB_SYMBOL*>
{
public:
    enum SCOPE : int
    {
        SCOPE_LIBRARY = 0,
        SCOPE_RELATED_SYMBOLS
    };

    LIB_FIELDS_EDITOR_GRID_DATA_MODEL( const std::vector<LIB_SYMBOL*>& aSymbolsList ) :
            m_symbolsList( aSymbolsList ),
            m_scope( SCOPE_LIBRARY )
    {
        m_includeExcluded = true;
    }

    static const wxString SYMBOL_NAME;
    static const wxString SYMBOL_KEYWORDS;
    static const wxString SYMBOL_IS_POWER;
    static const wxString SYMBOL_IS_LOCAL_POWER;

    void CreateDerivedSymbolImmediate( int aRow, int aCol, wxString& aNewSymbolName );

    wxString GetTypeName( int row, int col ) override;
    void     SetValue( int aRow, int aCol, const wxString& aValue ) override;

    wxGridCellAttr* GetAttr( int row, int col, wxGridCellAttr::wxAttrKind kind ) override;

    bool ColIsItemIdentifier( int aCol ) const override;

    const LIB_SYMBOL* GetSymbolForRow( int aRow )
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), nullptr );
        return m_rows[aRow].m_items[0];
    }

    void GetSymbolNames( wxArrayString& aList, SYMBOL_NAME_FILTER aFilter )
    {
        aList.Clear();

        for( const LIB_SYMBOL* symbol : m_symbolsList )
        {
            if( ( symbol->IsDerived() && ( aFilter == SYMBOL_NAME_FILTER::ROOT_ONLY ) )
                || ( symbol->IsRoot() && ( aFilter == SYMBOL_NAME_FILTER::DERIVED_ONLY ) ) )
            {
                continue;
            }

            aList.Add( UnescapeString( symbol->GetName() ) );
        }
    }

    void RebuildRows() override;

    void  SetScope( SCOPE aScope ) { m_scope = aScope; }
    SCOPE GetScope() { return m_scope; }

    void SetRelatedSymbolRoot( const wxString& aRootSymbolName ) { m_relatedSymbolRoot = aRootSymbolName; }

    void ApplyData( std::function<void( LIB_SYMBOL* )> symbolChangeHandler,
                    std::function<void()> postApplyHandler = nullptr );

    /// Get and clear the list of newly created derived symbols for library manager processing
    std::vector<std::pair<LIB_SYMBOL*, wxString>> GetAndClearCreatedDerivedSymbols()
    {
        auto result = std::move( m_createdDerivedSymbols );
        m_createdDerivedSymbols.clear();
        return result;
    }

    bool IsRowSingleSymbol( int aRow )
    {
        wxCHECK_MSG( aRow >= 0 && aRow < (int) m_rows.size(), false, "Invalid Row Number" );
        return m_rows[aRow].m_state == ROW_STATE::NON_EXPANDABLE
               || m_rows[aRow].m_state == ROW_STATE::EXPANDED_CHILD;
    }

    bool IsCellReadOnly( int aRow, int aCol ) override;

private:
    bool unitMatch( LIB_SYMBOL* const& lhItem, LIB_SYMBOL* const& rhItem ) override;

    bool fieldIsAttribute( const wxString& aFieldName ) const override;
    bool fieldIsItemProperty( const wxString& aFieldName ) const override;

    wxString getAttributeValue( const LIB_SYMBOL*, const wxString& aAttributeName );
    void setAttributeValue( LIB_SYMBOL* aSymbol, const wxString& aAttributeName, const wxString& aValue );
    wxString getAttributeResolvedValue( const wxString& aFieldName, bool aValue ) const override;

    bool isPowerSymbolControlledField( const wxString& aFieldName ) const;
    bool getStoredPowerSymbolValue( LIB_SYMBOL* aSymbol ) const;
    void setPowerSymbolDefaults( LIB_SYMBOL* aSymbol );

    wxString getFieldResolvedLiveValue( LIB_SYMBOL* const& aSymbol, const wxString& aFieldName ) override;
    wxString resolveTextVars( LIB_SYMBOL* const& aSymbol, const wxString& aText ) override;

    void createActualDerivedSymbol( const LIB_SYMBOL* aParentSymbol, const wxString& aNewSymbolName,
                                    const KIID& aNewSymbolUuid );

    bool getLiveFieldValue( LIB_SYMBOL* const& aSymbol, const wxString& aFieldName, wxString& aValue ) override;
    std::vector<LIB_SYMBOL*> getAllItems() const override;

    KIID_PATH getDataStoreKey( LIB_SYMBOL* const& aItem ) const override;
    wxString  getItemIdentifier( LIB_SYMBOL* const& aItem ) const override;

protected:
    std::vector<LIB_SYMBOL*> m_symbolsList;
    SCOPE                    m_scope;
    wxString                 m_relatedSymbolRoot;

    // Track newly created derived symbols for library manager integration
    std::vector<std::pair<LIB_SYMBOL*, wxString>> m_createdDerivedSymbols; // symbol, library name
};
