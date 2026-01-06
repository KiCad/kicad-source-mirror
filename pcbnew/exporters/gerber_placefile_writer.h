/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <layer_ids.h>
#include <math/vector2d.h>

class BOARD;
class FOOTPRINT;
class PAD;

/**
 * Used to create Gerber drill files.
 */
class PLACEFILE_GERBER_WRITER
{
public:
    PLACEFILE_GERBER_WRITER( BOARD* aPcb );

    virtual ~PLACEFILE_GERBER_WRITER()
    {
    }


    /**
     * Initialize internal parameters to match drill options
     *
     * @note PTH and NPTH are always separate files in Gerber format.
     *
     * @param aOffset is the drill coordinates offset.
     */
    void SetOptions( const VECTOR2I& aOffset )
    {
        m_offset = aOffset;
    }

    /**
     * Set the variant name for variant-aware filtering.
     * When set, DNP, BOM, and position file exclusion will check variant-specific status.
     */
    void SetVariant( const wxString& aVariant ) { m_variant = aVariant; }

    /**
     * Create an pnp gerber file.
     *
     * @param aFullFilename is the full filename.
     * @param aLayer is the layer (F_Cu or B_Cu) to generate.
     * @param aIncludeBrdEdges use true to include board outlines.
     * @return component count, or -1 if the file cannot be created.
     */
    int CreatePlaceFile( const wxString& aFullFilename, PCB_LAYER_ID aLayer, bool aIncludeBrdEdges,
                         bool aExcludeDNP, bool aExcludeBOM );

    /**
     * @param aFullBaseFilename = a full filename. it will be modified
     * to add "-pnp" and set the extension
     * @param aLayer = layer (F_Cu or B_Cu) to generate
     * @return a filename which identify the drill file function.
     */
    const wxString GetPlaceFileName( const wxString& aFullBaseFilename,
                                     PCB_LAYER_ID aLayer ) const;

private:
    /**
     * Convert a KiCad footprint orientation to gerber rotation both are in degrees.
     * @param aAngle is the Pcbnew angle in degrees
     * @param aIsFlipped = false for footprints on top side, true on bottom side (flipped)
     * @return the angle in degrees in Gerber convention:
     *  same as Pcbnew for top side
     *  180 + angle for bottom side
     */
    double mapRotationAngle( double aAngle, bool aIsFlipped );

    /**
     * Find the pad(s) 1 (or pad "A1") of a footprint.
     *
     * Useful to plot a marker at this (these) position(s).
     *
     * @param aPadList is the list to fill.
     * @param aFootprint is the footprint to test,
     */
    void findPads1( std::vector<PAD*>& aPadList, FOOTPRINT* aFootprint ) const;

    BOARD*       m_pcb;
    PCB_LAYER_ID m_layer;            // The board layer currently used (typically F_Cu or B_Cu)
    VECTOR2I     m_offset;           // Drill offset coordinates

    bool         m_plotPad1Marker;       // True to plot a flashed marker shape at pad 1 position
    bool         m_plotOtherPadsMarker;  // True to plot a marker shape at other pads position
                                         // This is a flashed 0 sized round pad

    wxString     m_variant;          // Variant name for variant-aware filtering
};

#endif  //  #ifndef PLACEFILE_GERBER_WRITER_H
