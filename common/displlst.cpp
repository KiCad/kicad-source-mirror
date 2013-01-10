/**
 * @file displlst.cpp
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <kicad_string.h>
#include <dialog_helpers.h>


EDA_LIST_DIALOG::EDA_LIST_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aTitle,
                                  const wxArrayString& aItemList, const wxString& aRefText,
                                  void(* aCallBackFunction)(wxString& Text),
                                  bool aSortList ) :
    EDA_LIST_DIALOG_BASE( aParent, wxID_ANY, aTitle )
{
    m_sortList = aSortList;
    m_callBackFct = aCallBackFunction;
    m_itemsListCp = &aItemList;

    InsertItems( aItemList, 0 );
    if( m_sortList )
        sortList();

    if( !aRefText.IsEmpty() )    // try to select the item matching aRefText
    {
        for( unsigned ii = 0; ii < aItemList.GetCount(); ii++ )
            if( aItemList[ii] == aRefText )
            {
                m_listBox->SetSelection( ii );
                break;
            }
    }

    if( m_callBackFct == NULL )
    {
        m_messages->Show(false);
        m_staticTextMsg->Show(false);
    }

    m_filterBox->SetFocus();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


EDA_LIST_DIALOG::~EDA_LIST_DIALOG()
{
}


void EDA_LIST_DIALOG::textChangeInFilterBox( wxCommandEvent& event )
{
    wxString filter;
    wxString itemName;

    filter = wxT("*") + m_filterBox->GetLineText(0).MakeLower() + wxT("*");

    m_listBox->Clear();

    for(unsigned i = 0; i < m_itemsListCp->GetCount(); i++)
    {
        itemName = m_itemsListCp->Item(i);

        if( itemName.MakeLower().Matches(filter) )
        {
            m_listBox->Insert(m_itemsListCp->Item(i),m_listBox->GetCount());
        }
    }

    if( m_sortList )
        sortList();
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

    if( m_sortList )
        sortList();
}


void EDA_LIST_DIALOG::onCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void EDA_LIST_DIALOG::onClickOnList( wxCommandEvent& event )
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


void EDA_LIST_DIALOG::onDClickOnList( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}


void EDA_LIST_DIALOG::onOkClick( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}


void EDA_LIST_DIALOG::onClose( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


/* Sort alphabetically, case insensitive.
 */
static int sortItems( const wxString& item1, const wxString& item2 )
{
    return StrNumCmp( item1, item2, INT_MAX, true );
}


void EDA_LIST_DIALOG::sortList()
{
    wxArrayString list = m_listBox->GetStrings();

    if( list.IsEmpty() )
        return;

    list.Sort( sortItems );
    m_listBox->Clear();
    m_listBox->Append( list );
}
