/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHOR.txt for contributors.
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
 * @file gendrill_excellon_writer.h
 * @brief Classes used in drill files, map files and report files generation.
 */


#ifndef _GENDRILL_EXCELLON_WRITER_
#define _GENDRILL_EXCELLON_WRITER_

#include "gendrill_writer_base.h"

#include <wx/filename.h>

class BOARD;
class PLOTTER;
class OUTPUTFORMATTER;


/**
 * Create Excellon drill, drill map, and drill report files.
 */
class EXCELLON_WRITER: public GENDRILL_WRITER_BASE
{
public:
    EXCELLON_WRITER( BOARD* aPcb );

    virtual ~EXCELLON_WRITER()
    {
    }

    /**
     * Return the plot offset (usually the position of the auxiliary axis.
     */
    VECTOR2I GetOffset() { return m_offset; }

    /**
     *
     */
    void SetRouteModeForOvalHoles( bool aUseRouteModeForOvalHoles )
    {
        m_useRouteModeForOval = aUseRouteModeForOvalHoles;
    }

    /**
     * Initialize internal parameters to match the given format.
     *
     * @param aMetric set to true for metric coordinates, false for imperial units.
     * @param aZerosFmt is the zero format DECIMAL_FORMAT, SUPPRESS_LEADING, SUPPRESS_TRAILING,
     *                  or KEEP_ZEROS.
     * @param aLeftDigits is the number of digits for integer part of coordinates
     *                    if <= 0 (default), a suitable value will be used, depending on units.
     * @param aRightDigits is number of digits for mantissa part of coordinates
     *                     if <= 0 (default), a suitable value will be used, depending on units.
     */
    void SetFormat( bool aMetric, ZEROS_FMT aZerosFmt = DECIMAL_FORMAT,
                    int aLeftDigits = 0, int aRightDigits = 0 );

    /**
     * Initialize internal parameters to match drill options.
     *
     * @param aMirror set to true to create mirrored coordinates (Y coordinates negated).
     * @param aMinimalHeader set to true to use a minimal header (no comments, no info).
     * @param aOffset is the drill coordinates offset.
     * @param aMerge_PTH_NPTH set to true to create only one file containing PTH and NPTH
     *                        false to create 2 separate files : one for PTH and one for NPTH.
     */
    void SetOptions( bool aMirror, bool aMinimalHeader, const VECTOR2I& aOffset,
                     bool aMerge_PTH_NPTH )
    {
        m_mirror = aMirror;
        m_offset = aOffset;
        m_minimalHeader = aMinimalHeader;
        m_merge_PTH_NPTH = aMerge_PTH_NPTH;
    }

    /**
     * Create the full set of Excellon drill file for the board.
     *
     * File names are computed from the board name and layer ID.
     *
     * @param aPlotDirectory is the output folder.
     * @param aGenDrill set to true to generate the EXCELLON drill file.
     * @param aGenMap set to true to generate a drill map file.
     * @param aReporter is a #REPORTER to return activity or any message (can be NULL)
     */
    bool CreateDrillandMapFilesSet( const wxString& aPlotDirectory, bool aGenDrill, bool aGenMap,
                                    REPORTER* aReporter = nullptr );

private:
    /**
     * Create an Excellon drill file.
     *
     * @param aFile is an opened file to write to will be closed by CreateDrillFile.
     * @param aLayerPair is the layer pair for the current holes.
     * @param aHolesType is the holes type (PTH, NPTH, mixed).
     * @return the hole count.
     */
    int createDrillFile( FILE* aFile, const DRILL_SPAN& aSpan, TYPE_FILE aHolesType,
                         bool aTagBackdrillHit = false );


    /**
     * Print the DRILL file header.
     *
     * The full header is something like:
     * M48
     * ;DRILL file {PCBNEW (2007-11-29-b)} date 17/1/2008-21:02:35
     * ;FORMAT={ <precision> / absolute / <units> / <numbers format>}
     * ; #@! TF.FileFunction,Plated,1,4,PTH
     * ; #@! TF.CreationDate,2018-11-23T15:59:51+01:00
     * ; #@! TF.GenerationSoftware,Kicad,Pcbnew,2017.04
     * FMAT,2
     * INCH,TZ
     *
     * @param aSpan is the drilling span for the current holes.
     * @param aHolesType is the holes type in file (PTH, NPTH, mixed).
     */
    void writeEXCELLONHeader( const DRILL_SPAN& aSpan, TYPE_FILE aHolesType );

    void writeEXCELLONEndOfFile();

    /**
     * Create a line like according to the selected format.
     */
    void writeCoordinates( char* aLine, size_t aLineSize, double aCoordX, double aCoordY );

    /**
     * Write a comment string giving the hole attribute.
     *
     * @param aAttribute is the hole attribute.
     */
    void writeHoleAttribute( HOLE_ATTRIBUTE aAttribute );

    wxFileName getBackdrillLayerPairFileName( const DRILL_SPAN& aSpan ) const;
    bool       writeBackdrillLayerPairFile( const wxString& aPlotDirectory,
                                            REPORTER* aReporter, const DRILL_SPAN& aSpan );
    void       writeHoleComments( const HOLE_INFO& aHole, bool aTagBackdrillHit );
    void       writePostMachiningComment( PAD_DRILL_POST_MACHINING_MODE aMode, int aSizeIU,
                                          int aDepthIU, int aAngleDeciDegree,
                                          const wxString& aSideLabel );
    wxString   formatLinearValue( int aValueIU ) const;

    FILE*     m_file;                    // The output file
    bool      m_minimalHeader;           // True to use minimal header
    bool      m_mirror;
    bool      m_useRouteModeForOval;     // True to use a route command for oval holes
                                         // False to use a G85 canned mode for oval holes
    int       m_mantissaLenght;          // Max number of digits printed in float numbers
};

#endif  //  #ifndef _GENDRILL_EXCELLON_WRITER_
