/*************************************************************************/
/* listboxes.cpp: class for displaying footprint list and component list */
/*************************************************************************/

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "id.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


/******************************************************************************/
/* Basic class (from wxListView) for displaying component and footprint lists */
/* Not directly used: the 2 list boxes actually used are derived from it      */
/******************************************************************************/

ListBoxBase::ListBoxBase( WinEDA_CvpcbFrame* parent,  wxWindowID id,
                          const wxPoint& loc, const wxSize& size ) :
    wxListView( parent, id, loc, size,
                wxSUNKEN_BORDER | wxLC_NO_HEADER |
                wxLC_SINGLE_SEL | wxLC_REPORT | wxLC_VIRTUAL )
{
    InsertColumn( 0, wxEmptyString );
    SetColumnWidth( 0, wxLIST_AUTOSIZE );
}


ListBoxBase::~ListBoxBase()
{
}


/*
 * Adjust the column width to the entire available window width
 */
void ListBoxBase::OnSize( wxSizeEvent& event )
{
    wxSize size  = GetClientSize();
    int    width = 0;

    SetColumnWidth( 0, MAX( width, size.x ) );

    event.Skip();
}


/*
 * Return an index for the selected item
 */
int ListBoxBase::GetSelection()
{
    return GetFirstSelected();
}


WinEDA_CvpcbFrame* ListBoxBase::GetParent()
{
    return (WinEDA_CvpcbFrame*) wxListView::GetParent();
}


/***************************************/
/* ListBox handling the footprint list */
/***************************************/

FootprintListBox::FootprintListBox( WinEDA_CvpcbFrame* parent,
                                    wxWindowID id, const wxPoint& loc,
                                    const wxSize& size,
                                    int nbitems, wxString choice[] ) :
    ListBoxBase( parent, id, loc, size )
{
    m_UseFootprintFullList = true;
	m_ActiveFootprintList = NULL;
    SetActiveFootprintList( TRUE );
}


FootprintListBox::~FootprintListBox()
{
}


/*
 * Return number of items
 */
int FootprintListBox::GetCount()
{
    return m_ActiveFootprintList->Count();
}


/*
 * Change an item text
 */
void FootprintListBox::SetString( unsigned linecount, const wxString& text )
{
    if( linecount >= m_ActiveFootprintList->Count() )
        linecount = m_ActiveFootprintList->Count() - 1;
    if( linecount >= 0 )
        (*m_ActiveFootprintList)[linecount] = text;
}


wxString FootprintListBox::GetSelectedFootprint()
{
    wxString FootprintName;
    int      ii = GetFirstSelected();

    if( ii >= 0 )
    {
        wxString msg = (*m_ActiveFootprintList)[ii];
        msg.Trim( TRUE );
        msg.Trim( FALSE );
        FootprintName = msg.AfterFirst( wxChar( ' ' ) );
    }

    return FootprintName;
}


void FootprintListBox::AppendLine( const wxString& text )
{
    m_ActiveFootprintList->Add( text );
    SetItemCount( m_ActiveFootprintList->Count() );
}


/*
 * Overlaid function: MUST be provided in wxLC_VIRTUAL mode
 * because real data is not handled by ListBoxBase
 */
wxString FootprintListBox::OnGetItemText( long item, long column ) const
{
    return m_ActiveFootprintList->Item( item );
}


/*
 * Enable or disable an item
 */
