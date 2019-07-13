/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew/pcbplot.h
 * @brief Board plot function definition file.
 */

#ifndef PCBPLOT_H_
#define PCBPLOT_H_

#include <wx/filename.h>
#include <pad_shapes.h>
#include <pcb_plot_params.h>
#include <layers_id_colors_and_visibility.h>

class PLOTTER;
class TEXTE_PCB;
class D_PAD;
class DRAWSEGMENT;
class DIMENSION;
class MODULE;
class EDGE_MODULE;
class PCB_TARGET;
class TEXTE_MODULE;
class ZONE_CONTAINER;
class BOARD;
class REPORTER;

///@{
/// \ingroup config

#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_LINE_WIDTH      wxT( "PrintLineWidth" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )
#define OPTKEY_PRINT_PAGE_FRAME      wxT( "PrintPageFrame" )
#define OPTKEY_PRINT_MONOCHROME_MODE wxT( "PrintMonochrome" )
#define OPTKEY_PRINT_PAGE_PER_LAYER  wxT( "PrintSinglePage" )
#define OPTKEY_PRINT_PADS_DRILL      wxT( "PrintPadsDrillOpt" )
#define OPTKEY_PLOT_X_FINESCALE_ADJ  wxT( "PlotXFineScaleAdj" )
#define OPTKEY_PLOT_Y_FINESCALE_ADJ  wxT( "PlotYFineScaleAdj" )
#define CONFIG_PS_FINEWIDTH_ADJ      wxT( "PSPlotFineWidthAdj" )
#define OPTKEY_PLOT_CHECK_ZONES      wxT( "CheckZonesBeforePlotting" )

///@}

// Define min and max reasonable values for plot/print scale
#define PLOT_MIN_SCALE 0.01
#define PLOT_MAX_SCALE 100.0

// Small drill marks (small pad holes) diameter value
#define SMALL_DRILL KiROUND( 0.35 * IU_PER_MM )


// A helper class to plot board items
class BRDITEMS_PLOTTER : public PCB_PLOT_PARAMS
{
    PLOTTER*    m_plotter;
    BOARD*      m_board;
    LSET        m_layerMask;

public:
    BRDITEMS_PLOTTER( PLOTTER* aPlotter, BOARD* aBoard, const PCB_PLOT_PARAMS& aPlotOpts ) :
        PCB_PLOT_PARAMS( aPlotOpts )
    {
        m_plotter = aPlotter;
        m_board = aBoard;
    }

    /**
     * @return a 'width adjustment' for the postscript engine
     * (useful for controlling toner bleeding during direct transfer)
     * added to track width and via/pads size
     */
    int getFineWidthAdj()
    {
        if( GetFormat() == PLOT_FORMAT_POST )
            return GetWidthAdjust();
        else
            return 0;
    }

    // Basic functions to plot a board item
    void SetLayerSet( LSET aLayerMask )     { m_layerMask = aLayerMask; }
    void Plot_Edges_Modules();
    void Plot_1_EdgeModule( EDGE_MODULE* aEdge );
    void PlotTextModule( TEXTE_MODULE* aTextMod, COLOR4D aColor );

    /*
     * Plot field of a module (footprint)
     * Reference, Value, and other fields are plotted only if
     * the corresponding option is enabled
     * Invisible text fields are plotted only if PlotInvisibleText option is set
     * usually they are not plotted.
     */
    bool PlotAllTextsModule( MODULE* aModule );

    void PlotDimension( DIMENSION* Dimension );
    void PlotPcbTarget( PCB_TARGET* PtMire );
    void PlotFilledAreas( ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aPolysList );
    void PlotTextePcb( TEXTE_PCB* pt_texte );
    void PlotDrawSegment( DRAWSEGMENT* PtSegm );

    /**
     * Plot a pad.
     * unlike other items, a pad had not a specific color,
     * and be drawn as a non filled item although the plot mode is filled
     * color and plot mode are needed by this function
     */
    void PlotPad( D_PAD* aPad, COLOR4D aColor, EDA_DRAW_MODE_T aPlotMode );

    /**
     * plot items like text and graphics,
     *  but not tracks and modules
     */
    void PlotBoardGraphicItems();

    /** Function PlotDrillMarks
     * Draw a drill mark for pads and vias.
     * Must be called after all drawings, because it
     * redraw the drill mark on a pad or via, as a negative (i.e. white) shape in
     * FILLED plot mode (for PS and PDF outputs)
     */
    void PlotDrillMarks();

    /**
     * Function getColor
     * @return the layer color
     * @param aLayer = the layer id
     * White color is special: cannot be seen on a white paper
     * and in B&W mode, is plotted as white but other colors are plotted in BLACK
     * so the returned color is LIGHTGRAY when the layer color is WHITE
     */
    COLOR4D getColor( LAYER_NUM aLayer );

private:
    /** Helper function to plot a single drill mark. It compensate and clamp
     * the drill mark size depending on the current plot options
     */
    void plotOneDrillMark( PAD_DRILL_SHAPE_T aDrillShape,
                           const wxPoint& aDrillPos, wxSize aDrillSize,
                           const wxSize& aPadSize,
                           double aOrientation, int aSmallDrill );

};

