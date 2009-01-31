/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_svg_print.cpp
// Author:      jean-pierre Charras
// Modified by:
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#include "dialog_SVG_print_base.h"

#include "dcsvg.h"

#include "program.h"
#include "general.h"

// Keys for configuration
#define PLOTSVGMODECOLOR_KEY  wxT( "PlotSVGModeColor" )

extern BASE_SCREEN* ActiveScreen;
#define WIDTH_MAX_VALUE 100
#define WIDTH_MIN_VALUE 1

// Variables locales
static bool s_Print_Frame_Ref     = true;
static int  s_PlotBlackAndWhite = 0;

class DIALOG_SVG_PRINT : public DIALOG_SVG_PRINT_base
{
private:
    WinEDA_DrawFrame * m_Parent;
    int m_ImageXSize_mm;
    wxConfig* m_Config;

public:
    DIALOG_SVG_PRINT( WinEDA_DrawFrame* parent );
    ~DIALOG_SVG_PRINT( ) {}

private:
	void OnCloseWindow( wxCloseEvent& event );
	void OnInitDialog( wxInitDialogEvent& event );
	void OnButtonPlotCurrentClick( wxCommandEvent& event );
	void OnButtonPlotAllClick( wxCommandEvent& event );
	void OnButtonCancelClick( wxCommandEvent& event );
    void OnSetColorModeSelected( wxCommandEvent& event );
    void SetPenWidth();
    void PrintSVGDoc( bool aPrintAll, bool aPrint_Sheet_Ref );
    bool DrawPage( const wxString& FullFileName, BASE_SCREEN* screen, bool aPrint_Sheet_Ref );
};

/*******************************************************/
void WinEDA_DrawFrame::SVG_Print( wxCommandEvent& event )
/*******************************************************/

/* Prepare les structures de donnees de gestion de l'impression
  * et affiche la fenetre de dialogue de gestion de l'impression des feuilles
 */
{
    DIALOG_SVG_PRINT frame( this );

    frame.ShowModal();
}


/*!
 * DIALOG_SVG_PRINT functions
 */
DIALOG_SVG_PRINT::DIALOG_SVG_PRINT( WinEDA_DrawFrame* parent )
    : DIALOG_SVG_PRINT_base( parent )
{
    m_Parent = parent;
    m_Config = wxGetApp().m_EDA_Config;
}


/*************************************************************/
void DIALOG_SVG_PRINT::OnInitDialog( wxInitDialogEvent& event )
/*************************************************************/
{
    SetFont( *g_DialogFont );
    SetFocus();     // Make ESC key working

    m_ImageXSize_mm = 270;
    if( m_Config )
    {
        m_Config->Read( OPTKEY_PLOT_LINEWIDTH_VALUE, &g_PlotLine_Width );
        m_Config->Read( PLOTSVGMODECOLOR_KEY, &s_PlotBlackAndWhite );
    }

    AddUnitSymbol(* m_TextPenWidth, g_UnitMetric );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue(g_UnitMetric, g_PlotLine_Width, m_Parent->m_InternalUnits ) );
    m_Print_Sheet_Ref->SetValue( s_Print_Frame_Ref );
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
}


/********************************************/
void DIALOG_SVG_PRINT::SetPenWidth()
/********************************************/
{
    g_PlotLine_Width = ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->m_InternalUnits );

    if( g_PlotLine_Width > WIDTH_MAX_VALUE )
    {
        g_PlotLine_Width = WIDTH_MAX_VALUE;
    }

    if( g_PlotLine_Width < WIDTH_MIN_VALUE )
    {
        g_PlotLine_Width = WIDTH_MIN_VALUE;
    }

    m_DialogPenWidth->SetValue(
        ReturnStringFromValue(g_UnitMetric, g_PlotLine_Width, m_Parent->m_InternalUnits ) );
}


