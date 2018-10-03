/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2016 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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
#include <footprint_edit_frame.h>
#include <base_units.h>
#include <pcbnew_printout.h>
#include <pcbnew.h>
#include <pcbplot.h>
#include <class_board.h>
#include <enabler.h>
#include <wx/valnum.h>
#include <widgets/unit_binder.h>

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

#include <dialog_print_pcbnew_base.h>

#define PEN_WIDTH_MAX_VALUE ( KiROUND( 5 * IU_PER_MM ) )
#define PEN_WIDTH_MIN_VALUE ( KiROUND( 0.005 * IU_PER_MM ) )


extern int g_DrawDefaultLineThickness;

// Define min and max reasonable values for print scale
#define MIN_SCALE 0.01
#define MAX_SCALE 100.0

// static print data and page setup data, to remember settings during the session
static wxPrintData* s_PrintData;
static wxPageSetupDialogData* s_pageSetupData = (wxPageSetupDialogData*) NULL;

static PRINT_PARAMETERS  s_Parameters;


/**
 * Dialog to print schematic. Class derived from DIALOG_PRINT_PCBNEW_BASE
 *  created by wxFormBuilder
 */
class DIALOG_PRINT_PCBNEW : public DIALOG_PRINT_PCBNEW_BASE
{
public:
    DIALOG_PRINT_PCBNEW( PCB_BASE_EDIT_FRAME* parent );
    ~DIALOG_PRINT_PCBNEW() override;

    /**
     * Set 'print border and title block' to a requested value and hides the
     * corresponding checkbox.
     */
    void ForcePrintBorder( bool aValue )
    {
        m_Print_Sheet_Ref->SetValue( aValue );
        m_Print_Sheet_Ref->Hide();
    }

private:
    PCB_BASE_EDIT_FRAME* m_parent;
    wxConfigBase*   m_config;
    // the list of existing board layers in wxCheckListBox, with the board layers id:
    std::pair<wxCheckListBox*, int> m_layers[PCB_LAYER_ID_COUNT];
    static bool     m_ExcludeEdgeLayer;
    wxFloatingPointValidator<double> m_scaleValidator;

    UNIT_BINDER     m_defaultPenWidth;

    bool TransferDataToWindow() override;

    void OnSelectAllClick( wxCommandEvent& event ) override;
    void OnDeselectAllClick( wxCommandEvent& event ) override;
    void OnSetCustomScale( wxCommandEvent& event ) override;
    void OnPageSetup( wxCommandEvent& event ) override;
    void OnPrintPreview( wxCommandEvent& event ) override;
    void OnPrintButtonClick( wxCommandEvent& event ) override;

    ///> (Un)checks all items in a checklist box
    void setListBoxValue( wxCheckListBox* aList, bool aValue )
    {
        for( int i = 0; i < aList->GetCount(); ++i )
            aList->Check( i, aValue );
    }

    void SetPrintParameters();
    int SetLayerSetFromListSelection();

    PCBNEW_PRINTOUT* createPrintout( const wxString& aTitle )
    {
        return new PCBNEW_PRINTOUT( m_parent->GetBoard(), s_Parameters,
            m_parent->GetGalCanvas()->GetView(), m_parent->GetPageSettings().GetSizeIU(), aTitle );
    }

    /**
     * Select a corresponing scale radio button and update custom scale value if needed.
     * @param aValue is the scale value to be selected (0 stands for fit-to-page).
     */
    void setScaleValue( double aValue ) const
    {
        wxASSERT( aValue >= 0.0 );

        if( aValue == 0.0 )
        {
            m_scaleFit->SetValue( true );
        }
        else if( aValue == 1.0 )
        {
            m_scale1->SetValue( true );
        }
        else
        {
            if( aValue > MAX_SCALE )
            {
                DisplayInfoMessage( NULL,
                        _( "Warning: Scale option set to a very large value" ) );
            }

            if( aValue < MIN_SCALE )
            {
                DisplayInfoMessage( NULL,
                        _( "Warning: Scale option set to a very small value" ) );
            }

            m_scaleCustom->SetValue( true );
            m_scaleCustomText->SetValue( wxString::Format( wxT( "%f" ), aValue ) );
        }
    }

    /**
     * Return scale value selected in the dialog.
     */
    double getScaleValue() const
    {
        if( m_scale1->GetValue() )
            return 1.0;

        if( m_scaleFit->GetValue() )
            return 0.0;

        if( m_scaleCustom->GetValue() )
        {
            double scale;

            wxCHECK( m_scaleCustomText->GetValue().ToDouble( &scale ), 1.0 );
            return scale;
        }

        wxCHECK( false, 1.0 );
    }
};


bool DIALOG_PRINT_PCBNEW::m_ExcludeEdgeLayer;


