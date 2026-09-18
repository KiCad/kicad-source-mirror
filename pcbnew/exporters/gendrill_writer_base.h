/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file gendrill_writer_base.h
 * @brief helper classes to handle hole info for drill files generators.
 */
#ifndef GENDRILL_WRITER_BASE_H
#define GENDRILL_WRITER_BASE_H

// holes can have an attribute in Excellon drill files, similar to attributes
// in Gerber X2 format
// They are only comments for a better identification of holes (vias, pads...)
// Set to 1 to add these comments and 0 to not use these comments
#define USE_ATTRIB_FOR_HOLES 1

#include <optional>
#include <string>
#include <vector>

#include <drill/drill_operation.h>
#include <layer_ids.h>
#include <plotters/plotter.h>
#include <padstack.h>


class BOARD;
class BOARD_ITEM;
class OUTPUTFORMATTER;
class PAGE_INFO;
class REPORTER;


// Via Protection features according to IPC-4761.
enum class IPC4761_FEATURES : int
{
    FILLED,
    CAPPED,
    PLUGGED_FRONT,
    PLUGGED_BACK,
    COVERED_FRONT,
    COVERED_BACK,
    TENTED_FRONT,
    TENTED_BACK
};

// the DRILL_TOOL class  handles tools used in the excellon drill file:
class DRILL_TOOL
{
public:
    int m_Diameter;                 // the diameter of the used tool
                                    // (for oblong, the smaller size)
    int m_TotalCount;               // how many times it is used (round and oblong)
    int m_OvalCount;                // oblong count
    bool m_Hole_NotPlated;          // Is the hole plated or not plated
    HOLE_ATTRIBUTE m_HoleAttribute; // Attribute (used in Excellon drill file)
    bool m_IsBackdrill;             // True when drilling a backdrill span
    bool m_HasPostMachining;        // True if any hole for this tool has post-machining
    std::optional<int> m_MinStubLength; // Minimum stub length for this tool (IU)
    std::optional<int> m_MaxStubLength; // Maximum stub length for this tool (IU)

public:
    DRILL_TOOL( int aDiameter, bool a_NotPlated )
    {
        m_TotalCount     = 0;
        m_OvalCount      = 0;
        m_Diameter       = aDiameter;
        m_Hole_NotPlated = a_NotPlated;
        m_HoleAttribute  = HOLE_ATTRIBUTE::HOLE_UNKNOWN;
        m_IsBackdrill    = false;
        m_HasPostMachining = false;
    }
};


/**
 * Helper to handle drill precision format in excellon files.
 */
class DRILL_PRECISION
{
public:
    DRILL_PRECISION( int l = 2, int r = 4 )
    {
        m_Lhs = l; m_Rhs = r;
    }


    wxString GetPrecisionString()
    {
        wxString text;

        text << m_Lhs << wxT( ":" ) << m_Rhs;
        return text;
    }

    int m_Lhs;      // Left digit number (integer value of coordinates)
    int m_Rhs;      // Right digit number (decimal value of coordinates)
};


/**
 * Create drill maps and drill reports and drill files.
 *
 * Drill files are created by specialized derived classes, depending on the file format.
 */
class GENDRILL_WRITER_BASE
{
public:
    enum ZEROS_FMT {            // Zero format in coordinates
        DECIMAL_FORMAT,         // Floating point coordinates
        SUPPRESS_LEADING,       // Suppress leading zeros
        SUPPRESS_TRAILING,      // Suppress trailing zeros
        KEEP_ZEROS              // keep zeros
    };

    enum TYPE_FILE {            // type of holes in file: PTH, NPTH, mixed
        PTH_FILE,               // PTH only, this is the default also for blind/buried holes
        NPTH_FILE,              // NPTH only
        MIXED_FILE              // PHT+NPTH (mixed)
    };

    virtual ~GENDRILL_WRITER_BASE()
    {
    }

    /**
     * Set the option to make separate drill files for PTH and NPTH.
     *
     * @param aMerge set to true to make only one file containing PTH and NPTH or false to
     *               create 2 separate files.
     */
    void SetMergeOption( bool aMerge ) { m_merge_PTH_NPTH = aMerge; }

    /**
     * Return the plot offset (usually the position of the drill/place origin).
     */
    VECTOR2I GetOffset() { return m_offset; }

