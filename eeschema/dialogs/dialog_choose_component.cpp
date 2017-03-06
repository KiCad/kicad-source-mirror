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
#include <sch_base_frame.h>
#include <widgets/footprint_preview_panel.h>
#include <widgets/two_column_tree_list.h>
#include <template_fieldnames.h>
#include <generate_alias_info.h>
#include <make_unique.h>

// Tree navigation helpers.
static wxDataViewItem GetPrevItem( const wxDataViewCtrl& ctrl, const wxDataViewItem& item );
static wxDataViewItem GetNextItem( const wxDataViewCtrl& ctrl, const wxDataViewItem& item );
static wxDataViewItem GetPrevSibling( const wxDataViewCtrl& ctrl, const wxDataViewItem& item );
static wxDataViewItem GetNextSibling( const wxDataViewCtrl& ctrl, const wxDataViewItem& item );

DIALOG_CHOOSE_COMPONENT::DIALOG_CHOOSE_COMPONENT( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                                                  CMP_TREE_MODEL_ADAPTER::PTR& aAdapter,
                                                  int aDeMorganConvert )
    : DIALOG_CHOOSE_COMPONENT_BASE( aParent, wxID_ANY, aTitle ), m_adapter( aAdapter ),
      m_footprintPreviewPanel( nullptr )
{
    m_parent = aParent;
    m_deMorganConvert = aDeMorganConvert >= 0 ? aDeMorganConvert : 0;
    m_external_browser_requested = false;
    m_adapter->AttachTo( m_libraryComponentTree );
    m_componentView->SetLayoutDirection( wxLayout_LeftToRight );
    m_dbl_click_timer = std::make_unique<wxTimer>( this );

    // Search box styling. wxSearchBox can handle this, but it's buggy...
    m_searchBox->SetHint( _( "Search" ) );
    #if defined( __WXMAC__ ) || defined( __WINDOWS__ )
    {
        // On Linux, the "Search" hint disappears when the dialog is focused,
        // meaning it's not present initially when the dialog opens. To ensure
        // the box is understood, a search icon is also provided.
        //
        // The icon provided by wx is ugly on macOS and Windows, *plus* these
        // ports display the "Search" hint in the empty field even when the
        // field is focused. Therefore, the icon is not required on these
        // platforms.
        m_searchBoxIcon->Hide();
        m_searchBoxIcon->GetParent()->Layout();
    }
    #endif // __WXMAC__

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
    m_chooseFootprint->GetParent()->Layout();
#endif

    Bind( wxEVT_TIMER, &DIALOG_CHOOSE_COMPONENT::OnCloseTimer, this );
    Bind( wxEVT_CHAR_HOOK, &DIALOG_CHOOSE_COMPONENT::OnSearchBoxCharHook, this );
    Layout();
    Centre();

    // Per warning in component_tree_search_container.h, this must be called
    // near the end of the constructor.
    m_adapter->UpdateSearchString( wxEmptyString );
    updateSelection();
}


DIALOG_CHOOSE_COMPONENT::~DIALOG_CHOOSE_COMPONENT()
{
    //m_search_container->SetTree( NULL );
}

void DIALOG_CHOOSE_COMPONENT::OnInitDialog( wxInitDialogEvent& event )
{
	m_searchBox->SetFocus();
}


