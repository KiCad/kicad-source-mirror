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
#include "pcb_plot_params.h"

#define PLOT_DEFAULT_MARGE 300      // mils

/* Keywords to r/w options in m_Config */
#define OPTKEY_XFINESCALE_ADJ    wxT( "PlotXFineScaleAdj" )
#define OPTKEY_YFINESCALE_ADJ    wxT( "PlotYFineScaleAdj" )

// Define min and max reasonable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

extern int g_DrawDefaultLineThickness;


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

    bool useA4()
    {
        return m_plotFormatOpt->GetSelection() == 3;
    }


    /**
     * Function getFormat
     * returns one of the values from the m_PlotFormat enum.  If the 4th
     * radio button is selected, map this back to postscript.
     */
    PlotFormat getFormat()
    {
        int radioNdx = m_plotFormatOpt->GetSelection();

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
    void applyPlotSettings( wxCommandEvent& event );
    void CreateDrillFile( wxCommandEvent& event );
};

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

    m_plotFormatOpt->SetSelection( g_PcbPlotOptions.m_PlotFormat );
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
    m_linesWidth->AppendText( msg );

    if( g_PcbPlotOptions.GetUseAuxOrigin() )
        m_choicePlotOffset->SetSelection( 1 );
    else
        m_choicePlotOffset->SetSelection( 0 );

    // Test for a reasonable scale value. Set to 1 if problem
    if( m_XScaleAdjust < MIN_SCALE || m_YScaleAdjust < MIN_SCALE
        || m_XScaleAdjust > MAX_SCALE || m_YScaleAdjust > MAX_SCALE )
        m_XScaleAdjust = m_YScaleAdjust = 1.0;

    msg.Printf( wxT( "%f" ), m_XScaleAdjust );
    m_fineAdjustXscaleOpt->AppendText( msg );

    msg.Printf( wxT( "%f" ), m_YScaleAdjust );
    m_fineAdjustYscaleOpt->AppendText( msg );

    m_plotPSNegativeOpt->SetValue( g_PcbPlotOptions.m_PlotPSNegative );


    // Create layer list.
    int      layer;
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

        long mask = 1 << layer;
        if( g_PcbPlotOptions.GetLayerSelection() & mask )
            m_BoxSelectLayer[layer]->SetValue( true );
    }


    // Option for using proper Gerber extensions
    m_useGerberExtensions->SetValue( g_PcbPlotOptions.GetUseGerberExtensions() );

    // Option for excluding contents of "Edges Pcb" layer
    m_excludeEdgeLayerOpt->SetValue( g_PcbPlotOptions.m_ExcludeEdgeLayer );

    m_subtractMaskFromSilk->SetValue( g_PcbPlotOptions.GetSubtractMaskFromSilk() );

    // Option to plot page references:
    if( m_Parent->m_Print_Sheet_Ref )
    {
        m_plotSheetRef->SetValue( g_PcbPlotOptions.m_PlotFrameRef );
    }
    else
    {
        m_plotSheetRef->Enable( false );
        g_PcbPlotOptions.m_PlotFrameRef = false;
    }

    // Option to plot pads on silkscreen layers or all layers
    m_plotPads_on_Silkscreen->SetValue( g_PcbPlotOptions.m_PlotPadsOnSilkLayer );

    // Options to plot texts on footprints
    m_plotModuleValueOpt->SetValue( g_PcbPlotOptions.m_PlotValue );
    m_plotModuleRefOpt->SetValue( g_PcbPlotOptions.m_PlotReference );
    m_plotTextOther->SetValue( g_PcbPlotOptions.m_PlotTextOther );
    m_plotInvisibleText->SetValue( g_PcbPlotOptions.m_PlotInvisibleTexts );

    // Options to plot pads and vias holes
    m_drillShapeOpt->SetSelection( g_PcbPlotOptions.m_DrillShapeOpt );

    // Scale option
    m_scaleOpt->SetSelection( g_PcbPlotOptions.GetScaleSelection() );

    // Plot mode
    m_plotModeOpt->SetSelection( g_PcbPlotOptions.m_PlotMode );

    // Plot mirror option
    m_plotMirrorOpt->SetValue( g_PcbPlotOptions.m_PlotMirror );

    // Put vias on mask layer
    m_plotNoViaOnMaskOpt->SetValue( g_PcbPlotOptions.m_PlotViaOnMaskLayer );

    // Output directory
    if( g_PcbPlotOptions.GetOutputDirectory().IsEmpty() )
    {
        fileName = m_Parent->GetScreen()->GetFileName();
        m_outputDirectoryName->SetValue( fileName.GetPath() );
    }
    else
    {
        m_outputDirectoryName->SetValue( g_PcbPlotOptions.GetOutputDirectory() );
    }

    // Update options values:
    wxCommandEvent cmd_event;
    SetPlotFormat( cmd_event );
    OnSetScaleOpt( cmd_event );

    // without this line, the ESC key does not work
    SetFocus();
}


