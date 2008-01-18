/*************************************************************************/
/* Functions to create drill data used to create files and report  files */
/*************************************************************************/

#include "fctsys.h"

using namespace std;

#include <vector>

#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "macros.h"
#include "gendrill.h"


/* Local Functions */
/* Compare function used for sorting holes  by increasing diameter value 
 * and X value
*/
static bool CmpHoleDiameterValue( const HOLE_INFO& a, const HOLE_INFO& b )
{
	if ( a.m_Hole_Diameter != b.m_Hole_Diameter )
		return a.m_Hole_Diameter < b.m_Hole_Diameter;
	if ( a.m_Hole_Pos_X != b.m_Hole_Pos_X )
		return a.m_Hole_Pos_X < b.m_Hole_Pos_X;
	return a.m_Hole_Pos_Y < b.m_Hole_Pos_Y;
}


/**
 * Function BuildHolesList
 * Create the list of holes and tools for a given board
 * The list is sorted by incraesin drill values
 * Only holes from aFirstLayer to aLastLayer copper layers  are listed (for vias, because pad holes are always through holes)
 * @param Pcb : the given board
 * @param aHoleListBuffer : the std::vector<HOLE_INFO> to fill with pcb holes info
 * @param aToolListBuffer : the std::vector<DRILL_TOOL> to fill with tools to use
 * @param aFirstLayer = first layer to consider
 * @param aLastLayer = last layer to consider
 */
void Build_Holes_List( BOARD* aPcb,
                       std::vector<HOLE_INFO>& aHoleListBuffer,
                       std::vector<DRILL_TOOL>& aToolListBuffer,
                       int aFirstLayer, int aLastLayer )
{
    HOLE_INFO new_hole;
    int       hole_value;

    aHoleListBuffer.clear();
    aToolListBuffer.clear();

    if( aFirstLayer > aLastLayer )
        EXCHG( aFirstLayer, aLastLayer );

    /* build hole list for vias */
    TRACK*  track = aPcb->m_Track;
    for( ; track != NULL; track = track->Next() )
    {
        if( track->Type() != TYPEVIA )
            continue;
        SEGVIA* via = (SEGVIA*) track;
        hole_value = via->GetDrillValue();
        if( hole_value == 0 )
            continue;
        new_hole.m_Tool_Reference = -1;         // Flag value for Not initialized
        new_hole.m_Hole_Orient    = 0;
        new_hole.m_Hole_Diameter  = hole_value;
        new_hole.m_Hole_SizeX = new_hole.m_Hole_SizeY = new_hole.m_Hole_Diameter;
        new_hole.m_Hole_Shape = 0;                                                          // hole shape: round
        new_hole.m_Hole_Pos_X = via->m_Start.x;
        new_hole.m_Hole_Pos_Y = via->m_Start.y;                                             // hole position
        via->ReturnLayerPair( &new_hole.m_Hole_Top_Layer, &new_hole.m_Hole_Bottom_Layer );
		// ReturnLayerPair return params with m_Hole_Bottom_Layer < m_Hole_Top_Layer
        if( new_hole.m_Hole_Bottom_Layer > aFirstLayer )
            continue;
        if( new_hole.m_Hole_Top_Layer < aLastLayer )
            continue;
        aHoleListBuffer.push_back( new_hole );
    }

    /* build hole list for pads */
    MODULE* Module = aPcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        /* Read and analyse pads */
        D_PAD* pad = Module->m_Pads;
        for( ; pad != NULL; pad = pad->Next() )
        {
            if( pad->m_Drill.x == 0 )
                continue;
            new_hole.m_Tool_Reference = -1;         // Flag is: Not initialized
            new_hole.m_Hole_Orient    = pad->m_Orient;
            new_hole.m_Hole_Shape    = 0;           // hole shape: round
            new_hole.m_Hole_Diameter = min( pad->m_Drill.x, pad->m_Drill.x );
            new_hole.m_Hole_SizeX    = new_hole.m_Hole_SizeY = new_hole.m_Hole_Diameter;
            if( pad->m_DrillShape != PAD_CIRCLE )
                new_hole.m_Hole_Shape = 1; // oval flag set
            new_hole.m_Hole_SizeX = pad->m_Drill.x;
            new_hole.m_Hole_SizeY = pad->m_Drill.y;
            new_hole.m_Hole_Pos_X = pad->m_Pos.x;
            new_hole.m_Hole_Pos_Y = pad->m_Pos.y;               // hole position
            new_hole.m_Hole_Bottom_Layer = COPPER_LAYER_N;
            new_hole.m_Hole_Top_Layer  = LAYER_CMP_N;          // pad holes are through holes
            aHoleListBuffer.push_back( new_hole );
        }
    }

    // Sort holes per increasing diameter value
    sort( aHoleListBuffer.begin(), aHoleListBuffer.end(), CmpHoleDiameterValue );

    // build the tool list
    int        LastHole = -1; /* Set to not initialised
                               * (this is a value not used for aHoleListBuffer[ii].m_Hole_Diameter) */
    DRILL_TOOL new_tool( 0 );
    unsigned   jj;
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        if( aHoleListBuffer[ii].m_Hole_Diameter != LastHole )
        {
            new_tool.m_Diameter = ( aHoleListBuffer[ii].m_Hole_Diameter );
            aToolListBuffer.push_back( new_tool );
            LastHole = new_tool.m_Diameter;
        }

        jj = aToolListBuffer.size();
        if( jj == 0 )
            continue;                                       // Should not occurs

        aHoleListBuffer[ii].m_Tool_Reference = jj;          // Tool value Initialized (value >= 1)

        aToolListBuffer.back().m_TotalCount++;
        if( aHoleListBuffer[ii].m_Hole_Shape )
            aToolListBuffer.back().m_OvalCount++;
    }
}
