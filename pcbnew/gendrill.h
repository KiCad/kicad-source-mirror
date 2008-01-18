/*******************************************************************************/
/* classes and functions declaration unsed in drill file and report generation */
/*******************************************************************************/

#ifndef GENDRILL_H
#define GENDRILL_H

/* the DRILL_TOOL class  handle tools used in the excellon drill file */
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
 * @param Pcb : the given board
 * @param aHoleListBuffer : the std::vector<HOLE_INFO> to fill with pcb info
 * @param aToolListBuffer : the std::vector<DRILL_TOOL> to fill with tools to use
 */
void    Build_Holes_List( BOARD* Pcb, std::vector<HOLE_INFO>& aHoleListBuffer,
                          std::vector<DRILL_TOOL>& aToolListBuffer,
                          int aFirstLayer, int aLastLayer );


void    GenDrillMapFile( BOARD* aPcb,
                         FILE* aFile,
                         const wxString& aFullFileName,
                         wxSize aSheetSize,
                         std::vector<HOLE_INFO> aHoleListBuffer,
                         std::vector<DRILL_TOOL> aToolListBuffer,
                         bool aUnit_Drill_is_Inch,
                         int format );

void    Gen_Drill_PcbMap( BOARD* aPcb, FILE* aFile,
                          std::vector<HOLE_INFO>& aHoleListBuffer,
                          std::vector<DRILL_TOOL>& aToolListBuffer,
                          int format );

/*
 *  Create a list of drill values and drill count
 */
void GenDrillReportFile( FILE* aFile, const wxString& aBoardFilename,
                         std::vector<DRILL_TOOL>& aToolListBuffer, bool aUnit_Drill_is_Inch );

#endif  //	#ifndef GENDRILL_H
