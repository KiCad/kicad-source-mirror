/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2017 KiCad Developers, see change_log.txt for contributors.
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
 * @file gendrill_file_writer_base.h
 * @brief helper classes to handle hole info for drill files generators.
 */
#ifndef GENDRILL_FILE_WRITER_BASE_H
#define GENDRILL_FILE_WRITER_BASE_H

#include <vector>

class BOARD_ITEM;


// the DRILL_TOOL class  handles tools used in the excellon drill file:
class DRILL_TOOL
{
public:
    int m_Diameter;         // the diameter of the used tool (for oblong, the smaller size)
    int m_TotalCount;       // how many times it is used (round and oblong)
    int m_OvalCount;        // oblong count
    bool m_Hole_NotPlated;  // Is the hole plated or not plated

public:
    DRILL_TOOL( int aDiameter, bool a_NotPlated )
    {
        m_TotalCount     = 0;
        m_OvalCount      = 0;
        m_Diameter       = aDiameter;
        m_Hole_NotPlated = a_NotPlated;
    }
};


/* the HOLE_INFO class handle hole which must be drilled (diameter, position and layers)
 * For buried or micro vias, the hole is not on all layers.
 * So we must generate a drill file for each layer pair (adjacent layers)
 * Not plated holes are always through holes, and must be output on a specific drill file
 * because they are drilled after the Pcb process is finished.
 */
class HOLE_INFO
{
public:
    BOARD_ITEM*  m_ItemParent;           // The pad or via parent of this hole
    int          m_Hole_Diameter;        // hole value, and for oblong: min(hole size x, hole size y)
    int          m_Tool_Reference;       // Tool reference for this hole = 1 ... n (values <=0 must not be used)
    wxSize       m_Hole_Size;            // hole size for oblong holes
    double       m_Hole_Orient;          // Hole rotation (= pad rotation) for oblong holes
    int          m_Hole_Shape;           // hole shape: round (0) or oval (1)
    wxPoint      m_Hole_Pos;             // hole position
    PCB_LAYER_ID m_Hole_Bottom_Layer;    // hole ending layer (usually back layer)
    PCB_LAYER_ID m_Hole_Top_Layer;       // hole starting layer (usually front layer):
                                         // m_Hole_Top_Layer < m_Hole_Bottom_Layer
    bool         m_Hole_NotPlated;       // hole not plated. Must be in a specific drill file or section

public:
    HOLE_INFO()
    {
        m_ItemParent = nullptr;
        m_Hole_NotPlated = false;
        m_Hole_Diameter = 0;
        m_Tool_Reference = 0;
        m_Hole_Orient = 0.0;
        m_Hole_Shape = 0;
        m_Hole_Bottom_Layer = B_Cu;
        m_Hole_Top_Layer = F_Cu;
    }
};


/* the DRILL_PRECISION helper class to handle drill precision format in excellon files
 */
class DRILL_PRECISION
{
public:
    int m_lhs;      // Left digit number (integer value of coordinates)
    int m_rhs;      // Right digit number (decimal value of coordinates)

public:
    DRILL_PRECISION( int l = 2, int r = 4 )
    {
        m_lhs = l; m_rhs = r;
    }


    wxString GetPrecisionString()
    {
        wxString text;

        text << m_lhs << wxT( ":" ) << m_rhs;
        return text;
    }
};


typedef std::pair<PCB_LAYER_ID, PCB_LAYER_ID>   DRILL_LAYER_PAIR;

/**
 * GENDRILL_WRITER_BASE is a class to create drill maps and drill report,
 * and a helper class to created drill files.
 * drill files are created by specialized derived classes, depenfing on the
 * file format.
 */
class GENDRILL_WRITER_BASE
{
public:
    enum ZEROS_FMT {            // Zero format in coordinates
        DECIMAL_FORMAT,         // Floating point coordinates
        SUPPRESS_LEADING,       // Suppress leading zeros
        SUPPRESS_TRAILING,      // Suppress trainling zeros
        KEEP_ZEROS              // keep zeros
    };

protected:
    BOARD*                   m_pcb;
    wxString                 m_drillFileExtension;      // .drl or .gbr, depending on format
    bool                     m_unitsDecimal;            // true = decimal, false = inches
    ZEROS_FMT                m_zeroFormat;              // the zero format option for output file
    DRILL_PRECISION          m_precision;               // The current coordinate precision (not used in decimal format)
    double                   m_conversionUnits;         // scaling factor to convert the board unites to
                                                        // Excellon/Gerber units (i.e inches or mm)
    wxPoint                  m_offset;                  // Drill offset coordinates
    bool                     m_merge_PTH_NPTH;          // True to generate only one drill file
    std::vector<HOLE_INFO>   m_holeListBuffer;          // Buffer containing holes
    std::vector<DRILL_TOOL>  m_toolListBuffer;          // Buffer containing tools

