/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_svg_print.cpp
// Author:      jean-pierre Charras
// Modified by:
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "dialog_SVG_print_base.h"

#include "dcsvg.h"

#include "pcbnew.h"
#include "pcbplot.h"

// Keys for configuration
#define PLOTSVGMODECOLOR_KEY wxT( "PlotSVGModeColor" )

// reasonnable values for default pen width (in 1/10000 inch)
#define WIDTH_MAX_VALUE 500
#define WIDTH_MIN_VALUE 1

// Local variables:
static int  s_PrintPenMinWidth  = 1;
static bool s_Print_Frame_Ref   = true;
static int  s_PlotBlackAndWhite = 0;
static long s_SelectedLayers    = LAYER_BACK | LAYER_FRONT |
                                  SILKSCREEN_LAYER_FRONT | SILKSCREEN_LAYER_BACK;

class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
private:
    WinEDA_BasePcbFrame* m_Parent;
    int m_ImageXSize_mm;
    wxConfig*            m_Config;
    long m_PrintMaskLayer;
    wxCheckBox*          m_BoxSelectLayer[32];

public:
    DIALOG_SVG_PRINT( WinEDA_DrawFrame* parent );
    ~DIALOG_SVG_PRINT() {}

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnInitDialog( wxInitDialogEvent& event );
    void OnButtonPrintSelectedClick( wxCommandEvent& event );
    void OnButtonPrintBoardClick( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event );
    void OnSetColorModeSelected( wxCommandEvent& event );
    void SetPenWidth();
    void PrintSVGDoc( bool aPrintAll, bool aPrint_Framet_Ref );
    bool DrawPage( const wxString& FullFileName, BASE_SCREEN* screen,
                   bool aPrint_Framet_Ref );
};


/* Prepare the data structures of print management
 * And displays the dialog window management of printing sheets
 */
void WinEDA_DrawFrame::SVG_Print( wxCommandEvent& event )
{
    DIALOG_SVG_PRINT frame( this );

    frame.ShowModal();
}


/*!
 * DIALOG_SVG_PRINT functions
 */
DIALOG_SVG_PRINT::DIALOG_SVG_PRINT( WinEDA_DrawFrame* parent ) :
    DIALOG_SVG_PRINT_base( parent )
{
    m_Parent = (WinEDA_BasePcbFrame*) parent;
    m_Config = wxGetApp().m_EDA_Config;
}


void DIALOG_SVG_PRINT::OnInitDialog( wxInitDialogEvent& event )
{
    SetFocus();     // Make ESC key working

    m_ImageXSize_mm = 270;
    if( m_Config )
    {
        m_Config->Read( OPTKEY_PLOT_LINEWIDTH_VALUE, &s_PrintPenMinWidth );
        m_Config->Read( PLOTSVGMODECOLOR_KEY, &s_PlotBlackAndWhite );
    }

    AddUnitSymbol( *m_TextPenWidth, g_UnitMetric );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UnitMetric, s_PrintPenMinWidth,
                               m_Parent->m_InternalUnits ) );

    m_Print_Frame_Ref_Ctrl->SetValue( s_Print_Frame_Ref );

    // Create layers list
    BOARD* board = m_Parent->GetBoard();
    int    mask  = 1;
    for( int layer = 0; layer<NB_LAYERS; layer++, mask <<= 1 )
    {
        m_BoxSelectLayer[layer] =
            new  wxCheckBox( this, -1, board->GetLayerName( layer ) );

        if( mask & s_SelectedLayers )
            m_BoxSelectLayer[layer]->SetValue( true );

        if( layer < 16 )
            m_CopperLayersBoxSizer->Add(  m_BoxSelectLayer[layer],
                                          0,
                                          wxGROW | wxALL,
                                          1 );
        else
            m_TechnicalBoxSizer->Add(  m_BoxSelectLayer[layer],
                                       0,
                                       wxGROW | wxALL,
                                       1 );
    }

    if( m_Config )
    {
        wxString layerKey;

        for( int layer = 0;  layer<NB_LAYERS;  ++layer )
        {
            bool option;

            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            if( m_Config->Read( layerKey, &option ) )
                m_BoxSelectLayer[layer]->SetValue( option );
        }
    }

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


void DIALOG_SVG_PRINT::SetPenWidth()
{
    s_PrintPenMinWidth = ReturnValueFromTextCtrl( *m_DialogPenWidth,
                                                  m_Parent->m_InternalUnits );

    if( s_PrintPenMinWidth > WIDTH_MAX_VALUE )
    {
        s_PrintPenMinWidth = WIDTH_MAX_VALUE;
    }

    if( s_PrintPenMinWidth < WIDTH_MIN_VALUE )
    {
        s_PrintPenMinWidth = WIDTH_MIN_VALUE;
    }

    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UnitMetric, s_PrintPenMinWidth,
                               m_Parent->m_InternalUnits ) );
}


