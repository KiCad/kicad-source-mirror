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


/******************************************************************************
* Basic class (from wxListView) to display component and footprint lists
* Not directly used: the 2 list boxes actually used are derived from it
******************************************************************************/

#define LISTB_STYLE wxSUNKEN_BORDER | wxLC_NO_HEADER | \
    wxLC_SINGLE_SEL | wxLC_REPORT | wxLC_VIRTUAL

ITEMS_LISTBOX_BASE::ITEMS_LISTBOX_BASE( WinEDA_CvpcbFrame* aParent, wxWindowID aId,
                                        const wxPoint& aLocation, const wxSize& aSize ) :
    wxListView( aParent, aId, aLocation, aSize, LISTB_STYLE )
{
    InsertColumn( 0, wxEmptyString );
    SetColumnWidth( 0, wxLIST_AUTOSIZE );
}


ITEMS_LISTBOX_BASE::~ITEMS_LISTBOX_BASE()
{
}


/*
 * Adjust the column width to the entire available window width
 */
void ITEMS_LISTBOX_BASE::OnSize( wxSizeEvent& event )
{
    wxSize size  = GetClientSize();
    int    width = 0;

    SetColumnWidth( 0, MAX( width, size.x ) );

    event.Skip();
}


/*
 * Return an index for the selected item
 */
int ITEMS_LISTBOX_BASE::GetSelection()
{
    return GetFirstSelected();
}


WinEDA_CvpcbFrame* ITEMS_LISTBOX_BASE::GetParent()
{
    return (WinEDA_CvpcbFrame*) wxListView::GetParent();
}


/*
 * Create or update the schematic components list.
 */
void WinEDA_CvpcbFrame::BuildCmpListBox()
{
    wxString msg;
    wxSize   size( 10, 10 );
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_ListCmp == NULL )
    {
        m_ListCmp = new COMPONENTS_LISTBOX( this, ID_CVPCB_COMPONENT_LIST,
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

    BOOST_FOREACH( COMPONENT & component, m_components ) {
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
void WinEDA_CvpcbFrame::BuildFOOTPRINTS_LISTBOX()
{
    wxString msg;
    wxSize   size( 10, 10 );
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_FootprintList == NULL )
    {
        m_FootprintList = new FOOTPRINTS_LISTBOX( this, ID_CVPCB_FOOTPRINT_LIST,
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
