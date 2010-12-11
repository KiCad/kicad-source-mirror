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
#include "pcbstruct.h"
#include "class_board_design_settings.h"
#include "dialog_plot_base.h"

#define PLOT_DEFAULT_MARGE 300      // mils

/* Keywords to r/w options in m_Config */
#define OPTKEY_GERBER_EXTENSIONS wxT( "GerberOptUseLayersExt" )
#define OPTKEY_XFINESCALE_ADJ    wxT( "PlotXFineScaleAdj" )
#define OPTKEY_YFINESCALE_ADJ    wxT( "PlotYFineScaleAdj" )

// Define min and max reasonable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// PCB_PLOT_PARAMS constructor: set the default values for plot options:
PCB_PLOT_PARAMS::PCB_PLOT_PARAMS()
{
    m_SubtractMaskFromSilk = false;
    m_PlotReference = true;
    m_PlotValue     = true;
    m_PlotTextOther = true;
    m_DrillShapeOpt = PCB_PLOT_PARAMS::SMALL_DRILL_SHAPE;
    Trace_Mode  = FILLED;
    m_PlotScale = 1.0;
    m_AutoScale = false;
    m_FineScaleAdjustX = 1.0;
    m_FineScaleAdjustY = 1.0;
    outputDirectory    = wxT( "" );
}


static long s_SelectedLayers = LAYER_BACK | LAYER_FRONT |
                               SILKSCREEN_LAYER_FRONT | SILKSCREEN_LAYER_BACK;

static bool s_PlotOriginIsAuxAxis = FALSE;


/* The group of plot options - sadly global XXX */
PCB_PLOT_PARAMS g_PcbPlotOptions;
extern int       g_DrawDefaultLineThickness;


/*******************************/
/* Dialog box for plot control */
/*******************************/

class DIALOG_PLOT : public DIALOG_PLOT_BASE
{
public:
    WinEDA_PcbFrame* m_Parent;
    wxConfig*        m_Config;
    wxCheckBox*      m_BoxSelectLayer[LAYER_COUNT];     // wxCheckBox list to select/deselec layers to plot
    double           m_XScaleAdjust;
    double           m_YScaleAdjust;
    static           int m_dlgPlotScaleOpt;         // Static to remember last selection


    bool useA4()
    {
        return m_PlotFormatOpt->GetSelection() == 3;
    }


    /**
     * Function getFormat
     * returns one of the values from the m_PlotFormat enum.  If the 4th
     * radio button is selected, map this back to postscript.
     */
    PlotFormat getFormat()
    {
        int radioNdx = m_PlotFormatOpt->GetSelection();

        // change the A4 to the simple postscript, according to the
        // m_PlotFormat enum
        switch( radioNdx )
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


public: DIALOG_PLOT( WinEDA_PcbFrame* parent );
private:
    void Init_Dialog();
    void Plot( wxCommandEvent& event );
    void OnQuit( wxCommandEvent& event );
    void OnClose( wxCloseEvent& event );
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event );
    void SetPlotFormat( wxCommandEvent& event );
    void OnSetScaleOpt( wxCommandEvent& event );
    void SaveOptPlot( wxCommandEvent& event );
    void CreateDrillFile( wxCommandEvent& event );
};

int DIALOG_PLOT::m_dlgPlotScaleOpt = 1;

const int UNITS_MILS = 1000;


