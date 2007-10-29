/*************************************************************************/
/* listboxes.cpp: class for displaying footprint list and component list */
/*************************************************************************/

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"

#include "id.h"

#include "protos.h"

/******************************************************************************/
/* Basic class (from wxListView) for displaying component and footprint lists */
/* Not directly used: the 2 list boxes actually used are derived from it      */
/******************************************************************************/

ListBoxBase::ListBoxBase( WinEDA_CvpcbFrame* parent,
                          wxWindowID id, const wxPoint& loc, const wxSize& size ) :
    LIST_BOX_TYPE( parent, id, loc, size,
                   wxSUNKEN_BORDER | wxLC_NO_HEADER |
                   wxLC_SINGLE_SEL | wxLC_REPORT | wxLC_VIRTUAL )
{
    m_Parent = parent;
    InsertColumn( 0, wxEmptyString );
    SetColumnWidth( 0, wxLIST_AUTOSIZE );
}


ListBoxBase::~ListBoxBase()
{
}


/************************************************/
void ListBoxBase::OnSize( wxSizeEvent& event )
/************************************************/

// Ajust the column width to the entire available window width
{
    wxSize size  = GetClientSize();
    int    width = 0;

//	SetColumnWidth(0, wxLIST_AUTOSIZE );
//	width = GetColumnWidth(0);
    SetColumnWidth( 0, MAX( width, size.x ) );

    event.Skip();
}


/*********************************/
int ListBoxBase::GetSelection()
/*********************************/

// Return an index for the selected item
{
    return GetFirstSelected();
}


/***************************************/
/* ListBox handling the footprint list */
/***************************************/

FootprintListBox::FootprintListBox( WinEDA_CvpcbFrame* parent,
                                    wxWindowID id, const wxPoint& loc, const wxSize& size,
                                    int nbitems, wxString choice[] ) :
    ListBoxBase( parent, id, loc, size )
{
    SetActiveFootprintList( TRUE );
}


FootprintListBox::~FootprintListBox()
{
}


/**********************************/
int FootprintListBox::GetCount()
/**********************************/

// Return number of items
{
    return m_ActiveFootprintList->Count();
}


/*****************************************************************************/
void FootprintListBox::SetString( unsigned linecount, const wxString& text )
/*****************************************************************************/

// Change an item text
{
    if( linecount >= m_ActiveFootprintList->Count() )
        linecount = m_ActiveFootprintList->Count() - 1;
    if( linecount >= 0 )
        (*m_ActiveFootprintList)[linecount] = text;
}


/***************************************************/
wxString FootprintListBox::GetSelectedFootprint()
/***************************************************/

// Return an index for the selected item
{
    wxString FootprintName;
    int      ii = GetFirstSelected();

    if( ii >= 0 )
    {
        wxString msg = (*m_ActiveFootprintList)[ii];
        msg.Trim( TRUE ); msg.Trim( FALSE );
        FootprintName = msg.AfterFirst( wxChar( ' ' ) );
    }

    return FootprintName;
}


/*********************************************************/
void FootprintListBox::AppendLine( const wxString& text )
/*********************************************************/

// Add an item at end of list
{
    m_ActiveFootprintList->Add( text );
    SetItemCount( m_ActiveFootprintList->Count() );
}


/*********************************************************************/
wxString FootprintListBox::OnGetItemText( long item, long column ) const
/*********************************************************************/

/* Overlayed function: MUST be provided in wxLC_VIRTUAL mode
 *  because real datas are not handled by ListBoxBase
 */
{
    return m_ActiveFootprintList->Item( item );
}


/*****************************************************************/
void FootprintListBox::SetSelection( unsigned index, bool State )
/*****************************************************************/

// Enable or disable an item
{
    if( (int) index >= GetCount() )
        index = GetCount() - 1;

#ifndef __WXMAC__
    Select( index, State );
#endif
    EnsureVisible( index );
#ifdef __WXMAC__
    Refresh();
#endif
}


/**************************************************/
/* ListBox handling the schematic components list */
/**************************************************/

ListBoxCmp::ListBoxCmp( WinEDA_CvpcbFrame* parent, wxWindowID id,
                        const wxPoint& loc, const wxSize& size,
                        int nbitems, wxString choice[] ) :
    ListBoxBase( parent, id, loc, size )
{
}


ListBoxCmp::~ListBoxCmp()
{
}


/* Build the events table for the schematic components list box
 */

BEGIN_EVENT_TABLE( ListBoxCmp, LIST_BOX_TYPE )
EVT_SIZE( ListBoxBase::OnSize )

END_EVENT_TABLE()

/****************************/
void ListBoxCmp::Clear()
/****************************/

