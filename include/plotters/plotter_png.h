/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef PLOTTER_PNG_H
#define PLOTTER_PNG_H

#include "plotter.h"

#include <cairo.h>


constexpr int DEFAULT_PNG_DPI = 300;
constexpr int MIN_PNG_DPI = 72;
constexpr int MAX_PNG_DPI = 2400;


/**
 * PNG rasterization plotter using Cairo graphics library.
 *
 * This plotter creates PNG images from KiCad drawings, used for
 * Gerber file visualization, diff output, and PCB board plotting.
 */
class PNG_PLOTTER : public PLOTTER
{
public:
    PNG_PLOTTER();
    virtual ~PNG_PLOTTER();

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::PNG;
    }

    static wxString GetDefaultFileExtension() { return wxString( wxT( "png" ) ); }

    /**
     * Set the output resolution in dots per inch.
     * @param aDPI resolution value (default 300)
     */
    void SetResolution( int aDPI ) { m_dpi = aDPI; }
    int  GetResolution() const { return m_dpi; }

    /**
     * Set the output image dimensions in pixels.
     * @param aWidth image width in pixels
     * @param aHeight image height in pixels
     */
    void SetPixelSize( int aWidth, int aHeight )
    {
        m_width = aWidth;
        m_height = aHeight;
    }

    int GetPixelWidth() const { return m_width; }
    int GetPixelHeight() const { return m_height; }

    /**
     * Set the background color for the image.
     * Default is transparent (alpha = 0).
     */
    void    SetBackgroundColor( const COLOR4D& aColor ) { m_backgroundColor = aColor; }
    COLOR4D GetBackgroundColor() const { return m_backgroundColor; }

    /**
     * Enable or disable anti-aliasing.
     * @param aEnable true for smooth edges, false for hard pixel edges
     */
    void SetAntialias( bool aEnable ) { m_antialias = aEnable; }
    bool GetAntialias() const { return m_antialias; }

    /**
     * Set whether the Y axis is reversed (Y-up vs Y-down).
     *
     * pcbnew uses Y-up coordinates while gerbview/Cairo use Y-down.
     * Default is false (Y-down, matching Cairo).
     *
     * Note: uses the base class PLOTTER::m_yaxisReversed member.
     */
    void SetYAxisReversed( bool aReversed ) { m_yaxisReversed = aReversed; }
    bool GetYAxisReversed() const { return m_yaxisReversed; }

    // PLOTTER interface implementation
    virtual bool OpenFile( const wxString& aFullFilename ) override;
    virtual bool StartPlot( const wxString& aPageNumber ) override;
    virtual bool EndPlot() override;

    virtual void SetCurrentLineWidth( int aWidth, void* aData = nullptr ) override;
    virtual void SetColor( const COLOR4D& aColor ) override;
    virtual void SetDash( int aLineWidth, LINE_STYLE aLineStyle ) override;

    /**
     * Switch the Cairo compositing operator between CLEAR and OVER.
     *
     * Use aClear=true to punch transparent holes in the alpha channel (negative/clear polarity
     * items on transparent exports). Restore with aClear=false when done.
     */
    void SetClearCompositing( bool aClear );

    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil, double aScale, bool aMirror ) override;

    // Primitive drawing operations
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T aFill, int aWidth,
                       int aCornerRadius = 0 ) override;

    virtual void Circle( const VECTOR2I& aCenter, int aDiameter, FILL_T aFill, int aWidth ) override;

    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle, double aRadius,
                      FILL_T aFill, int aWidth ) override;

    virtual void PenTo( const VECTOR2I& aPos, char aPlume ) override;

    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                           void* aData = nullptr ) override;

    virtual void PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor ) override;

    // Flash pad operations
    virtual void FlashPadCircle( const VECTOR2I& aPadPos, int aDiameter, void* aData ) override;

    virtual void FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                               void* aData ) override;

    virtual void FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                               void* aData ) override;

    virtual void FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize, int aCornerRadius,
                                    const EDA_ANGLE& aOrient, void* aData ) override;

    virtual void FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                                 SHAPE_POLY_SET* aPolygons, void* aData ) override;

    virtual void FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners, const EDA_ANGLE& aPadOrient,
                                 void* aData ) override;

    virtual void FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter, int aCornerCount,
                                      const EDA_ANGLE& aOrient, void* aData ) override;

    /**
     * Save the rendered image to a PNG file.
     * @param aPath output file path
     * @return true on success, false on failure
     */
    bool SaveFile( const wxString& aPath );

protected:
    /**
     * Transform coordinates from user space (IU) to device space (pixels).
     */
    virtual VECTOR2D userToDeviceCoordinates( const VECTOR2I& aCoordinate ) override;

    /**
     * Transform a size from user space to device space.
     */
    virtual VECTOR2D userToDeviceSize( const VECTOR2I& aSize ) override;

    /**
     * Transform a size from user space to device space (scalar version).
     */
    virtual double userToDeviceSize( double aSize ) const override;

private:
    void fillRect( double aX, double aY, double aWidth, double aHeight );
    void strokeRect( double aX, double aY, double aWidth, double aHeight );
    void fillCircle( double aCx, double aCy, double aRadius );
    void strokeCircle( double aCx, double aCy, double aRadius );

    cairo_surface_t* m_surface;
    cairo_t*         m_context;

    int     m_dpi;
    int     m_width;
    int     m_height;
    bool    m_antialias;
    COLOR4D m_backgroundColor;
    COLOR4D m_currentColor;
};

#endif // PLOTTER_PNG_H
