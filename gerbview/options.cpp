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

#include "pcbplot.h"
#include "gerbview.h"

#include "gerbview_id.h"


/** Function OnSelectOptionToolbar
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
        Affiche_Message( wxEmptyString );
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


class WinEDA_GerberGeneralOptionsFrame : public wxDialog
{
private:

    WinEDA_BasePcbFrame* m_Parent;
    wxRadioBox*          m_PolarDisplay;
    wxRadioBox*          m_BoxUnits;
    wxRadioBox*          m_CursorShape;
    wxRadioBox*          m_GerberDefaultScale;

public:
    WinEDA_GerberGeneralOptionsFrame( WinEDA_BasePcbFrame* parent );
    ~WinEDA_GerberGeneralOptionsFrame() {};

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( WinEDA_GerberGeneralOptionsFrame, wxDialog )
    EVT_BUTTON( wxID_OK, WinEDA_GerberGeneralOptionsFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_GerberGeneralOptionsFrame::OnCancelClick )
END_EVENT_TABLE()


WinEDA_GerberGeneralOptionsFrame::WinEDA_GerberGeneralOptionsFrame(
    WinEDA_BasePcbFrame* parent ) :
    wxDialog( parent, -1, _( "Gerbview Options" ),
              wxDefaultPosition, wxSize( 300, 240 ),
              wxDEFAULT_DIALOG_STYLE | wxFRAME_FLOAT_ON_PARENT )
{
    m_Parent = parent;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* RightBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* MiddleBoxSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* LeftBoxSizer   = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( MiddleBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    wxButton* Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    /* Display / not display polar coordinates: */
    wxString list_coord[2] =
    {
        _( "No Display" ),
        _( "Display" )
    };
    m_PolarDisplay = new wxRadioBox( this, -1, _( "Display Polar Coord" ),
                                     wxDefaultPosition, wxDefaultSize,
                                     2, list_coord, 1 );
    m_PolarDisplay->SetSelection( DisplayOpt.DisplayPolarCood ? 1 : 0 );
    LeftBoxSizer->Add( m_PolarDisplay, 0, wxGROW | wxALL, 5 );

    /* Selection of units */
    wxString list_units[2] =
    {
        _( "Inches" ),
        _( "millimeters" )
    };
    m_BoxUnits = new wxRadioBox( this, -1, _( "Units" ), wxDefaultPosition,
                                 wxDefaultSize,
                                 2, list_units, 1 );
    m_BoxUnits->SetSelection( g_UserUnit ? 1 : 0 );
    LeftBoxSizer->Add( m_BoxUnits, 0, wxGROW | wxALL, 5 );

    /* Selection of cursor shape */
    wxString list_cursors[2] = { _( "Small" ), _( "Big" ) };
    m_CursorShape = new wxRadioBox( this, -1, _( "Cursor" ), wxDefaultPosition,
                                    wxDefaultSize,
                                    2, list_cursors, 1 );
    m_CursorShape->SetSelection( parent->m_CursorShape ? 1 : 0 );
    MiddleBoxSizer->Add( m_CursorShape, 0, wxGROW | wxALL, 5 );

    /* Selection Default Scale (i.e. format 2.3 ou 3.4) */
    wxString list_scales[2] = { _( "format: 2.3" ), _( "format 3.4" ) };
    m_GerberDefaultScale = new wxRadioBox( this, -1, _( "Default format" ),
                                           wxDefaultPosition, wxDefaultSize,
                                           2, list_scales, 1 );
    m_GerberDefaultScale->SetSelection(
         (g_Default_GERBER_Format == 23) ? 0 : 1 );
    MiddleBoxSizer->Add( m_GerberDefaultScale, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void WinEDA_GerberGeneralOptionsFrame::OnCancelClick(
     wxCommandEvent& WXUNUSED(event) )
{
    EndModal( -1 );
}


void WinEDA_GerberGeneralOptionsFrame::OnOkClick( wxCommandEvent& event )
{
    DisplayOpt.DisplayPolarCood =
        (m_PolarDisplay->GetSelection() == 0) ? FALSE : TRUE;
    g_UserUnit  = (m_BoxUnits->GetSelection() == 0) ? INCHES : MILLIMETRES;
    m_Parent->m_CursorShape = m_CursorShape->GetSelection();
    g_Default_GERBER_Format =
        (m_GerberDefaultScale->GetSelection() == 0) ? 23 : 34;

    EndModal( 1 );
}


void WinEDA_GerberFrame::InstallGerberGeneralOptionsFrame( wxCommandEvent& event )
{
       WinEDA_GerberGeneralOptionsFrame dlg( this );
       dlg.ShowModal();
}
