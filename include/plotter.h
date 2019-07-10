/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Common plot library \n
 * Plot settings, and plotting engines (Postscript, Gerber, HPGL and DXF)
 *
 * @file plot_common.h
 */

#ifndef PLOT_COMMON_H_
#define PLOT_COMMON_H_

#include <vector>
#include <math/box2.h>
#include <gr_text.h>
#include <page_info.h>
#include <eda_text.h>       // FILL_T

class SHAPE_POLY_SET;
class SHAPE_LINE_CHAIN;
class GBR_NETLIST_METADATA;

/**
 * Enum PlotFormat
 * is the set of supported output plot formats.  They should be kept in order
 * of the radio buttons in the plot panel/windows.
 */
enum PlotFormat {
    PLOT_FORMAT_UNDEFINED = -1,
    PLOT_FIRST_FORMAT = 0,
    PLOT_FORMAT_HPGL = PLOT_FIRST_FORMAT,
    PLOT_FORMAT_GERBER,
    PLOT_FORMAT_POST,
    PLOT_FORMAT_DXF,
    PLOT_FORMAT_PDF,
    PLOT_FORMAT_SVG,
    PLOT_LAST_FORMAT = PLOT_FORMAT_SVG
};

/**
 * Enum for choosing which kind of text to output with the PSLIKE
 * plotters. You can:
 * 1) only use the internal vector font
 * 2) only use native postscript fonts
 * 3) use the internal vector font and add 'phantom' text to aid
 *    searching
 * 4) keep the default for the plot driver
 *
 * This is recognized by the DXF driver too, where NATIVE emits
 * TEXT entities instead of stroking the text
 */
enum PlotTextMode {
    PLOTTEXTMODE_STROKE,
    PLOTTEXTMODE_NATIVE,
    PLOTTEXTMODE_PHANTOM,
    PLOTTEXTMODE_DEFAULT
};

/**
 * Enum for choosing dashed line type
 */
enum PlotDashType {
    PLOTDASHTYPE_SOLID,
    PLOTDASHTYPE_DASH,
    PLOTDASHTYPE_DOT,
    PLOTDASHTYPE_DASHDOT,
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
    virtual PlotFormat GetPlotterType() const = 0;

    virtual bool StartPlot() = 0;
    virtual bool EndPlot() = 0;

    virtual void SetNegative( bool aNegative )
    {
        negativeMode = aNegative;
    }

    /** Plot in B/W or color.
     * @param aColorMode = true to plot in color, false to plot in black and white
     */
    virtual void SetColorMode( bool aColorMode )
    {
        colorMode = aColorMode;
    }

