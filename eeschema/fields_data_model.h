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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <sch_reference_list.h>
#include <wx/grid.h>
#include <widgets/wx_grid.h>


struct BOM_FIELD;
struct BOM_PRESET;
struct BOM_FMT_PRESET;


// Columns for the View Fields grid
#define DISPLAY_NAME_COLUMN   0     // The field name in the data model (translated)
#define LABEL_COLUMN          1     // The field name's label for exporting (CSV, etc.)
#define SHOW_FIELD_COLUMN     2
#define GROUP_BY_COLUMN       3
#define VIEW_FIELDS_COL_COUNT 4


// Data model for the list of fields to view (and to group-by) for the Symbol Fields Table
class VIEW_CONTROLS_GRID_DATA_MODEL : public WX_GRID_TABLE_BASE
{
public:
    VIEW_CONTROLS_GRID_DATA_MODEL( bool aForBOM ) :
            m_forBOM( aForBOM )
    {}

    ~VIEW_CONTROLS_GRID_DATA_MODEL() override = default;

    int GetNumberRows() override { return (int) m_fields.size(); }
    int GetNumberCols() override { return VIEW_FIELDS_COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override;

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case DISPLAY_NAME_COLUMN:
        case LABEL_COLUMN:        return aTypeName == wxGRID_VALUE_STRING;

        case SHOW_FIELD_COLUMN:
        case GROUP_BY_COLUMN:     return aTypeName == wxGRID_VALUE_BOOL;

        default:                  wxFAIL; return false;
        }
    }

    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        return CanGetValueAs( aRow, aCol, aTypeName );
    }

    wxString GetValue( int aRow, int aCol ) override;
    bool GetValueAsBool( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;
    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;

    void AppendRow( const wxString& aFieldName, const wxString& aBOMName, bool aShow, bool aGroupBy );
    void DeleteRow( int aRow );

    wxString GetCanonicalFieldName( int aRow );
    void SetCanonicalFieldName( int aRow, const wxString& aName );

protected:
    bool                   m_forBOM;
    std::vector<BOM_FIELD> m_fields;
};


struct DATA_MODEL_ROW
{
    DATA_MODEL_ROW( const SCH_REFERENCE& aFirstReference, GROUP_TYPE aType )
    {
        m_ItemNumber = 0;
        m_Refs.push_back( aFirstReference );
        m_Flag = aType;
    }

    int                        m_ItemNumber;
    GROUP_TYPE                 m_Flag;
    std::vector<SCH_REFERENCE> m_Refs;
};


struct DATA_MODEL_COL
{
    wxString m_fieldName;
    wxString m_label;
    bool     m_userAdded;
    bool     m_show;
    bool     m_group;
};


class FIELDS_EDITOR_GRID_DATA_MODEL : public WX_GRID_TABLE_BASE
{
public:
    enum SCOPE : int
    {
        SCOPE_ALL = 0,
        SCOPE_SHEET,
        SCOPE_SHEET_RECURSIVE
    };

    FIELDS_EDITOR_GRID_DATA_MODEL( const SCH_REFERENCE_LIST& aSymbolsList, wxGridCellAttr* aURLEditor ) :
            m_symbolsList( aSymbolsList ),
            m_edited( false ),
            m_sortColumn( 0 ),
            m_sortAscending( false ),
            m_scope( SCOPE_ALL ),
            m_groupingEnabled( false ),
            m_excludeDNP( false ),
            m_includeExcluded( false ),
            m_rebuildsEnabled( true ),
            m_urlEditor( aURLEditor )
    {
        m_symbolsList.SplitReferences();
    }

    ~FIELDS_EDITOR_GRID_DATA_MODEL() override
    {
        wxSafeDecRef( m_urlEditor );
    }

