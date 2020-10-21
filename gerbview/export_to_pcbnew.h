/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see change_log.txt for contributors.
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

#include <convert_to_biu.h>
#include <geometry/shape_poly_set.h>
#include <layers_id_colors_and_visibility.h>


struct EXPORT_VIA
{
    EXPORT_VIA( const wxPoint& aPos, int aSize, int aDrill ) :
            m_Pos( aPos ),
            m_Size( aSize ),
            m_Drill( aDrill )
    { }

    wxPoint m_Pos;
    int     m_Size;
    int     m_Drill;
};


class GERBER_DRAW_ITEM;
class GERBVIEW_FRAME;


/**
 * A helper class to export a Gerber set of files to Pcbnew
 */
class GBR_TO_PCB_EXPORTER
{
private:
    GERBVIEW_FRAME*         m_gerbview_frame;   // the main gerber frame
    wxString                m_pcb_file_name;    // BOARD file to write to
    FILE*                   m_fp;               // the board file
    int                     m_pcbCopperLayersCount;
    std::vector<EXPORT_VIA> m_vias;
public:
    GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName );
    ~GBR_TO_PCB_EXPORTER();

    /**
     * Function ExportPcb
     * saves a board from a set of Gerber images.
     */
    bool    ExportPcb( LAYER_NUM* aLayerLookUpTable, int aCopperLayers );

private:
    /**
     * collect holes from a drill layer.
     * We'll use these later when writing pads & vias.
     * @param aGbrItem
     */
    void    collect_hole( GERBER_DRAW_ITEM* aGbrItem );

    /**
     * write a via to the board file.
     * Some of these will represent actual vias while others are used to represent
     * holes in pads.  (We can't generate actual pads because the Gerbers don't contain
     * info on how to group them into footprints.)
     * @param aVia
     */
    void    export_via( const EXPORT_VIA& aVia );

    /**
     * write a non copper line or arc to the board file.
     * @param aGbrItem = the Gerber item (line, arc) to export
     * @param aLayer = the technical layer to use
     */
    void    export_non_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * write a non-copper polygon to the board file.
     * @param aLayer = the technical layer to use
     */
    void    writePcbPolygon( const SHAPE_POLY_SET& aPolys, LAYER_NUM aLayer,
                             const wxPoint& aOffset = { 0, 0 } );

    /**
     * write a zone item to the board file.
     * Currently: only experimental, for tests
     * @param aGbrItem = the Gerber item (line, arc) to export
     * @param aLayer = the technical layer to use
     */
    void    writePcbZoneItem( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * write a track (or via) to the board file.
     * @param aGbrItem = the Gerber item (line, arc, flashed) to export
     * @param aLayer = the copper layer to use
     */
    void    export_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * Function export_flashed_copper_item
     * write a synthetic pad to the board file.
     * We can't create real pads because the Gerbers don't store grouping/footprint info.
     * So we synthesize a pad with a via for the hole (if present) and a copper polygon for
     * the pad.
     * @param aGbrItem = the flashed Gerber item to export
     */
    void    export_flashed_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * Function export_segline_copper_item
     * write a track (not via) to the board file.
     * @param aGbrItem = the Gerber item (line only) to export
     * @param aLayer = the copper layer to use
     */
    void    export_segline_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * Function export_segarc_copper_item
     * write a set of tracks (arcs are approximated by track segments)
     * to the board file.
     * @param aGbrItem = the Gerber item (arc only) to export
     * @param aLayer = the copper layer to use
     */
    void    export_segarc_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * function writeCopperLineItem
     * basic write function to write a a TRACK item
     * to the board file, from a non flashed item
     */
    void    writeCopperLineItem( wxPoint& aStart, wxPoint& aEnd,
                                 int aWidth, LAYER_NUM aLayer );

    /**
     * function writePcbHeader
     * Write a very basic header to the board file
     */
    void    writePcbHeader( LAYER_NUM* aLayerLookUpTable );

    /** In Pcbnew files units are mm for coordinates.
     * So MapToPcbUnits converts internal gerbview to mm any pcbnew value
     * @param aValue is a coordinate value to convert in mm
     */
    double MapToPcbUnits( int aValue )
    {
        return aValue / IU_PER_MM;
    }
};
