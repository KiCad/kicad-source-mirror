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

/**
 * Plotting engines similar to ps (PostScript, Gerber, svg)
 *
 * @file plotters_pslike.h
 */

#pragma once

#include "plotter.h"
#include <memory>
#include <plotters/pdf_stroke_font.h>
#include <plotters/pdf_outline_font.h>
#include <math/vector3.h>

namespace MARKUP { struct NODE; }

/**
 * The PSLIKE_PLOTTER class is an intermediate class to handle common routines for engines
 * working more or less with the postscript imaging model.
 */
class PSLIKE_PLOTTER : public PLOTTER
{
public:
    PSLIKE_PLOTTER( const PROJECT* aProject = nullptr) :
            PLOTTER( aProject ),
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
    virtual void FlashPadCircle( const VECTOR2I& aPadPos, int aDiameter, void* aData ) override;
    virtual void FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aPadOrient, void* aData ) override;
    virtual void FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aPadOrient, void* aData ) override;
    virtual void FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                    int aCornerRadius, const EDA_ANGLE& aOrient,
                                    void* aData ) override;
    virtual void FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                 const EDA_ANGLE& aOrient, SHAPE_POLY_SET* aPolygons,
                                 void* aData ) override;
    virtual void FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                 const EDA_ANGLE& aPadOrient, void* aData ) override;
    virtual void FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter, int aCornerCount,
                                      const EDA_ANGLE& aOrient, void* aData ) override;

    /**
     * The SetColor implementation is split with the subclasses: the PSLIKE computes the rgb
     * values, the subclass emits the operator to actually do it
     */
    virtual void SetColor( const COLOR4D& color ) override;

protected:
    /**
     * This is the core for postscript/PDF text alignment.
     *
     * It computes the transformation matrix to generate a user space system aligned with the text.
     * Even the PS uses the concat operator to simplify PDF generation (concat is everything PDF
     * has to modify the CTM. Lots of parameters, both in and out.
     */
    void computeTextParameters( const VECTOR2I&          aPos,
                                const wxString&          aText,
                                const EDA_ANGLE&         aOrient,
                                const VECTOR2I&          aSize,
                                bool                     aMirror,
                                enum GR_TEXT_H_ALIGN_T   aH_justify,
                                enum GR_TEXT_V_ALIGN_T   aV_justify,
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

    /// convert a wxString unicode string to a char string compatible with the accepted
    /// string plotter format (convert special chars and non ascii7 chars)
    virtual std::string encodeStringForPlotter( const wxString& aUnicode );

    /// Virtual primitive for emitting the setrgbcolor operator
    virtual void emitSetRGBColor( double r, double g, double b, double a ) = 0;

    /// Height of the postscript font (from the AFM)
    static const double postscriptTextAscent; // = 0.718;

    /**
     * Sister function for the GRTextWidth in gr_text.cpp
     * Does the same processing (i.e. calculates a text string width) but using postscript metrics
     * for the Helvetica font (optionally used for PS and PDF plotting
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
    PS_PLOTTER( const PROJECT* aProject = nullptr ) :
        PSLIKE_PLOTTER( aProject )
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
    virtual bool StartPlot( const wxString& aPageNumber ) override;
    virtual bool EndPlot() override;

    /**
     * Set the current line width (in IUs) for the next plot.
     */
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override;

    /**
     * PostScript supports dashed lines.
     */
    virtual void SetDash( int aLineWidth, LINE_STYLE aLineStyle ) override;

    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                       int aCornerRadius = 0 ) override;
    virtual void Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width ) override;
    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                      const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth ) override;

    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill,
                           int aWidth, void* aData = nullptr ) override;

    /**
     * PostScript-likes at the moment are the only plot engines supporting bitmaps.
     */
    virtual void PlotImage( const wxImage& aImage, const VECTOR2I& aPos,
                            double aScaleFactor ) override;

    virtual void PenTo( const VECTOR2I& pos, char plume ) override;
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

    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aCornerList, FILL_T aFill,
                           int aWidth, void* aData = nullptr ) override;


protected:
    virtual void emitSetRGBColor( double r, double g, double b, double a ) override;
};


struct PDF_3D_VIEW
{
    std::string m_name;
    std::vector<float> m_cameraMatrix;
    float       m_cameraCenter;
    float       m_fov;
};


class PDF_PLOTTER : public PSLIKE_PLOTTER
{
public:
    PDF_PLOTTER( const PROJECT* aProject = nullptr ) :
            PSLIKE_PLOTTER( aProject ),
            m_pageTreeHandle( 0 ),
            m_fontResDictHandle( 0 ),
            m_imgResDictHandle( 0 ),
            m_jsNamesHandle( 0 ),
            m_pageStreamHandle( -1 ),
            m_streamLengthHandle( 0 ),
            m_workFile( nullptr ),
            m_totalOutlineNodes( 0 ),
            m_3dModelHandle( -1 ),
            m_3dExportMode( false ),
            m_strokeFontManager( nullptr ),
            m_outlineFontManager( nullptr )
    {
    }