    PlotFormat               m_mapFileFmt;              // the format of the map drill file,
                                                        // if this map is needed
    const PAGE_INFO*         m_pageInfo;                // the page info used to plot drill maps
                                                        // If NULL, use a A4 page format
    // This Ctor is protected.
    // Use derived classes to build a fully initialized GENDRILL_WRITER_BASE class.
    GENDRILL_WRITER_BASE( BOARD* aPcb )
    {
        m_pcb  = aPcb;
        m_conversionUnits = 1.0;
        m_unitsDecimal    = true;
        m_mapFileFmt = PLOT_FORMAT_PDF;
        m_pageInfo = NULL;
        m_merge_PTH_NPTH = false;
        m_zeroFormat = DECIMAL_FORMAT;
    }

public:
    virtual ~GENDRILL_WRITER_BASE()
    {
    }

    /**
     * set the option to make separate drill files for PTH and NPTH
     * @param aMerge = true to make only one file containing PTH and NPTH
     * = false to create 2 separate files
     */
    void SetMergeOption( bool aMerge ) { m_merge_PTH_NPTH = aMerge; }

    /**
     * Return the plot offset (usually the position
     * of the auxiliary axis
     */
    const wxPoint GetOffset() { return m_offset; }

    /**
     * Sets the page info used to plot drill maps
     * If NULL, a A4 page format will be used
     * @param aPageInfo = a reference to the page info, usually used to plot/display the board
     */
    void SetPageInfo( const PAGE_INFO* aPageInfo ) { m_pageInfo = aPageInfo; }

    /**
     * Initialize the format for the drill map file
     * @param aMapFmt = a PlotFormat value (one of
     * PLOT_FORMAT_HPGL, PLOT_FORMAT_POST, PLOT_FORMAT_GERBER,
     * PLOT_FORMAT_DXF, PLOT_FORMAT_SVG, PLOT_FORMAT_PDF
     * the most useful are PLOT_FORMAT_PDF and PLOT_FORMAT_POST
     */
    void SetMapFileFormat( PlotFormat aMapFmt ) { m_mapFileFmt = aMapFmt; }

    /**
     * Function CreateMapFilesSet
     * Creates the full set of map files for the board, in PS, PDF ... format
     * (use SetMapFileFormat() to select the format)
     * filenames are computed from the board name, and layers id
     * @param aPlotDirectory = the output folder
     * @param aReporter = a REPORTER to return activity or any message (can be NULL)
     */
    void CreateMapFilesSet( const wxString& aPlotDirectory,
                            REPORTER* aReporter = NULL );

