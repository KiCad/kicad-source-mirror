#include "dialog_lib_edit_pin.h"

DIALOG_LIB_EDIT_PIN::DIALOG_LIB_EDIT_PIN( wxWindow* parent ) :
    DIALOG_LIB_EDIT_PIN_BASE( parent )
{
    /* Required to make escape key work correctly in wxGTK. */
    m_textName->SetFocus();
}


void DIALOG_LIB_EDIT_PIN::SetOrientationList( const wxArrayString& list )
{
    m_choiceOrientation->Append( list );
}


void DIALOG_LIB_EDIT_PIN::SetElectricalTypeList( const wxArrayString& list )
{
    m_choiceElectricalType->Append( list );
}


void DIALOG_LIB_EDIT_PIN::SetStyleList( const wxArrayString& list )
{
    m_choiceStyle->Append( list );
}
