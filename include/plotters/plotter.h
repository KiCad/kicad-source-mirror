/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef PLOT_COMMON_H_
#define PLOT_COMMON_H_

#include <eda_shape.h>
#include <vector>
#include <math/box2.h>
#include <gr_text.h>
#include <page_info.h>
#include <gal/color4d.h>
#include <stroke_params.h>
#include <render_settings.h>
#include <font/font.h>


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
    INCH = 0,   // Do not use MM: it conficts with a Windows header
    MM = 1
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
 * Options to draw items with thickness ( segments, arcs, circles, texts...)
 */
enum DXF_OUTLINE_MODE
{
    SKETCH = 0,     // sketch mode: draw segments outlines only
    FILLED = 1      // normal mode: solid segments
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

class PLOT_PARAMS
{
public:
    virtual DXF_OUTLINE_MODE GetDXFPlotMode() const { wxFAIL; return DXF_OUTLINE_MODE::FILLED; }
    virtual PLOT_TEXT_MODE GetTextMode() const { return PLOT_TEXT_MODE::DEFAULT; }
};


/**
 * @enum DXF_LAYER_OUTPUT_MODE
 * @brief Specifies the output mode for the DXF layer.
 *
 * This enumeration is used to define the mode of output for the DXF layer.
 * It allows the user to choose between retrieving the layer name or the color name.
 */
enum class DXF_LAYER_OUTPUT_MODE
{
    Layer_Name,
    Layer_Color_Name,
    Current_Layer_Name,
    Current_Layer_Color_Name
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

    PLOTTER( const PROJECT* aProject = nullptr );

    virtual ~PLOTTER();

    /**
     * Return the effective plot engine in use. It's not very OO but for
     * now is required since some things are only done with some output devices
     * (like drill marks, emitted only for postscript
     */
    virtual PLOT_FORMAT GetPlotterType() const = 0;

    virtual bool StartPlot( const wxString& aPageNumber ) = 0;
    virtual bool EndPlot() = 0;

    virtual void SetNegative( bool aNegative ) { m_negativeMode = aNegative; }

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

    void SetPlotMirrored( bool aMirror ) { m_plotMirror = aMirror; };
    bool GetPlotMirrored() const { return m_plotMirror; }

    /**
     * Set the line width for the next drawing.
     *
     * @param width is specified in IUs.
     * @param aData is an auxiliary parameter, mainly used in gerber plotter.
     */
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) = 0;
    virtual int GetCurrentLineWidth() const  { return m_currentPenWidth; }

    virtual void SetColor( const COLOR4D& color ) = 0;

    virtual void SetDash( int aLineWidth, LINE_STYLE aLineStyle ) = 0;

