/**
 * Common plot library \n
 * Plot settings, and plotting engines (Postscript, Gerber, HPGL and DXF)
 *
 * @file plot_common.h
 */

#ifndef PLOT_COMMON_H_
#define PLOT_COMMON_H_

#include <vector>
#include <vector2d.h>
#include <drawtxt.h>
#include <common.h>         // PAGE_INFO
#include <eda_text.h>       // FILL_T

/**
 * Enum PlotFormat
 * is the set of supported output plot formats.  They should be kept in order
 * of the radio buttons in the plot panel/windows.
 */
enum PlotFormat {
    PLOT_FORMAT_HPGL,
    PLOT_FORMAT_GERBER,
    PLOT_FORMAT_POST,
    PLOT_FORMAT_DXF,
    PLOT_FORMAT_PDF
};

/**
 * Enum for choosing which kind of text to output with the PSLIKE
 * plotters. You can:
 * 1) only use the internal vector font
 * 2) only use native postscript fonts
 * 3) use the internal vector font and add 'phantom' text to aid
 *    searching
 */
enum PostscriptTextMode {
    PSTEXTMODE_STROKE,
    PSTEXTMODE_NATIVE,
    PSTEXTMODE_PHANTOM
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
    PLOTTER( );

    virtual ~PLOTTER()
    {
        // Emergency cleanup
        if( outputFile )
        {
            fclose( outputFile );
        }
    }

    /**
     * Returns the effective plot engine in use. It's not very OO but for
     * now is required since some things are only done with some output devices
     * (like drill marks, emitted only for postscript
     */
    virtual PlotFormat GetPlotterType() const = 0;

    virtual bool StartPlot( FILE* fout ) = 0;
    virtual bool EndPlot() = 0;

    virtual void SetNegative( bool _negative )
    {
        negativeMode = _negative;
    }

    virtual void SetColorMode( bool _color_mode )
    {
        colorMode = _color_mode;
    }

    bool GetColorMode() const
    {
        return colorMode;
    }

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings );

    /**
     * Set the line width for the next drawing.
     * @param width is specified in IUs
     */
    virtual void SetCurrentLineWidth( int width ) = 0;

    /**
     * Set the default line width. Used at the beginning and when a width
     * of -1 is requested.
     * @param width is specified in IUs
     */
    virtual void SetDefaultLineWidth( int width ) = 0;

    virtual int GetCurrentLineWidth() const
    {
        return currentPenWidth;
    }

    virtual void SetColor( EDA_COLOR_T color ) = 0;

    virtual void SetDash( bool dashed ) = 0;

    /** PLEASE NOTE: the plot width adjustment is actually done by the
     * pcbnew routines, the plotter class only carry it along!
     * XXX In fact it's only used during postscript plot, I'd move this
     * variable as a static in pcbnew/plot_rtn.cpp. Also: why it's double?
     * it's added to pad/track size and it's specified in IU, so it should
     * be an int */
    virtual void SetPlotWidthAdj( double width )
    {
    }

    virtual double GetPlotWidthAdj()
    {
        return 0.;
    }

    virtual void SetCreator( const wxString& _creator )
    {
        creator = _creator;
    }

    virtual void SetFilename( const wxString& _filename )
    {
        filename = _filename;
    }

    /**
     * Set the plot offset and scaling for the current plot
     * @param aIusPerDecimil gives the scaling factor from IUs to device
     *		units
     * @param aScale is the user set plot scaling factor (either explicitly
     * 		or using 'fit to A4')
     * @param aMirror flips the plot in the Y direction (useful for toner
     * 		transfers or some kind of film)
     */
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
	    double aScale, bool aMirror ) = 0;

    // Low level primitives
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = -1 ) = 0;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = -1 ) = 0;
    virtual void Arc( const wxPoint& centre, int StAngle, int EndAngle, int rayon,
                      FILL_T fill, int width = -1 );

    /**
     * moveto/lineto primitive, moves the 'pen' to the specified direction
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
     * @param aCornerList = corners list
     * @param aFill = type of fill
     * @param aWidth = line width
     */
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList, FILL_T aFill,
			   int aWidth = -1 ) = 0;

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
                               EDA_DRAW_MODE_T tracemode );
    virtual void ThickArc( const wxPoint& centre, int StAngle, int EndAngle, int rayon,
                           int width, EDA_DRAW_MODE_T tracemode );
    virtual void ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                            EDA_DRAW_MODE_T tracemode );
    virtual void ThickCircle( const wxPoint& pos, int diametre, int width,
                              EDA_DRAW_MODE_T tracemode );

    // Flash primitives
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 EDA_DRAW_MODE_T trace_mode ) = 0;
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, int orient,
                               EDA_DRAW_MODE_T trace_mode ) = 0;
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               int orient, EDA_DRAW_MODE_T trace_mode ) = 0;

    /** virtual function FlashPadTrapez
     * flash a trapezoidal pad
     * @param aPadPos = the position of the shape
     * @param aCorners = the list of 4 corners positions,
     * 		relative to the shape position, pad orientation 0
     * @param aPadOrient = the rotation of the shape
     * @param aTrace_Mode = FILLED or SKETCH
     */
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 int aPadOrient, EDA_DRAW_MODE_T aTrace_Mode ) = 0;


    /**
     * Draws text with the plotter. For convenience it accept the color to use
     */
    virtual void Text( const wxPoint&              aPos,
                       enum EDA_COLOR_T            aColor,
                       const wxString&             aText,
                       int                         aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold );

    /**
     * Draw a marker (used for the drill map)
     */
    static const unsigned MARKER_COUNT = 58;
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

