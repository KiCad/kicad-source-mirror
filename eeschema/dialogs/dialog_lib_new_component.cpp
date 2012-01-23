#include <dialog_lib_new_component.h>

DIALOG_LIB_NEW_COMPONENT::DIALOG_LIB_NEW_COMPONENT( wxWindow* parent ) :
    DIALOG_LIB_NEW_COMPONENT_BASE( parent )
{
    /* Required to make escape key work correctly in wxGTK. */
    m_sdbSizerOK->SetFocus();
    m_sdbSizerOK->SetDefault();
}