    /**
     * Function GenDrillReportFile
     *  Create a plain text report file giving a list of drill values and drill count
     *  for through holes, oblong holes, and for buried vias,
     *  drill values and drill count per layer pair
     *  there is only one report for all drill files even when buried or blinds vias exist
     *
     * Here is a sample created by this function:
     *  Drill report for F:/tmp/interf_u/interf_u.brd
     *  Created on 04/10/2012 20:48:38
     *  Selected Drill Unit: Imperial (inches)
     *
     *  Drill report for plated through holes :
     *  T1  0,025"  0,64mm  (88 holes)
     *  T2  0,031"  0,79mm  (120 holes)
     *  T3  0,032"  0,81mm  (151 holes)  (with 1 slot)
     *  T4  0,040"  1,02mm  (43 holes)
     *  T5  0,079"  2,00mm  (1 hole)  (with 1 slot)
     *  T6  0,120"  3,05mm  (1 hole)  (with 1 slot)
     *
     *  Total plated holes count 404
     *
     *
     *  Drill report for buried and blind vias :
     *
     *  Drill report for holes from layer Soudure to layer Interne1 :
     *
     *  Total plated holes count 0
     *
     *
     *  Drill report for holes from layer Interne1 to layer Interne2 :
     *  T1  0,025"  0,64mm  (3 holes)
     *
     *  Total plated holes count 3
     *
     *
     *  Drill report for holes from layer Interne2 to layer Composant :
     *  T1  0,025"  0,64mm  (1 hole)
     *
     *  Total plated holes count 1
     *
     *
     *  Drill report for unplated through holes :
     *  T1  0,120"  3,05mm  (1 hole)  (with 1 slot)
     *
     *  Total unplated holes count 1
     *
     * @param aFullFileName : the name of the file to create
     *
     * @return true if the file is created
     */
    bool GenDrillReportFile( const wxString& aFullFileName );

protected:
    /**
     * Function GenDrillMapFile
     * Plot a map of drill marks for holes.
     * Hole list must be created before calling this function, by buildHolesList()
     * for the right holes set (PTH, NPTH, buried/blind vias ...)
     * the paper sheet to use to plot the map is set in m_pageInfo
     * ( calls SetPageInfo() to set it )
     * if NULL, A4 format will be used
     * @param aFullFileName : the full filename of the map file to create,
     * @param aFormat : one of the supported plot formats (see enum PlotFormat )
     */
    bool genDrillMapFile( const wxString& aFullFileName, PlotFormat aFormat );

    /**
     * Function BuildHolesList
     * Create the list of holes and tools for a given board
     * The list is sorted by increasing drill size.
     * Only holes included within aLayerPair are listed.
     * If aLayerPair identifies with [F_Cu, B_Cu], then
     * pad holes are always included also.
     *
     * @param aLayerPair is an inclusive range of layers.
     * @param aGenerateNPTH_list :
     *       true to create NPTH only list (with no plated holes)
     *       false to created plated holes list (with no NPTH )
     */
    void buildHolesList( DRILL_LAYER_PAIR aLayerPair,
                         bool aGenerateNPTH_list );

    int  getHolesCount() const { return m_holeListBuffer.size(); }

    /** Helper function.
     * Writes the drill marks in HPGL, POSTSCRIPT or other supported formats
     * Each hole size has a symbol (circle, cross X, cross + ...) up to
     * PLOTTER::MARKER_COUNT different values.
     * If more than PLOTTER::MARKER_COUNT different values,
     * these other values share the same mark shape
     * @param aPlotter = a PLOTTER instance (HPGL, POSTSCRIPT ... plotter).
     */
    bool plotDrillMarks( PLOTTER* aPlotter );

    /// Get unique layer pairs by examining the micro and blind_buried vias.
    std::vector<DRILL_LAYER_PAIR> getUniqueLayerPairs() const;

    /**
     * Function printToolSummary
     * prints m_toolListBuffer[] tools to aOut and returns total hole count.
     * @param aOut = the current OUTPUTFORMATTER to print summary
     * @param aSummaryNPTH = true to print summary for NPTH, false for PTH
     */
    unsigned printToolSummary( OUTPUTFORMATTER& aOut, bool aSummaryNPTH ) const;

    /**
     * minor helper function.
     * @return a string from aPair to identify the layer layer pair.
     * string is "<layer1Name>"-"<layer2Name>"
     * used to generate a filename for drill files and drill maps
     */
    const std::string layerPairName( DRILL_LAYER_PAIR aPair ) const;

    /**
     * minor helper function.
     * @return a string from aLayer to identify the layer.
     * string are "front" "back" or "in<aLayer>"
     */
    const std::string layerName( PCB_LAYER_ID aLayer ) const;

    /**
     * @return a filename which identify the drill file function.
     * it is the board name with the layer pair names added, and for separate
     * (PTH and NPTH) files, "-NPH" or "-NPTH" added
     * @param aPair = the layer pair
     * @param aNPTH = true to generate the filename of NPTH holes
     * @param aMerge_PTH_NPTH = true to generate the filename of a file which containd both
     * NPH and NPTH holes
     */
    virtual const wxString getDrillFileName( DRILL_LAYER_PAIR aPair, bool aNPTH,
                                             bool aMerge_PTH_NPTH ) const;
};

#endif      // #define GENDRILL_FILE_WRITER_BASE_H