void FootprintListBox::SetSelection( unsigned index, bool State )
{
    if( (int) index >= GetCount() )
        index = GetCount() - 1;

	if ( (index >= 0)  && (GetCount() > 0) )
	{
#ifndef __WXMAC__
		Select( index, State );
#endif
		EnsureVisible( index );
#ifdef __WXMAC__
		Refresh();
#endif
	}
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

BEGIN_EVENT_TABLE( ListBoxCmp, ListBoxBase )
    EVT_SIZE( ListBoxBase::OnSize )
END_EVENT_TABLE()


void ListBoxCmp::Clear()
{
    m_ComponentList.Clear();
    SetItemCount( 0 );
}


int ListBoxCmp::GetCount()
{
    return m_ComponentList.Count();
}


void ListBoxCmp::SetString( unsigned linecount, const wxString& text )
{
    if( linecount >= m_ComponentList.Count() )
        linecount = m_ComponentList.Count() - 1;
    if( linecount >= 0 )
        m_ComponentList[linecount] = text;
}


void ListBoxCmp::AppendLine( const wxString& text )
{
    m_ComponentList.Add( text );
    SetItemCount( m_ComponentList.Count() );
}


/*
 * Overlaid function: MUST be provided in wxLC_VIRTUAL mode
 * because real data are not handled by ListBoxBase
 */
wxString ListBoxCmp::OnGetItemText( long item, long column ) const
{
    return m_ComponentList.Item( item );
}


/*
 * Enable or disable an item
 */
void ListBoxCmp::SetSelection( unsigned index, bool State )
{
    if( (int) index >= GetCount() )
        index = GetCount() - 1;

	if ( (index >= 0) && (GetCount() > 0) )
	{
#ifndef __WXMAC__
		Select( index, State );
#endif
		EnsureVisible( index );
#ifdef __WXMAC__
		Refresh();
#endif
	}
}


/*
 * Create or update the schematic components list.
 */
void WinEDA_CvpcbFrame::BuildCmpListBox()
{
    wxString   msg;
    wxSize     size( 10, 10 );
    wxFont     guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_ListCmp == NULL )
    {
        m_ListCmp = new ListBoxCmp( this, ID_CVPCB_COMPONENT_LIST,
                                    wxDefaultPosition, size,
                                    0, NULL );
        m_ListCmp->SetBackgroundColour( wxColour( 225, 255, 255 ) );
        m_ListCmp->SetForegroundColour( wxColour( 0, 0, 0 ) );
        m_ListCmp->SetFont( wxFont( guiFont.GetPointSize(),
                                    wxFONTFAMILY_MODERN,
                                    wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_NORMAL ) );
    }

    m_ListCmp->m_ComponentList.Clear();

    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        msg.Printf( CMP_FORMAT, m_ListCmp->GetCount() + 1,
                    component.m_Reference.GetData(),
                    component.m_Value.GetData(),
                    component.m_Module.GetData() );
        m_ListCmp->m_ComponentList.Add( msg );
    }

    m_ListCmp->SetItemCount( m_ListCmp->m_ComponentList.Count() );
    m_ListCmp->SetSelection( 0, TRUE );
}



/*
 * Create or update the footprint list.
 */
void WinEDA_CvpcbFrame::BuildFootprintListBox()
{
    wxString msg;
    wxSize   size( 10, 10 );
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_FootprintList == NULL )
    {
        m_FootprintList = new FootprintListBox( this, ID_CVPCB_FOOTPRINT_LIST,
                                                wxDefaultPosition, size,
                                                0, NULL );
        m_FootprintList->SetBackgroundColour( wxColour( 225, 255, 225 ) );
        m_FootprintList->SetForegroundColour( wxColour( 0, 0, 0 ) );
        m_FootprintList->SetFont( wxFont( guiFont.GetPointSize(),
                                          wxFONTFAMILY_MODERN,
                                          wxFONTSTYLE_NORMAL,
                                          wxFONTWEIGHT_NORMAL ) );
    }

    m_FootprintList->SetFootprintFullList( m_footprints );

    msg.Printf( _( "Footprints: %d" ), m_FootprintList->GetCount() );
    SetStatusText( msg, 2 );
}


void FootprintListBox::SetFootprintFullList( FOOTPRINT_LIST& list )
{
    wxString   msg;
    int        OldSelection = GetSelection();

    m_FullFootprintList.Clear();

    BOOST_FOREACH( FOOTPRINT& footprint, list )
    {
        msg.Printf( wxT( "%3d %s" ), m_FullFootprintList.GetCount() + 1,
                    footprint.m_Module.GetData() );
        m_FullFootprintList.Add( msg );
    }

    SetActiveFootprintList( TRUE );

    if( ( GetCount() == 0 )
        || ( OldSelection < 0 ) || ( OldSelection >= GetCount() ) )
        SetSelection( 0, TRUE );
    Refresh();
}


