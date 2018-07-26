/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2016-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <set>
#include <wx/utils.h>

#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include <class_library.h>
#include <sch_base_frame.h>
#include <template_fieldnames.h>
#include <symbol_lib_table.h>
#include <widgets/component_tree.h>
#include <widgets/footprint_preview_widget.h>
#include <widgets/footprint_select_widget.h>


wxSize DIALOG_CHOOSE_COMPONENT::m_last_dlg_size( -1, -1 );
int DIALOG_CHOOSE_COMPONENT::m_tree_canvas_sash_position = 0;

std::mutex DIALOG_CHOOSE_COMPONENT::g_Mutex;


DIALOG_CHOOSE_COMPONENT::DIALOG_CHOOSE_COMPONENT( SCH_BASE_FRAME* aParent, const wxString& aTitle,
        CMP_TREE_MODEL_ADAPTER::PTR& aAdapter, int aDeMorganConvert, bool aAllowFieldEdits,
        bool aShowFootprints )
        : DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, wxDefaultSize,
                  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
          m_fp_sel_ctrl( nullptr ),
          m_fp_view_ctrl( nullptr ),
          m_parent( aParent ),
          m_deMorganConvert( aDeMorganConvert >= 0 ? aDeMorganConvert : 0 ),
          m_allow_field_edits( aAllowFieldEdits ),
          m_show_footprints( aShowFootprints ),
          m_external_browser_requested( false )
{
    auto sizer = new wxBoxSizer( wxVERTICAL );

    // Use a slightly different layout, with a details pane spanning the entire window,
    // if we're not showing footprints.
    auto vsplitter = aShowFootprints ? nullptr : new wxSplitterWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH );

    m_splitter_tree_canvas = new wxSplitterWindow(
        vsplitter ? static_cast<wxWindow *>( vsplitter ) : static_cast<wxWindow *>( this ),
        wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH );

    //Avoid the splitter window being assigned as the Parent to additional windows
    m_splitter_tree_canvas->SetExtraStyle( wxWS_EX_TRANSIENT );

    auto details = aShowFootprints ? nullptr : new wxHtmlWindow(
        vsplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );

    m_tree = new COMPONENT_TREE( m_splitter_tree_canvas, Prj().SchSymbolLibTable(),
                                 aAdapter, COMPONENT_TREE::WIDGETS::ALL, details );
    m_symbol_view_panel = ConstructRightPanel( m_splitter_tree_canvas );
    auto buttons = new wxStdDialogButtonSizer();
    m_dbl_click_timer = new wxTimer( this );

    if( vsplitter )
        sizer->Add( vsplitter, 1, wxEXPAND | wxALL, 5 );
    else
        sizer->Add( m_splitter_tree_canvas, 1, wxEXPAND | wxALL, 5 );

    buttons->AddButton( new wxButton( this, wxID_OK ) );
    buttons->AddButton( new wxButton( this, wxID_CANCEL ) );
    buttons->Realize();

    sizer->Add( buttons, 0, wxEXPAND | wxBOTTOM, 10 );
    SetSizer( sizer );

    Bind( wxEVT_INIT_DIALOG, &DIALOG_CHOOSE_COMPONENT::OnInitDialog, this );
    Bind( wxEVT_ACTIVATE, &DIALOG_CHOOSE_COMPONENT::OnActivate, this );
    Bind( wxEVT_TIMER, &DIALOG_CHOOSE_COMPONENT::OnCloseTimer, this, m_dbl_click_timer->GetId() );
    Bind( COMPONENT_PRESELECTED, &DIALOG_CHOOSE_COMPONENT::OnComponentPreselected, this );
    Bind( COMPONENT_SELECTED, &DIALOG_CHOOSE_COMPONENT::OnComponentSelected, this );

    m_sch_view_ctrl->Bind( wxEVT_LEFT_DCLICK, &DIALOG_CHOOSE_COMPONENT::OnSchViewDClick, this );
    m_sch_view_ctrl->Bind( wxEVT_PAINT, &DIALOG_CHOOSE_COMPONENT::OnSchViewPaint, this );

    if( m_fp_sel_ctrl )
        m_fp_sel_ctrl->Bind( EVT_FOOTPRINT_SELECTED, &DIALOG_CHOOSE_COMPONENT::OnFootprintSelected, this );

    Layout();

    if( m_last_dlg_size == wxSize( -1, -1 ) )
        SetSizeInDU( 320, 256 );
    else
        SetSize( m_last_dlg_size );

    m_splitter_tree_canvas->SetSashGravity( 0.8 );
    m_splitter_tree_canvas->SetMinimumPaneSize( 20 );
    // We specify the width of the right window (m_symbol_view_panel), because specify
    // the width of the left window does not work as expected when SetSashGravity() is called
    m_splitter_tree_canvas->SplitVertically( m_tree,  m_symbol_view_panel,
                               m_tree_canvas_sash_position ? -m_tree_canvas_sash_position
                                                           : HorizPixelsFromDU( -100 ) );

    if( vsplitter )
    {
        vsplitter->SetSashGravity( 0.5 );
        vsplitter->SetMinimumPaneSize( 20 );
        vsplitter->SplitHorizontally( m_splitter_tree_canvas, details, VertPixelsFromDU( -80 ) );
    }
}


