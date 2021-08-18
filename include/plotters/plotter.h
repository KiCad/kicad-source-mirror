/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Plot settings, and plotting engines (PostScript, Gerber, HPGL and DXF)
 *
 * @file plotter.h
 */

#ifndef PLOT_COMMON_H_
#define PLOT_COMMON_H_

#include <fill_type.h>
#include <vector>
#include <math/box2.h>
#include <gr_text.h>
#include <page_info.h>
#include <outline_mode.h>
#include <gal/color4d.h>
#include <render_settings.h>

class COLOR_SETTINGS;
class SHAPE_ARC;
class SHAPE_POLY_SET;
class SHAPE_LINE_CHAIN;
class GBR_NETLIST_METADATA;
class PROJECT;

using KIGFX::RENDER_SETTINGS;


// Must be in the same order as the drop-down list in the plot dialog inside pcbnew
// Units (inch/mm for DXF plotter
enum class DXF_UNITS
{
    INCHES = 0,
    MILLIMETERS = 1
};


/**
 * The set of supported output plot formats.
 *
   They should be kept in order of the radio buttons in the plot panel/windows.
 */
enum class PLOT_FORMAT
{
    UNDEFINED    = -1,
    FIRST_FORMAT = 0,
    HPGL         = FIRST_FORMAT,
    GERBER,
    POST,
    DXF,
    PDF,
    SVG,
    LAST_FORMAT = SVG
};

/**
 * Which kind of text to output with the PSLIKE plotters.
 *
 * You can:
 * 1) only use the internal vector font
 * 2) only use native postscript fonts
 * 3) use the internal vector font and add 'phantom' text to aid
 *    searching
 * 4) keep the default for the plot driver
 *
 * This is recognized by the DXF driver too, where NATIVE emits
 * TEXT entities instead of stroking the text
 */
enum class PLOT_TEXT_MODE
{
    STROKE,
    NATIVE,
    PHANTOM,
    DEFAULT
};

/**
 * Dashed line types.
 */
enum class PLOT_DASH_TYPE
{
    DEFAULT    = -1,
    SOLID      = 0,
    FIRST_TYPE = SOLID,
    DASH,
    DOT,
    DASHDOT,
    LAST_TYPE = DASHDOT
};

/**
 * Base plotter engine class. General rule: all the interface with the caller
 * is done in IU, the IU size is specified with SetViewport. Internal and
 * output processing is usually done in decimils (or whatever unit the
 * effective engine class need to use)
 */
class PLOTTER
{
public:
    // These values are used as flag for pen or aperture selection
    static const int DO_NOT_SET_LINE_WIDTH = -2;    // Skip selection
    static const int USE_DEFAULT_LINE_WIDTH = -1;   // use the default pen

    PLOTTER();

    virtual ~PLOTTER();

    /**
     * Returns the effective plot engine in use. It's not very OO but for
     * now is required since some things are only done with some output devices
     * (like drill marks, emitted only for postscript
     */
    virtual PLOT_FORMAT GetPlotterType() const = 0;

    virtual bool StartPlot() = 0;
    virtual bool EndPlot() = 0;

    virtual void SetNegative( bool aNegative )
    {
        m_negativeMode = aNegative;
    }

    /**
     * Plot in B/W or color.
     *
     * @param aColorMode use true to plot in color, false to plot in black and white.
     */
    virtual void SetColorMode( bool aColorMode ) { m_colorMode = aColorMode; }
    bool GetColorMode() const { return m_colorMode; }