void DIALOG_PLOT::OnQuit( wxCommandEvent& event )
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
    bool scale1 = ( m_scaleOpt->GetSelection() == 1 );

    m_plotSheetRef->Enable( scale1 );

    if( !scale1 )
        m_plotSheetRef->SetValue( false );
}


void DIALOG_PLOT::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    wxString    currentDir;

    currentDir = m_outputDirectoryName->GetValue();
    wxDirDialog dirDialog( this, _( "Select Output Directory" ), currentDir );
    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;
    m_outputDirectoryName->SetValue( dirDialog.GetPath() );
}


void DIALOG_PLOT::SetPlotFormat( wxCommandEvent& event )
{
    int format = getFormat();

    switch( format  )
    {
    case PLOT_FORMAT_POST:
    default:
        m_drillShapeOpt->Enable( true );
        m_plotModeOpt->Enable( true );
        m_plotMirrorOpt->Enable( true );
        m_choicePlotOffset->Enable( false );
        m_linesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_subtractMaskFromSilk->Enable( false );
        m_useGerberExtensions->Enable( false );
        m_scaleOpt->Enable( true );
        m_fineAdjustXscaleOpt->Enable( true );
        m_fineAdjustYscaleOpt->Enable( true );
        m_plotPSNegativeOpt->Enable( true );
        break;

    case PLOT_FORMAT_GERBER:
        m_drillShapeOpt->Enable( false );
        m_plotModeOpt->SetSelection( 1 );
        m_plotModeOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_plotMirrorOpt->Enable( false );
        m_choicePlotOffset->Enable( true );
        m_linesWidth->Enable( true );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->Enable( true );
        m_subtractMaskFromSilk->Enable( true );
        m_useGerberExtensions->Enable( true );
        m_scaleOpt->SetSelection( 1 );
        m_scaleOpt->Enable( false );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        break;

    case PLOT_FORMAT_HPGL:
        m_plotMirrorOpt->Enable( true );
        m_drillShapeOpt->Enable( false );
        m_plotModeOpt->Enable( true );
        m_choicePlotOffset->Enable( false );
        m_linesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( true );
        m_HPGLPenSpeedOpt->Enable( true );
        m_HPGLPenOverlayOpt->Enable( true );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_subtractMaskFromSilk->Enable( false );
        m_useGerberExtensions->Enable( false );
        m_scaleOpt->Enable( true );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        break;

    case PLOT_FORMAT_DXF:
        m_plotMirrorOpt->Enable( false );
        m_plotMirrorOpt->SetValue( false );
        m_drillShapeOpt->Enable( false );
        m_plotModeOpt->Enable( true );
        m_choicePlotOffset->Enable( false );
        m_linesWidth->Enable( false );
        m_HPGLPenSizeOpt->Enable( false );
        m_HPGLPenSpeedOpt->Enable( false );
        m_HPGLPenOverlayOpt->Enable( false );
        m_excludeEdgeLayerOpt->SetValue( false );
        m_excludeEdgeLayerOpt->Enable( false );
        m_subtractMaskFromSilk->Enable( false );
        m_useGerberExtensions->Enable( false );
        m_scaleOpt->Enable( false );
        m_scaleOpt->SetSelection( 1 );
        m_fineAdjustXscaleOpt->Enable( false );
        m_fineAdjustYscaleOpt->Enable( false );
        m_plotPSNegativeOpt->SetValue( false );
        m_plotPSNegativeOpt->Enable( false );
        break;
    }

    g_PcbPlotOptions.m_PlotFormat = format;
}


