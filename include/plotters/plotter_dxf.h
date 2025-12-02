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

#pragma once

#include "plotter.h"

/**
 * Legacy colors for DXF file.
 */
enum class DXF_COLOR_T
{
    BLACK = 0,
    RED,
    YELLOW,
    GREEN,
    CYAN,
    BLUE,
    MAGENTA,
    WHITE,
    BROWN,
    ORANGE,
    LIGHTRED,
    LIGHTYELLOW,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTBLUE,
    LIGHTMAGENTA,
    LIGHTORANGE,
    LIGHTGRAY,
    DARKRED,
    DARKYELLOW,
    DARKGREEN,
    DARKCYAN,
    DARKBLUE,
    DARKMAGENTA,
    DARKBROWN,
    DARKORANGE,
    DARKGRAY,
    PURERED,
    PUREYELLOW,
    PUREGREEN,
    PURECYAN,
    PUREBLUE,
    PUREMAGENTA,
    PUREORANGE,
    PUREGRAY,
    REDONE,
    REDTWO,
    REDTHREE,
    REDFOUR,
    REDFIVE,
    REDSIX,
    ORANGEONE,
    ORANGETWO,
    ORANGETHREE,
    ORANGEFOUR,
    ORANGEFIVE,
    REDSEVEN,
    REDEIGHT,
    ORANGESIX,
    REDNINE,
    ORANGESEVEN,
    ORANGEEIGHT,
    ORANGENINE,
    ORANGETEN,
    ORANGEELEVEN,
    ORANGETWELVE,
    ORANGETHIRTEEN,
    ORANGEFOURTEEN,
    YELLOWONE,
    YELLOWTWO,
    YELLOWTHREE,
    YELLOWFOUR,
    YELLOWFIVE,
    YELLOWSIX,
    YELLOWSEVEN,
    YELLOWEIGHT,
    YELLOWNINE,
    YELLOWTEN,
    YELLOWELEVEN,
    YELLOWTWELVE,
    YELLOWTHIRTEEN,
    YELLOWFOURTEEN,
    GREENONE,
    GREENTWO,
    GREENTHREE,
    GREENFOUR,
    GREENFIVE,
    GREENSIX,
    GREENSEVEN,
    GREENEIGHT,
    GREENNINE,
    GREENTEN,
    GREENELEVEN,
    GREENTWELVE,
    GREENTHIRTEEN,
    GREENFOURTEEN,
    GREENFIFTEEN,
    GREENSIXTEEN,
    GREENSEVENTEEN,
    GREENEIGHTEEN,
    GREENNINETEEN,
    GREENTWENTY,
    GREENTWENTYONE,
    GREENTWENTYTWO,
    GREENTWENTYTHREE,
    GREENTWENTYFOUR,
    GREENTWENTYFIVE,
    GREENTWENTYSIX,
    GREENTWENTYSEVEN,
    GREENTWENTYEIGHT,
    GREENTWENTYNINE,
    GREENTHIRTY,
    GREENTHIRTYONE,
    GREENTHIRTYTWO,
    GREENTHIRTYTHREE,
    GREENTHIRTYFOUR,
    GREENTHIRTYFIVE,
    GREENTHIRTYSIX,
    GREENTHIRTYSEVEN,
    GREENTHIRTYEIGHT,
    GREENTHIRTYNINE,
    GREENFORTY,
    GREENFORTYONE,
    GREENFORTYTWO,
    GREENFORTYTHREE,
    GREENFORTYFOUR,
    GREENFORTYFIVE,
    GREENFORTYSIX,
    GREENFORTYSEVEN,
    GREENFORTYEIGHT,
    GREENFORTYNINE,
    GREENFIFTY,
    GREENFIFTYONE,
    GREENFIFTYTWO,
    GREENFIFTYTHREE,
    GREENFIFTYFOUR,
    GREENFIFTYFIVE,
    GREENFIFTYSIX,
    GREENFIFTYSEVEN,
    GREENFIFTYEIGHT,
    GREENFIFTYNINE,
    GREENSIXTY,
    GREENSIXTYONE,
    GREENSIXTYTWO,
    GREENSIXTYTHREE,
    GREENSIXTYFOUR,
    GREENSIXTYFIVE,
    CYANONE,
    CYANTWO,
    CYANTHREE,
    CYANFOUR,
    CYANFIVE,
    CYANSIX,
    CYANSEVEN,
    BLUEONE,
    BLUETWO,
    BLUETHREE,
    BLUEFOUR,
    BLUEFIVE,
    BLUESIX,
    BLUESEVEN,
    BLUEEIGHT,
    BLUENINE,
    BLUETEN,
    BLUEELEVEN,
    BLUETWELVE,
    BLUETHIRTEEN,
    BLUEFOURTEEN,
    BLUEFIFTEEN,
    BLUESIXTEEN,
    BLUESEVENTEEN,
    BLUEEIGHTEEN,
    BLUENINETEEN,
    BLUETWENTY,
    BLUETWENTYONE,
    BLUETWENTYTWO,
    BLUETWENTYTHREE,
    BLUETWENTYFOUR,
    BLUETWENTYFIVE,
    BLUETWENTYSIX,
    BLUETWENTYSEVEN,
    BLUETWENTYEIGHT,
    BLUETWENTYNINE,
    BLUETHIRTY,
    BLUETHIRTYONE,
    BLUETHIRTYTWO,
    BLUETHIRTYETHREE,
    BLUETHIRTYFOUR,
    VIOLETONE,
    VIOLETTWO,
    VIOLETTHREE,
    VIOLETFOUR,
    VIOLETFIVE,
    VIOLETSIX,
    VIOLETSEVEN,
    VIOLETEIGHT,
    VIOLETNINE,
    VIOLETTEN,
    VIOLETELEVEN,
    VIOLETTWELVE,
    VIOLETTHIRTEEN,
    VIOLETFOURTEEN,
    VIOLETFIFTEEN,
    VIOLETSIXTEEN,
    VIOLETSEVENTEEN,
    VIOLETEIGHTEEN,
    VIOLETNINETEEN,
    VIOLETTWENTY,
    VIOLETTWENTYONE,
    VIOLETTWENTYTWO,
    VIOLETTWENTYTHREE,
    VIOLETTWENTYFOUR,
    VIOLETTWENTYFIVE,
    VIOLETTWENTYSIX,
    VIOLETTWENTYSEVEN,
    VIOLETTWENTYEIGHT,
    VIOLETTWENTYNINE,
    VIOLETTHIRTY,
    MAGENTAONE,
    MAGENTATWO,
    MAGENTATHREE,
    MAGENTAFOUR,
    MAGENTAFIVE,
    MAGENTASIX,
    MAGENTASEVEN,
    MAGENTAEIGHT,
    MAGENTANINE,
    MAGENTATEN,
    MAGENTAELEVEN,
    MAGENTATWELVE,
    MAGENTATHIRTEEN,
    MAGENTAFOURTEEN,
    REDTEN,
    REDELEVEN,
    VIOLETTHIRTYONE,
    REDTWELVE,
    REDTHIRTEEN,
    REDFOURTEEN,
    REDFIFTEEN,
    REDSIXTEEN,
    REDSEVENTEEN,
    REDEIGHTEEN,
    REDNINETEEN,
    REDTWENTY,
    REDTWENTYONE,
    REDTWENTYTWO,
    REDTWENTYTHREE,
    REDTWENTYFOUR,
    REDTWENTYFIVE,
    REDTWENTYSIX,
    REDTWENTYSEVEN,
    REDTWENTYEIGHT,
    REDTWENTYNINE,
    REDTHIRTY,
    REDTHIRTYONE,
    GRAYONE,
    GRAYTWO,
    GRAYTHREE,
    GRAYFOUR,
    NBCOLORS  ///< Number of colors
};

