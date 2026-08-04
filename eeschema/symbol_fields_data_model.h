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


struct SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW
{
    SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW( const SCH_REFERENCE& aFirstReference, GROUP_TYPE aType )
    {
        m_ItemNumber = 0;
        m_Refs.push_back( aFirstReference );
        m_Flag = aType;
    }

    int                        m_ItemNumber;
    GROUP_TYPE                 m_Flag;
    std::vector<SCH_REFERENCE> m_Refs;
};


class SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL : public FIELDS_TABLE_DATA_MODEL_BASE
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
            m_edited( false ),
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
    void RemoveColumn( int aCol );
    void RenameColumn( int aCol, const wxString& newName );

    int GetNumberRows() override { return (int) m_rows.size(); }

    wxString        GetValue( int aRow, int aCol ) override;
    wxString        GetResolvedValue( int aRow, int aCol );
    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override;

    wxString GetValue( const SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& group, int aCol,
                       const wxString& refDelimiter = wxT( ", " ),
                       const wxString& refRangDelimiter = wxT( "-" ),
                       bool resolveVars = false,
                       bool listMixedValues = false );

    wxString GetExportValue( int aRow, int aCol, const wxString& refDelimiter,
                             const wxString& refRangeDelimiter ) override
    {
        return GetValue( m_rows[aRow], aCol, refDelimiter, refRangeDelimiter, true, true );
    }

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    GROUP_TYPE GetRowFlags( int aRow ) { return m_rows[aRow].m_Flag; }

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[aRow].m_Refs;
    }

    bool ColIsValue( int aCol );
    bool ColIsAttribute( int aCol );

    GROUP_TYPE GetGroupType( int aRow ) const override { return m_rows[aRow].m_Flag; }

    void RebuildRows() override;

    void ExpandRow( int aRow );
    void CollapseRow( int aRow );
    void ExpandCollapseRow( int aRow );
    void CollapseForSort();
    void ExpandAfterSort();

    void ApplyData( SCH_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames, const wxString& aVariantName );

    bool IsEdited() { return m_edited; }

    int GetDataWidth( int aCol );

    void  SetScope( SCOPE aScope ) { m_scope = aScope; }
    SCOPE GetScope() { return m_scope; }

    void                  SetPath( const SCH_SHEET_PATH& aPath ) { m_path = aPath; }
    const SCH_SHEET_PATH& GetPath() { return m_path; }

    void AddReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveSymbol( const SCH_SYMBOL& aSymbol );
    void UpdateReferences( const SCH_REFERENCE_LIST& aRefs );

    // Identity-based undo serialization (keyed by symbol, not row position) for the dialog's
    // Ctrl+Z, so it stays correct as rows are grouped/sorted/reordered.
    bool     HasUndoStateSerialization() const override { return true; }
    wxString SerializeUndoState() const override;
    void     RestoreUndoState( const wxString& aState ) override;

    bool DeleteRows( size_t aPosition = 0, size_t aNumRows = 1 ) override;

    const SCH_REFERENCE_LIST& GetReferenceList() const { return m_symbolsList; }

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

private:
    static bool cmp( const SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& lhGroup,
                     const SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& rhGroup, SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL* dataModel,
                     int sortCol, bool ascending );

    bool unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );
    bool groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );

    // Helper functions to deal with translating wxGrid values to and from
    // named field values like ${DNP}
    bool isAttribute( const wxString& aFieldName );

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
    bool attributeInheritedFromSheet( const SCH_REFERENCE& aRef, const wxString& aAttributeName ) const;
    bool rowAttributeInheritedFromSheet( const SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW& aGroup, int aCol );

    wxString getAttributeValue( const SCH_REFERENCE& aRef, const wxString& aAttributeName,
                                const wxString& aVariantNames );

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

    /* Helper function to get the resolved field value.
     * Handles symbols that are missing fields that would have a variable
     * in their value because their name is the same as a variable.
     * Example: BOM template provides ${DNP} as a field, but they symbol doesn't have the field. */
    wxString getFieldShownText( const SCH_REFERENCE& aRef, const wxString& aFieldName );

    void Sort();

    void updateDataStoreSymbolField( const SCH_REFERENCE& aSymbolRef, const wxString& aFieldName );

protected:
    /**
     * The flattened by hierarchy list of symbols.
     *
     * @warning This list **must** be kept sorted by symbol pointer.  Otherwise, the undo/redo
     *          commit actions will be broken.
     */
    SCH_REFERENCE_LIST    m_symbolsList;
    bool                  m_edited;
    SCOPE                 m_scope;
    SCH_SHEET_PATH        m_path;
    wxGridCellAttr*       m_urlEditor;
    wxGridCellRenderer*   m_textVarRenderer; ///< Renderer for cells with text variable references
    wxString              m_currentVariant;  ///< Current variant name for highlighting
    std::vector<wxString> m_variantNames;    ///< Variant names for multi-variant DNP filtering

    std::vector<SYMBOL_FIELDS_TABLE_DATA_MODEL_ROW> m_rows;

    // Data store
    // The data model is fundamentally m_componentRefs X m_fieldNames.
    // A map of compID : fieldSet, where fieldSet is a map of fieldName : fieldValue
    // The compID is now the full KIID_PATH (sheet path + symbol UUID) as a string
    std::map<KIID_PATH, std::map<wxString, wxString>> m_dataStore;
};
