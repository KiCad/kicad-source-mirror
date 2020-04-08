/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <regex>

#include <bitmaps.h>
#include <dialogs/dialog_color_picker.h>
#include <eeschema_settings.h>
#include <gal/gal_display_options.h>
#include <layers_id_colors_and_visibility.h>
#include <class_libentry.h>
#include <lib_polyline.h>
#include <menus_helpers.h>
#include <page_info.h>
#include <panel_eeschema_color_settings.h>
#include <pgm_base.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_painter.h>
#include <sch_preview_panel.h>
#include <sch_text.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <title_block.h>
#include <view/view.h>
#include <ws_proxy_view_item.h>
#include <sch_base_frame.h>
#include <validators.h>

// Width and height of every (color-displaying / bitmap) button in dialog units
const wxSize BUTTON_SIZE( 24, 12 );
const wxSize BUTTON_BORDER( 4, 4 );

// Button ID starting point
constexpr int FIRST_BUTTON_ID = 1800;


PANEL_EESCHEMA_COLOR_SETTINGS::PANEL_EESCHEMA_COLOR_SETTINGS( SCH_BASE_FRAME* aFrame,
                                                              wxWindow* aParent ) :
          PANEL_COLOR_SETTINGS( aParent ),
          m_frame( aFrame ),
          m_preview( nullptr ),
          m_currentSettings( nullptr ),
          m_page( nullptr ),
          m_titleBlock( nullptr ),
          m_ws( nullptr ),
          m_previewItems(),
          m_buttons(),
          m_copied( COLOR4D::UNSPECIFIED )
{
    m_buttonSizePx = ConvertDialogToPixels( BUTTON_SIZE );

    SETTINGS_MANAGER&  mgr = Pgm().GetSettingsManager();

    mgr.ReloadColorSettings();

    COMMON_SETTINGS*   common_settings = Pgm().GetCommonSettings();
    EESCHEMA_SETTINGS* app_settings = mgr.GetAppSettings<EESCHEMA_SETTINGS>();
    COLOR_SETTINGS*    current = mgr.GetColorSettings( app_settings->m_ColorTheme );

    m_cbTheme->Clear();

    for( COLOR_SETTINGS* settings : mgr.GetColorSettingsList() )
    {
        int pos = m_cbTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings == current )
            m_cbTheme->SetSelection( pos );
    }

    m_cbTheme->Append( wxT( "---" ) );
    m_cbTheme->Append( _( "New Theme..." ) );

    m_currentSettings = new COLOR_SETTINGS( *current );

    KIGFX::GAL_DISPLAY_OPTIONS options;
    options.ReadConfig( *common_settings, app_settings->m_Window, this );
    options.m_forceDisplayCursor = false;

    auto type = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( app_settings->m_Graphics.canvas_type );

    m_preview = new SCH_PREVIEW_PANEL( this, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ),
                                       options, type );
    m_preview->SetStealsFocus( false );
    m_preview->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );

    createButtons();

    Connect( FIRST_BUTTON_ID, FIRST_BUTTON_ID + ( SCH_LAYER_ID_END - SCH_LAYER_ID_START ),
             wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( PANEL_EESCHEMA_COLOR_SETTINGS::SetColor ) );

    m_colorsMainSizer->Add( 10, 0, 0, wxEXPAND, 5 );
    m_colorsMainSizer->Add( m_preview, 1, wxALL | wxEXPAND, 5 );
    m_colorsMainSizer->Add( 10, 0, 0, wxEXPAND, 5 );

    createPreviewItems();
    updatePreview();
    zoomFitPreview();
}


PANEL_EESCHEMA_COLOR_SETTINGS::~PANEL_EESCHEMA_COLOR_SETTINGS()
{
    delete m_page;
    delete m_titleBlock;

    for( EDA_ITEM* item : m_previewItems )
        delete item;
}


bool PANEL_EESCHEMA_COLOR_SETTINGS::TransferDataFromWindow()
{
    if( !saveCurrentTheme( true ) )
        return false;

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->LoadColors( m_currentSettings );

    SETTINGS_MANAGER& settingsMgr = Pgm().GetSettingsManager();
    EESCHEMA_SETTINGS* app_settings = settingsMgr.GetAppSettings<EESCHEMA_SETTINGS>();
    app_settings->m_ColorTheme = m_currentSettings->GetFilename();

    return true;
}


bool PANEL_EESCHEMA_COLOR_SETTINGS::TransferDataToWindow()
{
    zoomFitPreview();
    return true;
}


