#include <sch_reference_list.h>
#include <wx/grid.h>
#include <wx/srchctrl.h>

// The field name in the data model (translated)
#define DISPLAY_NAME_COLUMN   0
// The field name's label for exporting (CSV, etc.)
#define LABEL_COLUMN          1
#define SHOW_FIELD_COLUMN     2
#define GROUP_BY_COLUMN       3
// The internal field name (untranslated)
#define FIELD_NAME_COLUMN     4


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
            m_frame( aFrame ),
            m_symbolsList( aSymbolsList ),
            m_edited( false ),
            m_sortColumn( 0 ),
            m_sortAscending( false )
    {
        m_symbolsList.SplitReferences();
    }

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser );
    void RemoveColumn( int aCol );
    void RenameColumn( int aCol, const wxString& newName );
    void MoveColumn( int aCol, int aNewPos ) { std::swap( m_cols[aCol], m_cols[aNewPos] ); }

    int GetNumberRows() override { return (int) m_rows.size(); }
    int GetNumberCols() override { return (int) m_cols.size(); }

    void SetColLabelValue( int aCol, const wxString& aLabel ) override
    {
        m_cols[aCol].m_label = aLabel;
    }


    wxString GetColLabelValue( int aCol ) override { return m_cols[aCol].m_label; }

    wxString GetColFieldName( int aCol ) { return m_cols[aCol].m_fieldName; }
    int      GetFieldNameCol( wxString aFieldName );

    const std::vector<wxString> GetFieldsOrder();
    void                        SetFieldsOrder( const std::vector<wxString>& aNewOrder );

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override;
    wxString GetValue( const DATA_MODEL_ROW& group, int aCol );
    wxString GetRawValue( int aRow, int aCol ) { return GetValue( m_rows[aRow], aCol ); }
    void     SetValue( int aRow, int aCol, const wxString& aValue ) override;

    GROUP_TYPE GetRowFlags( int aRow ) { return m_rows[aRow].m_Flag; }

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow < (int) m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[aRow].m_Refs;
    }

    bool ColIsReference( int aCol )
    {
        return ( aCol < (int) m_cols.size() ) && m_cols[aCol].m_fieldName == _( "Reference" );
    }

    bool ColIsQuantity( int aCol )
    {
        return ( aCol < (int) m_cols.size() ) && m_cols[aCol].m_fieldName == _( "Qty" );
    }

    void Sort( int aColumn, bool ascending );

    void RebuildRows( wxSearchCtrl* aFilter, wxCheckBox* aGroupSymbolsBox,
                      wxDataViewListCtrl* aFieldsCtrl );
    void ExpandRow( int aRow );
    void CollapseRow( int aRow );
    void ExpandCollapseRow( int aRow );
    void CollapseForSort();
    void ExpandAfterSort();

    void ApplyData();

    bool IsEdited() { return m_edited; }

    int GetDataWidth( int aCol );

private:
    static bool cmp( const DATA_MODEL_ROW& lhGroup, const DATA_MODEL_ROW& rhGroup,
                     FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol, bool ascending );
    bool        unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );
    bool        groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef,
                            wxDataViewListCtrl* fieldsCtrl );

protected:
    SCH_EDIT_FRAME*    m_frame;
    SCH_REFERENCE_LIST m_symbolsList;
    bool               m_edited;
    int                m_sortColumn;
    bool               m_sortAscending;

    std::vector<DATA_MODEL_COL> m_cols;
    std::vector<DATA_MODEL_ROW> m_rows;

    // Data store
    // The data model is fundamentally m_componentRefs X m_fieldNames.
    // A map of compID : fieldSet, where fieldSet is a map of fieldName : fieldValue
    std::map<KIID, std::map<wxString, wxString>> m_dataStore;
};
