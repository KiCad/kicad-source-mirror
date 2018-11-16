#include "dialog_lib_edit_pin_table_base.h"

#include "class_library.h"

enum COL_ORDER
{
    COL_NUMBER,
    COL_NAME,
    COL_TYPE,
    COL_SHAPE,
    COL_ORIENTATION,
    COL_NUMBER_SIZE,
    COL_NAME_SIZE,
    COL_LENGTH,
    COL_POSX,
    COL_POSY,

    COL_COUNT       // keep as last
};


class PIN_TABLE_DATA_MODEL;


class DIALOG_LIB_EDIT_PIN_TABLE : public DIALOG_LIB_EDIT_PIN_TABLE_BASE
{
public:
    DIALOG_LIB_EDIT_PIN_TABLE( wxWindow* parent, LIB_PART* aPart );
    ~DIALOG_LIB_EDIT_PIN_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnColSort( wxGridEvent& aEvent );
    void OnAddRow( wxCommandEvent& event ) override;
    void OnDeleteRow( wxCommandEvent& event ) override;
    void OnSize( wxSizeEvent& event ) override;
    void OnCellEdited( wxGridEvent& event ) override;
    void OnRebuildRows( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnCancel( wxCommandEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;

protected:

    void updateSummary();
    void adjustGridColumns( int aWidth );

    wxConfigBase*         m_config;
    bool                  m_initialized = false;
    int                   m_originalColWidths[ COL_COUNT ];
    wxString              m_columnsShown;
    LIB_PART*             m_part;
    LIB_PINS              m_pins;       // a copy of the pins owned by me
    bool                  m_modified;   ///< true when there are unsaved changes

    PIN_TABLE_DATA_MODEL* m_dataModel;
};
