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
 * Plotting engines similar to ps (PostScript, Gerber, svg)
 *
 * @file plotters_pslike.h
 */

#pragma once

#include <plotter.h>


/**
 * The PSLIKE_PLOTTER class is an intermediate class to handle common routines for engines
 * working more or less with the postscript imaging model.
 */
class PSLIKE_PLOTTER : public PLOTTER
{
public:
    PSLIKE_PLOTTER() :
            plotScaleAdjX( 1 ),
            plotScaleAdjY( 1 ),
            m_textMode( PLOT_TEXT_MODE::PHANTOM )
    {
    }

    /**
     * PS and PDF fully implement native text (for the Latin-1 subset)
     */
    virtual void SetTextMode( PLOT_TEXT_MODE mode ) override
    {
        if( mode != PLOT_TEXT_MODE::DEFAULT )
            m_textMode = mode;
    }

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
                                 OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadOval( const wxPoint& aPadPos, const wxSize& aSize, double aPadOrient,
                               OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadRect( const wxPoint& aPadPos, const wxSize& aSize, double aPadOrient,
                               OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize, double aOrient,
                                 SHAPE_POLY_SET* aPolygons,
                                 OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashRegularPolygon( const wxPoint& aShapePos, int aDiameter, int aCornerCount,
                                      double aOrient, OUTLINE_MODE aTraceMode,
                                      void* aData ) override;

    /**
     * The SetColor implementation is split with the subclasses:
     * The PSLIKE computes the rgb values, the subclass emits the
     * operator to actually do it
     */
    virtual void SetColor( const COLOR4D& color ) override;

protected:
    /**
     * This is the core for postscript/PDF text alignment.
     *
     * It computes the transformation matrix to generate a user space
     * system aligned with the text. Even the PS uses the concat
     * operator to simplify PDF generation (concat is everything PDF
     * has to modify the CTM. Lots of parameters, both in and out.
     */
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

    /**
     * Computes the x coordinates for the overlining in a string of text.
     * Fills the passed vector with couples of (start, stop) values to be
     * used in the text coordinate system (use computeTextParameters to
     * obtain the parameters to establish such a system)
     */
    void postscriptOverlinePositions( const wxString& aText, int aXSize, bool aItalic, bool aBold,
                                      std::vector<int> *pos_pairs );

    /// convert a wxString unicode string to a char string compatible with the accepted
    /// string plotter format (convert special chars and non ascii7 chars)
    virtual std::string encodeStringForPlotter( const wxString& aUnicode );

    /// Virtual primitive for emitting the setrgbcolor operator
    virtual void emitSetRGBColor( double r, double g, double b ) = 0;

    /// Height of the postscript font (from the AFM)
    static const double postscriptTextAscent; // = 0.718;

    /**
     * Sister function for the GraphicTextWidth in drawtxt.cpp
     * Does the same processing (i.e. calculates a text string width) but
     * using postscript metrics for the Helvetica font (optionally used for
     * PS and PDF plotting
     */
    int returnPostscriptTextWidth( const wxString& aText, int aXSize, bool aItalic, bool aBold );

    /// Fine user scale adjust ( = 1.0 if no correction)
    double plotScaleAdjX, plotScaleAdjY;

    /// How to draw text
    PLOT_TEXT_MODE m_textMode;
};


class PS_PLOTTER : public PSLIKE_PLOTTER
{
public:
    PS_PLOTTER()
    {
        // The phantom plot in postscript is an hack and reportedly
        // crashes Adobe's own postscript interpreter!
        m_textMode = PLOT_TEXT_MODE::STROKE;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "ps" ) );
    }

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::POST;
    }

    /**
     * The code within this function (and the CloseFilePS function)
     * creates postscript files whose contents comply with Adobe's
     * Document Structuring Convention, as documented by assorted
     * details described within the following URLs:
     *
     * http://en.wikipedia.org/wiki/Document_Structuring_Conventions
     * http://partners.adobe.com/public/developer/en/ps/5001.DSC_Spec.pdf
     *
     *
     * BBox is the boundary box (position and size of the "client rectangle"
     * for drawings (page - margins) in mils (0.001 inch)
     */
    virtual bool StartPlot() override;
    virtual bool EndPlot() override;

    /**
     * Set the current line width (in IUs) for the next plot.
     */
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override;

    /**
     * PostScript supports dashed lines.
     */
    virtual void SetDash( PLOT_DASH_TYPE dashed ) override;

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_TYPE fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_TYPE fill, int width = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList, FILL_TYPE aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;

    /**
     * PostScript-likes at the moment are the only plot engines supporting bitmaps.
     */
    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;
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
protected:
    virtual void emitSetRGBColor( double r, double g, double b ) override;
};