    bool GetColorMode() const
    {
        return colorMode;
    }

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings );

    /**
     * Set the line width for the next drawing.
     * @param width is specified in IUs
     * @param aData is an auxiliary parameter, mainly used in gerber plotter
     */
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) = 0;

    /**
     * Set the default line width. Used at the beginning and when a width
     * of -1 (USE_DEFAULT_LINE_WIDTH) is requested.
     * @param width is specified in IUs
     */
    virtual void SetDefaultLineWidth( int width ) = 0;

    virtual int GetCurrentLineWidth() const
    {
        return currentPenWidth;
    }

    virtual void SetColor( COLOR4D color ) = 0;

    virtual void SetDash( int dashed ) = 0;

    virtual void SetCreator( const wxString& aCreator )
    {
        creator = aCreator;
    }

    virtual void SetTitle( const wxString& aTitle )
    {
        title = aTitle;
    }

    /**
     * Function AddLineToHeader
     * Add a line to the list of free lines to print at the beginning of the file
     * @param aExtraString is the string to print
     */
    void AddLineToHeader( const wxString& aExtraString )
    {
        m_headerExtraLines.Add( aExtraString );
    }

    /**
     * Function ClearHeaderLinesList
     * remove all lines from the list of free lines to print at the beginning of the file
     */
    void ClearHeaderLinesList()
    {
        m_headerExtraLines.Clear();
    }

    /**
     * Set the plot offset and scaling for the current plot
     * @param aOffset is the plot offset
     * @param aIusPerDecimil gives the scaling factor from IUs to device units
     * @param aScale is the user set plot scaling factor (either explicitly
     * 		or using 'fit to A4')
     * @param aMirror flips the plot in the Y direction (useful for toner
     * 		transfers or some kind of film)
     */
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
        double aScale, bool aMirror ) = 0;

    /**
     * Open or create the plot file aFullFilename
     * @param aFullFilename = the full file name of the file to create
     * @return true if success, false if the file cannot be created/opened
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
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) = 0;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) = 0;

    /**
     * Generic fallback: arc rendered as a polyline
     */
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH );

    /**
     * moveto/lineto primitive, moves the 'pen' to the specified direction
     * @param pos is the target position
     * @param plume specifies the kind of motion: 'U' only moves the pen,
     * 		'D' draw a line from the current position and 'Z' finish
     *		the drawing and returns the 'pen' to rest (flushes the trace)
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
     * Function PlotPoly
     * @brief Draw a polygon ( filled or not )
     * @param aCornerList = corners list (a std::vector< wxPoint >)
     * @param aFill = type of fill
     * @param aWidth = line width
     * @param aData an auxiliary info (mainly for gerber format)
     */
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList, FILL_T aFill,
               int aWidth = USE_DEFAULT_LINE_WIDTH, void * aData = NULL ) = 0;

    /**
     * Function PlotPoly
     * @brief Draw a polygon ( filled or not )
     * @param aCornerList = corners list (a SHAPE_LINE_CHAIN)
     * @param aFill = type of fill
     * @param aWidth = line width
     * @param aData an auxiliary info (mainly for gerber format)
     */
    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aCornerList, FILL_T aFill,
               int aWidth = USE_DEFAULT_LINE_WIDTH, void * aData = NULL );

    /**
     * Function PlotImage
     * Only Postscript plotters can plot bitmaps
     * for plotters that cannot plot a bitmap, a rectangle is plotted
     * @brief Draw an image bitmap
     * @param aImage = the bitmap
     * @param aPos = position of the center of the bitmap
     * @param aScaleFactor = the scale factor to apply to the bitmap size
     *                      (this is not the plot scale factor)
     */
    virtual void PlotImage( const wxImage & aImage, const wxPoint& aPos,
                            double aScaleFactor );

    // Higher level primitives -- can be drawn as line, sketch or 'filled'
    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               EDA_DRAW_MODE_T tracemode, void* aData );
    virtual void ThickArc( const wxPoint& centre, double StAngle, double EndAngle,
                           int rayon, int width, EDA_DRAW_MODE_T tracemode, void* aData );
    virtual void ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                            EDA_DRAW_MODE_T tracemode, void* aData );
    virtual void ThickCircle( const wxPoint& pos, int diametre, int width,
                              EDA_DRAW_MODE_T tracemode, void* aData );

    // Flash primitives

    /**
     * virtual function FlashPadCircle
     * @param aPadPos Position of the shape (center of the rectangle
     * @param aDiameter diameter of round pad
     * @param aTraceMode FILLED or SKETCH
     * @param aData an auxiliary info (mainly for gerber format attributes)
     */
    virtual void FlashPadCircle( const wxPoint& aPadPos, int aDiameter,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) = 0;

    /**
     * virtual function FlashPadOval
     * @param aPadPos Position of the shape (center of the rectangle
     * @param aSize = size of oblong shape
     * @param aPadOrient The rotation of the shape
     * @param aTraceMode FILLED or SKETCH
     * @param aData an auxiliary info (mainly for gerber format attributes)
     */
    virtual void FlashPadOval( const wxPoint& aPadPos, const wxSize& aSize, double aPadOrient,
                               EDA_DRAW_MODE_T aTraceMode, void* aData ) = 0;

    /**
     * virtual function FlashPadRect
     * @param aPadPos Position of the shape (center of the rectangle
     * @param aSize = size of rounded rect
     * @param aPadOrient The rotation of the shape
     * @param aTraceMode FILLED or SKETCH
     * @param aData an auxuliary info (mainly for gerber format attributes)
     */
    virtual void FlashPadRect( const wxPoint& aPadPos, const wxSize& aSize,
                               double aPadOrient, EDA_DRAW_MODE_T aTraceMode, void* aData ) = 0;

    /**
     * virtual function FlashPadRoundRect
     * @param aPadPos Position of the shape (center of the rectangle
     * @param aSize = size of rounded rect
     * @param aCornerRadius Radius of the rounded corners
     * @param aOrient The rotation of the shape
     * @param aTraceMode FILLED or SKETCH
     * @param aData an auxiliary info (mainly for gerber format attributes)
     */
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    EDA_DRAW_MODE_T aTraceMode, void* aData ) = 0;

    /**
     * virtual function FlashPadCustom
     * @param aPadPos Position of the shape (center of the rectangle
     * @param aSize = size of round reference pad
     * @param aPolygons the shape as polygon set
     * @param aTraceMode FILLED or SKETCH
     * @param aData an auxiliary info (mainly for gerber format attributes)
     */
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 SHAPE_POLY_SET* aPolygons,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) = 0;

    /** virtual function FlashPadTrapez
     * flash a trapezoidal pad
     * @param aPadPos = the position of the shape
     * @param aCorners = the list of 4 corners positions,
     * 		relative to the shape position, pad orientation 0
     * @param aPadOrient = the rotation of the shape
     * @param aTraceMode = FILLED or SKETCH
     * @param aData an auxiliary info (mainly for gerber format attributes)
     */
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, EDA_DRAW_MODE_T aTraceMode,
                                 void* aData ) = 0;


    /**
     * Draws text with the plotter. For convenience it accept the color to use
     * for specific plotters (GERBER) aData is used to pass extra parameters
     */
    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D               aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = NULL );

    /**
     * Draw a marker (used for the drill map)
     */
    static const unsigned MARKER_COUNT = 58;

    /**
     * Draw a pattern shape number aShapeId, to coord position.
     * Diameter diameter = (coord table) hole
     * AShapeId = index (used to generate forms characters)
     */
    void Marker( const wxPoint& position, int diametre, unsigned aShapeId );

    /**
     * Function SetLayerPolarity
     * sets current Gerber layer polarity to positive or negative
     * by writing \%LPD*\% or \%LPC*\% to the Gerber file, respectively.
     * (obviously starts a new Gerber layer, too)
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
     * explanation at the beginning of the file
     */
    virtual void SetTextMode( PlotTextMode mode )
    {
        // NOP for most plotters.
    }

    virtual void SetGerberCoordinatesFormat( int aResolution, bool aUseInches = false )
    {
        // NOP for most plotters. Only for Gerber plotter
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
     * Cdonvert a thick segment and plot it as an oval
     */
    void segmentAsOval( const wxPoint& start, const wxPoint& end, int width,
                        EDA_DRAW_MODE_T tracemode );

    void sketchOval( const wxPoint& pos, const wxSize& size, double orient, int width );

    // Coordinate and scaling conversion functions

    /**
     * Modifies coordinates according to the orientation,
     * scale factor, and offsets trace. Also convert from a wxPoint to DPOINT,
     * since some output engines needs floating point coordinates.
     */
    virtual DPOINT userToDeviceCoordinates( const wxPoint& aCoordinate );

    /**
     * Modifies size according to the plotter scale factors
     * (wxSize version, returns a DPOINT)
     */
    virtual DPOINT userToDeviceSize( const wxSize& size );

    /**
     * Modifies size according to the plotter scale factors
     * (simple double version)
     */
    virtual double userToDeviceSize( double size ) const;

    double GetDotMarkLenIU() const;

    double GetDashMarkLenIU() const;

    double GetDashGapLenIU() const;

protected:      // variables used in most of plotters:
    /// Plot scale - chosen by the user (even implicitly with 'fit in a4')
    double        plotScale;

    /* Caller scale (how many IUs in a decimil - always); it's a double
     * because in eeschema there are 0.1 IUs in a decimil (eeschema
     * always works in mils internally) while pcbnew can work in decimil
     * or nanometers, so this value would be >= 1 */
    double        m_IUsPerDecimil;

    /// Device scale (from IUs to plotter device units - usually decimils)
    double        iuPerDeviceUnit;

    /// Plot offset (in IUs)
    wxPoint       plotOffset;

    /// X axis orientation (SVG)
    /// and plot mirrored (only for PS, PDF HPGL and SVG)
    bool          m_plotMirror;
    bool          m_mirrorIsHorizontal;     /// true to mirror horizontally (else vertically)
    bool          m_yaxisReversed;          /// true if the Y axis is top to bottom (SVG)

    /// Output file
    FILE*         outputFile;

    // Pen handling
    bool          colorMode;        /// true to plot in color, false to plot in black and white
    bool          negativeMode;     /// true to generate a negative image (PS mode mainly)
    int           defaultPenWidth;
    int           currentPenWidth;
    /// Current pen state: 'U', 'D' or 'Z' (see PenTo)
    char          penState;
    /// Last pen positions; set to -1,-1 when the pen is at rest
    wxPoint       penLastpos;
    wxString      creator;
    wxString      filename;
    wxString      title;
    PAGE_INFO     pageInfo;
    /// Paper size in IU - not in mils
    wxSize        paperSize;

    wxArrayString m_headerExtraLines;  /// a set of string to print in header file
};