class DXF_PLOTTER : public PLOTTER
{
public:
    DXF_PLOTTER() :
            m_textAsLines( false )
    {
        m_textAsLines = true;
        m_currentColor = COLOR4D::BLACK;
        m_currentLineType = LINE_STYLE::SOLID;
        SetUnits( DXF_UNITS::INCH );
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
    virtual bool StartPlot( const wxString& aPageNumber ) override;
    virtual bool EndPlot() override;

    // For now we don't use 'thick' primitives, so no line width
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override
    {
        m_currentPenWidth = 0;
    }

    virtual void SetDash( int aLineWidth, LINE_STYLE aLineStyle ) override;

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
    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;

    /**
     * DXF rectangle: fill not supported.
     */
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                       int aCornerRadius = 0 ) override;

    /**
     * DXF circle: full functionality; it even does 'fills' drawing a
     * circle with a dual-arc polyline wide as the radius.
     *
     * I could use this trick to do other filled primitives.
     */
    virtual void Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width ) override;

    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                      const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth ) override;

    /**
     * DXF polygon: doesn't fill it but at least it close the filled ones
     * DXF does not know thick outline.
     *
     * It does not know thick segments, therefore filled polygons with thick outline
     * are converted to inflated polygon by aWidth/2.
     */
    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                           void* aData = nullptr ) override;

    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aLineChain, FILL_T aFill, int aWidth,
                           void* aData = nullptr ) override;

    virtual void ThickSegment( const VECTOR2I& start, const VECTOR2I& end, int width,
                               void* aData ) override;

    virtual void ThickArc( const VECTOR2D& aCentre, const EDA_ANGLE& aStAngle, const EDA_ANGLE& aAngle,
                           double aRadius, int aWidth, void* aData ) override;

    virtual void ThickRect( const VECTOR2I& p1, const VECTOR2I& p2, int width, void* aData ) override;

    virtual void ThickCircle( const VECTOR2I& pos, int diametre, int width, void* aData ) override;

    virtual void FilledCircle( const VECTOR2I& pos, int diametre, void* aData ) override;

    virtual void ThickPoly( const SHAPE_POLY_SET& aPoly, int aWidth, void* aData ) override;

    virtual void PenTo( const VECTOR2I& pos, char plume ) override;

    /**
     * DXF round pad: always done in sketch mode; it could be filled but it isn't
     * pretty if other kinds of pad aren't...
     */
    virtual void FlashPadCircle( const VECTOR2I& pos, int diametre, void* aData ) override;

    /**
     * DXF oval pad: always done in sketch mode.
     */
    virtual void FlashPadOval( const VECTOR2I& aPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aOrient, void* aData ) override;

    /**
     * DXF rectangular pad: always done in sketch mode.
     */
    virtual void FlashPadRect( const VECTOR2I& aPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aOrient, void* aData ) override;
    virtual void FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                    int aCornerRadius, const EDA_ANGLE& aOrient,
                                    void* aData ) override;
    virtual void FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                 const EDA_ANGLE& aOrient, SHAPE_POLY_SET* aPolygons,
                                 void* aData ) override;

    /**
     * DXF trapezoidal pad: only sketch mode is supported.
     */
    virtual void FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                 const EDA_ANGLE& aPadOrient, void* aData ) override;
    virtual void FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter, int aCornerCount,
                                      const EDA_ANGLE& aOrient, void* aData ) override;

    virtual void Text( const VECTOR2I&        aPos,
                       const COLOR4D&         aColor,
                       const wxString&        aText,
                       const EDA_ANGLE&       aOrient,
                       const VECTOR2I&        aSize,
                       enum GR_TEXT_H_ALIGN_T aH_justify,
                       enum GR_TEXT_V_ALIGN_T aV_justify,
                       int                    aWidth,
                       bool                   aItalic,
                       bool                   aBold,
                       bool                   aMultilineAllowed,
                       KIFONT::FONT*          aFont,
                       const KIFONT::METRICS& aFontMetrics,
                       void*                  aData = nullptr ) override;

    virtual void PlotText( const VECTOR2I&        aPos,
                           const COLOR4D&         aColor,
                           const wxString&        aText,
                           const TEXT_ATTRIBUTES& aAttributes,
                           KIFONT::FONT*          aFont,
                           const KIFONT::METRICS& aFontMetrics,
                           void*                  aData = nullptr ) override;

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

    // Finds the nearest legacy color to the given RGB values (aR, aG, aB).
    int FindNearestLegacyColor( int aR, int aG, int aB );

    // Retrieves the current layer name based on the output mode and optional layer ID.
    wxString GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE aMode, std::optional<PCB_LAYER_ID> aLayerId = std::nullopt );

protected:
    void plotOneLineOfText( const VECTOR2I& aPos, const COLOR4D& aColor, const wxString& aText,
                            const TEXT_ATTRIBUTES& aAttrs );

    bool         m_textAsLines;
    COLOR4D      m_currentColor;
    LINE_STYLE   m_currentLineType;

    DXF_UNITS    m_plotUnits;
    double       m_unitScalingFactor;
    unsigned int m_measurementDirective;
};
