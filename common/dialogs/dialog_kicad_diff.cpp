/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_kicad_diff.h"

#include <diff_merge/diff_tree_grouping.h>
#include <widgets/widget_diff_canvas.h>

#include <layer_ids.h>
#include <lseq.h>
#include <set>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/srchctrl.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/wupdlock.h>


DIALOG_KICAD_DIFF::DIALOG_KICAD_DIFF( wxWindow* aParent, const wxString& aReferencePath,
                                      const wxString& aComparisonPath, const KICAD_DIFF::DOCUMENT_DIFF& aDiff,
                                      KICAD_DIFF::DOCUMENT_GEOMETRY aReferenceGeometry,
                                      KICAD_DIFF::DOCUMENT_GEOMETRY aComparisonGeometry, SHEET_SWITCHER aSheetSwitcher,
                                      KIID_PATH aInitialSheet ) :
        DIALOG_KICAD_DIFF_BASE( aParent ),
        m_diff( aDiff ),
        m_sheetSwitcher( std::move( aSheetSwitcher ) ),
        m_currentCanvasSheet( std::move( aInitialSheet ) )
{
    SetEscapeId( wxID_OK );

    m_treeChanges->Bind( wxEVT_TREE_ITEM_MENU, &DIALOG_KICAD_DIFF::OnTreeItemMenu, this );

    m_pathReference->SetLabel( aReferencePath );
    m_pathComparison->SetLabel( aComparisonPath );

    // Two property columns: name | before -> after
    m_listProperties->AppendColumn( _( "Property" ), wxLIST_FORMAT_LEFT, 180 );
    m_listProperties->AppendColumn( _( "Before" ), wxLIST_FORMAT_LEFT, 160 );
    m_listProperties->AppendColumn( _( "After" ), wxLIST_FORMAT_LEFT, 160 );

    // Splice a search box above the change tree. Empty filter = show all.
    if( wxSizer* treeSizer = m_panelTree->GetSizer() )
    {
        wxSearchCtrl* search = new wxSearchCtrl( m_panelTree, wxID_ANY );
        search->SetDescriptiveText( _( "Filter changes…" ) );
        search->ShowCancelButton( true );

        auto onSearch = [this, search]( wxCommandEvent& aEvent )
        {
            // BuildChangeTreeGroups lowercases internally; keep the
            // raw input here so a future surface displaying the
            // active filter (e.g. tooltip) preserves capitalization.
            m_searchFilter = search->GetValue();
            buildTree();
            aEvent.Skip();
        };

        // wxSearchCtrl's cancel button fires its own event on some ports
        // (notably macOS); binding both keeps the filter cleared whichever
        // path the user takes to empty the box.
        search->Bind( wxEVT_TEXT, onSearch );
        search->Bind( wxEVT_SEARCHCTRL_CANCEL_BTN, onSearch );

        treeSizer->Insert( 0, search, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, 4 ) );
        m_panelTree->Layout();
    }

    // Splice the thumbnail into the detail sizer between the summary label
    // and the property list. Only construct when we can attach.
    if( wxSizer* detailSizer = m_panelDetail->GetSizer() )
    {
        // Board on top, property list below, with a draggable sash. Gravity 1
        // gives extra height to the board when the dialog grows.
        wxSplitterWindow* detailSplitter =
                new wxSplitterWindow( m_panelDetail, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE );
        detailSplitter->SetSashGravity( 1.0 );
        detailSplitter->SetMinimumPaneSize( 40 );

        m_canvas = new WIDGET_DIFF_CANVAS( detailSplitter );

        if( m_sheetSwitcher )
            m_sheetSwitcher( *m_canvas, m_currentCanvasSheet );

        KICAD_DIFF::DIFF_COLOR_THEME theme;
        KICAD_DIFF::DIFF_SCENE       scene = KICAD_DIFF::BuildScene( m_diff, theme );

        scene.referenceGeometry = std::move( aReferenceGeometry );
        scene.comparisonGeometry = std::move( aComparisonGeometry );
        KICAD_DIFF::ExpandBBoxToGeometry( scene );

        const LSET geometryLayers = KICAD_DIFF::GeometryLayerSet( scene.referenceGeometry )
                                    | KICAD_DIFF::GeometryLayerSet( scene.comparisonGeometry );

        wxBoxSizer* filterSizer = new wxBoxSizer( wxHORIZONTAL );

        const std::array<std::pair<KICAD_DIFF::CATEGORY, wxString>, 4> categories{ {
                { KICAD_DIFF::CATEGORY::ADDED, _( "Added" ) },
                { KICAD_DIFF::CATEGORY::REMOVED, _( "Removed" ) },
                { KICAD_DIFF::CATEGORY::MODIFIED, _( "Modified" ) },
                { KICAD_DIFF::CATEGORY::CONFLICT, _( "Conflict" ) },
        } };

        for( const auto& [cat, label] : categories )
        {
            wxCheckBox* cb = new wxCheckBox( m_panelDetail, wxID_ANY, label );
            cb->SetValue( true );
            cb->Bind( wxEVT_CHECKBOX,
                      [this, cat]( wxCommandEvent& aEvent )
                      {
                          m_canvas->SetCategoryVisible( cat, aEvent.IsChecked() );
                          buildTree();
                      } );
            filterSizer->Add( cb, wxSizerFlags().Border( wxRIGHT, 8 ).Centre() );
        }

        if( geometryLayers.any() )
        {
            filterSizer->Add( new wxStaticText( m_panelDetail, wxID_ANY, _( "Layers:" ) ),
                              wxSizerFlags().Border( wxLEFT | wxRIGHT, 4 ).Centre() );

            for( PCB_LAYER_ID layer : geometryLayers.UIOrder() )
            {
                wxCheckBox* cb = new wxCheckBox( m_panelDetail, wxID_ANY, LayerName( layer ) );
                cb->SetValue( true );
                cb->Bind( wxEVT_CHECKBOX,
                          [this, layer]( wxCommandEvent& aEvent )
                          {
                              m_canvas->SetLayerVisible( layer, aEvent.IsChecked() );
                          } );
                filterSizer->Add( cb, wxSizerFlags().Border( wxRIGHT, 8 ).Centre() );
            }
        }

        // Toggle the property list so the board can use the full detail panel.
        filterSizer->AddStretchSpacer();

        wxCheckBox* propsToggle = new wxCheckBox( m_panelDetail, wxID_ANY, _( "Properties" ) );
        propsToggle->SetValue( true );
        propsToggle->Bind( wxEVT_CHECKBOX,
                           [this, detailSplitter]( wxCommandEvent& aEvent )
                           {
                               if( aEvent.IsChecked() && !detailSplitter->IsSplit() )
                                   detailSplitter->SplitHorizontally( m_canvas, m_listProperties );
                               else if( !aEvent.IsChecked() && detailSplitter->IsSplit() )
                                   detailSplitter->Unsplit( m_listProperties );
                           } );
        filterSizer->Add( propsToggle, wxSizerFlags().Border( wxRIGHT, 8 ).Centre() );

        // Move the property list out of the detail sizer and under the splitter
        // so the sash can resize it. A small min height lets it shrink freely.
        detailSizer->Detach( m_listProperties );
        m_listProperties->Reparent( detailSplitter );
        m_listProperties->SetMinSize( wxSize( -1, 40 ) );
        detailSplitter->SplitHorizontally( m_canvas, m_listProperties, -200 );

        propsToggle->SetValue( detailSplitter->IsSplit() );

        size_t insertAt = std::min<size_t>( 1, detailSizer->GetItemCount() );
        detailSizer->Insert( insertAt++, filterSizer, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, 4 ) );
        detailSizer->Insert( insertAt, detailSplitter, wxSizerFlags( 1 ).Expand().Border( wxALL, 4 ) );
        m_panelDetail->Layout();

        m_canvas->SetScene( std::move( scene ) );

        m_canvas->SetPickHandler(
                [this]( const std::optional<KIID_PATH>& aChangeId )
                {
                    if( aChangeId )
                    {
                        m_suppressCenter = true;
                        selectChangeById( *aChangeId );
                        m_suppressCenter = false;
                    }
                    else if( m_treeChanges )
                    {
                        m_treeChanges->UnselectAll();
                        showChange( nullptr );
                    }
                } );
    }

    buildTree();

    if( m_diff.Empty() )
        m_labelSummary->SetLabel( _( "No differences detected." ) );
    else
        m_labelSummary->SetLabel( wxString::Format( _( "%zu change(s) detected." ), m_diff.Size() ) );

    Layout();
}


