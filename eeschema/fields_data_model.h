#include <sch_reference_list.h>
#include <wx/grid.h>

// The field name in the data model (translated)
#define DISPLAY_NAME_COLUMN   0
// The field name's label for exporting (CSV, etc.)
#define LABEL_COLUMN          1
#define SHOW_FIELD_COLUMN     2
#define GROUP_BY_COLUMN       3
// The internal field name (untranslated)
#define FIELD_NAME_COLUMN     4

struct BOM_EXPORT_SETTINGS
{
    wxString FieldDelimiter;
    wxString StringDelimiter;
    bool     SpacedRefs;
    bool     RemoveTabs;
    bool     RemoveLineBreaks;
};

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
        m_Refs.push_back( aFirstReference );
        m_Flag = aType;
    }

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


class FIELDS_EDITOR_GRID_DATA_MODEL : public wxGridTableBase
{
public:
    FIELDS_EDITOR_GRID_DATA_MODEL( SCH_EDIT_FRAME* aFrame, SCH_REFERENCE_LIST& aSymbolsList ) :
            m_frame( aFrame ), m_symbolsList( aSymbolsList ), m_edited( false ), m_sortColumn( 0 ),
            m_sortAscending( false ), m_groupingEnabled( false )
    {
        m_symbolsList.SplitReferences();
    }

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser );
    void RemoveColumn( int aCol );
    void RenameColumn( int aCol, const wxString& newName );
    void MoveColumn( int aCol, int aNewPos )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        std::swap( m_cols[aCol], m_cols[aNewPos] );
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

    int      GetFieldNameCol( wxString aFieldName );

    const std::vector<wxString> GetFieldsOrder();
    void                        SetFieldsOrder( const std::vector<wxString>& aNewOrder );

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override;
    wxString GetValue( const DATA_MODEL_ROW& group, int aCol, bool spacedRefs = true );
    wxString GetRawValue( int aRow, int aCol, bool spacedRefs )
    {
        return GetValue( m_rows[aRow], aCol, spacedRefs );
    }
    void     SetValue( int aRow, int aCol, const wxString& aValue ) override;

    GROUP_TYPE GetRowFlags( int aRow ) { return m_rows[aRow].m_Flag; }

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow >= 0 && aRow < (int) m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[aRow].m_Refs;
    }

    bool ColIsReference( int aCol )
    {
        wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
        return m_cols[aCol].m_fieldName == _( "Reference" );
    }

    bool ColIsQuantity( int aCol )
    {
        wxCHECK( aCol >= 0 && aCol < (int) m_cols.size(), false );
        return m_cols[aCol].m_fieldName == _( "Qty" );
    }

    void SetSorting( int aCol, bool ascending )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_sortColumn = aCol;
        m_sortAscending = ascending;
    }

    void RebuildRows();
    void ExpandRow( int aRow );
    void CollapseRow( int aRow );
    void ExpandCollapseRow( int aRow );
    void CollapseForSort();
    void ExpandAfterSort();

    void ApplyData();

    bool IsEdited() { return m_edited; }

    int GetDataWidth( int aCol );

    void SetFilter( const wxString& aFilter ) { m_filter = aFilter; }
    void SetGroupingEnabled( bool group ) { m_groupingEnabled = group; }
    void SetGroupColumn( int aCol, bool group )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_cols[aCol].m_group = group;
    }

    void SetShowColumn( int aCol, bool show )
    {
        wxCHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_cols[aCol].m_show = show;
    }

    wxString Export( const BOM_EXPORT_SETTINGS& settings );

private:
    static bool cmp( const DATA_MODEL_ROW& lhGroup, const DATA_MODEL_ROW& rhGroup,
                     FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol, bool ascending );
    bool        unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );
    bool        groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );

    void Sort();


protected:
    SCH_EDIT_FRAME*    m_frame;
    SCH_REFERENCE_LIST m_symbolsList;
    bool               m_edited;
    int                m_sortColumn;
    bool               m_sortAscending;
    wxString           m_filter;
    bool               m_groupingEnabled;

    std::vector<DATA_MODEL_COL> m_cols;
    std::vector<DATA_MODEL_ROW> m_rows;

    // Data store
    // The data model is fundamentally m_componentRefs X m_fieldNames.
    // A map of compID : fieldSet, where fieldSet is a map of fieldName : fieldValue
    std::map<KIID, std::map<wxString, wxString>> m_dataStore;
};
