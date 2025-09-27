/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef PCBPLOT_H_
#define PCBPLOT_H_

#include <lset.h>
#include <padstack.h>         // for PAD_DRILL_SHAPE
#include <pcb_plot_params.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>

class EDA_TEXT;
class PLOTTER;
class PCB_TEXT;
class PCB_BARCODE;
class PAD;
class PCB_SHAPE;
class PCB_TABLE;
class PCB_DIMENSION_BASE;
class FOOTPRINT;
class PCB_TARGET;
class ZONE;
class REPORTER;
class wxFileName;

namespace KIFONT
{
class FONT;
class METRICS;
}


// Define min and max reasonable values for plot/print scale
#define PLOT_MIN_SCALE 0.01
#define PLOT_MAX_SCALE 100.0



// A helper class to plot board items
class BRDITEMS_PLOTTER : public PCB_PLOT_PARAMS
{
public:
    BRDITEMS_PLOTTER( PLOTTER* aPlotter, BOARD* aBoard, const PCB_PLOT_PARAMS& aPlotOpts ) :
            PCB_PLOT_PARAMS( aPlotOpts ),
            m_plotter( aPlotter ),
            m_board( aBoard )
    { }

    /**
     * @return a 'width adjustment' for the postscript engine
     * (useful for controlling toner bleeding during direct transfer)
     * added to track width and via/pads size
     */
    int getFineWidthAdj() const
    {
        if( GetFormat() == PLOT_FORMAT::POST )
            return GetWidthAdjust();
        else
            return 0;
    }

    // Basic functions to plot a board item
    void SetLayerSet( const LSET& aLayerMask ) { m_layerMask = aLayerMask; }
    void PlotFootprintGraphicItems( const FOOTPRINT* aFootprint );
    void PlotFootprintTextItems( const FOOTPRINT* aFootprint );

    void PlotDimension( const PCB_DIMENSION_BASE* aDim );
    void PlotPcbTarget( const PCB_TARGET* aMire );
    void PlotZone( const ZONE* aZone, PCB_LAYER_ID aLayer, const SHAPE_POLY_SET& aPolysList );
    void PlotText( const EDA_TEXT* aText, PCB_LAYER_ID aLayer, bool aIsKnockout,
                   const KIFONT::METRICS& aFontMetrics, bool aStrikeout = false );
    void PlotShape( const PCB_SHAPE* aShape );
    void PlotTableBorders( const PCB_TABLE* aTable );
    void PlotBarCode( const PCB_BARCODE* aBarCode );

    /**
     * Plot a pad.
     *
     * Unlike other items, a pad had not a specific color and be drawn as a non filled item
     * although the plot mode is filled color and plot mode are needed by this function.
     */
    void PlotPad( const PAD* aPad, PCB_LAYER_ID aLayer, const COLOR4D& aColor,
                  bool aSketchMode );

    void PlotPadNumber( const PAD* aPad, const COLOR4D& aColor );

    /**
     * Plot items like text and graphics but not tracks and footprints.
     */
    void PlotBoardGraphicItem( const BOARD_ITEM* item );

    /**
     * Draw a drill mark for pads and vias.
     *
     * Must be called after all drawings, because it redraws the drill mark on a pad or via, as
     * a negative (i.e. white) shape in FILLED plot mode (for PS and PDF outputs).
     */
    void PlotDrillMarks();

    /**
     * White color is special because it cannot be seen on a white paper in B&W mode. It is
     * plotted as white but other colors are plotted in BLACK so the returned color is LIGHTGRAY
     * when the layer color is WHITE.
     *
     * @param aLayer is the layer id.
     * @return the layer color.
     */
    COLOR4D getColor( int aLayer ) const;

private:
    bool hideDNPItems( PCB_LAYER_ID aLayer )
    {
        return GetHideDNPFPsOnFabLayers() && ( aLayer == F_Fab || aLayer == B_Fab );
    }

    bool crossoutDNPItems( PCB_LAYER_ID aLayer )
    {
        return GetCrossoutDNPFPsOnFabLayers() && ( aLayer == F_Fab || aLayer == B_Fab );
    }

    /**
     * Helper function to plot a single drill mark.
     *
     * It compensate and clamp the drill mark size depending on the current plot options.
     */
    void plotOneDrillMark( PAD_DRILL_SHAPE aDrillShape, const VECTOR2I& aDrillPos,
                           const VECTOR2I& aDrillSize, const VECTOR2I& aPadSize,
                           const EDA_ANGLE& aOrientation, int aSmallDrill );

    PLOTTER*  m_plotter;
    BOARD*    m_board;
    LSET      m_layerMask;
};


PLOTTER* StartPlotBoard( BOARD* aBoard, const PCB_PLOT_PARAMS* aPlotOpts, int aLayer,
                         const wxString& aLayerName, const wxString& aFullFileName,
                         const wxString& aSheetName, const wxString& aSheetPath,
                         const wxString& aPageName = wxT( "1" ),
                         const wxString& aPageNumber = wxEmptyString,
                         const int aPageCount = 1);

void setupPlotterNewPDFPage( PLOTTER* aPlotter, BOARD* aBoard, PCB_PLOT_PARAMS* aPlotOpts,
                             const wxString& aLayerName, const wxString& aSheetName,
                             const wxString& aSheetPath, const wxString& aPageNumber,
                             int aPageCount );
        /**
 * Plot a sequence of board layer IDs.
 *
 * @param aBoard is the board to plot.
 * @param aPlotter is the plotter to use.
 * @param aLayerSequence is the sequence of layer IDs to plot.
 * @param aPlotOptions are the plot options (files, sketch). Has meaning for some formats only.
 */