protected:
    // These are marker subcomponents
    void markerCircle( const wxPoint& pos, int radius );
    void markerHBar( const wxPoint& pos, int radius );
    void markerSlash( const wxPoint& pos, int radius );
    void markerBackSlash( const wxPoint& pos, int radius );
    void markerVBar( const wxPoint& pos, int radius );
    void markerSquare( const wxPoint& position, int radius );
    void markerLozenge( const wxPoint& position, int radius );

    // Helper function for sketched filler segment
    void segmentAsOval( const wxPoint& start, const wxPoint& end, int width,
                          EDA_DRAW_MODE_T tracemode );
    void sketchOval( const wxPoint& pos, const wxSize& size, int orient,
                      int width );

    // Coordinate and scaling conversion functions
    virtual DPOINT userToDeviceCoordinates( const wxPoint& pos );
    virtual DPOINT userToDeviceSize( const wxSize& size );
    virtual double userToDeviceSize( double size );

    /// Plot scale - chosen by the user (even implicitly with 'fit in a4')
    double        plotScale;

    /// Device scale (from IUs to device units - usually decimils)
    double        iuPerDeviceUnit;

    /// Plot offset (in IUs)
    wxPoint       plotOffset;

    /// Output file
    FILE*         outputFile;

    // Pen handling
    bool          colorMode, negativeMode;
    int           defaultPenWidth;
    int           currentPenWidth;
    /// Current pen state: 'U', 'D' or 'Z' (see PenTo)
    char          penState;
    /// Last pen positions; set to -1,-1 when the pen is at rest
    wxPoint       penLastpos;
    bool          plotMirror;
    wxString      creator;
    wxString      filename;
    PAGE_INFO     pageInfo;
    /// Paper size in IU - not in mils
    wxSize        paperSize;
};


class HPGL_PLOTTER : public PLOTTER
{
public:
    HPGL_PLOTTER()
    {
    }

    virtual PlotFormat GetPlotterType() const
    {
	return PLOT_FORMAT_HPGL;
    }

    virtual bool StartPlot( FILE* fout );
    virtual bool EndPlot();

    /// HPGL doesn't handle line thickness or color
    virtual void SetCurrentLineWidth( int width )
    {
        // This is the truth
        currentPenWidth = userToDeviceSize( penDiameter );
    }

    virtual void SetDefaultLineWidth( int width ) {};
    virtual void SetDash( bool dashed );

    virtual void SetColor( EDA_COLOR_T color ) {};

    virtual void SetPenSpeed( int speed )
    {
        wxASSERT( outputFile == 0 );
        penSpeed = speed;
    }

    virtual void SetPenNumber( int number )
    {
        wxASSERT( outputFile == 0 );
        penNumber = number;
    }

    virtual void SetPenDiameter( double diameter )
    {
        wxASSERT( outputFile == 0 );
        penDiameter = diameter;
    }

    virtual void SetPenOverlap( double overlap )
    {
        wxASSERT( outputFile == 0 );
        penOverlap = overlap;
    }

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
			      double aScale, bool aMirror );
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
		       int width = -1 );
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = -1 );
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = -1);

    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               EDA_DRAW_MODE_T tracemode );
    virtual void Arc( const wxPoint& centre, int StAngle, int EndAngle, int rayon,
                      FILL_T fill, int width = -1 );
    virtual void PenTo( const wxPoint& pos, char plume );
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, int orient,
                               EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               int orient, EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 int aPadOrient, EDA_DRAW_MODE_T aTrace_Mode );