void DIALOG_KICAD_DIFF::OnClose( wxCloseEvent& aEvent )
{
    EndModal( wxID_OK );
}


void DIALOG_KICAD_DIFF::OnOK( wxCommandEvent& aEvent )
{
    EndModal( wxID_OK );
}


void DIALOG_KICAD_DIFF::OnTreeSelectionChanged( wxTreeEvent& aEvent )
{
    wxTreeItemId sel = aEvent.GetItem();

    if( !sel.IsOk() )
    {
        showChange( nullptr );
        return;
    }

    auto it = m_changeByTreeId.find( reinterpret_cast<wxUIntPtr>( sel.GetID() ) );

    if( it == m_changeByTreeId.end() )
    {
        showChange( nullptr ); // group node — no per-item details
        return;
    }

    showChange( it->second );
}


void DIALOG_KICAD_DIFF::buildTree()
{
    // Snapshot the selected change so it can be restored after rebuild —
    // wxTreeItemIds don't survive DeleteAllItems, so we re-find the node
    // by its underlying KIID_PATH below.
    std::optional<KIID_PATH> selectedId;

    if( wxTreeItemId sel = m_treeChanges->GetSelection(); sel.IsOk() )
    {
        auto it = m_changeByTreeId.find( reinterpret_cast<wxUIntPtr>( sel.GetID() ) );

        if( it != m_changeByTreeId.end() && it->second )
            selectedId = it->second->id;
    }

    wxWindowUpdateLocker updateLock( m_treeChanges );

    m_treeChanges->DeleteAllItems();
    m_changeByTreeId.clear();

    wxTreeItemId root = m_treeChanges->AddRoot( wxS( "root" ) );

    // Per-category visibility comes from the canvas's checkbox row when
    // present (the dialog hosts both a thumbnail canvas and the tree).
    // With no canvas (degenerate test path) treat every category as visible.
    std::array<bool, KICAD_DIFF::CATEGORY_COUNT> visibleCats{};
    visibleCats.fill( true );

    if( m_canvas )
    {
        for( std::size_t i = 0; i < KICAD_DIFF::CATEGORY_COUNT; ++i )
        {
            visibleCats[i] = m_canvas->IsCategoryVisible( static_cast<KICAD_DIFF::CATEGORY>( i ) );
        }
    }

    for( const KICAD_DIFF::CHANGE_TREE_GROUP& group :
         KICAD_DIFF::BuildChangeTreeGroups( m_diff, m_searchFilter, visibleCats ) )
    {
        wxTreeItemId groupNode = m_treeChanges->AppendItem( root, group.groupLabel );

        for( const KICAD_DIFF::CHANGE_TREE_ENTRY& entry : group.entries )
        {
            wxTreeItemId itemNode = m_treeChanges->AppendItem( groupNode, entry.itemLabel );
            m_changeByTreeId[reinterpret_cast<wxUIntPtr>( itemNode.GetID() )] = entry.change;
        }

        m_treeChanges->Expand( groupNode );
    }

    if( selectedId && !selectChangeById( *selectedId ) )
        showChange( nullptr );

    applyHiddenToTree();
}


