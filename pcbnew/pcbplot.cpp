/***************/
/* pcbplot.cpp */
/***************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbplot.h"
#include "worksheet.h"
#include "pcbnew_id.h"
#include "protos.h"

#define PLOT_DEFAULT_MARGE 300      // mils

/* Keywords to r/w options in config */
#define OPTKEY_EDGELAYER_GERBER   wxT( "EdgeLayerGerberOpt" )
#define OPTKEY_XFINESCALE_ADJ     wxT( "PlotXFineScaleAdj" )
#define OPTKEY_YFINESCALE_ADJ     wxT( "PlotYFineScaleAdj" )
#define OPTKEY_PADS_ON_SILKSCREEN wxT( "PlotPadsOnSilkscreen" )
#define OPTKEY_ALWAYS_PRINT_PADS  wxT( "PlotAlwaysPads" )
#define OPTKEY_OUTPUT_FORMAT      wxT( "PlotOutputFormat" )

// Define min and max reasonable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// PCB_Plot_Options constructor: set the default values for plot options:
PCB_Plot_Options::PCB_Plot_Options()
{
    Sel_Texte_Reference = true;
    Sel_Texte_Valeur    = true;
    Sel_Texte_Divers    = true;
    DrillShapeOpt = PCB_Plot_Options::SMALL_DRILL_SHAPE;
    Trace_Mode    = FILLED;
    Scale = 1.0;
    ScaleAdjX = 1.0;
    ScaleAdjY = 1.0;
    PlotScaleOpt = 1;
}

static long s_SelectedLayers = LAYER_BACK | LAYER_FRONT |
                               SILKSCREEN_LAYER_CMP | SILKSCREEN_LAYER_CU;

static bool s_PlotOriginIsAuxAxis = FALSE;


enum id_plotps {
    ID_DRILL_SHAPE_OPT = 8020,
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


/* The group of plot options - sadly global XXX */
PCB_Plot_Options g_pcb_plot_options;

/*******************************/
/* Dialog box for plot control */
/*******************************/

class WinEDA_PlotFrame : public wxDialog
{
public:
    WinEDA_BasePcbFrame*    m_Parent;
    wxButton*               m_PlotButton;           // button with initial focus
    wxCheckBox*             m_BoxSelectLayer[32];
    wxRadioBox*             m_PlotFormatOpt;
    wxRadioBox*             m_Choice_Plot_Offset;
    wxRadioBox*             m_Drill_Shape_Opt;
    wxRadioBox*             m_Scale_Opt;
    wxRadioBox*             m_PlotModeOpt;
    wxCheckBox*             m_PlotMirorOpt;
    wxCheckBox*             m_PlotNoViaOnMaskOpt;
    wxCheckBox*             m_Exclude_Edges_Pcb;
    wxCheckBox*             m_Plot_Sheet_Ref;
    wxCheckBox*             m_Plot_Invisible_Text;
    wxCheckBox*             m_Plot_Text_Value;
    wxCheckBox*             m_Plot_Text_Ref;
    wxCheckBox*             m_Plot_Text_Div;
    wxCheckBox*             m_Plot_Pads_on_Silkscreen;
    wxCheckBox*             m_Force_Plot_Pads;
    wxCheckBox*             m_Plot_PS_Negative;
    WinEDA_ValueCtrl*       m_LinesWidth;
    WinEDA_ValueCtrl*       m_HPGLPenSizeOpt;
    WinEDA_ValueCtrl*       m_HPGLPenSpeedOpt;
    WinEDA_ValueCtrl*       m_HPGLPenOverlayOpt;
    wxStaticBox*            m_HPGL_OptionsBox;

    WinEDA_DFloatValueCtrl* m_FineAdjustXscaleOpt;
    WinEDA_DFloatValueCtrl* m_FineAdjustYscaleOpt;
    double                  m_XScaleAdjust;
    double                  m_YScaleAdjust;

    bool useA4()
    {
        return m_PlotFormatOpt->GetSelection() == 3;
    }