class PDF_PLOTTER : public PSLIKE_PLOTTER
{
public:
    PDF_PLOTTER() :
            pageTreeHandle( 0 ),
            fontResDictHandle( 0 ),
            pageStreamHandle( 0 ),
            streamLengthHandle( 0 ),
            workFile( nullptr )
    {
    }

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::PDF;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "pdf" ) );
    }

    /**
     * Open or create the plot file aFullFilename.
     *
     * The base class open the file in text mode, so we should have this
     * function overlaid for PDF files, which are binary files.
     *
     * @param aFullFilename is the full file name of the file to create.
     * @return true if success, false if the file cannot be created/opened.
     */
    virtual bool OpenFile( const wxString& aFullFilename ) override;

    /**
     * The PDF engine supports multiple pages; the first one is opened 'for free' the following
     * are to be closed and reopened. Between each page parameters can be set.
     */
    virtual bool StartPlot() override;
    virtual bool EndPlot() override;

    /**
     * Start a new page in the PDF document.
     */
    virtual void StartPage();

    /**
     * Close the current page in the PDF document (and emit its compressed stream).
     */
    virtual void ClosePage();

    /**
     * Pen width setting for PDF.
     *
     * Since the specs *explicitly* says that a 0 width is a bad thing to use (since it
     * results in 1 pixel traces), we convert such requests to the minimal width (like 1)
     * Note pen width = 0 is used in plot polygons to plot filled polygons with no outline
     * thickness.  Use in this case pen width = 1 does not actually change the polygon.
     */
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override;

    /**
     * PDF supports dashed lines
     */
    virtual void SetDash( PLOT_DASH_TYPE dashed ) override;

    /**
     * PDF can have multiple pages, so SetPageSettings can be called
     * with the outputFile open (but not inside a page stream!)
     */
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;

    /**
     * Rectangles in PDF. Supported by the native operator.
     */
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;

    /**
     * Circle drawing for PDF. They're approximated by curves, but fill is supported
     */
    virtual void Circle( const wxPoint& pos, int diametre, FILL_TYPE fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;

    /**
     * The PDF engine can't directly plot arcs, it uses the base emulation.
     * So no filled arcs (not a great loss... )
     */
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_TYPE fill, int width = USE_DEFAULT_LINE_WIDTH ) override;

    /**
     * Polygon plotting for PDF. Everything is supported
     */
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList, FILL_TYPE aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;

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
     * PDF images are handles as inline, not XObject streams...
     */
    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor ) override;