void DIALOG_SVG_PRINT::PrintSVGDoc( bool aPrintAll, bool aPrint_Framet_Ref )
{
    wxFileName fn;
    wxString   msg;

    SetPenWidth();

    BASE_SCREEN* screen = m_Parent->GetBaseScreen();

    if( aPrintAll )
        m_PrintMaskLayer = 0xFFFFFFFF;

    for( int layer = 0; layer<NB_LAYERS; layer++ )
    {
        if( !aPrintAll && !m_BoxSelectLayer[layer]->GetValue() )
            continue;

        fn = m_FileNameCtrl->GetValue();
        if( !fn.IsOk() )
        {
            fn = screen->m_FileName;
        }

        if( aPrintAll )
            fn.SetName( fn.GetName() + wxT( "-brd" ) );
        else
        {
            fn.SetName( fn.GetName() + wxT( "-" ) +
                        m_BoxSelectLayer[layer]->GetLabel() );

            m_PrintMaskLayer = 1 << layer;
            if( m_PrintBoardEdgesCtrl->IsChecked() )
                m_PrintMaskLayer |= EDGE_LAYER;
        }

        fn.SetExt( wxT( "svg" ) );

        bool success = DrawPage( fn.GetFullPath(), screen, aPrint_Framet_Ref );
        msg = _( "Create file " ) + fn.GetFullPath();
        if( !success )
            msg += _( " error" );
        msg += wxT( "\n" );
        m_MessagesBox->AppendText( msg );

        if( aPrintAll )
            break;
    }
}


/*
 * Routine actual print
 */
bool DIALOG_SVG_PRINT::DrawPage( const wxString& FullFileName,
                                 BASE_SCREEN*    screen,
                                 bool            aPrint_Framet_Ref )
{
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxSize  SheetSize;  // Sheet size in internal units
    wxPoint old_org;
    float   dpi;
    bool    success = true;

    /* Change frames and local settings */
    tmp_startvisu = screen->m_StartVisu;
    tmpzoom = screen->GetZoom();
    old_org = screen->m_DrawOrg;
    screen->m_DrawOrg.x   = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x = screen->m_StartVisu.y = 0;
    SheetSize    = screen->m_CurrentSheetDesc->m_Size;  // size in 1/1000 inch
    SheetSize.x *= m_Parent->m_InternalUnits / 1000;
    SheetSize.y *= m_Parent->m_InternalUnits / 1000;    // size in pixels

    screen->SetScalingFactor( 1.0 );
    dpi = (float) SheetSize.x * 25.4 / m_ImageXSize_mm;

    WinEDA_DrawPanel* panel = m_Parent->DrawPanel;

    wxSVGFileDC       dc( FullFileName, SheetSize.x, SheetSize.y, dpi );

    EDA_Rect          tmp = panel->m_ClipBox;
    GRResetPenAndBrush( &dc );
    s_PrintPenMinWidth = ReturnValueFromTextCtrl( *m_DialogPenWidth,
                                                  m_Parent->m_InternalUnits );
    SetPenMinWidth( s_PrintPenMinWidth );
    GRForceBlackPen( m_ModeColorOption->GetSelection() == 0 ? FALSE : true );


    panel->m_ClipBox.SetX( 0 ); panel->m_ClipBox.SetY( 0 );
    panel->m_ClipBox.SetWidth( 0x7FFFFF0 ); panel->m_ClipBox.SetHeight(
        0x7FFFFF0 );

    screen->m_IsPrinting = true;
    SetLocaleTo_C_standard();       // Switch the locale to standard C (needed
                                    // to print floating point numbers like
                                    // 1.3)
    panel->PrintPage( &dc, aPrint_Framet_Ref, m_PrintMaskLayer, false );
    SetLocaleTo_Default();          // revert to the current  locale
    screen->m_IsPrinting = false;
    panel->m_ClipBox     = tmp;

    GRForceBlackPen( FALSE );
    SetPenMinWidth( 1 );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );

    return success;
}


void DIALOG_SVG_PRINT::OnButtonPrintBoardClick( wxCommandEvent& event )
{
    s_Print_Frame_Ref = m_Print_Frame_Ref_Ctrl->IsChecked();
    PrintSVGDoc( true, s_Print_Frame_Ref );
}


void DIALOG_SVG_PRINT::OnButtonPrintSelectedClick( wxCommandEvent& event )
{
    s_Print_Frame_Ref = m_Print_Frame_Ref_Ctrl->IsChecked();
    PrintSVGDoc( false, s_Print_Frame_Ref );
}


void DIALOG_SVG_PRINT::OnButtonCancelClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SVG_PRINT::OnCloseWindow( wxCloseEvent& event )
{
    SetPenWidth();
    s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
    if( m_Config )
    {
        m_Config->Write( OPTKEY_PLOT_LINEWIDTH_VALUE, s_PrintPenMinWidth );
        m_Config->Write( PLOTSVGMODECOLOR_KEY, s_PlotBlackAndWhite );
        wxString layerKey;
        for( int layer = 0;  layer<NB_LAYERS;  ++layer )
        {
            layerKey.Printf( OPTKEY_LAYERBASE, layer );
            m_Config->Write( layerKey, m_BoxSelectLayer[layer]->IsChecked() );
        }
    }
    EndModal( 0 );
}


/* called on radiobox color/black and white selection
 */
void DIALOG_SVG_PRINT::OnSetColorModeSelected( wxCommandEvent& event )
{
    s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
}
