/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <dialog_choose_component.h>

#include <set>
#include <boost/foreach.hpp>
#include <wx/tokenzr.h>

#include <class_library.h>
#include <component_tree_search_container.h>
#include <sch_base_frame.h>

// Tree navigation helpers.
static wxTreeItemId GetPrevItem( const wxTreeCtrl& tree, const wxTreeItemId& item );
static wxTreeItemId GetNextItem( const wxTreeCtrl& tree, const wxTreeItemId& item );

DIALOG_CHOOSE_COMPONENT::DIALOG_CHOOSE_COMPONENT( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                                                  COMPONENT_TREE_SEARCH_CONTAINER* aContainer,
                                                  int aDeMorganConvert )
    : DIALOG_CHOOSE_COMPONENT_BASE( aParent, wxID_ANY, aTitle ),
      m_search_container( aContainer ),
      m_deMorganConvert( aDeMorganConvert >= 0 ? aDeMorganConvert : 0 ),
      m_external_browser_requested( false ),
      m_received_doubleclick_in_tree( false )
{
    m_parent = aParent;
    m_search_container->SetTree( m_libraryComponentTree );
    m_searchBox->SetFocus();
    m_componentDetails->SetEditable( false );

#if wxCHECK_VERSION( 3, 0, 0 )
    m_libraryComponentTree->ScrollTo( m_libraryComponentTree->GetFocusedItem() );
#endif

    // The tree showing libs and component uses a fixed font,
    // because we want controle the position of some info when drawing the
    // tree. Using tabs does not work very well (does not work on Windows)
    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    m_libraryComponentTree->SetFont( wxFont( font.GetPointSize(),
             wxFONTFAMILY_MODERN,  wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ) );
}


DIALOG_CHOOSE_COMPONENT::~DIALOG_CHOOSE_COMPONENT()
{
    m_search_container->SetTree( NULL );
}