class HPGL_PLOTTER : public PLOTTER
{
public:
    HPGL_PLOTTER();

    virtual PlotFormat GetPlotterType() const override
    {
        return PLOT_FORMAT_HPGL;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "plt" ) );
    }

    virtual bool StartPlot() override;
    virtual bool EndPlot() override;

    /// HPGL doesn't handle line thickness or color
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) override
    {
        // This is the truth
        currentPenWidth = userToDeviceSize( penDiameter );
    }

    virtual void SetDefaultLineWidth( int width ) override {}
    virtual void SetDash( int dashed ) override;

    virtual void SetColor( COLOR4D color ) override {}

    virtual void SetPenSpeed( int speed )
    {
        penSpeed = speed;
    }

    virtual void SetPenNumber( int number )
    {
        penNumber = number;
    }

    virtual void SetPenDiameter( double diameter );

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
               int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH,
                           void * aData = NULL) override;

    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               EDA_DRAW_MODE_T tracemode, void* aData ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PenTo( const wxPoint& pos, char plume ) override;
    virtual void FlashPadCircle( const wxPoint& aPadPos, int aDiameter,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadOval( const wxPoint& aPadPos, const wxSize& aSize, double aPadOrient,
                               EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadRect( const wxPoint& aPadPos, const wxSize& aSize,
                               double aOrient, EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 SHAPE_POLY_SET* aPolygons,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, EDA_DRAW_MODE_T aTraceMode,
                                 void* aData ) override;

protected:
    void penControl( char plume );

    int    penSpeed;
    int    penNumber;
    double penDiameter;
};

/**
 * The PSLIKE_PLOTTER class is an intermediate class to handle common
 * routines for engines working more or less with the postscript imaging
 * model
 */
class PSLIKE_PLOTTER : public PLOTTER
{
public:
    PSLIKE_PLOTTER() : plotScaleAdjX( 1 ), plotScaleAdjY( 1 ),
                       m_textMode( PLOTTEXTMODE_PHANTOM )
    {
    }

    /**
     * PS and PDF fully implement native text (for the Latin-1 subset)
     */
    virtual void SetTextMode( PlotTextMode mode ) override
    {
        if( mode != PLOTTEXTMODE_DEFAULT )
            m_textMode = mode;
    }

    virtual void SetDefaultLineWidth( int width ) override;

    /**
     * Set the 'fine' scaling for the postscript engine
     */
    void SetScaleAdjust( double scaleX, double scaleY )
    {
        plotScaleAdjX = scaleX;
        plotScaleAdjY = scaleY;
    }

    // Pad routines are handled with lower level primitives
    virtual void FlashPadCircle( const wxPoint& aPadPos, int aDiameter,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadOval( const wxPoint& aPadPos, const wxSize& aSize, double aPadOrient,
                               EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadRect( const wxPoint& aPadPos, const wxSize& aSize,
                               double aPadOrient, EDA_DRAW_MODE_T aTraceMode,
                               void* aData ) override;
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 SHAPE_POLY_SET* aPolygons,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, EDA_DRAW_MODE_T aTraceMode, void* aData ) override;

    /** The SetColor implementation is split with the subclasses:
     * The PSLIKE computes the rgb values, the subclass emits the
     * operator to actually do it
     */
    virtual void SetColor( COLOR4D color ) override;

protected:
    void computeTextParameters( const wxPoint&           aPos,
                                const wxString&          aText,
                                int                      aOrient,
                                const wxSize&            aSize,
                                bool                     aMirror,
                                enum EDA_TEXT_HJUSTIFY_T aH_justify,
                                enum EDA_TEXT_VJUSTIFY_T aV_justify,
                                int                      aWidth,
                                bool                     aItalic,
                                bool                     aBold,
                                double                   *wideningFactor,
                                double                   *ctm_a,
                                double                   *ctm_b,
                                double                   *ctm_c,
                                double                   *ctm_d,
                                double                   *ctm_e,
                                double                   *ctm_f,
                                double                   *heightFactor );
    void postscriptOverlinePositions( const wxString& aText, int aXSize,
                                      bool aItalic, bool aBold,
                                      std::vector<int> *pos_pairs );
    void fputsPostscriptString(FILE *fout, const wxString& txt);

    /// Virtual primitive for emitting the setrgbcolor operator
    virtual void emitSetRGBColor( double r, double g, double b ) = 0;

    /// Height of the postscript font (from the AFM)
    static const double postscriptTextAscent; // = 0.718;

    int returnPostscriptTextWidth( const wxString& aText, int aXSize,
                                   bool aItalic, bool aBold );

    /// Fine user scale adjust ( = 1.0 if no correction)
    double plotScaleAdjX, plotScaleAdjY;

    /// How to draw text
    PlotTextMode m_textMode;
};


class PS_PLOTTER : public PSLIKE_PLOTTER
{
public:
    PS_PLOTTER()
    {
        // The phantom plot in postscript is an hack and reportedly
        // crashes Adobe's own postscript interpreter!
        m_textMode = PLOTTEXTMODE_STROKE;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "ps" ) );
    }

    virtual PlotFormat GetPlotterType() const override
    {
        return PLOT_FORMAT_POST;
    }

    virtual bool StartPlot() override;
    virtual bool EndPlot() override;
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) override;
    virtual void SetDash( int dashed ) override;

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH,
                           void * aData = NULL ) override;

    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;
    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D               aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = NULL ) override;
