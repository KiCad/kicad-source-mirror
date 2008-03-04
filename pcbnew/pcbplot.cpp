/***********************/
/* fichier pcbplot.cpp */
/***********************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "worksheet.h"
#include "id.h"

#include "protos.h"

#define PLOT_DEFAULT_MARGE 300  // mils

/* Keywords to r/w options in config */
#define EDGELAYER_GERBER_OPT_KEY wxT( "EdgeLayerGerberOpt" )
#define PLOT_XFINESCALE_ADJ_KEY  wxT( "PlotXFineScaleAdj" )
#define PLOT_YFINESCALE_ADJ_KEY  wxT( "PlotYFineScaleAdj" )

// variables locale :
static long s_SelectedLayers = CUIVRE_LAYER | CMP_LAYER |
                               SILKSCREEN_LAYER_CMP | SILKSCREEN_LAYER_CU;
static bool s_PlotOriginIsAuxAxis = FALSE;

// Routines Locales

enum id_plotps {
    ID_CLOSE_PLOT = 8020,
    ID_DRILL_SHAPE_OPT,
    ID_SCALE_OPT,
    ID_ROTATE_OPT,
    ID_MIROR_OPT,
    ID_EXEC_PLOT,
    ID_PLOT_MODE_OPT,
    ID_MASKVIA_OPT,
    ID_PLOT_CENTRE_OPT,
    ID_SEL_PLOT_FORMAT,
    ID_SEL_GERBER_FORMAT,
    ID_SAVE_OPT_PLOT,
    ID_EXCLUDE_EDGES_PCB,
    ID_PRINT_REF,
    ID_PRINT_VALUE,
    ID_PRINT_MODULE_TEXTS,
    ID_FORCE_PRINT_INVISIBLE_TEXT,
    ID_PRINT_PAD_ON_SILKSCREEN,
    ID_FORCE_PRINT_PAD,
    ID_CREATE_DRILL_FILE,
    ID_SEL_PLOT_OFFSET_OPTION
};


/*******************************/
/* Dialog box for plot control */
/*******************************/

