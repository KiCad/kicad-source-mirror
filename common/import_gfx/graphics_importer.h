/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
#include <gal/color4d.h>
#include <stroke_params.h>

#include <list>
#include <memory>
#include <vector>

class EDA_ITEM;
class EDA_SHAPE;

/**
 * A clone of IMPORTED_STROKE, but with floating-point width.
 */
class IMPORTED_STROKE
{
public:
    IMPORTED_STROKE( double aWidth = 0, LINE_STYLE aPlotStyle = LINE_STYLE::DEFAULT,
                     const KIGFX::COLOR4D& aColor = KIGFX::COLOR4D::UNSPECIFIED ) :
            m_width( aWidth ),
            m_plotstyle( aPlotStyle ), m_color( aColor )
    {
    }

    double GetWidth() const { return m_width; }
    void   SetWidth( double aWidth ) { m_width = aWidth; }

    LINE_STYLE GetPlotStyle() const { return m_plotstyle; }
    void       SetPlotStyle( LINE_STYLE aPlotStyle ) { m_plotstyle = aPlotStyle; }

    KIGFX::COLOR4D GetColor() const { return m_color; }
    void           SetColor( const KIGFX::COLOR4D& aColor ) { m_color = aColor; }

private:
    double         m_width;
    LINE_STYLE     m_plotstyle;
    KIGFX::COLOR4D m_color;
};

/**
 * Interface that creates objects representing shapes for a given data model.
 */
class GRAPHICS_IMPORTER
{
public:
    enum POLY_FILL_RULE
    {
        PF_NONZERO = 0,
        PF_EVEN_ODD
    };

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
    bool Load( const wxString& aFileName );


    /**
     * Import shapes from loaded file.
     *
     * It is important to have the file loaded before importing.
     *
     * @param aScale allow import graphic items with a non 1:1 import ratio
     * VECTOR2D( 1.0, 1.0 ) to import graphics with their actual size.
     */
    bool Import( const VECTOR2D& aScale = VECTOR2D( 1.0, 1.0 ) );

    /**
     * Collect warning and error messages after loading/importing.
     *
     * @return the list of messages in one string. Each message ends by '\n'
     */
    const wxString& GetMessages() const
    {
        return m_plugin->GetMessages();
    }

    void ReportMsg( const wxString& aMessage )
    {
        m_plugin->ReportMsg( aMessage );
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
     * Set the line width for the imported outlines (in mm).
     */
    void SetLineWidthMM( double aWidth )
    {
        m_lineWidth = aWidth;
    }

    /**
     * Return the line width used for importing the outlines (in mm).
     */
    double GetLineWidthMM() const
    {
        return m_lineWidth;
    }

    /**
     * @return the scale factor affecting the imported shapes.
     */
    VECTOR2D GetScale() const
    {
        return m_scale;
    }

    /**
     * @return the offset in millimeters to add to coordinates when importing graphic items.
     */
    const VECTOR2D& GetImportOffsetMM() const
    {
        return m_offsetCoordmm;
    }

    /**
     * Set the offset in millimeters to add to coordinates when importing graphic items.
     */
    void SetImportOffsetMM( const VECTOR2D& aOffset )
    {
        m_offsetCoordmm = aOffset;
    }

    /**
     * Set the scale factor affecting the imported shapes.
     *
     * This allows conversion between imported shapes units and millimeters.
     */
    void SetScale( const VECTOR2D& aScale )
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
    VECTOR2D ImportScalingFactor() const
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

    /**
     * Empties out the imported shapes list
     */
    void ClearItems()
    {
        m_items.clear();
    }

    /// Default line thickness (in mm).
    static constexpr unsigned int DEFAULT_LINE_WIDTH_DFX = 1;

    virtual void NewShape( POLY_FILL_RULE aFillRule = PF_NONZERO );

    /**
     * Create an object representing a line segment.
     *
     * @param aOrigin is the segment origin point expressed in mm.
     * @param aEnd is the segment end point expressed in mm.
     * @param aStroke is the shape stroke parameters.
     */
    virtual void AddLine( const VECTOR2D& aOrigin, const VECTOR2D& aEnd,
                          const IMPORTED_STROKE& aStroke ) = 0;

    /**
     * Create an object representing a circle.
     *
     * @param aCenter is the circle center point expressed in mm.
     * @param aRadius is the circle radius expressed in mm.
     * @param aStroke is the shape stroke parameters.
     */
    virtual void AddCircle( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke,
                            bool aFilled, const COLOR4D& aFillColor ) = 0;

    /**
     * Create an object representing an arc.
     *
     * @param aCenter is the arc center point expressed in mm.
     * @param aStart is the arc arm end point expressed in mm.
     * Its length is the arc radius.
     * @param aAngle is the arc angle.
     * @param aStroke is the shape stroke parameters.
     */
    virtual void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                         const IMPORTED_STROKE& aStroke ) = 0;

    /**
     * Create an object representing a polygon.
     *
     * @param aVertices is the array of vertices.
     * @param aWidth is the stroke width.
     * @param aStroke is the shape stroke parameters.
     * @param aFillColor is the fill color.
     */
    virtual void AddPolygon( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke,
                             bool aFilled, const COLOR4D& aFillColor ) = 0;

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
     * @param aColor is the shape color
     */
    virtual void AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight,
                          double aWidth, double aThickness, double aOrientation,
                          GR_TEXT_H_ALIGN_T aHJustify, GR_TEXT_V_ALIGN_T aVJustify,
                          const COLOR4D& aColor ) = 0;

    /**
     * Create an object representing an arc.
     *
     * @param aStart is the curve start point expressed in mm.
     * @param aBezierControl1 is the first Bezier control point expressed in mm.
     * @param aBezierControl2 is the second Bezier control point expressed in mm.
     * @param aEnd is the curve end point expressed in mm.
     * @param aStroke is the shape stroke parameters.
     */
    virtual void AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                            const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                            const IMPORTED_STROKE& aStroke ) = 0;

protected:
    /// Add an item to the imported shapes list.
    void addItem( std::unique_ptr<EDA_ITEM> aItem );

    /**
     * Configure a shape as a spline or a line segment if it's degenerate.
     *
     * @return false if the shape is near-zero length and should be ignored.
     */
    bool setupSplineOrLine( EDA_SHAPE& aShape, int aAccuracy );

    /// Factor to convert millimeters to Internal Units.
    double m_millimeterToIu;

    /// Offset (in mm) for imported coordinates.
    VECTOR2D m_offsetCoordmm;

    std::vector<POLY_FILL_RULE> m_shapeFillRules;

private:
    /// List of imported items.
    std::list<std::unique_ptr<EDA_ITEM>> m_items;

    /// Plugin used to load a file.
    std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> m_plugin;

    /// Total image width.
    double m_originalWidth;

    /// Total image Height.
    double m_originalHeight;

    /**
     * Scale factor applied to the imported graphics.
     *
     * 1.0 does not change the size of imported items
     * scale < 1.0 reduce the size of imported items
     */
    VECTOR2D m_scale;

    /// Default line thickness for the imported graphics.
    double m_lineWidth;
};

#endif /* GRAPHICS_IMPORTER_H */