DIALOG_PLOT::DIALOG_PLOT( WinEDA_PcbFrame* parent ) :
    DIALOG_PLOT_BASE( parent )
{
    m_Parent = parent;
    m_Config = wxGetApp().m_EDA_Config;

    Init_Dialog();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_PLOT::Init_Dialog()
{
    wxString   msg;
    wxFileName fileName;

    BOARD*     board = m_Parent->GetBoard();

    m_Config->Read( OPTKEY_XFINESCALE_ADJ, &m_XScaleAdjust );
    m_Config->Read( OPTKEY_YFINESCALE_ADJ, &m_YScaleAdjust );

    m_PlotFormatOpt->SetSelection( g_PcbPlotOptions.m_PlotFormat );
    g_PcbPlotOptions.m_PlotLineWidth = g_DrawDefaultLineThickness;


    // Set units and value for HPGL pen speed.
    AddUnitSymbol( *m_textPenSize, g_UserUnit );
    msg = ReturnStringFromValue( g_UserUnit, g_PcbPlotOptions.m_HPGLPenDiam, UNITS_MILS );
    m_HPGLPenSizeOpt->AppendText( msg );

    // Set units to cm for standard HPGL pen speed.
    msg = ReturnStringFromValue( UNSCALED_UNITS, g_PcbPlotOptions.m_HPGLPenSpeed, 1 );
    m_HPGLPenSpeedOpt->AppendText( msg );

    // Set units and value for HPGL pen overlay.
    AddUnitSymbol( *m_textPenOvr, g_UserUnit );
    msg = ReturnStringFromValue( g_UserUnit,
                                 g_PcbPlotOptions.m_HPGLPenOvr,
                                 UNITS_MILS );
    m_HPGLPenOverlayOpt->AppendText( msg );

    msg = ReturnStringFromValue( g_UserUnit,
                                 g_PcbPlotOptions.m_PlotLineWidth,
                                 PCB_INTERNAL_UNIT );
    m_LinesWidth->AppendText( msg );

    if( s_PlotOriginIsAuxAxis )
        m_Choice_Plot_Offset->SetSelection( 1 );

    // Create scale adjust option
    m_XScaleAdjust = m_YScaleAdjust = 1.0;

    // Test for a reasonable scale value. Set to 1 if problem
    if( m_XScaleAdjust < MIN_SCALE || m_YScaleAdjust < MIN_SCALE
        || m_XScaleAdjust > MAX_SCALE || m_YScaleAdjust > MAX_SCALE )
        m_XScaleAdjust = m_YScaleAdjust = 1.0;

    msg.Printf( wxT( "%f" ), m_XScaleAdjust );
    m_FineAdjustXscaleOpt->AppendText( msg );

    msg.Printf( wxT( "%f" ), m_YScaleAdjust );
    m_FineAdjustYscaleOpt->AppendText( msg );

    m_plotPSNegativeOpt->SetValue( g_PcbPlotOptions.m_PlotPSNegative );


    // Create layer list.
    int      layer;
    wxString layerKey;
    for( layer = 0; layer < NB_LAYERS; ++layer )
    {
        if( !board->IsLayerEnabled( layer ) )
            m_BoxSelectLayer[layer] = NULL;
        else
            m_BoxSelectLayer[layer] =
                new wxCheckBox( this, -1, board->GetLayerName( layer ) );
    }

    // Add wxCheckBoxes in layers lists dialog
    //  List layers in same order than in setup layers dialog
    // (Front or Top to Back or Bottom)
    DECLARE_LAYERS_ORDER_LIST( layersOrder );
    for( int layer_idx = 0; layer_idx < NB_LAYERS; ++layer_idx )
    {
        layer = layersOrder[layer_idx];

        wxASSERT( layer < NB_LAYERS );

        if( m_BoxSelectLayer[layer] == NULL )
            continue;

        if( layer < NB_COPPER_LAYERS )
            m_CopperLayersBoxSizer->Add( m_BoxSelectLayer[layer],
                                         0, wxGROW | wxALL, 1 );
        else
            m_TechnicalLayersBoxSizer->Add( m_BoxSelectLayer[layer],
                                            0, wxGROW | wxALL, 1 );


        layerKey.Printf( OPTKEY_LAYERBASE, layer );
        bool option;
        if( m_Config->Read( layerKey, &option ) )
            m_BoxSelectLayer[layer]->SetValue( option );
        else
        {
            long mask = 1 << layer;
            if( mask & s_SelectedLayers )
                m_BoxSelectLayer[layer]->SetValue( true );
        }
    }


    // Option for using proper Gerber extensions
    long ltmp;
    m_Config->Read( OPTKEY_GERBER_EXTENSIONS, &ltmp );
    m_Use_Gerber_Extensions->SetValue( ltmp );

    // Option for excluding contents of "Edges Pcb" layer
    m_excludeEdgeLayerOpt->SetValue( g_PcbPlotOptions.m_ExcludeEdgeLayer );

    m_SubtractMaskFromSilk->SetValue( g_PcbPlotOptions.GetSubtractMaskFromSilk() );

    // Option to plot page references:
    if( m_Parent->m_Print_Sheet_Ref )
    {
        m_Plot_Sheet_Ref->SetValue( g_PcbPlotOptions.m_PlotFrameRef );
    }
    else
    {
        m_Plot_Sheet_Ref->Enable( false );
        g_PcbPlotOptions.m_PlotFrameRef = false;
    }

    // Option to plot pads on silkscreen layers or all layers
    m_Plot_Pads_on_Silkscreen->SetValue( g_PcbPlotOptions.m_PlotPadsOnSilkLayer );

    // Options to plot texts on footprints
    m_Plot_Text_Value->SetValue( g_PcbPlotOptions.m_PlotValue );
    m_Plot_Text_Ref->SetValue( g_PcbPlotOptions.m_PlotReference );
    m_Plot_Text_Div->SetValue( g_PcbPlotOptions.m_PlotTextOther );
    m_Plot_Invisible_Text->SetValue( g_PcbPlotOptions.m_PlotInvisibleTexts );

    // Options to plot pads and vias holes
    m_Drill_Shape_Opt->SetSelection( g_PcbPlotOptions.m_DrillShapeOpt );

    // Scale option
    m_Scale_Opt->SetSelection( m_dlgPlotScaleOpt );

    // Plot mode
    m_PlotModeOpt->SetSelection( g_PcbPlotOptions.Trace_Mode );

    // Plot mirror option
    m_PlotMirorOpt->SetValue( g_PcbPlotOptions.m_PlotMirror );

    // Put vias on mask layer
    m_PlotNoViaOnMaskOpt->SetValue( g_PcbPlotOptions.m_PlotViaOnMaskLayer );

    // Output directory
    if( g_PcbPlotOptions.GetOutputDirectory().IsEmpty() )
    {
        fileName = m_Parent->GetScreen()->m_FileName;
        m_OutputDirectory->SetValue( fileName.GetPath() );
    }
    else
    {
        m_OutputDirectory->SetValue( g_PcbPlotOptions.GetOutputDirectory() );
    }

    // Update options values:
    wxCommandEvent cmd_event;
    SetPlotFormat( cmd_event );
    OnSetScaleOpt( cmd_event );

    // without this line, the ESC key does not work
    SetFocus();
}


void DIALOG_PLOT::OnQuit( wxCommandEvent& WXUNUSED( event ) )
{
    Close( true );    // true is to force the frame to close
}


void DIALOG_PLOT::OnClose( wxCloseEvent& event )
{
    EndModal( 0 );
}


void DIALOG_PLOT::CreateDrillFile( wxCommandEvent& event )
{
    ( (WinEDA_PcbFrame*) m_Parent )->InstallDrillFrame( event );
}


void DIALOG_PLOT::OnSetScaleOpt( wxCommandEvent& event )
{
    /* Disable sheet reference for scale != 1:1 */
    bool scale1 = ( m_Scale_Opt->GetSelection() == 1 );

    m_Plot_Sheet_Ref->Enable( scale1 );

    if( !scale1 )
        m_Plot_Sheet_Ref->SetValue( false );
}


void DIALOG_PLOT::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    wxString    currentDir;

    currentDir = m_OutputDirectory->GetValue();
    wxDirDialog dirDialog( this, _( "Select Output Directory" ), currentDir );
    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;
    m_OutputDirectory->SetValue( dirDialog.GetPath() );
}