protected:
    void penControl( char plume );

    int    penSpeed;
    int    penNumber;
    double penDiameter;
    double penOverlap;
};

/**
 * The PSLIKE_PLOTTER class is an intermediate class to handle common
 * routines for engines working more or less with the postscript imaging
 * model
 */
class PSLIKE_PLOTTER : public PLOTTER
{
public:
    PSLIKE_PLOTTER() : plotScaleAdjX( 1 ), plotScaleAdjY( 1 ), plotWidthAdj( 0 ),
                       psTextMode( PSTEXTMODE_PHANTOM )
    {
    }

    /**
     * Change the current postscript text mode
     */
    void SetPsTextMode( PostscriptTextMode mode )
    {
        psTextMode = mode;
    }

    virtual void SetDefaultLineWidth( int width );

    /**
     * Set the 'fine' scaling for the postscript engine
     */
    void SetScaleAdjust( double scaleX, double scaleY )
    {
        plotScaleAdjX = scaleX;
        plotScaleAdjY = scaleY;
    }

    /**
     * Set the 'width adjustment' for the postscript engine
     * (useful for controlling toner bleeding during direct transfer)
     */
    virtual void SetPlotWidthAdj( double width )
    {
        plotWidthAdj = width;
    }

    virtual double GetPlotWidthAdj() const
    {
        return plotWidthAdj;
    }

    // Pad routines are handled with lower level primitives
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, int orient,
                               EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               int orient, EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 int aPadOrient, EDA_DRAW_MODE_T aTrace_Mode );

    /** The SetColor implementation is split with the subclasses:
     * The PSLIKE computes the rgb values, the subclass emits the
     * operator to actually do it
     */
    virtual void SetColor( EDA_COLOR_T color );

protected:
    void computeTextParameters( const wxPoint&           aPos,
                                const wxString&          aText,
                                int                      aOrient,
                                const wxSize&            aSize,
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
    static const double postscriptTextAscent = 0.718;

    int returnPostscriptTextWidth( const wxString& aText, int aXSize,
                                   bool aItalic, bool aBold );

    /// Fine user scale
    double plotScaleAdjX, plotScaleAdjY;

    /** Plot width adjust XXX should be moved in the PCB plotting
     * routines!
     */
    double plotWidthAdj;

    /// How to draw text
    PostscriptTextMode  psTextMode;
};


class PS_PLOTTER : public PSLIKE_PLOTTER
{
public:
    PS_PLOTTER()
    {
    }

    virtual PlotFormat GetPlotterType() const
    {
	return PLOT_FORMAT_POST;
    }

    virtual bool StartPlot( FILE* fout );
    virtual bool EndPlot();
    virtual void SetCurrentLineWidth( int width );
    virtual void SetDash( bool dashed );

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
			      double aScale, bool aMirror );
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = -1 );
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = -1 );
    virtual void Arc( const wxPoint& centre, int StAngle, int EndAngle,
		      int rayon, FILL_T fill, int width = -1 );

    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = -1);

    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor );

    virtual void PenTo( const wxPoint& pos, char plume );
    virtual void Text( const wxPoint&              aPos,
                       enum EDA_COLOR_T            aColor,
                       const wxString&             aText,
                       int                         aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold );
protected:
    virtual void emitSetRGBColor( double r, double g, double b );
};

class PDF_PLOTTER : public PSLIKE_PLOTTER
{
public:
    PDF_PLOTTER() : pageStreamHandle( 0 ), workFile( NULL )
    {
    }

    virtual PlotFormat GetPlotterType() const
    {
	return PLOT_FORMAT_PDF;
    }

    virtual bool StartPlot( FILE* fout );
    virtual bool EndPlot();
    virtual void StartPage();
    virtual void ClosePage();
    virtual void SetCurrentLineWidth( int width );
    virtual void SetDash( bool dashed );