DIALOG_CHOOSE_COMPONENT::~DIALOG_CHOOSE_COMPONENT()
{
    Unbind( wxEVT_INIT_DIALOG, &DIALOG_CHOOSE_COMPONENT::OnInitDialog, this );
    Unbind( wxEVT_ACTIVATE, &DIALOG_CHOOSE_COMPONENT::OnActivate, this );
    Unbind( wxEVT_TIMER, &DIALOG_CHOOSE_COMPONENT::OnCloseTimer, this );
    Unbind( COMPONENT_PRESELECTED, &DIALOG_CHOOSE_COMPONENT::OnComponentPreselected, this );
    Unbind( COMPONENT_SELECTED, &DIALOG_CHOOSE_COMPONENT::OnComponentSelected, this );

    m_sch_view_ctrl->Unbind( wxEVT_LEFT_DCLICK, &DIALOG_CHOOSE_COMPONENT::OnSchViewDClick, this );
    m_sch_view_ctrl->Unbind( wxEVT_PAINT, &DIALOG_CHOOSE_COMPONENT::OnSchViewPaint, this );

    if( m_fp_sel_ctrl )
        m_fp_sel_ctrl->Unbind( EVT_FOOTPRINT_SELECTED, &DIALOG_CHOOSE_COMPONENT::OnFootprintSelected, this );

    // I am not sure the following two lines are necessary,
    // but they will not hurt anyone
    m_dbl_click_timer->Stop();
    delete m_dbl_click_timer;

    m_last_dlg_size = GetSize();
    m_tree_canvas_sash_position = m_splitter_tree_canvas->GetClientSize().x
                                  - m_splitter_tree_canvas->GetSashPosition();
}


wxPanel* DIALOG_CHOOSE_COMPONENT::ConstructRightPanel( wxWindow* aParent )
{
    auto panel = new wxPanel( aParent );
    auto sizer = new wxBoxSizer( wxVERTICAL );

    m_sch_view_ctrl = new wxPanel( panel, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ),
            wxFULL_REPAINT_ON_RESIZE | wxTAB_TRAVERSAL );
    m_sch_view_ctrl->SetLayoutDirection( wxLayout_LeftToRight );

    if( m_show_footprints )
    {
        FOOTPRINT_LIST* fp_list = FOOTPRINT_LIST::GetInstance( Kiway() );

        if( m_allow_field_edits )
            m_fp_sel_ctrl = new FOOTPRINT_SELECT_WIDGET( panel, fp_list, true );

        m_fp_view_ctrl = new FOOTPRINT_PREVIEW_WIDGET( panel, Kiway() );


        sizer->Add( m_sch_view_ctrl, 1, wxEXPAND | wxALL, 5 );

        if( m_fp_sel_ctrl )
            sizer->Add( m_fp_sel_ctrl, 0, wxEXPAND | wxALL, 5 );

        sizer->Add( m_fp_view_ctrl, 1, wxEXPAND | wxALL, 5 );
    }
    else
    {
        sizer->Add( m_sch_view_ctrl, 1, wxEXPAND | wxALL, 5 );
    }

    panel->SetSizer( sizer );
    panel->Layout();
    sizer->Fit( panel );

    return panel;
}


