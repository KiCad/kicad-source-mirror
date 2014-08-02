/**
 * @file libedit_plot_component.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gr_basic.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <eeschema_id.h>
#include <class_sch_screen.h>

#include <general.h>
#include <libeditframe.h>
#include <class_library.h>
#include <dialogs/dialog_plot_schematic.h>

#include <boost/foreach.hpp>


void LIB_EDIT_FRAME::OnPlotCurrentComponent( wxCommandEvent& event )
{
    LIB_COMPONENT* cmp = GetComponent();
    wxString   FullFileName;
    wxString   file_ext;
    wxString   mask;

    if( cmp == NULL )
    {
        wxMessageBox( _( "No component" ) );
        return;
    }

    switch( event.GetId() )
    {
    case ID_LIBEDIT_GEN_PNG_FILE:
        {
            bool       fmt_is_jpeg = false; // could be selectable later. so keep this option.

            file_ext = fmt_is_jpeg ? wxT( "jpg" ) : wxT( "png" );
            mask     = wxT( "*." ) + file_ext;
            wxFileName fn( cmp->GetName() );
            fn.SetExt( file_ext );

            FullFileName = EDA_FileSelector( _( "Filename:" ), wxGetCwd(),
                                             fn.GetFullName(), file_ext, mask, this,
                                             wxFD_SAVE, true );

            if( FullFileName.IsEmpty() )
                return;

            // calling wxYield is mandatory under Linux, after closing the file selector dialog
            // to refresh the screen before creating the PNG or JPEG image from screen
            wxYield();
            CreatePNGorJPEGFile( FullFileName, fmt_is_jpeg );
        }
        break;

    case ID_LIBEDIT_GEN_SVG_FILE:
        {
            file_ext = wxT( "svg" );
            mask     = wxT( "*." ) + file_ext;
            wxFileName fn( cmp->GetName() );
            fn.SetExt( file_ext );
            FullFileName = EDA_FileSelector( _( "Filename:" ), wxGetCwd(),
                                             fn.GetFullName(), file_ext, mask, this,
                                             wxFD_SAVE, true );

            if( FullFileName.IsEmpty() )
                return;

            PAGE_INFO pageSave = GetScreen()->GetPageSettings();
            PAGE_INFO pageTemp = pageSave;

            wxSize componentSize = m_component->GetBoundingBox( m_unit, m_convert ).GetSize();

            // Add a small margin to the plot bounding box
            pageTemp.SetWidthMils(  int( componentSize.x * 1.2 ) );
            pageTemp.SetHeightMils( int( componentSize.y * 1.2 ) );

            GetScreen()->SetPageSettings( pageTemp );
            SVG_PlotComponent( FullFileName );
            GetScreen()->SetPageSettings( pageSave );
        }
        break;
    }
}


void LIB_EDIT_FRAME::CreatePNGorJPEGFile( const wxString& aFileName, bool aFmt_jpeg )
{
    wxSize     image_size = m_canvas->GetClientSize();

    wxClientDC dc( m_canvas );
    wxBitmap   bitmap( image_size.x, image_size.y );
    wxMemoryDC memdc;

    memdc.SelectObject( bitmap );
    memdc.Blit( 0, 0, image_size.x, image_size.y, &dc, 0, 0 );
    memdc.SelectObject( wxNullBitmap );

    wxImage image = bitmap.ConvertToImage();

    if( !image.SaveFile( aFileName, aFmt_jpeg ? wxBITMAP_TYPE_JPEG : wxBITMAP_TYPE_PNG ) )
    {
        wxString msg;
        msg.Printf( _( "Can't save file <%s>" ), GetChars( aFileName ) );
        wxMessageBox( msg );
    }

    image.Destroy();
}


void LIB_EDIT_FRAME::SVG_PlotComponent( const wxString& aFullFileName )
{
    const bool plotBW = false;
    const PAGE_INFO& pageInfo = GetScreen()->GetPageSettings();

    SVG_PLOTTER* plotter = new SVG_PLOTTER();
    plotter->SetPageSettings( pageInfo );
    plotter->SetDefaultLineWidth( GetDefaultLineThickness() );
    plotter->SetColorMode( plotBW );

    wxPoint plot_offset;
    const double scale = 1.0;
    plotter->SetViewport( plot_offset, IU_PER_DECIMILS, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    if( ! plotter->OpenFile( aFullFileName ) )
    {
        delete plotter;
        return;
    }

    LOCALE_IO   toggle;

    plotter->StartPlot();

    if( m_component )
    {
        TRANSFORM temp;     // Uses default transform
        wxPoint plotPos;
        plotPos.x = pageInfo.GetWidthIU() /2;
        plotPos.y = pageInfo.GetHeightIU()/2;

        m_component->Plot( plotter, GetUnit(), GetConvert(), plotPos, temp );

        // Plot lib fields, not plotted by m_component->Plot():
        m_component->PlotLibFields( plotter, GetUnit(), GetConvert(),
                                    plotPos, temp );
    }

    plotter->EndPlot();
    delete plotter;
}

void LIB_EDIT_FRAME::PrintPage( wxDC* aDC, LSET aPrintMask, bool aPrintMirrorMode, void* aData)
{
    if( ! m_component )
        return;

    wxSize pagesize = GetScreen()->GetPageSettings().GetSizeIU();

    /* Plot item centered to the page
     * In libedit, the component is centered at 0,0 coordinates.
     * So we must plot it with an offset = pagesize/2.
     */
    wxPoint plot_offset;
    plot_offset.x = pagesize.x/2;
    plot_offset.y = pagesize.y/2;

    m_component->Draw( m_canvas, aDC, plot_offset, m_unit, m_convert, GR_DEFAULT_DRAWMODE );
}