void DIALOG_PLOT::SetPlotFormat( wxCommandEvent& event )
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
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_SubtractMaskFromSilk->Enable( false );
        m_Use_Gerber_Extensions->Enable( false );
        m_Scale_Opt->Enable( true );
        m_FineAdjustXscaleOpt->Enable( true );
        m_FineAdjustYscaleOpt->Enable( true );
        m_plotPSNegativeOpt->Enable( true );
        break;

    case PLOT_FORMAT_GERBER:
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->SetSelection( 1 );
        m_PlotModeOpt->Enable( false );
        m_PlotMirorOpt->SetValue( false );
        m_PlotMirorOpt->Enable( false );
        m_Choice_Plot_Offset->Enable( true );
        m_LinesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_SubtractMaskFromSilk->Enable( true );
        m_Use_Gerber_Extensions->Enable( true );
        m_Scale_Opt->SetSelection( 1 );
        m_Scale_Opt->Enable( false );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        break;

    case PLOT_FORMAT_HPGL:
        m_PlotMirorOpt->Enable( true );
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->Enable( true );
        m_Choice_Plot_Offset->Enable( false );
        m_LinesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( true );
        m_HPGLPenSpeedOpt->Enable( true );
        m_HPGLPenOverlayOpt->Enable( true );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_SubtractMaskFromSilk->Enable( false );
        m_Use_Gerber_Extensions->Enable( false );
        m_Scale_Opt->Enable( true );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        break;

    case PLOT_FORMAT_DXF:
        m_PlotMirorOpt->Enable( false );
        m_PlotMirorOpt->SetValue( false );
        m_Drill_Shape_Opt->Enable( false );
        m_PlotModeOpt->Enable( true );
        m_Choice_Plot_Offset->Enable( false );
        m_LinesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_SubtractMaskFromSilk->Enable( false );
        m_Use_Gerber_Extensions->Enable( false );
        m_Scale_Opt->Enable( false );
        m_Scale_Opt->SetSelection( 1 );
        m_FineAdjustXscaleOpt->Enable( false );
        m_FineAdjustYscaleOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        break;
    }

    g_PcbPlotOptions.m_PlotFormat = format;
}