void PlotBoardLayers( BOARD* aBoard, PLOTTER* aPlotter, const LSEQ& aLayerSequence,
                      const PCB_PLOT_PARAMS& aPlotOptions );

/**
 * Plot interactive items (hypertext links, properties, etc.).
 */
void PlotInteractiveLayer( BOARD* aBoard, PLOTTER* aPlotter, const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Plot one copper or technical layer.
 *
 * It prepares options and calls the specialized plot function according to the layer type.
 *
 * @param aBoard is the board to plot.
 * @param aPlotter is the plotter to use.
 * @param aLayer is the layer id to plot.
 * @param aPlotOpt is the plot options (files, sketch). Has meaning for some formats only.
 */
void PlotOneBoardLayer( BOARD* aBoard, PLOTTER* aPlotter, PCB_LAYER_ID aLayer,
                        const PCB_PLOT_PARAMS& aPlotOpt, bool isPrimaryLayer );

/**
 * Plot copper or technical layers.
 *
 * This is not used for silk screen layers because these layers have specific requirements.
 * This is  mainly for pads.
 *
 * @param aBoard is the board to plot.
 * @param aPlotter is the plotter to use.
 * @param aLayerMask is the mask to define the layers to plot.
 * @param aPlotOpt is the plot options (files, sketch). Has meaning for some formats only.
 *
 * aPlotOpt has 3 important options which are set depending on the layer type to plot:
 *    SetEnablePlotVia( bool aEnable )
 *        aEnable = true to plot vias, false to skip vias (has meaning only for solder mask layers)
 *    SetSkipPlotNPTH_Pads( bool aSkip )
 *        aSkip = true to skip NPTH Pads, when the pad size and the pad hole have the same size.
 *        Used in GERBER format only.
 *    SetDrillMarksType( DrillMarksType aVal )
 *        aVal = no hole, small hole, actual hole size
 */
void PlotStandardLayer( BOARD* aBoard, PLOTTER* aPlotter, const LSET& aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Plot copper outline of a copper layer.
 *
 * @param aBoard is the board to plot.
 * @param aPlotter is the plotter to use.
 * @param aLayerMask is the mask to define the layers to plot.
 * @param aPlotOpt is the plot options. Has meaning for some formats only.
 */
void PlotLayerOutlines( BOARD* aBoard, PLOTTER* aPlotter, const LSET& aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Complete a plot filename.
 *
 * It forces the output directory, adds a suffix to the name, and sets the specified extension.
 * The suffix is usually the layer name and replaces illegal file name character in the suffix
 * with an underscore character.
 *
 * @param aFilename is the file name to initialize that contains the base filename.
 * @param aOutputDir is the path.
 * @param aSuffix is the suffix to add to the base filename.
 * @param aExtension is the file extension.
 */
void BuildPlotFileName( wxFileName* aFilename, const wxString& aOutputDir, const wxString& aSuffix,
                        const wxString& aExtension );


/**
 * @return the appropriate Gerber file extension for \a aLayer
 */
const wxString GetGerberProtelExtension( int aLayer );

/**
 * Return the "file function" attribute for \a aLayer, as defined in the
 * Gerber file format specification J1 (chapter 5).
 *
 * The returned string includes the "%TF.FileFunction" attribute prefix and the "*%" suffix.
 *
 * @param aBoard is the board, needed to get the total count of copper layers.
 * @param aLayer is the layer number to create the attribute for.
 * @return The attribute, as a text string
 */
const wxString GetGerberFileFunctionAttribute( const BOARD* aBoard, int aLayer );

/**
 * Calculate some X2 attributes as defined in the Gerber file format specification J4
 * (chapter 5) and add them the to the gerber file header.
 *
 * TF.GenerationSoftware
 * TF.CreationDate
 * TF.ProjectId
 * file format attribute is not added
 *
 * @param aPlotter is the current plotter.
 * @param aBoard is the board, needed to extract some info.
 * @param aUseX1CompatibilityMode set to false to generate X2 attributes, true to
 *        use X1 compatibility (X2 attributes added as structured comments,
 *        starting by "G04 #@! " followed by the X2 attribute
 */
void AddGerberX2Header( PLOTTER* aPlotter, const BOARD* aBoard,
                        bool aUseX1CompatibilityMode = false );

/**
 * Calculate some X2 attributes as defined in the Gerber file format specification and add them
 * to the gerber file header.
 *
 * TF.GenerationSoftware
 * TF.CreationDate
 * TF.ProjectId
 * TF.FileFunction
 * TF.FilePolarity
 *
 * @param aPlotter is the current plotter.
 * @param aBoard is the board, needed to extract some info.
 * @param aLayer is the layer number to create the attribute for.
 * @param aUseX1CompatibilityMode set to false to generate X2 attributes, true to use X1
 *        compatibility (X2 attributes added as structured comments, starting by "G04 #@! "
 *        followed by the X2 attribute.
 */
void AddGerberX2Attribute( PLOTTER* aPlotter, const BOARD* aBoard, int aLayer,
                           bool aUseX1CompatibilityMode );

#endif // PCBPLOT_H_