protected:
    virtual void emitSetRGBColor( double r, double g, double b ) override;
};

class PDF_PLOTTER : public PSLIKE_PLOTTER
{
public:
    PDF_PLOTTER() : pageStreamHandle( 0 ), workFile( NULL )
    {
        // Avoid non initialized variables:
        pageStreamHandle = streamLengthHandle = fontResDictHandle = 0;
        pageTreeHandle = 0;
    }

    virtual PlotFormat GetPlotterType() const override
    {
        return PLOT_FORMAT_PDF;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "pdf" ) );
    }

    /**
     * Open or create the plot file aFullFilename
     * @param aFullFilename = the full file name of the file to create
     * @return true if success, false if the file cannot be created/opened
     *
     * The base class open the file in text mode, so we should have this
     * function overlaid for PDF files, which are binary files
     */
    virtual bool OpenFile( const wxString& aFullFilename ) override;

    virtual bool StartPlot() override;
    virtual bool EndPlot() override;
    virtual void StartPage();
    virtual void ClosePage();
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) override;
    virtual void SetDash( int dashed ) override;

    /** PDF can have multiple pages, so SetPageSettings can be called
     * with the outputFile open (but not inside a page stream!) */
    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) override;
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH,
                           void * aData = NULL ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;

    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D               aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = NULL ) override;

    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor ) override;


