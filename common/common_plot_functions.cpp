/**
 * @file common_plot_functions.cpp
 * @brief Kicad: Common plotting functions
 */

#include <fctsys.h>
#include <base_struct.h>
#include <plot_common.h>
#include <worksheet.h>
#include <class_base_screen.h>
#include <drawtxt.h>
#include <class_title_block.h>
#include "worksheet_shape_builder.h"


wxString GetDefaultPlotExtension( PlotFormat aFormat )
{
    switch( aFormat )
    {
    case PLOT_FORMAT_DXF:
        return DXF_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_POST:
        return PS_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_PDF:
        return PDF_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_HPGL:
        return HPGL_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_GERBER:
        return GERBER_PLOTTER::GetDefaultFileExtension();

    case PLOT_FORMAT_SVG:
        return SVG_PLOTTER::GetDefaultFileExtension();

    default:
        wxASSERT( false );
        return wxEmptyString;
    }
}



void PlotWorkSheet( PLOTTER* plotter, const TITLE_BLOCK& aTitleBlock,
                    const PAGE_INFO& aPageInfo,
                    int aSheetNumber, int aNumberOfSheets,
                    const wxString &aSheetDesc, const wxString &aFilename )
{
    /* Note: Page sizes values are given in mils
     */
    double   iusPerMil = plotter->GetIUsPerDecimil() * 10.0;
    wxSize   pageSize = aPageInfo.GetSizeMils();  // in mils

    wxPoint LTmargin;
    LTmargin.x = aPageInfo.GetLeftMarginMils() * iusPerMil;
    LTmargin.y = aPageInfo.GetTopMarginMils()  * iusPerMil;

    wxPoint RBmargin;
    RBmargin.x = aPageInfo.GetRightMarginMils() * iusPerMil;
    RBmargin.y = aPageInfo.GetBottomMarginMils()  * iusPerMil;

    EDA_COLOR_T plotColor = plotter->GetColorMode() ? RED : BLACK;
    plotter->SetColor( plotColor );
    plotter->SetCurrentLineWidth( PLOTTER::DEFAULT_LINE_WIDTH );
    WS_DRAW_ITEM_LIST drawList;

    drawList.BuildWorkSheetGraphicList( pageSize, LTmargin, RBmargin,
                               aPageInfo.GetType(), aFilename,
                               aSheetDesc,
                               aTitleBlock, aNumberOfSheets, aSheetNumber,
                               PLOTTER::DEFAULT_LINE_WIDTH, iusPerMil,
                               plotColor, plotColor );

    // Draw item list
    for( WS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item;
         item = drawList.GetNext() )
    {
        switch( item->GetType() )
        {
        case WS_DRAW_ITEM_BASE::wsg_line:
            {
                WS_DRAW_ITEM_LINE* line = (WS_DRAW_ITEM_LINE*) item;
                plotter->MoveTo( line->GetStart() );
                plotter->FinishTo( line->GetEnd() );
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_rect:
            {
                WS_DRAW_ITEM_RECT* rect = (WS_DRAW_ITEM_RECT*) item;
                plotter->Rect( rect->GetStart(), rect->GetEnd(), NO_FILL );           }
            break;

        case WS_DRAW_ITEM_BASE::wsg_text:
            {
                WS_DRAW_ITEM_TEXT* text = (WS_DRAW_ITEM_TEXT*) item;
                plotter->Text( text->GetTextPosition(), text->GetColor(),
                               text->GetText(), text->GetOrientation(),
                               text->GetSize(),
                               text->GetHorizJustify(), text->GetVertJustify(),
                               text->GetPenWidth(),
                               text->IsItalic(), text->IsBold() );
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_poly:
            {
                WS_DRAW_ITEM_POLYGON* poly = (WS_DRAW_ITEM_POLYGON*) item;
                plotter->PlotPoly( poly->m_Corners, NO_FILL );
            }
            break;
        }
    }
}