    virtual ~PDF_PLOTTER();

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
    virtual bool StartPlot( const wxString& aPageNumber ) override;

    virtual bool StartPlot( const wxString& aPageNumber,
                            const wxString& aPageName = wxEmptyString );

    virtual bool EndPlot() override;

    /**
     * Start a new page in the PDF document.
     */
    virtual void StartPage( const wxString& aPageNumber,
                            const wxString& aPageName = wxEmptyString,
                            const wxString& aParentPageNumber = wxEmptyString,
                            const wxString& aParentPageName = wxEmptyString );

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
    virtual void SetDash( int aLineWidth, LINE_STYLE aLineStyle ) override;

    /**
     * PDF can have multiple pages, so SetPageSettings can be called
     * with the outputFile open (but not inside a page stream!)
     */
    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil, double aScale,
                              bool aMirror ) override;

    /**
     * Rectangles in PDF. Supported by the native operator.
     */
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                       int aCornerRadius = 0 ) override;

    /**
     * Circle drawing for PDF. They're approximated by curves, but fill is supported
     */
    virtual void Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width ) override;

    /**
     * The PDF engine can't directly plot arcs so we use polygonization.
     */
    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle,
                      double aRadius, FILL_T aFill, int aWidth ) override;

    /**
     * Polygon plotting for PDF. Everything is supported
     */
    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;

    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aLineChain, FILL_T aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;

    virtual void PenTo( const VECTOR2I& pos, char plume ) override;

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

private:
    // Structure to hold overbar drawing information
    struct OVERBAR_INFO
    {
        VECTOR2I          startPos;    // Start position of overbar text
        VECTOR2I          endPos;      // End position of overbar text
        VECTOR2I          fontSize;    // Font size for proper overbar positioning
        bool              isOutline;   // True if the overbar applies to an outline font run
        GR_TEXT_V_ALIGN_T vAlign;      // Original vertical alignment of the parent text
    };

    /**
     * Render a single word with the given style parameters
     */
    VECTOR2I renderWord( const wxString& aWord, const VECTOR2I& aPosition, const VECTOR2I& aSize,
                         const EDA_ANGLE& aOrient, bool aTextMirrored, int aWidth, bool aBold, bool aItalic,
                         KIFONT::FONT* aFont, const KIFONT::METRICS& aFontMetrics,
                         enum GR_TEXT_V_ALIGN_T aV_justify, TEXT_STYLE_FLAGS aTextStyle );

    /**
     * Recursively render markup nodes with appropriate styling
     */
    VECTOR2I renderMarkupNode( const MARKUP::NODE* aNode, const VECTOR2I& aPosition,
                               const VECTOR2I& aBaseSize, const EDA_ANGLE& aOrient, bool aTextMirrored,
                               int aWidth, bool aBaseBold, bool aBaseItalic, KIFONT::FONT* aFont,
                               const KIFONT::METRICS& aFontMetrics, enum GR_TEXT_V_ALIGN_T aV_justify,
                               TEXT_STYLE_FLAGS aTextStyle, std::vector<OVERBAR_INFO>& aOverbars );

    /**
     * Draw overbar lines above text
     */
    void drawOverbars( const std::vector<OVERBAR_INFO>& aOverbars, const EDA_ANGLE& aOrient,
                       const KIFONT::METRICS& aFontMetrics );

    std::vector<VECTOR2D> arcPath( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                                   const EDA_ANGLE& aAngle, double aRadius );

public:
    virtual void PlotText( const VECTOR2I&        aPos,
                           const COLOR4D&         aColor,
                           const wxString&        aText,
                           const TEXT_ATTRIBUTES& aAttributes,
                           KIFONT::FONT*          aFont,
                           const KIFONT::METRICS& aFontMetrics,
                           void*                  aData = nullptr ) override;

    void HyperlinkBox( const BOX2I& aBox, const wxString& aDestinationURL ) override;

    void HyperlinkMenu( const BOX2I& aBox, const std::vector<wxString>& aDestURLs ) override;

    void Bookmark( const BOX2I& aBox, const wxString& aName,
                   const wxString& aGroupName = wxEmptyString ) override;

    void Plot3DModel( const wxString& aSourcePath, const std::vector<PDF_3D_VIEW>& a3DViews );

    void Set3DExport( bool aYes ) { m_3dExportMode = aYes; }

    /**
     * PDF images are handles as inline, not XObject streams...
     */
    void PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor ) override;

    /**
     * Generates the camera to world matrix for use with a 3D View.
     *
     * @param aTargetPosition The position of the target, which is also the camera rotation center
     * @param aCameraDistance The distance the camera should be set back from the target
     * @param aYawDegrees The yaw angle in degrees
     * @param aPitchDegrees The pitch angle in degrees
     * @param aRollDegrees The roll angle in degrees
     *
     * @return A vector of 12 floats that represent the camera to world matrix
     * in linear form as specified by the PDF Spec 1.7 section 13.6.5
     *
     */
    static std::vector<float> CreateC2WMatrixFromAngles( const VECTOR3D& aTargetPosition,
                                                         float aCameraDistance,
                                                         float aYawDegrees,
                                                         float aPitchDegrees,
                                                         float aRollDegrees );