void DIALOG_PLOT::SaveOptPlot( wxCommandEvent& event )
{
    g_PcbPlotOptions.m_ExcludeEdgeLayer = m_excludeEdgeLayerOpt->GetValue();

    g_PcbPlotOptions.SetSubtractMaskFromSilk( m_SubtractMaskFromSilk->GetValue() );

    if( m_Plot_Sheet_Ref )
        g_PcbPlotOptions.m_PlotFrameRef = m_Plot_Sheet_Ref->GetValue();

    g_PcbPlotOptions.m_PlotPadsOnSilkLayer = m_Plot_Pads_on_Silkscreen->GetValue();

    s_PlotOriginIsAuxAxis =
        (m_Choice_Plot_Offset->GetSelection() == 0) ? FALSE : TRUE;

    g_PcbPlotOptions.m_PlotValue     = m_Plot_Text_Value->GetValue();
    g_PcbPlotOptions.m_PlotReference = m_Plot_Text_Ref->GetValue();
    g_PcbPlotOptions.m_PlotTextOther = m_Plot_Text_Div->GetValue();
    g_PcbPlotOptions.m_PlotInvisibleTexts = m_Plot_Invisible_Text->GetValue();

    m_dlgPlotScaleOpt  = m_Scale_Opt->GetSelection();
    g_PcbPlotOptions.m_DrillShapeOpt =
        (PCB_PLOT_PARAMS::DrillShapeOptT) m_Drill_Shape_Opt->GetSelection();
    g_PcbPlotOptions.m_PlotMirror = m_PlotMirorOpt->GetValue();
    g_PcbPlotOptions.Trace_Mode   = (GRTraceMode) m_PlotModeOpt->GetSelection();
    g_PcbPlotOptions.m_PlotViaOnMaskLayer = m_PlotNoViaOnMaskOpt->GetValue();

    wxString msg = m_HPGLPenSizeOpt->GetValue();
    int      tmp = ReturnValueFromString( g_UserUnit, msg, UNITS_MILS );
    g_PcbPlotOptions.m_HPGLPenDiam = tmp;

    msg = m_HPGLPenSpeedOpt->GetValue();
    tmp = ReturnValueFromString( MILLIMETRES, msg, 1 );
    g_PcbPlotOptions.m_HPGLPenSpeed = tmp;

    msg = m_HPGLPenOverlayOpt->GetValue();
    tmp = ReturnValueFromString( g_UserUnit, msg, UNITS_MILS );
    g_PcbPlotOptions.m_HPGLPenOvr = tmp;

    msg = m_LinesWidth->GetValue();
    tmp = ReturnValueFromString( g_UserUnit, msg, PCB_INTERNAL_UNIT );
    g_PcbPlotOptions.m_PlotLineWidth = tmp;
    g_DrawDefaultLineThickness = g_PcbPlotOptions.m_PlotLineWidth;

    msg = m_FineAdjustXscaleOpt->GetValue();
    msg.ToDouble( &m_XScaleAdjust );
    msg = m_FineAdjustYscaleOpt->GetValue();
    msg.ToDouble( &m_YScaleAdjust );

    m_Config->Write( OPTKEY_GERBER_EXTENSIONS,
                    m_Use_Gerber_Extensions->GetValue() );
    m_Config->Write( OPTKEY_XFINESCALE_ADJ, m_XScaleAdjust );
    m_Config->Write( OPTKEY_YFINESCALE_ADJ, m_YScaleAdjust );

    g_PcbPlotOptions.m_PlotFormat = m_PlotFormatOpt->GetSelection();

    wxString layerKey;
    for( int layer = 0; layer<NB_LAYERS; ++layer )
    {
        if( m_BoxSelectLayer[layer] == NULL )
            continue;
        layerKey.Printf( OPTKEY_LAYERBASE, layer );
        m_Config->Write( layerKey, m_BoxSelectLayer[layer]->IsChecked() );
    }

    g_PcbPlotOptions.m_PlotPSNegative = m_plotPSNegativeOpt->GetValue();

    g_PcbPlotOptions.SetOutputDirectory( m_OutputDirectory->GetValue() );
}


