/*************************************/
/*  tool_gerber.cpp: Build tool bars */
/*************************************/

#include "fctsys.h"

#include "common.h"
#include "macros.h"
#include "gerbview.h"
#include "bitmaps.h"
#include "gerbview_id.h"
#include "hotkeys.h"
#include "class_GERBER.h"
#include "class_layer_box_selector.h"
#include "class_DCodeSelectionbox.h"
#include "dialog_helpers.h"

void GERBVIEW_FRAME::ReCreateHToolbar( void )
{
    int           ii;
    wxString      msg;

    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new EDA_TOOLBAR( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );

    // Set up toolbar
    m_HToolBar->AddTool( ID_GERBVIEW_ERASE_ALL, wxEmptyString,
                         KiBitmap( gerbview_clear_layers_xpm ),
                         _( "Erase all layers" ) );

    m_HToolBar->AddTool( wxID_FILE, wxEmptyString, KiBitmap( gerber_file_xpm ),
                         _( "Load a new Gerber file on the current layer. Previous data will be deleted" ) );

    m_HToolBar->AddTool( ID_GERBVIEW_LOAD_DRILL_FILE, wxEmptyString,
                         KiBitmap( gerbview_drill_file_xpm ),
                         _( "Load an excellon drill file on the current layer. Previous data will be deleted" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_PRINT, wxEmptyString, KiBitmap( print_button_xpm ),
                         _( "Print layers" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_IN, false );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_OUT, false );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( _( "Redraw view" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_REDRAW, false );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_AUTO, false );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    m_HToolBar->AddSeparator();

    wxArrayString choices;

    for( ii = 0; ii < 32; ii++ )
    {
        msg.Printf( _( "Layer %d" ), ii + 1 );
        choices.Add( msg );
    }

    m_SelLayerBox = new LAYER_BOX_SELECTOR( m_HToolBar, ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                                            wxDefaultPosition, wxSize( 150, -1 ), choices );
    m_HToolBar->AddControl( m_SelLayerBox );

    m_HToolBar->AddSeparator();

    m_DCodesList.Alloc(TOOLS_MAX_COUNT+1);
    m_DCodesList.Add( _( "No tool" ) );

    for( ii = FIRST_DCODE; ii < TOOLS_MAX_COUNT; ii++ )
    {
        msg = _( "Tool " );
        msg << ii;
        m_DCodesList.Add( msg );
    }

    m_DCodeSelector = new DCODE_SELECTION_BOX( m_HToolBar, ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE,
                                               wxDefaultPosition, wxSize( 150, -1 ),
                                               m_DCodesList );
    m_HToolBar->AddControl( m_DCodeSelector );

    m_TextInfo = new wxTextCtrl( m_HToolBar, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                 wxSize(150,-1), wxTE_READONLY );
    m_HToolBar->AddControl( m_TextInfo );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_HToolBar->Realize();
}


/**
 * Create or update the right vertical toolbar
 * Current no used
 */
void GERBVIEW_FRAME::ReCreateVToolbar( void )
{
    if( m_VToolBar )
        return;

    m_VToolBar = new EDA_TOOLBAR( TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ) );
    m_VToolBar->AddSeparator();

    m_VToolBar->Realize();
}


/**
 * Create or update the left vertical toolbar (option toolbar
 */
void GERBVIEW_FRAME::ReCreateOptToolbar( void )
{
    if( m_OptionsToolBar )
        return;

    // creation of tool bar options
    m_OptionsToolBar = new EDA_TOOLBAR( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiBitmap( polar_coord_xpm ),
                               _( "Turn polar coordinate on" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ),
                               _( "Set units to inches" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Set units to millimeters" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH, wxEmptyString,
                               KiBitmap( pad_sketch_xpm ),
                               _( "Show spots in sketch mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_LINES_SKETCH, wxEmptyString,
                               KiBitmap( showtrack_xpm ),
                               _( "Show lines in sketch mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, wxEmptyString,
                               KiBitmap( opt_show_polygon_xpm ),
                               _( "Show polygons in sketch mode" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_DCODES, wxEmptyString,
                               KiBitmap( show_dcodenumber_xpm ),
                               _( "Show dcode number" ), wxITEM_CHECK );

    // tools to select draw mode in gerbview
    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_0, wxEmptyString,
                               KiBitmap( gbr_select_mode0_xpm ),
                               _( "Show layers in raw mode \
(could have problems with negative items when more than one gerber file is shown)" ),
                               wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_1, wxEmptyString,
                               KiBitmap( gbr_select_mode1_xpm ),
                               _( "Show layers in stacked mode \
(show negative items without artefact, sometimes slow)" ),
                               wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_2, wxEmptyString,
                               KiBitmap( gbr_select_mode2_xpm ),
                               _( "Show layers in tranparency mode \
(show negative items without artefact, sometimes slow)" ),
                               wxITEM_CHECK );

    // Tools to show/hide toolbars:
    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                               wxEmptyString,
                               KiBitmap( layers_manager_xpm ),
                               _( "Show/hide the layers manager toolbar" ),
                               wxITEM_CHECK );


    m_OptionsToolBar->Realize();
}


void GERBVIEW_FRAME::OnUpdateDrawMode( wxUpdateUIEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_TB_OPTIONS_SHOW_GBR_MODE_0:
        aEvent.Check( GetDisplayMode() == 0 );
        break;

    case ID_TB_OPTIONS_SHOW_GBR_MODE_1:
        aEvent.Check( GetDisplayMode() == 1 );
        break;

    case ID_TB_OPTIONS_SHOW_GBR_MODE_2:
        aEvent.Check( GetDisplayMode() == 2 );
        break;

    default:
        break;
    }
}


void GERBVIEW_FRAME::OnUpdateFlashedItemsDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayPadFill );
}


void GERBVIEW_FRAME::OnUpdateLinesDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayPcbTrackFill );
}


void GERBVIEW_FRAME::OnUpdatePolygonsDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( g_DisplayPolygonsModeSketch != 0 );
}


void GERBVIEW_FRAME::OnUpdateShowDCodes( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( IsElementVisible( DCODES_VISIBLE ) );
}


void GERBVIEW_FRAME::OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_show_layer_manager_tools );

    if( m_OptionsToolBar )
    {
        if( m_show_layer_manager_tools )
            m_OptionsToolBar->SetToolShortHelp( aEvent.GetId(), _("Hide layers manager" ) );
        else
            m_OptionsToolBar->SetToolShortHelp( aEvent.GetId(), _("Show layers manager" ) );
    }
}


void GERBVIEW_FRAME::OnUpdateSelectDCode( wxUpdateUIEvent& aEvent )
{
    int layer = getActiveLayer();
    GERBER_IMAGE* gerber = g_GERBER_List[layer];
    int selected = ( gerber ) ? gerber->m_Selected_Tool : 0;

    if( m_DCodeSelector && m_DCodeSelector->GetSelectedDCodeId() != selected )
        m_DCodeSelector->SetDCodeSelection( selected );

    aEvent.Enable( gerber != NULL );
}


void GERBVIEW_FRAME::OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent )
{
    if(  m_SelLayerBox && (m_SelLayerBox->GetSelection() != getActiveLayer()) )
    {
        m_SelLayerBox->SetSelection( getActiveLayer() );
    }
}