    /** PDF can have multiple pages, so SetPageSettings can be called
     * with the outputFile open (but not inside a page stream!) */
    virtual void SetPageSettings( const PAGE_INFO& aPageSettings );
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
			      double aScale, bool aMirror );
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = -1 );
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = -1 );
    virtual void Arc( const wxPoint& centre, int StAngle, int EndAngle,
		      int rayon, FILL_T fill, int width = -1 );

    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = -1);

    virtual void PenTo( const wxPoint& pos, char plume );

    virtual void Text( const wxPoint&              aPos,
                       enum EDA_COLOR_T            aColor,
                       const wxString&             aText,
                       int                         aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold );

    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor );


protected:
    virtual void emitSetRGBColor( double r, double g, double b );
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

/* Class to handle a D_CODE when plotting a board : */
#define FIRST_DCODE_VALUE 10    // D_CODE < 10 is a command, D_CODE >= 10 is a tool

struct APERTURE
{
    enum APERTURE_TYPE {
        Circle   = 1,
        Rect     = 2,
        Plotting = 3,
        Oval     = 4
    };

    wxSize        Size;     // horiz and Vert size
    APERTURE_TYPE Type;     // Type ( Line, rect , circulaire , ovale .. )
    int           DCode;    // code number ( >= 10 );

    /* Trivia question: WHY Gerber decided to use D instead of the usual T for
     * tool change? */
};


class GERBER_PLOTTER : public PLOTTER
{
public:
    GERBER_PLOTTER()
    {
        workFile  = 0;
        finalFile = 0;
        currentAperture = apertures.end();
    }

    virtual PlotFormat GetPlotterType() const
    {
	return PLOT_FORMAT_GERBER;
    }

    virtual bool StartPlot( FILE* fout );
    virtual bool EndPlot();
    virtual void SetCurrentLineWidth( int width );
    virtual void SetDefaultLineWidth( int width );

    // RS274X has no dashing, nor colours
    virtual void SetDash( bool dashed ) {};
    virtual void SetColor( EDA_COLOR_T color ) {};
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
	                      double aScale, bool aMirror );
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = -1 );
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = -1 );
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = -1);

    virtual void PenTo( const wxPoint& pos, char plume );
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, int orient,
                               EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               int orient, EDA_DRAW_MODE_T trace_mode );

    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 int aPadOrient, EDA_DRAW_MODE_T aTrace_Mode );

    virtual void SetLayerPolarity( bool aPositive );

protected:
    void selectAperture( const wxSize& size, APERTURE::APERTURE_TYPE type );
    void emitDcode( const DPOINT& pt, int dcode );

    std::vector<APERTURE>::iterator
	getAperture( const wxSize& size, APERTURE::APERTURE_TYPE type );

    FILE* workFile;
    FILE* finalFile;
    wxString m_workFilename;

    void writeApertureList();

    std::vector<APERTURE>           apertures;
    std::vector<APERTURE>::iterator currentAperture;
};


class DXF_PLOTTER : public PLOTTER
{
public:
    DXF_PLOTTER() : textAsLines( false )
    {
    }

    virtual PlotFormat GetPlotterType() const
    {
	return PLOT_FORMAT_DXF;
    }

    /// We can plot text as strokes or as TEXT entities
    void SetDXFTextMode( bool aTextAsLines )
    {
        textAsLines = aTextAsLines;
    }

    virtual bool StartPlot( FILE* fout );
    virtual bool EndPlot();

    // For now we don't use 'thick' primitives, so no line width
    virtual void SetCurrentLineWidth( int width )
    {
        currentPenWidth = 0;
    }

    virtual void SetDefaultLineWidth( int width )
    {
        // DXF lines are infinitesimal
        defaultPenWidth = 0;
    }

    virtual void SetDash( bool dashed );

    virtual void SetColor( EDA_COLOR_T color );

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
			      double aScale, bool aMirror );
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                       int width = -1 );
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = -1 );
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = -1 );
    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               EDA_DRAW_MODE_T tracemode );
    virtual void Arc( const wxPoint& centre, int StAngle, int EndAngle, int rayon,
                      FILL_T fill, int width = -1 );
    virtual void PenTo( const wxPoint& pos, char plume );
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadOval( const wxPoint& pos, const wxSize& size, int orient,
                               EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadRect( const wxPoint& pos, const wxSize& size,
                               int orient, EDA_DRAW_MODE_T trace_mode );
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 int aPadOrient, EDA_DRAW_MODE_T aTrace_Mode );

    virtual void Text( const wxPoint&              aPos,
                       enum EDA_COLOR_T            aColor,
                       const wxString&             aText,
                       int                         aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold );

protected:
    bool textAsLines;
    int currentColor;
};

#endif  // PLOT_COMMON_H_
