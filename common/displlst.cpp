/**
 * @file displlst.cpp
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <kicad_string.h>
#include <dialog_helpers.h>

EDA_LIST_DIALOG::EDA_LIST_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aTitle,
                                  const wxArrayString& aItemHeaders,
                                  const std::vector<wxArrayString>& aItemList,
                                  const wxString& aRefText,
                                  void(*aCallBackFunction)(wxString& Text),
                                  bool aSortList ) :
    EDA_LIST_DIALOG_BASE( aParent, wxID_ANY, aTitle )
{
    m_sortList    = aSortList;
    m_callBackFct = aCallBackFunction;
    m_itemsListCp = &aItemList;

    for( unsigned i = 0; i < aItemHeaders.Count(); i++ )
    {
        wxListItem column;
        column.SetId( i );
        column.SetText( aItemHeaders.Item( i ) );
        column.SetWidth( 300 / aItemHeaders.Count() );
        EDA_LIST_DIALOG_BASE::m_listBox->InsertColumn( i, column );
    }
    
    InsertItems( aItemList, 0 );
    
    if( m_sortList )
        sortList();

    if( !aRefText.IsEmpty() )    // try to select the item matching aRefText
    {
        for( unsigned ii = 0; ii < aItemList.size(); ii++ )
            if( aItemList[ii][0] == aRefText )
            {
                m_listBox->SetItemState( ii, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
                break;
            }
    }

    if( m_callBackFct == NULL )
    {
        m_messages->Show( false );
        m_staticTextMsg->Show( false );
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

    filter = wxT( "*" ) + m_filterBox->GetLineText( 0 ).MakeLower() + wxT( "*" );

    m_listBox->DeleteAllItems();

    for( unsigned i = 0; i < m_itemsListCp->size(); i++ )
    {
        itemName = (*m_itemsListCp)[i].Item( 0 );

        if( itemName.MakeLower().Matches( filter ) )
        {
            Append( (*m_itemsListCp)[i] );
        }
    }

    if( m_sortList )
        sortList();
}

wxString EDA_LIST_DIALOG::GetTextSelection()
{
    long item = -1;
    item = m_listBox->GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
    wxString text = m_listBox->GetItemText( item );
    return text;
}


void EDA_LIST_DIALOG::Append( const wxArrayString& itemList )
{
    long itemIndex = m_listBox->InsertItem( m_listBox->GetItemCount(), itemList[0] );

    m_listBox->SetItemData( itemIndex, (long) &(itemList[0]) );
    
    // Adding the next columns content
    for( unsigned i = 1; i < itemList.size(); i++ )
    {
        m_listBox->SetItem( itemIndex, i, itemList[i] );
    }
}

void EDA_LIST_DIALOG::InsertItems( const std::vector<wxArrayString>& itemList,
                                   int position )
{
    for( unsigned i = 0; i < itemList.size(); i++ )
    {
        long itemIndex = m_listBox->InsertItem( position+i, itemList[i].Item( 0 ) );
        m_listBox->SetItemData( itemIndex, (long) &( itemList[i].Item( 0 ) ) );
        
        // Adding the next columns content
        for( unsigned j = 1; j < itemList[i].GetCount(); j++ )
        {
            m_listBox->SetItem( itemIndex, j, itemList[i].Item( j ) );
        }
    }

    if( m_sortList )
        sortList();
}


void EDA_LIST_DIALOG::onCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void EDA_LIST_DIALOG::onListItemSelected( wxListEvent& event )
{
    
    if( m_callBackFct )
    {
        m_messages->Clear();
        wxString text = GetTextSelection();
        m_callBackFct( text );
        m_messages->WriteText( text );
    }
}


void EDA_LIST_DIALOG::onListItemActivated( wxListEvent& event )
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
static int wxCALLBACK MyCompareFunction( long aItem1, long aItem2, long aSortData )
{  
    wxString* component1Name = (wxString*) aItem1;
    wxString* component2Name = (wxString*) aItem2;
    
    return StrNumCmp( *component1Name, *component2Name, INT_MAX, true ); 
}

void EDA_LIST_DIALOG::sortList()
{
    m_listBox->SortItems( MyCompareFunction, 0 );
}
