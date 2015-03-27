#include "dialog_lib_edit_pin_table_base.h"

#include "class_library.h"

class DIALOG_LIB_EDIT_PIN_TABLE :
    public DIALOG_LIB_EDIT_PIN_TABLE_BASE
{
public:
    DIALOG_LIB_EDIT_PIN_TABLE( wxWindow* parent, LIB_PART& aPart );
    ~DIALOG_LIB_EDIT_PIN_TABLE();

    virtual void OnColumnHeaderRightClicked( wxDataViewEvent& aEvent );

private:
    class DataViewModel;

    wxObjectDataPtr<DataViewModel> m_Model;
};