class WinEDA_PlotFrame : public wxDialog
{
public:
    WinEDA_BasePcbFrame*    m_Parent;
    wxButton*               m_MergePlotButton;
    wxCheckBox*             m_BoxSelecLayer[32];
    wxRadioBox*             m_PlotFormatOpt;
    wxRadioBox*             m_Choice_Plot_Offset;
    wxRadioBox*             m_Drill_Shape_Opt;
    wxRadioBox*             m_Scale_Opt;
    wxRadioBox*             m_PlotModeOpt;
    wxCheckBox*             m_PlotMirorOpt;
    wxCheckBox*             m_PlotNoViaOnMaskOpt;
    wxCheckBox*             m_HPGL_PlotCenter_Opt;
    wxCheckBox*             m_Exclude_Edges_Pcb;
    wxCheckBox*             m_Plot_Sheet_Ref;
    wxCheckBox*             m_Plot_Invisible_Text;
    wxCheckBox*             m_Plot_Text_Value;
    wxCheckBox*             m_Plot_Text_Ref;
    wxCheckBox*             m_Plot_Text_Div;
    wxCheckBox*             m_Plot_Pads_on_Silkscreen;
    wxCheckBox*             m_Force_Plot_Pads;
    wxCheckBox*             m_Plot_PS_Negative;
    WinEDA_ValueCtrl*       m_GerbSpotSizeMinOpt;
    WinEDA_ValueCtrl*       m_LinesWidth;
    WinEDA_ValueCtrl*       m_HPGLPenSizeOpt;
    WinEDA_ValueCtrl*       m_HPGLPenSpeedOpt;
    WinEDA_ValueCtrl*       m_HPGLPenOverlayOpt;
    WinEDA_DFloatValueCtrl* m_FineAdjustXscaleOpt, * m_FineAdjustYscaleOpt;
    double m_XScaleAdjust, m_YScaleAdjust;
    int    m_PlotFormat;

public:
    WinEDA_PlotFrame( WinEDA_BasePcbFrame * parent );
private:
    void    Plot( wxCommandEvent& event );
    void    OnQuit( wxCommandEvent& event );
    void    OnClose( wxCloseEvent& event );
    void    SetCommands( wxCommandEvent& event );
    void    SaveOptPlot( wxCommandEvent& event );
    void    CreateDrillFile( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_PlotFrame, wxDialog )
    EVT_CLOSE( WinEDA_PlotFrame::OnClose )
    EVT_BUTTON( ID_CLOSE_PLOT, WinEDA_PlotFrame::OnQuit )
    EVT_BUTTON( ID_EXEC_PLOT, WinEDA_PlotFrame::Plot )
    EVT_BUTTON( ID_SAVE_OPT_PLOT, WinEDA_PlotFrame::SaveOptPlot )
    EVT_BUTTON( ID_CREATE_DRILL_FILE, WinEDA_PlotFrame::CreateDrillFile )
    EVT_RADIOBOX( ID_SEL_PLOT_FORMAT, WinEDA_PlotFrame::SetCommands )
END_EVENT_TABLE()


const int UNITS_MILS = 1000;


/********************************************************************/
WinEDA_PlotFrame::WinEDA_PlotFrame( WinEDA_BasePcbFrame* parent ) :
    wxDialog( parent, -1, _( "Plot" ),
              wxPoint( -1, -1 ), wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE )
/********************************************************************/
{
    wxButton* Button;
    int       ii;

    m_Parent = parent;

    BOARD* board = parent->m_Pcb;

    SetFont( *g_DialogFont );
    Centre();

    m_PlotFormat     = format_plot;
    m_Plot_Sheet_Ref = NULL;

    wxBoxSizer* MainBoxSizer = new      wxBoxSizer( wxHORIZONTAL );

    SetSizer( MainBoxSizer );
    wxBoxSizer* RightBoxSizer = new     wxBoxSizer( wxVERTICAL );

    wxBoxSizer* MidRightBoxSizer = new  wxBoxSizer( wxVERTICAL );

    wxBoxSizer* MidLeftBoxSizer = new   wxBoxSizer( wxVERTICAL );

    wxBoxSizer* LeftBoxSizer = new      wxBoxSizer( wxVERTICAL );

    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( MidLeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( MidRightBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxGROW | wxALL, 5 );

    wxBoxSizer* LayersBoxSizer = new wxBoxSizer( wxHORIZONTAL );

    LeftBoxSizer->Add( LayersBoxSizer, 0, wxGROW | wxALL, 5 );

    static const wxString fmtmsg[4] = {
        wxT( "HPGL" ), wxT( "Gerber" ), wxT( "Postscript" ), wxT( "Postscript A4" )
    };

    m_PlotFormatOpt = new wxRadioBox( this, ID_SEL_PLOT_FORMAT,
                                      _( "Plot Format" ), wxDefaultPosition, wxSize( -1, -1 ),
                                      4, fmtmsg, 1, wxRA_SPECIFY_COLS );

    MidRightBoxSizer->Add( m_PlotFormatOpt, 0, wxGROW | wxALL, 5 );

    switch( m_PlotFormat )
    {
    case PLOT_FORMAT_HPGL:
        m_PlotFormatOpt->SetSelection( 0 );
        break;

    case PLOT_FORMAT_GERBER:
        m_PlotFormatOpt->SetSelection( 1 );
        break;

    default: // ( PLOT_FORMAT_POST or PLOT_FORMAT_POST_A4 )
        // As m_PlotFormat is never set to a value of PLOT_FORMAT_POST_A4,
        // use the value of g_ForcePlotPS_On_A4 to determine whether the
        // "Postscript" or "Postscipt A4" radiobutton had been selected
        // previously (and thus which button should be reselected now).
        if( g_ForcePlotPS_On_A4 )
            m_PlotFormatOpt->SetSelection( 3 );
        else
            m_PlotFormatOpt->SetSelection( 2 );
        break;
    }

    // Creation des menus d'option du format GERBER
    m_GerbSpotSizeMinOpt = new  WinEDA_ValueCtrl( this, _( "Spot min" ),
                                                  spot_mini, g_UnitMetric, MidRightBoxSizer,
                                                  UNITS_MILS );

    // Creation des menus d'option du format HPGL
    m_HPGLPenSizeOpt = new      WinEDA_ValueCtrl( this, _( "Pen Size" ),
                                                  g_HPGL_Pen_Diam, g_UnitMetric, MidRightBoxSizer,
                                                  UNITS_MILS );

    // unites standards = cm  pour vitesse plume en HPGL
    m_HPGLPenSpeedOpt = new     WinEDA_ValueCtrl( this, _( "Pen Speed (cm/s)" ),
                                                  g_HPGL_Pen_Speed, CENTIMETRE, MidRightBoxSizer,
                                                  1 );

    m_HPGLPenSpeedOpt->SetToolTip( _( "Set pen speed in cm/s" ) );

    m_HPGLPenOverlayOpt = new   WinEDA_ValueCtrl( this, _( "Pen Ovr" ),
                                                  g_HPGL_Pen_Recouvrement, g_UnitMetric,
                                                  MidRightBoxSizer, UNITS_MILS );

    m_HPGLPenOverlayOpt->SetToolTip( _( "Set plot overlay for filling" ) );

    m_LinesWidth = new          WinEDA_ValueCtrl( this, _( "Lines Width" ),
                                                  g_PlotLine_Width, g_UnitMetric, MidRightBoxSizer,
                                                  PCB_INTERNAL_UNIT );

    m_LinesWidth->SetToolTip( _( "Set width for lines in Line plot mode" ) );

    // Create the right column commands
    static const wxString choice_plot_offset_msg[] =
    {  _( "Absolute" ), _( "Auxiliary axis" )  };

    m_Choice_Plot_Offset = new wxRadioBox( this, ID_SEL_PLOT_OFFSET_OPTION,
                                           _( "Plot Origin" ),
                                           wxDefaultPosition, wxSize( -1, -1 ),
                                           2, choice_plot_offset_msg, 1, wxRA_SPECIFY_COLS );

    if( s_PlotOriginIsAuxAxis )
        m_Choice_Plot_Offset->SetSelection( 1 );
    RightBoxSizer->Add( m_Choice_Plot_Offset, 0, wxGROW | wxALL, 5 );
    /* Add a spacer for a better look */
    RightBoxSizer->Add( 5, 5, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 20 );

    /* Create the command buttons */
    Button = new    wxButton( this, ID_EXEC_PLOT, _( "Plot" ) );

    Button->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new    wxButton( this, ID_CLOSE_PLOT, _( "Close" ) );

    Button->SetForegroundColour( *wxBLUE );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new    wxButton( this, ID_SAVE_OPT_PLOT, _( "Save options" ) );

    Button->SetForegroundColour( wxColour( 0, 80, 0 ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new    wxButton( this, ID_CREATE_DRILL_FILE, _( "Create Drill File" ) );

    Button->SetForegroundColour( wxColour( 0, 80, 80 ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    // Create scale adjust option
    m_XScaleAdjust = m_YScaleAdjust = 1.0;
    if( m_Parent->m_Parent->m_EDA_Config )
    {
        m_Parent->m_Parent->m_EDA_Config->Read( EDGELAYER_GERBER_OPT_KEY, &g_Exclude_Edges_Pcb );
        m_Parent->m_Parent->m_EDA_Config->Read( PLOT_XFINESCALE_ADJ_KEY, &m_XScaleAdjust );
        m_Parent->m_Parent->m_EDA_Config->Read( PLOT_XFINESCALE_ADJ_KEY, &m_YScaleAdjust );
    }
    m_FineAdjustXscaleOpt = new WinEDA_DFloatValueCtrl( this, _(
                                                            "X Scale Adjust" ), m_XScaleAdjust,
                                                        RightBoxSizer );

    m_FineAdjustXscaleOpt->SetToolTip( _( "Set X scale adjust for exact scale plotting" ) );
    m_FineAdjustYscaleOpt = new WinEDA_DFloatValueCtrl( this, _(
                                                            "Y Scale Adjust" ), m_YScaleAdjust,
                                                        RightBoxSizer );

    m_FineAdjustYscaleOpt->SetToolTip( _( "Set Y scale adjust for exact scale plotting" ) );

    m_Plot_PS_Negative = new    wxCheckBox( this, -1, _( "Plot Negative" ) );

    m_Plot_PS_Negative->SetValue( g_Plot_PS_Negative );
    RightBoxSizer->Add( m_Plot_PS_Negative, 0, wxGROW | wxALL, 5 );

    // Creation de la liste des layers
    int         mask = 1;
    wxBoxSizer* OneColonLayerBoxSizer = new wxBoxSizer( wxVERTICAL );

    LayersBoxSizer->Add( OneColonLayerBoxSizer, 0, wxGROW | wxALL, 5 );

    for( ii = 0; ii < NB_LAYERS; ii++, mask <<= 1 )
    {
        if( ii == 16 )
        {
            OneColonLayerBoxSizer = new wxBoxSizer( wxVERTICAL );

            LayersBoxSizer->Add( OneColonLayerBoxSizer, 0, wxGROW | wxALL, 5 );
        }

        m_BoxSelecLayer[ii] = new       wxCheckBox( this, -1, board->GetLayerName (ii) );

        if( mask & s_SelectedLayers )
            m_BoxSelecLayer[ii]->SetValue( true );
        OneColonLayerBoxSizer->Add( m_BoxSelecLayer[ii], 0, wxGROW | wxALL, 1 );
    }

    // Option for excluding contents of "Edges Pcb" layer

    m_Exclude_Edges_Pcb = new wxCheckBox( this,
                                         ID_EXCLUDE_EDGES_PCB, _( "Exclude Edges Pcb Layer" ) );

    m_Exclude_Edges_Pcb->SetValue( g_Exclude_Edges_Pcb );
    m_Exclude_Edges_Pcb->SetToolTip(
        _( "Exclude contents of Edges Pcb layer from all other layers" ) );
    LeftBoxSizer->Add( m_Exclude_Edges_Pcb, 0, wxGROW | wxALL, 1 );

    // Option d'impression du cartouche:
    if( m_Parent->m_Print_Sheet_Ref )
    {
        m_Plot_Sheet_Ref = new wxCheckBox( this, ID_PRINT_REF, _( "Print Sheet Ref" ) );

        m_Plot_Sheet_Ref->SetValue( Plot_Sheet_Ref );
        LeftBoxSizer->Add( m_Plot_Sheet_Ref, 0, wxGROW | wxALL, 1 );
    }
    else
        Plot_Sheet_Ref = false;

    // Option d'impression des pads sur toutes les couches
    m_Plot_Pads_on_Silkscreen = new wxCheckBox( this,
                                               ID_PRINT_PAD_ON_SILKSCREEN, _(
                                                   "Print Pads on Silkscreen" ) );

    m_Plot_Pads_on_Silkscreen->SetValue( PlotPadsOnSilkLayer );
    m_Plot_Pads_on_Silkscreen->SetToolTip(
        _( "Enable/disable print/plot pads on Silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Pads_on_Silkscreen, 0, wxGROW | wxALL, 1 );

    m_Force_Plot_Pads = new wxCheckBox( this, ID_FORCE_PRINT_PAD,
                                       _( "Always Print Pads" ) );

    m_Force_Plot_Pads->SetValue( Plot_Pads_All_Layers );
    m_Force_Plot_Pads->SetToolTip( _( "Force print/plot pads on ALL layers" ) );
    LeftBoxSizer->Add( m_Force_Plot_Pads, 0, wxGROW | wxALL, 1 );

    // Options d'impression des textes modules
    m_Plot_Text_Value = new wxCheckBox( this, ID_PRINT_VALUE, _( "Print Module Value" ) );

    m_Plot_Text_Value->SetValue( Sel_Texte_Valeur );
    m_Plot_Text_Value->SetToolTip(
        _( "Enable/disable print/plot module value on Silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Text_Value, 0, wxGROW | wxALL, 1 );

    m_Plot_Text_Ref = new wxCheckBox( this, ID_PRINT_REF, _( "Print Module Reference" ) );

    m_Plot_Text_Ref->SetValue( Sel_Texte_Reference );
    m_Plot_Text_Ref->SetToolTip(
        _( "Enable/disable print/plot module reference on Silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Text_Ref, 0, wxGROW | wxALL, 1 );

    m_Plot_Text_Div = new wxCheckBox( this, ID_PRINT_MODULE_TEXTS,
                                     _( "Print other Module texts" ) );

    m_Plot_Text_Div->SetValue( Sel_Texte_Divers );
    m_Plot_Text_Div->SetToolTip(
        _( "Enable/disable print/plot module field texts on Silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Text_Div, 0, wxGROW | wxALL, 1 );

    m_Plot_Invisible_Text = new wxCheckBox( this,
                                           ID_FORCE_PRINT_INVISIBLE_TEXT, _(
                                               "Force Print Invisible Texts" ) );

    m_Plot_Invisible_Text->SetValue( Sel_Texte_Invisible );
    m_Plot_Invisible_Text->SetToolTip(
        _( "Force print/plot module invisible texts on Silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Invisible_Text, 0, wxGROW | wxALL, 1 );


    static const wxString drillmsg[3] = { _( "No Drill mark" ), _( "Small mark" ), _( "Real Drill" ) };
    m_Drill_Shape_Opt = new wxRadioBox( this, ID_DRILL_SHAPE_OPT,
                                        _( "Pads Drill Opt" ), wxDefaultPosition, wxSize( -1, -1 ),
                                        3, drillmsg, 1, wxRA_SPECIFY_COLS );

    m_Drill_Shape_Opt->SetSelection( g_DrillShapeOpt );
    MidLeftBoxSizer->Add( m_Drill_Shape_Opt, 0, wxGROW | wxALL, 5 );

    static const wxString scalemsg[5] =
    { _( "Auto scale" ), _( "Scale 1" ), _( "Scale 1.5" ), _( "Scale 2" ), _( "Scale 3" ) };

    m_Scale_Opt = new wxRadioBox( this, ID_SCALE_OPT,
                                  _( "Scale Opt" ), wxDefaultPosition, wxSize( -1, -1 ),
                                  5, scalemsg, 1, wxRA_SPECIFY_COLS );

    m_Scale_Opt->SetSelection( g_PlotScaleOpt );
    MidLeftBoxSizer->Add( m_Scale_Opt, 0, wxGROW | wxALL, 5 );

    static const wxString list_opt3[3] = { _( "Line" ), _( "Filled" ), _( "Sketch" ) };
    m_PlotModeOpt = new wxRadioBox( this, ID_PLOT_MODE_OPT, _( "Plot Mode" ),
                                    wxDefaultPosition, wxDefaultSize,
                                    3, list_opt3, 1 );

    m_PlotModeOpt->SetSelection( Plot_Mode );
    MidLeftBoxSizer->Add( m_PlotModeOpt, 0, wxGROW | wxALL, 5 );

    m_PlotMirorOpt = new wxCheckBox( this, ID_MIROR_OPT,
                                    _( "Plot Mirror" ) );

    m_PlotMirorOpt->SetValue( Plot_Set_MIROIR );
    MidLeftBoxSizer->Add( m_PlotMirorOpt, 0, wxGROW | wxALL, 5 );

    m_PlotNoViaOnMaskOpt = new wxCheckBox( this, ID_MASKVIA_OPT,
                                          _( "Vias on Mask" ) );

    m_PlotNoViaOnMaskOpt->SetValue( g_DrawViaOnMaskLayer );
    m_PlotNoViaOnMaskOpt->SetToolTip(
        _( "Print/plot vias on mask layers. They are in this case not protected" ) );
    MidLeftBoxSizer->Add( m_PlotNoViaOnMaskOpt, 0, wxGROW | wxALL, 5 );

    m_HPGL_PlotCenter_Opt = new wxCheckBox( this, ID_PLOT_CENTRE_OPT,
                                           _( "Org = Centre" ) );

    m_HPGL_PlotCenter_Opt->SetValue( HPGL_Org_Centre );
    m_HPGL_PlotCenter_Opt->SetToolTip( _( "Draw origin ( 0,0 ) in sheet center" ) );
    MidLeftBoxSizer->Add( m_HPGL_PlotCenter_Opt, 0, wxGROW | wxALL, 5 );

    // Mise a jour des activations des menus:
    wxCommandEvent event;
    SetCommands( event );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/***************************************************************/
void WinEDA_PlotFrame::OnQuit( wxCommandEvent& WXUNUSED (event) )
/***************************************************************/

/* Called by the close button
 */
{
    Close( true );    // true is to force the frame to close
}


/****************************************************/
void WinEDA_PlotFrame::OnClose( wxCloseEvent& event )
/****************************************************/

/* Called when WinEDA_PlotFrame is closed
 */
{
    EndModal( 0 );
}


/*********************************************************/
void WinEDA_PlotFrame::CreateDrillFile( wxCommandEvent& event )
/*********************************************************/

/* Create the board drill file used with gerber documents
 */
{
    ( (WinEDA_PcbFrame*) m_Parent )->InstallDrillFrame( event );
}


/*********************************************************/
void WinEDA_PlotFrame::SetCommands( wxCommandEvent& event )
/*********************************************************/

/* active ou désactive les différents menus d'option selon le standard choisi
 */
{
    int format;

    static const int format_list[] = {
        PLOT_FORMAT_HPGL, PLOT_FORMAT_GERBER,
        PLOT_FORMAT_POST, PLOT_FORMAT_POST_A4
    };

    format = format_list[m_PlotFormatOpt->GetSelection()];

    switch( format  )
    {
    case PLOT_FORMAT_POST_A4:
    case PLOT_FORMAT_POST:
    default:
        m_Drill_Shape_Opt->Enable( true );
        m_PlotModeOpt->Enable( true );
        m_PlotMirorOpt->Enable( true );
        m_GerbSpotSizeMinOpt->Enable( false );
        m_Choice_Plot_Offset->Enable( false );
        m_LinesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_HPGL_PlotCenter_Opt->Enable( false );
        m_Exclude_Edges_Pcb->Enable( false );
        m_Plot_Sheet_Ref->Enable( true );
        m_Scale_Opt->Enable( true );
        m_FineAdjustXscaleOpt->Enable( true );
        m_FineAdjustYscaleOpt->Enable( true );
        m_PlotFormat = PLOT_FORMAT_POST;
        g_ForcePlotPS_On_A4 = (format == PLOT_FORMAT_POST_A4);
        m_Plot_PS_Negative->Enable( true );
        break;

    case PLOT_FORMAT_GERBER:
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->Enable( false );
        m_PlotMirorOpt->Enable( false );
        m_GerbSpotSizeMinOpt->Enable( true );
        m_Choice_Plot_Offset->Enable( true );
        m_LinesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_HPGL_PlotCenter_Opt->Enable( false );
        m_Exclude_Edges_Pcb->Enable( true );
        m_Plot_Sheet_Ref->Enable( false );
        m_Scale_Opt->Enable( false );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_PlotFormat = PLOT_FORMAT_GERBER;
        m_Plot_PS_Negative->Enable( false );
        break;

    case PLOT_FORMAT_HPGL:
        m_PlotMirorOpt->Enable( true );
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->Enable( true );
        m_GerbSpotSizeMinOpt->Enable( false );
        m_Choice_Plot_Offset->Enable( false );
        m_LinesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( true );
        m_HPGLPenSpeedOpt->Enable( true );
        m_HPGLPenOverlayOpt->Enable( true );
        m_HPGL_PlotCenter_Opt->Enable( true );
        m_Exclude_Edges_Pcb->Enable( false );
        m_Plot_Sheet_Ref->Enable( true );
        m_Scale_Opt->Enable( true );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_PlotFormat = PLOT_FORMAT_HPGL;
        m_Plot_PS_Negative->Enable( false );
        break;
    }

    format_plot = m_PlotFormat;
}


/*********************************************************/
void WinEDA_PlotFrame::SaveOptPlot( wxCommandEvent& event )
/*********************************************************/
{
    g_Exclude_Edges_Pcb = m_Exclude_Edges_Pcb->GetValue();

    if( m_Plot_Sheet_Ref )
        Plot_Sheet_Ref = m_Plot_Sheet_Ref->GetValue();

    PlotPadsOnSilkLayer  = m_Plot_Pads_on_Silkscreen->GetValue();
    Plot_Pads_All_Layers = m_Force_Plot_Pads->GetValue();

    s_PlotOriginIsAuxAxis = (m_Choice_Plot_Offset->GetSelection() == 0) ? FALSE : TRUE;

    Sel_Texte_Valeur    = m_Plot_Text_Value->GetValue();
    Sel_Texte_Reference = m_Plot_Text_Ref->GetValue();
    Sel_Texte_Divers    = m_Plot_Text_Div->GetValue();
    Sel_Texte_Invisible = m_Plot_Invisible_Text->GetValue();

    g_PlotScaleOpt  = m_Scale_Opt->GetSelection();
    g_DrillShapeOpt = m_Drill_Shape_Opt->GetSelection();
    Plot_Set_MIROIR = m_PlotMirorOpt->GetValue();
    if( Plot_Set_MIROIR )
        g_PlotOrient = PLOT_MIROIR;
    else
        g_PlotOrient = 0;
    Plot_Mode = m_PlotModeOpt->GetSelection();
    g_DrawViaOnMaskLayer = m_PlotNoViaOnMaskOpt->GetValue();
    spot_mini = m_GerbSpotSizeMinOpt->GetValue();
    g_HPGL_Pen_Diam  = m_HPGLPenSizeOpt->GetValue();
    g_HPGL_Pen_Speed = m_HPGLPenSpeedOpt->GetValue();
    g_HPGL_Pen_Recouvrement = m_HPGLPenOverlayOpt->GetValue();
    HPGL_Org_Centre  = m_HPGL_PlotCenter_Opt->GetValue();
    g_PlotLine_Width = m_LinesWidth->GetValue();

    m_XScaleAdjust = m_FineAdjustXscaleOpt->GetValue();
    m_YScaleAdjust = m_FineAdjustYscaleOpt->GetValue();

    if( m_Parent->m_Parent->m_EDA_Config )
    {
        m_Parent->m_Parent->m_EDA_Config->Write( EDGELAYER_GERBER_OPT_KEY, g_Exclude_Edges_Pcb );
        m_Parent->m_Parent->m_EDA_Config->Write( PLOT_XFINESCALE_ADJ_KEY, m_XScaleAdjust );
        m_Parent->m_Parent->m_EDA_Config->Write( PLOT_YFINESCALE_ADJ_KEY, m_YScaleAdjust );
    }

    g_Plot_PS_Negative = m_Plot_PS_Negative->GetValue();
}


/***************************************************/
void WinEDA_PlotFrame::Plot( wxCommandEvent& event )
/***************************************************/
{
    int      layer_to_plot;
    wxString FullFileName, BaseFileName;
    wxString ext;

    BOARD*   board = m_Parent->m_Pcb;

    SaveOptPlot( event );

    switch( g_PlotScaleOpt )
    {
    default:
        Scale_X = Scale_Y = 1;
        break;

    case 2:
        Scale_X = Scale_Y = 1.5;
        break;

    case 3:
        Scale_X = Scale_Y = 2;
        break;

    case 4:
        Scale_X = Scale_Y = 3;
        break;
    }

    Scale_X *= m_XScaleAdjust;
    Scale_Y *= m_YScaleAdjust;

    BaseFileName = m_Parent->GetScreen()->m_FileName;
    ChangeFileNameExt( BaseFileName, wxT( "-" ) );

    switch( m_PlotFormat )
    {
    case PLOT_FORMAT_POST:
        ext = wxT( ".ps" );
        break;

    case PLOT_FORMAT_GERBER:
        ext = wxT( ".pho" );
        break;

    case PLOT_FORMAT_HPGL:
        ext = wxT( ".plt" );
        break;
    }

    int mask = 1;
    s_SelectedLayers = 0;

    for( layer_to_plot = 0; layer_to_plot < NB_LAYERS; layer_to_plot++, mask <<= 1 )
    {
        if( m_BoxSelecLayer[layer_to_plot]->GetValue() )
        {
            s_SelectedLayers |= mask;

            // Calcul du nom du fichier
            FullFileName = BaseFileName + board->GetLayerName( layer_to_plot ) + ext;

            switch( m_PlotFormat )
            {
            case PLOT_FORMAT_POST:
                m_Parent->Genere_PS( FullFileName, layer_to_plot );
                break;

            case PLOT_FORMAT_GERBER:
                m_Parent->Genere_GERBER( FullFileName, layer_to_plot, s_PlotOriginIsAuxAxis );
                break;

            case PLOT_FORMAT_HPGL:
                m_Parent->Genere_HPGL( FullFileName, layer_to_plot );
                break;
            }
        }
    }

//	Close(true);
}


/**************************************************************/
void WinEDA_BasePcbFrame::ToPlotter( wxCommandEvent& event )
/***************************************************************/
{
    WinEDA_PlotFrame* frame = new WinEDA_PlotFrame( this );

    frame->ShowModal(); frame->Destroy();
}
