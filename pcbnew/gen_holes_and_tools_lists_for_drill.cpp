/*************************************************************************/
/* Functions to create drill data used to create files and report  files */
/*************************************************************************/

#include <algorithm> // sort

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <gendrill.h>


// Local Functions

/* Compare function used for sorting holes  by increasing diameter value
 * and X value
 */
static bool CmpHoleDiameterValue( const HOLE_INFO& a, const HOLE_INFO& b )
{
    if( a.m_Hole_Diameter != b.m_Hole_Diameter )
        return a.m_Hole_Diameter < b.m_Hole_Diameter;

    if( a.m_Hole_Pos.x != b.m_Hole_Pos.x )
        return a.m_Hole_Pos.x < b.m_Hole_Pos.x;

    return a.m_Hole_Pos.y < b.m_Hole_Pos.y;
}


/*
 * Function BuildHolesList
 * Create the list of holes and tools for a given board
 * The list is sorted by increasing drill values
 * Only holes from aFirstLayer to aLastLayer copper layers  are listed (for vias, because pad holes are always through holes)
 * param aPcb : the given board
 * param aHoleListBuffer : the std::vector<HOLE_INFO> to fill with pcb holes info
 * param aToolListBuffer : the std::vector<DRILL_TOOL> to fill with tools to use
 * param aFirstLayer = first layer to consider. if < 0 aFirstLayer is ignored   (used to creates report file)
 * param aLastLayer = last layer to consider. if < 0 aLastLayer is ignored
 * param aExcludeThroughHoles : if true, exclude through holes ( pads and vias through )
 * param aGenerateNPTH_list :
 *       true to create NPTH only list (with no plated holes)
 *       false to created plated holes list (with no NPTH )
 */
void Build_Holes_List( BOARD* aPcb,
                       std::vector<HOLE_INFO>& aHoleListBuffer,
                       std::vector<DRILL_TOOL>& aToolListBuffer,
                       int aFirstLayer, int aLastLayer, bool aExcludeThroughHoles,
                       bool aGenerateNPTH_list )
{
    HOLE_INFO new_hole;
    int       hole_value;

    aHoleListBuffer.clear();
    aToolListBuffer.clear();

    if( (aFirstLayer >= 0) && (aLastLayer >= 0) )
    {
        if( aFirstLayer > aLastLayer )
            EXCHG( aFirstLayer, aLastLayer );
    }

    /* build hole list for vias
    */
    if( ! aGenerateNPTH_list )  // vias are always plated !
    {
        for( TRACK* track = aPcb->m_Track;  track;  track = track->Next() )
        {
            if( track->Type() != PCB_VIA_T )
                continue;

            SEGVIA* via = (SEGVIA*) track;
            hole_value = via->GetDrillValue();

            if( hole_value == 0 )
                continue;

            new_hole.m_Tool_Reference = -1;         // Flag value for Not initialized
            new_hole.m_Hole_Orient    = 0;
            new_hole.m_Hole_Diameter  = hole_value;
            new_hole.m_Hole_Size.x = new_hole.m_Hole_Size.y = new_hole.m_Hole_Diameter;

            new_hole.m_Hole_Shape = 0;              // hole shape: round
            new_hole.m_Hole_Pos = via->m_Start;
            via->ReturnLayerPair( &new_hole.m_Hole_Top_Layer, &new_hole.m_Hole_Bottom_Layer );

            // ReturnLayerPair return params with m_Hole_Bottom_Layer < m_Hole_Top_Layer
            if( (new_hole.m_Hole_Bottom_Layer > aFirstLayer) && (aFirstLayer >= 0) )
                continue;

            if( (new_hole.m_Hole_Top_Layer < aLastLayer) && (aLastLayer >= 0) )
                continue;

            if( aExcludeThroughHoles  && (new_hole.m_Hole_Bottom_Layer == LAYER_N_BACK)
               && (new_hole.m_Hole_Top_Layer == LAYER_N_FRONT) )
                continue;

            aHoleListBuffer.push_back( new_hole );
        }
    }

    // build hole list for pads (assumed always through holes)
    if( !aExcludeThroughHoles || aGenerateNPTH_list )
    {
        for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
        {
            // Read and analyse pads
            for( D_PAD* pad = module->m_Pads;  pad;  pad = pad->Next() )
            {
                if( ! aGenerateNPTH_list && pad->GetAttribute() == PAD_HOLE_NOT_PLATED )
                    continue;

                if( aGenerateNPTH_list && pad->GetAttribute() != PAD_HOLE_NOT_PLATED )
                    continue;

                if( pad->GetDrillSize().x == 0 )
                    continue;

                new_hole.m_Hole_NotPlated = (pad->GetAttribute() == PAD_HOLE_NOT_PLATED);
                new_hole.m_Tool_Reference = -1;         // Flag is: Not initialized
                new_hole.m_Hole_Orient    = pad->GetOrientation();
                new_hole.m_Hole_Shape    = 0;           // hole shape: round
                new_hole.m_Hole_Diameter = min( pad->GetDrillSize().x, pad->GetDrillSize().y );
                new_hole.m_Hole_Size.x    = new_hole.m_Hole_Size.y = new_hole.m_Hole_Diameter;

                if( pad->GetDrillShape() != PAD_CIRCLE )
                    new_hole.m_Hole_Shape = 1; // oval flag set

                new_hole.m_Hole_Size = pad->GetDrillSize();
                new_hole.m_Hole_Pos = pad->GetPosition();               // hole position
                new_hole.m_Hole_Bottom_Layer = LAYER_N_BACK;
                new_hole.m_Hole_Top_Layer    = LAYER_N_FRONT;// pad holes are through holes
                aHoleListBuffer.push_back( new_hole );
            }
        }
    }

    // Sort holes per increasing diameter value
    sort( aHoleListBuffer.begin(), aHoleListBuffer.end(), CmpHoleDiameterValue );

    // build the tool list
    int        LastHole = -1; /* Set to not initialised (this is a value not used
                               * for aHoleListBuffer[ii].m_Hole_Diameter) */
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