void DIALOG_CHOOSE_COMPONENT::OnInitDialog( wxInitDialogEvent& aEvent )
{
    if( m_fp_view_ctrl && m_fp_view_ctrl->IsInitialized() )
    {
        // This hides the GAL panel and shows the status label
        m_fp_view_ctrl->SetStatusText( wxEmptyString );
    }

    if( m_fp_sel_ctrl )
        m_fp_sel_ctrl->Load( Kiway(), Prj() );
}


void DIALOG_CHOOSE_COMPONENT::OnActivate( wxActivateEvent& event )
{
    m_tree->SetFocus();

    event.Skip();    // required under wxMAC
}


LIB_ID DIALOG_CHOOSE_COMPONENT::GetSelectedLibId( int* aUnit ) const
{
    return m_tree->GetSelectedLibId( aUnit );
}


void DIALOG_CHOOSE_COMPONENT::OnCloseTimer( wxTimerEvent& aEvent )
{
    // Hack handler because of eaten MouseUp event. See
    // DIALOG_CHOOSE_COMPONENT::OnComponentSelected for the beginning
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
        EndQuasiModal( wxID_OK );
    }
}


void DIALOG_CHOOSE_COMPONENT::OnSchViewDClick( wxMouseEvent& aEvent )
{
    m_external_browser_requested = true;
    EndQuasiModal( wxID_OK );
}


void DIALOG_CHOOSE_COMPONENT::ShowFootprintFor( LIB_ID const& aLibId )
{
    if( !m_fp_view_ctrl || !m_fp_view_ctrl->IsInitialized() )
        return;

    LIB_ALIAS* alias = nullptr;

    try
    {
        alias = Prj().SchSymbolLibTable()->LoadSymbol( aLibId );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( wxString::Format( _( "Error occurred loading symbol %s from library %s."
                                         "\n\n%s" ),
                                      aLibId.GetLibItemName().wx_str(),
                                      aLibId.GetLibNickname().wx_str(),
                                      ioe.What() ) );
    }

    if( alias == nullptr )
    {
        return;
    }

    LIB_FIELD* fp_field = alias->GetPart()->GetField( FOOTPRINT );
    wxString   fp_name = fp_field ? fp_field->GetFullText() : wxString( "" );

    ShowFootprint( fp_name );
}


void DIALOG_CHOOSE_COMPONENT::ShowFootprint( wxString const& aName )
{
    if( !m_fp_view_ctrl || !m_fp_view_ctrl->IsInitialized() )
    {
        return;
    }

    if( aName == wxEmptyString )
    {
        m_fp_view_ctrl->SetStatusText( _( "No footprint specified" ) );
    }
    else
    {
        LIB_ID lib_id;

        if( lib_id.Parse( aName, LIB_ID::ID_PCB ) == -1 && lib_id.IsValid() )
        {
            m_fp_view_ctrl->ClearStatus();
            m_fp_view_ctrl->CacheFootprint( lib_id );
            m_fp_view_ctrl->DisplayFootprint( lib_id );
        }
        else
        {
            m_fp_view_ctrl->SetStatusText( _( "Invalid footprint specified" ) );
        }
    }
}


void DIALOG_CHOOSE_COMPONENT::PopulateFootprintSelector( LIB_ID const& aLibId )
{
    if( !m_fp_sel_ctrl )
        return;

    m_fp_sel_ctrl->ClearFilters();

    LIB_ALIAS* alias = nullptr;

    if( aLibId.IsValid() )
    {
        try
        {
            alias = Prj().SchSymbolLibTable()->LoadSymbol( aLibId );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( wxString::Format( _( "Error occurred loading symbol %s from library %s."
                                             "\n\n%s" ),
                                          aLibId.GetLibItemName().wx_str(),
                                          aLibId.GetLibNickname().wx_str(),
                                          ioe.What() ) );
        }
    }

    if( alias != nullptr )
    {
        LIB_PINS   temp_pins;
        LIB_FIELD* fp_field = alias->GetPart()->GetField( FOOTPRINT );
        wxString   fp_name = fp_field ? fp_field->GetFullText() : wxString( "" );

        alias->GetPart()->GetPins( temp_pins );

        m_fp_sel_ctrl->FilterByPinCount( temp_pins.size() );
        m_fp_sel_ctrl->FilterByFootprintFilters( alias->GetPart()->GetFootprints(), true );
        m_fp_sel_ctrl->SetDefaultFootprint( fp_name );
        m_fp_sel_ctrl->UpdateList();
        m_fp_sel_ctrl->Enable();
    }
    else
    {
        m_fp_sel_ctrl->UpdateList();
        m_fp_sel_ctrl->Disable();
    }
}


