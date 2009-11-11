#include "dialog_lib_edit_pin.h"

DIALOG_LIB_EDIT_PIN::DIALOG_LIB_EDIT_PIN( wxWindow* parent ) :
    DIALOG_LIB_EDIT_PIN_BASE( parent )
{
    /* Required to make escape key work correctly in wxGTK. */
    m_textName->SetFocus();

    /* This ugly hack fixes a bug in wxWidgets 2.8.7 and likely earlier
     * versions for the flex grid sizer in wxGTK that prevents the last
     * column from being sized correctly.
     */
#ifdef __WXGTK__
    m_staticNameTextSizeUnits->SetMinSize( wxSize( 75, -1 ) );
#endif
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
