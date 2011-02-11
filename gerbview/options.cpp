/************************/
/*   File options.cpp   */
/************************/

/*
 * Set some general options of Gerbview
 */


#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "gerbview.h"
#include "gerbview_id.h"


/**
 * Function OnSelectOptionToolbar
 *  called to validate current choices
 */
void WinEDA_GerberFrame::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int id = event.GetId();
    bool state;
    switch( id )
    {
        case ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG:
            state = ! m_show_layer_manager_tools;
            id = ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR;
            break;

        default:
            state = m_OptionsToolBar->GetToolState( id );
            break;
    }

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_GRID:
        SetGridVisibility( state );
        DrawPanel->Refresh( TRUE );
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_MM:
        g_UserUnit = MILLIMETRES;
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_INCH:
        g_UserUnit = INCHES;
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SHOW_POLAR_COORD:
        SetStatusText( wxEmptyString );
        DisplayOpt.DisplayPolarCood = state;
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SELECT_CURSOR:
        m_CursorShape = state;
        DrawPanel->Refresh( TRUE );
        break;

    case ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH:
        if( state )
        {
            DisplayOpt.DisplayPadFill = m_DisplayPadFill = false;
        }
        else
        {
            DisplayOpt.DisplayPadFill = m_DisplayPadFill = true;
        }
        DrawPanel->Refresh( TRUE );
        break;

    case ID_TB_OPTIONS_SHOW_LINES_SKETCH:
        if(state )
        {
            m_DisplayPcbTrackFill = FALSE;
            DisplayOpt.DisplayPcbTrackFill = FALSE;
        }
        else
        {
            m_DisplayPcbTrackFill = TRUE;
            DisplayOpt.DisplayPcbTrackFill = TRUE;
        }
        DrawPanel->Refresh( TRUE );
        break;

    case ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH:
        if( state )      // Polygons filled asked
            g_DisplayPolygonsModeSketch = 1;
        else
            g_DisplayPolygonsModeSketch = 0;
        DrawPanel->Refresh( TRUE );
        break;

    case ID_TB_OPTIONS_SHOW_DCODES:
        SetElementVisibility( DCODES_VISIBLE, state );
        DrawPanel->Refresh( TRUE );
        break;

    case ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR:
        // show/hide auxiliary Vertical layers and visibility manager toolbar
        m_show_layer_manager_tools = state;
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
        m_auimgr.Update();
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_PcbFrame::OnSelectOptionToolbar error" ) );
        break;
    }

    SetToolbars();
}

