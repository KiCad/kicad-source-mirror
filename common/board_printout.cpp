/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
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

#include <board_printout.h>

#include <view/view.h>
#include <gal/gal_print.h>
#include <painter.h>

PRINT_PARAMETERS::PRINT_PARAMETERS()
{
    // TODO Millimeter2iu is depends on PCBNEW/GERBVIEW #ifdefs, so it cannot be compiled to libcommon
    //m_PenDefaultSize        = Millimeter2iu( 0.2 ); // A reasonable default value to draw items
                                    // which do not have a specified line width
    m_PenDefaultSize        = 0.0;
    m_PrintScale            = 1.0;
    m_XScaleAdjust          = 1.0;
    m_YScaleAdjust          = 1.0;
    m_Print_Sheet_Ref       = false;
    m_PrintMaskLayer.set();
    m_PrintMirror           = false;
    m_Print_Black_and_White = true;
    m_OptionPrintPage       = 1;
    m_PageCount             = 1;
    m_ForceCentered         = false;
    m_Flags                 = 0;
    m_DrillShapeOpt         = PRINT_PARAMETERS::SMALL_DRILL_SHAPE;
    m_PageSetupData         = NULL;
}


BOARD_PRINTOUT::BOARD_PRINTOUT( const PRINT_PARAMETERS& aParams, const KIGFX::VIEW* aView,
        const wxSize& aSheetSize, const wxString& aTitle ) :
    wxPrintout( aTitle )
{
    m_view = aView;
    m_PrintParams = aParams;
    m_sheetSize = aSheetSize;
}


void BOARD_PRINTOUT::GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
{
    *minPage     = 1;
    *selPageFrom = 1;

    int icnt = 1;

    if( m_PrintParams.m_OptionPrintPage == 0 )
        icnt = m_PrintParams.m_PageCount;

    *maxPage   = icnt;
    *selPageTo = icnt;
}


bool BOARD_PRINTOUT::HasPage( int aPage )
{
    if( aPage <= m_PrintParams.m_PageCount )
        return true;
    else
        return false;
}


void BOARD_PRINTOUT::DrawPage( const wxString& aLayerName, int aPageNum, int aPageCount )
{
    auto dc = GetDC();
    KIGFX::GAL_DISPLAY_OPTIONS options;
    auto galPrint = KIGFX::GAL_PRINT::Create( options, dc );
    auto gal = galPrint->GetGAL();
    auto printCtx = galPrint->GetPrintCtx();
    auto painter = getPainter( gal );
    std::unique_ptr<KIGFX::VIEW> view( m_view->DataReference() );

    wxRect page = GetLogicalPageRect();
    galPrint->SetSheetSize( VECTOR2D( page.width / dc->GetPPI().x, page.height / dc->GetPPI().y ) );

    view->SetGAL( gal );
    view->SetPainter( painter.get() );
    view->SetScaleLimits( 10e9, 0.0001 );
    view->SetScale( 1.0 );

    setupViewLayers( view, m_PrintParams.m_PrintMaskLayer );

    BOX2I bBox; // determine printout bounding box

    if( m_PrintParams.PrintBorderAndTitleBlock() )
    {
        bBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( m_sheetSize ) );
        view->SetLayerVisible( LAYER_WORKSHEET, true );
    }
    else
    {
        EDA_RECT targetBbox = getBoundingBox();
        bBox = BOX2I( targetBbox.GetOrigin(), targetBbox.GetSize() );
        view->SetLayerVisible( LAYER_WORKSHEET, false );
    }


    double scale = m_PrintParams.m_PrintScale;

    if( m_PrintParams.m_PrintScale == 0.0 )
    {
        // Fit to page
        scale = m_sheetSize.GetWidth() / bBox.GetWidth();
    }
    else if( m_PrintParams.m_PrintScale == 1.0 )
    {
        // TODO "accurate scale" that allows the user to specify custom scale
        // I think it should be renamed to "custom scale", and "approx. scale 1" should be replaced with "Scale 1"
        // TODO do not separate X and Y scale adjustments
        scale = m_PrintParams.m_XScaleAdjust;
    }


    // TODO fix 'Preview' button
    VECTOR2D nps( GetLogicalPageRect().width, GetLogicalPageRect().height );

    galPrint->SetNativePaperSize( VECTOR2D( nps.x / dc->GetPPI().x, nps.y / dc->GetPPI().y ),
            printCtx->HasNativeLandscapeRotation() );
    gal->SetLookAtPoint( bBox.Centre() );
    gal->SetZoomFactor( scale );

    {
    KIGFX::GAL_DRAWING_CONTEXT ctx( gal );
    view->Redraw();
    }
}


void BOARD_PRINTOUT::setupViewLayers( const std::unique_ptr<KIGFX::VIEW>& aView,
        const LSET& aLayerSet )
{
    // Disable all layers by default, let specific implementions enable required layers
    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; ++i )
    {
        aView->SetLayerVisible( i, false );
        aView->SetLayerTarget( i, KIGFX::TARGET_NONCACHED );
    }
}