void DIALOG_KICAD_DIFF::OnTreeItemMenu( wxTreeEvent& aEvent )
{
    wxTreeItemId item = aEvent.GetItem();

    if( !item.IsOk() )
        return;

    auto it = m_changeByTreeId.find( reinterpret_cast<wxUIntPtr>( item.GetID() ) );

    if( it == m_changeByTreeId.end() || !it->second )
        return; // group node, nothing to hide

    const std::vector<KIID_PATH> ids = changeRowIds( *it->second );
    const bool                   hidden = m_hiddenChanges.count( it->second->id ) > 0;

    enum
    {
        ID_HIDE = wxID_HIGHEST + 1,
        ID_SHOW,
        ID_SHOW_ALL
    };

    wxMenu menu;

    if( hidden )
        menu.Append( ID_SHOW, _( "Show Change" ) );
    else
        menu.Append( ID_HIDE, _( "Hide Change" ) );

    menu.Append( ID_SHOW_ALL, _( "Show All Hidden" ) );
    menu.Enable( ID_SHOW_ALL, !m_hiddenChanges.empty() );

    switch( m_treeChanges->GetPopupMenuSelectionFromUser( menu ) )
    {
    case ID_HIDE: m_hiddenChanges.insert( ids.begin(), ids.end() ); break;
    case ID_SHOW:
        for( const KIID_PATH& id : ids )
            m_hiddenChanges.erase( id );
        break;
    case ID_SHOW_ALL: m_hiddenChanges.clear(); break;
    default: return;
    }

    if( m_canvas )
        m_canvas->SetHiddenChanges( m_hiddenChanges );

    applyHiddenToTree();
}