bool PANEL_EESCHEMA_COLOR_SETTINGS::saveCurrentTheme( bool aValidate )
{
    if( aValidate )
    {
        COLOR4D bgcolor = m_currentSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

        for( SCH_LAYER_ID layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; ++layer )
        {
            if( bgcolor == m_currentSettings->GetColor( layer )
                && layer != LAYER_SCHEMATIC_BACKGROUND && layer != LAYER_SHEET_BACKGROUND )
            {
                wxString msg = _( "Some items have the same color as the background\n"
                                  "and they will not be seen on the screen.  Are you\n"
                                  "sure you want to use these colors?" );

                if( wxMessageBox( msg, _( "Warning" ), wxYES_NO | wxICON_QUESTION, this ) == wxNO )
                    return false;

                break;
            }
        }
    }

    SETTINGS_MANAGER& settingsMgr = Pgm().GetSettingsManager();
    COLOR_SETTINGS* selected = settingsMgr.GetColorSettings( m_currentSettings->GetFilename() );

    for( SCH_LAYER_ID layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; ++layer )
    {
        COLOR4D color;

        if( layer == LAYER_SHEET )
            color = m_frame->GetDefaultSheetBorderColor();
        else if( layer == LAYER_SHEET_BACKGROUND )
            color = m_frame->GetDefaultSheetBackgroundColor();
        else
            color = m_currentSettings->GetColor( layer );

        // Do not allow non-background layers to be completely white.
        // This ensures the BW printing recognizes that the colors should be printed black.
        if( color == COLOR4D::WHITE
                && layer != LAYER_SCHEMATIC_BACKGROUND && layer != LAYER_SHEET_BACKGROUND )
        {
            color.Darken( 0.01 );
        }

        selected->SetColor( layer, color );
    }

    settingsMgr.SaveColorSettings( selected, "schematic" );

    return true;
}


void PANEL_EESCHEMA_COLOR_SETTINGS::createButtons()
{
    const int flags  = wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxRIGHT;
    wxSize    border = ConvertDialogToPixels( BUTTON_BORDER );

    std::vector<SCH_LAYER_ID> layers;

    for( SCH_LAYER_ID i = SCH_LAYER_ID_START; i < SCH_LAYER_ID_END; ++i )
        layers.push_back( i );

    std::sort( layers.begin(), layers.end(),
               []( SCH_LAYER_ID a, SCH_LAYER_ID b )
               {
                   return LayerName( a ) < LayerName( b );
               } );

    for( SCH_LAYER_ID layer : layers )
    {
        // Sheet borders and backgrounds are now sheet-specific
        if( layer == LAYER_SHEET || layer == LAYER_SHEET_BACKGROUND )
            continue;

        wxString      name  = LayerName( layer );
        wxStaticText* label = new wxStaticText( m_colorsListWindow, wxID_ANY, name );
        COLOR4D       color = m_currentSettings->GetColor( layer );

        wxMemoryDC    iconDC;
        wxBitmap      bitmap( m_buttonSizePx );

        iconDC.SelectObject( bitmap );
        iconDC.SetPen( *wxBLACK_PEN );

        wxBrush brush;
        brush.SetColour( color.ToColour() );
        brush.SetStyle( wxBRUSHSTYLE_SOLID );
        iconDC.SetBrush( brush );
        iconDC.DrawRectangle( 0, 0, m_buttonSizePx.x, m_buttonSizePx.y );

        int id = FIRST_BUTTON_ID + ( layer - SCH_LAYER_ID_START );

        auto button = new wxBitmapButton( m_colorsListWindow, id, bitmap, wxDefaultPosition,
                                          m_buttonSizePx + border + wxSize( 1, 1 ) );
        button->SetToolTip( _( "Edit color (right click for options)" ) );

        m_colorsGridSizer->Add( label, 0, flags, 5 );
        m_colorsGridSizer->Add( button, 0, flags, 5 );

        m_buttons[layer] = button;

        button->Bind( wxEVT_RIGHT_DOWN,
                      [&, layer]( wxMouseEvent& aEvent )
                      {
                          ShowColorContextMenu( aEvent, layer );
                      } );
    }
}


