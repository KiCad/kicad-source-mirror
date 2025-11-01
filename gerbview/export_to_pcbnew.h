/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <geometry/shape_poly_set.h>
#include <layer_ids.h>


struct EXPORT_SLOT
{
    EXPORT_SLOT( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth ) :
            m_Start( aStart ),
            m_End( aEnd ),
            m_Width( aWidth )
    { }

    VECTOR2I m_Start;
    VECTOR2I m_End;
    int      m_Width;
};


struct EXPORT_VIA
{
    EXPORT_VIA( const VECTOR2I& aPos, int aSize, int aDrill ) :
            m_Pos( aPos ),
            m_Size( aSize ),
            m_Drill( aDrill )
    { }

    VECTOR2I m_Pos;
    int      m_Size;
    int      m_Drill;
};


class GERBER_DRAW_ITEM;
class GERBVIEW_FRAME;


/**
 * A helper class to export a Gerber set of files to Pcbnew.
 */
class GBR_TO_PCB_EXPORTER
{
public:
    GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName );
    ~GBR_TO_PCB_EXPORTER();

    /**
     * Save a board from a set of Gerber images.
     */
    bool ExportPcb( const int* aLayerLookUpTable, int aCopperLayers );

private:
    /**
     * Collect holes from a drill layer.
     *
     * We'll use these later when writing pads & vias.  Many holes will be pads, but we have
     * no way to create those without footprints, and creating a footprint per pad is not
     * really viable.  We use vias to mimic holes, with the loss of any hole shape (as we only
     * have round holes in vias at present).  We start out with a via size minimally larger
     * than the hole.  We'll leave it this way if the pad gets drawn as a copper polygon, or
     * increase it to the proper size if it has a circular, concentric copper flashing.
     */
    void collect_hole( const GERBER_DRAW_ITEM* aGbrItem );

    /**
     * Write a via to the board file.
     *
     * Some of these will represent actual vias while others are used to represent
     * holes in pads.  (We can't generate actual pads because the Gerbers don't contain
     * info on how to group them into footprints.)
     */
    void export_via( const EXPORT_VIA& aVia );

    void export_slot( const EXPORT_SLOT& aSlot );

    /**
     * Write a non copper line or arc to the board file.
     *
     * @param aGbrItem is the Gerber item (line, arc) to export.
     * @param aLayer is the technical layer to use.
     */
    void export_non_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer );

    /**
     * Write a non copper arc to the board file.
     *
     * @param aGbrItem is the Gerber item (line, arc) to export.
     * @param aLayer is the technical layer to use.
     */
    void export_non_copper_arc( const GERBER_DRAW_ITEM* aGbrItem, int aLayer );

    /**
     * Write the stroke info (thickness, line type) to the board file.
     *
     * @param aWidth is the line thickness gerber units).
     */
    void export_stroke_info( double aWidth );

    /**
     * Write a non-copper polygon to the board file.
     *
     * @param aLayer is the technical layer to use.
     */
    void writePcbPolygon( const SHAPE_POLY_SET& aPolys, int aLayer,
                          const VECTOR2I& aOffset = { 0, 0 } );

    /**
     * Write a filled circle to the board file (with line thickness = 0).
     *
     * @param aCenterPosition is the actual position of the filled circle,
     *  given by <round_flashed_shape>->GetABPosition()
     * @param aRadius is the circle radius.
     * @param aLayer is the layer to use.
     */
    void writePcbFilledCircle( const VECTOR2I& aCenterPosition, int aRadius, int aLayer );

    /**
     * Write a zone item to the board file.
     *
     * @warning This is experimental for tests only.
     *
     * @param aGbrItem is the Gerber item (line, arc) to export.
     * @param aLayer is the technical layer to use.
     */
    void writePcbZoneItem( const GERBER_DRAW_ITEM* aGbrItem, int aLayer );

    /**
     * Write a track (or via) to the board file.
     *
     * @param aGbrItem is the Gerber item (line, arc, flashed) to export.
     * @param aLayer is the copper layer to use.
     */
    void export_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer );

    /**
     * Write a synthetic pad to the board file.
     *
     * We can't create real pads because the Gerbers don't store grouping/footprint info.
     * So we synthesize a pad with a via for the hole (if present) and a copper polygon for
     * the pad.
     *
     * @param aGbrItem is the flashed Gerber item to export.
     */
    void export_flashed_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer );

    /**
     * Write a track (not via) to the board file.
     *
     * @param aGbrItem is the Gerber item (line only) to export.
     * @param aLayer is the copper layer to use.
     */
    void export_segline_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer );

    /**
     * Write a set of tracks (arcs are approximated by track segments) to the board file.
     *
     * @param aGbrItem is the Gerber item (arc only) to export.
     * @param aLayer is the copper layer to use
     */
    void export_segarc_copper_item( const GERBER_DRAW_ITEM* aGbrItem, int aLayer );

    /**
     * Basic write function to write a a #PCB_TRACK to the board file from a non flashed item.
     */
    void writeCopperLineItem( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
                              int aLayer );

    /**
     * Write a very basic header to the board file.
     */
    void writePcbHeader( const int* aLayerLookUpTable );

    /**
     * Map GerbView internal units to millimeters for Pcbnew board files.
     *
     * @param aValue is a coordinate value to convert in mm.
     */
    double MapToPcbUnits( int aValue ) const
    {
        return aValue / gerbIUScale.IU_PER_MM;
    }

private:
    GERBVIEW_FRAME*          m_gerbview_frame;   // the main gerber frame
    wxString                 m_pcb_file_name;    // BOARD file to write to
    FILE*                    m_fp;               // the board file
    int                      m_pcbCopperLayersCount;
    std::vector<EXPORT_VIA>  m_vias;
    std::vector<EXPORT_SLOT> m_slots;
};
