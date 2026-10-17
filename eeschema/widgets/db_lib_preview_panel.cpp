/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/db_lib_preview_panel.h>

#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>

#include <footprint_library_query.h>
#include <generate_alias_info.h>
#include <ki_exception.h>
#include <kiplatform/ui.h>
#include <lib_symbol.h>
#include <widgets/footprint_preview_widget.h>
#include <widgets/lib_tree.h>
#include <widgets/symbol_preview_widget.h>


/**
 * Tree model over symbols the preview panel owns.  Details come from those symbols rather than a
 * library table, and pinning is disabled because the library is not in the project.
 */
class DB_LIB_PREVIEW_TREE_ADAPTER : public LIB_TREE_MODEL_ADAPTER
{
public:
    DB_LIB_PREVIEW_TREE_ADAPTER( EDA_BASE_FRAME* aFrame, APP_SETTINGS_BASE::LIB_TREE& aSettings,
                                 std::function<const LIB_SYMBOL*( const LIB_ID& )> aLookup ) :
            LIB_TREE_MODEL_ADAPTER( aFrame, wxEmptyString, aSettings ),
            m_lookup( std::move( aLookup ) )
    {
    }

    void SetLibrary( const wxString& aNickname, const std::vector<LIB_TREE_ITEM*>& aItems )
    {
        // The tree control still holds rows for the nodes about to be freed
        ResetTreeView resetGuard( *this );
        KIPLATFORM::UI::CancelPendingScroll( m_widget );

        m_tree.Clear();

        if( !aItems.empty() )
            DoAddLibrary( aNickname, wxEmptyString, aItems, false, false );
    }

    wxString GenerateInfo( const LIB_ID& aLibId, int aUnit ) override
    {
        const LIB_SYMBOL* symbol = m_lookup( aLibId );
        return symbol ? GenerateAliasInfo( symbol, aUnit ) : wxString();
    }

    bool CanPinLibraries() const override { return false; }

protected:
    PROJECT::LIB_TYPE_T getLibType() override { return PROJECT::LIB_TYPE_T::SYMBOL_LIB; }

private:
    std::function<const LIB_SYMBOL*( const LIB_ID& )> m_lookup;
};


DB_LIB_PREVIEW_PANEL::DB_LIB_PREVIEW_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame, KIWAY* aKiway,
                                            EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) :
        wxPanel( aParent ),
        m_footprintPreview( nullptr ),
        m_kiway( aKiway ),
        m_footprintLoadRetry( [this]()
                              {
                                  showFootprint( m_footprintChoice->GetStringSelection() );
                              },
                              250 )
{
    m_adapter = new DB_LIB_PREVIEW_TREE_ADAPTER( aFrame, m_treeSettings,
                                                 [this]( const LIB_ID& aLibId )
                                                 {
                                                     return findSymbol( aLibId );
                                                 } );

    wxBoxSizer*       sizer = new wxBoxSizer( wxVERTICAL );
    wxSplitterWindow* splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                       wxSP_LIVE_UPDATE | wxSP_3D );

    wxPanel*    treePanel = new wxPanel( splitter );
    wxBoxSizer* treeSizer = new wxBoxSizer( wxVERTICAL );

    m_tree = new LIB_TREE( treePanel, wxT( "database_preview" ), m_adapter, LIB_TREE::ALL_WIDGETS );
    treeSizer->Add( m_tree, 1, wxEXPAND, 0 );

    m_status = new wxStaticText( treePanel, wxID_ANY, wxEmptyString );
    treeSizer->Add( m_status, 0, wxEXPAND | wxALL, 5 );

    treePanel->SetSizer( treeSizer );

    wxPanel*    previewPanel = new wxPanel( splitter );
    wxBoxSizer* previewSizer = new wxBoxSizer( wxVERTICAL );

    m_symbolPreview = new SYMBOL_PREVIEW_WIDGET( previewPanel, aKiway, true, aCanvasType );
    previewSizer->Add( m_symbolPreview, 11, wxEXPAND | wxALL, 5 );

    m_footprintChoice = new wxChoice( previewPanel, wxID_ANY );
    m_footprintChoice->SetToolTip( _( "Footprints offered by the selected row" ) );
    previewSizer->Add( m_footprintChoice, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );

    if( aKiway )
    {
        m_footprintPreview = new FOOTPRINT_PREVIEW_WIDGET( previewPanel, *aKiway );
        previewSizer->Add( m_footprintPreview, 10, wxEXPAND | wxALL, 5 );

        // Nothing else may have loaded the footprint libraries, e.g. when opened from the library table
        StartFootprintLibrariesLoad( *aKiway );
    }

    previewPanel->SetSizer( previewSizer );

    splitter->SetSashGravity( 0.5 );
    splitter->SetMinimumPaneSize( 80 );
    splitter->SplitHorizontally( treePanel, previewPanel );

    sizer->Add( splitter, 1, wxEXPAND, 0 );
    SetSizer( sizer );

    m_adapter->FinishTreeInitialization();

    Bind( EVT_LIBITEM_SELECTED, &DB_LIB_PREVIEW_PANEL::onSymbolSelected, this );
    m_footprintChoice->Bind( wxEVT_CHOICE, &DB_LIB_PREVIEW_PANEL::onFootprintSelected, this );

    showSymbol( nullptr, 1 );
}