void PANEL_EESCHEMA_COLOR_SETTINGS::drawButton( wxBitmapButton* aButton, const COLOR4D& aColor )
{
    wxMemoryDC iconDC;

    wxBitmap bitmap = aButton->GetBitmapLabel();
    iconDC.SelectObject( bitmap );
    iconDC.SetPen( *wxBLACK_PEN );

    wxBrush  brush;
    brush.SetColour( aColor.ToColour() );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, m_buttonSizePx.x, m_buttonSizePx.y );
    aButton->SetBitmapLabel( bitmap );
    aButton->Refresh();
}


void PANEL_EESCHEMA_COLOR_SETTINGS::createPreviewItems()
{
    KIGFX::VIEW* view = m_preview->GetView();

    m_page       = new PAGE_INFO( PAGE_INFO::Custom );
    m_titleBlock = new TITLE_BLOCK;

    m_page->SetHeightMils( 5000 );
    m_page->SetWidthMils( 5500 );

    m_ws = new KIGFX::WS_PROXY_VIEW_ITEM( (int) IU_PER_MILS, m_page, nullptr, m_titleBlock );
    m_ws->SetColorLayer( LAYER_SCHEMATIC_WORKSHEET );
    view->Add( m_ws );

    // NOTE: It would be nice to parse a schematic file here.
    // This is created from the color_settings.sch file in demos folder

    auto addItem = [&]( EDA_ITEM* aItem )
                   {
                       view->Add( aItem );
                       m_previewItems.push_back( aItem );
                   };

    std::vector<std::pair<SCH_LAYER_ID, std::pair<wxPoint, wxPoint>>> lines = {
                { LAYER_WIRE, { { 1950, 1500 }, { 2325, 1500 } } },
                { LAYER_WIRE, { { 1950, 2600 }, { 2350, 2600 } } },
                { LAYER_WIRE, { { 2150, 1700 }, { 2325, 1700 } } },
                { LAYER_WIRE, { { 2150, 2000 }, { 2150, 1700 } } },
                { LAYER_WIRE, { { 2925, 1600 }, { 3075, 1600 } } },
                { LAYER_WIRE, { { 3075, 1600 }, { 3075, 2000 } } },
                { LAYER_WIRE, { { 3075, 1600 }, { 3250, 1600 } } },
                { LAYER_WIRE, { { 3075, 2000 }, { 2150, 2000 } } },
                { LAYER_BUS, { { 1750, 1400 }, { 1850, 1400 } } },
                { LAYER_BUS, { { 1850, 2500 }, { 1850, 1400 } } },
                { LAYER_NOTES, { { 2350, 2125 }, { 2350, 2300 } } },
                { LAYER_NOTES, { { 2350, 2125 }, { 2950, 2125 } } },
                { LAYER_NOTES, { { 2950, 2125 }, { 2950, 2300 } } },
                { LAYER_NOTES, { { 2950, 2300 }, { 2350, 2300 } } }
            };

    for( const auto& line : lines )
    {
        SCH_LINE* wire = new SCH_LINE;
        wire->SetLayer( line.first );
        wire->SetStartPoint(
                wxPoint( Mils2iu( line.second.first.x ), Mils2iu( line.second.first.y ) ) );
        wire->SetEndPoint(
                wxPoint( Mils2iu( line.second.second.x ), Mils2iu( line.second.second.y ) ) );
        addItem( wire );
    }

    auto nc = new SCH_NO_CONNECT;
    nc->SetPosition( wxPoint( Mils2iu( 2525 ), Mils2iu( 1300 ) ) );
    addItem( nc );

    auto e1 = new SCH_BUS_WIRE_ENTRY;
    e1->SetPosition( wxPoint( Mils2iu( 1850 ), Mils2iu( 1400 ) ) );
    addItem( e1 );

    auto e2 = new SCH_BUS_WIRE_ENTRY;
    e2->SetPosition( wxPoint( Mils2iu( 1850 ), Mils2iu( 2500 ) ) );
    addItem( e2 );

    auto t1 = new SCH_TEXT( wxPoint( Mils2iu( 2850 ), Mils2iu( 2250 ) ), wxT( "PLAIN TEXT" ) );
    t1->SetLabelSpinStyle( LABEL_SPIN_STYLE::SPIN::LEFT );
    addItem( t1 );

    auto t2 = new SCH_LABEL( wxPoint( Mils2iu( 1975 ), Mils2iu( 1500 ) ), wxT( "GLOBAL0" ) );
    t2->SetLabelSpinStyle( LABEL_SPIN_STYLE::SPIN::RIGHT );
    t2->SetIsDangling( false );
    addItem( t2 );

    auto t3 = new SCH_LABEL( wxPoint( Mils2iu( 1975 ), Mils2iu( 2600 ) ), wxT( "GLOBAL1" ) );
    t3->SetLabelSpinStyle( LABEL_SPIN_STYLE::SPIN::RIGHT );
    t3->SetIsDangling( false );
    addItem( t3 );

    auto t4 = new SCH_GLOBALLABEL(
            wxPoint( Mils2iu( 1750 ), Mils2iu( 1400 ) ), wxT( "GLOBAL[3..0]" ) );
    t4->SetLabelSpinStyle( LABEL_SPIN_STYLE::SPIN::LEFT );
    t4->SetIsDangling( false );
    addItem( t4 );

    auto t5 = new SCH_HIERLABEL( wxPoint( Mils2iu( 3250 ), Mils2iu( 1600 ) ), wxT( "HIER_LABEL" ) );
    t5->SetLabelSpinStyle( LABEL_SPIN_STYLE::SPIN::RIGHT );
    t5->SetIsDangling( false );
    addItem( t5 );

    auto j = new SCH_JUNCTION( wxPoint( Mils2iu( 3075 ), Mils2iu( 1600 ) ) );
    addItem( j );

    e2->SetBrightened();
    t2->SetSelected();

    {
        auto part = new LIB_PART( wxEmptyString );
        wxPoint p( 2625, -1600 );

        part->SetShowPinNames( true );
        part->SetShowPinNumbers( true );

        addItem( part );

        auto comp_body = new LIB_POLYLINE( part );

        comp_body->SetUnit( 0 );
        comp_body->SetConvert( 0 );
        comp_body->SetWidth( Mils2iu( 10 ) );
        comp_body->SetFillMode( FILLED_WITH_BG_BODYCOLOR );
        comp_body->AddPoint( wxPoint( Mils2iu( p.x - 200 ), Mils2iu( p.y + 200 ) ) );
        comp_body->AddPoint( wxPoint( Mils2iu( p.x + 200 ), Mils2iu( p.y ) ) );
        comp_body->AddPoint( wxPoint( Mils2iu( p.x - 200 ), Mils2iu( p.y - 200 ) ) );
        comp_body->AddPoint( wxPoint( Mils2iu( p.x - 200 ), Mils2iu( p.y + 200 ) ) );

        addItem( comp_body );
    }

    zoomFitPreview();
}