std::vector<KIID_PATH> DIALOG_KICAD_DIFF::changeRowIds( const KICAD_DIFF::ITEM_CHANGE& aChange ) const
{
    if( !KICAD_DIFF::IsRoutingNetChange( aChange ) )
        return { aChange.id };

    std::vector<KIID_PATH> ids;

    std::function<void( const std::vector<KICAD_DIFF::ITEM_CHANGE>& )> walk =
            [&]( const std::vector<KICAD_DIFF::ITEM_CHANGE>& aChanges )
            {
                for( const KICAD_DIFF::ITEM_CHANGE& c : aChanges )
                {
                    if( KICAD_DIFF::IsRoutingNetChange( c ) && c.kind == aChange.kind && c.refdes == aChange.refdes )
                    {
                        ids.push_back( c.id );
                    }

                    walk( c.children );
                }
            };

    walk( m_diff.changes );
    return ids;
}


void DIALOG_KICAD_DIFF::applyHiddenToTree()
{
    const wxColour grey = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
    const wxColour normal = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    for( const auto& [key, change] : m_changeByTreeId )
    {
        if( !change )
            continue;

        wxTreeItemId node( reinterpret_cast<void*>( key ) );
        const bool   hidden = m_hiddenChanges.count( change->id ) > 0;

        m_treeChanges->SetItemTextColour( node, hidden ? grey : normal );
    }
}


