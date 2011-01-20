/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_svg_print.cpp
// Author:      jean-pierre Charras
// Modified by:
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"
#include "dcsvg.h"
#include "general.h"
#include "libeditframe.h"
#include "sch_sheet_path.h"

#include "dialog_SVG_print.h"


// Keys for configuration
#define PLOTSVGMODECOLOR_KEY wxT( "PlotSVGModeColor" )

extern BASE_SCREEN* ActiveScreen;
#define WIDTH_MAX_VALUE 100
#define WIDTH_MIN_VALUE 1

// Variables locales
static bool s_Print_Frame_Ref      = true;
static int  s_PlotBlackAndWhite    = 0;


/*!
 * DIALOG_SVG_PRINT functions
 */
DIALOG_SVG_PRINT::DIALOG_SVG_PRINT( WinEDA_DrawFrame* parent ) :
    DIALOG_SVG_PRINT_base( parent )
{
    m_Parent   = parent;
    m_Config   = wxGetApp().m_EDA_Config;
}


void DIALOG_SVG_PRINT::OnInitDialog( wxInitDialogEvent& event )
{
    SetFocus();     // Make ESC key working

    if( m_Config )
    {
        m_Config->Read( PLOTSVGMODECOLOR_KEY, &s_PlotBlackAndWhite );
    }

    m_ModeColorOption->SetSelection( s_PlotBlackAndWhite );

    AddUnitSymbol( *m_TextPenWidth, g_UserUnit );
    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, g_DrawDefaultLineThickness,
                               m_Parent->m_InternalUnits ) );
    m_Print_Sheet_Ref->SetValue( s_Print_Frame_Ref );

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


void DIALOG_SVG_PRINT::SetPenWidth()
{
    g_DrawDefaultLineThickness =
        ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->m_InternalUnits );

    if( g_DrawDefaultLineThickness > WIDTH_MAX_VALUE )
    {
        g_DrawDefaultLineThickness = WIDTH_MAX_VALUE;
    }

    if( g_DrawDefaultLineThickness < WIDTH_MIN_VALUE )
    {
        g_DrawDefaultLineThickness = WIDTH_MIN_VALUE;
    }

    m_DialogPenWidth->SetValue(
        ReturnStringFromValue( g_UserUnit, g_DrawDefaultLineThickness,
                               m_Parent->m_InternalUnits ) );
}


void DIALOG_SVG_PRINT::PrintSVGDoc( bool aPrintAll, bool aPrint_Sheet_Ref )
{
    wxString    msg;
    wxFileName  fn;

    SetPenWidth();

    g_DrawDefaultLineThickness =
        ReturnValueFromTextCtrl( *m_DialogPenWidth, m_Parent->m_InternalUnits );

    BASE_SCREEN*    screen     = m_Parent->GetBaseScreen();
    BASE_SCREEN*    oldscreen  = screen;

    if( aPrintAll && m_Parent->m_Ident == SCHEMATIC_FRAME )
    {
        SCH_EDIT_FRAME*  schframe = (SCH_EDIT_FRAME*) m_Parent;
        SCH_SHEET_PATH*  sheetpath, * oldsheetpath = schframe->GetSheet();
        SCH_SCREEN*      schscreen = schframe->GetScreen();
        oldscreen = schscreen;
        SCH_SHEET_LIST   SheetList( NULL );
        sheetpath = SheetList.GetFirst();
        SCH_SHEET_PATH   list;

        for( ; ; )
        {
            if( sheetpath == NULL )
                break;
            list.Clear();

            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                schframe->m_CurrentSheet = &list;
                schframe->m_CurrentSheet->UpdateAllScreenReferences();
                schframe->SetSheetNumberAndCount();
                schscreen      = schframe->m_CurrentSheet->LastScreen();
                ActiveScreen   = schscreen;
            }
            else  // Should not happen
                return;

            sheetpath = SheetList.GetNext();

            fn = schframe->GetUniqueFilenameForCurrentSheet() + wxT( ".svg" );

            bool success = DrawSVGPage( m_Parent, fn.GetFullPath(), ( SCH_SCREEN* ) schscreen,
                                        m_ModeColorOption->GetSelection() == 0 ? false : true,
                                        aPrint_Sheet_Ref );
            msg = _( "Create file " ) + fn.GetFullPath();
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
        fn = m_FileNameCtrl->GetValue();

        if( !fn.IsOk() )
            fn = screen->GetFileName();

        fn.SetExt( wxT( "svg" ) );
        fn.MakeAbsolute();

        bool success = DrawSVGPage( m_Parent, fn.GetFullPath(), ( SCH_SCREEN* ) screen,
                                    m_ModeColorOption->GetSelection() == 0 ? false : true,
                                    aPrint_Sheet_Ref );
        msg = _( "Create file " ) + fn.GetFullPath();

        if( !success )
            msg += _( " error" );

        msg += wxT( "\n" );
        m_MessagesBox->AppendText( msg );
    }

    ActiveScreen = oldscreen;
}


