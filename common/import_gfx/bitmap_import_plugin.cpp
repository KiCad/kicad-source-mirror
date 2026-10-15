/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "bitmap_import_plugin.h"

#include <eda_item.h>
#include "graphics_importer.h"

#include <bitmap_tracing.h>
#include <math/util.h>
#include <reporter.h>
#include <wx/log.h>
#include <wx/mstream.h>


static const double MM_PER_INCH = 25.4;

/// Sub units per pixel while tracing. A whole pixel is too coarse a step for the integer tracer.
static const double TRACE_SUBUNITS = 1000.0;


void BITMAP_IMPORT_PLUGIN::ReportMsg( const wxString& aMessage )
{
    m_messages += aMessage;
    m_messages += '\n';
}


bool BITMAP_IMPORT_PLUGIN::Load( const wxString& aFileName )
{
    wxLogNull doNotLog;

    m_image.Destroy();

    return m_image.LoadFile( aFileName );
}


bool BITMAP_IMPORT_PLUGIN::LoadFromMemory( const wxMemoryBuffer& aMemBuffer )
{
    wxLogNull doNotLog;

    wxMemoryInputStream stream( aMemBuffer.GetData(), aMemBuffer.GetDataLen() );

    m_image.Destroy();

    return m_image.LoadFile( stream );
}


static int imageResolution( const wxImage& aImage, const wxString& aOption, int aDefault )
{
    // Read as a string because GetOptionInt truncates a fractional pixels per cm value.
    double   dpi = 0.0;
    wxString resolution = aImage.GetOption( aOption );

    resolution.ToCDouble( &dpi );

    if( dpi <= 1.0 )
        return aDefault;

    if( aImage.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONUNIT ) == wxIMAGE_RESOLUTION_CM )
        return KiROUND( dpi * 2.54 );

    return KiROUND( dpi );
}


double BITMAP_IMPORT_PLUGIN::pixelSizeXmm() const
{
    return MM_PER_INCH / imageResolution( m_image, wxIMAGE_OPTION_RESOLUTIONX, DEFAULT_DPI );
}


double BITMAP_IMPORT_PLUGIN::pixelSizeYmm() const
{
    return MM_PER_INCH / imageResolution( m_image, wxIMAGE_OPTION_RESOLUTIONY, DEFAULT_DPI );
}


double BITMAP_IMPORT_PLUGIN::GetImageWidth() const
{
    return m_image.GetWidth() * pixelSizeXmm();
}


double BITMAP_IMPORT_PLUGIN::GetImageHeight() const
{
    return m_image.GetHeight() * pixelSizeYmm();
}


BOX2D BITMAP_IMPORT_PLUGIN::GetImageBBox() const
{
    return BOX2D( VECTOR2D( 0, 0 ), VECTOR2D( GetImageWidth(), GetImageHeight() ) );
}


bool BITMAP_IMPORT_PLUGIN::Import()
{
    wxCHECK( m_importer, false );

    // A traced area is filled, so it has no outline. The importer spells that -1.
    static const IMPORTED_STROKE noStroke( -1, LINE_STYLE::SOLID, COLOR4D::UNSPECIFIED );

    if( !m_image.IsOk() )
        return false;

    wxImage bilevel = ConvertImageToBilevel( m_image, m_threshold, m_invert );

    WX_STRING_REPORTER reporter;
    SHAPE_POLY_SET     polygons = TraceBilevelImage( bilevel, TRACE_SUBUNITS, TRACE_SUBUNITS, reporter );

    if( reporter.HasMessage() )
        ReportMsg( reporter.GetMessages() );

    for( int ii = 0; ii < polygons.OutlineCount(); ii++ )
    {
        const SHAPE_LINE_CHAIN& outline = polygons.COutline( ii );
        std::vector<VECTOR2D>   vertices;

        vertices.reserve( outline.PointCount() );

        for( int jj = 0; jj < outline.PointCount(); jj++ )
        {
            vertices.emplace_back( outline.CPoint( jj ).x / TRACE_SUBUNITS * pixelSizeXmm(),
                                   outline.CPoint( jj ).y / TRACE_SUBUNITS * pixelSizeYmm() );
        }

        m_importer->AddPolygon( vertices, noStroke, true, COLOR4D::UNSPECIFIED );
    }

    return true;
}