    void SetRenderSettings( RENDER_SETTINGS* aSettings ) { m_renderSettings = aSettings; }
    RENDER_SETTINGS* RenderSettings() { return m_renderSettings; }

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) { m_pageInfo = aPageSettings; }
    PAGE_INFO& PageSettings() { return m_pageInfo; }

    /**
     * Set the line width for the next drawing.
     *
     * @param width is specified in IUs.
     * @param aData is an auxiliary parameter, mainly used in gerber plotter.
     */
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) = 0;
    virtual int GetCurrentLineWidth() const  { return m_currentPenWidth; }

    virtual void SetColor( const COLOR4D& color ) = 0;

    virtual void SetDash( PLOT_DASH_TYPE dashed ) = 0;

    virtual void SetCreator( const wxString& aCreator ) { m_creator = aCreator; }

    virtual void SetTitle( const wxString& aTitle ) { m_title = aTitle; }

    /**
     * Add a line to the list of free lines to print at the beginning of the file.
     *
     * @param aExtraString is the string to print
     */
    void AddLineToHeader( const wxString& aExtraString )
    {
        m_headerExtraLines.Add( aExtraString );
    }

    /**
     * Remove all lines from the list of free lines to print at the beginning of the file
     */
    void ClearHeaderLinesList()
    {
        m_headerExtraLines.Clear();
    }

    /**
     * Set the plot offset and scaling for the current plot
     *
     * @param aOffset is the plot offset.
     * @param aIusPerDecimil gives the scaling factor from IUs to device units
     * @param aScale is the user set plot scaling factor (either explicitly
     *      or using 'fit to A4').
     * @param aMirror flips the plot in the Y direction (useful for toner
     *      transfers or some kind of film).
     */
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) = 0;

    /**
     * Open or create the plot file \a aFullFilename.
     *
     * @param aFullFilename is the full file name of the file to create.
     * @return true if success, false if the file cannot be created/opened.
     *
     * Virtual because some plotters use ascii files, some others binary files (PDF)
     * The base class open the file in text mode
     */
    virtual bool OpenFile( const wxString& aFullFilename );

    /**
     * The IUs per decimil are an essential scaling factor when
     * plotting; they are set and saved when establishing the viewport.
     * Here they can be get back again
     */
    double GetIUsPerDecimil() const { return m_IUsPerDecimil; }

    int GetPlotterArcLowDef() const { return m_IUsPerDecimil * 8; }
    int GetPlotterArcHighDef() const { return m_IUsPerDecimil * 2; }

    // Low level primitives
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) = 0;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_TYPE fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) = 0;

    /**
     * Generic fallback: arc rendered as a polyline.
     */
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_TYPE fill, int width = USE_DEFAULT_LINE_WIDTH );
    virtual void Arc(  const SHAPE_ARC& aArc );

    /**
     * Generic fallback: Cubic Bezier curve rendered as a polyline
     * In KiCad the bezier curves have 4 control points:
     * start ctrl1 ctrl2 end
     */
    virtual void BezierCurve( const wxPoint& aStart, const wxPoint& aControl1,
                              const wxPoint& aControl2, const wxPoint& aEnd,
                              int aTolerance, int aLineThickness = USE_DEFAULT_LINE_WIDTH );

    /**
     * Moveto/lineto primitive, moves the 'pen' to the specified direction.
     *
     * @param pos is the target position.
     * @param plume specifies the kind of motion: 'U' only moves the pen,
     *      'D' draw a line from the current position and 'Z' finish
     *      the drawing and returns the 'pen' to rest (flushes the trace).
     */
    virtual void PenTo( const wxPoint& pos, char plume ) = 0;

    // Convenience functions for PenTo
    void MoveTo( const wxPoint& pos )
    {
        PenTo( pos, 'U' );
    }

    void LineTo( const wxPoint& pos )
    {
        PenTo( pos, 'D' );
    }

    void FinishTo( const wxPoint& pos )
    {
        PenTo( pos, 'D' );
        PenTo( pos, 'Z' );
    }

    void PenFinish()
    {
        // The point is not important with Z motion
        PenTo( wxPoint( 0, 0 ), 'Z' );
    }

    /**
     * Draw a polygon ( filled or not ).
     *
     * @param aCornerList is the corners list (a std::vector< wxPoint >).
     * @param aFill is the type of fill.
     * @param aWidth is the line width.
     * @param aData is an auxiliary info (mainly for gerber format).
     */
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList, FILL_TYPE aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) = 0;

    /**
     * Draw a polygon ( filled or not ).
     * @param aCornerList is the corners list (a SHAPE_LINE_CHAIN).
     *        must be closed (IsClosed() == true) for a polygon. Otherwise this is a polyline.
     * @param aFill is the type of fill.
     * @param aWidth is the line width.
     * @param aData is an auxiliary info (mainly for gerber format).
     */
    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aCornerList, FILL_TYPE aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr );

    /**
     * Only PostScript plotters can plot bitmaps.
     *
     * A rectangle is plotted for plotters that cannot plot a bitmap.
     *
     * @param aImage is the bitmap.
     * @param aPos is position of the center of the bitmap.
     * @param aScaleFactor is the scale factor to apply to the bitmap size
     *                      (this is not the plot scale factor).
     */
    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos, double aScaleFactor );

    // Higher level primitives -- can be drawn as line, sketch or 'filled'
    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               OUTLINE_MODE tracemode, void* aData );
    virtual void ThickArc( const wxPoint& centre, double StAngle, double EndAngle,
                           int rayon, int width, OUTLINE_MODE tracemode, void* aData );
    virtual void ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                            OUTLINE_MODE tracemode, void* aData );
    virtual void ThickCircle( const wxPoint& pos, int diametre, int width,
                              OUTLINE_MODE tracemode, void* aData );
    virtual void FilledCircle( const wxPoint& pos, int diametre,
                              OUTLINE_MODE tracemode, void* aData );


    // Flash primitives

    /**
     * @param aPadPos Position of the shape (center of the rectangle.
     * @param aDiameter is the diameter of round pad.
     * @param aTraceMode is the drawing mode, FILLED or SKETCH.
     * @param aData is an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadCircle( const wxPoint& aPadPos, int aDiameter,
                                 OUTLINE_MODE aTraceMode, void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape (center of the rectangle.
     * @param aSize is the size of oblong shape.
     * @param aPadOrient The rotation of the shape.
     * @param aTraceMode is the drawing mode, FILLED or SKETCH.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadOval( const wxPoint& aPadPos, const wxSize& aSize, double aPadOrient,
                               OUTLINE_MODE aTraceMode, void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape (center of the rectangle).
     * @param aSize is the size of rounded rect.
     * @param aPadOrient The rotation of the shape.
     * @param aTraceMode is the drawing mode, FILLED or SKETCH.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadRect( const wxPoint& aPadPos, const wxSize& aSize,
                               double aPadOrient, OUTLINE_MODE aTraceMode, void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape (center of the rectangle.
     * @param aSize is the size of rounded rect.
     * @param aCornerRadius Radius of the rounded corners.
     * @param aOrient The rotation of the shape.
     * @param aTraceMode is the drawing mode, FILLED or SKETCH.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    OUTLINE_MODE aTraceMode, void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape.
     * @param aSize is the size of round reference pad.
     * @param aPadOrient is the pad rotation, used only with aperture macros (Gerber plotter).
     * @param aPolygons the shape as polygon set.
     * @param aTraceMode is the drawing mode, FILLED or SKETCH.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 double aPadOrient, SHAPE_POLY_SET* aPolygons,
                                 OUTLINE_MODE aTraceMode, void* aData ) = 0;

    /**
     * Flash a trapezoidal pad.
     *
     * @param aPadPos is the the position of the shape.
     * @param aCorners is the list of 4 corners positions, relative to the shape position,
     *                 pad orientation 0.
     * @param aPadOrient is the rotation of the shape.
     * @param aTraceMode is the drawing mode, FILLED or SKETCH.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, OUTLINE_MODE aTraceMode,
                                 void* aData ) = 0;

    /**
     * Flash a regular polygon. Useful only in Gerber files to flash a regular polygon.
     *
     * @param aShapePos is the center of the circle containing the polygon.
     * @param aRadius is the radius of the circle containing the polygon.
     * @param aCornerCount is the number of vertices.
     * @param aOrient is the polygon rotation in degrees.
     * @param aData is a auxiliary parameter used (if needed) to handle extra info
     *              specific to the plotter.
     */
    virtual void FlashRegularPolygon( const wxPoint& aShapePos, int aDiameter, int aCornerCount,
                                      double aOrient, OUTLINE_MODE aTraceMode, void* aData ) = 0 ;

    /**
     * Draw text with the plotter.
     *
     * For convenience it accept the color to use for specific plotters (GERBER) aData is used
     * to pass extra parameters.
     */
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
                       void*                       aData = nullptr );

    /**
     * Draw a marker (used for the drill map).
     */
    static const unsigned MARKER_COUNT = 58;

    /**
     * Draw a pattern shape number aShapeId, to coord position.
     *
     * @param aPosition is the position of the marker.
     * @param aDiameter is the diameter of the marker.
     * @param aShapeId is the index (used to generate forms characters).
     */
    void Marker( const wxPoint& position, int diametre, unsigned aShapeId );

    /**
     * Set the current Gerber layer polarity to positive or negative
     * by writing \%LPD*\% or \%LPC*\% to the Gerber file, respectively.
     * (obviously starts a new Gerber layer, too)
     *
     * @param aPositive is the layer polarity and true for positive.
     * It's not useful with most other plotter since they can't 'scratch'
     * the film like photoplotter imagers do
     */
    virtual void SetLayerPolarity( bool aPositive )
    {
        // NOP for most plotters
    }

    /**
     * Change the current text mode. See the PlotTextMode
     * explanation at the beginning of the file.
     */
    virtual void SetTextMode( PLOT_TEXT_MODE mode )
    {
        // NOP for most plotters.
    }

    virtual void SetGerberCoordinatesFormat( int aResolution, bool aUseInches = false )
    {
        // NOP for most plotters. Only for Gerber plotter
    }

    virtual void SetSvgCoordinatesFormat( unsigned aResolution, bool aUseInches = false )
    {
        // NOP for most plotters. Only for SVG plotter
    }

    /**
     * calling this function allows one to define the beginning of a group
     * of drawing items, for instance in SVG  or Gerber format.
     * (example: group all segments of a letter or a text)
     * @param aData can define any parameter
     * for most of plotters: do nothing
     */
    virtual void StartBlock( void* aData ) {}

    /**
     * calling this function allows one to define the end of a group of drawing
     * items for instance in SVG  or Gerber format.
     * the group is started by StartBlock()
     * @param aData can define any parameter
     * for most of plotters: do nothing
     */
    virtual void EndBlock( void* aData ) {}