    virtual void SetCreator( const wxString& aCreator ) { m_creator = aCreator; }
    virtual void SetTitle( const wxString& aTitle ) { m_title = aTitle; }
    virtual void SetAuthor( const wxString& aAuthor ) { m_author = aAuthor; }
    virtual void SetSubject( const wxString& aSubject ) { m_subject = aSubject; }

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
    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) = 0;

    /**
     * Sets the list of layers to export to the specified vector.
     *
     * This function updates the member variable m_layersToExport with the
     * vector provided in aLayersToExport.
     *
     * @param aLayersToExport The vector containing the names of layers to export.
     *                        This updates the internal list of layers that will
     *                        be processed for export.
     */
    void SetLayersToExport( const std::vector<std::pair<PCB_LAYER_ID, wxString>>& aLayersToExport ) { m_layersToExport = aLayersToExport; }

    /**
     * @brief Gets the ID of the current layer.
     *
     * This function returns the ID of the layer that the current item is on.
     *
     * @return PCB_LAYER_ID The ID of the current layer.
     */
    PCB_LAYER_ID GetLayer() const { return m_layer; }

    /**
     * @brief Sets the ID of the current layer.
     *
     * This function sets the ID of the layer for the current item.
     *
     * @param aLayer The ID of the layer to be set.
     */
    void SetLayer( PCB_LAYER_ID aLayer ) { m_layer = aLayer; }

    /**
     * Open or create the plot file \a aFullFilename.
     * .
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
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                       int aCornerRadius = 0 ) = 0;
    virtual void Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width ) = 0;

    virtual void Arc( const VECTOR2D& aStart, const VECTOR2D& aMid, const VECTOR2D& aEnd,
                      FILL_T aFill, int aWidth );

    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                      const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill,
                      int aWidth );

    /**
     * Generic fallback: Cubic Bezier curve rendered as a polyline.
     *
     * In KiCad the bezier curves have 4 control points: start ctrl1 ctrl2 end
     */
    virtual void BezierCurve( const VECTOR2I& aStart, const VECTOR2I& aControl1,
                              const VECTOR2I& aControl2, const VECTOR2I& aEnd,
                              int aTolerance, int aLineThickness );

    /**
     * Moveto/lineto primitive, moves the 'pen' to the specified direction.
     *
     * @param pos is the target position.
     * @param plume specifies the kind of motion: 'U' only moves the pen,
     *      'D' draw a line from the current position and 'Z' finish
     *      the drawing and returns the 'pen' to rest (flushes the trace).
     */
    virtual void PenTo( const VECTOR2I& pos, char plume ) = 0;

    // Convenience functions for PenTo
    void MoveTo( const VECTOR2I& pos )
    {
        PenTo( pos, 'U' );
    }

    void LineTo( const VECTOR2I& pos )
    {
        PenTo( pos, 'D' );
    }

    void FinishTo( const VECTOR2I& pos )
    {
        PenTo( pos, 'D' );
        PenTo( pos, 'Z' );
    }

    void PenFinish()
    {
        // The point is not important with Z motion
        PenTo( VECTOR2I( 0, 0 ), 'Z' );
    }

    /**
     * Draw a polygon ( filled or not ).
     *
     * @param aCornerList is the corners list (a std::vector< VECTOR2I >).
     * @param aFill is the type of fill.
     * @param aWidth is the line width.
     * @param aData is an auxiliary info (mainly for gerber format).
     */
    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                           void* aData ) = 0;

    /**
     * Draw a polygon ( filled or not ).
     *
     * @param aLineChain is a list of segments and arcs.  Must be closed (IsClosed() == true) for
     *                   a polygon. Otherwise this is a polyline.
     * @param aFill is the type of fill.
     * @param aWidth is the line width.
     * @param aData is an auxiliary info (mainly for gerber format).
     */
    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aLineChain, FILL_T aFill, int aWidth, void* aData );

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
    virtual void PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor );

    // Higher level primitives -- can be drawn as line, sketch or 'filled'
    virtual void ThickSegment( const VECTOR2I& start, const VECTOR2I& end, int width, void* aData );

    virtual void ThickArc( const EDA_SHAPE& aArcShape, void* aData, int aWidth );

    virtual void ThickArc( const VECTOR2D& aCentre, const EDA_ANGLE& aStAngle,
                           const EDA_ANGLE& aAngle, double aRadius, int aWidth, void* aData );

    virtual void ThickRect( const VECTOR2I& p1, const VECTOR2I& p2, int width, void* aData );

    virtual void ThickCircle( const VECTOR2I& pos, int diametre, int width, void* aData );

    virtual void FilledCircle( const VECTOR2I& pos, int diametre, void* aData );

    virtual void ThickOval( const VECTOR2I& aPos, const VECTOR2I& aSize, const EDA_ANGLE& aOrient,
                            int aWidth, void* aData );

    virtual void ThickPoly( const SHAPE_POLY_SET& aPoly, int aWidth, void* aData );

    // Flash primitives

    /**
     * @param aPadPos Position of the shape (center of the rectangle.
     * @param aDiameter is the diameter of round pad.
     * @param aData is an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadCircle( const VECTOR2I& aPadPos, int aDiameter, void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape (center of the rectangle.
     * @param aSize is the size of oblong shape.
     * @param aPadOrient The rotation of the shape.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aPadOrient, void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape (center of the rectangle).
     * @param aSize is the size of rounded rect.
     * @param aPadOrient The rotation of the shape.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aPadOrient, void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape (center of the rectangle.
     * @param aSize is the size of rounded rect.
     * @param aCornerRadius Radius of the rounded corners.
     * @param aOrient The rotation of the shape.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                    int aCornerRadius, const EDA_ANGLE& aOrient,
                                    void* aData ) = 0;

    /**
     * @param aPadPos Position of the shape.
     * @param aSize is the size of round reference pad.
     * @param aPadOrient is the pad rotation, used only with aperture macros (Gerber plotter).
     * @param aPolygons the shape as polygon set.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                 const EDA_ANGLE& aPadOrient, SHAPE_POLY_SET* aPolygons,
                                 void* aData ) = 0;

    /**
     * Flash a trapezoidal pad.
     *
     * @param aPadPos is the the position of the shape.
     * @param aCorners is the list of 4 corners positions, relative to the shape position,
     *                 pad orientation 0.
     * @param aPadOrient is the rotation of the shape.
     * @param aData an auxiliary info (mainly for gerber format attributes).
     */
    virtual void FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                 const EDA_ANGLE& aPadOrient, void* aData ) = 0;

    /**
     * Flash a regular polygon. Useful only in Gerber files to flash a regular polygon.
     *
     * @param aShapePos is the center of the circle containing the polygon.
     * @param aRadius is the radius of the circle containing the polygon.
     * @param aCornerCount is the number of vertices.
     * @param aOrient is the polygon rotation.
     * @param aData is a auxiliary parameter used (if needed) to handle extra info
     *              specific to the plotter.
     */
    virtual void FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter, int aCornerCount,
                                      const EDA_ANGLE& aOrient, void* aData ) = 0;

    /**
     * Draw text with the plotter.
     *
     * For convenience it accept the color to use for specific plotters
     * aData is used to pass extra parameters (GERBER).
     *
     * @param aPos is the text position (according to aH_justify, aV_justify).
     * @param aColor is the text color.
     * @param aText is the text to draw.
     * @param aOrient is the angle.
     * @param aSize is the text size (size.x or size.y can be < 0 for mirrored texts).
     * @param aH_justify is the horizontal justification (Left, center, right).
     * @param aV_justify is the vertical justification (bottom, center, top).
     * @param aPenWidth is the line width (if = 0, use plot default line width).
     * @param aItalic is the true to simulate an italic font.
     * @param aBold use true to use a bold font Useful only with default width value
     *              (aPenWidth = 0).
     * @param aMultilineAllowed use true to plot text as multiline, otherwise single line.
     * @param aData is a parameter used by some plotters in SetCurrentLineWidth(),
     *              not directly used here.
     */
    virtual void Text( const VECTOR2I&        aPos,
                       const COLOR4D&         aColor,
                       const wxString&        aText,
                       const EDA_ANGLE&       aOrient,
                       const VECTOR2I&        aSize,
                       enum GR_TEXT_H_ALIGN_T aH_justify,
                       enum GR_TEXT_V_ALIGN_T aV_justify,
                       int                    aPenWidth,
                       bool                   aItalic,
                       bool                   aBold,
                       bool                   aMultilineAllowed,
                       KIFONT::FONT*          aFont,
                       const KIFONT::METRICS& aFontMetrics,
                       void*                  aData = nullptr );

    virtual void PlotText( const VECTOR2I&        aPos,
                           const COLOR4D&         aColor,
                           const wxString&        aText,
                           const TEXT_ATTRIBUTES& aAttributes,
                           KIFONT::FONT*          aFont = nullptr,
                           const KIFONT::METRICS& aFontMetrics = KIFONT::METRICS::Default(),
                           void*                  aData = nullptr );
    /**
     * Create a clickable hyperlink with a rectangular click area
     *
     * @param aBox is the rectangular click target
     * @param aDestinationURL is the target URL
     */
    virtual void HyperlinkBox( const BOX2I& aBox, const wxString& aDestinationURL )
    {
        // NOP for most plotters.
    }

    /**
     * Create a clickable hyperlink menu with a rectangular click area
     *
     * @param aBox is the rectangular click target
     * @param aDestURLs is the target URL
     */
    virtual void HyperlinkMenu( const BOX2I& aBox, const std::vector<wxString>& aDestURLs )
    {
        // NOP for most plotters.
    }

    /**
     * Create a bookmark to a symbol
     *
     * @param aBox is the rectangular click target
     * @param aSymbolReference is the symbol schematic ref
     */
    virtual void Bookmark( const BOX2I& aBox, const wxString& aName,
                           const wxString& aGroupName = wxEmptyString )
    {
        // NOP for most plotters.
    }

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
    void Marker( const VECTOR2I& position, int diametre, unsigned aShapeId );

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
     * Change the current text mode.
     *
     * See the PlotTextMode explanation at the beginning of the file.
     */
    virtual void SetTextMode( PLOT_TEXT_MODE mode )
    {
        // NOP for most plotters.
    }

    virtual void SetGerberCoordinatesFormat( int aResolution, bool aUseInches = false )
    {
        // NOP for most plotters. Only for Gerber plotter
    }

    /// Set the number of digits for mantissa in coordinates in mm for SVG plotter
    virtual void SetSvgCoordinatesFormat( unsigned aPrecision )
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

    /**
     * @return the plot offset in IUs, set by SetViewport() and used to plot items.
     */
    VECTOR2I GetPlotOffsetUserUnits() {return m_plotOffset; }


