/**
 * @file displlst.cpp
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <gr_basic.h>
#include <common.h>
#include <macros.h>
#include <kicad_string.h>
#include <dialog_helpers.h>


enum listbox {
    ID_LISTBOX_LIST = 8000
};


BEGIN_EVENT_TABLE( EDA_LIST_DIALOG, wxDialog )
    EVT_BUTTON( wxID_OK, EDA_LIST_DIALOG::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, EDA_LIST_DIALOG::OnCancelClick )
    EVT_LISTBOX( ID_LISTBOX_LIST, EDA_LIST_DIALOG::ClickOnList )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_LIST, EDA_LIST_DIALOG::D_ClickOnList )
    EVT_CHAR( EDA_LIST_DIALOG::OnKeyEvent )
    EVT_CHAR_HOOK( EDA_LIST_DIALOG::OnKeyEvent )
    EVT_CLOSE( EDA_LIST_DIALOG::OnClose )
END_EVENT_TABLE()


EDA_LIST_DIALOG::EDA_LIST_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aTitle,
                                  const wxArrayString& aItemList, const wxString& aRefText,
                                  void(* aCallBackFunction)(wxString& Text), wxPoint aPos ) :
    wxDialog( aParent, wxID_ANY, aTitle, aPos, wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | MAYBE_RESIZE_BORDER )
{
    m_callBackFct = aCallBackFunction;
    m_messages    = NULL;

    wxBoxSizer* GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );

    SetSizer( GeneralBoxSizer );

    m_listBox = new wxListBox( this, ID_LISTBOX_LIST, wxDefaultPosition,
                               wxSize( 300, 200 ), 0, NULL,
                               wxLB_NEEDED_SB | wxLB_SINGLE | wxLB_HSCROLL );

    GeneralBoxSizer->Add( m_listBox, 2, wxGROW | wxALL, 5 );

    InsertItems( aItemList, 0 );

    if( m_callBackFct )
    {
        m_messages = new wxTextCtrl( this, -1, wxEmptyString,
                                     wxDefaultPosition, wxSize( -1, 60 ),
                                     wxTE_READONLY | wxTE_MULTILINE );

        GeneralBoxSizer->Add( m_messages, 1, wxGROW | wxALL, 5 );
    }

    wxSizer* buttonSizer = CreateButtonSizer( wxOK | wxCANCEL );

    if( buttonSizer )
        GeneralBoxSizer->Add( buttonSizer, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


EDA_LIST_DIALOG::~EDA_LIST_DIALOG()
{
}


void EDA_LIST_DIALOG::MoveMouseToOrigin()
{
    int    x, y, w, h;
    wxSize list_size = m_listBox->GetSize();
    int    orgx = m_listBox->GetRect().GetLeft();
    int    orgy = m_listBox->GetRect().GetTop();

    wxClientDisplayRect( &x, &y, &w, &h );

    WarpPointer( x + orgx + 20, y + orgy + (list_size.y / 2) );
}


wxString EDA_LIST_DIALOG::GetTextSelection()
{
    wxString text = m_listBox->GetStringSelection();
    return text;
}


void EDA_LIST_DIALOG::Append( const wxString& item )
{
    m_listBox->Append( item );
}


void EDA_LIST_DIALOG::InsertItems( const wxArrayString& itemlist, int position )
{
    m_listBox->InsertItems( itemlist, position );
}


void EDA_LIST_DIALOG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void EDA_LIST_DIALOG::ClickOnList( wxCommandEvent& event )
{
    wxString text;

    if( m_callBackFct )
    {
        m_messages->Clear();
        text = m_listBox->GetStringSelection();
        m_callBackFct( text );
        m_messages->WriteText( text );
    }
}


void EDA_LIST_DIALOG::D_ClickOnList( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}


void EDA_LIST_DIALOG::OnOkClick( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}


void EDA_LIST_DIALOG::OnClose( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


/* Sort alphabetically, case insensitive.
 */
static int SortItems( const wxString& item1, const wxString& item2 )
{
    return StrNumCmp( item1, item2, INT_MAX, true );
}


void EDA_LIST_DIALOG::SortList()
{
    wxArrayString list = m_listBox->GetStrings();

    if( list.IsEmpty() <= 0 )
        return;

    list.Sort( SortItems );

    m_listBox->Clear();

    m_listBox->Append( list );
}


void EDA_LIST_DIALOG::OnKeyEvent( wxKeyEvent& event )
{
    event.Skip();
}
