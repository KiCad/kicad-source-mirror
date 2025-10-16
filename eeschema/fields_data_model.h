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
#include <sch_reference_list.h>
#include <wx/grid.h>
#include <widgets/wx_grid.h>

// The field name in the data model (translated)
#define DISPLAY_NAME_COLUMN   0

// The field name's label for exporting (CSV, etc.)
#define LABEL_COLUMN          1
#define SHOW_FIELD_COLUMN     2
#define GROUP_BY_COLUMN       3

// The internal field name (untranslated)
#define FIELD_NAME_COLUMN     4

struct BOM_FIELD;
struct BOM_PRESET;
struct BOM_FMT_PRESET;

enum GROUP_TYPE
{
    GROUP_SINGLETON,
    GROUP_COLLAPSED,
    GROUP_COLLAPSED_DURING_SORT,
    GROUP_EXPANDED,
    CHILD_ITEM
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
        SCOPE_SHEET = 1,
        SCOPE_SHEET_RECURSIVE = 2
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
        if( m_urlEditor )
            m_urlEditor->DecRef();

        for( const auto& [col, attr] : m_colAttrs )
            wxSafeDecRef( attr );
    }

    static const wxString QUANTITY_VARIABLE;
    static const wxString ITEM_NUMBER_VARIABLE;

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser );
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

    int GetNumberRows() override { return static_cast<int>( m_rows.size() ); }
    int GetNumberCols() override { return static_cast<int>( m_cols.size() ); }

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

    void ApplyData( SCH_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames );

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

    void     ApplyBomPreset( const BOM_PRESET& preset );
    BOM_PRESET GetBomSettings();
    wxString Export( const BOM_FMT_PRESET& settings );

    void AddReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveSymbol( const SCH_SYMBOL& aSymbol );
    void UpdateReferences( const SCH_REFERENCE_LIST& aRefs );

    void SetColAttr( wxGridCellAttr* aAttr, int aCol ) override
    {
        wxSafeDecRef( m_colAttrs[aCol] );
        m_colAttrs[aCol] = aAttr;
    }

private:
    static bool cmp( const DATA_MODEL_ROW& lhGroup, const DATA_MODEL_ROW& rhGroup,
                     FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol, bool ascending );
    bool        unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );
    bool        groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );

    // Helper functions to deal with translating wxGrid values to and from
    // named field values like ${DNP}
    bool     isAttribute( const wxString& aFieldName );
    wxString getAttributeValue( const SCH_SYMBOL&, const wxString& aAttributeName );
    void     setAttributeValue( SCH_SYMBOL& aSymbol, const wxString& aAttributeName,
                                const wxString& aValue );

    /* Helper function to get the resolved field value.
     * Handles symbols that are missing fields that would have a variable
     * in their value because their name is the same as a variable.
     * Example: BOM template provides ${DNP} as a field, but they symbol doesn't have the field. */
    wxString getFieldShownText( const SCH_REFERENCE& aRef, const wxString& aFieldName );

    /**
     * Create a composite key for the data store using sheet path and symbol UUID.
     * This is necessary for hierarchical designs where the same symbol (same UUID)
     * is instantiated in multiple sheets with different field values.
     *
     * The key is a string representation of the full KIID_PATH including the symbol's UUID.
     */
    KIID_PATH makeDataStoreKey( const SCH_SHEET_PATH& aSheetPath, const SCH_SYMBOL& aSymbol ) const
    {
        KIID_PATH path = aSheetPath.Path();
        path.push_back( aSymbol.m_Uuid );
        return path;
    }

    KIID_PATH makeDataStoreKey( const SCH_REFERENCE& aRef ) const
    {
        return makeDataStoreKey( aRef.GetSheetPath(), *aRef.GetSymbol() );
    }

    void Sort();

    SCH_REFERENCE_LIST getSymbolReferences( SCH_SYMBOL* aSymbol );
    void               storeReferenceFields( SCH_REFERENCE& aRef );
    void updateDataStoreSymbolField( const SCH_SYMBOL& aSymbol, const wxString& aFieldName );
    void updateDataStoreSymbolField( const SCH_SYMBOL& aSymbol, const wxString& aFieldName,
                                     const SCH_SHEET_PATH* aSheetPath );

protected:
    SCH_REFERENCE_LIST m_symbolsList;
    bool               m_edited;
    int                m_sortColumn;
    bool               m_sortAscending;
    wxString           m_filter;
    enum SCOPE         m_scope;
    SCH_SHEET_PATH     m_path;
    bool               m_groupingEnabled;
    bool               m_excludeDNP;
    bool               m_includeExcluded;
    bool               m_rebuildsEnabled;

    wxGridCellAttr*                m_urlEditor;
    std::map<int, wxGridCellAttr*> m_colAttrs;

    std::vector<DATA_MODEL_COL> m_cols;
    std::vector<DATA_MODEL_ROW> m_rows;

    // Data store
    // The data model is fundamentally m_componentRefs X m_fieldNames.
    // For hierarchical designs, symbols can be instantiated in multiple sheets with the same UUID,
    // so we use a composite key containing the full KIID_PATH (sheet path + symbol UUID).
    // This ensures each hierarchical instance maintains its own distinct field values.
    std::map<KIID_PATH, std::map<wxString, wxString>> m_dataStore;
};