void DIALOG_PLOT::Plot( wxCommandEvent& event )
{
    int        layer;
    wxFileName fn;
    wxString   ext;

    BOARD*     board = m_Parent->GetBoard();

    SaveOptPlot( event );

    // Create output directory if it does not exist
    if( !wxFileName::DirExists( m_OutputDirectory->GetValue() ) )
    {
        if( wxMkdir( m_OutputDirectory->GetValue() ) )
        {
            wxString msg;
            msg.Printf( _( "Directory %s created.\n" ), GetChars( m_OutputDirectory->GetValue() ) );
            m_MessagesBox->AppendText( msg );
        }
        else
        {
            wxMessageBox( _( "Cannot create output directory!" ), _( "Plot" ), wxICON_INFORMATION );
            return;
        }
    }

    g_PcbPlotOptions.m_AutoScale = false;
    g_PcbPlotOptions.m_PlotScale = 1;
    switch( m_dlgPlotScaleOpt )
    {
    default:
        break;

    case 0:
        g_PcbPlotOptions.m_AutoScale = true;
        break;

    case 2:
        g_PcbPlotOptions.m_PlotScale = 1.5;
        break;

    case 3:
        g_PcbPlotOptions.m_PlotScale = 2;
        break;

    case 4:
        g_PcbPlotOptions.m_PlotScale = 3;
        break;
    }

    /* If the scale factor edit controls are disabled or the scale value
     * is 0, don't adjust the base scale factor.   This fixes a bug when
     * the default scale adjust is initialized to 0 and saved in program
     * settings resulting in a divide by zero fault.
     */
    if( m_FineAdjustXscaleOpt->IsEnabled()  && m_XScaleAdjust != 0.0 )
        g_PcbPlotOptions.m_FineScaleAdjustX = m_XScaleAdjust;
    if( m_FineAdjustYscaleOpt->IsEnabled() && m_YScaleAdjust != 0.0 )
        g_PcbPlotOptions.m_FineScaleAdjustY = m_YScaleAdjust;

    int format = getFormat();

    switch( format )
    {
    case PLOT_FORMAT_POST:
        ext = wxT( "ps" );
        break;

    case PLOT_FORMAT_GERBER:
        g_PcbPlotOptions.m_PlotScale = 1.0; // No scale option allowed in gerber format
        ext = wxT( "pho" );
        break;

    case PLOT_FORMAT_HPGL:
        ext = wxT( "plt" );
        break;

    case PLOT_FORMAT_DXF:
        g_PcbPlotOptions.m_PlotScale = 1.0;
        ext = wxT( "dxf" );
        break;
    }

    // Test for a reasonable scale value
    if( g_PcbPlotOptions.m_PlotScale < MIN_SCALE )
        DisplayInfoMessage( this,
                           _( "Warning: Scale option set to a very small value" ) );
    if( g_PcbPlotOptions.m_PlotScale > MAX_SCALE )
        DisplayInfoMessage( this,
                           _( "Warning: Scale option set to a very large value" ) );

    int mask = 1;
    s_SelectedLayers = 0;
    for( layer = 0; layer < NB_LAYERS; layer++, mask <<= 1 )
    {
        if( m_BoxSelectLayer[layer] == NULL )
            continue;
        bool success = false;
        if( m_BoxSelectLayer[layer]->GetValue() )
        {
            s_SelectedLayers |= mask;

            fn = m_Parent->GetScreen()->m_FileName;
            fn.SetPath( m_OutputDirectory->GetValue() );

            // Create file name.
            wxString layername = board->GetLayerName( layer );
            layername.Trim( true ); layername.Trim( false );    // remove leading and trailing spaces if any
            fn.SetName( fn.GetName() + wxT( "-" ) + layername );

            // Use Gerber Extensions based on layer number
            // (See http://en.wikipedia.org/wiki/Gerber_File)
            if( (format == PLOT_FORMAT_GERBER) && m_Use_Gerber_Extensions->GetValue() )
            {
                switch( layer )
                {
                case LAYER_N_FRONT:
                    fn.SetExt( wxT( "gtl" ) );
                    break;

                case LAYER_N_2:
                case LAYER_N_3:
                case LAYER_N_4:
                case LAYER_N_5:
                case LAYER_N_6:
                case LAYER_N_7:
                case LAYER_N_8:
                case LAYER_N_9:
                case LAYER_N_10:
                case LAYER_N_11:
                case LAYER_N_12:
                case LAYER_N_13:
                case LAYER_N_14:
                case LAYER_N_15:

                    // TODO: see if we use .gbr or a layer identifier (gb1 .. gbnn ?)
                    // according to the new internal layers designation
                    // (1 is the first internal layer from the front layer)
                    fn.SetExt( wxT( "gbr" ) );
                    break;

                case LAYER_N_BACK:
                    fn.SetExt( wxT( "gbl" ) );
                    break;

                case ADHESIVE_N_BACK:
                    fn.SetExt( wxT( "gba" ) );
                    break;

                case ADHESIVE_N_FRONT:
                    fn.SetExt( wxT( "gta" ) );
                    break;

                case SOLDERPASTE_N_BACK:
                    fn.SetExt( wxT( "gbp" ) );
                    break;

                case SOLDERPASTE_N_FRONT:
                    fn.SetExt( wxT( "gtp" ) );
                    break;

                case SILKSCREEN_N_BACK:
                    fn.SetExt( wxT( "gbo" ) );
                    break;

                case SILKSCREEN_N_FRONT:
                    fn.SetExt( wxT( "gto" ) );
                    break;

                case SOLDERMASK_N_BACK:
                    fn.SetExt( wxT( "gbs" ) );
                    break;

                case SOLDERMASK_N_FRONT:
                    fn.SetExt( wxT( "gts" ) );
                    break;

                case DRAW_N:
                case COMMENT_N:
                case ECO1_N:
                case ECO2_N:
                case EDGE_N:
                default:
                    fn.SetExt( wxT( "gbr" ) );
                    break;
                }
            }
            else
            {
                fn.SetExt( ext );
            }

            switch( format )
            {
            case PLOT_FORMAT_POST:
                success = m_Parent->Genere_PS( fn.GetFullPath(), layer, useA4(),
                                               g_PcbPlotOptions.Trace_Mode );
                break;

            case PLOT_FORMAT_GERBER:
                success = m_Parent->Genere_GERBER( fn.GetFullPath(), layer,
                                                   s_PlotOriginIsAuxAxis,
                                                   g_PcbPlotOptions.Trace_Mode );
                break;

            case PLOT_FORMAT_HPGL:
                success = m_Parent->Genere_HPGL( fn.GetFullPath(), layer,
                                                 g_PcbPlotOptions.Trace_Mode );
                break;

            case PLOT_FORMAT_DXF:
                success = m_Parent->Genere_DXF( fn.GetFullPath(), layer,
                                                g_PcbPlotOptions.Trace_Mode );
                break;
            }

            // Print diags in messages box:
            wxString msg;
            if( success )
                msg.Printf( _( "Plot file <%s> created" ), GetChars( fn.GetFullPath() ) );
            else
                msg.Printf( _( "Unable to create <%s>" ), GetChars( fn.GetFullPath() ) );
            msg << wxT( "\n" );
            m_MessagesBox->AppendText( msg );
        }
    }

    // If no layer selected, we have nothing plotted.
    // Prompt user if it happens
    // because he could think there is a bug in pcbnew:
    if( s_SelectedLayers == 0 )
        DisplayError( this, _( "No layer selected" ) );
}


void WinEDA_PcbFrame::ToPlotter( wxCommandEvent& event )
{
    DIALOG_PLOT dlg( this );
    dlg.ShowModal();
}
