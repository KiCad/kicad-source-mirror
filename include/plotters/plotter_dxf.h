/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Plotting engine (DXF)
 *
 * @file plotter_dxf.h
 */

#pragma once

#include "plotter.h"


class DXF_PLOTTER : public PLOTTER
{
public:
    DXF_PLOTTER() : m_textAsLines( false )
    {
        m_textAsLines = true;
        m_currentColor = COLOR4D::BLACK;
        m_currentLineType = PLOT_DASH_TYPE::SOLID;
        SetUnits( DXF_UNITS::INCHES );
    }

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::DXF;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "dxf" ) );
    }

    /**
     * DXF handles NATIVE text emitting TEXT entities
     */
    virtual void SetTextMode( PLOT_TEXT_MODE mode ) override
    {
        if( mode != PLOT_TEXT_MODE::DEFAULT )
            m_textAsLines = ( mode != PLOT_TEXT_MODE::NATIVE );
    }

    /**
     * Open the DXF plot with a skeleton header.
     */
    virtual bool StartPlot() override;
    virtual bool EndPlot() override;

    // For now we don't use 'thick' primitives, so no line width
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override
    {
        m_currentPenWidth = 0;
    }

    virtual void SetDash( PLOT_DASH_TYPE dashed ) override;

    /**
     * The DXF exporter handles 'colors' as layers...
     */
    virtual void SetColor( const COLOR4D& color ) override;

    /**
     * Set the scale/position for the DXF plot.
     *
     * The DXF engine doesn't support line widths and mirroring. The output
     * coordinate system is in the first quadrant (in mm).
     */
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;

    /**
     * DXF rectangle: fill not supported.
     */
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;

    /**
     * DXF circle: full functionality; it even does 'fills' drawing a
     * circle with a dual-arc polyline wide as the radius.
     *
     * I could use this trick to do other filled primitives.
     */
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;

    /**
     * DXF polygon: doesn't fill it but at least it close the filled ones
     * DXF does not know thick outline.
     *
     * It does not know thick segments, therefore filled polygons with thick outline
     * are converted to inflated polygon by aWidth/2.
     */
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList, FILL_T aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;
    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               OUTLINE_MODE tracemode, void* aData ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PenTo( const wxPoint& pos, char plume ) override;

    /**
     * DXF round pad: always done in sketch mode; it could be filled but it isn't
     * pretty if other kinds of pad aren't...
     */
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 OUTLINE_MODE trace_mode, void* aData ) override;

    /**
     * DXF oval pad: always done in sketch mode.
     */
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, double orient,
                               OUTLINE_MODE trace_mode, void* aData ) override;

    /**
     * DXF rectangular pad: always done in sketch mode.
     */
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               double orient, OUTLINE_MODE trace_mode, void* aData ) override;
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize, double aOrient,
                                 SHAPE_POLY_SET* aPolygons,
                                 OUTLINE_MODE aTraceMode, void* aData ) override;

    /**
     * DXF trapezoidal pad: only sketch mode is supported.
     */
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashRegularPolygon( const wxPoint& aShapePos, int aDiameter, int aCornerCount,
                            double aOrient, OUTLINE_MODE aTraceMode, void* aData ) override;

    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D&              aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = nullptr ) override;


    /**
     * Set the units to use for plotting the DXF file.
     *
     * @param aUnit - The units to use
     */
    void SetUnits( DXF_UNITS aUnit );

    /**
     * The units currently enabled for plotting
     *
     * @return The currently configured units
     */
    DXF_UNITS GetUnits() const
    {
        return m_plotUnits;
    }

    /**
     * Get the scale factor to apply to convert the device units to be in the
     * currently set units.
     *
     * @return Scaling factor to apply for unit conversion
     */
    double GetUnitScaling() const
    {
        return m_unitScalingFactor;
    }

    /**
     * Get the correct value for the $MEASUREMENT field given the current units
     *
     * @return the $MEASUREMENT directive field value
     */
    unsigned int GetMeasurementDirective() const
    {
        return m_measurementDirective;
    }

protected:
    bool           m_textAsLines;
    COLOR4D        m_currentColor;
    PLOT_DASH_TYPE m_currentLineType;

    DXF_UNITS      m_plotUnits;
    double         m_unitScalingFactor;
    unsigned int   m_measurementDirective;
};
