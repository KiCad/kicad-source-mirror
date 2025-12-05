/**
 * @file gendrill_gerber_writer.h
 * @brief Classes used in drill files, map files and report files generation.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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

#ifndef _GENDRILL_GERBER_WRITER_
#define _GENDRILL_GERBER_WRITER_

#include "gendrill_writer_base.h"

#include <wx/filename.h>

class BOARD;

/**
 * Used to create Gerber drill files.
 */
class GERBER_WRITER: public GENDRILL_WRITER_BASE
{
public:
    GERBER_WRITER( BOARD* aPcb );

    virtual ~GERBER_WRITER()
    {
    }

    /**
     * Initialize internal parameters to match the given format.
     *
     * @param aRightDigits is the number of digits for mantissa part of coordinates (5 or 6).
     */
    void SetFormat( int aRightDigits = 6 );

    /**
     * Initialize internal parameters to match drill options.
     *
     * @note PTH and NPTH are always separate files in Gerber format.
     *
     * @param aOffset is the drill coordinates offset.
     */
    void SetOptions( const VECTOR2I& aOffset )
    {
        m_offset = aOffset;
        m_merge_PTH_NPTH = false;
    }

    /**
     * Create the full set of Excellon drill file for the board filenames are computed from
     * the board name, and layers id.
     *
     * @param aPlotDirectory is the output folder.
     * @param aGenDrill set to true to generate the EXCELLON drill file.
     * @param aGenMap set to true to generate a drill map file.
     * @param aGenTenting set to true to generate tenting layer files.
     * @param aReporter is a #REPORTER to return activity or any message (can be NULL).
     *
     * @return True if successful, false if any error occurred
     */
    bool CreateDrillandMapFilesSet( const wxString& aPlotDirectory, bool aGenDrill, bool aGenMap,
                                    bool aGenTenting, REPORTER* aReporter = nullptr );

private:
    /**
     * Create an Excellon drill file.
     *
     * @param aFullFilename is the full file name.
     * @param aIsNpth set to true for a NPTH file or false for a PTH file.
     * @param aLayerPair is the first board layer and the last board layer for this drill file
     *                   for blind buried vias, they are not always top and bottom layers/
     * @return hole count or -1 if the file cannot be created.
     */
    int createDrillFile( wxString& aFullFilename, bool aIsNpth, const DRILL_SPAN& aSpan );

    /**
     * Create a Gerber X2 file for via protection features.
     *
     * @param aFullFilename is the full file name
     * @param aFeature the protection feature this file represents
     * @param aLayerPair is the first and last layer pair for the via drill. Usually is top and bottom.
     */
    int createProtectionFile( const wxString& aFullFilename, IPC4761_FEATURES aFeature,
                              DRILL_LAYER_PAIR aLayerPair );

    /**
     * @param aPair is the layer pair.
     * @param aNPTH set to true to generate the filename of NPTH holes.
     * @param aMerge_PTH_NPTH set to true to generate the filename of a file which contains both
     *                        NPH and NPTH holes
     * @return a filename which identify the drill file function.  It is the board name with the
     *         layer pair names added, and for separate (PTH and NPTH) files, "-NPH" or "-NPTH"
     *         added.
     */
    virtual const wxString getDrillFileName( const DRILL_SPAN& aSpan, bool aNPTH,
                                             bool aMerge_PTH_NPTH ) const override;

    /**
     * test for an existing via having the given feature IPC4761_FEATURES
     * @param aFeature is the feature to find
     * @return true if at least one via having this feature is found in m_holeListBuffer
     */
    bool hasViaType( IPC4761_FEATURES aFeature );

    wxFileName getBackdrillLayerPairFileName( const DRILL_SPAN& aSpan ) const;
    bool       writeBackdrillLayerPairFile( const wxString& aPlotDirectory, REPORTER* aReporter,
                                            const DRILL_SPAN& aSpan );
};

#endif  //  #ifndef _GENDRILL_GERBER_WRITER_
