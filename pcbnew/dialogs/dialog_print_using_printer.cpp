/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2016 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <pcbnew_printout.h>
#include <pcbnew.h>
#include <pcbplot.h>
#include <class_board.h>
#include <enabler.h>
#include <widgets/unit_binder.h>

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

#include <dialog_print_using_printer_base.h>

#define PEN_WIDTH_MAX_VALUE ( KiROUND( 5 * IU_PER_MM ) )
#define PEN_WIDTH_MIN_VALUE ( KiROUND( 0.005 * IU_PER_MM ) )


extern int g_DrawDefaultLineThickness;

// Local variables
static double s_ScaleList[] = { 0, 0.5, 0.7, 0.999, 1.0, 1.4, 2.0, 3.0, 4.0 };

// Define min et max reasonable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// static print data and page setup data, to remember settings during the session
static wxPrintData* s_PrintData;
static wxPageSetupDialogData* s_pageSetupData = (wxPageSetupDialogData*) NULL;

static PRINT_PARAMETERS  s_Parameters;


/**
 * Dialog to print schematic. Class derived from DIALOG_PRINT_USING_PRINTER_BASE
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_BASE
{
public:
    DIALOG_PRINT_USING_PRINTER( PCB_EDIT_FRAME* parent );
    ~DIALOG_PRINT_USING_PRINTER() override;

private:
    PCB_EDIT_FRAME* m_parent;
    wxConfigBase*   m_config;
    // the list of existing board layers in wxCheckListBox, with the board layers id:
    std::pair<wxCheckListBox*, int> m_layers[PCB_LAYER_ID_COUNT];
    static bool     m_ExcludeEdgeLayer;

    UNIT_BINDER     m_defaultPenWidth;

    bool TransferDataToWindow() override;

    void OnPageSetup( wxCommandEvent& event ) override;
    void OnPrintPreview( wxCommandEvent& event ) override;
    void OnPrintButtonClick( wxCommandEvent& event ) override;
    void OnScaleSelectionClick( wxCommandEvent& event ) override;

    void SetPrintParameters();
    int SetLayerSetFromListSelection();

    PCBNEW_PRINTOUT* createPrintout( const wxString& aTitle )
    {
        return new PCBNEW_PRINTOUT( m_parent->GetBoard(), s_Parameters,
            m_parent->GetGalCanvas()->GetView(), m_parent->GetPageSettings().GetSizeIU(), aTitle );
    }
};


bool DIALOG_PRINT_USING_PRINTER::m_ExcludeEdgeLayer;


void PCB_EDIT_FRAME::ToPrinter( wxCommandEvent& event )
{
    // Selection affects the original item visibility
    GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    const PAGE_INFO& pageInfo = GetPageSettings();

    if( s_PrintData == NULL )  // First print
    {
        s_PrintData = new wxPrintData();

        if( !s_PrintData->Ok() )
            DisplayError( this, _( "Error Init Printer info" ) );

        s_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGH;
    }

    if( s_pageSetupData == NULL )
        s_pageSetupData = new wxPageSetupDialogData( *s_PrintData );

    s_pageSetupData->SetPaperId( pageInfo.GetPaperId() );
    s_pageSetupData->GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

    if( pageInfo.IsCustom() )
    {
        if( pageInfo.IsPortrait() )
            s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetWidthMils() ),
                                                   Mils2mm( pageInfo.GetHeightMils() ) ) );
        else
            s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetHeightMils() ),
                                                   Mils2mm( pageInfo.GetWidthMils() ) ) );
    }

    *s_PrintData = s_pageSetupData->GetPrintData();

    DIALOG_PRINT_USING_PRINTER dlg( this );

    dlg.ShowModal();
}


DIALOG_PRINT_USING_PRINTER::DIALOG_PRINT_USING_PRINTER( PCB_EDIT_FRAME* parent ) :
    DIALOG_PRINT_USING_PRINTER_BASE( parent ),
    m_parent( parent ),
    m_defaultPenWidth( parent, m_penWidthLabel, m_penWidthCtrl, m_penWidthUnits, true,
                       PEN_WIDTH_MIN_VALUE, PEN_WIDTH_MAX_VALUE )
{
    m_config = Kiface().KifaceSettings();
    memset( m_layers, 0, sizeof( m_layers ) );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Print" ) );
    m_sdbSizer1Apply->SetLabel( _( "Print Preview" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();

#ifdef __WXMAC__
    /* Problems with modal on wx-2.9 - Anyway preview is standard for OSX */
   m_sdbSizer1Apply->Hide();
