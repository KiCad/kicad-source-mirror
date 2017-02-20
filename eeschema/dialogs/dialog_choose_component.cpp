/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/tokenzr.h>
#include <wx/utils.h>

#include <class_library.h>
#include <component_tree_search_container.h>
#include <sch_base_frame.h>
#include <widgets/footprint_preview_panel.h>
#include <widgets/two_column_tree_list.h>
#include <template_fieldnames.h>
#include <generate_alias_info.h>

// Tree navigation helpers.
static wxTreeListItem GetPrevItem( const wxTreeListCtrl& tree, const wxTreeListItem& item );
static wxTreeListItem GetNextItem( const wxTreeListCtrl& tree, const wxTreeListItem& item );
static wxTreeListItem GetPrevSibling( const wxTreeListCtrl& tree, const wxTreeListItem& item );

DIALOG_CHOOSE_COMPONENT::DIALOG_CHOOSE_COMPONENT( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                                                  COMPONENT_TREE_SEARCH_CONTAINER* const aContainer,
                                                  int aDeMorganConvert )
    : DIALOG_CHOOSE_COMPONENT_BASE( aParent, wxID_ANY, aTitle ), m_search_container( aContainer ),
      m_footprintPreviewPanel( nullptr )
{
    m_parent = aParent;
    m_deMorganConvert = aDeMorganConvert >= 0 ? aDeMorganConvert : 0;
    m_external_browser_requested = false;
    m_received_doubleclick_in_tree = false;
    m_search_container->SetTree( m_libraryComponentTree );
    m_componentView->SetLayoutDirection( wxLayout_LeftToRight );

    // Initialize footprint preview through Kiway
    m_footprintPreviewPanel =
        FOOTPRINT_PREVIEW_PANEL::InstallOnPanel( Kiway(), m_footprintView, true );

    if( m_footprintPreviewPanel )
    {
        // This hides the GAL panel and shows the status label
        m_footprintPreviewPanel->SetStatusText( wxEmptyString );
    }

#ifndef KICAD_FOOTPRINT_SELECTOR
    // Footprint chooser isn't implemented yet or isn't selected, don't show it.
    m_chooseFootprint->Hide();
#endif

    Layout();
    Centre();

    // Per warning in component_tree_search_container.h, this must be called
    // near the end of the constructor.
    m_search_container->UpdateSearchTerm( wxEmptyString );
    updateSelection();
}


DIALOG_CHOOSE_COMPONENT::~DIALOG_CHOOSE_COMPONENT()
{
    m_search_container->SetTree( NULL );
}

void DIALOG_CHOOSE_COMPONENT::OnInitDialog( wxInitDialogEvent& event )
{
	m_searchBox->SetFocus();
}