void DIALOG_PLOT::applyPlotSettings( wxCommandEvent& event )
{
    PCB_PLOT_PARAMS tempOptions;

    tempOptions.m_ExcludeEdgeLayer = m_excludeEdgeLayerOpt->GetValue();

    tempOptions.SetSubtractMaskFromSilk( m_subtractMaskFromSilk->GetValue() );

    if( m_plotSheetRef )
        tempOptions.m_PlotFrameRef = m_plotSheetRef->GetValue();

    tempOptions.m_PlotPadsOnSilkLayer = m_plotPads_on_Silkscreen->GetValue();

    if( m_choicePlotOffset->GetSelection() == 0 )
        tempOptions.SetUseAuxOrigin( false );
    else
        tempOptions.SetUseAuxOrigin( true );

    tempOptions.m_PlotValue     = m_plotModuleValueOpt->GetValue();
    tempOptions.m_PlotReference = m_plotModuleRefOpt->GetValue();
    tempOptions.m_PlotTextOther = m_plotTextOther->GetValue();
    tempOptions.m_PlotInvisibleTexts = m_plotInvisibleText->GetValue();

    tempOptions.SetScaleSelection( m_scaleOpt->GetSelection() );

    tempOptions.m_DrillShapeOpt =
        (PCB_PLOT_PARAMS::DrillShapeOptT) m_drillShapeOpt->GetSelection();
    tempOptions.m_PlotMirror = m_plotMirrorOpt->GetValue();
    tempOptions.m_PlotMode   = (GRTraceMode) m_plotModeOpt->GetSelection();
    tempOptions.m_PlotViaOnMaskLayer = m_plotNoViaOnMaskOpt->GetValue();

    wxString msg = m_HPGLPenSizeOpt->GetValue();
    int      tmp = ReturnValueFromString( g_UserUnit, msg, UNITS_MILS );
    tempOptions.m_HPGLPenDiam = tmp;

    msg = m_HPGLPenSpeedOpt->GetValue();
    tmp = ReturnValueFromString( UNSCALED_UNITS, msg, 1 );
    tempOptions.m_HPGLPenSpeed = tmp;

    msg = m_HPGLPenOverlayOpt->GetValue();
    tmp = ReturnValueFromString( g_UserUnit, msg, UNITS_MILS );
    tempOptions.m_HPGLPenOvr = tmp;

    msg = m_linesWidth->GetValue();
    tmp = ReturnValueFromString( g_UserUnit, msg, PCB_INTERNAL_UNIT );
    tempOptions.m_PlotLineWidth = tmp;
    g_DrawDefaultLineThickness = tempOptions.m_PlotLineWidth;

    msg = m_fineAdjustXscaleOpt->GetValue();
    msg.ToDouble( &m_XScaleAdjust );
    msg = m_fineAdjustYscaleOpt->GetValue();
    msg.ToDouble( &m_YScaleAdjust );

    tempOptions.SetUseGerberExtensions( m_useGerberExtensions->GetValue() );
    m_Config->Write( OPTKEY_XFINESCALE_ADJ, m_XScaleAdjust );
    m_Config->Write( OPTKEY_YFINESCALE_ADJ, m_YScaleAdjust );

    tempOptions.m_PlotFormat = m_plotFormatOpt->GetSelection();

    long selectedLayers = 0;
    long mask = 1;
    int layer;
    for( layer = 0; layer < NB_LAYERS; layer++, mask <<= 1 )
    {
        if( m_BoxSelectLayer[layer] == NULL )
            continue;
        if( m_BoxSelectLayer[layer]->GetValue() )
            selectedLayers |= mask;
    }
    tempOptions.SetLayerSelection( selectedLayers );

    tempOptions.m_PlotPSNegative = m_plotPSNegativeOpt->GetValue();

    tempOptions.SetOutputDirectory( m_outputDirectoryName->GetValue() );

    if( g_PcbPlotOptions != tempOptions )
    {
        g_PcbPlotOptions = tempOptions;
        m_Parent->OnModify();
    }
}