PLOTTER* StartPlotBoard( BOARD* aBoard,
                         PCB_PLOT_PARAMS* aPlotOpts,
                         int aLayer,
                         const wxString& aFullFileName,
                         const wxString& aSheetDesc );

/**
 * Function PlotOneBoardLayer
 * main function to plot one copper or technical layer.
 * It prepare options and calls the specialized plot function,
 * according to the layer type
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayer = the layer id to plot
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 */
void PlotOneBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, PCB_LAYER_ID aLayer,
                        const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Function PlotStandardLayer
 * plot copper or technical layers.
 * not used for silk screen layers, because these layers have specific
 * requirements, mainly for pads
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayerMask = the mask to define the layers to plot
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 *
 * aPlotOpt has 3 important options to control this plot,
 * which are set, depending on the layer type to plot
 *      SetEnablePlotVia( bool aEnable )
 *          aEnable = true to plot vias, false to skip vias (has meaning
 *                      only for solder mask layers).
 *      SetSkipPlotNPTH_Pads( bool aSkip )
 *          aSkip = true to skip NPTH Pads, when the pad size and the pad hole
 *                  have the same size. Used in GERBER format only.
 *      SetDrillMarksType( DrillMarksType aVal ) controle the actual hole:
 *              no hole, small hole, actual hole
 */
void PlotStandardLayer( BOARD* aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Function PlotLayerOutlines
 * plot copper outline of a copper layer.
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayerMask = the mask to define the layers to plot
 * @param aPlotOpt = the plot options. Has meaning for some formats only
 */
void PlotLayerOutlines( BOARD *aBoard, PLOTTER* aPlotter,
                        LSET aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Function PlotSilkScreen
 * plot silkscreen layers which have specific requirements, mainly for pads.
 * Should not be used for other layers
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayerMask = the mask to define the layers to plot (silkscreen Front and/or Back)
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 */
void PlotSilkScreen( BOARD* aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                     const PCB_PLOT_PARAMS&  aPlotOpt );


/**
 * Function BuildPlotFileName (helper function)
 * Complete a plot filename: forces the output directory,
 * add a suffix to the name and sets the specified extension
 * the suffix is usually the layer name
 * replaces not allowed chars in suffix by '_'
 * @param aFilename = the wxFileName to initialize
 *                  Contains the base filename
 * @param aOutputDir = the path
 * @param aSuffix = the suffix to add to the base filename
 * @param aExtension = the file extension
 */
void BuildPlotFileName( wxFileName*     aFilename,
                        const wxString& aOutputDir,
                        const wxString& aSuffix,
                        const wxString& aExtension );


/**
 * Function GetGerberProtelExtension
 * @return the appropriate Gerber file extension for \a aLayer
 * used by Protel, and still sometimes in use (although the
 * official Gerber Ext is now .gbr)
 */
const wxString GetGerberProtelExtension( LAYER_NUM aLayer );

/**
 * Function GetGerberFileFunctionAttribute
 * Returns the "file function" attribute for \a aLayer, as defined in the
 * Gerber file format specification J1 (chapter 5). The returned string includes
 * the "%TF.FileFunction" attribute prefix and the "*%" suffix.
 * @param aBoard = the board, needed to get the total count of copper layers
 * @param aLayer = the layer number to create the attribute for
 * @return The attribute, as a text string
 */
const wxString GetGerberFileFunctionAttribute( const BOARD *aBoard, LAYER_NUM aLayer );

/**
 * Calculates some X2 attributes, as defined in the
 * Gerber file format specification J4 (chapter 5) and add them
 * the to the gerber file header:
 * TF.GenerationSoftware
 * TF.CreationDate
 * TF.ProjectId
 * file format attribute is not added
 * @param aPlotter = the current plotter.
 * @param aBoard = the board, needed to extract some info
 * @param aUseX1CompatibilityMode = false to generate X2 attributes, true to
 * use X1 compatibility (X2 attributes added as structured comments,
 * starting by "G04 #@! " followed by the X2 attribute
 */
void AddGerberX2Header( PLOTTER * aPlotter,
            const BOARD *aBoard, bool aUseX1CompatibilityMode = false );

/**
 * Calculates some X2 attributes, as defined in the Gerber file format
 * specification and add them to the gerber file header:
 * TF.GenerationSoftware
 * TF.CreationDate
 * TF.ProjectId
 * TF.FileFunction
 * TF.FilePolarity
 *
 * @param aPlotter = the current plotter.
 * @param aBoard = the board, needed to extract some info
 * @param aLayer = the layer number to create the attribute for
 * @param aUseX1CompatibilityMode = false to generate X2 attributes, true to
 * use X1 compatibility (X2 attributes added as structured comments,
 * starting by "G04 #@! " followed by the X2 attribute
 */
void AddGerberX2Attribute( PLOTTER * aPlotter, const BOARD *aBoard,
                           LAYER_NUM aLayer, bool aUseX1CompatibilityMode );

#endif // PCBPLOT_H_
