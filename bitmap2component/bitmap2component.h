/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef BITMAP2COMPONENT_H
#define BITMAP2COMPONENT_H

#include <geometry/shape_poly_set.h>
#include <potracelib.h>

class REPORTER;

enum OUTPUT_FMT_ID
{
    SYMBOL_FMT,
    SYMBOL_PASTE_FMT, // This does not include the header information
    FOOTPRINT_FMT,
    POSTSCRIPT_FMT,
    DRAWING_SHEET_FMT,
};

/* Helper class to handle useful info to convert a bitmap image to
 *  a polygonal object description
 */
class BITMAPCONV_INFO
{
private:
    enum OUTPUT_FMT_ID m_Format;        // File format
    int                m_PixmapWidth;
    int                m_PixmapHeight;  // the bitmap size in pixels
    double             m_ScaleX;
    double             m_ScaleY;        // the conversion scale
    potrace_path_t*    m_Paths;         // the list of paths, from potrace (list of lines and bezier curves)
    std::string        m_CmpName;       // The string used as cmp/footprint name
    std::string&       m_Data;          // the buffer containing the conversion
    REPORTER&          m_reporter;

public:
    BITMAPCONV_INFO( std::string& aData, REPORTER& aReporter );

    /**
     * Run the conversion of the bitmap
     */
    int ConvertBitmap( potrace_bitmap_t* aPotrace_bitmap, OUTPUT_FMT_ID aFormat, int aDpi_X,
                       int aDpi_Y, const wxString& aLayer );

private:
    /**
     * Creates the data specified by m_Format
     */
    void createOutputData( const wxString& aBrdLayerName = wxT( "F.SilkS" ) );

    /**
     * Function outputDataHeader
     * write to file the header depending on file format
     */
    void outputDataHeader( const wxString& aBrdLayerName );

    /**
     * Function outputDataEnd
     * write to file the last strings depending on file format
     */
    void outputDataEnd();

    /**
     * Function outputOnePolygon
     * write one polygon to output file.
     * Polygon coordinates are expected scaled by the polygon extraction function
     */
    void outputOnePolygon( SHAPE_LINE_CHAIN & aPolygon, const wxString& aBrdLayerName );
};

#endif  // BITMAP2COMPONENT_H
