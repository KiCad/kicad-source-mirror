/******************************************************************************/
/* classes and functions declaration used in drill file and report generation */
/******************************************************************************/
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean_Pierre Charras <jp.charras@ujf-grenoble.fr>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _GENDRILL_H_
#define _GENDRILL_H_

/* the DRILL_TOOL class  handles tools used in the excellon drill file */
class DRILL_TOOL
{
public:
    int m_Diameter;         // the diameter of the used tool (for oblong, the smaller size)
    int m_TotalCount;       // how many times it is used (round and oblong)
    int m_OvalCount;        // oblong count
public: DRILL_TOOL( int diametre )
    {
        m_TotalCount = 0;
        m_OvalCount  = 0;
        m_Diameter   = diametre;
    }
};


/* the HOLE_INFO class handle hole which must be drilled (diameter, position and layers) */
class HOLE_INFO
{
public:
    int m_Hole_Diameter;            // hole value, and for oblong min(hole size x, hole size y)
    int m_Tool_Reference;           // Tool reference for this hole = 1 ... n (values <=0 must not be used)
    int m_Hole_SizeX;               // hole size x for oblong holes
    int m_Hole_SizeY;               // hole size y for oblong holes
    int m_Hole_Orient;              // Hole rotation (= pad rotation) for oblong holes
    int m_Hole_Shape;               // hole shape: round (0) or oval (1)
    int m_Hole_Pos_X;               // hole position X
    int m_Hole_Pos_Y;               // hole position Y
    int m_Hole_Bottom_Layer;        // hole starting layer (usually copper)
    int m_Hole_Top_Layer;           // hole ending layer (usually component): m_Hole_First_Layer < m_Hole_Last_Layer
};


/* the DRILL_PRECISION helper class to handle drill precision format in excellon files
 */
class DRILL_PRECISION
{
public:
    int m_lhs;      // Left digit number (integer value of coordinates)
    int m_rhs;      // Right digit number (deciam value of coordinates)

public: DRILL_PRECISION( int l = 2, int r = 4 )
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


// A helper class to create Excellon drill files
class EXCELLON_WRITER
{
public:
    enum zeros_fmt {
        // Zero format in coordinates
        DECIMAL_FORMAT,
        SUPPRESS_LEADING,
        SUPPRESS_TRAILING,
        KEEP_ZEROS
    };
    wxPoint                  m_Offset;          // offset coordinates
    bool                     m_ShortHeader;     // true to generate the smallest header (strip comments)

private:
    FILE*                    m_file;                    // The output file
    BOARD*                   m_pcb;
    bool                     m_minimalHeader;           // True to use minimal haeder
                                                        // in excellon file (strip comments)
    bool                     m_unitsDecimal;            // true = decimal, false = inches
    zeros_fmt                m_zeroFormat;              // the zero format option for output file
    DRILL_PRECISION          m_precision;               // The current coordinate precision (not used in decimat format)
    double                   m_conversionUnits;         // scaling factor to convert the board unites to Excellon units
                                                        // (i.e inches or mm)
    bool                     m_mirror;
    wxPoint                  m_offset;                  // Drill offset ooordinates
    std::vector<HOLE_INFO>*  m_holeListBuffer;          // Buffer containing holes
    std::vector<DRILL_TOOL>* m_toolListBuffer;          // Buffer containing tools

public: EXCELLON_WRITER( BOARD* aPcb, FILE* aFile,
                         wxPoint aOffset,
                         std::vector<HOLE_INFO>* aHoleListBuffer,
                         std::vector<DRILL_TOOL>* aToolListBuffer )
    {
        m_file = aFile;
        m_pcb  = aPcb;
        m_zeroFormat      = DECIMAL_FORMAT;
        m_holeListBuffer  = aHoleListBuffer;
        m_toolListBuffer  = aToolListBuffer;
        m_conversionUnits = 0.0001;
        m_unitsDecimal    = false;
        m_mirror = false;
        m_minimalHeader = false;
    }


    ~EXCELLON_WRITER()
    {
    }


    /**
     * Function SetFormat
     * Initialize internal parameters to match the given format
     * @param aMetric = true for metric coordinates, false for imperial units
     * @param aZerosFmt =  DECIMAL_FORMAT, SUPPRESS_LEADING, SUPPRESS_TRAILING, KEEP_ZEROS
     * @param aLeftDigits = number of digits for integer part of coordinates
     * @param aRightDigits = number of digits for mantissa part of coordinates
     */
    void SetFormat( bool aMetric, zeros_fmt aZerosFmt, int aLeftDigits, int aRightDigits );

    /**
     * Function SetOptions
     * Initialize internal parameters to match drill options
     * @param aMirror = true to create mirrored coordinates (Y coordinates negated)
     * @param aMinimalHeader = true to use a minimal header (no comments, no info)
     * @param aOffset = drill coordinates offset
     */
    void SetOptions( bool aMirror, bool aMinimalHeader, wxPoint aOffset )
    {
        m_mirror = aMirror;
        m_offset = aOffset;
        m_minimalHeader = aMinimalHeader;
    }


    /**
     * Function CreateDrillFile
     * Creates an Excellon drill file
     * @return hole count
     */
    int  CreateDrillFile();

private:
    void WriteHeader();
    void WriteEndOfFile();
    void WriteCoordinates( char* aLine, double aCoordX, double aCoordY );
};

/**
 * Function BuildHolesList
 * Create the list of holes and tools for a given board
 * The list is sorted by increasing drill values
 * Only holes from aFirstLayer to aLastLayer copper layers  are listed (for vias, because pad holes are always through holes)
 * @param aPcb : the given board
 * @param aHoleListBuffer : the std::vector<HOLE_INFO> to fill with pcb holes info
 * @param aToolListBuffer : the std::vector<DRILL_TOOL> to fill with tools to use
 * @param aFirstLayer = first layer to consider. if < 0 aFirstLayer is ignored
 * @param aLastLayer = last layer to consider. if < 0 aLastLayer is ignored
 * @param aExcludeThroughHoles : if true, exclude through holes ( pads and vias through )
 */
void     Build_Holes_List( BOARD* aPcb, std::vector<HOLE_INFO>& aHoleListBuffer,
                           std::vector<DRILL_TOOL>& aToolListBuffer,
                           int aFirstLayer, int aLastLayer, bool aExcludeThroughHoles );


void     GenDrillMapFile( BOARD* aPcb,
                          FILE* aFile,
                          const wxString& aFullFileName,
                          Ki_PageDescr* aSheet,
                          std::vector<HOLE_INFO> aHoleListBuffer,
                          std::vector<DRILL_TOOL> aToolListBuffer,
                          bool aUnit_Drill_is_Inch,
                          int format, const wxPoint& auxoffset );

void Gen_Drill_PcbMap( BOARD* aPcb, PLOTTER* plotter,
                       std::vector<HOLE_INFO>& aHoleListBuffer,
                       std::vector<DRILL_TOOL>& aToolListBuffer );

/*
 *  Create a list of drill values and drill count
 *  there is only one report for all drill files even when buried or blinds vias exist
 */
void GenDrillReportFile( FILE* aFile, BOARD* aPcb, const wxString& aBoardFilename,
                         bool aUnit_Drill_is_Inch,
                         std::vector<HOLE_INFO>& aHoleListBuffer,
                         std::vector<DRILL_TOOL>& aToolListBuffer
                         );

#endif  //	#ifndef _GENDRILL_H_
