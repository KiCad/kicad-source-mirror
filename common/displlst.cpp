/****************/
/* displlst.cpp */
/****************/

#include "fctsys.h"
#include "wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "kicad_string.h"
#include "dialog_helpers.h"


enum listbox {
    ID_LISTBOX_LIST = 8000
};


BEGIN_EVENT_TABLE( WinEDAListBox, wxDialog )
EVT_BUTTON( wxID_OK, WinEDAListBox::OnOkClick )
EVT_BUTTON( wxID_CANCEL, WinEDAListBox::OnCancelClick )
EVT_LISTBOX( ID_LISTBOX_LIST, WinEDAListBox::ClickOnList )
EVT_LISTBOX_DCLICK( ID_LISTBOX_LIST, WinEDAListBox::D_ClickOnList )
EVT_CHAR( WinEDAListBox::OnKeyEvent )
EVT_CHAR_HOOK( WinEDAListBox::OnKeyEvent )
EVT_CLOSE( WinEDAListBox::OnClose )
END_EVENT_TABLE()


/**
 * Used to display a list of elements for selection, and display comment of info lines
 * about the selected item.
 * @param aParent = apointeur to the parent window
 * @param aTitle = the title shown on top.
 * @param aItemList = a wxArrayString: the list of elements.
 * @param aRefText = an item name if an item must be preselected.
 * @param aCallBackFunction callback function to display comments
 * @param aPos = position of the dialog.
 */
WinEDAListBox::WinEDAListBox( WinEDA_DrawFrame* aParent, const wxString& aTitle,
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

    GeneralBoxSizer->Add( m_listBox, 0, wxGROW | wxALL, 5 );

    InsertItems( aItemList, 0 );

    if( m_callBackFct )
    {
        m_messages = new wxTextCtrl( this, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize( -1, 60 ),
                                   wxTE_READONLY | wxTE_MULTILINE );

        GeneralBoxSizer->Add( m_messages, 0, wxGROW | wxALL, 5 );
    }

    wxSizer* buttonSizer = CreateButtonSizer( wxOK | wxCANCEL );

    if( buttonSizer )
        GeneralBoxSizer->Add( buttonSizer, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


WinEDAListBox::~WinEDAListBox()
{
}


void WinEDAListBox::MoveMouseToOrigin()
{
    int    x, y, w, h;
    wxSize list_size = m_listBox->GetSize();
    int    orgx = m_listBox->GetRect().GetLeft();
    int    orgy = m_listBox->GetRect().GetTop();

    wxClientDisplayRect( &x, &y, &w, &h );

    WarpPointer( x + orgx + 20, y + orgy + (list_size.y / 2) );
}


wxString WinEDAListBox::GetTextSelection()
{
    wxString text = m_listBox->GetStringSelection();
    return text;
}


void WinEDAListBox::Append( const wxString& item )
{
    m_listBox->Append( item );
}


void WinEDAListBox::InsertItems( const wxArrayString& itemlist, int position )
{
    m_listBox->InsertItems( itemlist, position );
}


void WinEDAListBox::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void WinEDAListBox::ClickOnList( wxCommandEvent& event )
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


void WinEDAListBox::D_ClickOnList( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}


void WinEDAListBox::OnOkClick( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}


void WinEDAListBox::OnClose( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


/* Sort alphabetically, case insensitive.
 */
static int SortItems( const wxString** ptr1, const wxString** ptr2 )
{
    return StrNumICmp( (*ptr1)->GetData(), (*ptr2)->GetData() );
}


void WinEDAListBox:: SortList()
{
    int ii, NbItems = m_listBox->GetCount();
    const wxString** BufList;

    if( NbItems <= 0 )
        return;

    BufList = (const wxString**) MyZMalloc( 100 * NbItems * sizeof(wxString*) );
    for( ii = 0; ii < NbItems; ii++ )
    {
        BufList[ii] = new wxString( m_listBox->GetString( ii ) );
    }

    qsort( BufList, NbItems, sizeof(wxString*),
           ( int( * ) ( const void*, const void* ) )SortItems );

    m_listBox->Clear();
    for( ii = 0; ii < NbItems; ii++ )
    {
        m_listBox->Append( *BufList[ii] );
        delete BufList[ii];
    }

    free( BufList );
}


void WinEDAListBox::OnKeyEvent( wxKeyEvent& event )
{
    event.Skip();
}