void PANEL_EESCHEMA_COLOR_SETTINGS::SetColor( wxCommandEvent& event )
{
    auto button = static_cast<wxBitmapButton*>( event.GetEventObject() );
    auto layer =
            static_cast<SCH_LAYER_ID>( button->GetId() - FIRST_BUTTON_ID + SCH_LAYER_ID_START );

    COLOR4D oldColor = m_currentSettings->GetColor( layer );
    COLOR4D newColor = COLOR4D::UNSPECIFIED;
    DIALOG_COLOR_PICKER dialog( this, oldColor, false );

    if( dialog.ShowModal() == wxID_OK )
        newColor = dialog.GetColor();

    if( newColor == COLOR4D::UNSPECIFIED || oldColor == newColor )
        return;

    updateColor( layer, newColor );
}


void PANEL_EESCHEMA_COLOR_SETTINGS::updateColor( SCH_LAYER_ID aLayer, const KIGFX::COLOR4D& aColor )
{
    m_currentSettings->SetColor( aLayer, aColor );

    drawButton( m_buttons[aLayer], aColor );

    updatePreview();
}


void PANEL_EESCHEMA_COLOR_SETTINGS::OnBtnResetClicked( wxCommandEvent& event )
{
    for( const auto& pair : m_buttons )
    {
        SCH_LAYER_ID    layer  = pair.first;
        wxBitmapButton* button = pair.second;

        COLOR4D defaultColor = m_currentSettings->GetDefaultColor( layer );

        m_currentSettings->SetColor( layer, defaultColor );
        drawButton( button, defaultColor );
    }

    updatePreview();
}


void PANEL_EESCHEMA_COLOR_SETTINGS::updatePreview()
{
    KIGFX::VIEW* view = m_preview->GetView();
    auto settings = static_cast<KIGFX::SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    settings->LoadColors( m_currentSettings );

    m_preview->GetGAL()->SetClearColor( settings->GetBackgroundColor() );

    view->UpdateAllItems( KIGFX::COLOR );
    auto rect = m_preview->GetScreenRect();
    m_preview->Refresh( true, &rect );
}