protected:
    struct OUTLINE_NODE
    {
        int      actionHandle;  ///< Handle to action
        wxString title;         ///< Title of outline node
        int      entryHandle;   ///< Allocated handle for this outline entry

        std::vector<OUTLINE_NODE*> children;    ///< Ordered list of children

        ~OUTLINE_NODE()
        {
            std::for_each( children.begin(), children.end(),
                           []( OUTLINE_NODE* node )
                           {
                               delete node;
                           } );
        }

        OUTLINE_NODE* AddChild( int aActionHandle, const wxString& aTitle, int aEntryHandle )
        {
            OUTLINE_NODE* child = new OUTLINE_NODE
            {
                aActionHandle, aTitle, aEntryHandle, {}
            };

            children.push_back( child );

            return child;
        }
    };

    /**
     * Add a new outline node entry.
     *
     * The PDF object handle is automatically allocated.
     *
     * @param aParent Parent node to append the new node to.
     * @param aActionHandle The handle of an action that may be performed on click, set to -1
     *                      for no action.
     * @param aTitle Title of node to display
     */
    OUTLINE_NODE* addOutlineNode( OUTLINE_NODE* aParent, int aActionHandle,
                                  const wxString& aTitle );

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
    virtual void emitSetRGBColor( double r, double g, double b, double a ) override;

    /**
     * Allocate a new handle in the table of the PDF object.
     *
     * The handle must be completed using startPdfObject. It's an in-RAM operation
     * only, no output is done.
     */
    int allocPdfObject();

    /**
     * Open a new PDF object and returns the handle if the parameter is -1.
     * Otherwise fill in the xref entry for the passed object
     */
    int startPdfObject( int aHandle = -1 );

    /**
     * Close the current PDF object
     */
    void closePdfObject();

    /**
     * Start a PDF stream (for the page).
     *
     * @param handle -1 (default) for a new object. Especially from PDF 1.5 streams can contain
     *               a lot of things, but for the moment we only handle page content.
     * @eturn The object handle opened
     */
    int startPdfStream( int aHandle = -1 );

    /**
     * Finish the current PDF stream (writes the deferred length, too).
     */
    void closePdfStream();

    /**
     * Starts emitting the outline object.
     */
    int emitOutline();

    /**
     * Emits a outline item object and recurses into any children.
     */
    void emitOutlineNode( OUTLINE_NODE* aNode, int aParentHandle, int aNextNode, int aPrevNode );

    /**
     * Emit an action object that instructs a goto coordinates on a page.
     *
     * @return Generated action handle.
     */
    int emitGoToAction( int aPageHandle, const VECTOR2I& aBottomLeft, const VECTOR2I& aTopRight );
    int emitGoToAction( int aPageHandle );


    void endPlotEmitResources();

    void emitStrokeFonts();
    void emitOutlineFonts();

    std::string encodeByteString( const std::string& aBytes );

    int m_pageTreeHandle;           ///< Handle to the root of the page tree object.
    int m_fontResDictHandle;        ///< Font resource dictionary.
    int m_imgResDictHandle;         ///< Image resource dictionary.
    int m_jsNamesHandle;            ///< Handle for Names dictionary with JS.
    std::vector<int> m_pageHandles; ///< Handles to the page objects.
    int m_pageStreamHandle;         ///< Handle of the page content object.
    int m_streamLengthHandle;       ///< Handle to the deferred stream length.

    wxString m_workFilename;
    wxString m_pageName;
    wxString m_parentPageName;

    FILE* m_workFile;               ///< Temporary file to construct the stream before zipping.
    std::vector<long> m_xrefTable;  ///< The PDF xref offset table.

    /// List of user-space page numbers for resolving internal hyperlinks.
    std::vector<wxString>                                  m_pageNumbers;

    /// List of loaded hyperlinks in current page.
    std::vector<std::pair<BOX2I, wxString>>                m_hyperlinksInPage;
    std::vector<std::pair<BOX2I, std::vector<wxString>>>   m_hyperlinkMenusInPage;

    /// Handles for all the hyperlink objects that will be deferred.
    std::map<int, std::pair<BOX2D, wxString>>              m_hyperlinkHandles;
    std::map<int, std::pair<BOX2D, std::vector<wxString>>> m_hyperlinkMenuHandles;

    std::map<wxString, std::vector<std::pair<BOX2I, wxString>>>      m_bookmarksInPage;

    std::map<int, wxImage> m_imageHandles;

    std::unique_ptr<OUTLINE_NODE> m_outlineRoot;        ///< Root outline node.
    int                           m_totalOutlineNodes;  ///< Total number of outline nodes.

    int  m_3dModelHandle;
    bool m_3dExportMode;
    std::unique_ptr<PDF_STROKE_FONT_MANAGER> m_strokeFontManager;
    std::unique_ptr<PDF_OUTLINE_FONT_MANAGER> m_outlineFontManager;
};