DIALOG_PRINT_PCBNEW::DIALOG_PRINT_PCBNEW( PCB_BASE_EDIT_FRAME* parent ) :
    DIALOG_PRINT_PCBNEW_BASE( parent ),
    m_parent( parent ),
    m_defaultPenWidth( parent, m_penWidthLabel, m_penWidthCtrl, m_penWidthUnits, true,
                       PEN_WIDTH_MIN_VALUE, PEN_WIDTH_MAX_VALUE )
{
    m_config = Kiface().KifaceSettings();
    memset( m_layers, 0, sizeof( m_layers ) );

    m_scaleValidator.SetRange( 1e-3, 1e3 );
    m_scaleCustomText->SetValidator( m_scaleValidator );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Print" ) );
    m_sdbSizer1Apply->SetLabel( _( "Print Preview" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();

#if defined(__WXMAC__) or defined(__WXGTK__)
    // Preview does not work well on GTK or Mac,
    // but these platforms provide native print preview
    m_sdbSizer1Apply->Hide();
#endif

    FinishDialogSettings();
}


DIALOG_PRINT_PCBNEW::~DIALOG_PRINT_PCBNEW()
{
    SetPrintParameters();

    if( m_config )
    {
        m_config->Write( OPTKEY_PRINT_SCALE, getScaleValue() );
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


bool DIALOG_PRINT_PCBNEW::TransferDataToWindow()
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
    double scale = 1.0;

    if( m_config )
    {
        m_config->Read( OPTKEY_PRINT_SCALE, &scale );
        m_config->Read( OPTKEY_PRINT_PAGE_FRAME, &s_Parameters.m_Print_Sheet_Ref, 1);
        m_config->Read( OPTKEY_PRINT_MONOCHROME_MODE, &s_Parameters.m_Print_Black_and_White, 1);
        m_config->Read( OPTKEY_PRINT_PAGE_PER_LAYER, &s_Parameters.m_OptionPrintPage, 0);
        int tmp;
        m_config->Read( OPTKEY_PRINT_PADS_DRILL,  &tmp, PRINT_PARAMETERS::SMALL_DRILL_SHAPE );
        s_Parameters.m_DrillShapeOpt = (PRINT_PARAMETERS::DrillShapeOptT) tmp;
    }

    setScaleValue( scale );
    s_Parameters.m_PrintScale = getScaleValue();
    m_Print_Mirror->SetValue(s_Parameters.m_PrintMirror);
    m_Exclude_Edges_Pcb->SetValue(m_ExcludeEdgeLayer);
    m_Print_Sheet_Ref->SetValue( s_Parameters.m_Print_Sheet_Ref );

    // Options to plot pads and vias holes
    m_drillMarksChoice->SetSelection( s_Parameters.m_DrillShapeOpt );

    m_outputMode->SetSelection( s_Parameters.m_Print_Black_and_White ? 1 : 0 );

    m_PagesOption->SetSelection( s_Parameters.m_OptionPrintPage );
    s_Parameters.m_PenDefaultSize = g_DrawDefaultLineThickness;
    m_defaultPenWidth.SetValue( s_Parameters.m_PenDefaultSize );

    // Update the layout when layers are added
    GetSizer()->Fit( this );

    return true;
}


int DIALOG_PRINT_PCBNEW::SetLayerSetFromListSelection()
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


void DIALOG_PRINT_PCBNEW::SetPrintParameters()
{
    PCB_PLOT_PARAMS plot_opts = m_parent->GetPlotSettings();

    s_Parameters.m_PrintMirror = m_Print_Mirror->GetValue();
    s_Parameters.m_Print_Sheet_Ref = m_Print_Sheet_Ref->GetValue();
    s_Parameters.m_Print_Black_and_White = m_outputMode->GetSelection() != 0;

    s_Parameters.m_DrillShapeOpt =
        (PRINT_PARAMETERS::DrillShapeOptT) m_drillMarksChoice->GetSelection();

    s_Parameters.m_OptionPrintPage = m_PagesOption->GetSelection() != 0;

    SetLayerSetFromListSelection();

    s_Parameters.m_PrintScale = getScaleValue();
    plot_opts.SetScale( s_Parameters.m_PrintScale );

    m_parent->SetPlotSettings( plot_opts );

    s_Parameters.m_PenDefaultSize = m_defaultPenWidth.GetValue();
    g_DrawDefaultLineThickness = s_Parameters.m_PenDefaultSize;
}


void DIALOG_PRINT_PCBNEW::OnSelectAllClick( wxCommandEvent& event )
{
    setListBoxValue( m_CopperLayersList, true );
    setListBoxValue( m_TechnicalLayersList, true );
}


void DIALOG_PRINT_PCBNEW::OnDeselectAllClick( wxCommandEvent& event )
{
    setListBoxValue( m_CopperLayersList, false );
    setListBoxValue( m_TechnicalLayersList, false );
}


void DIALOG_PRINT_PCBNEW::OnSetCustomScale( wxCommandEvent& event )
{
    // Select 'custom scale' radio button when user types in a value in the
    // custom scale text box
    m_scaleCustom->SetValue( true );
}


void DIALOG_PRINT_PCBNEW::OnPageSetup( wxCommandEvent& event )
{
    wxPageSetupDialog pageSetupDialog( this, s_pageSetupData );
    pageSetupDialog.ShowModal();

    (*s_PrintData) = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
    (*s_pageSetupData) = pageSetupDialog.GetPageSetupDialogData();
}


void DIALOG_PRINT_PCBNEW::OnPrintPreview( wxCommandEvent& event )
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


void DIALOG_PRINT_PCBNEW::OnPrintButtonClick( wxCommandEvent& event )
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


void PCB_BASE_EDIT_FRAME::preparePrintout()
{
    // Selection affects the original item visibility
    GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    const PAGE_INFO& pageInfo = GetPageSettings();

    if( s_PrintData == NULL )  // First print
    {
        s_PrintData = new wxPrintData();

        if( !s_PrintData->Ok() )
            DisplayError( this, _( "An error occurred initializing the printer information." ) );

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
}


void PCB_EDIT_FRAME::ToPrinter( wxCommandEvent& event )
{
    preparePrintout();
    DIALOG_PRINT_PCBNEW dlg( this );
    dlg.ShowModal();
}


void FOOTPRINT_EDIT_FRAME::ToPrinter( wxCommandEvent& event )
{
    preparePrintout();
    DIALOG_PRINT_PCBNEW dlg( this );
    dlg.ForcePrintBorder( false );
    dlg.ShowModal();
}