// Reset ALL datas
{
    m_ComponentList.Clear();
    SetItemCount( 0 );
}


/******************************/
int ListBoxCmp::GetCount()
/******************************/

// Return number of items
{
    return m_ComponentList.Count();
}


/********************************************************************/
void ListBoxCmp::SetString( unsigned linecount, const wxString& text )
/********************************************************************/

// Change an item text
{
    if( linecount >= m_ComponentList.Count() )
        linecount = m_ComponentList.Count() - 1;
    if( linecount >= 0 )
        m_ComponentList[linecount] = text;
}


/****************************************************/
void ListBoxCmp::AppendLine( const wxString& text )
/****************************************************/

// Add an item at end of list
{
    m_ComponentList.Add( text );
    SetItemCount( m_ComponentList.Count() );
}


/****************************************************************/
wxString ListBoxCmp::OnGetItemText( long item, long column ) const
/****************************************************************/

/* Overlayed function: MUST be provided in wxLC_VIRTUAL mode
 *  because real datas are not handled by ListBoxBase
 */
{
    return m_ComponentList.Item( item );
}


/********************************************************/
void ListBoxCmp::SetSelection( unsigned index, bool State )
/*********************************************************/

// Enable or disable an item
{
    if( (int) index >= GetCount() )
        index = GetCount() - 1;

#ifndef __WXMAC__
    Select( index, State );
#endif
    EnsureVisible( index );
#ifdef __WXMAC__
    Refresh();
#endif
}


/********************************************/
void WinEDA_CvpcbFrame::BuildCmpListBox()
/********************************************/

/* Create or update the schematic components list.
 */
{
    int       ii;
    STORECMP* Composant;
    wxString  msg;
    wxSize    size( 10, 10 );

    if( m_ListCmp == NULL )
    {
        m_ListCmp = new ListBoxCmp( this, ID_CVPCB_COMPONENT_LIST,
                                    wxDefaultPosition, size,
                                    0, NULL );
        m_ListCmp->SetBackgroundColour( wxColour( 225, 255, 255 ) );
        m_ListCmp->SetForegroundColour( wxColour( 0, 0, 0 ) );
        m_ListCmp->SetFont( *g_FixedFont );
    }

    m_ListCmp->m_ComponentList.Clear();
    Composant = g_BaseListeCmp;
    for( ii = 1; Composant != NULL; Composant = Composant->Pnext, ii++ )
    {
        msg.Printf( CMP_FORMAT, ii,
                   Composant->m_Reference.GetData(), Composant->m_Valeur.GetData(),
                   Composant->m_Module.GetData() );
        m_ListCmp->m_ComponentList.Add( msg );
    }

    m_ListCmp->SetItemCount( m_ListCmp->m_ComponentList.Count() );

    m_ListCmp->SetSelection( 0, TRUE );
}


/**********************************************/
void WinEDA_CvpcbFrame::BuildFootprintListBox()
/**********************************************/

/* Create or update the footprint list.
 */
{
    wxString msg;
    wxSize   size( 10, 10 );

    if( m_FootprintList == NULL )
    {
        m_FootprintList = new FootprintListBox( this, ID_CVPCB_FOOTPRINT_LIST,
                                                wxDefaultPosition, size,
                                                0, NULL );
        m_FootprintList->SetBackgroundColour( wxColour( 225, 255, 225 ) );
        m_FootprintList->SetForegroundColour( wxColour( 0, 0, 0 ) );
        m_FootprintList->SetFont( *g_FixedFont );
    }

    m_FootprintList->SetFootprintFullList();

    msg.Printf( _( "Footprints: %d" ), m_FootprintList->GetCount() );
    SetStatusText( msg, 2 );
}


/************************************************/
void FootprintListBox::SetFootprintFullList()
/************************************************/
{
    STOREMOD* FootprintItem;
    wxString  msg;
    int       OldSelection = GetSelection();

    m_FullFootprintList.Clear();
    FootprintItem = g_BaseListePkg;

    for( int ii = 1; FootprintItem != NULL; FootprintItem = FootprintItem->Pnext, ii++ )
    {
        msg.Printf( wxT( "%3d %s" ), ii, FootprintItem->m_Module.GetData() );
        m_FullFootprintList.Add( msg );
    }

    SetActiveFootprintList( TRUE );

    if( (GetCount() == 0) ||  (OldSelection < 0) || ( OldSelection >= GetCount() ) )
        SetSelection( 0, TRUE );
    Refresh();
}


