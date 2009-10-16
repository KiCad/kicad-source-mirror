/*********************************/
/*  get_component_dialog.cpp     */
/*********************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "wxstruct.h"
#include "get_component_dialog.h"


/****************************************************************************/
/* Show a dialog frame to choose a name from an history list, or a new name */
/* to select a component or a module                                        */
/****************************************************************************/

static unsigned s_HistoryMaxCount = 8;  // Max number of items displayed in history list


BEGIN_EVENT_TABLE( WinEDA_SelectCmp, wxDialog )
    EVT_BUTTON( ID_ACCEPT_NAME, WinEDA_SelectCmp::Accept )
    EVT_BUTTON( ID_ACCEPT_KEYWORD, WinEDA_SelectCmp::Accept )
    EVT_BUTTON( ID_CANCEL, WinEDA_SelectCmp::Accept )
    EVT_BUTTON( ID_LIST_ALL, WinEDA_SelectCmp::Accept )
    EVT_BUTTON( ID_EXTRA_TOOL, WinEDA_SelectCmp::GetExtraSelection )
    EVT_LISTBOX( ID_SEL_BY_LISTBOX, WinEDA_SelectCmp::Accept )
END_EVENT_TABLE()


/*
 * Dialog frame to choose a component or a footprint
 *   This dialog shows an history of last selected items
 */
WinEDA_SelectCmp::WinEDA_SelectCmp( WinEDA_DrawFrame* parent,
                                    const wxPoint&    framepos,
                                    wxArrayString&    HistoryList,
                                    const wxString&   Title,
                                    bool              show_extra_tool ) :
    wxDialog( parent, -1, Title, framepos, wxDefaultSize, DIALOG_STYLE )
{
    wxButton*     Button;
    wxStaticText* Text;

    m_AuxTool = show_extra_tool;
    m_GetExtraFunction = false;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );

    wxBoxSizer* LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer,
                       0,
                       wxALIGN_CENTER_HORIZONTAL | wxALL | wxADJUST_MINSIZE,
                       5 );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5 );

    Text = new wxStaticText( this, -1, _( "Name:" ) );
    LeftBoxSizer->Add( Text, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_TextCtrl = new wxTextCtrl( this, ID_ENTER_NAME, *m_Text );
    m_TextCtrl->SetInsertionPoint( 1 );
    LeftBoxSizer->Add( m_TextCtrl,
                       0,
                       wxGROW | wxLEFT | wxRIGHT | wxBOTTOM | wxADJUST_MINSIZE,
                       5 );

    Text = new wxStaticText( this, -1, _( "History list:" ) );
    LeftBoxSizer->Add( Text, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_List = new wxListBox( this, ID_SEL_BY_LISTBOX, wxDefaultPosition,
                            wxSize( 220, -1 ), HistoryList, wxLB_SINGLE );
    LeftBoxSizer->Add( m_List,
                       0,
                       wxGROW | wxLEFT | wxRIGHT | wxBOTTOM | wxADJUST_MINSIZE,
                       5 );

    Button = new wxButton( this, ID_ACCEPT_NAME, _( "OK" ) );
    Button->SetDefault();
    RightBoxSizer->Add( Button,
                        0,
                        wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM,
                        5 );

    Button = new wxButton( this, ID_ACCEPT_KEYWORD, _( "Search by Keyword" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    Button = new wxButton( this, ID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    Button = new wxButton( this, ID_LIST_ALL, _( "List All" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

#ifndef __WXMAC__
    if( m_AuxTool )     /* The selection can be done by an extra function */
    {
        Button = new wxButton( this, ID_EXTRA_TOOL, _( "Select by Browser" ) );
        RightBoxSizer->Add( Button, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    }
#endif

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void WinEDA_SelectCmp::Accept( wxCommandEvent& event )
{
    int id = wxID_OK;

    switch( event.GetId() )
    {
    case ID_SEL_BY_LISTBOX:
        m_Text = m_List->GetStringSelection();
        break;

    case ID_ACCEPT_NAME:
        m_Text = m_TextCtrl->GetValue();
        break;

    case ID_ACCEPT_KEYWORD:
        m_Text = wxT( "= " ) + m_TextCtrl->GetValue();
        break;

    case ID_CANCEL:
        m_Text = wxEmptyString;
        id = wxID_CANCEL;
        break;

    case ID_LIST_ALL:
        m_Text = wxT( "*" );
        break;
    }

    m_Text.Trim( false );      // Remove blanks at beginning
    m_Text.Trim( true );       // Remove blanks at end

    if( IsModal() )
        EndModal( id );
    else
        Close( id );
}


/* Get the component name by the extra function */
void WinEDA_SelectCmp::GetExtraSelection( wxCommandEvent& event )
{
    m_GetExtraFunction = true;

    if( IsModal() )
        EndModal( wxID_OK );
    else
        Close( wxID_OK );
}


wxString WinEDA_SelectCmp::GetComponentName( void )
{
    return m_Text;
}


void WinEDA_SelectCmp::SetComponentName( const wxString& name )
{
    if( m_TextCtrl )
        m_TextCtrl->SetValue( name );
}


wxPoint GetComponentDialogPosition( void )
{
    wxPoint pos;
    int     x, y, w, h;

    pos = wxGetMousePosition();
    wxClientDisplayRect( &x, &y, &w, &h );
    pos.x -= 100;
    pos.y -= 50;
    if( pos.x < x )
        pos.x = x;
    if( pos.y < y )
        pos.y = y;
    if( pos.x < x )
        pos.x = x;
    x += w - 350;
    if( pos.x > x )
        pos.x = x;
    if( pos.y < y )
        pos.y = y;

    return pos;
}


/*
 * Add the string "Name" to the history list HistoryList
 */
void AddHistoryComponentName( wxArrayString& HistoryList, const wxString& Name )
{
    int ii, c_max;

    if( HistoryList.GetCount() > 0 )
    {
        if( Name == HistoryList[0] )
            return;

        /* remove an old identical selection if exists */
        for( ii = 1; (unsigned) ii < HistoryList.GetCount(); ii++ )
        {
            if( Name == HistoryList[ii] )
            {
                HistoryList.RemoveAt( ii ); ii--;
            }
        }

        /* shift the list */
        if( HistoryList.GetCount() < s_HistoryMaxCount )
            HistoryList.Add( wxT( "" ) );

        c_max = HistoryList.GetCount() - 2;
        for( ii = c_max; ii >= 0; ii-- )
            HistoryList[ii + 1] = HistoryList[ii];

        /* Add the new name at the beginning of the history list */
        HistoryList[0] = Name;
    }
    else
        HistoryList.Add( Name );
}