protected:
    /**
     * Generic fallback: arc rendered as a polyline.
     * Note also aCentre and aRadius are double to avoid creating rounding issues due
     * to the fact a arc is defined in Kicad by a start point, a end point and third point
     * not angles and radius.
     * In some plotters (i.e. dxf) whe need a good precision when calculating an arc
     * without error introduced by rounding, to avoid moving the end points,
     * usually important in outlines when plotting an arc given by center, radius and angles.
     * Winding direction: counter-clockwise in right-down coordinate system.
     */
    virtual void polyArc( const VECTOR2D& aCentre, const EDA_ANGLE& aStartAngle,
                          const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth );

    // These are marker subcomponents
    /**
     * Plot a circle centered on the position. Building block for markers
     */
    void markerCircle( const VECTOR2I& pos, int radius );

    /**
     * Plot a - bar centered on the position. Building block for markers
     */
    void markerHBar( const VECTOR2I& pos, int radius );

    /**
     * Plot a / bar centered on the position. Building block for markers
     */
    void markerSlash( const VECTOR2I& pos, int radius );

    /**
     * Plot a \ bar centered on the position. Building block for markers
     */
    void markerBackSlash( const VECTOR2I& pos, int radius );

    /**
     * Plot a | bar centered on the position. Building block for markers
     */
    void markerVBar( const VECTOR2I& pos, int radius );

    /**
     * Plot a square centered on the position. Building block for markers
     */
    void markerSquare( const VECTOR2I& position, int radius );

    /**
     * Plot a lozenge centered on the position. Building block for markers
     */
    void markerLozenge( const VECTOR2I& position, int radius );

    // Helper function for sketched filler segment


    // Coordinate and scaling conversion functions

    /**
     * Modify coordinates according to the orientation, scale factor, and offsets trace. Also
     * convert from a VECTOR2I to VECTOR2D, since some output engines needs floating point
     * coordinates.
     */
    virtual VECTOR2D userToDeviceCoordinates( const VECTOR2I& aCoordinate );

    /**
     * Modify size according to the plotter scale factors (VECTOR2I version, returns a VECTOR2D).
     */
    virtual VECTOR2D userToDeviceSize( const VECTOR2I& size );

    /**
     * Modify size according to the plotter scale factors (simple double version).
     */
    virtual double userToDeviceSize( double size ) const;

    double GetDotMarkLenIU( int aLineWidth ) const;

    double GetDashMarkLenIU( int aLineWidth ) const;

    double GetDashGapLenIU( int aLineWidth ) const;


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
    VECTOR2I         m_plotOffset;          // Plot offset (in IUs)
    bool             m_plotMirror;          // X axis orientation (SVG)
                                            // and plot mirrored (only for PS, PDF and SVG)
    bool             m_mirrorIsHorizontal;  // true to mirror horizontally (else vertically)
    bool             m_yaxisReversed;       // true if the Y axis is top to bottom (SVG)

    /// Output file
    FILE*            m_outputFile;

    // Pen handling
    bool             m_colorMode;           // true to plot in color, otherwise black & white
    bool             m_negativeMode;        // true to generate a negative image (PS mode mainly)
    int              m_currentPenWidth;
    char             m_penState;            // current pen state: 'U', 'D' or 'Z' (see PenTo)
    VECTOR2I         m_penLastpos;          // last pen position; -1,-1 when pen is at rest

    wxString         m_creator;
    wxString         m_filename;
    wxString         m_title;
    wxString         m_author;
    wxString         m_subject;
    PAGE_INFO        m_pageInfo;
    VECTOR2I         m_paperSize;           // Paper size in IU - not in mils

    wxArrayString    m_headerExtraLines;    // a set of string to print in header file

    RENDER_SETTINGS* m_renderSettings;

    const PROJECT*   m_project;
    std::vector<std::pair<PCB_LAYER_ID, wxString>> m_layersToExport;

    PCB_LAYER_ID m_layer;
};


class TITLE_BLOCK;

void PlotDrawingSheet( PLOTTER* plotter, const PROJECT* aProject, const TITLE_BLOCK& aTitleBlock,
                       const PAGE_INFO& aPageInfo, const std::map<wxString, wxString>*aProperties,
                       const wxString& aSheetNumber, int aSheetCount, const wxString& aSheetName,
                       const wxString& aSheetPath, const wxString& aFilename,
                       COLOR4D aColor = COLOR4D::UNSPECIFIED, bool aIsFirstPage = true );

/**
 * Return the default plot extension for a format.
 */
wxString GetDefaultPlotExtension( PLOT_FORMAT aFormat );


#endif  // PLOT_COMMON_H_