    /**
     * Set the page info used to plot drill maps.
     *
     * If NULL, a A4 page format will be used.
     *
     * @param aPageInfo is a reference to the page info, usually used to plot/display the board.
     */
    void SetPageInfo( const PAGE_INFO* aPageInfo ) { m_pageInfo = aPageInfo; }

    /**
     * Initialize the format for the drill map file.
     *
     * @param aMapFmt a PlotFormat value (one of PLOT_FORMAT_POST, PLOT_FORMAT_GERBER, PLOT_FORMAT_DXF,
     *                PLOT_FORMAT_SVG, PLOT_FORMAT_PDF
     *                the most useful are PLOT_FORMAT_PDF and PLOT_FORMAT_POST.
     */
    void SetMapFileFormat( PLOT_FORMAT aMapFmt )
    {
        m_mapFileFmt = aMapFmt;
    }

    /**
     * Create the full set of map files for the board, in PS, PDF ... format
     * (use SetMapFileFormat() to select the format).
     *
     * File names are computed from the board name and layer ID.
     *
     * @param aPlotDirectory is the output folder.
     * @param aReporter is a REPORTER to return activity or any message (can be NULL)
     */
    bool CreateMapFilesSet( const wxString& aPlotDirectory, REPORTER* aReporter = nullptr );

    /**
     * Create a plain text report file giving a list of drill values and drill count for through
     * holes, oblong holes, and for buried vias, drill values and drill count per layer pair
     * there is only one report for all drill files even when buried or blinds vias exist.
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
     * @param aFullFileName is the name of the file to create.
     * @return true if the file is created.
     */
    bool GenDrillReportFile( const wxString& aFullFileName, REPORTER* aReporter = nullptr );

    /**
     * Returns the file extension of the drill writer format
     */
    wxString GetDrillFileExt() const { return m_drillFileExtension; }

    const std::vector<wxString>& GetCreatedFiles() const { return m_createdFiles; }

protected:
    /**
     * Plot a map of drill marks for holes.
     *
     * Hole list must be created before calling this function, by buildHolesList() for the
     * right holes set (PTH, NPTH, buried/blind vias ...) the paper sheet to use to plot the
     * map is set in m_pageInfo ( calls SetPageInfo() to set it ).  If NULL, A4 format will
     * be used.
     *
     * @param aFullFileName is the full filename of the map file to create.
     * @param aFormat is one of the supported plot formats (see enum PlotFormat ).
     */
    bool genDrillMapFile( const wxString& aFullFileName, PLOT_FORMAT aFormat );

    /**
     * Create the list of holes and tools for a given board.
     *
     * The list is sorted by increasing drill size.   Only holes included within aLayerPair
     * are listed.  If aLayerPair identifies with [F_Cu, B_Cu], then pad holes are always
     * included also.
     *
     * @param aLayerPair is an inclusive range of layers.
     * @param aGenerateNPTH_list :
     *       true to create NPTH only list (with no plated holes)
     *       false to created plated holes list (with no NPTH )
     */
    void buildHolesList( const DRILL_SPAN& aSpan, bool aGenerateNPTH_list );

    int  getHolesCount() const { return m_holeListBuffer.size(); }

    /**
     * Write the drill marks in PDF, POSTSCRIPT or other supported formats/
     *
     * Each hole size has a symbol (circle, cross X, cross + ...) up to PLOTTER::MARKER_COUNT
     * different values.  If more than PLOTTER::MARKER_COUNT different values, these other
     * values share the same mark shape.
     *
     * @param aPlotter is a PLOTTER instance (PDF, POSTSCRIPT ... plotter).
     */
    bool plotDrillMarks( PLOTTER* aPlotter );

    /// Get unique layer pairs by examining the micro and blind_buried vias.
    std::vector<DRILL_SPAN> getUniqueLayerPairs() const;

    /// Selects which subset of m_toolListBuffer a summary covers.
    enum class TOOL_SUMMARY
    {
        PLATED,     ///< Plated holes.
        UNPLATED,   ///< Non-plated holes, excluding backdrills.
        BACKDRILL   ///< Backdrills, which are non-plated by construction.
    };

    /**
     * Print m_toolListBuffer[] tools to aOut and returns total hole count.
     *
     * @param aOut is the current OUTPUTFORMATTER to print summary.
     * @param aSummary selects which tools the summary covers.
     */
    unsigned printToolSummary( FILE* out, TOOL_SUMMARY aSummary ) const;