void DIALOG_KICAD_DIFF::SetRevisionChooser( const std::vector<wxString>& aLabels, int aSelected,
                                            REVISION_HANDLER aOnChange )
{
    wxSizer* top = GetSizer();

    if( !top || aLabels.empty() )
        return;

    m_revisionHandler = std::move( aOnChange );

    wxBoxSizer* row = new wxBoxSizer( wxHORIZONTAL );
    row->Add( new wxStaticText( this, wxID_ANY, _( "Revision:" ) ), wxSizerFlags().Border( wxRIGHT, 4 ).Centre() );

    wxArrayString choices;

    for( const wxString& label : aLabels )
        choices.Add( label );

    m_revisionChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices );
    m_revisionChoice->SetSelection( aSelected );
    row->Add( m_revisionChoice, wxSizerFlags( 1 ).Centre() );

    wxButton* olderBtn = new wxButton( this, wxID_ANY, _( "Older" ), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    wxButton* newerBtn = new wxButton( this, wxID_ANY, _( "Newer" ), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    row->Add( olderBtn, wxSizerFlags().Border( wxLEFT, 4 ).Centre() );
    row->Add( newerBtn, wxSizerFlags().Border( wxLEFT, 2 ).Centre() );

    auto fire = [this]( int aIndex )
    {
        if( aIndex < 0 || aIndex >= static_cast<int>( m_revisionChoice->GetCount() ) )
            return;

        m_revisionChoice->SetSelection( aIndex );

        if( m_revisionHandler )
            m_revisionHandler( aIndex );
    };

    m_revisionChoice->Bind( wxEVT_CHOICE,
                            [fire]( wxCommandEvent& aEvent )
                            {
                                fire( aEvent.GetSelection() );
                            } );

    // Snapshots are newest-first, so older steps up the index and newer down.
    olderBtn->Bind( wxEVT_BUTTON,
                    [this, fire]( wxCommandEvent& )
                    {
                        fire( m_revisionChoice->GetSelection() + 1 );
                    } );
    newerBtn->Bind( wxEVT_BUTTON,
                    [this, fire]( wxCommandEvent& )
                    {
                        fire( m_revisionChoice->GetSelection() - 1 );
                    } );

    top->Insert( 0, row, wxSizerFlags().Expand().Border( wxALL, 4 ) );
    Layout();
}


void DIALOG_KICAD_DIFF::Reload( const wxString& aReferencePath, const wxString& aComparisonPath,
                                KICAD_DIFF::DOCUMENT_DIFF aDiff, KICAD_DIFF::DOCUMENT_GEOMETRY aReferenceGeometry,
                                KICAD_DIFF::DOCUMENT_GEOMETRY aComparisonGeometry, SHEET_SWITCHER aSheetSwitcher,
                                KIID_PATH aInitialSheet )
{
    m_pathReference->SetLabel( aReferencePath );
    m_pathComparison->SetLabel( aComparisonPath );

    m_diff = std::move( aDiff );

    // Outlives the canvas reconfigure below: it owns cloned items the view still references.
    SHEET_SWITCHER previousSwitcher = std::move( m_sheetSwitcher );
    m_sheetSwitcher = std::move( aSheetSwitcher );
    m_currentCanvasSheet = std::move( aInitialSheet );

    if( m_canvas )
    {
        // Hold rendering so reconfiguring the context and setting the new scene
        // produce a single repaint instead of flashing an intermediate frame.
        // EndUpdate must run even on a throw, or rendering stays frozen.
        m_canvas->BeginUpdate();

        try
        {
            m_canvas->HighlightChange( std::nullopt );

            if( m_sheetSwitcher )
                m_sheetSwitcher( *m_canvas, m_currentCanvasSheet );

            KICAD_DIFF::DIFF_COLOR_THEME theme;
            KICAD_DIFF::DIFF_SCENE       scene = KICAD_DIFF::BuildScene( m_diff, theme );

            scene.referenceGeometry = std::move( aReferenceGeometry );
            scene.comparisonGeometry = std::move( aComparisonGeometry );
            KICAD_DIFF::ExpandBBoxToGeometry( scene );

            m_canvas->SetScene( std::move( scene ) );
        }
        catch( ... )
        {
            m_canvas->EndUpdate();
            throw;
        }

        m_canvas->EndUpdate();
    }

    buildTree();

    if( m_diff.Empty() )
        m_labelSummary->SetLabel( _( "No differences detected." ) );
    else
        m_labelSummary->SetLabel( wxString::Format( _( "%zu change(s) detected." ), m_diff.Size() ) );

    showChange( nullptr );
    Layout();
}


void DIALOG_KICAD_DIFF::SwitchCanvasToSheet( const KIID_PATH& aSheetPath )
{
    if( !m_canvas || !m_sheetSwitcher )
        return;

    if( aSheetPath == m_currentCanvasSheet )
        return;

    m_currentCanvasSheet = aSheetPath;
    m_sheetSwitcher( *m_canvas, aSheetPath );

    if( m_treeChanges )
        m_treeChanges->UnselectAll();

    showChange( nullptr );
}


void DIALOG_KICAD_DIFF::showChange( const KICAD_DIFF::ITEM_CHANGE* aChange )
{
    m_listProperties->DeleteAllItems();

    if( m_changeSelectedFn )
        m_changeSelectedFn( aChange ? aChange->id : KIID_PATH() );

    if( !aChange )
    {
        m_labelSummary->SetLabel( _( "Select a change in the tree to view details" ) );

        if( m_canvas )
            m_canvas->HighlightChange( std::nullopt );

        return;
    }

    m_labelSummary->SetLabel(
            wxString::Format( wxS( "%s — %s" ), KICAD_DIFF::ChangeKindLabel( aChange->kind ), aChange->typeName ) );

    if( m_canvas )
    {
        // Only auto-switch when the change carries a sheet prefix. Scoped
        // diffs emit single-element ids with no sheet path.
        if( aChange->id.size() > 1 )
        {
            KIID_PATH sheetPath = aChange->id;
            sheetPath.pop_back();

            if( sheetPath != m_currentCanvasSheet && m_sheetSwitcher )
            {
                m_currentCanvasSheet = sheetPath;
                m_sheetSwitcher( *m_canvas, sheetPath );
            }
        }

        m_canvas->HighlightChange( aChange->id );

        if( !m_suppressCenter )
            m_canvas->CenterOnHighlight();
    }

    long               row = 0;
    std::set<wxString> seen;

    auto emitDelta = [&]( const wxString& aPrefix, const KICAD_DIFF::PROPERTY_DELTA& d )
    {
        const wxString label = aPrefix.IsEmpty() ? d.name : aPrefix + wxS( " / " ) + d.name;
        const wxString beforeStr = d.before.ToDisplayString();
        const wxString afterStr = d.after.ToDisplayString();
        const wxString key = label + wxS( "|" ) + beforeStr + wxS( "|" ) + afterStr;

        if( !seen.insert( key ).second )
            return;

        long idx = m_listProperties->InsertItem( row++, label );
        m_listProperties->SetItem( idx, 1, beforeStr );
        m_listProperties->SetItem( idx, 2, afterStr );
    };

    for( const KICAD_DIFF::PROPERTY_DELTA& d : aChange->properties )
        emitDelta( wxEmptyString, d );

    // When the clicked item carries its real diffs in children (e.g. a
    // FOOTPRINT whose pad or field changed), surface those here so the panel
    // is not empty for the parent.
    std::function<void( const wxString&, const KICAD_DIFF::ITEM_CHANGE& )> walkChildren =
            [&]( const wxString& aPrefix, const KICAD_DIFF::ITEM_CHANGE& aC )
            {
                for( const KICAD_DIFF::ITEM_CHANGE& child : aC.children )
                {
                    wxString prefix = child.typeName;

                    if( child.refdes && !child.refdes->IsEmpty() )
                        prefix += wxS( " " ) + *child.refdes;

                    if( !aPrefix.IsEmpty() )
                        prefix = aPrefix + wxS( " / " ) + prefix;

                    for( const KICAD_DIFF::PROPERTY_DELTA& d : child.properties )
                        emitDelta( prefix, d );

                    walkChildren( prefix, child );
                }
            };

    walkChildren( wxEmptyString, *aChange );
}


bool DIALOG_KICAD_DIFF::selectChangeById( const KIID_PATH& aChangeId )
{
    for( const auto& [treeIdNum, change] : m_changeByTreeId )
    {
        if( change && change->id == aChangeId )
        {
            wxTreeItemId treeId( reinterpret_cast<void*>( treeIdNum ) );

            m_treeChanges->EnsureVisible( treeId );
            m_treeChanges->SelectItem( treeId );
            return true;
        }
    }

    return false;
}