bool DIALOG_SVG_PRINT::DrawSVGPage( WinEDA_DrawFrame* frame,
                                    const wxString&   FullFileName,
                                    SCH_SCREEN*       screen,
                                    bool              aPrintBlackAndWhite,
                                    bool              aPrint_Sheet_Ref )
{
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxSize  SheetSize;  // Sheet size in internal units
    wxPoint old_org;
    bool    success = true;

    tmp_startvisu = screen->m_StartVisu;
    tmpzoom    = screen->GetZoom();
    old_org    = screen->m_DrawOrg;
    screen->m_DrawOrg.x    = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x  = screen->m_StartVisu.y = 0;

    SheetSize = screen->ReturnPageSize();      // page size in 1/1000 inch, ie in internal units

    screen->SetScalingFactor( 1.0 );
    WinEDA_DrawPanel* panel = frame->DrawPanel;

    SetLocaleTo_C_standard();       // Switch the locale to standard C (needed
                                    // to print floating point numbers like 1.3)

    float       dpi = (float) frame->m_InternalUnits;
    wxSVGFileDC dc( FullFileName, SheetSize.x, SheetSize.y, dpi );

    EDA_Rect    tmp = panel->m_ClipBox;
    GRResetPenAndBrush( &dc );
    GRForceBlackPen( aPrintBlackAndWhite );


    panel->m_ClipBox.SetX( -0x3FFFFF0 );
    panel->m_ClipBox.SetY( -0x3FFFFF0 );
    panel->m_ClipBox.SetWidth( 0x7FFFFF0 );
    panel->m_ClipBox.SetHeight( 0x7FFFFF0 );

    screen->m_IsPrinting = true;
    screen->Draw( panel, &dc, GR_COPY );

    if( aPrint_Sheet_Ref )
        frame->TraceWorkSheet( &dc, screen, g_DrawDefaultLineThickness );

    SetLocaleTo_Default();       // revert to the current locale
    screen->m_IsPrinting   = false;
    panel->m_ClipBox       = tmp;


    GRForceBlackPen( false );

    screen->m_StartVisu    = tmp_startvisu;
    screen->m_DrawOrg      = old_org;
    screen->SetZoom( tmpzoom );

    return success;
}


void DIALOG_SVG_PRINT::OnButtonPlotAllClick( wxCommandEvent& event )
{
    PrintSVGDoc( true, m_Print_Sheet_Ref->GetValue() );
}


void DIALOG_SVG_PRINT::OnButtonPlotCurrentClick( wxCommandEvent& event )
{
    PrintSVGDoc( false, m_Print_Sheet_Ref->GetValue() );
}


void DIALOG_SVG_PRINT::OnButtonCancelClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SVG_PRINT::OnCloseWindow( wxCloseEvent& event )
{
    if( m_Config )
    {
        s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
        m_Config->Write( PLOTSVGMODECOLOR_KEY, s_PlotBlackAndWhite );
    }
    EndModal( 0 );
}


/* called on radiobox color/black and white selection
 */
void DIALOG_SVG_PRINT::OnSetColorModeSelected( wxCommandEvent& event )
{
    s_PlotBlackAndWhite = m_ModeColorOption->GetSelection();
}
