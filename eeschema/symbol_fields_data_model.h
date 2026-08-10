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

#include <fields_table_data_model.h>
#include <sch_reference_list.h>


class SCH_SYMBOL;


struct FIELD_CASE_CONFLICT
{
    SCH_SYMBOL*                                symbol;
    SCH_SHEET_PATH                             sheetPath;
    wxString                                   reference;
    wxString                                   caseFoldedKey;
    std::vector<std::pair<wxString, wxString>> variants;
};


std::vector<FIELD_CASE_CONFLICT> DetectFieldCaseConflicts( const SCH_REFERENCE_LIST& aSymbols );

using SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW = DATA_MODEL_ROW<SCH_REFERENCE>;

class SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL : public FIELDS_TABLE_DATA_MODEL<SCH_REFERENCE>
{
public:
    enum SCOPE : int
    {
        SCOPE_ALL = 0,
        SCOPE_SHEET,
        SCOPE_SHEET_RECURSIVE
    };

    SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL( const SCH_REFERENCE_LIST& aSymbolsList, wxGridCellAttr* aURLEditor ) :
            m_symbolsList( aSymbolsList ),
            m_scope( SCOPE_ALL ),
            m_urlEditor( aURLEditor ),
            m_textVarRenderer( nullptr )
    {
        m_symbolsList.SplitReferences();
    }

    ~SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL() override
    {
        wxSafeDecRef( m_urlEditor );
        wxSafeDecRef( m_textVarRenderer );
    }

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser ) override;

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    void RebuildRows() override;

    void ApplyData( SCH_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames, const wxString& aVariantName );

    void  SetScope( SCOPE aScope ) { m_scope = aScope; }
    SCOPE GetScope() { return m_scope; }

    void                  SetPath( const SCH_SHEET_PATH& aPath ) { m_path = aPath; }
    const SCH_SHEET_PATH& GetPath() { return m_path; }

    void AddReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveSymbol( const SCH_SYMBOL& aSymbol );
    void UpdateReferences( const SCH_REFERENCE_LIST& aRefs );

    bool DeleteRows( size_t aPosition = 0, size_t aNumRows = 1 ) override;

    const SCH_REFERENCE_LIST& GetReferenceList() const { return m_symbolsList; }

private:
    bool unitMatch( const SCH_REFERENCE& lhItem, const SCH_REFERENCE& rhItem ) override;

    /**
     * Test whether a field's storage is common to every sheet path that reaches a symbol.
     *
     * Shared sheets present one symbol under several paths.  Edits to storage they have in
     * common must be recorded against all of those paths, while variant overrides belong to
     * a single symbol instance.
     *
     * @param aFieldName is the canonical field or attribute name.
     * @retval true if the field is stored on the symbol.
     * @retval false if the field is stored on the symbol instance.
     */
    bool storageIsSharedAcrossPaths( const wxString& aFieldName ) const;

    // True when an ancestor sheet forces this attribute on, not the symbol itself.
    bool attributeInheritedFromSheet( const SCH_REFERENCE& aRef, const wxString& aAttributeName ) const override;
    bool rowAttributeInheritedFromSheet( const SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& aRow, int aCol );
    bool isCellReadOnly( int aRow, int aCol ) override;

    wxString getAttributeValue( const SCH_REFERENCE& aRef, const wxString& aAttributeName,
                                const wxString& aVariantNames );
    wxString getFieldValueForVariant( const SCH_REFERENCE& aRef, const wxString& aFieldName,
                                      const wxString& aVariantName );

    /**
     * Get the default (non-variant) value for a field.
     *
     * This retrieves the field value as it would appear without any variant override.
     *
     * @param aRef The symbol reference.
     * @param aFieldName The name of the field.
     * @return The default field value.
     */
    wxString getDefaultFieldValue( const SCH_REFERENCE& aRef, const wxString& aFieldName );

    /**
     * Set the attribute value.
     *
     * @param aReference is a reference to the symbol to set the attribute.
     * @param aAttributeName is the name of the symbol attribute.
     * @param aValue is the value to set the attribute.
     * @param aVariantName is an optional variant name to set the variant attribute.
     * @retval true if the symbol attribute value has changed.
     * @retval false if the symbol attribute has **not** changed.
     */
    bool setAttributeValue( SCH_REFERENCE& aRef, const wxString& aAttributeName, const wxString& aValue,
                            const wxString& aVariantName = wxEmptyString );

    wxString getFieldResolvedLiveValue( const SCH_REFERENCE& aRef, const wxString& aFieldName ) override;
    wxString resolveTextVars( const SCH_REFERENCE& aRef, const wxString& aText ) override;

    void updateDataStoreSymbolField( const SCH_REFERENCE& aSymbolRef, const wxString& aFieldName );

    KIID_PATH getDataStoreKey( const SCH_REFERENCE& aItem ) const override;
    wxString  getItemReference( const SCH_REFERENCE& aItem ) const override;

protected:
    /**
     * The flattened by hierarchy list of symbols.
     *
     * @warning This list **must** be kept sorted by symbol pointer.  Otherwise, the undo/redo
     *          commit actions will be broken.
     */
    SCH_REFERENCE_LIST    m_symbolsList;
    SCOPE                 m_scope;
    SCH_SHEET_PATH        m_path;
    wxGridCellAttr*       m_urlEditor;
    wxGridCellRenderer*   m_textVarRenderer; ///< Renderer for cells with text variable references
};
