#include "fctsys.h"
#include "dialog_load_error.h"
#include "macros.h"

DIALOG_LOAD_ERROR::DIALOG_LOAD_ERROR( wxWindow* parent )
:
DIALOG_DISPLAY_HTML_TEXT_BASE( parent, wxID_ANY, _("Load Error!"),wxDefaultPosition, wxSize( 450,250 ) )
{
    SetFocus();
    ListClear();
}

void DIALOG_LOAD_ERROR::OnCloseButtonClick( wxCommandEvent& event )
{
    EndModal(0);
}


void DIALOG_LOAD_ERROR::ListClear(void)
{
	m_htmlWindow->SetPage(wxEmptyString);
}

/** Function ListSet
 * Add a list of items.
 * @param list = a string containing items. Items are separated by '\n'
 */
void DIALOG_LOAD_ERROR::ListSet(const wxString &list)
{
    wxArrayString* wxStringSplit( wxString txt, wxChar splitter );

    wxArrayString* strings_list = wxStringSplit( list, wxChar('\n') );
    m_htmlWindow->AppendToPage(wxT("<ul>") );
	for ( unsigned ii = 0; ii < strings_list->GetCount(); ii ++ )
    {
        m_htmlWindow->AppendToPage(wxT("<li>") );
        m_htmlWindow->AppendToPage( strings_list->Item(ii) );
        m_htmlWindow->AppendToPage(wxT("</li>") );
    }
    m_htmlWindow->AppendToPage(wxT("</ul>") );

    delete strings_list;
}

/** Function MessageSet
 * Add a message (in bold) to message list.
 * @param message = the message
 */
void DIALOG_LOAD_ERROR::MessageSet(const wxString &message)
{
	wxString message_value;
    message_value.Printf(wxT("<b>%s</b><br>"), GetChars( message ) );
    m_htmlWindow->AppendToPage( message_value );
}

