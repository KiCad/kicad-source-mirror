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
#include "printout_controler.h"

// Keys for configuration
#define PLOTSVGMODECOLOR_KEY wxT( "PlotSVGModeColor" )

// reasonnable values for default pen width (in 1/10000 inch)
#define WIDTH_MAX_VALUE 500
#define WIDTH_MIN_VALUE 1

extern int g_DrawDefaultLineThickness;

// Local variables:
static PRINT_PARAMETERS  s_Parameters;
static long s_SelectedLayers    = LAYER_BACK | LAYER_FRONT |
                                  SILKSCREEN_LAYER_FRONT | SILKSCREEN_LAYER_BACK;

class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
private:
    WinEDA_BasePcbFrame* m_Parent;
    wxConfig*            m_Config;
    long m_PrintMaskLayer;
    wxCheckBox*          m_BoxSelectLayer[32];

public:
    DIALOG_SVG_PRINT( WinEDA_DrawFrame* parent );
    ~DIALOG_SVG_PRINT() {}

private:
    void OnCloseWindow( wxCloseEvent& event );
    void initDialog( );
    void OnButtonPrintSelectedClick( wxCommandEvent& event );
    void OnButtonPrintBoardClick( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event );
    void OnSetColorModeSelected( wxCommandEvent& event );
    void SetPenWidth();
    void PrintSVGDoc( bool aPrintAll, bool aPrint_Frame_Ref );
    bool DrawPage( const wxString& FullFileName, BASE_SCREEN* screen,
                   bool aPrint_Frame_Ref );
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
    initDialog( );
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_SVG_PRINT::initDialog( )
{
    SetFocus();     // Make ESC key working

    if( m_Config )
    {
        m_Config->Read( PLOTSVGMODECOLOR_KEY, &s_Parameters.m_Print_Black_and_White );
    }

    s_Parameters.m_PenDefaultSize = g_DrawDefaultLineThickness;
    AddUnitSymbol( *m_TextPenWidth, g_UserUnit );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, s_Parameters.m_PenDefaultSize,
                               m_Parent->m_InternalUnits ) );

    m_Print_Frame_Ref_Ctrl->SetValue( s_Parameters.m_Print_Sheet_Ref );

    // Create layers list
    BOARD* board = m_Parent->GetBoard();
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
    DECLARE_LAYERS_ORDER_LIST(layersOrder);
    for( int layer_idx = 0; layer_idx < NB_LAYERS; ++layer_idx )
    {
        layer = layersOrder[layer_idx];

        wxASSERT(layer < NB_LAYERS);

        if( m_BoxSelectLayer[layer] == NULL )
            continue;

        long mask = 1 << layer;
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
            if ( m_BoxSelectLayer[layer] == NULL )
                continue;
            layerKey.Printf( OPTKEY_LAYERBASE, layer );

            if( m_Config->Read( layerKey, &option ) )
                m_BoxSelectLayer[layer]->SetValue( option );
        }
    }
}


void DIALOG_SVG_PRINT::SetPenWidth()
{
    s_Parameters.m_PenDefaultSize = ReturnValueFromTextCtrl( *m_DialogPenWidth,
                                                  m_Parent->m_InternalUnits );

    if( s_Parameters.m_PenDefaultSize > WIDTH_MAX_VALUE )
    {
        s_Parameters.m_PenDefaultSize = WIDTH_MAX_VALUE;
    }

    if( s_Parameters.m_PenDefaultSize < WIDTH_MIN_VALUE )
    {
        s_Parameters.m_PenDefaultSize = WIDTH_MIN_VALUE;
    }

    g_DrawDefaultLineThickness = s_Parameters.m_PenDefaultSize;
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, s_Parameters.m_PenDefaultSize,
                               m_Parent->m_InternalUnits ) );
}


void DIALOG_SVG_PRINT::PrintSVGDoc( bool aPrintAll, bool aPrint_Frame_Ref )
{
    wxFileName fn;
    wxString   msg;

    SetPenWidth();

    BASE_SCREEN* screen = m_Parent->GetBaseScreen();

    if( aPrintAll )
        m_PrintMaskLayer = 0xFFFFFFFF;
    else
        m_PrintMaskLayer = 0;

    if( m_PrintBoardEdgesCtrl->IsChecked() )
            m_PrintMaskLayer |= EDGE_LAYER;
    else
            m_PrintMaskLayer &= ~EDGE_LAYER;

    for( int layer = 0; layer<NB_LAYERS; layer++ )
    {
        if ( m_BoxSelectLayer[layer] == NULL )
            continue;

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

        bool success = DrawPage( fn.GetFullPath(), screen, aPrint_Frame_Ref );
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
 * Actual print function.
 */
bool DIALOG_SVG_PRINT::DrawPage( const wxString& FullFileName,
                                 BASE_SCREEN*    screen,
                                 bool            aPrint_Frame_Ref )
{
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxSize  SheetSize;  // Sheet size in internal units
    wxPoint old_org;
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
    float dpi = (float)m_Parent->m_InternalUnits;

    WinEDA_DrawPanel* panel = m_Parent->DrawPanel;

    SetLocaleTo_C_standard();       // Switch the locale to standard C (needed
                                    // to print floating point numbers like 1.3)
    wxSVGFileDC       dc( FullFileName, SheetSize.x, SheetSize.y, dpi );

    EDA_Rect          tmp = panel->m_ClipBox;
    GRResetPenAndBrush( &dc );
    GRForceBlackPen( m_ModeColorOption->GetSelection() == 0 ? false : true );
    s_Parameters.m_DrillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;


    panel->m_ClipBox.SetX( 0 );
    panel->m_ClipBox.SetY( 0 );
    panel->m_ClipBox.SetWidth( 0x7FFFFF0 );
    panel->m_ClipBox.SetHeight( 0x7FFFFF0 );

    screen->m_IsPrinting = true;

    int bg_color = g_DrawBgColor;
    g_DrawBgColor = WHITE;
    m_Parent->PrintPage( &dc, aPrint_Frame_Ref, m_PrintMaskLayer, false, &s_Parameters);
    g_DrawBgColor = bg_color;
    SetLocaleTo_Default();          // revert to the current  locale
    screen->m_IsPrinting = false;
    panel->m_ClipBox     = tmp;

    GRForceBlackPen( false );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );

    return success;
}


void DIALOG_SVG_PRINT::OnButtonPrintBoardClick( wxCommandEvent& event )
{
    s_Parameters.m_Print_Sheet_Ref = m_Print_Frame_Ref_Ctrl->IsChecked();
    PrintSVGDoc( true, s_Parameters.m_Print_Sheet_Ref );
}


void DIALOG_SVG_PRINT::OnButtonPrintSelectedClick( wxCommandEvent& event )
{
    s_Parameters.m_Print_Sheet_Ref = m_Print_Frame_Ref_Ctrl->IsChecked();
    PrintSVGDoc( false, s_Parameters.m_Print_Sheet_Ref );
}


void DIALOG_SVG_PRINT::OnButtonCancelClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SVG_PRINT::OnCloseWindow( wxCloseEvent& event )
{
    SetPenWidth();
    s_Parameters.m_Print_Black_and_White = m_ModeColorOption->GetSelection();
    if( m_Config )
    {
        m_Config->Write( PLOTSVGMODECOLOR_KEY, s_Parameters.m_Print_Black_and_White );
        wxString layerKey;
        for( int layer = 0; layer<NB_LAYERS;  ++layer )
        {
            if( m_BoxSelectLayer[layer] == NULL )
                continue;
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
    s_Parameters.m_Print_Black_and_White = m_ModeColorOption->GetSelection();
}