#endif

   FinishDialogSettings();
}


DIALOG_PRINT_USING_PRINTER::~DIALOG_PRINT_USING_PRINTER()
{
    SetPrintParameters();

    if( m_config )
    {
        ConfigBaseWriteDouble( m_config, OPTKEY_PRINT_X_FINESCALE_ADJ, s_Parameters.m_XScaleAdjust );
        ConfigBaseWriteDouble( m_config, OPTKEY_PRINT_Y_FINESCALE_ADJ, s_Parameters.m_YScaleAdjust );
        m_config->Write( OPTKEY_PRINT_SCALE, m_ScaleOption->GetSelection() );
        m_config->Write( OPTKEY_PRINT_PAGE_FRAME, s_Parameters.m_Print_Sheet_Ref);
        m_config->Write( OPTKEY_PRINT_MONOCHROME_MODE, s_Parameters.m_Print_Black_and_White);
        m_config->Write( OPTKEY_PRINT_PAGE_PER_LAYER, s_Parameters.m_OptionPrintPage );
        m_config->Write( OPTKEY_PRINT_PADS_DRILL, (long) s_Parameters.m_DrillShapeOpt );

        for( unsigned layer = 0; layer < DIM(m_layers); ++layer )
        {
            if( m_layers[layer].first )
            {
                wxString key = wxString::Format( OPTKEY_LAYERBASE, layer );
                bool value = m_layers[layer].first->IsChecked( m_layers[layer].second );
                m_config->Write( key, value );
            }
        }
    }
}


bool DIALOG_PRINT_USING_PRINTER::TransferDataToWindow()
{
    wxString msg;
    BOARD*   board = m_parent->GetBoard();

    s_Parameters.m_PageSetupData = s_pageSetupData;

    // Create layer list.
    for( LSEQ seq = board->GetEnabledLayers().UIOrder(); seq; ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        int checkIndex;

        if( IsCopperLayer( layer ) )
        {
            checkIndex = m_CopperLayersList->Append( board->GetLayerName( layer ) );
            m_layers[layer] = std::make_pair( m_CopperLayersList, checkIndex );
        }
        else
        {
            checkIndex = m_TechnicalLayersList->Append( board->GetLayerName( layer ) );
            m_layers[layer] = std::make_pair( m_TechnicalLayersList, checkIndex );
        }

        if( m_config )
        {
            wxString layerKey;
            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            bool option;

            if( m_config->Read( layerKey, &option ) )
                m_layers[layer].first->Check( checkIndex, option );
        }
    }

    // Option for excluding contents of "Edges Pcb" layer
    m_Exclude_Edges_Pcb->Show( true );

    // Read the scale adjust option
    int scale_idx = 4; // default selected scale = ScaleList[4] = 1.000

    if( m_config )
    {
        m_config->Read( OPTKEY_PRINT_X_FINESCALE_ADJ, &s_Parameters.m_XScaleAdjust );
        m_config->Read( OPTKEY_PRINT_Y_FINESCALE_ADJ, &s_Parameters.m_YScaleAdjust );
        m_config->Read( OPTKEY_PRINT_SCALE, &scale_idx );
        m_config->Read( OPTKEY_PRINT_PAGE_FRAME, &s_Parameters.m_Print_Sheet_Ref, 1);
        m_config->Read( OPTKEY_PRINT_MONOCHROME_MODE, &s_Parameters.m_Print_Black_and_White, 1);
        m_config->Read( OPTKEY_PRINT_PAGE_PER_LAYER, &s_Parameters.m_OptionPrintPage, 0);
        int tmp;
        m_config->Read( OPTKEY_PRINT_PADS_DRILL,  &tmp, PRINT_PARAMETERS::SMALL_DRILL_SHAPE );
        s_Parameters.m_DrillShapeOpt = (PRINT_PARAMETERS::DrillShapeOptT) tmp;

        // Test for a reasonable scale value. Set to 1 if problem
        if( s_Parameters.m_XScaleAdjust < MIN_SCALE || s_Parameters.m_XScaleAdjust > MAX_SCALE ||
            s_Parameters.m_YScaleAdjust < MIN_SCALE || s_Parameters.m_YScaleAdjust > MAX_SCALE )
            s_Parameters.m_XScaleAdjust = s_Parameters.m_YScaleAdjust = 1.0;
    }

    m_ScaleOption->SetSelection( scale_idx );
    scale_idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale = s_ScaleList[scale_idx];
    m_Print_Mirror->SetValue(s_Parameters.m_PrintMirror);
    m_Exclude_Edges_Pcb->SetValue(m_ExcludeEdgeLayer);
    m_Print_Sheet_Ref->SetValue( s_Parameters.m_Print_Sheet_Ref );

    // Options to plot pads and vias holes
    m_drillMarksChoice->SetSelection( s_Parameters.m_DrillShapeOpt );

    m_outputMode->SetSelection( s_Parameters.m_Print_Black_and_White ? 1 : 0 );

    m_PagesOption->SetSelection(s_Parameters.m_OptionPrintPage);
    s_Parameters.m_PenDefaultSize = g_DrawDefaultLineThickness;
    m_defaultPenWidth.SetValue( s_Parameters.m_PenDefaultSize );

    // Create scale adjust option
    msg.Printf( wxT( "%f" ), s_Parameters.m_XScaleAdjust );
    m_FineAdjustXscaleOpt->SetValue( msg );

    msg.Printf( wxT( "%f" ), s_Parameters.m_YScaleAdjust );
    m_FineAdjustYscaleOpt->SetValue( msg );

    bool enable = ( s_Parameters.m_PrintScale == 1.0 );
    m_FineAdjustXscaleOpt->Enable(enable);
    m_FineAdjustYscaleOpt->Enable(enable);

    return true;
}