void FootprintListBox::SetFootprintFilteredList( COMPONENT* Component,
                                                 FOOTPRINT_LIST& list )
{
    FOOTPRINT_LIST::iterator i;
    wxString   msg;
    unsigned   jj;
    int        OldSelection = GetSelection();
    bool       HasItem = FALSE;

    m_FilteredFootprintList.Clear();

    BOOST_FOREACH( FOOTPRINT& footprint, list )
    {
        /* Search for matching footprints */
        for( jj = 0; jj < Component->m_FootprintFilter.GetCount(); jj++ )
        {
            if( !footprint.m_Module.Matches( Component->m_FootprintFilter[jj] ) )
                continue;
            msg.Printf( wxT( "%3d %s" ), m_FilteredFootprintList.GetCount() + 1,
                        footprint.m_Module.GetData() );
            m_FilteredFootprintList.Add( msg );
            HasItem = TRUE;
        }
    }

    if( HasItem )
        SetActiveFootprintList( FALSE );
    else
        SetActiveFootprintList( TRUE );

    if( ( GetCount() == 0 ) || ( OldSelection >= GetCount() ) )
        SetSelection( 0, TRUE );

    Refresh();
}


/** Set the footprint list. We can have 2 footprint list:
 *  The full footprint list
 *  The filtered footprint list (if the current selected component has a
 * filter for footprints)
 *  @param FullList true = full footprint list, false = filtered footprint list
 *  @param Redraw = true to redraw the window
 */
void FootprintListBox::SetActiveFootprintList( bool FullList, bool Redraw )
{
    bool old_selection = m_UseFootprintFullList;

#ifdef __WINDOWS__
    /* Workaround for a curious bug in wxWidgets:
     * if we switch from a long list of footprints to a short list (a
     * filtered footprint list), and if the selected item is near the end
     * of the long list,  the new list is not displayed from the top of
     * the list box
     */
	if ( m_ActiveFootprintList )
	{
		bool new_selection;
		if( FullList )
            new_selection = TRUE;
		else
            new_selection = FALSE;
		if( new_selection != old_selection )
			SetSelection( 0, TRUE );
	}
#endif
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
        if( !m_UseFootprintFullList
            || ( m_UseFootprintFullList != old_selection ) )
        {
            Refresh();
        }
    }

    if( !m_UseFootprintFullList || ( m_UseFootprintFullList != old_selection ) )
    {
        GetParent()->SetStatusText( wxEmptyString, 0 );
        GetParent()->SetStatusText( wxEmptyString, 1 );
    }

    wxString msg;
    if( FullList )
        msg.Printf( _( "Footprints (All): %d" ),
                    m_ActiveFootprintList->GetCount() );
    else
        msg.Printf( _( "Footprints (filtered): %d" ),
                    m_ActiveFootprintList->GetCount() );
    GetParent()->SetStatusText( msg, 2 );
}


/**************************************/
/* Event table for the footprint list */
/**************************************/

BEGIN_EVENT_TABLE( FootprintListBox, ListBoxBase )
    EVT_SIZE( ListBoxBase::OnSize )
END_EVENT_TABLE()


/********************************************************/
void FootprintListBox::OnLeftClick( wxListEvent& event )
/********************************************************/
{
    FOOTPRINT* Module;
    wxString  FootprintName = GetSelectedFootprint();

    Module = GetModuleDescrByName( FootprintName, GetParent()->m_footprints );
    if( GetParent()->DrawFrame )
    {
        GetParent()->CreateScreenCmp(); /* refresh general */
    }

    if( Module )
    {
        wxString  msg;
        msg = Module->m_Doc;
        GetParent()->SetStatusText( msg, 0 );

        msg = wxT( "KeyW: " );
        msg += Module->m_KeyWord;
        GetParent()->SetStatusText( msg, 1 );
    }
}


/******************************************************/
void FootprintListBox::OnLeftDClick( wxListEvent& event )
/******************************************************/
{
    wxString FootprintName = GetSelectedFootprint();

    GetParent()->SetNewPkg( FootprintName );
}


FOOTPRINT* GetModuleDescrByName( const wxString& FootprintName,
                                 FOOTPRINT_LIST& list )
{
    BOOST_FOREACH( FOOTPRINT& footprint, list )
    {
        if( footprint.m_Module == FootprintName )
            return &footprint;
    }

    return NULL;
}