    static const wxString QUANTITY_VARIABLE;
    static const wxString ITEM_NUMBER_VARIABLE;

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser,
                    const wxString& aVariantName );
    void RemoveColumn( int aCol );
    void RenameColumn( int aCol, const wxString& newName );

    void MoveColumn( int aCol, int aNewPos )
    {
        wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );

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
        wxCHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );
        m_cols[aCol].m_label = aLabel;
    }

    wxString GetColLabelValue( int aCol ) override
    {
        wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxString() );
        return m_cols[aCol].m_label;
    }

    wxString GetColFieldName( int aCol )
    {
        wxCHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxString() );
        return m_cols[aCol].m_fieldName;
    }

    int GetFieldNameCol( const wxString& aFieldName ) const;

    std::vector<BOM_FIELD> GetFieldsOrdered();
    void                   SetFieldsOrder( const std::vector<wxString>& aNewOrder );

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override;
    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override;

    wxString GetValue( const DATA_MODEL_ROW& group, int aCol,
                       const wxString& refDelimiter = wxT( ", " ),
                       const wxString& refRangDelimiter = wxT( "-" ),
                       bool resolveVars = false,
                       bool listMixedValues = false );

    wxString GetExportValue( int aRow, int aCol, const wxString& refDelimiter,
                             const wxString& refRangeDelimiter )
    {
        return GetValue( m_rows[aRow], aCol, refDelimiter, refRangeDelimiter, true, true );
    }

    void     SetValue( int aRow, int aCol, const wxString& aValue ) override;

    GROUP_TYPE GetRowFlags( int aRow ) { return m_rows[aRow].m_Flag; }

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[aRow].m_Refs;
    }

    bool ColIsReference( int aCol );
    bool ColIsValue( int aCol );
    bool ColIsQuantity( int aCol );
    bool ColIsItemNumber( int aCol );
    bool ColIsAttribute( int aCol );

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

    // These are used to disable the RebuildRows functionality while we're generating
    // lots of events in the UI, e.g. applying a BOM preset, that would thrash the grid.
    void EnableRebuilds();
    void DisableRebuilds();
    void RebuildRows();

    void ExpandRow( int aRow );
    void CollapseRow( int aRow );
    void ExpandCollapseRow( int aRow );
    void CollapseForSort();
    void ExpandAfterSort();

    void ApplyData( SCH_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames, const wxString& aVariantName );

    bool IsEdited() { return m_edited; }

    int GetDataWidth( int aCol );

    void SetFilter( const wxString& aFilter ) { m_filter = aFilter; }
    const wxString& GetFilter() { return m_filter; }

    void  SetScope( SCOPE aScope ) { m_scope = aScope; }
    SCOPE GetScope() { return m_scope; }

    void                  SetPath( const SCH_SHEET_PATH& aPath ) { m_path = aPath; }
    const SCH_SHEET_PATH& GetPath() { return m_path; }

    void SetGroupingEnabled( bool group ) { m_groupingEnabled = group; }
    bool GetGroupingEnabled() { return m_groupingEnabled; }

    /* These contradictorily named functions force including symbols that
     * have the Exclude from BOM check box ticked. This is needed so we can view
     * these parts in the symbol fields table dialog, while also excluding from the
     * BOM export */
    void SetIncludeExcludedFromBOM( bool include ) { m_includeExcluded = include; }
    bool GetIncludeExcludedFromBOM() { return m_includeExcluded; }

    void SetExcludeDNP( bool exclude ) { m_excludeDNP = exclude; }
    bool GetExcludeDNP() { return m_excludeDNP; }

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

    void     ApplyBomPreset( const BOM_PRESET& preset, const wxString& aVariantName );
    BOM_PRESET GetBomSettings();
    wxString Export( const BOM_FMT_PRESET& settings );

    void AddReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveSymbol( const SCH_SYMBOL& aSymbol );
    void UpdateReferences( const SCH_REFERENCE_LIST& aRefs, const wxString& aVariantName );

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
    void SetCurrentVariant( const wxString& aVariantName ) { m_currentVariant = aVariantName; }
    const wxString& GetCurrentVariant() const { return m_currentVariant; }

    void SetVariantNames( const std::vector<wxString>& aVariantNames ) { m_variantNames = aVariantNames; }
    const std::vector<wxString>& GetVariantNames() const { return m_variantNames; }

private:
    static bool cmp( const DATA_MODEL_ROW& lhGroup, const DATA_MODEL_ROW& rhGroup,
                     FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol, bool ascending );

    bool unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );
    bool groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );

    // Helper functions to deal with translating wxGrid values to and from
    // named field values like ${DNP}
    bool     isAttribute( const wxString& aFieldName );
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

    void updateDataStoreSymbolField( const SCH_REFERENCE& aSymbolRef, const wxString& aFieldName,
                                     const wxString& aVariantName );

protected:
    /**
     * The flattened by hierarchy list of symbols.
     *
     * @warning This list **must** be kept sorted by symbol pointer.  Otherwise, the undo/redo
     *          commit actions will be broken.
     */
    SCH_REFERENCE_LIST m_symbolsList;
    bool               m_edited;
    int                m_sortColumn;
    bool               m_sortAscending;
    wxString           m_filter;
    SCOPE              m_scope;
    SCH_SHEET_PATH     m_path;
    bool               m_groupingEnabled;
    bool               m_excludeDNP;
    bool               m_includeExcluded;
    bool               m_rebuildsEnabled;
    wxGridCellAttr*    m_urlEditor;
    wxString                m_currentVariant;  ///< Current variant name for highlighting
    std::vector<wxString>   m_variantNames;    ///< Variant names for multi-variant DNP filtering

    std::vector<DATA_MODEL_COL> m_cols;
    std::vector<DATA_MODEL_ROW> m_rows;

    // Data store
    // The data model is fundamentally m_componentRefs X m_fieldNames.
    // A map of compID : fieldSet, where fieldSet is a map of fieldName : fieldValue
    // The compID is now the full KIID_PATH (sheet path + symbol UUID) as a string
    std::map<KIID_PATH, std::map<wxString, wxString>> m_dataStore;
};