class SVG_PLOTTER : public PSLIKE_PLOTTER
{
public:
    SVG_PLOTTER( const PROJECT* aProject = nullptr );

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "svg" ) );
    }

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::SVG;
    }

    /**
     * Create SVG file header.
     */
    virtual bool StartPlot( const wxString& aPageNumber ) override;
    virtual bool EndPlot() override;

    /**
     * Set the current line width (in IUs) for the next plot.
     */
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override;

    /**
     * SVG supports dashed lines.
     */
    virtual void SetDash( int aLineWidth, LINE_STYLE aLineStyle ) override;

    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil, double aScale,
                              bool aMirror ) override;
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                       int aCornerRadius = 0 ) override;
    virtual void Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width ) override;
    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                      const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill,
                      int aWidth ) override;

    virtual void BezierCurve( const VECTOR2I& aStart, const VECTOR2I& aControl1,
                              const VECTOR2I& aControl2, const VECTOR2I& aEnd,
                              int aTolerance, int aLineThickness ) override;

    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill,
                           int aWidth, void * aData ) override;

    /**
     * PostScript-likes at the moment are the only plot engines supporting bitmaps.
     */
    virtual void PlotImage( const wxImage& aImage, const VECTOR2I& aPos,
                            double aScaleFactor ) override;

    virtual void PenTo( const VECTOR2I& pos, char plume ) override;

    /**
     * Select SVG coordinate precision (number of digits needed for 1 mm  )
     * (SVG plotter uses always metric unit)
     * Should be called only after SetViewport() is called
     *
     * @param aPrecision = number of digits in mantissa of coordinate
     *                      use a value from 3-6
     *                      do not use value > 6 to avoid overflow in PCBNEW
     *                      do not use value > 4 to avoid overflow for other parts
     */
    virtual void SetSvgCoordinatesFormat( unsigned aPrecision ) override;

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

    /**
     * Start a new named layer group in the SVG output.
     * @param aLayerName The name/id for the layer group
     */
    void StartLayer( const wxString& aLayerName );

    /**
     * End the current layer group in the SVG output.
     */
    void EndLayer();

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

protected:
    /**
     * Initialize m_pen_rgb_color from reduced values r, g ,b
     * ( reduced values are 0.0 to 1.0 )
     */
    virtual void emitSetRGBColor( double r, double g, double b, double a ) override;

    /**
     * Output the string which define pen and brush color, shape, transparency
     *
     * @param aIsGroup If false, do not form a new group for the style.
     * @param aExtraStyle If given, the string will be added into the style string before closing
     */
    void setSVGPlotStyle( int aLineWidth, bool aIsGroup = true,
                          const std::string& aExtraStyle = {} );

    /**
     * Prepare parameters for setSVGPlotStyle()
     */
    void setFillMode( FILL_T fill );

    FILL_T     m_fillMode;          // true if the current contour rect, arc, circle, polygon must
                                    // be filled
    uint32_t   m_pen_rgb_color;     // current rgb color value: each color has a value 0 ... 255,
                                    //   and the 3 colors are grouped in a 3x8 bits value (written
                                    //   in hex to svg files)
    uint32_t   m_brush_rgb_color;   // same as m_pen_rgb_color, used to fill some contours.
    double     m_brush_alpha;
    bool       m_graphics_changed;  // true if a pen/brush parameter is modified color, pen size,
                                    //   fill mode ... the new SVG stype must be output on file
    LINE_STYLE m_dashed;            // plot line style
    unsigned   m_precision;         // How fine the step size is
                                    // Use 3-6 (3 means um precision, 6 nm precision) in PcbNew
                                    // 3-4 in other modules (avoid values >4 to avoid overflow)
                                    // see also comment for m_useInch.
};