protected:
    virtual void emitSetRGBColor( double r, double g, double b ) override;
    int allocPdfObject();
    int startPdfObject(int handle = -1);
    void closePdfObject();
    int startPdfStream(int handle = -1);
    void closePdfStream();
    int pageTreeHandle;		 /// Handle to the root of the page tree object
    int fontResDictHandle;	 /// Font resource dictionary
    std::vector<int> pageHandles;/// Handles to the page objects
    int pageStreamHandle;	 /// Handle of the page content object
    int streamLengthHandle;      /// Handle to the deferred stream length
    wxString workFilename;
    FILE* workFile;  	         /// Temporary file to costruct the stream before zipping
    std::vector<long> xrefTable; /// The PDF xref offset table
};

class SVG_PLOTTER : public PSLIKE_PLOTTER
{
public:
    SVG_PLOTTER();

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "svg" ) );
    }

    virtual PlotFormat GetPlotterType() const override
    {
        return PLOT_FORMAT_SVG;
    }

    virtual void SetColor( COLOR4D color ) override;
    virtual bool StartPlot() override;
    virtual bool EndPlot() override;
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) override;
    virtual void SetDash( int dashed ) override;

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH,
                           void * aData = NULL ) override;

    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;


    /**
     * calling this function allows one to define the beginning of a group
     * of drawing items (used in SVG format to separate components)
     * @param aData should be a string for the SVG ID tage
     */
    virtual void StartBlock( void* aData ) override;

    /**
     * calling this function allows one to define the end of a group of drawing
     * items the group is started by StartBlock()
     * @param aData should be null
     */
    virtual void EndBlock( void* aData ) override;

    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D               aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = NULL ) override;