void DIALOG_PLOT::Plot( wxCommandEvent& event )
{
    int        layer;
    wxFileName fn;
    wxString   ext;

    BOARD*     board = m_Parent->GetBoard();

    applyPlotSettings( event );

    // Create output directory if it does not exist
    if( !wxFileName::DirExists( m_outputDirectoryName->GetValue() ) )
    {
        if( wxMkdir( m_outputDirectoryName->GetValue() ) )
        {
            wxString msg;
            msg.Printf( _( "Directory %s created.\n" ), GetChars( m_outputDirectoryName->GetValue() ) );
            m_messagesBox->AppendText( msg );
        }
        else
        {
            wxMessageBox( _( "Cannot create output directory!" ), _( "Plot" ), wxICON_INFORMATION );
            return;
        }
    }

    g_PcbPlotOptions.m_AutoScale = false;
    g_PcbPlotOptions.m_PlotScale = 1;
    switch( g_PcbPlotOptions.GetScaleSelection() )
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
    if( m_fineAdjustXscaleOpt->IsEnabled()  && m_XScaleAdjust != 0.0 )
        g_PcbPlotOptions.m_FineScaleAdjustX = m_XScaleAdjust;
    if( m_fineAdjustYscaleOpt->IsEnabled() && m_YScaleAdjust != 0.0 )
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

    long mask = 1;
    for( layer = 0; layer < NB_LAYERS; layer++, mask <<= 1 )
    {
        if( m_BoxSelectLayer[layer] == NULL )
            continue;
        bool success = false;
        if( m_BoxSelectLayer[layer]->GetValue() )
        {
            fn = m_Parent->GetScreen()->GetFileName();
            fn.SetPath( m_outputDirectoryName->GetValue() );

            // Create file name.
            wxString layername = board->GetLayerName( layer );
            layername.Trim( true ); layername.Trim( false );    // remove leading and trailing spaces if any
            fn.SetName( fn.GetName() + wxT( "-" ) + layername );

            // Use Gerber Extensions based on layer number
            // (See http://en.wikipedia.org/wiki/Gerber_File)
            if( (format == PLOT_FORMAT_GERBER) && m_useGerberExtensions->GetValue() )
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
                                               g_PcbPlotOptions.m_PlotMode );
                break;

            case PLOT_FORMAT_GERBER:
                success = m_Parent->Genere_GERBER( fn.GetFullPath(), layer,
                                                   g_PcbPlotOptions.GetUseAuxOrigin(),
                                                   g_PcbPlotOptions.m_PlotMode );
                break;

            case PLOT_FORMAT_HPGL:
                success = m_Parent->Genere_HPGL( fn.GetFullPath(), layer,
                                                 g_PcbPlotOptions.m_PlotMode );
                break;

            case PLOT_FORMAT_DXF:
                success = m_Parent->Genere_DXF( fn.GetFullPath(), layer,
                                                g_PcbPlotOptions.m_PlotMode );
                break;
            }

            // Print diags in messages box:
            wxString msg;
            if( success )
                msg.Printf( _( "Plot file <%s> created" ), GetChars( fn.GetFullPath() ) );
            else
                msg.Printf( _( "Unable to create <%s>" ), GetChars( fn.GetFullPath() ) );
            msg << wxT( "\n" );
            m_messagesBox->AppendText( msg );
        }
    }

    // If no layer selected, we have nothing plotted.
    // Prompt user if it happens
    // because he could think there is a bug in pcbnew:
    if( !g_PcbPlotOptions.GetLayerSelection() )
        DisplayError( this, _( "No layer selected" ) );
}


void WinEDA_PcbFrame::ToPlotter( wxCommandEvent& event )
{
    DIALOG_PLOT dlg( this );
    dlg.ShowModal();
}