void DIALOG_CHOOSE_COMPONENT::OnSchViewPaint( wxPaintEvent& aEvent )
{
    int unit = 0;
    LIB_ID id = m_tree->GetSelectedLibId( &unit );

    if( !id.IsValid() )
    {
        // No symbol to show, display a tooltip
        RenderPreview( nullptr, unit );
        return;
    }

    LIB_ALIAS* alias = nullptr;

    try
    {
        alias = Prj().SchSymbolLibTable()->LoadSymbol( id );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( wxString::Format( _( "Error occurred loading symbol %s from library %s."
                                         "\n\n%s" ),
                                      id.GetLibItemName().wx_str(),
                                      id.GetLibNickname().wx_str(),
                                      ioe.What() ) );
    }

    if( alias == nullptr )
        return;

    LIB_PART*  part = alias ? alias->GetPart() : nullptr;

    // Don't draw if we don't have a part to show
    // just display a tooltip
    if( !part )
    {
        RenderPreview( nullptr, unit );
        return;
    }

    if( alias->IsRoot() )
    {
        // just show the part directly
        RenderPreview( part, unit );
    }
    else
    {
        // switch out the name temporarily for the alias name
        wxString tmp( part->GetName() );
        part->SetName( alias->GetName() );

        RenderPreview( part, unit );

        part->SetName( tmp );
    }
}


void DIALOG_CHOOSE_COMPONENT::OnFootprintSelected( wxCommandEvent& aEvent )
{
    m_fp_override = aEvent.GetString();

    m_field_edits.erase(
            std::remove_if( m_field_edits.begin(), m_field_edits.end(),
                    []( std::pair<int, wxString> const& i ) { return i.first == FOOTPRINT; } ),
            m_field_edits.end() );

    m_field_edits.push_back( std::make_pair( FOOTPRINT, m_fp_override ) );

    ShowFootprint( m_fp_override );
}


void DIALOG_CHOOSE_COMPONENT::OnComponentPreselected( wxCommandEvent& aEvent )
{
    int unit = 0;

    LIB_ID id = m_tree->GetSelectedLibId( &unit );

    m_sch_view_ctrl->Refresh();

    if( id.IsValid() )
    {
        ShowFootprintFor( id );
        PopulateFootprintSelector( id );
    }
    else
    {
        if( m_fp_view_ctrl && m_fp_view_ctrl->IsInitialized() )
            m_fp_view_ctrl->SetStatusText( wxEmptyString );

        PopulateFootprintSelector( id );
    }
}


void DIALOG_CHOOSE_COMPONENT::OnComponentSelected( wxCommandEvent& aEvent )
{
    if( m_tree->GetSelectedLibId().IsValid() )
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
}


void DIALOG_CHOOSE_COMPONENT::RenderPreview( LIB_PART* aComponent, int aUnit )
{
    wxPaintDC dc( m_sch_view_ctrl );

    const wxSize dc_size = dc.GetSize();

    // Avoid rendering when either dimension is zero
    if( dc_size.x == 0 || dc_size.y == 0 )
        return;

    if( !aComponent )   // display a tooltip
    {
        wxString tooltip = _( "Double-click here to select a symbol from the library browser" );
        GRDrawWrappedText( dc, tooltip );
        return;
    }

    GRResetPenAndBrush( &dc );

    COLOR4D bgColor = m_parent->GetDrawBgColor();

    dc.SetBackground( wxBrush( bgColor.ToColour() ) );
    dc.Clear();

    int unit = aUnit > 0 ? aUnit : 1;
    int convert = m_deMorganConvert > 0 ? m_deMorganConvert : 1;

    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Find joint bounding box for everything we are about to draw.
    EDA_RECT     bBox = aComponent->GetUnitBoundingBox( unit, convert );
    const double xscale = (double) dc_size.x / bBox.GetWidth();
    const double yscale = (double) dc_size.y / bBox.GetHeight();
    const double scale = std::min( xscale, yscale ) * 0.85;

    dc.SetUserScale( scale, scale );

    wxPoint offset = -bBox.Centre();

    auto opts = PART_DRAW_OPTIONS::Default();
    opts.draw_hidden_fields = false;
    aComponent->Draw( nullptr, &dc, offset, unit, convert, opts );
}
