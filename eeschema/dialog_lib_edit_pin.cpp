#include "fctsys.h"

#include "dialog_lib_edit_pin.h"


DIALOG_LIB_EDIT_PIN::DIALOG_LIB_EDIT_PIN( wxWindow* parent ) :
    DIALOG_LIB_EDIT_PIN_BASE( parent )
{
    /* Required to make escape key work correctly in wxGTK. */
    m_textName->SetFocus();
}


void DIALOG_LIB_EDIT_PIN::SetOrientationList( const wxArrayString& list,
                                              const char *** aBitmaps  )
{
    for ( unsigned ii = 0; ii < list.GetCount( ); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceOrientation->Append(list[ii]);
        else
            m_choiceOrientation->Insert(list[ii],
                wxBitmap(aBitmaps[ii]), ii);
        }
}


void DIALOG_LIB_EDIT_PIN::SetElectricalTypeList( const wxArrayString& list,
                                                 const char *** aBitmaps  )
{
    for ( unsigned ii = 0; ii < list.GetCount( ); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceElectricalType->Append(list[ii]);
        else
            m_choiceElectricalType->Insert(list[ii],
                wxBitmap(aBitmaps[ii]), ii);
    }
}


void DIALOG_LIB_EDIT_PIN::SetStyleList( const wxArrayString& list,
                                        const char *** aBitmaps   )
{
    for ( unsigned ii = 0; ii < list.GetCount( ); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceStyle->Append(list[ii]);
        else
            m_choiceStyle->Insert(list[ii],
                wxBitmap(aBitmaps[ii]), ii);
    }
}
