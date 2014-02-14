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

// Combine descriptions of all aliases from given component.
static wxString combineDescriptions( LIB_COMPONENT* aComponent )
{
    std::set<wxString> descriptions;

    for( size_t i = 0; i < aComponent->GetAliasCount(); ++i )
    {
        LIB_ALIAS* a = aComponent->GetAlias( i );

        if ( !a->GetDescription().empty() )
            descriptions.insert( a->GetDescription() );
    }

    wxString result;
    BOOST_FOREACH( const wxString& s, descriptions )
        result += s + wxT("\n");
    return result;
}


// Combine keywords. Keywords come as a string, but are considered space-separated
// individual words. Return a string with a unique set of these.
static wxString combineKeywords( LIB_COMPONENT* aComponent )
{
    std::set<wxString> keywords;

    for( size_t i = 0; i < aComponent->GetAliasCount(); ++i )
    {
        LIB_ALIAS* a = aComponent->GetAlias( i );
        wxStringTokenizer tokenizer( a->GetKeyWords() );

        while ( tokenizer.HasMoreTokens() )
            keywords.insert( tokenizer.GetNextToken() );
    }

    wxString result;
    BOOST_FOREACH( const wxString& s, keywords )
        result += s + wxT(" ");
    return result;
}


DIALOG_CHOOSE_COMPONENT::DIALOG_CHOOSE_COMPONENT( wxWindow* aParent, const wxString& aTitle,
                                                  COMPONENT_TREE_SEARCH_CONTAINER* aContainer )
    : DIALOG_CHOOSE_COMPONENT_BASE( aParent, wxID_ANY, aTitle ),
      m_search_container( aContainer ),
      m_selected_component( NULL ),
      m_external_browser_requested( false ),
      m_received_doubleclick_in_tree( false )
{
    // TODO: restore last size user was choosing.
    m_search_container->SetTree( m_libraryComponentTree );
    m_searchBox->SetFocus();
    m_componentDetails->SetEditable( false );
}


// After this dialog is done: return the component that has been selected, or an
// empty string if there is none.
wxString DIALOG_CHOOSE_COMPONENT::GetSelectedComponentName() const
{
    if ( m_selected_component == NULL )
        return wxEmptyString;

    return m_selected_component->GetName();
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxChange( wxCommandEvent& aEvent )
{
    m_selected_component = NULL;
    m_search_container->UpdateSearchTerm( m_searchBox->GetLineText(0) );
    updateSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxEnter( wxCommandEvent& aEvent )
{
    EndModal( wxID_OK );   // We are done.
}


void DIALOG_CHOOSE_COMPONENT::SelectIfValid( const wxTreeItemId& aTreeId )
{
    if ( aTreeId.IsOk() && aTreeId != m_libraryComponentTree->GetRootItem() )
        m_libraryComponentTree->SelectItem( aTreeId );
}


void DIALOG_CHOOSE_COMPONENT::OnInterceptSearchBoxKey( wxKeyEvent& aKeyStroke )
{
    // Cursor up/down are forwarded to the tree. This is done by intercepting some navigational
    // keystrokes that normally would go to the text search box (which has the focus by default).
    const wxTreeItemId sel = m_libraryComponentTree->GetSelection();

    switch ( aKeyStroke.GetKeyCode() )
    {
    case WXK_UP:
        SelectIfValid( GetPrevItem( *m_libraryComponentTree, sel ) );
        break;

    case WXK_DOWN:
        SelectIfValid( GetNextItem( *m_libraryComponentTree, sel ) );
        break;

    default:
        aKeyStroke.Skip();  // Pass on to search box.
        break;
    }
}


void DIALOG_CHOOSE_COMPONENT::OnTreeSelect( wxTreeEvent& aEvent )
{
    updateSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnDoubleClickTreeSelect( wxTreeEvent& aEvent )
{
    updateSelection();

    if ( m_selected_component == NULL )
        return;

    // Ok, got selection. We don't just end the modal dialog here, but
    // wait for the MouseUp event to occur. Otherwise something (broken?)
    // happens: the dialog will close and will deliver the 'MouseUp' event
    // to the eeschema canvas, that will immediately place the component.
    m_received_doubleclick_in_tree = true;
}


void DIALOG_CHOOSE_COMPONENT::OnTreeMouseUp( wxMouseEvent& aMouseEvent )
{
    if ( m_received_doubleclick_in_tree )
        EndModal( wxID_OK );     // We are done (see OnDoubleClickTreeSelect)
    else
        aMouseEvent.Skip();      // Let upstream handle it.
}


void DIALOG_CHOOSE_COMPONENT::OnStartComponentBrowser( wxMouseEvent& aEvent )
{
    m_external_browser_requested = true;
    EndModal( wxID_OK );   // We are done.
}


void DIALOG_CHOOSE_COMPONENT::updateSelection()
{
    LIB_COMPONENT* selection = m_search_container->GetSelectedComponent();

    if ( selection == m_selected_component )
        return;   // no change.

    m_selected_component = selection;

    m_componentDetails->Clear();

    if ( m_selected_component == NULL )
        return;

    wxFont font_normal = m_componentDetails->GetFont();
    wxFont font_bold = m_componentDetails->GetFont();
    font_bold.SetWeight( wxFONTWEIGHT_BOLD );

    wxTextAttr headline_attribute;
    headline_attribute.SetFont(font_bold);
    wxTextAttr text_attribute;
    text_attribute.SetFont(font_normal);

    const wxString description = combineDescriptions( selection );

    if ( !description.empty() )
    {
        m_componentDetails->SetDefaultStyle( headline_attribute );
        m_componentDetails->AppendText( _("Description\n") );
        m_componentDetails->SetDefaultStyle( text_attribute );
        m_componentDetails->AppendText( description );
        m_componentDetails->AppendText( wxT("\n") );
    }

    const wxString keywords = combineKeywords( selection );

    if ( !keywords.empty() )
    {
        m_componentDetails->SetDefaultStyle( headline_attribute );
        m_componentDetails->AppendText( _("Keywords\n") );
        m_componentDetails->SetDefaultStyle( text_attribute );
        m_componentDetails->AppendText( keywords );
    }

    m_componentDetails->SetInsertionPoint( 0 );  // scroll up.
}

static wxTreeItemId GetPrevItem( const wxTreeCtrl& tree, const wxTreeItemId& item )
{
    wxTreeItemId prevItem = tree.GetPrevSibling( item );

    if ( !prevItem.IsOk() )
    {
        const wxTreeItemId parent = tree.GetItemParent( item );
        prevItem = tree.GetLastChild( tree.GetPrevSibling( parent ) );
    }

    return prevItem;
}

static wxTreeItemId GetNextItem( const wxTreeCtrl& tree, const wxTreeItemId& item )
{
    wxTreeItemId nextItem = tree.GetNextSibling( item );

    if ( !nextItem.IsOk() )
    {
        const wxTreeItemId parent = tree.GetItemParent( item );
        wxTreeItemIdValue dummy;
        nextItem = tree.GetFirstChild( tree.GetNextSibling( parent ), dummy );
    }

    return nextItem;
}
