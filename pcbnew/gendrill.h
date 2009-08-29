/******************************************************************************/
/* classes and functions declaration used in drill file and report generation */
/******************************************************************************/

#ifndef GENDRILL_H
#define GENDRILL_H

/* the DRILL_TOOL class  handles tools used in the excellon drill file */
class DRILL_TOOL
{
public:
    int m_Diameter;			// the diameter of the used tool (for oblong, the smaller size)
    int m_TotalCount;		// how many times it is used (round and oblong)
    int m_OvalCount;		// oblong count
public:
    DRILL_TOOL( int diametre )
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
    int m_Hole_Pos_X;				// hole position X
    int m_Hole_Pos_Y;               // hole position Y
    int m_Hole_Bottom_Layer;        // hole starting layer (usually copper)
    int m_Hole_Top_Layer;           // hole ending layer (usually component): m_Hole_First_Layer < m_Hole_Last_Layer
};


/* the DrillPrecision class to handle drill precision format in excellon files*/
class DrillPrecision
{
public:
    int m_lhs;
    int m_rhs;

public:
    DrillPrecision( int l, int r ) { m_lhs = l; m_rhs = r; }
};

/* zeros format */
enum zeros_fmt {
    DECIMAL_FORMAT,
    SUPPRESS_LEADING,
    SUPPRESS_TRAILING,
    KEEP_ZEROS
};


/**
 * Function BuildHolesList
 * Create the list of holes and tools for a given board
 * The list is sorted by incraesin drill values
 * Only holes from aFirstLayer to aLastLayer copper layers  are listed (for vias, because pad holes are always through holes)
 * @param Pcb : the given board
 * @param aHoleListBuffer : the std::vector<HOLE_INFO> to fill with pcb holes info
 * @param aToolListBuffer : the std::vector<DRILL_TOOL> to fill with tools to use
 * @param aFirstLayer = first layer to consider. if < 0 aFirstLayer is ignored
 * @param aLastLayer = last layer to consider. if < 0 aLastLayer is ignored
 * @param aExcludeThroughHoles : if true, exclude through holes ( pads and vias through )
 */
void    Build_Holes_List( BOARD* Pcb, std::vector<HOLE_INFO>& aHoleListBuffer,
                          std::vector<DRILL_TOOL>& aToolListBuffer,
                          int aFirstLayer, int aLastLayer, bool aExcludeThroughHoles );


void    GenDrillMapFile( BOARD* aPcb,
                         FILE* aFile,
                         const wxString& aFullFileName,
                         Ki_PageDescr *aSheet,
                         std::vector<HOLE_INFO> aHoleListBuffer,
                         std::vector<DRILL_TOOL> aToolListBuffer,
                         bool aUnit_Drill_is_Inch,
                         int format, const wxPoint& auxoffset );

void    Gen_Drill_PcbMap( BOARD* aPcb, PLOTTER* plotter,
                          std::vector<HOLE_INFO>& aHoleListBuffer,
                          std::vector<DRILL_TOOL>& aToolListBuffer);

/*
 *  Create a list of drill values and drill count
 *  there is only one report for all drill files even when buried or blinds vias exist
 */
void GenDrillReportFile( FILE* aFile, BOARD * aPcb, const wxString& aBoardFilename,
                         bool aUnit_Drill_is_Inch,
						std::vector<HOLE_INFO> & aHoleListBuffer,
						std::vector<DRILL_TOOL>& aToolListBuffer
						);

#endif  //	#ifndef GENDRILL_H
