/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 Kicad Developers, see change_log.txt for contributors.
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

// for consistency this enum should conform to the
// indices in m_radioBoxFormat from bitmap2cmp_gui.cpp
enum OUTPUT_FMT_ID
{
    EESCHEMA_FMT = 0,
    PCBNEW_KICAD_MOD,
    POSTSCRIPT_FMT,
    KICAD_WKS_LOGO,
    FINAL_FMT = KICAD_WKS_LOGO
};

enum BMP2CMP_MOD_LAYER
{
    MOD_LYR_FSILKS = 0,
    MOD_LYR_FSOLDERMASK,
    MOD_LYR_ECO1,
    MOD_LYR_ECO2,
    MOD_LYR_FINAL = MOD_LYR_ECO2
};


/* Helper class to handle useful info to convert a bitmap image to
 *  a polygonal object description
 */
class BITMAPCONV_INFO
{
private:
    enum OUTPUT_FMT_ID m_Format;    // File format
    int m_PixmapWidth;
    int m_PixmapHeight;             // the bitmap size in pixels
    double             m_ScaleX;
    double             m_ScaleY;    // the conversion scale
    potrace_path_t*    m_Paths;     // the list of paths, from potrace (list of lines and bezier curves)
    std::string m_CmpName;          // The string used as cmp/footprint name
    std::string&  m_Data;           // the buffer containing the conversion
    std::string   m_errors;         // a buffer to return error messages

public:
    BITMAPCONV_INFO( std::string& aData );

    /**
     * Run the conversion of the bitmap
     */
    int ConvertBitmap( potrace_bitmap_t* aPotrace_bitmap,
                      OUTPUT_FMT_ID aFormat, int aDpi_X, int aDpi_Y,
                      BMP2CMP_MOD_LAYER aModLayer );

    std::string& GetErrorMessages() {return m_errors; }

private:
    /**
     * Creates the data specified by m_Format
     */
    void createOutputData( BMP2CMP_MOD_LAYER aModLayer = (BMP2CMP_MOD_LAYER) 0 );

    /**
     * Function outputDataHeader
     * write to file the header depending on file format
     */
    void outputDataHeader(  const char * aBrdLayerName );

    /**
     * Function outputDataEnd
     * write to file the last strings depending on file format
     */
    void outputDataEnd();


    /**
     * @return the board layer name depending on the board layer selected
     * @param aChoice = the choice (MOD_LYR_FSILKS to MOD_LYR_FINAL)
     */
    const char * getBoardLayerName( BMP2CMP_MOD_LAYER aChoice );

    /**
     * Function outputOnePolygon
     * write one polygon to output file.
     * Polygon coordinates are expected scaled by the polygon extraction function
     */
    void outputOnePolygon( SHAPE_LINE_CHAIN & aPolygon, const char* aBrdLayerName );

};

#endif  // BITMAP2COMPONENT_H