LIB_ALIAS* DIALOG_CHOOSE_COMPONENT::GetSelectedAlias( int* aUnit ) const
{
    return m_search_container->GetSelectedAlias( aUnit );
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxChange( wxCommandEvent& aEvent )
{
    m_search_container->UpdateSearchTerm( m_searchBox->GetLineText( 0 ) );
    updateSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxEnter( wxCommandEvent& aEvent )
{
    EndModal( wxID_OK );   // We are done.
}


void DIALOG_CHOOSE_COMPONENT::selectIfValid( const wxTreeListItem& aTreeId )
{
    if( aTreeId.IsOk() && aTreeId != m_libraryComponentTree->GetRootItem() )
        m_libraryComponentTree->Select( aTreeId );

    updateSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnInterceptSearchBoxKey( wxKeyEvent& aKeyStroke )
{
    // Cursor up/down and partiallyi cursor are use to do tree navigation operations.
    // This is done by intercepting some navigational keystrokes that normally would go to
    // the text search box (which has the focus by default). That way, we are mostly keyboard
    // operable.
    // (If the tree has the focus, it can handle that by itself).
    const wxTreeListItem sel = m_libraryComponentTree->GetSelection();

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


void DIALOG_CHOOSE_COMPONENT::OnTreeSelect( wxTreeListEvent& aEvent )
{
    updateSelection();
}


// Test strategy for OnDoubleClickTreeActivation()/OnTreeMouseUp() work around wxWidgets bug:
//  - search for an item.
//  - use the mouse to double-click on an item in the tree.
//  -> The dialog should close, and the component should _not_ be immediately placed
void DIALOG_CHOOSE_COMPONENT::OnDoubleClickTreeActivation( wxTreeListEvent& aEvent )
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

    if( selection == NULL )
    {
        if( m_footprintPreviewPanel )
        {
            m_footprintPreviewPanel->SetStatusText( wxEmptyString );
        }

        m_componentDetails->SetPage( wxEmptyString );

        return false;
    }

    m_componentDetails->SetPage( GenerateAliasInfo( selection, unit ) );

    updateFootprint();

    return true;
}


void DIALOG_CHOOSE_COMPONENT::updateFootprint()
{
    if( !m_footprintPreviewPanel )
        return;

    int dummy_unit = 0;
    LIB_ALIAS* selection = m_search_container->GetSelectedAlias( &dummy_unit );

    if( !selection )
        return;

    LIB_FIELDS fields;
    selection->GetPart()->GetFields( fields );

    for( auto const & field: fields )
    {
        if( field.GetId() != FOOTPRINT )
            continue;
        wxString fpname = field.GetFullText();
        if( fpname == wxEmptyString )
        {
            m_footprintPreviewPanel->SetStatusText( _( "No footprint specified" ) );
        }
        else
        {
            m_footprintPreviewPanel->ClearStatus();
            m_footprintPreviewPanel->CacheFootprint( LIB_ID( fpname ) );
            m_footprintPreviewPanel->DisplayFootprint( LIB_ID( fpname ) );
        }
        break;
    }
}


void DIALOG_CHOOSE_COMPONENT::OnDatasheetClick( wxHtmlLinkEvent& aEvent )
{
    const wxHtmlLinkInfo & info = aEvent.GetLinkInfo();
    ::wxLaunchDefaultBrowser( info.GetHref() );
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

    const wxSize dc_size = dc.GetSize();

    // Avoid rendering when either dimension is zero
    if( dc_size.x == 0 || dc_size.y == 0 )
        return;

    GRResetPenAndBrush( &dc );

    COLOR4D bgColor = m_parent->GetDrawBgColor();

    dc.SetBackground( wxBrush( bgColor.ToColour() ) );
    dc.Clear();

    if( aComponent == NULL )
        return;

    if( aUnit <= 0 )
        aUnit = 1;

    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Find joint bounding box for everything we are about to draw.
    EDA_RECT bBox = aComponent->GetUnitBoundingBox( aUnit, m_deMorganConvert );
    const double xscale = (double) dc_size.x / bBox.GetWidth();
    const double yscale = (double) dc_size.y / bBox.GetHeight();
    const double scale  = std::min( xscale, yscale ) * 0.85;

    dc.SetUserScale( scale, scale );

    wxPoint offset = -bBox.Centre();

    auto opts = PART_DRAW_OPTIONS::Default();
    opts.draw_hidden_fields = false;
    aComponent->Draw( NULL, &dc, offset, aUnit, m_deMorganConvert, opts );
}


static wxTreeListItem GetPrevItem( const wxTreeListCtrl& tree, const wxTreeListItem& item )
{
    wxTreeListItem prevItem = GetPrevSibling( tree, item );

    if( !prevItem.IsOk() )
    {
        prevItem = tree.GetItemParent( item );
    }
    else if( tree.IsExpanded( prevItem ) )
    {
        // wxTreeListCtrl has no .GetLastChild. Simulate it
        prevItem = tree.GetFirstChild( prevItem );
        wxTreeListItem next;
        do
        {
            next = tree.GetNextSibling( prevItem );
            if( next.IsOk() )
            {
                prevItem = next;
            }
        } while( next.IsOk() );
    }

    return prevItem;
}


static wxTreeListItem GetNextItem( const wxTreeListCtrl& tree, const wxTreeListItem& item )
{
    wxTreeListItem nextItem;

    if( !item.IsOk() )
        return nextItem;    // item is not valid: return a not valid wxTreeListItem

    if( tree.IsExpanded( item ) )
    {
        nextItem = tree.GetFirstChild( item );
    }
    else
    {
        wxTreeListItem root_cell=  tree.GetRootItem();

        // Walk up levels until we find one that has a next sibling.
        for ( wxTreeListItem walk = item; walk.IsOk(); walk = tree.GetItemParent( walk ) )
        {
            if( walk == root_cell )     // the root cell (not displayed) is reached
                break;                  // Exit (calling GetNextSibling( root_cell ) crashes.

            nextItem = tree.GetNextSibling( walk );

            if( nextItem.IsOk() )
                break;
        }
    }

    return nextItem;
}


static wxTreeListItem GetPrevSibling( const wxTreeListCtrl& tree, const wxTreeListItem& item )
{
    // Why wxTreeListCtrl has no GetPrevSibling when it does have GetNextSibling
    // is beyond me. wxTreeCtrl has this.

    wxTreeListItem last_item;
    wxTreeListItem parent = tree.GetItemParent( item );

    if( !parent.IsOk() )
        return last_item; // invalid signifies not found

    last_item = tree.GetFirstChild( parent );
    while( last_item.IsOk() )
    {
        wxTreeListItem next_item = tree.GetNextSibling( last_item );
        if( next_item == item )
        {
            return last_item;
        }
        else
        {
            last_item = next_item;
        }
    }

    return last_item;
}
