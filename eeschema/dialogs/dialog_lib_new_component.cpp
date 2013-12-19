#include <dialog_lib_new_component.h>

DIALOG_LIB_NEW_COMPONENT::DIALOG_LIB_NEW_COMPONENT( wxWindow* parent ) :
    DIALOG_LIB_NEW_COMPONENT_BASE( parent )
{
    // initial focus should be on first editable field.
    m_textName->SetFocus();

    // What happens when user presses "Enter"? OK button!  OK?
    m_sdbSizerOK->SetDefault();
}