/**********************************************************************/
void FootprintListBox::SetFootprintFilteredList( STORECMP* Component )
/*********************************************************************/
{
    STOREMOD* FootprintItem;
    wxString  msg;
    int       OldSelection = GetSelection();
    bool      HasItem = FALSE;

    m_FilteredFootprintList.Clear();
    FootprintItem = g_BaseListePkg;

    int cmpnum = 1;
    for( int ii = 0; FootprintItem != NULL; FootprintItem = FootprintItem->Pnext, ii++ )
    {
        /* Search for matching footprints */
        for( unsigned jj = 0; jj < Component->m_FootprintFilter.GetCount(); jj++ )
        {
            if( !FootprintItem->m_Module.Matches( Component->m_FootprintFilter[jj] ) )
                continue;
            msg.Printf( wxT( "%3d %s" ), cmpnum++, FootprintItem->m_Module.GetData() );
            m_FilteredFootprintList.Add( msg );
            HasItem = TRUE;
        }
    }

    if( HasItem )
        SetActiveFootprintList( FALSE );
    else
        SetActiveFootprintList( TRUE );

    if( (GetCount() == 0) || ( OldSelection >= GetCount() ) )
        SetSelection( 0, TRUE );

    Refresh();
}


/**************************************************************************/
void FootprintListBox::SetActiveFootprintList( bool FullList, bool Redraw )
/**************************************************************************/

/** Set the footprint list. We can have 2 footprint list:
 *  The full footprint list
 *  The filtered footprint list (if the current selected component has a filter for footprints)
 *  @param FullList true = full footprint list, false = filtered footprint list
 *  @param Redraw = true to redraw the window
 */
{
    bool old_selection = m_UseFootprintFullList;

    /* Workaround for a curious bug in wxWidgets:
     *  if we switch from a long list of footprints to a short list (a filtered footprint list),
     *  and if the selected item is near the end of the long list,
     *  the new list is not displayed from the top of the list box
     */
	if ( m_ActiveFootprintList )
	{
		bool new_selection;
		if( FullList ) new_selection = TRUE;
		else new_selection = FALSE;
		if( new_selection != old_selection )
			SetSelection( 0, TRUE );
	}

    if( FullList )
    {
        m_UseFootprintFullList = TRUE;
        m_ActiveFootprintList  = &m_FullFootprintList;
        SetItemCount( m_FullFootprintList.GetCount() );
    }
    else
    {
        m_UseFootprintFullList = FALSE;
        m_ActiveFootprintList  = &m_FilteredFootprintList;
        SetItemCount( m_FilteredFootprintList.GetCount() );
    }

    if( Redraw )
    {
        if( !m_UseFootprintFullList || (m_UseFootprintFullList != old_selection) )
        {
            Refresh();
        }
    }

    if( !m_UseFootprintFullList || (m_UseFootprintFullList != old_selection) )
    {
        m_Parent->SetStatusText( wxEmptyString, 0 );
        m_Parent->SetStatusText( wxEmptyString, 1 );
    }

    wxString msg;
    if( FullList )
        msg.Printf( _( "Footprints (All): %d" ), m_ActiveFootprintList->GetCount() );
    else
        msg.Printf( _( "Footprints (filtered): %d" ), m_ActiveFootprintList->GetCount() );
    m_Parent->SetStatusText( msg, 2 );
}


/**************************************/
/* Event table for the footprint list */
/**************************************/

BEGIN_EVENT_TABLE( FootprintListBox, LIST_BOX_TYPE )
EVT_SIZE( ListBoxBase::OnSize )

END_EVENT_TABLE()


/********************************************************/
void FootprintListBox::OnLeftClick( wxListEvent& event )
/********************************************************/
{
    STOREMOD* Module;
    wxString  msg;
    wxString  FootprintName = GetSelectedFootprint();

    Module = GetModuleDescrByName( FootprintName );
    if( m_Parent->DrawFrame )
    {
        m_Parent->CreateScreenCmp(); /* refresh general */
    }

    if( Module )
        msg = Module->m_Doc;
    m_Parent->SetStatusText( msg, 0 );

    msg = wxT( "KeyW: " );
    if( Module )
        msg += Module->m_KeyWord;
    m_Parent->SetStatusText( msg, 1 );
}


/******************************************************/
void FootprintListBox::OnLeftDClick( wxListEvent& event )
/******************************************************/
{
    wxString FootprintName = GetSelectedFootprint();

    m_Parent->SetNewPkg( FootprintName );
}


/**************************************************************/
STOREMOD* GetModuleDescrByName( const wxString& FootprintName )
/**************************************************************/
{
    STOREMOD* FootprintItem = g_BaseListePkg;

    for( ; FootprintItem != NULL; FootprintItem = FootprintItem->Pnext )
    {
        if( FootprintItem->m_Module == FootprintName )
            break; // found !
    }

    return FootprintItem;
}
