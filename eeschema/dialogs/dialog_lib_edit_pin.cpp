#include "fctsys.h"

#include "dialog_lib_edit_pin.h"

// dialog should remember its previous screen position and size
// Not also if the defaut size is > s_LastSize, default size is used
wxPoint DIALOG_LIB_EDIT_PIN::s_LastPos( -1, -1 );
wxSize  DIALOG_LIB_EDIT_PIN::s_LastSize;

DIALOG_LIB_EDIT_PIN::DIALOG_LIB_EDIT_PIN( wxWindow* parent ) :
    DIALOG_LIB_EDIT_PIN_BASE( parent )
{
    /* Required to make escape key work correctly in wxGTK. */
    SetFocus();
    // Set tab order
    m_textPinName-> MoveAfterInTabOrder(this);
    m_textPadName-> MoveAfterInTabOrder(m_textPinName);
    m_sdbSizerButtonsOK->SetDefault();
}

void DIALOG_LIB_EDIT_PIN::SetLastSizeAndPosition()
{
    if( s_LastPos.x != -1 )
    {
        wxSize defaultSize = GetSize();
        if(  s_LastSize.x < defaultSize.x )
            s_LastSize.x = defaultSize.x;
        SetSize( s_LastSize );
        SetPosition( s_LastPos );
    }
    else
        Center();
}

void DIALOG_LIB_EDIT_PIN::OnCloseDialog( wxCloseEvent& event )
{
    // Save the dialog's position
    s_LastPos  = GetPosition();
    s_LastSize = GetSize();
    EndModal( wxID_CANCEL );
}

void DIALOG_LIB_EDIT_PIN::OnCancelButtonClick( wxCommandEvent& event )
{
    // Save the dialog's position
    s_LastPos  = GetPosition();
    s_LastSize = GetSize();
    EndModal( wxID_CANCEL );
}

void DIALOG_LIB_EDIT_PIN::OnOKButtonClick( wxCommandEvent& event )
{
    // Save the dialog's position
    s_LastPos  = GetPosition();
    s_LastSize = GetSize();
    EndModal( wxID_OK );
}



void DIALOG_LIB_EDIT_PIN::SetOrientationList( const wxArrayString& list,
                                              const char*** aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceOrientation->Append( list[ii] );
        else
            m_choiceOrientation->Insert( list[ii], wxBitmap( aBitmaps[ii] ), ii );
    }
}


void DIALOG_LIB_EDIT_PIN::SetElectricalTypeList( const wxArrayString& list,
                                                 const char*** aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceElectricalType->Append( list[ii] );
        else
            m_choiceElectricalType->Insert( list[ii], wxBitmap( aBitmaps[ii] ), ii );
    }
}


void DIALOG_LIB_EDIT_PIN::SetStyleList( const wxArrayString& list, const char*** aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceStyle->Append( list[ii] );
        else
            m_choiceStyle->Insert( list[ii], wxBitmap( aBitmaps[ii] ), ii );
    }
}