    /**
     * Function getFormat
     * returns one of the values from the PlotFormat enum.  If the 4th
     * radio button is selected, map this back to postscript.
     */
    PlotFormat getFormat()
    {
        int radioNdx = m_PlotFormatOpt->GetSelection();

        // change the A4 to the simple postscript, according to the
        // PlotFormat enum
    switch (radioNdx)
    {
    case 3:
            radioNdx = PLOT_FORMAT_POST;
        break;
    case 4:
        radioNdx = PLOT_FORMAT_DXF;
        break;
    }

        return PlotFormat( radioNdx );
    }


public:
    WinEDA_PlotFrame( WinEDA_BasePcbFrame* parent );
private:
    void OnInitDialog( wxInitDialogEvent& event );
    void Plot( wxCommandEvent& event );
    void OnQuit( wxCommandEvent& event );
    void OnClose( wxCloseEvent& event );
    void SetCommands( wxCommandEvent& event );
    void OnSetScaleOpt( wxCommandEvent& event );
    void SaveOptPlot( wxCommandEvent& event );
    void CreateDrillFile( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_PlotFrame, wxDialog )
    EVT_INIT_DIALOG( WinEDA_PlotFrame::OnInitDialog )
    EVT_CLOSE( WinEDA_PlotFrame::OnClose )
    EVT_BUTTON( wxID_CANCEL, WinEDA_PlotFrame::OnQuit )
    EVT_BUTTON( ID_EXEC_PLOT, WinEDA_PlotFrame::Plot )
    EVT_BUTTON( ID_SAVE_OPT_PLOT, WinEDA_PlotFrame::SaveOptPlot )
    EVT_BUTTON( ID_CREATE_DRILL_FILE, WinEDA_PlotFrame::CreateDrillFile )
    EVT_RADIOBOX( ID_SEL_PLOT_FORMAT, WinEDA_PlotFrame::SetCommands )
    EVT_RADIOBOX( ID_SCALE_OPT, WinEDA_PlotFrame::OnSetScaleOpt )
END_EVENT_TABLE()


const int UNITS_MILS = 1000;


WinEDA_PlotFrame::WinEDA_PlotFrame( WinEDA_BasePcbFrame* parent ) :
    wxDialog( parent, -1, _( "Plot" ), wxPoint( -1, -1 ), wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE )
{
    m_Parent = parent;
    Centre();
}


void WinEDA_PlotFrame::OnInitDialog( wxInitDialogEvent& event )
{
    wxButton* button;

    BOARD*    board = m_Parent->GetBoard();

    wxConfig* config = wxGetApp().m_EDA_Config;

    m_Plot_Sheet_Ref = NULL;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );

    SetSizer( MainBoxSizer );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* MidRightBoxSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* MidLeftBoxSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* LeftBoxSizer = new wxBoxSizer( wxVERTICAL );

    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( MidLeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( MidRightBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxGROW | wxALL, 5 );

    wxBoxSizer* LayersBoxSizer = new wxBoxSizer( wxHORIZONTAL );

    LeftBoxSizer->Add( LayersBoxSizer, 0, wxGROW | wxALL, 5 );

    static const wxString fmtmsg[5] =
    {
        wxT( "HPGL" ),
        wxT( "Gerber" ),
        wxT( "Postscript" ),
        wxT( "Postscript A4" ),
        wxT( "DXF Export" )
    };

    m_PlotFormatOpt = new wxRadioBox( this, ID_SEL_PLOT_FORMAT,
                                      _( "Plot Format" ), wxDefaultPosition,
                                      wxSize( -1, -1 ),
                                      5, fmtmsg, 1, wxRA_SPECIFY_COLS );
    MidRightBoxSizer->Add( m_PlotFormatOpt, 0, wxGROW | wxALL, 5 );

    if( config )
    {
        config->Read( OPTKEY_OUTPUT_FORMAT, &g_pcb_plot_options.PlotFormat );
        config->Read( OPTKEY_PLOT_LINEWIDTH_VALUE,
                      &g_pcb_plot_options.PlotLine_Width );
    }

    m_PlotFormatOpt->SetSelection( g_pcb_plot_options.PlotFormat );

    m_HPGL_OptionsBox = new wxStaticBox( this, wxID_ANY, _( "HPGL Options:" ) );
    wxStaticBoxSizer* HPGL_OptionsBoxSizer =
        new wxStaticBoxSizer( m_HPGL_OptionsBox, wxVERTICAL );
    MidRightBoxSizer->Add( HPGL_OptionsBoxSizer, 0, wxGROW | wxALL, 5 );

    m_HPGLPenSizeOpt = new WinEDA_ValueCtrl( this, _( "Pen size" ),
                                             g_pcb_plot_options.HPGL_Pen_Diam,
                                             g_UnitMetric,
                                             HPGL_OptionsBoxSizer,
                                             UNITS_MILS );

    // Set units to cm for standard HPGL pen speed.
    m_HPGLPenSpeedOpt = new WinEDA_ValueCtrl( this, _( "Pen Speed (cm/s)" ),
                                              g_pcb_plot_options.HPGL_Pen_Speed, CENTIMETRE,
                                              HPGL_OptionsBoxSizer, 1 );

    m_HPGLPenSpeedOpt->SetToolTip( _( "Set pen speed in cm/s" ) );

    m_HPGLPenOverlayOpt = new WinEDA_ValueCtrl( this, _( "Pen ovr" ),
                                                g_pcb_plot_options.HPGL_Pen_Recouvrement,
                                                g_UnitMetric,
                                                HPGL_OptionsBoxSizer,
                                                UNITS_MILS );

    m_HPGLPenOverlayOpt->SetToolTip( _( "Set plot overlay for filling" ) );

    m_LinesWidth = new WinEDA_ValueCtrl( this, _( "Line width" ),
                                         g_pcb_plot_options.PlotLine_Width,
                                         g_UnitMetric,
                                         MidRightBoxSizer,
                                         PCB_INTERNAL_UNIT );

    m_LinesWidth->SetToolTip( _( "Set lines width used to plot in sketch \
mode and plot pads outlines on silk screen layers" ) );

    // Create the right column commands
    static const wxString choice_plot_offset_msg[] =
    { _( "Absolute" ), _( "Auxiliary axis" ) };

    m_Choice_Plot_Offset = new wxRadioBox( this, ID_SEL_PLOT_OFFSET_OPTION,
                                           _( "Plot Origin" ),
                                           wxDefaultPosition,
                                           wxSize( -1, -1 ),
                                           2, choice_plot_offset_msg, 1,
                                           wxRA_SPECIFY_COLS );

    if( s_PlotOriginIsAuxAxis )
        m_Choice_Plot_Offset->SetSelection( 1 );
    RightBoxSizer->Add( m_Choice_Plot_Offset, 0, wxGROW | wxALL, 5 );
    /* Add a spacer for a better look */
    RightBoxSizer->Add( 5, 5, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 20 );

    // Create scale adjust option
    m_XScaleAdjust = m_YScaleAdjust = 1.0;

    if( config )
    {
        config->Read( OPTKEY_EDGELAYER_GERBER, &g_pcb_plot_options.Exclude_Edges_Pcb );
        config->Read( OPTKEY_XFINESCALE_ADJ, &m_XScaleAdjust );
        config->Read( OPTKEY_YFINESCALE_ADJ, &m_YScaleAdjust );
    }

    // Test for a reasonable scale value. Set to 1 if problem
    if( m_XScaleAdjust < MIN_SCALE || m_YScaleAdjust < MIN_SCALE
        || m_XScaleAdjust > MAX_SCALE || m_YScaleAdjust > MAX_SCALE )
        m_XScaleAdjust = m_YScaleAdjust = 1.0;

    m_FineAdjustXscaleOpt = new WinEDA_DFloatValueCtrl( this,
                                                        _( "X scale adjust" ),
                                                        m_XScaleAdjust,
                                                        RightBoxSizer );
    m_FineAdjustXscaleOpt->SetToolTip( _( "Set X scale adjust for exact \
scale plotting" ) );

    m_FineAdjustYscaleOpt = new WinEDA_DFloatValueCtrl( this,
                                                        _( "Y scale adjust" ),
                                                        m_YScaleAdjust,
                                                        RightBoxSizer );
    m_FineAdjustYscaleOpt->SetToolTip( _( "Set Y scale adjust for exact \
scale plotting" ) );

    m_Plot_PS_Negative = new wxCheckBox( this, -1, _( "Plot negative" ) );
    m_Plot_PS_Negative->SetValue( g_pcb_plot_options.Plot_PS_Negative );
    RightBoxSizer->Add( m_Plot_PS_Negative, 0, wxGROW | wxALL, 5 );


    /* Create the command buttons */
    m_PlotButton = new wxButton( this, ID_EXEC_PLOT, _( "Plot" ) );
    RightBoxSizer->Add( m_PlotButton, 0, wxGROW | wxALL, 5 );

    button = new wxButton( this, ID_SAVE_OPT_PLOT, _( "Save Options" ) );
    RightBoxSizer->Add( button, 0, wxGROW | wxALL, 5 );

    button = new wxButton( this, ID_CREATE_DRILL_FILE,
                           _( "Generate drill file" ) );
    RightBoxSizer->Add( button, 0, wxGROW | wxALL, 5 );

    button = new wxButton( this, wxID_CANCEL, _( "Close" ) );
    RightBoxSizer->Add( button, 0, wxGROW | wxALL, 5 );

    // Create layer list.
    wxBoxSizer* OneColumnLayerBoxSizer = new wxBoxSizer( wxVERTICAL );
    LayersBoxSizer->Add( OneColumnLayerBoxSizer, 0, wxGROW | wxALL, 5 );

    int mask = 1;

    for( int layer = 0; layer<NB_LAYERS; layer++, mask <<= 1 )
    {
        if( layer == 16 )
        {
            OneColumnLayerBoxSizer = new wxBoxSizer( wxVERTICAL );
            LayersBoxSizer->Add( OneColumnLayerBoxSizer, 0, wxGROW | wxALL, 5 );
        }

        m_BoxSelectLayer[layer] =
            new wxCheckBox( this, -1, board->GetLayerName( layer ) );

        if( mask & s_SelectedLayers )
            m_BoxSelectLayer[layer]->SetValue( true );

        OneColumnLayerBoxSizer->Add( m_BoxSelectLayer[layer],
                                     0, wxGROW | wxALL, 1 );
    }

    if( config )
    {
        wxString layerKey;

        for( int layer = 0; layer < NB_LAYERS; ++layer )
        {
            bool option;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            if( config->Read( layerKey, &option ) )
                m_BoxSelectLayer[layer]->SetValue( option );
        }
    }

    // Option for excluding contents of "Edges Pcb" layer

    m_Exclude_Edges_Pcb = new wxCheckBox( this,
                                          ID_EXCLUDE_EDGES_PCB,
                                          _( "Exclude pcb edge layer" ) );

    m_Exclude_Edges_Pcb->SetValue( g_pcb_plot_options.Exclude_Edges_Pcb );
    m_Exclude_Edges_Pcb->SetToolTip(
        _( "Exclude contents of the pcb edge layer from all other layers" ) );
    LeftBoxSizer->Add( m_Exclude_Edges_Pcb, 0, wxGROW | wxALL, 1 );

    // Option to plot page references:
    if( m_Parent->m_Print_Sheet_Ref )
    {
        m_Plot_Sheet_Ref = new wxCheckBox( this, ID_PRINT_REF,
                                           _( "Print sheet reference" ) );

        m_Plot_Sheet_Ref->SetValue( g_pcb_plot_options.Plot_Frame_Ref );
        LeftBoxSizer->Add( m_Plot_Sheet_Ref, 0, wxGROW | wxALL, 1 );
    }
    else
        g_pcb_plot_options.Plot_Frame_Ref = false;

    // Option to plot pads on silkscreen layers or all layers
    m_Plot_Pads_on_Silkscreen = new wxCheckBox( this,
                                                ID_PRINT_PAD_ON_SILKSCREEN,
                                                _( "Print pads on silkscreen" ) );

    if( config )
        config->Read( OPTKEY_PADS_ON_SILKSCREEN,
                      &g_pcb_plot_options.PlotPadsOnSilkLayer );

    m_Plot_Pads_on_Silkscreen->SetValue( &g_pcb_plot_options.PlotPadsOnSilkLayer );
    m_Plot_Pads_on_Silkscreen->SetToolTip(
        _( "Enable/disable print/plot pads on silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Pads_on_Silkscreen, 0, wxGROW | wxALL, 1 );

    m_Force_Plot_Pads = new wxCheckBox( this, ID_FORCE_PRINT_PAD,
                                        _( "Always print pads" ) );
    if( config )
        config->Read( OPTKEY_ALWAYS_PRINT_PADS,
                      &g_pcb_plot_options.Plot_Pads_All_Layers );

    m_Force_Plot_Pads->SetValue( g_pcb_plot_options.Plot_Pads_All_Layers );
    m_Force_Plot_Pads->SetToolTip( _( "Force print/plot pads on ALL layers" ) );
    LeftBoxSizer->Add( m_Force_Plot_Pads, 0, wxGROW | wxALL, 1 );

    // Options to plot texts on footprints
    m_Plot_Text_Value = new wxCheckBox( this, ID_PRINT_VALUE,
                                        _( "Print module value" ) );

    m_Plot_Text_Value->SetValue( g_pcb_plot_options.Sel_Texte_Valeur );
    m_Plot_Text_Value->SetToolTip(
        _( "Enable/disable print/plot module value on silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Text_Value, 0, wxGROW | wxALL, 1 );

    m_Plot_Text_Ref = new wxCheckBox( this, ID_PRINT_REF,
                                      _( "Print module reference" ) );

    m_Plot_Text_Ref->SetValue( g_pcb_plot_options.Sel_Texte_Reference );
    m_Plot_Text_Ref->SetToolTip(
        _( "Enable/disable print/plot module reference on silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Text_Ref, 0, wxGROW | wxALL, 1 );

    m_Plot_Text_Div = new wxCheckBox( this, ID_PRINT_MODULE_TEXTS,
                                      _( "Print other module texts" ) );

    m_Plot_Text_Div->SetValue( g_pcb_plot_options.Sel_Texte_Divers );
    m_Plot_Text_Div->SetToolTip(
        _( "Enable/disable print/plot module field texts on silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Text_Div, 0, wxGROW | wxALL, 1 );

    m_Plot_Invisible_Text = new wxCheckBox( this,
                                            ID_FORCE_PRINT_INVISIBLE_TEXT,
                                            _( "Force print invisible texts" ) );

    m_Plot_Invisible_Text->SetValue( g_pcb_plot_options.Sel_Texte_Invisible );
    m_Plot_Invisible_Text->SetToolTip(
        _( "Force print/plot module invisible texts on silkscreen layers" ) );
    LeftBoxSizer->Add( m_Plot_Invisible_Text, 0, wxGROW | wxALL, 1 );


    static const wxString drillmsg[3] = {
        _( "No drill mark" ),
        _( "Small mark" ),
        _( "Real drill" )
    };

    m_Drill_Shape_Opt = new wxRadioBox( this, ID_DRILL_SHAPE_OPT,
                                        _( "Pads Drill Opt" ),
                                        wxDefaultPosition, wxSize( -1, -1 ),
                                        3, drillmsg, 1, wxRA_SPECIFY_COLS );

    m_Drill_Shape_Opt->SetSelection( g_pcb_plot_options.DrillShapeOpt );
    MidLeftBoxSizer->Add( m_Drill_Shape_Opt, 0, wxGROW | wxALL, 5 );

    static const wxString scalemsg[5] =
    {
        _( "Auto scale" ),
        _( "Scale 1" ),
        _( "Scale 1.5" ),
        _( "Scale 2" ),
        _( "Scale 3" )
    };

    m_Scale_Opt = new wxRadioBox( this, ID_SCALE_OPT,
                                  _( "Scale Opt" ), wxDefaultPosition,
                                  wxSize( -1, -1 ),
                                  5, scalemsg, 1, wxRA_SPECIFY_COLS );

    m_Scale_Opt->SetSelection( g_pcb_plot_options.PlotScaleOpt );
    MidLeftBoxSizer->Add( m_Scale_Opt, 0, wxGROW | wxALL, 5 );

    static const wxString list_opt3[3] = { _( "Line" ), _( "Filled" ),
                                           _( "Sketch" ) };

    m_PlotModeOpt = new wxRadioBox( this, ID_PLOT_MODE_OPT, _( "Plot Mode" ),
                                    wxDefaultPosition, wxDefaultSize,
                                    3, list_opt3, 1 );

    m_PlotModeOpt->SetSelection( g_pcb_plot_options.Trace_Mode );
    MidLeftBoxSizer->Add( m_PlotModeOpt, 0, wxGROW | wxALL, 5 );

    m_PlotMirorOpt = new wxCheckBox( this, ID_MIROR_OPT,
                                     _( "Plot mirror" ) );

    m_PlotMirorOpt->SetValue( g_pcb_plot_options.Plot_Set_MIROIR );
    MidLeftBoxSizer->Add( m_PlotMirorOpt, 0, wxGROW | wxALL, 5 );

    m_PlotNoViaOnMaskOpt = new wxCheckBox( this, ID_MASKVIA_OPT,
                                           _( "Vias on mask" ) );

    m_PlotNoViaOnMaskOpt->SetValue( g_pcb_plot_options.DrawViaOnMaskLayer );
    m_PlotNoViaOnMaskOpt->SetToolTip(
        _( "Print/plot vias on mask layers. They are in this case not protected" ) );
    MidLeftBoxSizer->Add( m_PlotNoViaOnMaskOpt, 0, wxGROW | wxALL, 5 );

    // Update options values:
    wxCommandEvent cmd_event;
    SetCommands( cmd_event );
    OnSetScaleOpt( cmd_event );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    // without this line, the ESC key does not work
    SetFocus();
}


void WinEDA_PlotFrame::OnQuit( wxCommandEvent& WXUNUSED( event ) )
{
    Close( true );    // true is to force the frame to close
}


void WinEDA_PlotFrame::OnClose( wxCloseEvent& event )
{
    EndModal( 0 );
}


void WinEDA_PlotFrame::CreateDrillFile( wxCommandEvent& event )
{
    ( (WinEDA_PcbFrame*) m_Parent )->InstallDrillFrame( event );
}

void WinEDA_PlotFrame::OnSetScaleOpt( wxCommandEvent& event )
{
    /* Disable sheet reference for scale != 1:1 */
    bool scale1 = ( m_Scale_Opt->GetSelection() == 1 );
    m_Plot_Sheet_Ref->Enable( scale1 );

    if ( !scale1 )
        m_Plot_Sheet_Ref->SetValue( false );
}

void WinEDA_PlotFrame::SetCommands( wxCommandEvent& event )
{
    int format = getFormat();

    switch( format  )
    {
    case PLOT_FORMAT_POST:
    default:
        m_Drill_Shape_Opt->Enable( true );
        m_PlotModeOpt->Enable( true );
        m_PlotMirorOpt->Enable( true );
        m_Choice_Plot_Offset->Enable( false );
        m_LinesWidth->Enable( true );
        m_HPGL_OptionsBox->Enable( false );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_Exclude_Edges_Pcb->SetValue( false );
        m_Exclude_Edges_Pcb->Enable( false );
        m_Scale_Opt->Enable( true );
        m_FineAdjustXscaleOpt->Enable( true );
        m_FineAdjustYscaleOpt->Enable( true );
        m_Plot_PS_Negative->Enable( true );
        break;

    case PLOT_FORMAT_GERBER:
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->SetSelection( 1 );
        m_PlotModeOpt->Enable( false );
        m_PlotMirorOpt->SetValue( false );
        m_PlotMirorOpt->Enable( false );
        m_Choice_Plot_Offset->Enable( true );
        m_LinesWidth->Enable( true );
        m_HPGL_OptionsBox->Enable( false );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_Exclude_Edges_Pcb->Enable( true );
        m_Scale_Opt->SetSelection( 1 );
        m_Scale_Opt->Enable( false );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_Plot_PS_Negative->SetValue( false );
        m_Plot_PS_Negative->Enable( false );
        break;

    case PLOT_FORMAT_HPGL:
        m_PlotMirorOpt->Enable( true );
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->Enable( true );
        m_Choice_Plot_Offset->Enable( false );
        m_LinesWidth->Enable( false );
        m_HPGL_OptionsBox->Enable( true );
        m_HPGLPenSizeOpt->Enable( true );
        m_HPGLPenSpeedOpt->Enable( true );
        m_HPGLPenOverlayOpt->Enable( true );
        m_Exclude_Edges_Pcb->SetValue( false );
        m_Exclude_Edges_Pcb->Enable( false );
        m_Scale_Opt->Enable( true );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_Plot_PS_Negative->SetValue( false );
        m_Plot_PS_Negative->Enable( false );
        break;

    case PLOT_FORMAT_DXF:
        m_PlotMirorOpt->Enable( false );
        m_PlotMirorOpt->SetValue( false );
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->Enable( true );
        m_Choice_Plot_Offset->Enable( false );
        m_LinesWidth->Enable( false );
        m_HPGL_OptionsBox->Enable( false );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_Exclude_Edges_Pcb->SetValue( false );
        m_Exclude_Edges_Pcb->Enable( false );
        m_Scale_Opt->Enable( false );
        m_Scale_Opt->SetSelection( 1 );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_Plot_PS_Negative->SetValue( false );
        m_Plot_PS_Negative->Enable( false );
        break;
    }

    g_pcb_plot_options.PlotFormat = format;
}


void WinEDA_PlotFrame::SaveOptPlot( wxCommandEvent& event )
{
    g_pcb_plot_options.Exclude_Edges_Pcb = m_Exclude_Edges_Pcb->GetValue();

    if( m_Plot_Sheet_Ref )
        g_pcb_plot_options.Plot_Frame_Ref = m_Plot_Sheet_Ref->GetValue();

    g_pcb_plot_options.PlotPadsOnSilkLayer  = m_Plot_Pads_on_Silkscreen->GetValue();
    g_pcb_plot_options.Plot_Pads_All_Layers = m_Force_Plot_Pads->GetValue();

    s_PlotOriginIsAuxAxis =
        (m_Choice_Plot_Offset->GetSelection() == 0) ? FALSE : TRUE;

    g_pcb_plot_options.Sel_Texte_Valeur    = m_Plot_Text_Value->GetValue();
    g_pcb_plot_options.Sel_Texte_Reference = m_Plot_Text_Ref->GetValue();
    g_pcb_plot_options.Sel_Texte_Divers    = m_Plot_Text_Div->GetValue();
    g_pcb_plot_options.Sel_Texte_Invisible = m_Plot_Invisible_Text->GetValue();

    g_pcb_plot_options.PlotScaleOpt  = m_Scale_Opt->GetSelection();
    g_pcb_plot_options.DrillShapeOpt =
        (PCB_Plot_Options::DrillShapeOptT) m_Drill_Shape_Opt->GetSelection();
    g_pcb_plot_options.Plot_Set_MIROIR = m_PlotMirorOpt->GetValue();
    if( g_pcb_plot_options.Plot_Set_MIROIR )
        g_pcb_plot_options.PlotOrient = PLOT_MIROIR;
    else
        g_pcb_plot_options.PlotOrient = 0;
    g_pcb_plot_options.Trace_Mode = (GRTraceMode)m_PlotModeOpt->GetSelection();
    g_pcb_plot_options.DrawViaOnMaskLayer = m_PlotNoViaOnMaskOpt->GetValue();

    g_pcb_plot_options.HPGL_Pen_Diam  = m_HPGLPenSizeOpt->GetValue();
    g_pcb_plot_options.HPGL_Pen_Speed = m_HPGLPenSpeedOpt->GetValue();
    g_pcb_plot_options.HPGL_Pen_Recouvrement = m_HPGLPenOverlayOpt->GetValue();
    g_pcb_plot_options.PlotLine_Width = m_LinesWidth->GetValue();

    m_XScaleAdjust = m_FineAdjustXscaleOpt->GetValue();
    m_YScaleAdjust = m_FineAdjustYscaleOpt->GetValue();

    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config )
    {
        config->Write( OPTKEY_EDGELAYER_GERBER,
                       g_pcb_plot_options.Exclude_Edges_Pcb );
        config->Write( OPTKEY_XFINESCALE_ADJ, m_XScaleAdjust );
        config->Write( OPTKEY_YFINESCALE_ADJ, m_YScaleAdjust );
        config->Write( OPTKEY_PADS_ON_SILKSCREEN,
                       g_pcb_plot_options.PlotPadsOnSilkLayer );
        config->Write( OPTKEY_ALWAYS_PRINT_PADS,
                       g_pcb_plot_options.Plot_Pads_All_Layers );

        int formatNdx = m_PlotFormatOpt->GetSelection();
        config->Write( OPTKEY_OUTPUT_FORMAT, formatNdx );
        config->Write( OPTKEY_PLOT_LINEWIDTH_VALUE,
                       g_pcb_plot_options.PlotLine_Width );

        wxString layerKey;
        for( int layer = 0;  layer<NB_LAYERS;  ++layer )
        {
            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            config->Write( layerKey, m_BoxSelectLayer[layer]->IsChecked() );
        }
    }

    g_pcb_plot_options.Plot_PS_Negative = m_Plot_PS_Negative->GetValue();
}


void WinEDA_PlotFrame::Plot( wxCommandEvent& event )
{
    int        layer_to_plot;
    wxFileName fn;
    wxString   ext;
    wxString   wildcard;

    BOARD*     board = m_Parent->GetBoard();

    SaveOptPlot( event );

    switch( g_pcb_plot_options.PlotScaleOpt )
    {
    default:
        g_pcb_plot_options.Scale = 1;
        break;

    case 2:
        g_pcb_plot_options.Scale = 1.5;
        break;

    case 3:
        g_pcb_plot_options.Scale = 2;
        break;

    case 4:
        g_pcb_plot_options.Scale = 3;
        break;
    }

    /* If the scale factor edit controls are disabled or the scale value
     * is 0, don't adjust the base scale factor.   This fixes a bug when
     * the default scale adjust is initialized to 0 and saved in program
     * settings resulting in a divide by zero fault.
     */
    if( m_FineAdjustXscaleOpt->m_ValueCtrl->IsEnabled()
        && m_XScaleAdjust != 0.0 )
        g_pcb_plot_options.ScaleAdjX = m_XScaleAdjust;
    if( m_FineAdjustYscaleOpt->m_ValueCtrl->IsEnabled()
        && m_YScaleAdjust != 0.0 )
        g_pcb_plot_options.ScaleAdjY = m_YScaleAdjust;

    int format = getFormat();

    switch( format )
    {
    case PLOT_FORMAT_POST:
        ext = wxT( "ps" );
        wildcard = _( "Adobe post script files (.ps)|*.ps" );
        break;

    case PLOT_FORMAT_GERBER:
        g_pcb_plot_options.Scale = 1.0; // No scale option allowed in gerber format
        ext      = wxT( "pho" );
        wildcard = _( "GERBER photo plot files (.pho)|*.pho" );
        break;

    case PLOT_FORMAT_HPGL:
        ext = wxT( "plt" );
        wildcard = _( "HPGL plot files (.plt)|*.plt" );
        break;

    case PLOT_FORMAT_DXF:
        g_pcb_plot_options.Scale = 1.0;
        ext = wxT( "dxf" );
        wildcard = _( "DXF files (.dxf)|*.dxf" );
        break;
    }

    // Test for a reasonable scale value
    if( g_pcb_plot_options.Scale < MIN_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very small value" ) );
    if( g_pcb_plot_options.Scale > MAX_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very large value" ) );

    int mask = 1;
    s_SelectedLayers = 0;

    for( layer_to_plot = 0;
         layer_to_plot < NB_LAYERS;
         layer_to_plot++, mask <<= 1 )
    {
        if( m_BoxSelectLayer[layer_to_plot]->GetValue() )
        {
            s_SelectedLayers |= mask;

            fn = m_Parent->GetScreen()->m_FileName;

            // Create file name.
            fn.SetName( fn.GetName() + wxT( "-" ) +
                        board->GetLayerName( layer_to_plot ) );
            fn.SetExt( ext );

            switch( format )
            {
            case PLOT_FORMAT_POST:
                m_Parent->Genere_PS( fn.GetFullPath(), layer_to_plot, useA4(),
                                     g_pcb_plot_options.Trace_Mode );
                break;

            case PLOT_FORMAT_GERBER:
                m_Parent->Genere_GERBER( fn.GetFullPath(), layer_to_plot,
                                         s_PlotOriginIsAuxAxis,
                     g_pcb_plot_options.Trace_Mode );
                break;

            case PLOT_FORMAT_HPGL:
                m_Parent->Genere_HPGL( fn.GetFullPath(), layer_to_plot,
                                       g_pcb_plot_options.Trace_Mode );
                break;

            case PLOT_FORMAT_DXF:
                m_Parent->Genere_DXF( fn.GetFullPath(), layer_to_plot,
                                      g_pcb_plot_options.Trace_Mode );
                break;
            }
        }
    }

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in pcbnew:
    if( s_SelectedLayers == 0 )
        DisplayError( this, _( "No layer selected" ) );
}


void WinEDA_BasePcbFrame::ToPlotter( wxCommandEvent& event )
{
    WinEDA_PlotFrame* frame = new WinEDA_PlotFrame( this );

    frame->ShowModal();
    frame->Destroy();
}