    /**
     * @return a string from aPair to identify the layer layer pair.
     * string is "<layer1Name>"-"<layer2Name>"
     * used to generate a filename for drill files and drill maps
     */
    const std::string layerPairName( DRILL_LAYER_PAIR aPair ) const;

    /**
     * @return a string from aLayer to identify the layer.
     * string are "front" "back" or "in<aLayer>"
     */
    const std::string layerName( PCB_LAYER_ID aLayer ) const;

    /**
     * @param aPair is the layer pair.
     * @param aNPTH use true to generate the filename of NPTH holes.
     * @param aMerge_PTH_NPTH use true to generate the filename of a file which containd both
     *                        NPH and NPTH holes.
     * @return a filename which identify the drill file function.
     * it is the board name with the layer pair names added, and for separate
     * (PTH and NPTH) files, "-NPH" or "-NPTH" added
     */
    virtual const wxString getDrillFileName( const DRILL_SPAN& aSpan, bool aNPTH,
                                             bool aMerge_PTH_NPTH ) const;


    /**
     * @param aPair is the layer pair.
     * @param aFeature Is the protection feature represented by the file
     * @return a filename which identifies the specific protection feature.
     * It is the board file name followed by the feature name and the layer(s) associated with it.
     */
    virtual const wxString getProtectionFileName( const DRILL_SPAN& aSpan,
                                                  IPC4761_FEATURES aFeature ) const;


    /**
     * @param aLayerPair is the layer pair (Drill from rom first layer to second layer)
     * @param aHoleType is type of drill file (PTH, NPTH, mixed)
     * @param aCompatNCdrill is true when generating NC (Excellon) compatible drill file
     * @return a wxString containing the .FileFunction attribute.
     * the standard X2 FileFunction for drill files is
     * %TF.FileFunction,Plated[NonPlated],layer1num,layer2num,PTH[NPTH][Blind][Buried],
     * Drill[Route][Mixed]*%
     * There is no X1 version, as the Gerber drill files uses only X2 format
     * There is a compatible NC drill version.
     */
    const wxString BuildFileFunctionAttributeString( const DRILL_SPAN& aSpan,
                                                     TYPE_FILE aHoleType,
                                                     bool aCompatNCdrill = false ) const;


protected:
    // Use derived classes to build a fully initialized GENDRILL_WRITER_BASE class.
    GENDRILL_WRITER_BASE( BOARD* aPcb )
    {
        m_pcb             = aPcb;
        m_conversionUnits = 1.0;
        m_unitsMetric     = true;
        m_mapFileFmt      = PLOT_FORMAT::PDF;
        m_pageInfo        = nullptr;
        m_merge_PTH_NPTH  = false;
        m_zeroFormat      = DECIMAL_FORMAT;
    }

    void AddCreatedFile( const wxString& aPath ) { m_createdFiles.emplace_back( aPath ); }

    BOARD*                   m_pcb;
    wxString                 m_drillFileExtension;      // .drl or .gbr, depending on format
    bool                     m_unitsMetric;             // true = mm, false = inches
    ZEROS_FMT                m_zeroFormat;              // the zero format option for output file
    DRILL_PRECISION          m_precision;               // The current coordinate precision (not
                                                        // used in decimal format).
    double                   m_conversionUnits;         // scaling factor to convert the board
                                                        // unites to Excellon/Gerber units (i.e
                                                        // inches or mm)
    VECTOR2I                 m_offset;                  // Drill offset coordinates
    bool                     m_merge_PTH_NPTH;          // True to generate only one drill file
    std::vector<DRILL_OPERATION> m_holeListBuffer;

    // One-based tool numbers, indexed by the sorted operation list.
    std::vector<int>         m_holeToolReferences;
    std::vector<DRILL_TOOL>  m_toolListBuffer;          // Buffer containing tools

    PLOT_FORMAT m_mapFileFmt;                           // the format of the map drill file,
                                                        // if this map is needed
    const PAGE_INFO*         m_pageInfo;                // the page info used to plot drill maps
                                                        // If NULL, use a A4 page format
    std::vector<wxString>    m_createdFiles;
};

#endif      // #define GENDRILL_FILE_WRITER_BASE_H