protected:
    /// convert a wxString unicode string to a char string compatible with the accepted
    /// string PDF format (convert special chars and non ascii7 chars)
    std::string encodeStringForPlotter( const wxString& aUnicode ) override;

    /**
     * PDF supports colors fully. It actually has distinct fill and pen colors,
     * but we set both at the same time.
     *
     * XXX Keeping them divided could result in a minor optimization in
     * Eeschema filled shapes, but would propagate to all the other plot
     * engines. Also arcs are filled as pies but only the arc is stroked so
     * it would be difficult to handle anyway.
     */
    virtual void emitSetRGBColor( double r, double g, double b ) override;

    /**
     * Allocate a new handle in the table of the PDF object. The
     * handle must be completed using startPdfObject. It's an in-RAM operation
     * only, no output is done.
     */
    int allocPdfObject();

    /**
     * Open a new PDF object and returns the handle if the parameter is -1.
     * Otherwise fill in the xref entry for the passed object
     */
    int startPdfObject(int handle = -1);

    /**
     * Close the current PDF object
     */
    void closePdfObject();

    /**
     * Starts a PDF stream (for the page). Returns the object handle opened
     * Pass -1 (default) for a fresh object. Especially from PDF 1.5 streams
     * can contain a lot of things, but for the moment we only handle page
     * content.
     */
    int startPdfStream(int handle = -1);

    /**
     * Finish the current PDF stream (writes the deferred length, too)
     */
    void closePdfStream();

    int pageTreeHandle;      /// Handle to the root of the page tree object
    int fontResDictHandle;   /// Font resource dictionary
    std::vector<int> pageHandles;/// Handles to the page objects
    int pageStreamHandle;    /// Handle of the page content object
    int streamLengthHandle;      /// Handle to the deferred stream length
    wxString workFilename;
    FILE* workFile;              /// Temporary file to construct the stream before zipping
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

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::SVG;
    }

    virtual void SetColor( const COLOR4D& color ) override;

    /**
     * Create SVG file header.
     */
    virtual bool StartPlot() override;
    virtual bool EndPlot() override;

    /**
     * Set the current line width (in IUs) for the next plot.
     */
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override;

    /**
     * SVG supports dashed lines.
     */
    virtual void SetDash( PLOT_DASH_TYPE dashed ) override;

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_TYPE fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_TYPE fill, int width = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void BezierCurve( const wxPoint& aStart, const wxPoint& aControl1,
                              const wxPoint& aControl2, const wxPoint& aEnd,
                              int aTolerance,
                              int aLineThickness = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_TYPE aFill, int aWidth = USE_DEFAULT_LINE_WIDTH,
                           void * aData = nullptr ) override;

    /**
     * PostScript-likes at the moment are the only plot engines supporting bitmaps.
     */
    virtual void PlotImage( const wxImage& aImage, const wxPoint& aPos,
                            double aScaleFactor ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;

    /**
     * Select SVG step size (number of digits needed for 1 mm or 1 inch )
     *
     * Should be called only after SetViewport() is called
     *
     * @param aResolution = number of digits in mantissa of coordinate
     *                      use a value from 3-6
     *                      do not use value > 6 to avoid overflow in PCBNEW
     *                      do not use value > 4 to avoid overflow for other parts
     * @param aUseInches = true to use inches, false to use mm (default)
     */
    virtual void SetSvgCoordinatesFormat( unsigned aResolution, bool aUseInches = false ) override;

    /**
     * Calling this function allows one to define the beginning of a group
     * of drawing items (used in SVG format to separate components)
     * @param aData should be a string for the SVG ID tag
     */
    virtual void StartBlock( void* aData ) override;

    /**
     * Calling this function allows one to define the end of a group of drawing
     * items the group is started by StartBlock()
     * @param aData should be null
     */
    virtual void EndBlock( void* aData ) override;

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

protected:
    /**
     * Initialize m_pen_rgb_color from reduced values r, g ,b
     * ( reduced values are 0.0 to 1.0 )
     */
    virtual void emitSetRGBColor( double r, double g, double b ) override;

    /**
     * Output the string which define pen and brush color, shape, transparency
     *
     * @param aIsGroup If false, do not form a new group for the style.
     * @param aExtraStyle If given, the string will be added into the style string before closing
     */
    void setSVGPlotStyle( bool aIsGroup = true, const std::string& aExtraStyle = {} );

    /**
     * Prepare parameters for setSVGPlotStyle()
     */
    void setFillMode( FILL_TYPE fill );

    FILL_TYPE      m_fillMode;          // true if the current contour
                                        // rect, arc, circle, polygon must be filled
    long           m_pen_rgb_color;     // current rgb color value: each color has
                                        // a value 0 ... 255, and the 3 colors are
                                        // grouped in a 3x8 bits value
                                        // (written in hex to svg files)
    long           m_brush_rgb_color;   // same as m_pen_rgb_color, used to fill
                                        // some contours.
    bool           m_graphics_changed;  // true if a pen/brush parameter is modified
                                        // color, pen size, fill mode ...
                                        // the new SVG stype must be output on file
    PLOT_DASH_TYPE m_dashed;            // plot line style
    bool           m_useInch;           // is 0 if the step size is 10**-n*mm
                                        // is 1 if the step size is 10**-n*inch
                                        // Where n is given from m_precision
    unsigned       m_precision;         // How fine the step size is
                                        // Use 3-6 (3 means um precision, 6 nm precision) in PcbNew
                                        // 3-4 in other modules (avoid values >4 to avoid overflow)
                                        // see also comment for m_useInch.
};
