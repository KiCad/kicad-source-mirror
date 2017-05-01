/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2017 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file gendrill_Excellon_writer.h
 * @brief Classes used in drill files, map files and report files generation.
 */


#ifndef _GENDRILL_EXCELLON_WRITER_
#define _GENDRILL_EXCELLON_WRITER_

#include <gendrill_file_writer_base.h>

class BOARD;
class PLOTTER;
class OUTPUTFORMATTER;

/**
 * EXCELLON_WRITER is a class mainly used to create Excellon drill files
 * However, this class is also used to create drill maps and drill report
 */
class EXCELLON_WRITER: public GENDRILL_WRITER_BASE
{
private:
    FILE*                    m_file;                    // The output file
    bool                     m_minimalHeader;           // True to use minimal header
    bool                     m_mirror;

public:
    EXCELLON_WRITER( BOARD* aPcb );

    virtual ~EXCELLON_WRITER()
    {
    }

    /**
     * Return the plot offset (usually the position
     * of the auxiliary axis
     */
    const wxPoint GetOffset() { return m_offset; }

    /**
     * Function SetFormat
     * Initialize internal parameters to match the given format
     * @param aMetric = true for metric coordinates, false for imperial units
     * @param aZerosFmt =  DECIMAL_FORMAT, SUPPRESS_LEADING, SUPPRESS_TRAILING, KEEP_ZEROS
     * @param aLeftDigits = number of digits for integer part of coordinates
     *          if <= 0 (default), a suitable value will be used, depending on units
     * @param aRightDigits = number of digits for mantissa part of coordinates
     *          if <= 0 (default), a suitable value will be used, depending on units
     */
    void SetFormat( bool aMetric, ZEROS_FMT aZerosFmt = DECIMAL_FORMAT,
                    int aLeftDigits = 0, int aRightDigits = 0 );



    /**
     * Function SetOptions
     * Initialize internal parameters to match drill options
     * @param aMirror = true to create mirrored coordinates (Y coordinates negated)
     * @param aMinimalHeader = true to use a minimal header (no comments, no info)
     * @param aOffset = drill coordinates offset
     */
    void SetOptions( bool aMirror, bool aMinimalHeader, wxPoint aOffset, bool aMerge_PTH_NPTH )
    {
        m_mirror = aMirror;
        m_offset = aOffset;
        m_minimalHeader = aMinimalHeader;
        m_merge_PTH_NPTH = aMerge_PTH_NPTH;
    }

    /**
     * Function CreateDrillandMapFilesSet
     * Creates the full set of Excellon drill file for the board
     * filenames are computed from the board name, and layers id
     * @param aPlotDirectory = the output folder
     * @param aGenDrill = true to generate the EXCELLON drill file
     * @param aGenMap = true to generate a drill map file
     * @param aReporter = a REPORTER to return activity or any message (can be NULL)
     */
    void CreateDrillandMapFilesSet( const wxString& aPlotDirectory,
                                    bool aGenDrill, bool aGenMap,
                                    REPORTER * aReporter = NULL );


private:
    /**
     * Function CreateDrillFile
     * Creates an Excellon drill file
     * @param aFile = an opened file to write to will be closed by CreateDrillFile
     * @return hole count
     */
    int  createDrillFile( FILE * aFile );


    /* Print the DRILL file header. The full header is:
     * M48
     * ;DRILL file {PCBNEW (2007-11-29-b)} date 17/1/2008-21:02:35
     * ;FORMAT={ <precision> / absolute / <units> / <numbers format>}
     * FMAT,2
     * INCH,TZ
     */
    void writeEXCELLONHeader();

    void writeEXCELLONEndOfFile();

    /* Created a line like:
     * X48000Y19500
     * According to the selected format
     */
    void writeCoordinates( char* aLine, double aCoordX, double aCoordY );
};

#endif  //  #ifndef _GENDRILL_EXCELLON_WRITER_