LIB_ALIAS* DIALOG_CHOOSE_COMPONENT::GetSelectedAlias( int* aUnit ) const
{
    return m_search_container->GetSelectedAlias( aUnit );
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxChange( wxCommandEvent& aEvent )
{
    m_search_container->UpdateSearchTerm( m_searchBox->GetLineText( 0 ) );
    updateSelection();
    m_searchBox->SetFocus();
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxEnter( wxCommandEvent& aEvent )
{
    EndModal( wxID_OK );   // We are done.
}


void DIALOG_CHOOSE_COMPONENT::selectIfValid( const wxTreeItemId& aTreeId )
{
    if( aTreeId.IsOk() && aTreeId != m_libraryComponentTree->GetRootItem() )
        m_libraryComponentTree->SelectItem( aTreeId );
}


void DIALOG_CHOOSE_COMPONENT::OnInterceptSearchBoxKey( wxKeyEvent& aKeyStroke )
{
    // Cursor up/down and partiallyi cursor are use to do tree navigation operations.
    // This is done by intercepting some navigational keystrokes that normally would go to
    // the text search box (which has the focus by default). That way, we are mostly keyboard
    // operable.
    // (If the tree has the focus, it can handle that by itself).
    const wxTreeItemId sel = m_libraryComponentTree->GetSelection();

    switch( aKeyStroke.GetKeyCode() )
    {
    case WXK_UP:
        selectIfValid( GetPrevItem( *m_libraryComponentTree, sel ) );
        break;

    case WXK_DOWN:
        selectIfValid( GetNextItem( *m_libraryComponentTree, sel ) );
        break;

        // The following keys we can only hijack if they are not needed by the textbox itself.

    case WXK_LEFT:
        if( m_searchBox->GetInsertionPoint() == 0 )
            m_libraryComponentTree->Collapse( sel );
        else
            aKeyStroke.Skip();   // Use for original purpose: move cursor.
        break;

    case WXK_RIGHT:
        if( m_searchBox->GetInsertionPoint() >= (long) m_searchBox->GetLineText( 0 ).length() )
            m_libraryComponentTree->Expand( sel );
        else
            aKeyStroke.Skip();   // Use for original purpose: move cursor.
        break;

    default:
        aKeyStroke.Skip();  // Any other key: pass on to search box directly.
        break;
    }
}


void DIALOG_CHOOSE_COMPONENT::OnTreeSelect( wxTreeEvent& aEvent )
{
    updateSelection();
}


// Test strategy for OnDoubleClickTreeActivation()/OnTreeMouseUp() work around wxWidgets bug:
//  - search for an item.
//  - use the mouse to double-click on an item in the tree.
//  -> The dialog should close, and the component should _not_ be immediately placed
void DIALOG_CHOOSE_COMPONENT::OnDoubleClickTreeActivation( wxTreeEvent& aEvent )
{
    if( !updateSelection() )
        return;

    // Ok, got selection. We don't just end the modal dialog here, but
    // wait for the MouseUp event to occur. Otherwise something (broken?)
    // happens: the dialog will close and will deliver the 'MouseUp' event
    // to the eeschema canvas, that will immediately place the component.
    m_received_doubleclick_in_tree = true;
}


void DIALOG_CHOOSE_COMPONENT::OnTreeMouseUp( wxMouseEvent& aMouseEvent )
{
    if( m_received_doubleclick_in_tree )
        EndModal( wxID_OK );     // We are done (see OnDoubleClickTreeSelect)
    else
        aMouseEvent.Skip();      // Let upstream handle it.
}

// Test strategy to see if OnInterceptTreeEnter() works:
//  - search for an item.
//  - click into the tree once to set focus on tree; navigate. Press 'Enter'
//  -> The dialog should close and the component be available to place.
void DIALOG_CHOOSE_COMPONENT::OnInterceptTreeEnter( wxKeyEvent& aEvent )
{
    // We have to do some special handling for double-click on a tree-item because
    // of some superfluous event delivery bug in wxWidgets (see OnDoubleClickTreeActivation()).
    // In tree-activation, we assume we got a double-click and need to take special precaution
    // that the mouse-up event is not delivered to the window one level up by going through
    // a state-sequence OnDoubleClickTreeActivation() -> OnTreeMouseUp().

    // Pressing 'Enter' within a tree will also call OnDoubleClickTreeActivation(),
    // but since this is not due to the double-click and we have no way of knowing that it is
    // not, we need to intercept the 'Enter' key before that to know that it is time to exit.
    if( aEvent.GetKeyCode() == WXK_RETURN )
        EndModal( wxID_OK );    // Dialog is done.
    else
        aEvent.Skip();          // Let tree handle that key for navigation.
}


void DIALOG_CHOOSE_COMPONENT::OnStartComponentBrowser( wxMouseEvent& aEvent )
{
    m_external_browser_requested = true;
    EndModal( wxID_OK );   // We are done.
}


bool DIALOG_CHOOSE_COMPONENT::updateSelection()
{
    int unit = 0;
    LIB_ALIAS* selection = m_search_container->GetSelectedAlias( &unit );

    m_componentView->Refresh();

    m_componentDetails->Clear();

    if( selection == NULL )
        return false;

    m_componentDetails->Freeze();
    wxFont font_normal = m_componentDetails->GetFont();
    wxFont font_bold = m_componentDetails->GetFont();
    font_bold.SetWeight( wxFONTWEIGHT_BOLD );

    wxTextAttr headline_attribute;
    headline_attribute.SetFont( font_bold );
    wxTextAttr text_attribute;
    text_attribute.SetFont( font_normal );

    const wxString name = selection->GetName();

    if ( !name.empty() )
    {
        m_componentDetails->SetDefaultStyle( headline_attribute );
        m_componentDetails->AppendText( name );
    }

    const wxString description = selection->GetDescription();

    if( !description.empty() )
    {
        if ( !m_componentDetails->IsEmpty() )
            m_componentDetails->AppendText( wxT( "\n\n" ) );

        m_componentDetails->SetDefaultStyle( headline_attribute );
        m_componentDetails->AppendText( _( "Description\n" ) );
        m_componentDetails->SetDefaultStyle( text_attribute );
        m_componentDetails->AppendText( description );
    }

    const wxString keywords = selection->GetKeyWords();

    if( !keywords.empty() )
    {
        if ( !m_componentDetails->IsEmpty() )
            m_componentDetails->AppendText( wxT( "\n\n" ) );

        m_componentDetails->SetDefaultStyle( headline_attribute );
        m_componentDetails->AppendText( _( "Keywords\n" ) );
        m_componentDetails->SetDefaultStyle( text_attribute );
        m_componentDetails->AppendText( keywords );
    }

    if ( !selection->IsRoot() )
    {
        LIB_PART* root_part = selection->GetPart();
        const wxString root_component_name( root_part ? root_part->GetName() : _( "Unknown" ) );

        if ( !m_componentDetails->IsEmpty() )
            m_componentDetails->AppendText( wxT( "\n\n" ) );

        m_componentDetails->SetDefaultStyle( headline_attribute );
        m_componentDetails->AppendText( _( "Alias of " ) );
        m_componentDetails->SetDefaultStyle( text_attribute );
        m_componentDetails->AppendText( root_component_name );
    }

    m_componentDetails->SetInsertionPoint( 0 );  // scroll up.
    m_componentDetails->Thaw();

    return true;
}


void DIALOG_CHOOSE_COMPONENT::OnHandlePreviewRepaint( wxPaintEvent& aRepaintEvent )
{
    int unit = 0;
    LIB_ALIAS*  selection = m_search_container->GetSelectedAlias( &unit );
    LIB_PART*   part = selection ? selection->GetPart() : NULL;

    // Don't draw anything (not even the background) if we don't have
    // a part to show
    if( !part )
        return;

    if( selection->IsRoot() )
    {
        // just show the part directly
        renderPreview( part, unit );
    }
    else
    {
        // switch out the name temporarily for the alias name
        wxString tmp( part->GetName() );
        part->SetName( selection->GetName() );

        renderPreview( part, unit );

        part->SetName( tmp );
    }
}


// Render the preview in our m_componentView. If this gets more complicated, we should
// probably have a derived class from wxPanel; but this keeps things local.
void DIALOG_CHOOSE_COMPONENT::renderPreview( LIB_PART* aComponent, int aUnit )
{
    wxPaintDC dc( m_componentView );
    EDA_COLOR_T bgcolor = m_parent->GetDrawBgColor();

    dc.SetBackground( bgcolor == BLACK ? *wxBLACK_BRUSH : *wxWHITE_BRUSH );
    dc.Clear();

    if( aComponent == NULL )
        return;

    if( aUnit <= 0 )
        aUnit = 1;

    const wxSize dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Find joint bounding box for everything we are about to draw.
    EDA_RECT bBox = aComponent->GetBoundingBox( aUnit, m_deMorganConvert );
    const double xscale = (double) dc_size.x / bBox.GetWidth();
    const double yscale = (double) dc_size.y / bBox.GetHeight();
    const double scale  = std::min( xscale, yscale ) * 0.85;

    dc.SetUserScale( scale, scale );

    wxPoint offset =  bBox.Centre();
    NEGATE( offset.x );
    NEGATE( offset.y );

    aComponent->Draw( NULL, &dc, offset, aUnit, m_deMorganConvert, GR_COPY,
                      UNSPECIFIED_COLOR, DefaultTransform, true, true, false );
}


static wxTreeItemId GetPrevItem( const wxTreeCtrl& tree, const wxTreeItemId& item )
{
    wxTreeItemId prevItem = tree.GetPrevSibling( item );

    if( !prevItem.IsOk() )
    {
        prevItem = tree.GetItemParent( item );
    }
    else if( tree.IsExpanded( prevItem ) )
    {
        prevItem = tree.GetLastChild( prevItem );
    }

    return prevItem;
}


static wxTreeItemId GetNextItem( const wxTreeCtrl& tree, const wxTreeItemId& item )
{
    wxTreeItemId nextItem;

    if( tree.IsExpanded( item ) )
    {
        wxTreeItemIdValue dummy;
        nextItem = tree.GetFirstChild( item, dummy );
    }
    else
    {
        // Walk up levels until we find one that has a next sibling.
        for ( wxTreeItemId walk = item; walk.IsOk(); walk = tree.GetItemParent( walk ) )
        {
            nextItem = tree.GetNextSibling( walk );

            if( nextItem.IsOk() )
                break;
        }
    }

    return nextItem;
}
