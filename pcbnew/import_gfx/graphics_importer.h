/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef GRAPHICS_IMPORTER_H
#define GRAPHICS_IMPORTER_H

#include "graphics_import_mgr.h"
#include "graphics_import_plugin.h"

#include <eda_text.h>
#include <math/vector2d.h>

#include <list>
#include <memory>
#include <vector>

class EDA_ITEM;

/**
 * Interface that creates objects representing shapes for a given data model.
 */
class GRAPHICS_IMPORTER
{
public:
    GRAPHICS_IMPORTER();

    virtual ~GRAPHICS_IMPORTER()
    {
    }

    /**
     * Set the import plugin used to obtain shapes from a file.
     */
    void SetPlugin( std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> aPlugin )
    {
        m_plugin = std::move( aPlugin );

        if( m_plugin )
            m_plugin->SetImporter( this );
    }

    /**
     * Load file and get its basic data
     *
     */
    bool  Load( const wxString &aFileName );


    /**
     * Import shapes from loaded file.
     *
     * It is important to have the file loaded before importing.
     *
     * @param aScale allow import graphic items with a non 1:1 import ratio
     * aScale = 1.0 to import graphics with their actual size.
     */
    bool Import( double aScale = 1.0 );

    /**
     * Collect warning and error messages after loading/importing.
     *
     * @return the list of messages in one string. Each message ends by '\n'
     */
    wxString GetMessages() const
    {
        return m_plugin->GetMessages();
    }

    /**
     * Get original image Width.
     *
     * @return Width of the loaded image in mm.
     */
    double GetImageWidthMM() const
    {
        return m_originalWidth;
    }

    /**
     * Get original image Height
     *
     * @return Height of the loaded image in mm.
     */
    double GetImageHeightMM() const
    {
        return m_originalHeight;
    }

    /**
     * Sets the line width for the imported outlines (in mm).
     */
    void SetLineWidthMM( double aWidth )
    {
        m_lineWidth = aWidth;
    }

    /**
     * Returns the line width used for importing the outlines (in mm).
     */
    double GetLineWidthMM() const
    {
        return m_lineWidth;
    }

    /**
     * @return the scale factor affecting the imported shapes.
     */
    double GetScale() const
    {
        return m_scale;
    }

    /**
     * @return the offset to add to coordinates when importing graphic items.
     * The offset is always in mm
     */
    const VECTOR2D& GetImportOffsetMM() const
    {
        return m_offsetCoordmm;
    }

    /**
     * Set the offset to add to coordinates when importing graphic items.
     * The offset is always in mm
     */
    void SetImportOffsetMM( const VECTOR2D& aOffset )
    {
        m_offsetCoordmm = aOffset;
    }

    /**
     * Set the scale factor affecting the imported shapes.
     * it allows conversion between imported shapes units and mm
     */
    void SetScale( double aScale )
    {
        m_scale = aScale;
    }

    /**
     * @return the conversion factor from mm to internal unit
     */
    double GetMillimeterToIuFactor()
    {
        return m_millimeterToIu;
    }

    /**
     * @return the overall scale factor to convert the imported shapes dimension to mm.
     */
    double ImportScalingFactor() const
    {
        return m_scale * m_millimeterToIu;
    }

    /**
     * Return the list of objects representing the imported shapes.
     */
    std::list<std::unique_ptr<EDA_ITEM>>& GetItems()
    {
        return m_items;
    }

    ///> Default line thickness (in mm)
    static constexpr unsigned int DEFAULT_LINE_WIDTH_DFX = 1;

    // Methods to be implemented by derived graphics importers

    /**
     * Create an object representing a line segment.
     *
     * @param aOrigin is the segment origin point expressed in mm.
     * @param aEnd is the segment end point expressed in mm.
     * @param aWidth is the segment thickness in mm. Use -1 for default line thickness
     */
    virtual void AddLine( const VECTOR2D& aOrigin, const VECTOR2D& aEnd, double aWidth ) = 0;

    /**
     * Create an object representing a circle.
     *
     * @param aCenter is the circle center point expressed in mm.
     * @param aRadius is the circle radius expressed in mm.
     * @param aWidth is the segment thickness in mm. Use -1 for default line thickness
     */
    virtual void AddCircle( const VECTOR2D& aCenter, double aRadius, double aWidth ) = 0;

    /**
     * Create an object representing an arc.
     *
     * @param aCenter is the arc center point expressed in mm.
     * @param aStart is the arc arm end point expressed in mm.
     * Its length is the arc radius.
     * @param aAngle is the arc angle expressed in degrees.
     * @param aWidth is the segment thickness in mm. Use -1 for default line thickness
     */
    virtual void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, double aAngle,
                         double aWidth ) = 0;

    virtual void AddPolygon( const std::vector< VECTOR2D >& aVertices, double aWidth ) = 0;

    /**
     * Create an object representing a text.
     *
     * @param aOrigin is the text position.
     * @param aText is the displayed text.
     * @param aHeight is the text height expressed in mm.
     * @param aWidth is the text width expressed in mm.
     * @param aOrientation is the text orientation angle expressed in degrees.
     * @param aHJustify is the text horizontal justification.
     * @param aVJustify is the text vertical justification.
     * @param aWidth is the segment thickness in mm. Use -1 for default line thickness
     */
    virtual void AddText( const VECTOR2D& aOrigin, const wxString& aText,
            double aHeight, double aWidth, double aThickness, double aOrientation,
            EDA_TEXT_HJUSTIFY_T aHJustify, EDA_TEXT_VJUSTIFY_T aVJustify ) = 0;

    /**
     * Create an object representing an arc.
     *
     * @param aStart is the curve start point expressed in mm.
     * @param aBezierControl1 is the first Bezier control point expressed in mm.
     * @param aBezierControl2 is the second Bezier control point expressed in mm.
     * @param aEnd is the curve end point expressed in mm.
     * @param aWidth is the segment thickness in mm. Use -1 for default line thickness
     */
    virtual void AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                            const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                            double aWidth ) = 0;

protected:
    ///> Adds an item to the imported shapes list.
    void addItem( std::unique_ptr<EDA_ITEM> aItem )
    {
        m_items.emplace_back( std::move( aItem ) );
    }

private:
    ///> List of imported items
    std::list<std::unique_ptr<EDA_ITEM>> m_items;

    ///> Plugin used to load a file
    std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> m_plugin;

    ///> Total image width
    double m_originalWidth;

    ///> Total image Height;
    double m_originalHeight;

    /**
     * Scale factor applied to the imported graphics.
     * 1.0 does not change the size of imported items
     * scale < 1.0 reduce the size of imported items
     */
    double m_scale;

    ///> Default line thickness for the imported graphics
    double m_lineWidth;

protected:
    ///> factor to convert millimeters to Internal Units
    double m_millimeterToIu;

    ///> Offset (in mm) for imported coordinates
    VECTOR2D m_offsetCoordmm;
};

#endif /* GRAPHICS_IMPORTER_H */