protected:
    FILL_T m_fillMode;              // true if the current contour
                                    // rect, arc, circle, polygon must be filled
    long m_pen_rgb_color;           // current rgb color value: each color has
                                    // a value 0 ... 255, and the 3 colors are
                                    // grouped in a 3x8 bits value
                                    // (written in hex to svg files)
    long m_brush_rgb_color;         // same as m_pen_rgb_color, used to fill
                                    // some contours.
    bool m_graphics_changed;        // true if a pen/brush parameter is modified
                                    // color, pen size, fil mode ...
                                    // the new SVG stype must be output on file
    int m_dashed;                   // 0 = plot solid line style
                                    // 1 = plot dashed line style
                                    // 2 = plot dotted line style
                                    // 3 = plot dash-dot line style

    /**
     * function emitSetRGBColor()
     * initialize m_pen_rgb_color from reduced values r, g ,b
     * ( reduced values are 0.0 to 1.0 )
     */
    virtual void emitSetRGBColor( double r, double g, double b ) override;

    /**
     * function setSVGPlotStyle()
     * output the string which define pen and brush color, shape, transparency
     *
     * @param aIsGroup If false, do not form a new group for the style.
     * @param aExtraStyle If given, the string will be added into the style string before closing
     */
    void setSVGPlotStyle( bool aIsGroup = true, const std::string& aExtraStyle = {} );

    /**
     * function setFillMode()
     * prepare parameters for setSVGPlotStyle()
     */
    void setFillMode( FILL_T fill );
};

/* Class to handle a D_CODE when plotting a board : */
#define FIRST_DCODE_VALUE 10    // D_CODE < 10 is a command, D_CODE >= 10 is a tool

class APERTURE
{
public:
    enum APERTURE_TYPE {
        Circle   = 1,
        Rect     = 2,
        Plotting = 3,
        Oval     = 4
    };

    wxSize        m_Size;     // horiz and Vert size
    APERTURE_TYPE m_Type;     // Type ( Line, rect , circulaire , ovale .. )
    int           m_DCode;    // code number ( >= 10 );
    int           m_ApertureAttribute;  // the attribute attached to this aperture
                                        // Only one attribute is allowed by aperture
                                        // 0 = no specific aperture attribute
};


class GERBER_PLOTTER : public PLOTTER
{
public:
    GERBER_PLOTTER();