DB_LIB_PREVIEW_PANEL::~DB_LIB_PREVIEW_PANEL()
{
    Unbind( EVT_LIBITEM_SELECTED, &DB_LIB_PREVIEW_PANEL::onSymbolSelected, this );
    m_footprintChoice->Unbind( wxEVT_CHOICE, &DB_LIB_PREVIEW_PANEL::onFootprintSelected, this );
    m_footprintLoadRetry.Cancel();
}


void DB_LIB_PREVIEW_PANEL::SetSymbols( const wxString& aLibNickname,
                                       std::vector<std::unique_ptr<LIB_SYMBOL>> aSymbols )
{
    int    unit = 0;
    LIB_ID selected = m_tree->GetSelectedLibId( &unit );

    m_tree->Unselect();

    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> symbols;
    std::vector<LIB_TREE_ITEM*>                      items;

    for( std::unique_ptr<LIB_SYMBOL>& symbol : aSymbols )
    {
        LIB_ID id = symbol->GetLibId();
        id.SetLibNickname( aLibNickname );
        id.SetLibItemName( symbol->GetName() );
        symbol->SetLibId( id );

        symbols[symbol->GetName()] = std::move( symbol );
    }

    // Keys that sanitize to the same name collapse to one symbol, so take pointers after the map settles
    for( const auto& [name, symbol] : symbols )
        items.push_back( symbol.get() );

    // The tree selects asynchronously after a rebuild, so carry the selection through its preselect,
    // which matches nodes by nickname and name only
    LIB_ID preselect;

    if( selected.IsValid() && symbols.contains( selected.GetLibItemName().wx_str() ) )
        preselect = LIB_ID( aLibNickname, selected.GetLibItemName() );

    m_adapter->SetPreselectNode( preselect, preselect.IsValid() ? unit : 0 );

    static_cast<DB_LIB_PREVIEW_TREE_ADAPTER*>( m_adapter.get() )->SetLibrary( aLibNickname, items );

    // Tree nodes copied what they need, so the old symbols can go now
    m_symbols = std::move( symbols );
    m_nickname = aLibNickname;

    m_tree->Regenerate( false );

    if( !m_nickname.IsEmpty() )
        m_adapter->OpenLibs( { m_nickname } );
}


void DB_LIB_PREVIEW_PANEL::SetStatus( const wxString& aText )
{
    m_status->SetLabel( aText );

    // The splitter does not lay out its panes again when only their contents change
    m_status->GetParent()->Layout();
    Layout();
}