LIB_ALIAS* DIALOG_CHOOSE_COMPONENT::GetSelectedAlias( int* aUnit ) const
{
    auto sel = m_libraryComponentTree->GetSelection();

    if( aUnit && m_adapter->GetUnitFor( sel ) )
        *aUnit = m_adapter->GetUnitFor( sel );

    return m_adapter->GetAliasFor( sel );
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxChange( wxCommandEvent& aEvent )
{
    m_adapter->UpdateSearchString( m_searchBox->GetLineText( 0 ) );
    updateSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxEnter( wxCommandEvent& aEvent )
{
    HandleItemSelection();
}


void DIALOG_CHOOSE_COMPONENT::selectIfValid( const wxDataViewItem& aTreeId )
{
    if( aTreeId.IsOk() )
    {
        m_libraryComponentTree->Select( aTreeId );
        m_libraryComponentTree->EnsureVisible( aTreeId );
    }

    updateSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnSearchBoxCharHook( wxKeyEvent& aKeyStroke )
{
    auto const sel = m_libraryComponentTree->GetSelection();

    switch( aKeyStroke.GetKeyCode() )
    {
    case WXK_UP:
        selectIfValid( GetPrevItem( *m_libraryComponentTree, sel ) );
        break;

    case WXK_DOWN:
        selectIfValid( GetNextItem( *m_libraryComponentTree, sel ) );
        break;

    default:
        aKeyStroke.Skip();  // Any other key: pass on to search box directly.
        break;
    }
}


void DIALOG_CHOOSE_COMPONENT::OnTreeSelect( wxDataViewEvent& aEvent )
{
    updateSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnTreeActivate( wxDataViewEvent& aEvent )
{
    updateSelection();
    HandleItemSelection();
}


void DIALOG_CHOOSE_COMPONENT::OnCloseTimer( wxTimerEvent& aEvent )
{
    // Hack handler because of eaten MouseUp event. See
    // DIALOG_CHOOSE_COMPONENT::OnDoubleClickTreeActivation for the beginning
    // of this spaghetti noodle.

    auto state = wxGetMouseState();

    if( state.LeftIsDown() )
    {
        // Mouse hasn't been raised yet, so fire the timer again. Otherwise the
        // purpose of this timer is defeated.
        m_dbl_click_timer->StartOnce( DIALOG_CHOOSE_COMPONENT::DblClickDelay );
    }
    else
    {
        EndModal( wxID_OK );
    }
}


void DIALOG_CHOOSE_COMPONENT::OnStartComponentBrowser( wxMouseEvent& aEvent )
{
    m_external_browser_requested = true;
    EndModal( wxID_OK );
}


bool DIALOG_CHOOSE_COMPONENT::updateSelection()
{
    auto sel = m_libraryComponentTree->GetSelection();
    int unit = m_adapter->GetUnitFor( sel );
    LIB_ALIAS* alias = m_adapter->GetAliasFor( sel );

    m_componentView->Refresh();

    if( alias == NULL )
    {
        if( m_footprintPreviewPanel )
        {
            m_footprintPreviewPanel->SetStatusText( wxEmptyString );
        }

        m_componentDetails->SetPage( wxEmptyString );

        return false;
    }

    m_componentDetails->SetPage( GenerateAliasInfo( alias, unit ) );

    updateFootprint();

    return true;
}


void DIALOG_CHOOSE_COMPONENT::updateFootprint()
{
    if( !m_footprintPreviewPanel )
        return;

    auto sel = m_libraryComponentTree->GetSelection();
    LIB_ALIAS* alias = m_adapter->GetAliasFor( sel );

    if( !alias )
        return;

    LIB_FIELDS fields;
    alias->GetPart()->GetFields( fields );

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
    auto sel = m_libraryComponentTree->GetSelection();

    int unit = m_adapter->GetUnitFor( sel );
    LIB_ALIAS* alias = m_adapter->GetAliasFor( sel );
    LIB_PART*   part = alias ? alias->GetPart() : NULL;

    // Don't draw anything (not even the background) if we don't have
    // a part to show
    if( !part )
        return;

    if( alias->IsRoot() )
    {
        // just show the part directly
        renderPreview( part, unit );
    }
    else
    {
        // switch out the name temporarily for the alias name
        wxString tmp( part->GetName() );
        part->SetName( alias->GetName() );

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


void DIALOG_CHOOSE_COMPONENT::HandleItemSelection()
{
    if( m_adapter->GetAliasFor( m_libraryComponentTree->GetSelection() ) )
    {
        // Got a selection. We can't just end the modal dialog here, because
        // wx leaks some events back to the parent window (in particular, the
        // MouseUp following a double click).
        //
        // NOW, here's where it gets really fun. wxTreeListCtrl eats MouseUp.
        // This isn't really feasible to bypass without a fully custom
        // wxDataViewCtrl implementation, and even then might not be fully
        // possible (docs are vague). To get around this, we use a one-shot
        // timer to schedule the dialog close.
        //
        // See DIALOG_CHOOSE_COMPONENT::OnCloseTimer for the other end of this
        // spaghetti noodle.
        m_dbl_click_timer->StartOnce( DIALOG_CHOOSE_COMPONENT::DblClickDelay );
    }
    else
    {
        auto const sel = m_libraryComponentTree->GetSelection();

        if( m_libraryComponentTree->IsExpanded( sel ) )
            m_libraryComponentTree->Collapse( sel );
        else
            m_libraryComponentTree->Expand( sel );
    }
}


static wxDataViewItem GetPrevItem( const wxDataViewCtrl& tree, const wxDataViewItem& item )
{
    auto prevItem = GetPrevSibling( tree, item );

    if( !prevItem.IsOk() )
    {
        prevItem = tree.GetModel()->GetParent( item );
    }
    else if( tree.IsExpanded( prevItem ) )
    {
        wxDataViewItemArray children;
        tree.GetModel()->GetChildren( prevItem, children );
        prevItem = children[children.size() - 1];
    }

    return prevItem;
}


static wxDataViewItem GetNextItem( const wxDataViewCtrl& tree, const wxDataViewItem& item )
{
    wxDataViewItem nextItem;

    if( !item.IsOk() )
    {
        // No selection. Select the first.
        wxDataViewItemArray children;
        tree.GetModel()->GetChildren( item, children );
        return children[0];
    }

    if( tree.IsExpanded( item ) )
    {
        wxDataViewItemArray children;
        tree.GetModel()->GetChildren( item, children );
        nextItem = children[0];
    }
    else
    {
        // Walk up levels until we find one that has a next sibling.
        for ( wxDataViewItem walk = item; walk.IsOk(); walk = tree.GetModel()->GetParent( walk ) )
        {
            nextItem = GetNextSibling( tree, walk );

            if( nextItem.IsOk() )
                break;
        }
    }

    return nextItem;
}


static wxDataViewItem GetPrevSibling( const wxDataViewCtrl& tree, const wxDataViewItem& item )
{
    wxDataViewItemArray siblings;
    wxDataViewItem invalid;
    wxDataViewItem parent = tree.GetModel()->GetParent( item );

    tree.GetModel()->GetChildren( parent, siblings );

    for( size_t i = 0; i < siblings.size(); ++i )
    {
        if( siblings[i] == item )
        {
            if( i == 0 )
                return invalid;
            else
                return siblings[i - 1];
        }
    }

    return invalid;
}


static wxDataViewItem GetNextSibling( const wxDataViewCtrl& tree, const wxDataViewItem& item )
{
    wxDataViewItemArray siblings;
    wxDataViewItem invalid;
    wxDataViewItem parent = tree.GetModel()->GetParent( item );

    tree.GetModel()->GetChildren( parent, siblings );

    for( size_t i = 0; i < siblings.size(); ++i )
    {
        if( siblings[i] == item )
        {
            if( i == siblings.size() - 1 )
                return invalid;
            else
                return siblings[i + 1];
        }
    }

    return invalid;
}