    virtual PlotFormat GetPlotterType() const override
    {
        return PLOT_FORMAT_GERBER;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "gbr" ) );
    }

    /**
     * Function StartPlot
     * Write GERBER header to file
     * initialize global variable g_Plot_PlotOutputFile
     */
    virtual bool StartPlot() override;
    virtual bool EndPlot() override;
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) override;
    virtual void SetDefaultLineWidth( int width ) override;

    // RS274X has no dashing, nor colours
    virtual void SetDash( int dashed ) override {}
    virtual void SetColor( COLOR4D color ) override {}
    // Currently, aScale and aMirror are not used in gerber plotter
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                          double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Arc( const wxPoint& aCenter, double aStAngle, double aEndAngle,
                      int aRadius, FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               EDA_DRAW_MODE_T tracemode, void* aData ) override;

    virtual void ThickArc( const wxPoint& centre, double StAngle, double EndAngle,
                           int rayon, int width, EDA_DRAW_MODE_T tracemode, void* aData ) override;
    virtual void ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                            EDA_DRAW_MODE_T tracemode, void* aData ) override;
    virtual void ThickCircle( const wxPoint& pos, int diametre, int width,
                              EDA_DRAW_MODE_T tracemode, void* aData ) override;
    /**
     * Gerber polygon: they can (and *should*) be filled with the
     * appropriate G36/G37 sequence
     */
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH,
                           void * aData = NULL ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;

    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D               aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = NULL ) override;

    /**
     * Filled circular flashes are stored as apertures
     */
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 EDA_DRAW_MODE_T trace_mode, void* aData ) override;

    /**
     * Filled oval flashes are handled as aperture in the 90 degree positions only
     */
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, double orient,
                               EDA_DRAW_MODE_T trace_mode, void* aData ) override;
    /**
     * Filled rect flashes are handled as aperture in the 0 90 180 or 270 degree orientation only
     * and as polygon for other orientations
     * TODO: always use flashed shapes (aperture macros)
     */
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               double orient, EDA_DRAW_MODE_T trace_mode, void* aData ) override;

    /**
     * Roundrect pad at the moment are not handled as aperture, since
     * they require aperture macros
     * TODO: always use flashed shapes (aperture macros)
     */
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 SHAPE_POLY_SET* aPolygons,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    /**
     * Trapezoidal pad at the moment are *never* handled as aperture, since
     * they require aperture macros
     * TODO: always use flashed shapes (aperture macros)
     */
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, EDA_DRAW_MODE_T aTraceMode, void* aData ) override;

    /**
     * Change the plot polarity and begin a new layer
     * Used to 'scratch off' silk screen away from solder mask
     */
    virtual void SetLayerPolarity( bool aPositive ) override;

    /**
     * Function SetGerberCoordinatesFormat
     * selection of Gerber units and resolution (number of digits in mantissa)
     * @param aResolution = number of digits in mantissa of coordinate
     *                      use 5 or 6 for mm and 6 or 7 for inches
     *                      do not use value > 6 (mm) or > 7 (in) to avoid overflow
     * @param aUseInches = true to use inches, false to use mm (default)
     *
     * Should be called only after SetViewport() is called
     */
    virtual void SetGerberCoordinatesFormat( int aResolution, bool aUseInches = false ) override;

    void UseX2format( bool aEnable ) { m_useX2format = aEnable; }
    void UseX2NetAttributes( bool aEnable ) { m_useNetAttributes = aEnable; }

    /**
     * calling this function allows one to define the beginning of a group
     * of drawing items (used in X2 format with netlist attributes)
     * @param aData can define any parameter
     */
    virtual void StartBlock( void* aData ) override;

    /**
     * calling this function allows one to define the end of a group of drawing
     * items the group is started by StartBlock()
     * (used in X2 format with netlist attributes)
     * @param aData can define any parameter
     */
    virtual void EndBlock( void* aData ) override;

protected:
    /**
     * Pick an existing aperture or create a new one, matching the
     * size, type and attributes.
     * write the DCode selection on gerber file
     */
    void selectAperture( const wxSize& aSize, APERTURE::APERTURE_TYPE aType,
                         int aApertureAttribute );

    /**
     * Emit a D-Code record, using proper conversions
     * to format a leading zero omitted gerber coordinate
     * (for n decimal positions, see header generation in start_plot
     */
    void emitDcode( const DPOINT& pt, int dcode );

    /**
     * print a Gerber net attribute object record.
     * In a gerber file, a net attribute is owned by a graphic object
     * formatNetAttribute must be called before creating the object
     * @param aData contains the dato to format.
     * the generated string depends on the type of netlist info
     */
    void formatNetAttribute( GBR_NETLIST_METADATA* aData );

    /**
     * clear a Gerber net attribute record (clear object attribute dictionnary)
     * and output the clear object attribute dictionnary command to gerber file
     */
    void clearNetAttribute();

    /**
     * Function getAperture returns a reference to the aperture which meets the size anf type of tool
     * if the aperture does not exist, it is created and entered in aperture list
     * @param aSize = the size of tool
     * @param aType = the type ( shape ) of tool
     * @param aApertureAttribute = an aperture attribute of the tool (a tool can have onlu one attribute)
     * 0 = no specific attribute
     */
    std::vector<APERTURE>::iterator getAperture( const wxSize& aSize,
                    APERTURE::APERTURE_TYPE aType, int aApertureAttribute );

    // the attributes dictionnary created/modifed by %TO, attached the objects, when they are created
    // by D01, D03 G36/G37 commands
    // standard attributes are .P, .C and .N
    // this is used by gerber readers when creating a new object. Cleared by %TD command
    // Note: m_objectAttributesDictionnary can store more than one attribute
    // the string stores the line(s) actually written to the gerber file
    // it can store a .P, .C or .N attribute, or 2 or 3 attributes, separated by a \n char (EOL)
    std::string   m_objectAttributesDictionnary;

    // The last aperture attribute generated (only one aperture attribute can be set)
    int           m_apertureAttribute;

    FILE* workFile;
    FILE* finalFile;
    wxString m_workFilename;

    /**
     * Generate the table of D codes
     */
    void writeApertureList();

    std::vector<APERTURE>           apertures;
    std::vector<APERTURE>::iterator currentAperture;

    bool     m_gerberUnitInch;  // true if the gerber units are inches, false for mm
    int      m_gerberUnitFmt;   // number of digits in mantissa.
                                // usually 6 in Inches and 5 or 6  in mm
    bool    m_useX2format;      // In recent gerber files, attributes are added.
                                // Attributes in file header will be added using X2 format if true
                                // If false (X1 format), these attributes will be added as comments.
    bool    m_useNetAttributes; // In recent gerber files, netlist info can be added.
                                // It will be added if this param is true, using X2 or X1 format
};