void PANEL_EESCHEMA_COLOR_SETTINGS::zoomFitPreview()
{
    auto view = m_preview->GetView();

    view->SetScale( 1.0 );
    VECTOR2D screenSize = view->ToWorld( m_preview->GetClientSize(), false );

    VECTOR2I psize( m_page->GetWidthIU(), m_page->GetHeightIU() );
    double scale = view->GetScale() / std::max( fabs( psize.x / screenSize.x ),
                                                fabs( psize.y / screenSize.y ) );

    view->SetScale( scale / 1.02 );
    view->SetCenter( m_ws->ViewBBox().Centre() );
    m_preview->ForceRefresh();
}


void PANEL_EESCHEMA_COLOR_SETTINGS::OnSize( wxSizeEvent& aEvent )
{
    zoomFitPreview();
    aEvent.Skip();
}


void PANEL_EESCHEMA_COLOR_SETTINGS::OnThemeChanged( wxCommandEvent& event )
{
    int idx = m_cbTheme->GetSelection();

    if( idx == m_cbTheme->GetCount() - 2 )
    {
        // separator; re-select active theme
        m_cbTheme->SetStringSelection( m_currentSettings->GetName() );
        return;
    }

    if( idx == m_cbTheme->GetCount() - 1 )
    {
        // New Theme...

        if( !saveCurrentTheme( false ) )
            return;

        MODULE_NAME_CHAR_VALIDATOR themeNameValidator;
        wxTextEntryDialog dlg( this, _( "New theme name:" ), _( "Add Color Theme" ) );
        dlg.SetTextValidator( themeNameValidator );

        if( dlg.ShowModal() != wxID_OK )
            return;

        wxString themeName = dlg.GetValue();
        wxFileName fn( themeName + wxT( ".json" ) );
        fn.SetPath( SETTINGS_MANAGER::GetColorSettingsPath() );

        if( fn.Exists() )
        {
            wxMessageBox( _( "Theme already exists!" ) );
            return;
        }

        SETTINGS_MANAGER& settingsMgr = Pgm().GetSettingsManager();
        COLOR_SETTINGS* newSettings = settingsMgr.AddNewColorSettings( themeName );
        newSettings->SetName( themeName );

        for( SCH_LAYER_ID layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; ++layer )
            newSettings->SetColor( layer, m_currentSettings->GetColor( layer ) );

        newSettings->SaveToFile( settingsMgr.GetPathForSettingsFile( newSettings ) );

        idx = m_cbTheme->Insert( themeName, idx - 1, static_cast<void*>( newSettings ) );
        m_cbTheme->SetSelection( idx );
        *m_currentSettings = *newSettings;
    }
    else
    {
        COLOR_SETTINGS* selected = static_cast<COLOR_SETTINGS*>( m_cbTheme->GetClientData( idx ) );

        if( selected->GetFilename() != m_currentSettings->GetFilename() )
        {
            if( !saveCurrentTheme( false ) )
                return;

            *m_currentSettings = *selected;
            updatePreview();

            for( auto pair : m_buttons )
                drawButton( pair.second, m_currentSettings->GetColor( pair.first ) );
        }
    }
}


void PANEL_EESCHEMA_COLOR_SETTINGS::ShowColorContextMenu( wxMouseEvent& aEvent,
                                                          SCH_LAYER_ID aLayer )
{
    auto selected =
            static_cast<COLOR_SETTINGS*>( m_cbTheme->GetClientData( m_cbTheme->GetSelection() ) );

    COLOR4D current = m_currentSettings->GetColor( aLayer );
    COLOR4D saved   = selected->GetColor( aLayer );

    wxMenu menu;

    AddMenuItem( &menu, ID_COPY, _( "Copy color" ), KiBitmap( copy_xpm ) );

    if( m_copied != COLOR4D::UNSPECIFIED )
        AddMenuItem( &menu, ID_PASTE, _( "Paste color" ), KiBitmap( paste_xpm ) );

    if( current != saved )
        AddMenuItem( &menu, ID_REVERT, _( "Revert to saved color" ), KiBitmap( undo_xpm ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, [&]( wxCommandEvent& aCmd ) {
        switch( aCmd.GetId() )
        {
        case ID_COPY:
            m_copied = current;
            break;

        case ID_PASTE:
            updateColor( aLayer, m_copied );
            break;

        case ID_REVERT:
            updateColor( aLayer, saved );
            break;

        default:
            aCmd.Skip();
        }
    } );

    PopupMenu( &menu );
}
