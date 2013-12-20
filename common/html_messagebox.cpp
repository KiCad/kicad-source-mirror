#include <fctsys.h>
#include <html_messagebox.h>
#include <macros.h>
#include <common.h>


HTML_MESSAGE_BOX::HTML_MESSAGE_BOX( wxWindow* parent, const wxString& aTitle,
        wxPoint aPos, wxSize aSize) :
    DIALOG_DISPLAY_HTML_TEXT_BASE( parent, wxID_ANY, aTitle, aPos, aSize )
{
    ListClear();
    Center();
}


void HTML_MESSAGE_BOX::OnCloseButtonClick( wxCommandEvent& event )
{
    EndModal( 0 );
}


void HTML_MESSAGE_BOX::ListClear()
{
    m_htmlWindow->SetPage( wxEmptyString );
}


void HTML_MESSAGE_BOX::ListSet( const wxString& aList )
{
    // wxArrayString* wxStringSplit( wxString txt, wxChar splitter );

    wxArrayString* strings_list = wxStringSplit( aList, wxChar( '\n' ) );

    wxString msg = wxT( "<ul>" );

    for ( unsigned ii = 0; ii < strings_list->GetCount(); ii++ )
    {
        msg += wxT( "<li>" );
        msg += strings_list->Item( ii ) + wxT( "</li>" );
    }

    msg += wxT( "</ul>" );

    m_htmlWindow->AppendToPage( msg );

    delete strings_list;
}


void HTML_MESSAGE_BOX::ListSet( const wxArrayString& aList )
{
    wxString msg = wxT( "<ul>" );

    for( unsigned ii = 0; ii < aList.GetCount(); ii++ )
    {
        msg += wxT( "<li>" );
        msg += aList.Item( ii ) + wxT( "</li>" );
    }

    msg += wxT( "</ul>" );

    m_htmlWindow->AppendToPage( msg );
}


void HTML_MESSAGE_BOX::MessageSet( const wxString& message )
{
    wxString message_value = wxString::Format(
                wxT( "<b>%s</b><br>" ), GetChars( message ) );

    m_htmlWindow->AppendToPage( message_value );
}


void HTML_MESSAGE_BOX::AddHTML_Text( const wxString& message )
{
    m_htmlWindow->AppendToPage( message );
}