class DXF_PLOTTER : public PLOTTER
{
public:
    DXF_PLOTTER() : textAsLines( false )
    {
        textAsLines = true;
        m_currentColor = COLOR4D::BLACK;
        m_currentLineType = 0;
        SetUnits( DXF_PLOTTER::DXF_UNIT_INCHES );
    }

    virtual PlotFormat GetPlotterType() const override
    {
        return PLOT_FORMAT_DXF;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "dxf" ) );
    }

    /**
     * DXF handles NATIVE text emitting TEXT entities
     */
    virtual void SetTextMode( PlotTextMode mode ) override
    {
        if( mode != PLOTTEXTMODE_DEFAULT )
            textAsLines = ( mode != PLOTTEXTMODE_NATIVE );
    }

    virtual bool StartPlot() override;
    virtual bool EndPlot() override;

    // For now we don't use 'thick' primitives, so no line width
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) override
    {
        currentPenWidth = 0;
    }

    virtual void SetDefaultLineWidth( int width ) override
    {
        // DXF lines are infinitesimal
        defaultPenWidth = 0;
    }

    virtual void SetDash( int dashed ) override;

    virtual void SetColor( COLOR4D color ) override;

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH, void * aData = NULL ) override;
    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               EDA_DRAW_MODE_T tracemode, void* aData ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PenTo( const wxPoint& pos, char plume ) override;

    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 EDA_DRAW_MODE_T trace_mode, void* aData ) override;
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, double orient,
                               EDA_DRAW_MODE_T trace_mode, void* aData ) override;
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               double orient, EDA_DRAW_MODE_T trace_mode, void* aData ) override;
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 SHAPE_POLY_SET* aPolygons,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, EDA_DRAW_MODE_T aTraceMode, void* aData ) override;

    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D               aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = NULL ) override;


    // Must be in the same order as the drop-down list in the plot dialog inside pcbnew
    enum DXF_UNITS
    {
        DXF_UNIT_INCHES = 0,
        DXF_UNIT_MILLIMETERS = 1
    };

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
    bool textAsLines;
    COLOR4D m_currentColor;
    int m_currentLineType;

    DXF_UNITS    m_plotUnits;
    double       m_unitScalingFactor;
    unsigned int m_measurementDirective;
};

class TITLE_BLOCK;
void PlotWorkSheet( PLOTTER* plotter, const TITLE_BLOCK& aTitleBlock,
                    const PAGE_INFO& aPageInfo,
                    int aSheetNumber, int aNumberOfSheets,
                    const wxString &aSheetDesc,
                    const wxString &aFilename,
                    const COLOR4D aColor = COLOR4D::UNSPECIFIED );

/** Returns the default plot extension for a format
  */
wxString GetDefaultPlotExtension( PlotFormat aFormat );


#endif  // PLOT_COMMON_H_