protected:
    // These are marker subcomponents
    /**
     * Plot a circle centered on the position. Building block for markers
     */
    void markerCircle( const wxPoint& pos, int radius );

    /**
     * Plot a - bar centered on the position. Building block for markers
     */
    void markerHBar( const wxPoint& pos, int radius );

    /**
     * Plot a / bar centered on the position. Building block for markers
     */
    void markerSlash( const wxPoint& pos, int radius );

    /**
     * Plot a \ bar centered on the position. Building block for markers
     */
    void markerBackSlash( const wxPoint& pos, int radius );

    /**
     * Plot a | bar centered on the position. Building block for markers
     */
    void markerVBar( const wxPoint& pos, int radius );

    /**
     * Plot a square centered on the position. Building block for markers
     */
    void markerSquare( const wxPoint& position, int radius );

    /**
     * Plot a lozenge centered on the position. Building block for markers
     */
    void markerLozenge( const wxPoint& position, int radius );

    // Helper function for sketched filler segment

    /**
     * Convert a thick segment and plot it as an oval
     */
    void segmentAsOval( const wxPoint& start, const wxPoint& end, int width,
                        OUTLINE_MODE tracemode );

    void sketchOval( const wxPoint& pos, const wxSize& size, double orient, int width );

    // Coordinate and scaling conversion functions

    /**
     * Modify coordinates according to the orientation, scale factor, and offsets trace. Also
     * convert from a wxPoint to DPOINT, since some output engines needs floating point
     * coordinates.
     */
    virtual DPOINT userToDeviceCoordinates( const wxPoint& aCoordinate );

    /**
     * Modify size according to the plotter scale factors (wxSize version, returns a DPOINT).
     */
    virtual DPOINT userToDeviceSize( const wxSize& size );

    /**
     * Modify size according to the plotter scale factors (simple double version).
     */
    virtual double userToDeviceSize( double size ) const;

    double GetDotMarkLenIU() const;

    double GetDashMarkLenIU() const;

    double GetDashGapLenIU() const;

