/**
 * @file gendrill_gerber_writer.h
 * @brief Classes used in drill files, map files and report files generation.
 */

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

#ifndef _GENDRILL_GERBER_WRITER_
#define _GENDRILL_GERBER_WRITER_

#include <gendrill_file_writer_base.h>

class BOARD;

/**
 * GERBER_WRITER is a class mainly used to create Gerber drill files
 */
class GERBER_WRITER: public GENDRILL_WRITER_BASE
{
public:
    GERBER_WRITER( BOARD* aPcb );

    virtual ~GERBER_WRITER()
    {
    }

    /**
     * Function SetFormat
     * Initialize internal parameters to match the given format
     * @param aRightDigits = number of digits for mantissa part of coordinates (5 or 6)
     */
    void SetFormat( int aRightDigits = 6 );

    /**
     * Function SetOptions
     * Initialize internal parameters to match drill options
     * note: PTH and NPTH are always separate files in Gerber format
     * @param aOffset = drill coordinates offset
     */
    void SetOptions( wxPoint aOffset )
    {
        m_offset = aOffset;
        m_merge_PTH_NPTH = false;
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
     * Function createDrillFile
     * Creates an Excellon drill file
     * @param aFullFilename = the full filename
     * @param aIsNpth = true for a NPTH file, false for a PTH file
     * @param aLayer1 = the first board layer
     * @param aLayer2 = the last board layer
     * for blind buried vias, they are not always top and bottom layers
     * @return hole count, or -1 if the file cannot be created
     */
    int  createDrillFile( wxString& aFullFilename, bool aIsNpth, int aLayer1, int aLayer2 );

    /**
     * @return a filename which identify the drill file function.
     * it is the board name with the layer pair names added, and for separate
     * (PTH and NPTH) files, "-NPH" or "-NPTH" added
     * @param aPair = the layer pair
     * @param aNPTH = true to generate the filename of NPTH holes
     * @param aMerge_PTH_NPTH = true to generate the filename of a file which containd both
     * NPH and NPTH holes
     */
    virtual const wxString getDrillFileName( DRILL_LAYER_PAIR aPair, bool aNPTH,
                                             bool aMerge_PTH_NPTH ) const override;
};

#endif  //  #ifndef _GENDRILL_GERBER_WRITER_
