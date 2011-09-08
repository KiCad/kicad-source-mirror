#include "fctsys.h"
#include "html_messagebox.h"
#include "macros.h"

HTML_MESSAGE_BOX::HTML_MESSAGE_BOX( wxWindow* parent, const wxString & aTitle,
                                      wxPoint aPos, wxSize aSize)
    : DIALOG_DISPLAY_HTML_TEXT_BASE( parent, wxID_ANY, aTitle, aPos, aSize )
{
    SetFocus();
    ListClear();
}

void HTML_MESSAGE_BOX::OnCloseButtonClick( wxCommandEvent& event )
{
    EndModal(0);
}


void HTML_MESSAGE_BOX::ListClear(void)
{
    m_htmlWindow->SetPage(wxEmptyString);
}

/**
 * Function ListSet
 * Add a list of items.
 * @param aList = a string containing items. Items are separated by '\n'
 */
void HTML_MESSAGE_BOX::ListSet(const wxString &aList)
{
    wxArrayString* wxStringSplit( wxString txt, wxChar splitter );

    wxArrayString* strings_list = wxStringSplit( aList, wxChar('\n') );
    wxString msg = wxT("<ul>");
    for ( unsigned ii = 0; ii < strings_list->GetCount(); ii ++ )
    {
        msg += wxT("<li>");
        msg += strings_list->Item(ii) + wxT("</li>");
    }
    msg += wxT("</ul>");
    m_htmlWindow->AppendToPage( msg );

    delete strings_list;
}

/**
 * Function ListSet
 * Add a list of items.
 * @param aList = a wxArrayString containing items
 */
void HTML_MESSAGE_BOX::ListSet(const wxArrayString &aList)
{
    wxString msg = wxT("<ul>");
    for ( unsigned ii = 0; ii < aList.GetCount(); ii ++ )
    {
        msg += wxT("<li>");
        msg += aList.Item(ii) + wxT("</li>");
    }
    msg += wxT("</ul>");
    m_htmlWindow->AppendToPage( msg );
}

/**
 * Function MessageSet
 * Add a message (in bold) to message list.
 * @param message = the message
 */
void HTML_MESSAGE_BOX::MessageSet(const wxString &message)
{
    wxString message_value;
    message_value.Printf(wxT("<b>%s</b><br>"), GetChars( message ) );
    m_htmlWindow->AppendToPage( message_value );
}

/**
 * Function AddHTML_Text
 * Add a text to message list.
 * @param message = the text to add
 */
void HTML_MESSAGE_BOX::AddHTML_Text(const wxString &message)
{
    m_htmlWindow->AppendToPage( message );
}