int DIALOG_PRINT_USING_PRINTER::SetLayerSetFromListSelection()
{
    int page_count = 0;

    s_Parameters.m_PrintMaskLayer = LSET();

    for( unsigned layer = 0; layer < DIM(m_layers); ++layer )
    {
        if( m_layers[layer].first && m_layers[layer].first->IsChecked( m_layers[layer].second ) )
        {
            page_count++;
            s_Parameters.m_PrintMaskLayer.set( layer );
        }
    }

    // In Pcbnew force the EDGE layer to be printed or not with the other layers
    m_ExcludeEdgeLayer = m_Exclude_Edges_Pcb->IsChecked();
    s_Parameters.m_Flags = m_ExcludeEdgeLayer ? 0 : 1;

    if( m_PagesOption->GetSelection() != 0 )
        page_count = 1;

    s_Parameters.m_PageCount = page_count;

    return page_count;
}


void DIALOG_PRINT_USING_PRINTER::SetPrintParameters()
{
    PCB_PLOT_PARAMS plot_opts = m_parent->GetPlotSettings();

    s_Parameters.m_PrintMirror = m_Print_Mirror->GetValue();
    s_Parameters.m_Print_Sheet_Ref = m_Print_Sheet_Ref->GetValue();
    s_Parameters.m_Print_Black_and_White = m_outputMode->GetSelection() != 0;

    s_Parameters.m_DrillShapeOpt =
        (PRINT_PARAMETERS::DrillShapeOptT) m_drillMarksChoice->GetSelection();

    s_Parameters.m_OptionPrintPage = m_PagesOption->GetSelection() != 0;

    SetLayerSetFromListSelection();

    int idx = m_ScaleOption->GetSelection();
    s_Parameters.m_PrintScale =  s_ScaleList[idx];
    plot_opts.SetScale( s_Parameters.m_PrintScale );

    if( m_FineAdjustXscaleOpt )
    {
        if( s_Parameters.m_XScaleAdjust > MAX_SCALE || s_Parameters.m_YScaleAdjust > MAX_SCALE )
            DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very large value" ) );

        m_FineAdjustXscaleOpt->GetValue().ToDouble( &s_Parameters.m_XScaleAdjust );
    }

    if( m_FineAdjustYscaleOpt )
    {
        // Test for a reasonable scale value
        if( s_Parameters.m_XScaleAdjust < MIN_SCALE || s_Parameters.m_YScaleAdjust < MIN_SCALE )
            DisplayInfoMessage( NULL, _( "Warning: Scale option set to a very small value" ) );

        m_FineAdjustYscaleOpt->GetValue().ToDouble( &s_Parameters.m_YScaleAdjust );
    }

    plot_opts.SetFineScaleAdjustX( s_Parameters.m_XScaleAdjust );
    plot_opts.SetFineScaleAdjustY( s_Parameters.m_YScaleAdjust );

    m_parent->SetPlotSettings( plot_opts );

    s_Parameters.m_PenDefaultSize = m_defaultPenWidth.GetValue();
    g_DrawDefaultLineThickness = s_Parameters.m_PenDefaultSize;
}


