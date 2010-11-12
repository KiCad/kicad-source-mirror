/*************************************************************************/
/* listboxes.cpp: class for displaying footprint list and component list */
/*************************************************************************/

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


/**************************************************/
/* ListBox handling the schematic components list */
/**************************************************/

COMPONENTS_LISTBOX::COMPONENTS_LISTBOX( WinEDA_CvpcbFrame* parent, wxWindowID id,
                                        const wxPoint& loc, const wxSize& size,
                                        int nbitems, wxString choice[] ) :
    ITEMS_LISTBOX_BASE( parent, id, loc, size )
{
}


COMPONENTS_LISTBOX::~COMPONENTS_LISTBOX()
{
}


/* Build the events table for the schematic components list box
 */

BEGIN_EVENT_TABLE( COMPONENTS_LISTBOX, ITEMS_LISTBOX_BASE )
    EVT_SIZE( ITEMS_LISTBOX_BASE::OnSize )
    EVT_CHAR( COMPONENTS_LISTBOX::OnChar )
END_EVENT_TABLE()


void COMPONENTS_LISTBOX::Clear()
{
    m_ComponentList.Clear();
    SetItemCount( 0 );
}


int COMPONENTS_LISTBOX::GetCount()
{
    return m_ComponentList.Count();
}


void COMPONENTS_LISTBOX::SetString( unsigned linecount, const wxString& text )
{
    if( linecount >= m_ComponentList.Count() )
        linecount = m_ComponentList.Count() - 1;
    if( linecount >= 0 )
        m_ComponentList[linecount] = text;
}


void COMPONENTS_LISTBOX::AppendLine( const wxString& text )
{
    m_ComponentList.Add( text );
    SetItemCount( m_ComponentList.Count() );
}


/*
 * Overlaid function: MUST be provided in wxLC_VIRTUAL mode
 * because real data are not handled by ITEMS_LISTBOX_BASE
 */
wxString COMPONENTS_LISTBOX::OnGetItemText( long item, long column ) const
{
    return m_ComponentList.Item( item );
}


/*
 * Enable or disable an item
 */
void COMPONENTS_LISTBOX::SetSelection( unsigned index, bool State )
{
    if( (int) index >= GetCount() )
        index = GetCount() - 1;

    if( (index >= 0) && (GetCount() > 0) )
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


/**
 * Function OnChar
 * called on a key pressed
 * Call default handler for some special keys,
 * and for "ascii" keys, select the first component
 * that the name starts by the letter.
 * This is the defaut behaviour of a listbox, but because we use
 * virtual lists, the listbox does not know anything to what is displayed,
 * we must handle this behaviour here.
 * Furthermore the reference of components is not at the beginning of
 * displayed lines (the first word is the line number)
 */
void COMPONENTS_LISTBOX::OnChar( wxKeyEvent& event )
{
    int key = event.GetKeyCode();
    switch( key )
    {
        case WXK_HOME:
        case WXK_END:
         case WXK_UP:
        case WXK_DOWN:
        case WXK_PAGEUP:
        case WXK_PAGEDOWN:
        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:
            event.Skip();
            return;

        case WXK_RIGHT:
        case WXK_NUMPAD_RIGHT:
            GetParent()->m_FootprintList->SetFocus();
            return;

        default:
             break;
    }

    // Search for an item name starting by the key code:
    key = toupper(key);
    for( unsigned ii = 0; ii < m_ComponentList.GetCount(); ii++ )
    {
        wxString text = m_ComponentList.Item(ii);
        /* search for the start char of the footprint name.
         * we must skip the line number
        */
        text.Trim(false);      // Remove leading spaces in line
        unsigned jj = 0;
        for( ; jj < text.Len(); jj++ )
        {   // skip line number
            if( text[jj] == ' ' )
                break;
        }
        for( ; jj < text.Len(); jj++ )
        {   // skip blanks
            if( text[jj] != ' ' )
                break;
        }
        int start_char = toupper(text[jj]);
        if ( key == start_char )
        {
            Focus( ii );
            SetSelection( ii, true );   // Ensure visible
            break;
        }
    }

}
