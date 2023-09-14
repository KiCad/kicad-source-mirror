/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <symbol_diff_frame.h>
#include <sch_base_frame.h>

#include <wx/event.h>

BEGIN_EVENT_TABLE( SYMBOL_DIFF_FRAME, SCH_BASE_FRAME )
    // Window events
    EVT_SIZE( SYMBOL_DIFF_FRAME::OnSize )
    EVT_ACTIVATE( SYMBOL_DIFF_FRAME::OnActivate )

    // Toolbar events
    // EVT_TOOL( ID_LIBVIEW_SELECT_PART, SYMBOL_DIFF_FRAME::OnSelectSymbol )
    // EVT_TOOL( ID_LIBVIEW_NEXT, SYMBOL_DIFF_FRAME::onSelectNextSymbol )
    // EVT_TOOL( ID_LIBVIEW_PREVIOUS, SYMBOL_DIFF_FRAME::onSelectPreviousSymbol )
    // EVT_CHOICE( ID_LIBVIEW_SELECT_UNIT_NUMBER, SYMBOL_DIFF_FRAME::onSelectSymbolUnit )

    // listbox events
    // EVT_TEXT( ID_LIBVIEW_LIB_FILTER, SYMBOL_DIFF_FRAME::OnLibFilter )
    // EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, SYMBOL_DIFF_FRAME::ClickOnLibList )
    // EVT_TEXT( ID_LIBVIEW_SYM_FILTER, SYMBOL_DIFF_FRAME::OnSymFilter )
    // EVT_LISTBOX( ID_LIBVIEW_SYM_LIST, SYMBOL_DIFF_FRAME::ClickOnSymbolList )
    // EVT_LISTBOX_DCLICK( ID_LIBVIEW_SYM_LIST, SYMBOL_DIFF_FRAME::DClickOnSymbolList )

    // Menu (and/or hotkey) events
    EVT_MENU( wxID_CLOSE, SYMBOL_DIFF_FRAME::CloseLibraryViewer )

    EVT_UPDATE_UI( ID_LIBVIEW_SELECT_UNIT_NUMBER, SYMBOL_DIFF_FRAME::onUpdateUnitChoice )

END_EVENT_TABLE()

#define LIB_VIEW_STYLE ( KICAD_DEFAULT_DRAWFRAME_STYLE )
#define LIB_VIEW_STYLE_MODAL ( KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT )

SYMBOL_DIFF_FRAME::SYMBOL_DIFF_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                          const wxString& aLibraryName ) :
    SCH_BASE_FRAME( aKiway, aParent, aFrameType, _( "Symbol Library Browser" ),
                    wxDefaultPosition, wxDefaultSize, wxS( "SymbolDiff" ) )
{
    SetModal( true );

    m_aboutTitle = _HKI( "KiCad Symbol Difference Viewer" );

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::library_browser ) );
    SetIcon( icon );


    SetScreen( new SCH_SCREEN );
    GetScreen()->m_Center = true;      // Axis origin centered on screen.
    LoadSettings( config() );

    // Ensure axis are always drawn (initial default display was not drawn)
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = GetGalDisplayOptions();
    gal_opts.m_axesEnabled = true;
    gal_opts.m_gridMinSpacing = 10.0;
    gal_opts.NotifyChanged();

    GetRenderSettings()->LoadColors( GetColorSettings() );
    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );

    GetRenderSettings()->SetDefaultPenWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS );

    setupTools();
    setupUIConditions();

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateMenuBar();

    wxPanel* libPanel = new wxPanel( this );
    wxSizer* libSizer = new wxBoxSizer( wxVERTICAL );


    wxPanel* symbolPanel = new wxPanel( this );
    wxSizer* symbolSizer = new wxBoxSizer( wxVERTICAL );

    m_symbolFilter = new wxSearchCtrl( symbolPanel, ID_LIBVIEW_SYM_FILTER, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_symbolFilter->SetDescriptiveText( _( "Filter" ) );
    m_symbolFilter->SetToolTip(
            _( "Filter on symbol name, keywords, description and pin count.\n"
               "Search terms are separated by spaces.  All search terms must match.\n"
               "A term which is a number will also match against the pin count." ) );
    symbolSizer->Add( m_symbolFilter, 0, wxEXPAND, 5 );

#ifdef __WXGTK__
    // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
    // See https://gitlab.com/kicad/code/kicad/-/issues/9019
    m_libFilter->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
    m_symbolFilter->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#endif

    m_symbolList = new WX_LISTBOX( symbolPanel, ID_LIBVIEW_SYM_LIST, wxDefaultPosition, wxDefaultSize,
                                   0, nullptr, wxLB_HSCROLL | wxNO_BORDER );
    symbolSizer->Add( m_symbolList, 1, wxEXPAND, 5 );

    symbolPanel->SetSizer( symbolSizer );
    symbolPanel->Fit();

    // Preload libraries
    loadAllLibraries();

    if( aLibraryName.empty() )
    {
        ReCreateLibList();
    }
    else
    {
        m_currentSymbol.SetLibNickname( aLibraryName );
        m_currentSymbol.SetLibItemName( "" );
        m_unit = 1;
        m_convert = 1;
    }

    m_selection_changed = false;

    DisplayLibInfos();

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Manage main toolbar
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ) .Bottom().Layer(6) );

    m_auimgr.AddPane( libPanel, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(2)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 200, -1 ) );
    m_auimgr.AddPane( symbolPanel, EDA_PANE().Palette().Name( "Symbols" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 300, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    m_auimgr.GetPane( libPanel ).Show( aLibraryName.empty() );

    m_auimgr.Update();

    if( m_libListWidth > 0 )
        SetAuiPaneSize( m_auimgr, m_auimgr.GetPane( "Libraries" ), m_libListWidth, -1 );

    if( m_symbolListWidth > 0 )
        SetAuiPaneSize( m_auimgr, m_auimgr.GetPane( "Symbols" ), m_symbolListWidth, -1 );

    FinishAUIInitialization();

    if( !IsModal() )        // For modal mode, calling ShowModal() will show this frame
    {
        Raise();
        Show( true );
    }

    SyncView();
    GetCanvas()->SetCanFocus( false );

    setupUnits( config() );

    // Set the working/draw area size to display a symbol to a reasonable value:
    // A 450mm x 450mm with a origin at the area center looks like a large working area
    double max_size_x = schIUScale.mmToIU( 450 );
    double max_size_y = schIUScale.mmToIU( 450 );
    BOX2D bbox;
    bbox.SetOrigin( -max_size_x / 2, -max_size_y / 2 );
    bbox.SetSize( max_size_x, max_size_y );
    GetCanvas()->GetView()->SetBoundary( bbox );
    GetToolManager()->RunAction( ACTIONS::zoomFitScreen );

    // If a symbol was previously selected in m_symbolList from a previous run, show it
    wxString symbName = m_symbolList->GetStringSelection();

    if( !symbName.IsEmpty() )
    {
        SetSelectedSymbol( symbName );
        updatePreviewSymbol();
    }
}


SYMBOL_DIFF_FRAME::~SYMBOL_DIFF_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    if( m_previewItem )
    {
        GetCanvas()->GetView()->Remove( m_previewItem.get() );
        m_previewItem = nullptr;
    }
}