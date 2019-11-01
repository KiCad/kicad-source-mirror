/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file gerber_placefile_writer.h
 * @brief Classes used in place file generation.
 */

#ifndef PLACEFILE_GERBER_WRITER_H
#define PLACEFILE_GERBER_WRITER_H

#include <layers_id_colors_and_visibility.h>

class BOARD;
class MODULE;
class D_PAD;

/**
 * PLACEFILE_GERBER_WRITER is a class mainly used to create Gerber drill files
 */
class PLACEFILE_GERBER_WRITER
{
public:
    PLACEFILE_GERBER_WRITER( BOARD* aPcb );

    virtual ~PLACEFILE_GERBER_WRITER()
    {
    }


    /**
     * Function SetOptions
     * Initialize internal parameters to match drill options
     * note: PTH and NPTH are always separate files in Gerber format
     * @param aOffset = drill coordinates offset
     */
    void SetOptions( wxPoint aOffset )
    {
        m_offset = aOffset;
    }

    /**
     * Creates an pnp gerber file
     * @param aFullFilename = the full filename
     * @param aLayer = layer (F_Cu or B_Cu) to generate
     * @param aIncludeBrdEdges = true to include board outlines
     * @return component count, or -1 if the file cannot be created
     */
    int  CreatePlaceFile( wxString& aFullFilename, PCB_LAYER_ID aLayer, bool aIncludeBrdEdges );

    /**
     * @return a filename which identify the drill file function.
     * @param aFullBaseFilename = a full filename. it will be modified
     * to add "-pnp" and set the extension
     * @param aLayer = layer (F_Cu or B_Cu) to generate
     */
    const wxString GetPlaceFileName( const wxString& aFullBaseFilename,
                                     PCB_LAYER_ID aLayer ) const;

private:
    BOARD*          m_pcb;
    /// The board layer currently used (typically F_Cu or B_Cu)
    PCB_LAYER_ID    m_layer;
    double          m_conversionUnits;  // scaling factor to convert the board unites to
                                        // Excellon/Gerber units (i.e inches or mm)
    wxPoint         m_offset;           // Drill offset coordinates
    bool            m_forceSmdItems;
    // True to plot a flashed marker shape at pad 1 position
    bool            m_plotPad1Marker;
    // True to plot a marker shape at other pads position
    // This is a flashed 0 sized round pad
    bool            m_plotOtherPadsMarker;


    /** convert a kicad footprint orientation to gerber rotation
     *  both are in degrees
     */
    double      mapRotationAngle( double aAngle );

    /** Find the pad(s) 1 (or pad "A1") of a footprint
     * Usefull to plot a marker at this (these) position(s)
     * @param aPadList is the list to fill
     * @param aFootprint is the footprint to test
     */
    void findPads1( std::vector<D_PAD*>& aPadList, MODULE* aFootprint ) const;
};

#endif  //  #ifndef PLACEFILE_GERBER_WRITER_H