/***************************************************************************/
void DIALOG_SVG_PRINT::PrintSVGDoc( bool aPrintAll, bool aPrint_Sheet_Ref )
/***************************************************************************/
{
    wxString msg;

    SetPenWidth();

    wxString FullFileName;
    BASE_SCREEN* screen    = m_Parent->GetBaseScreen();
    BASE_SCREEN* oldscreen = screen;

    if( aPrintAll && m_Parent->m_Ident == SCHEMATIC_FRAME )
    {
        WinEDA_SchematicFrame* schframe     = (WinEDA_SchematicFrame*) m_Parent;
        DrawSheetPath* sheetpath, *oldsheetpath = schframe->GetSheet();
        SCH_SCREEN*            schscreen       = schframe->GetScreen();
        oldscreen    = schscreen;
        EDA_SheetList SheetList( NULL );
        sheetpath = SheetList.GetFirst();
        DrawSheetPath list;

        for( ; ;  )
        {
            if( sheetpath == NULL )
                break;
            list.Clear();
            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                schframe->m_CurrentSheet = &list;
                schframe->m_CurrentSheet->UpdateAllScreenReferences();
                schframe->SetSheetNumberAndCount();
                schscreen = schframe->m_CurrentSheet->LastScreen();
                ActiveScreen = schscreen;
            }
            else  // Should not happen
                return;
            sheetpath = SheetList.GetNext();

            FullFileName = schframe->GetUniqueFilenameForCurrentSheet( ) + wxT( ".svg" );

            bool     success = DrawPage( FullFileName, schscreen, aPrint_Sheet_Ref );
            msg = _( "Create file " ) + FullFileName;
            if( !success )
                msg += _( " error" );
            msg += wxT( "\n" );
            m_MessagesBox->AppendText( msg );
        }
        schframe->m_CurrentSheet = oldsheetpath;
        schframe->m_CurrentSheet->UpdateAllScreenReferences();
        schframe->SetSheetNumberAndCount();
    }
    else
    {
        FullFileName = m_FileNameCtrl->GetValue();
        if( FullFileName.IsEmpty() )
        {
            FullFileName = screen->m_FileName;
            ChangeFileNameExt( FullFileName, wxT( ".svg" ) );
        }
        bool     success = DrawPage( FullFileName, screen, aPrint_Sheet_Ref );
        msg = _( "Create file " ) + FullFileName;
        if( !success )
            msg += _( " error" );
        msg += wxT( "\n" );
        m_MessagesBox->AppendText( msg );
    }
    ActiveScreen = oldscreen;
}


/*****************************************************************/
bool DIALOG_SVG_PRINT::DrawPage( const wxString& FullFileName, BASE_SCREEN* screen
            , bool aPrint_Sheet_Ref)
/*****************************************************************/

/*
  * Routine effective d'impression
 */
{
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxSize  SheetSize;  // Sheet size in internal units
    wxPoint old_org;
    float   dpi;
    bool    success = true;

    /* modification des cadrages et reglages locaux */
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

    wxSVGFileDC dc( FullFileName, SheetSize.x, SheetSize.y, dpi );

    if( !dc.Ok() )
    {
        DisplayError( this, wxT( "SVGprint error: wxSVGFileDC not OK" ) );
        success = FALSE;
    }
    else
    {
        EDA_Rect tmp = panel->m_ClipBox;
        GRResetPenAndBrush( &dc );
        g_PlotLine_Width =  ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->m_InternalUnits );
        SetPenMinWidth( g_PlotLine_Width );
        GRForceBlackPen( m_ModeColorOption->GetSelection() == 0 ? FALSE : true );


        panel->m_ClipBox.SetX( 0 ); panel->m_ClipBox.SetY( 0 );
        panel->m_ClipBox.SetWidth( 0x7FFFFF0 ); panel->m_ClipBox.SetHeight( 0x7FFFFF0 );

        g_IsPrinting = true;
        SetLocaleTo_C_standard( );   // Switch the locale to standard C (needed to print floating point numbers like 1.3)
        panel->PrintPage( &dc, aPrint_Sheet_Ref, 1, false );
        SetLocaleTo_Default( );    // revert to the current  locale
        g_IsPrinting     = FALSE;
        panel->m_ClipBox = tmp;
    }


    GRForceBlackPen( FALSE );
    SetPenMinWidth( 1 );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );

    return success;
}


/********************************************************************/
void DIALOG_SVG_PRINT::OnButtonPlotAllClick( wxCommandEvent& event )
/********************************************************************/
{
    PrintSVGDoc( true, m_Print_Sheet_Ref->GetValue() );
}

/********************************************************************/
void DIALOG_SVG_PRINT::OnButtonPlotCurrentClick( wxCommandEvent& event )
/********************************************************************/
{
    PrintSVGDoc( false, m_Print_Sheet_Ref->GetValue() );
}


/******************************************************************/
void DIALOG_SVG_PRINT::OnButtonCancelClick( wxCommandEvent& event )
/******************************************************************/
{
    Close( );
}


/***********************************************************/
void DIALOG_SVG_PRINT::OnCloseWindow( wxCloseEvent& event )
/***********************************************************/
{
    if( m_Config )
    {
        s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
        m_Config->Write( OPTKEY_PLOT_LINEWIDTH_VALUE, g_PlotLine_Width );
        m_Config->Write( PLOTSVGMODECOLOR_KEY, s_PlotBlackAndWhite );
    }
    EndModal( 0 );
}

/*********************************************************************/
void DIALOG_SVG_PRINT::OnSetColorModeSelected( wxCommandEvent& event )
/*********************************************************************/
/* called on radiobox color/black and white selection
*/
{
    s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
}
