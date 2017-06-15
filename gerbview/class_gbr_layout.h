/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_gbr_layout.h
 * @brief Class CLASS_GBR_LAYOUT to handle info to draw/print loaded Gerber images
 * and page frame reference
 */

#ifndef CLASS_GBR_LAYOUT_H
#define CLASS_GBR_LAYOUT_H


#include <dlist.h>

#include <class_colors_design_settings.h>
#include <common.h>                         // PAGE_INFO
#include <gerbview.h>                       // GERBER_DRAWLAYERS_COUNT
#include <class_title_block.h>
#include <class_gerber_draw_item.h>
#include <class_gbr_display_options.h>

#include <gr_basic.h>
#include <class_eda_rect.h>

class GERBER_FILE_IMAGE_LIST;

/**
 * Class GBR_LAYOUT
 * holds list of GERBER_DRAW_ITEM currently loaded.
 */
class GBR_LAYOUT
{
private:
    EDA_RECT            m_BoundingBox;
    TITLE_BLOCK         m_titles;
    wxPoint             m_originAxisPosition;
    std::vector<int>    m_printLayersList;  // When printing: the list of graphic layers Id to print

public:

    GBR_LAYOUT();
    ~GBR_LAYOUT();

    // Accessor to the GERBER_FILE_IMAGE_LIST,
    // which handles the list of gerber files (and drill files) images loaded
    GERBER_FILE_IMAGE_LIST* GetImagesList();

    const wxPoint&      GetAuxOrigin() const
    {
        return m_originAxisPosition;
    }

    void SetAuxOrigin( const wxPoint& aPosition )
    {
        m_originAxisPosition = aPosition;
    }

    TITLE_BLOCK& GetTitleBlock()
    {
        return m_titles;
    }

    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
    {
        m_titles = aTitleBlock;
    }

    /**
     * Function ComputeBoundingBox
     * calculates the bounding box containing all Gerber items.
     * @return EDA_RECT - the full item list bounding box
     */
    EDA_RECT ComputeBoundingBox();

    /**
     * Function GetBoundingBox
     * may be called soon after ComputeBoundingBox() to return the same EDA_RECT,
     * as long as the CLASS_GBR_LAYOUT has not changed.
     */
    EDA_RECT GetBoundingBox() const { return m_BoundingBox; }

    void SetBoundingBox( const EDA_RECT& aBox ) { m_BoundingBox = aBox; }

    /**
     * Function Draw.
     * Redraw the CLASS_GBR_LAYOUT items but not cursors, axis or grid.
     * @param aPanel = the draw canvas
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ... (not always used)
     * @param aOffset = an draw offset value
     * @param aDisplayOptions = a GBR_DISPLAY_OPTIONS for draw/print display opts
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset,
               GBR_DISPLAY_OPTIONS* aDisplayOptions );

    /**
     * Function DrawItemsDCodeID
     * Draw the DCode value (if exists) corresponding to gerber item
     * (polygons do not have a DCode)
     * @param aPanel = the draw canvas
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ...
     * @param aDrawColor = the color of dcode texts
     */
    void DrawItemsDCodeID( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                           GR_DRAWMODE aDrawMode, COLOR4D aDrawColor );


    /**
     * Function SetPrintableLayers
     * Set the list of printable graphic layers
     * @param aLayerList = the new list (std::vector) of printable layer id
     */
    void SetPrintableLayers( const std::vector<int>& aLayerList  )
    {
        m_printLayersList = aLayerList;
    }

    /**
     * Function GetPrintableLayers
     * @return the list of printable layers
     */
    std::vector<int> GetPrintableLayers()
    {
        return m_printLayersList;
    }

    /**
     * Function ClearPrintableLayers
     * Clear the list of graphic layers to print
     */
    void ClearPrintableLayers()
    {
        m_printLayersList.clear();
    }

    /**
     * Function AddLayerToPrintableList
     * Add a layer to the list of graphic layers to print
     * @param aLayer = the id of the graphic layer.
     */
    void AddLayerToPrintableList( int aLayer)
    {
        m_printLayersList.push_back( aLayer );
    }


     /**
     * Function IsLayerPrintable
     * tests whether a given layer is visible
     * @param aLayer = The layer to be tested
     * @return bool - true if the layer is in print list.
     */
    bool    IsLayerPrintable( int aLayer ) const;

#if defined(DEBUG)
    void    Show( int nestLevel, std::ostream& os ) const;

#endif
};

#endif      // #ifndef CLASS_GBR_LAYOUT_H