void DB_LIB_PREVIEW_PANEL::SelectSymbol( const wxString& aSymbolName )
{
    if( !m_symbols.contains( aSymbolName ) )
        return;

    LIB_ID id( m_nickname, aSymbolName );

    m_tree->SelectLibId( id );
    m_tree->CenterLibId( id );
}


const LIB_SYMBOL* DB_LIB_PREVIEW_PANEL::findSymbol( const LIB_ID& aLibId ) const
{
    auto it = m_symbols.find( aLibId.GetLibItemName().wx_str() );
    return it == m_symbols.end() ? nullptr : it->second.get();
}


void DB_LIB_PREVIEW_PANEL::onSymbolSelected( wxCommandEvent& aEvent )
{
    int    unit = 0;
    LIB_ID id = m_tree->GetSelectedLibId( &unit );

    showSymbol( findSymbol( id ), std::max( unit, 1 ) );
}


void DB_LIB_PREVIEW_PANEL::showSymbol( const LIB_SYMBOL* aSymbol, int aUnit )
{
    m_footprintChoice->Clear();

    if( !aSymbol )
    {
        // Clear without the status overlay; a GL canvas hidden since creation lays out wrongly on GTK
        m_symbolPreview->DisplayPart( nullptr, 0 );
        m_footprintChoice->Disable();
        showFootprint( wxEmptyString );
        return;
    }

    // The loader leaves the source unset when the row's symbols column did not resolve
    if( !aSymbol->GetSourceLibId().IsValid() )
    {
        m_symbolPreview->SetStatusText( _( "Symbol not found; check the symbols column" ) );
    }
    else
    {
        try
        {
            m_symbolPreview->DisplayPart( aSymbol->IsDerived() ? aSymbol->Flatten().get() : aSymbol, aUnit );
        }
        catch( const IO_ERROR& )
        {
            // A derived symbol whose parent is gone still shows its own items
            m_symbolPreview->DisplayPart( aSymbol, aUnit );
        }
    }

    // The database loader lists every footprint of the row in the footprint filters
    for( const wxString& footprint : aSymbol->GetFPFilters() )
    {
        if( footprint.Contains( wxS( ":" ) ) && !footprint.Contains( wxS( "*" ) ) && !footprint.Contains( wxS( "?" ) ) )
            m_footprintChoice->Append( footprint );
    }

    m_footprintChoice->Enable( m_footprintChoice->GetCount() > 1 );

    if( m_footprintChoice->GetCount() > 0 )
    {
        m_footprintChoice->SetSelection( 0 );
        showFootprint( m_footprintChoice->GetString( 0 ) );
    }
    else
    {
        showFootprint( wxEmptyString );
    }
}


void DB_LIB_PREVIEW_PANEL::onFootprintSelected( wxCommandEvent& aEvent )
{
    showFootprint( m_footprintChoice->GetStringSelection() );
}


void DB_LIB_PREVIEW_PANEL::showFootprint( const wxString& aFootprint )
{
    if( !m_footprintPreview || !m_footprintPreview->IsInitialized() )
        return;

    LIB_ID id;

    if( aFootprint.IsEmpty() )
    {
        m_footprintPreview->SetStatusText( _( "No footprint specified" ) );
    }
    else if( m_kiway && GetFootprintLibrariesLoadProgress( *m_kiway ) < 1.0f )
    {
        m_footprintPreview->SetStatusText( _( "Loading footprint libraries..." ) );
        m_footprintLoadRetry.Restart();
    }
    else if( id.Parse( aFootprint ) == -1 && id.IsValid() )
    {
        m_footprintPreview->ClearStatus();
        m_footprintPreview->DisplayFootprint( id );
    }
    else
    {
        m_footprintPreview->SetStatusText( _( "Invalid footprint specified" ) );
    }
}