void DIALOG_PRINT_USING_PRINTER::OnScaleSelectionClick( wxCommandEvent& event )
{
    double scale = s_ScaleList[m_ScaleOption->GetSelection()];
    bool enable = (scale == 1.0);

    if( m_FineAdjustXscaleOpt )
        m_FineAdjustXscaleOpt->Enable(enable);
    if( m_FineAdjustYscaleOpt )
        m_FineAdjustYscaleOpt->Enable(enable);
}


void DIALOG_PRINT_USING_PRINTER::OnPageSetup( wxCommandEvent& event )
{
    wxPageSetupDialog pageSetupDialog( this, s_pageSetupData );
    pageSetupDialog.ShowModal();

    (*s_PrintData) = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
    (*s_pageSetupData) = pageSetupDialog.GetPageSetupDialogData();
}


void DIALOG_PRINT_USING_PRINTER::OnPrintPreview( wxCommandEvent& event )
{
    SetPrintParameters();

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in Pcbnew:
    if( s_Parameters.m_PrintMaskLayer == 0 )
    {
        DisplayError( this, _( "No layer selected" ) );
        return;
    }

    // Pass two printout objects: for preview, and possible printing.
    wxString title = _( "Print Preview" );
    wxPrintPreview* preview =
            new wxPrintPreview( createPrintout( title ), createPrintout( title ), s_PrintData );

    preview->SetZoom( 100 );

    wxPreviewFrame* frame = new wxPreviewFrame( preview, this, title, m_parent->GetPosition(),
                                                m_parent->GetSize() );
    frame->SetMinSize( wxSize( 550, 350 ) );
    frame->Center();

    // On wxGTK, set the flag wxTOPLEVEL_EX_DIALOG is mandatory, if we want
    // close the frame using the X box in caption, when the preview frame is run
    // from a dialog
    frame->SetExtraStyle( frame->GetExtraStyle() | wxTOPLEVEL_EX_DIALOG );

    // We use here wxPreviewFrame_WindowModal option to make the wxPrintPreview frame
    // modal for its caller only.
    // An other reason is the fact when closing the frame without this option,
    // all top level frames are reenabled.
    // With this option, only the parent is reenabled.
    // Reenabling all top level frames should be made by the parent dialog.
    frame->InitializeWithModality( wxPreviewFrame_WindowModal );

    frame->Raise(); // Needed on Ubuntu/Unity to display the frame
    frame->Show( true );
}


void DIALOG_PRINT_USING_PRINTER::OnPrintButtonClick( wxCommandEvent& event )
{
    SetPrintParameters();

    // If no layer selected, we have no plot. prompt user if it happens
    // because he could think there is a bug in Pcbnew:
    if( s_Parameters.m_PrintMaskLayer == 0 )
    {
        DisplayError( this, _( "No layer selected" ) );
        return;
    }

    wxPrintDialogData printDialogData( *s_PrintData );
    printDialogData.SetMaxPage( s_Parameters.m_PageCount );

    wxPrinter printer( &printDialogData );
    auto printout = std::unique_ptr<PCBNEW_PRINTOUT>( createPrintout( _( "Print" ) ) );

    // Disable 'Print' button to prevent issuing another print
    // command before the previous one is finished (causes problems on Windows)
    ENABLER printBtnDisable( *m_sdbSizer1OK, false );

    if( !printer.Print( this, printout.get(), true ) )
    {
        if( wxPrinter::GetLastError() == wxPRINTER_ERROR )
            DisplayError( this, _( "There was a problem printing." ) );
    }
    else
    {
        *s_PrintData = printer.GetPrintDialogData().GetPrintData();
    }
}