protected:      // variables used in most of plotters:
    /// Plot scale - chosen by the user (even implicitly with 'fit in a4')
    double           m_plotScale;

    /* Caller scale (how many IUs in a decimil - always); it's a double
     * because in Eeschema there are 0.1 IUs in a decimil (Eeschema
     * always works in mils internally) while PcbNew can work in decimil
     * or nanometers, so this value would be >= 1 */
    double           m_IUsPerDecimil;

    double           m_iuPerDeviceUnit;     // Device scale (from IUs to plotter device units;
                                            // usually decimils)
    wxPoint          m_plotOffset;          // Plot offset (in IUs)
    bool             m_plotMirror;          // X axis orientation (SVG)
                                            // and plot mirrored (only for PS, PDF HPGL and SVG)
    bool             m_mirrorIsHorizontal;  // true to mirror horizontally (else vertically)
    bool             m_yaxisReversed;       // true if the Y axis is top to bottom (SVG)

    /// Output file
    FILE*            m_outputFile;

    // Pen handling
    bool             m_colorMode;           // true to plot in color, otherwise black & white
    bool             m_negativeMode;        // true to generate a negative image (PS mode mainly)
    int              m_currentPenWidth;
    char             m_penState;            // current pen state: 'U', 'D' or 'Z' (see PenTo)
    wxPoint          m_penLastpos;          // last pen position; -1,-1 when pen is at rest

    wxString         m_creator;
    wxString         m_filename;
    wxString         m_title;
    PAGE_INFO        m_pageInfo;
    wxSize           m_paperSize;           // Paper size in IU - not in mils

    wxArrayString    m_headerExtraLines;    // a set of string to print in header file

    RENDER_SETTINGS* m_renderSettings;
};


class TITLE_BLOCK;

void PlotDrawingSheet( PLOTTER* plotter, const PROJECT* aProject, const TITLE_BLOCK& aTitleBlock,
                       const PAGE_INFO& aPageInfo, const wxString& aSheetNumber, int aSheetCount,
                       const wxString& aSheetDesc, const wxString& aFilename,
                       COLOR4D aColor = COLOR4D::UNSPECIFIED, bool aIsFirstPage = true );

/** Returns the default plot extension for a format
  */
wxString GetDefaultPlotExtension( PLOT_FORMAT aFormat );


#endif  // PLOT_COMMON_H_
