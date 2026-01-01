/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <collectors.h>
#include <macros.h>
#include <reporter.h>
#include <string_utils.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotter_gerber.h>
#include <plotters/plotters_pslike.h>
#include <pcbplot.h>
#include <pcb_painter.h>
#include <pcb_shape.h>
#include <fmt.h>
#include <wx/ffile.h>
#include <reporter.h>

#include <set>

#include <gendrill_writer_base.h>


/* Helper function for sorting hole list.
 * Compare function used for sorting holes type type:
 * plated then not plated
 * then by increasing diameter value
 * then by attribute type (vias, pad, mechanical)
 * then by X then Y position
 */
static bool cmpHoleSorting( const HOLE_INFO& a, const HOLE_INFO& b )
{
    if( a.m_Hole_NotPlated != b.m_Hole_NotPlated )
        return b.m_Hole_NotPlated;

    if( a.m_Hole_Diameter != b.m_Hole_Diameter )
        return a.m_Hole_Diameter < b.m_Hole_Diameter;

    // At this point (same diameter, same plated type), group by attribute
    // type (via, pad, mechanical, although currently only not plated pads are mechanical)
    if( a.m_HoleAttribute != b.m_HoleAttribute )
        return a.m_HoleAttribute < b.m_HoleAttribute;

    // At this point (same diameter, same type), sort by X then Y position.
    // This is optimal for drilling and make the file reproducible as long as holes
    // have not changed, even if the data order has changed.
    if( a.m_Hole_Pos.x != b.m_Hole_Pos.x )
        return a.m_Hole_Pos.x < b.m_Hole_Pos.x;

    return a.m_Hole_Pos.y < b.m_Hole_Pos.y;
}


void GENDRILL_WRITER_BASE::buildHolesList( const DRILL_SPAN& aSpan, bool aGenerateNPTH_list )
{
    HOLE_INFO new_hole;

    m_holeListBuffer.clear();
    m_toolListBuffer.clear();

    wxASSERT( aSpan.TopLayer() < aSpan.BottomLayer() );  // fix the caller

    auto computeStubLength = [&]( PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aEndLayer )
    {
        if( aStartLayer == UNDEFINED_LAYER || aEndLayer == UNDEFINED_LAYER )
            return std::optional<int>();

        BOARD_STACKUP& stackup = m_pcb->GetDesignSettings().GetStackupDescriptor();
        return std::optional<int>( stackup.GetLayerDistance( aStartLayer, aEndLayer ) );
    };

    if( !aGenerateNPTH_list )
    {
        for( PCB_TRACK* track : m_pcb->Tracks() )
        {
            if( track->Type() != PCB_VIA_T )
                continue;

            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            if( aSpan.m_IsBackdrill )
            {
                const PADSTACK::DRILL_PROPS& secondary = via->Padstack().SecondaryDrill();

                if( secondary.start == UNDEFINED_LAYER || secondary.end == UNDEFINED_LAYER )
                    continue;

                DRILL_LAYER_PAIR secondaryPair( std::min( secondary.start, secondary.end ),
                                                std::max( secondary.start, secondary.end ) );

                if( secondaryPair != aSpan.Pair() )
                    continue;

                if( secondary.start != aSpan.DrillStartLayer()
                        || secondary.end != aSpan.DrillEndLayer() )
                {
                    continue;
                }

                if( secondary.size.x <= 0 && secondary.size.y <= 0 )
                    continue;

                new_hole = HOLE_INFO();
                new_hole.m_ItemParent = via;
                new_hole.m_HoleAttribute = HOLE_ATTRIBUTE::HOLE_VIA_BACKDRILL;
                new_hole.m_Tool_Reference = -1;
                new_hole.m_Hole_Orient = ANGLE_0;
                new_hole.m_Hole_NotPlated = true;
                new_hole.m_Hole_Shape = 0;
                new_hole.m_Hole_Pos = via->GetStart();
                new_hole.m_Hole_Top_Layer = aSpan.TopLayer();
                new_hole.m_Hole_Bottom_Layer = aSpan.BottomLayer();

                int diameter = secondary.size.x;

                if( secondary.size.y > 0 )
                    diameter = ( diameter > 0 ) ? std::min( diameter, secondary.size.y )
                                                : secondary.size.y;

                new_hole.m_Hole_Diameter = diameter;
                new_hole.m_Hole_Size = secondary.size;

                if( secondary.shape != PAD_DRILL_SHAPE::CIRCLE
                        && secondary.size.x != secondary.size.y )
                {
                    new_hole.m_Hole_Shape = 1;
                }

                new_hole.m_Hole_Filled = secondary.is_filled.value_or( false );
                new_hole.m_Hole_Capped = secondary.is_capped.value_or( false );
                new_hole.m_Hole_Top_Covered = via->Padstack().IsCovered( new_hole.m_Hole_Top_Layer )
                                                    .value_or( false );
                new_hole.m_Hole_Bot_Covered = via->Padstack().IsCovered( new_hole.m_Hole_Bottom_Layer )
                                                    .value_or( false );
                new_hole.m_Hole_Top_Plugged = via->Padstack().IsPlugged( new_hole.m_Hole_Top_Layer )
                                                    .value_or( false );
                new_hole.m_Hole_Bot_Plugged = via->Padstack().IsPlugged( new_hole.m_Hole_Bottom_Layer )
                                                    .value_or( false );
                new_hole.m_Hole_Top_Tented = via->Padstack().IsTented( new_hole.m_Hole_Top_Layer )
                                                    .value_or( false );
                new_hole.m_Hole_Bot_Tented = via->Padstack().IsTented( new_hole.m_Hole_Bottom_Layer )
                                                    .value_or( false );
                new_hole.m_IsBackdrill = true;
                new_hole.m_FrontPostMachining = PAD_DRILL_POST_MACHINING_MODE::UNKNOWN;
                new_hole.m_FrontPostMachiningSize = 0;
                new_hole.m_FrontPostMachiningDepth = 0;
                new_hole.m_FrontPostMachiningAngle = 0;
                new_hole.m_BackPostMachining = PAD_DRILL_POST_MACHINING_MODE::UNKNOWN;
                new_hole.m_BackPostMachiningSize = 0;
                new_hole.m_BackPostMachiningDepth = 0;
                new_hole.m_BackPostMachiningAngle = 0;
                new_hole.m_DrillStart = secondary.start;
                new_hole.m_DrillEnd = secondary.end;
                new_hole.m_StubLength = computeStubLength( secondary.start, secondary.end );

                m_holeListBuffer.push_back( new_hole );
                continue;
            }

            int hole_sz = via->GetDrillValue();

            if( hole_sz == 0 )
                continue;

            PCB_LAYER_ID top_layer;
            PCB_LAYER_ID bottom_layer;
            via->LayerPair( &top_layer, &bottom_layer );

            if( DRILL_LAYER_PAIR( top_layer, bottom_layer ) != aSpan.Pair() )
                continue;

            new_hole = HOLE_INFO();
            new_hole.m_ItemParent = via;

            if( aSpan.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
                new_hole.m_HoleAttribute = HOLE_ATTRIBUTE::HOLE_VIA_THROUGH;
            else
                new_hole.m_HoleAttribute = HOLE_ATTRIBUTE::HOLE_VIA_BURIED;

            new_hole.m_Tool_Reference = -1;
            new_hole.m_Hole_Orient = ANGLE_0;
            new_hole.m_Hole_Diameter = hole_sz;
            new_hole.m_Hole_NotPlated = false;
            new_hole.m_Hole_Size.x = new_hole.m_Hole_Size.y = new_hole.m_Hole_Diameter;
            new_hole.m_Hole_Shape = 0;
            new_hole.m_Hole_Pos = via->GetStart();
            new_hole.m_Hole_Top_Layer = top_layer;
            new_hole.m_Hole_Bottom_Layer = bottom_layer;
            new_hole.m_Hole_Filled = via->Padstack().IsFilled().value_or( false );
            new_hole.m_Hole_Capped = via->Padstack().IsCapped().value_or( false );
            new_hole.m_Hole_Top_Covered = via->Padstack().IsCovered( top_layer ).value_or( false );
            new_hole.m_Hole_Bot_Covered = via->Padstack().IsCovered( bottom_layer ).value_or( false );
            new_hole.m_Hole_Top_Plugged = via->Padstack().IsPlugged( top_layer ).value_or( false );
            new_hole.m_Hole_Bot_Plugged = via->Padstack().IsPlugged( bottom_layer ).value_or( false );
            new_hole.m_Hole_Top_Tented = via->Padstack().IsTented( top_layer ).value_or( false );
            new_hole.m_Hole_Bot_Tented = via->Padstack().IsTented( bottom_layer ).value_or( false );
            new_hole.m_IsBackdrill = false;
            new_hole.m_FrontPostMachining = via->Padstack().FrontPostMachining().mode.value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN );
            new_hole.m_FrontPostMachiningSize = via->Padstack().FrontPostMachining().size;
            new_hole.m_FrontPostMachiningDepth = via->Padstack().FrontPostMachining().depth;
            new_hole.m_FrontPostMachiningAngle = via->Padstack().FrontPostMachining().angle;
            new_hole.m_BackPostMachining = via->Padstack().BackPostMachining().mode.value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN );
            new_hole.m_BackPostMachiningSize = via->Padstack().BackPostMachining().size;
            new_hole.m_BackPostMachiningDepth = via->Padstack().BackPostMachining().depth;
            new_hole.m_BackPostMachiningAngle = via->Padstack().BackPostMachining().angle;
            new_hole.m_DrillStart = via->Padstack().Drill().start;
            new_hole.m_DrillEnd = bottom_layer;

            m_holeListBuffer.push_back( new_hole );
        }
    }

    if( !aSpan.m_IsBackdrill && aSpan.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
    {
        for( FOOTPRINT* footprint : m_pcb->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( !m_merge_PTH_NPTH )
                {
                    if( !aGenerateNPTH_list && pad->GetAttribute() == PAD_ATTRIB::NPTH )
                        continue;

                    if( aGenerateNPTH_list && pad->GetAttribute() != PAD_ATTRIB::NPTH )
                        continue;
                }

                if( pad->GetDrillSize().x == 0 )
                    continue;

                new_hole = HOLE_INFO();
                new_hole.m_ItemParent = pad;
                new_hole.m_Hole_NotPlated = ( pad->GetAttribute() == PAD_ATTRIB::NPTH );

                if( new_hole.m_Hole_NotPlated )
                    new_hole.m_HoleAttribute = HOLE_ATTRIBUTE::HOLE_MECHANICAL;
                else
                {
                    if( pad->GetProperty() == PAD_PROP::CASTELLATED )
                        new_hole.m_HoleAttribute = HOLE_ATTRIBUTE::HOLE_PAD_CASTELLATED;
                    else if( pad->GetProperty() == PAD_PROP::PRESSFIT )
                        new_hole.m_HoleAttribute = HOLE_ATTRIBUTE::HOLE_PAD_PRESSFIT;
                    else
                        new_hole.m_HoleAttribute = HOLE_ATTRIBUTE::HOLE_PAD;
                }

                new_hole.m_Tool_Reference = -1;
                new_hole.m_Hole_Orient = pad->GetOrientation();
                new_hole.m_Hole_Shape = 0;
                new_hole.m_Hole_Diameter = std::min( pad->GetDrillSize().x, pad->GetDrillSize().y );
                new_hole.m_Hole_Size.x = new_hole.m_Hole_Size.y = new_hole.m_Hole_Diameter;

                if( pad->GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE
                        && pad->GetDrillSizeX() != pad->GetDrillSizeY() )
                {
                    new_hole.m_Hole_Shape = 1;
                }

                new_hole.m_Hole_Size = pad->GetDrillSize();
                new_hole.m_Hole_Pos = pad->GetPosition();
                new_hole.m_Hole_Bottom_Layer = B_Cu;
                new_hole.m_Hole_Top_Layer = F_Cu;
                m_holeListBuffer.push_back( new_hole );
            }
        }
    }

    // Sort holes per increasing diameter value (and for each dimater, by position)
    sort( m_holeListBuffer.begin(), m_holeListBuffer.end(), cmpHoleSorting );

    // build the tool list
    int last_hole = -1;     // Set to not initialized (this is a value not used
                            // for m_holeListBuffer[ii].m_Hole_Diameter)
    bool last_notplated_opt = false;
    HOLE_ATTRIBUTE last_attribute = HOLE_ATTRIBUTE::HOLE_UNKNOWN;

    DRILL_TOOL new_tool( 0, false );
    unsigned   jj;

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        if( m_holeListBuffer[ii].m_Hole_Diameter != last_hole
            || m_holeListBuffer[ii].m_Hole_NotPlated != last_notplated_opt
#if USE_ATTRIB_FOR_HOLES
            || m_holeListBuffer[ii].m_HoleAttribute != last_attribute
#endif
            )
        {
            new_tool.m_Diameter = m_holeListBuffer[ii].m_Hole_Diameter;
            new_tool.m_Hole_NotPlated = m_holeListBuffer[ii].m_Hole_NotPlated;
            new_tool.m_HoleAttribute = m_holeListBuffer[ii].m_HoleAttribute;
            m_toolListBuffer.push_back( new_tool );
            last_hole = new_tool.m_Diameter;
            last_notplated_opt = new_tool.m_Hole_NotPlated;
            last_attribute = new_tool.m_HoleAttribute;
        }

        jj = m_toolListBuffer.size();

        if( jj == 0 )
            continue;                                        // Should not occurs

        m_holeListBuffer[ii].m_Tool_Reference = jj;          // Tool value Initialized (value >= 1)

        m_toolListBuffer.back().m_TotalCount++;

        if( m_holeListBuffer[ii].m_Hole_Shape )
            m_toolListBuffer.back().m_OvalCount++;

        if( m_holeListBuffer[ii].m_IsBackdrill )
        {
            m_toolListBuffer.back().m_IsBackdrill = true;

            if( m_holeListBuffer[ii].m_StubLength.has_value() )
            {
                int stub = *m_holeListBuffer[ii].m_StubLength;

                if( !m_toolListBuffer.back().m_MinStubLength.has_value()
                        || stub < *m_toolListBuffer.back().m_MinStubLength )
                {
                    m_toolListBuffer.back().m_MinStubLength = stub;
                }

                if( !m_toolListBuffer.back().m_MaxStubLength.has_value()
                        || stub > *m_toolListBuffer.back().m_MaxStubLength )
                {
                    m_toolListBuffer.back().m_MaxStubLength = stub;
                }
            }
        }

        if( m_holeListBuffer[ii].m_FrontPostMachining == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
            || m_holeListBuffer[ii].m_FrontPostMachining == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK
            || m_holeListBuffer[ii].m_BackPostMachining == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
            || m_holeListBuffer[ii].m_BackPostMachining == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK
            || m_holeListBuffer[ii].m_IsBackdrill )
            m_toolListBuffer.back().m_HasPostMachining = true;
    }
}


std::vector<DRILL_SPAN> GENDRILL_WRITER_BASE::getUniqueLayerPairs() const
{
    wxASSERT( m_pcb );

    PCB_TYPE_COLLECTOR vias;

    vias.Collect( m_pcb, { PCB_VIA_T } );

    std::set<DRILL_SPAN> unique;

    for( int i = 0; i < vias.GetCount(); ++i )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( vias[i] );
        PCB_LAYER_ID top_layer;
        PCB_LAYER_ID bottom_layer;

        via->LayerPair( &top_layer, &bottom_layer );

        if( DRILL_LAYER_PAIR( top_layer, bottom_layer ) != DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
            unique.emplace( top_layer, bottom_layer, false, false );

        const PADSTACK::DRILL_PROPS& secondary = via->Padstack().SecondaryDrill();

        if( secondary.start == UNDEFINED_LAYER || secondary.end == UNDEFINED_LAYER )
            continue;

        if( secondary.size.x <= 0 && secondary.size.y <= 0 )
            continue;

        unique.emplace( secondary.start, secondary.end, true, false );
    }

    std::vector<DRILL_SPAN> ret;

    ret.emplace_back( F_Cu, B_Cu, false, false );

    for( const DRILL_SPAN& span : unique )
    {
        if( span.m_IsBackdrill || span.Pair() != DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
            ret.push_back( span );
    }

    return ret;
}


const std::string GENDRILL_WRITER_BASE::layerName( PCB_LAYER_ID aLayer ) const
{
    // Generic names here.
    switch( aLayer )
    {
    case F_Cu:
        return "front";
    case B_Cu:
        return "back";
    default:
    {
        // aLayer use even values, and the first internal layer (In1) is B_Cu + 2.
        int ly_id = ( aLayer - B_Cu ) / 2;
        return fmt::format( "in{}", ly_id );
    }
    }
}


const std::string GENDRILL_WRITER_BASE::layerPairName( DRILL_LAYER_PAIR aPair ) const
{
    std::string ret = layerName( aPair.first );
    ret += '-';
    ret += layerName( aPair.second );

    return ret;
}


const wxString GENDRILL_WRITER_BASE::getDrillFileName( const DRILL_SPAN& aSpan, bool aNPTH,
                                                       bool aMerge_PTH_NPTH ) const
{
    wxASSERT( m_pcb );

    wxString    extend;

    auto layerIndex = [&]( PCB_LAYER_ID aLayer )
    {
        int conventional_layer_num = 1;

        for( PCB_LAYER_ID layer : LSET::AllCuMask( m_pcb->GetCopperLayerCount() ).UIOrder() )
        {
            if( layer == aLayer )
                return conventional_layer_num;

            conventional_layer_num++;
        }

        return conventional_layer_num;
    };

    if( aSpan.m_IsBackdrill )
    {
        extend.Printf( wxT( "_Backdrills_Drill_%d_%d" ),
                       layerIndex( aSpan.DrillStartLayer() ),
                       layerIndex( aSpan.DrillEndLayer() ) );
    }
    else if( aNPTH )
    {
        extend = wxT( "-NPTH" );
    }
    else if( aSpan.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
    {
        if( !aMerge_PTH_NPTH )
            extend = wxT( "-PTH" );
        // if merged, extend with nothing
    }
    else
    {
        extend += '-';
        extend += layerPairName( aSpan.Pair() );
    }

    wxFileName  fn = m_pcb->GetFileName();

    fn.SetName( fn.GetName() + extend );
    fn.SetExt( m_drillFileExtension );

    wxString ret = fn.GetFullName();

    return ret;
}


const wxString GENDRILL_WRITER_BASE::getProtectionFileName( const DRILL_SPAN& aSpan,
                                                            IPC4761_FEATURES aFeature ) const
{
    wxASSERT( m_pcb );

    wxString extend;

    switch( aFeature )
    {
    case IPC4761_FEATURES::FILLED:
        extend << wxT( "-filling-" );
        extend << layerPairName( aSpan.Pair() );
        break;
    case IPC4761_FEATURES::CAPPED:
        extend << wxT( "-capping-" );
        extend << layerPairName( aSpan.Pair() );
        break;
    case IPC4761_FEATURES::COVERED_BACK:
        extend << wxT( "-covering-" );
        extend << layerName( aSpan.Pair().second );
        break;
    case IPC4761_FEATURES::COVERED_FRONT:
        extend << wxT( "-covering-" );
        extend << layerName( aSpan.Pair().first );
        break;
    case IPC4761_FEATURES::PLUGGED_BACK:
        extend << wxT( "-plugging-" );
        extend << layerName( aSpan.Pair().second );
        break;
    case IPC4761_FEATURES::PLUGGED_FRONT:
        extend << wxT( "-plugging-" );
        extend << layerName( aSpan.Pair().first );
        break;
    case IPC4761_FEATURES::TENTED_BACK:
        extend << wxT( "-tenting-" );
        extend << layerName( aSpan.Pair().second );
        break;
    case IPC4761_FEATURES::TENTED_FRONT:
        extend << wxT( "-tenting-" );
        extend << layerName( aSpan.Pair().first );
        break;
    }

    wxFileName fn = m_pcb->GetFileName();

    fn.SetName( fn.GetName() + extend );
    fn.SetExt( m_drillFileExtension );

    wxString ret = fn.GetFullName();

    return ret;
}


bool GENDRILL_WRITER_BASE::CreateMapFilesSet( const wxString& aPlotDirectory, REPORTER * aReporter )
{
    wxFileName  fn;
    wxString    msg;

    std::vector<DRILL_SPAN> hole_sets = getUniqueLayerPairs();

    if( !m_merge_PTH_NPTH )
        hole_sets.emplace_back( F_Cu, B_Cu, false, true );

    for( std::vector<DRILL_SPAN>::const_iterator it = hole_sets.begin(); it != hole_sets.end(); ++it )
    {
        const DRILL_SPAN& span = *it;
        bool doing_npth = span.m_IsNonPlatedFile;

        buildHolesList( span, doing_npth );

        if( getHolesCount() > 0 || doing_npth || span.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
        {
            fn = GENDRILL_WRITER_BASE::getDrillFileName( span, doing_npth, m_merge_PTH_NPTH );
            fn.SetPath( aPlotDirectory );

            fn.SetExt( wxEmptyString ); // Will be added by GenDrillMap
            wxString fullfilename = fn.GetFullPath() + wxT( "-drl_map" );
            fullfilename << wxT(".") << GetDefaultPlotExtension( m_mapFileFmt );

            bool success = genDrillMapFile( fullfilename, m_mapFileFmt );

            if( ! success )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Failed to create file '%s'." ), fullfilename );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }

                return false;
            }
            else
            {
                if( aReporter )
                {
                    msg.Printf( _( "Created file '%s'." ), fullfilename );
                    aReporter->Report( msg, RPT_SEVERITY_ACTION );
                }
            }
        }
    }

    return true;
}


const wxString GENDRILL_WRITER_BASE::BuildFileFunctionAttributeString( const DRILL_SPAN& aSpan,
                                                                       TYPE_FILE aHoleType,
                                                                       bool aCompatNCdrill ) const
{
// Build a wxString containing the .FileFunction attribute for drill files.
// %TF.FileFunction,Plated[NonPlated],layer1num,layer2num,PTH[NPTH][Blind][Buried],Drill[Route][Mixed]*%
    wxString text;

    if( aCompatNCdrill )
        text = wxT( "; #@! " );
    else
        text = wxT( "%" );

    text << wxT( "TF.FileFunction," );

    if( aSpan.m_IsBackdrill || aHoleType == NPTH_FILE )
        text << wxT( "NonPlated," );
    else if( aHoleType == MIXED_FILE )  // only for Excellon format
        text << wxT( "MixedPlating," );
    else
        text << wxT( "Plated," );

    int layer1 = aSpan.Pair().first;
    int layer2 = aSpan.Pair().second;
    // In Gerber files, layers num are 1 to copper layer count instead of F_Cu to B_Cu
    // (0 to copper layer count-1)
    // Note also for a n copper layers board, gerber layers num are 1 ... n
    //
    // Copper layers use even values, so the layer id in file is
    // (Copper layer id) /2 + 1 if layer is not B_Cu
    if( layer1 == F_Cu )
        layer1 = 1;
    else
        layer1 = ( ( layer1 - B_Cu ) / 2 ) + 1;

    if( layer2 == B_Cu )
        layer2 = m_pcb->GetCopperLayerCount();
    else
        layer2 = ( ( layer2 - B_Cu ) / 2) + 1;

    text << layer1 << wxT( "," ) << layer2;

    // Now add PTH or NPTH or Blind or Buried attribute
    int toplayer = 1;
    int bottomlayer = m_pcb->GetCopperLayerCount();

    if( aSpan.m_IsBackdrill )
        text << wxT( ",Blind" );
    else if( aHoleType == NPTH_FILE )
        text << wxT( ",NPTH" );
    else if( aHoleType == MIXED_FILE )      // only for Excellon format
        ; // write nothing
    else if( layer1 == toplayer && layer2 == bottomlayer )
        text << wxT( ",PTH" );
    else if( layer1 == toplayer || layer2 == bottomlayer )
        text << wxT( ",Blind" );
    else
        text << wxT( ",Buried" );

    // In NC drill file, these previous parameters should be enough:
    if( aCompatNCdrill )
        return text;


    // Now add Drill or Route or Mixed:
    // file containing only round holes have Drill attribute
    // file containing only oblong holes have Routed attribute
    // file containing both holes have Mixed attribute
    bool hasOblong = false;
    bool hasDrill = false;

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        const HOLE_INFO& hole_descr = m_holeListBuffer[ii];

        if( hole_descr.m_Hole_Shape )   // m_Hole_Shape not 0 is an oblong hole)
            hasOblong = true;
        else
            hasDrill = true;
    }

    if( hasOblong && hasDrill )
        text << wxT( ",Mixed" );
    else if( hasDrill )
        text << wxT( ",Drill" );
    else if( hasOblong )
        text << wxT( ",Rout" );

    // else: empty file.

    // End of .FileFunction attribute:
    text << wxT( "*%" );

    return text;
}


/* Conversion utilities - these will be used often in there... */
inline double diameter_in_inches( double ius )
{
    return ius * 0.001 / pcbIUScale.IU_PER_MILS;
}


inline double diameter_in_mm( double ius )
{
    return ius / pcbIUScale.IU_PER_MM;
}


// return a pen size to plot markers and having a readable shape
// clamped to be >= MIN_SIZE_MM to avoid too small line width
static int getMarkerBestPenSize( int aMarkerDiameter )
{
    int bestsize = aMarkerDiameter / 10;

    const double MIN_SIZE_MM = 0.1;
    bestsize = std::max( bestsize, pcbIUScale.mmToIU( MIN_SIZE_MM ) );

    return bestsize;
}


// return a pen size to plot outlines for oval holes
inline int getSketchOvalBestPenSize()
{
    const double SKETCH_LINE_WIDTH_MM = 0.1;
    return pcbIUScale.mmToIU( SKETCH_LINE_WIDTH_MM );
}


// return a default pen size to plot items with no specific line thickness
inline int getDefaultPenSize()
{
    const double DEFAULT_LINE_WIDTH_MM = 0.2;
    return pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH_MM );
}


bool GENDRILL_WRITER_BASE::genDrillMapFile( const wxString& aFullFileName, PLOT_FORMAT aFormat )
{
    // Remark:
    // Hole list must be created before calling this function, by buildHolesList(),
    // for the right holes set (PTH, NPTH, buried/blind vias ...)

    double    scale = 1.0;
    VECTOR2I  offset = GetOffset();
    PLOTTER*  plotter = nullptr;
    PAGE_INFO dummy( PAGE_SIZE_TYPE::A4, false );
    int       bottom_limit = 0; // Y coord limit of page. 0 mean do not use

    PCB_PLOT_PARAMS plot_opts; // starts plotting with default options

    const PAGE_INFO& page_info = m_pageInfo ? *m_pageInfo : dummy;

    // Calculate dimensions and center of PCB. The Edge_Cuts layer must be visible
    // to calculate the board edges bounding box
    LSET visibleLayers = m_pcb->GetVisibleLayers();
    m_pcb->SetVisibleLayers( visibleLayers | LSET( { Edge_Cuts } ) );
    BOX2I bbbox = m_pcb->GetBoardEdgesBoundingBox();
    m_pcb->SetVisibleLayers( visibleLayers );

    // Some formats cannot be used to generate a document like the map files
    // Currently HPGL (old format not very used)

    if( aFormat == PLOT_FORMAT::HPGL )
        aFormat = PLOT_FORMAT::PDF;

    // Calculate the scale for the format type, scale 1 in HPGL, drawing on
    // an A4 sheet in PS, + text description of symbols
    switch( aFormat )
    {
    case PLOT_FORMAT::GERBER:
        plotter = new GERBER_PLOTTER();
        plotter->SetViewport( offset, pcbIUScale.IU_PER_MILS / 10, scale, false );
        plotter->SetGerberCoordinatesFormat( 5 ); // format x.5 unit = mm
        break;

    default: wxASSERT( false ); KI_FALLTHROUGH;

    case PLOT_FORMAT::PDF:
    case PLOT_FORMAT::POST:
    case PLOT_FORMAT::SVG:
    {
        VECTOR2I pageSizeIU = page_info.GetSizeIU( pcbIUScale.IU_PER_MILS );

        // Reserve a 10 mm margin around the page.
        int margin = pcbIUScale.mmToIU( 10 );

        // Calculate a scaling factor to print the board on the sheet
        double Xscale = double( pageSizeIU.x - ( 2 * margin ) ) / bbbox.GetWidth();

        // We should print the list of drill sizes, so reserve room for it
        // 60% height for board 40% height for list
        int    ypagesize_for_board = KiROUND( pageSizeIU.y * 0.6 );
        double Yscale = double( ypagesize_for_board - margin ) / bbbox.GetHeight();

        scale = std::min( Xscale, Yscale );

        // Experience shows the scale should not to large, because texts
        // create problem (can be to big or too small).
        // So the scale is clipped at 3.0;
        scale = std::min( scale, 3.0 );

        offset.x = KiROUND( double( bbbox.Centre().x ) - ( pageSizeIU.x / 2.0 ) / scale );
        offset.y = KiROUND( double( bbbox.Centre().y ) - ( ypagesize_for_board / 2.0 ) / scale );

        // bottom_limit is used to plot the legend (drill diameters)
        // texts are scaled differently for scale > 1.0 and <= 1.0
        // so the limit is scaled differently.
        bottom_limit = ( pageSizeIU.y - margin ) / std::min( scale, 1.0 );

        if( aFormat == PLOT_FORMAT::SVG )
            plotter = new SVG_PLOTTER;
        else if( aFormat == PLOT_FORMAT::PDF )
            plotter = new PDF_PLOTTER;
        else
            plotter = new PS_PLOTTER;

        plotter->SetPageSettings( page_info );
        plotter->SetViewport( offset, pcbIUScale.IU_PER_MILS / 10, scale, false );
        break;
    }

    case PLOT_FORMAT::DXF:
    {
        DXF_PLOTTER* dxf_plotter = new DXF_PLOTTER;

        dxf_plotter->SetUnits( m_unitsMetric ? DXF_UNITS::MM : DXF_UNITS::INCH );

        plotter = dxf_plotter;
        plotter->SetPageSettings( page_info );
        plotter->SetViewport( offset, pcbIUScale.IU_PER_MILS / 10, scale, false );
        break;
    }
    }

    plotter->SetCreator( wxT( "PCBNEW" ) );
    plotter->SetColorMode( false );

    KIGFX::PCB_RENDER_SETTINGS renderSettings;
    renderSettings.SetDefaultPenWidth( getDefaultPenSize() );

    plotter->SetRenderSettings( &renderSettings );

    if( !plotter->OpenFile( aFullFileName ) )
    {
        delete plotter;
        return false;
    }

    plotter->ClearHeaderLinesList();

    // For the Gerber X2 format we need to set the  "FileFunction" to Drillmap
    // and set a few other options.
    if( plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
    {
        GERBER_PLOTTER* gbrplotter = static_cast<GERBER_PLOTTER*>( plotter );
        gbrplotter->DisableApertMacros( false );
        gbrplotter->UseX2format( true );         // Mandatory
        gbrplotter->UseX2NetAttributes( false ); // net attributes have no meaning here

        // Attributes are added using X2 format
        AddGerberX2Header( gbrplotter, m_pcb, false );

        wxString text;

        // Add the TF.FileFunction
        text = "%TF.FileFunction,Drillmap*%";
        gbrplotter->AddLineToHeader( text );

        // Add the TF.FilePolarity
        text = wxT( "%TF.FilePolarity,Positive*%" );
        gbrplotter->AddLineToHeader( text );
    }

    plotter->StartPlot( wxT( "1" ) );

    // Draw items on edge layer.
    // Not all, only items useful for drill map, i.e. board outlines.
    BRDITEMS_PLOTTER itemplotter( plotter, m_pcb, plot_opts );

    // Use attributes of a drawing layer (we are not really draw the Edge.Cuts layer)
    itemplotter.SetLayerSet( { Dwgs_User } );

    for( BOARD_ITEM* item : m_pcb->Drawings() )
    {
        if( item->GetLayer() != Edge_Cuts )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
        {
            PCB_SHAPE dummy_shape( *static_cast<PCB_SHAPE*>( item ) );
            dummy_shape.SetLayer( Dwgs_User );
            dummy_shape.SetParentGroup( nullptr ); // Remove group association, not needed for plotting
            itemplotter.PlotShape( &dummy_shape );
        }
        break;

        default: break;
        }
    }

    // Plot edge cuts in footprints
    for( const FOOTPRINT* footprint : m_pcb->Footprints() )
    {
        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( item->GetLayer() != Edge_Cuts )
                continue;

            switch( item->Type() )
            {
            case PCB_SHAPE_T:
            {
                PCB_SHAPE dummy_shape( *static_cast<PCB_SHAPE*>( item ) );
                dummy_shape.SetLayer( Dwgs_User );
                dummy_shape.SetParentGroup( nullptr ); // Remove group association, not needed for plotting
                itemplotter.PlotShape( &dummy_shape );
            }
            break;

            default: break;
            }
        }
    }

    int      plotX, plotY, TextWidth;
    int      intervalle = 0;
    char     line[1024];
    wxString msg;
    int      textmarginaftersymbol = pcbIUScale.mmToIU( 2 );

    // Set Drill Symbols width
    plotter->SetCurrentLineWidth( -1 );

    // Plot board outlines and drill map
    plotDrillMarks( plotter );

    // Print a list of symbols used.
    int charSize = pcbIUScale.mmToIU( 2 ); // text size in IUs

    // real char scale will be 1/scale, because the global plot scale is scale
    // for scale < 1.0 ( plot bigger actual size)
    // Therefore charScale = 1.0 / scale keep the initial charSize
    // (for scale < 1 we use the global scaling factor: the board must be plotted
    // smaller than the actual size)
    double charScale = std::min( 1.0, 1.0 / scale );

    TextWidth = KiROUND( ( charSize * charScale ) / 10.0 ); // Set text width (thickness)
    intervalle = KiROUND( charSize * charScale ) + TextWidth;

    // Trace information.
    plotX = KiROUND( bbbox.GetX() + textmarginaftersymbol * charScale );
    plotY = bbbox.GetBottom() + intervalle;

    // Plot title  "Info"
    wxString Text = wxT( "Drill Map:" );

    TEXT_ATTRIBUTES attrs;
    attrs.m_StrokeWidth = TextWidth;
    attrs.m_Angle = ANGLE_HORIZONTAL;
    attrs.m_Size = KiROUND( charSize * charScale, charSize * charScale );
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;
    attrs.m_Multiline = false;

    plotter->PlotText( VECTOR2I( plotX, plotY ), COLOR4D::UNSPECIFIED, Text, attrs, nullptr /* stroke font */,
                       KIFONT::METRICS::Default() );

    // For some formats (PS, PDF SVG) we plot the drill size list on more than one column
    // because the list must be contained inside the printed page
    // (others formats do not have a defined page size)
    int max_line_len = 0; // The max line len in iu of the currently plotted column

    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        DRILL_TOOL& tool = m_toolListBuffer[ii];

        if( tool.m_TotalCount == 0 )
            continue;

        plotY += intervalle;

        // Ensure there are room to plot the line
        if( bottom_limit && ( plotY + intervalle > bottom_limit ) )
        {
            plotY = bbbox.GetBottom() + intervalle;
            plotX += max_line_len + pcbIUScale.mmToIU( 10 ); //column_width;
            max_line_len = 0;
        }

        int plot_diam = KiROUND( tool.m_Diameter );

        // For markers plotted with the comment, keep marker size <= text height
        plot_diam = std::min( plot_diam, KiROUND( charSize * charScale ) );
        int x = KiROUND( plotX - textmarginaftersymbol * charScale - plot_diam / 2.0 );
        int y = KiROUND( plotY + charSize * charScale );

        plotter->SetCurrentLineWidth( getMarkerBestPenSize( plot_diam ) );
        plotter->Marker( VECTOR2I( x, y ), plot_diam, ii );
        plotter->SetCurrentLineWidth( -1 );

        // List the diameter of each drill in mm and inches.
        snprintf( line, sizeof( line ), "%3.3fmm / %2.4f\" ", diameter_in_mm( tool.m_Diameter ),
                  diameter_in_inches( tool.m_Diameter ) );

        msg = From_UTF8( line );
        wxString extraInfo;

        if( tool.m_HoleAttribute == HOLE_ATTRIBUTE::HOLE_PAD_CASTELLATED )
            extraInfo += wxT( ", castellated" );
        else if( tool.m_HoleAttribute == HOLE_ATTRIBUTE::HOLE_PAD_PRESSFIT )
            extraInfo += wxT( ", press-fit" );

        if( tool.m_IsBackdrill )
        {
            if( tool.m_MinStubLength.has_value() )
            {
                double minStub = pcbIUScale.IUTomm( *tool.m_MinStubLength );

                if( tool.m_MaxStubLength.has_value() && tool.m_MaxStubLength != tool.m_MinStubLength )
                {
                    double maxStub = pcbIUScale.IUTomm( *tool.m_MaxStubLength );
                    extraInfo += wxString::Format( wxT( ", backdrill stub %.3f-%.3fmm" ), minStub, maxStub );
                }
                else
                {
                    extraInfo += wxString::Format( wxT( ", backdrill stub %.3fmm" ), minStub );
                }
            }
            else
            {
                extraInfo += wxT( ", backdrill" );
            }
        }

        if( tool.m_HasPostMachining )
            extraInfo += wxT( ", post-machined" );

        wxString counts;

        if( ( tool.m_TotalCount == 1 ) && ( tool.m_OvalCount == 0 ) )
            counts.Printf( wxT( "(1 hole%s)" ), extraInfo );
        else if( tool.m_TotalCount == 1 )
            counts.Printf( wxT( "(1 slot%s)" ), extraInfo );
        else if( tool.m_OvalCount == 0 )
            counts.Printf( wxT( "(%d holes%s)" ), tool.m_TotalCount, extraInfo );
        else if( tool.m_OvalCount == 1 )
            counts.Printf( wxT( "(%d holes + 1 slot%s)" ), tool.m_TotalCount - 1, extraInfo );
        else
            counts.Printf( wxT( "(%d holes + %d slots%s)" ), tool.m_TotalCount - tool.m_OvalCount, tool.m_OvalCount,
                           extraInfo );

        msg += counts;

        if( tool.m_Hole_NotPlated )
            msg += wxT( " (not plated)" );

        plotter->PlotText( VECTOR2I( plotX, y ), COLOR4D::UNSPECIFIED, msg, attrs, nullptr /* stroke font */,
                           KIFONT::METRICS::Default() );

        intervalle = KiROUND( ( ( charSize * charScale ) + TextWidth ) * 1.2 );

        if( intervalle < ( plot_diam + ( 1 * pcbIUScale.IU_PER_MM / scale ) + TextWidth ) )
            intervalle = plot_diam + ( 1 * pcbIUScale.IU_PER_MM / scale ) + TextWidth;

        // Evaluate the text horizontal size, to know the maximal column size
        // This is a rough value, but ok to create a new column to plot next texts
        int text_len = msg.Len() * ( ( charSize * charScale ) + TextWidth );
        max_line_len = std::max( max_line_len, text_len + plot_diam );
    }

    plotter->EndPlot();
    delete plotter;

    return true;
}


bool GENDRILL_WRITER_BASE::GenDrillReportFile( const wxString& aFullFileName, REPORTER* aReporter )
{
    wxFFile out( aFullFileName, "wb" );
    if( !out.IsOpened() )
    {
        wxString msg = wxString::Format( _( "Error creating drill report file '%s'" ), aFullFileName );
        aReporter->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    FILE*                outFp = out.fp();

    static const char separator[] =
        "    =============================================================\n";

    wxASSERT( m_pcb );

    unsigned    totalHoleCount;
    wxFileName  brdFilename( m_pcb->GetFileName() );

    std::vector<DRILL_SPAN> hole_sets = getUniqueLayerPairs();

    bool writeError = false;

    try
    {
        fmt::print( outFp, "Drill report for {}\n", TO_UTF8( brdFilename.GetFullName() ) );
        fmt::print( outFp, "Created on {}\n\n", TO_UTF8( GetISO8601CurrentDateTime() ) );

        // Output the cu layer stackup, so layer name references make sense.
        fmt::print( outFp, "Copper Layer Stackup:\n" );
        fmt::print( outFp, "{}", separator );

        int conventional_layer_num = 1;

        for( PCB_LAYER_ID layer : LSET::AllCuMask( m_pcb->GetCopperLayerCount() ).UIOrder() )
        {
            fmt::print( outFp, "    L{:<2}:  {:<25} {}\n", conventional_layer_num++,
                        TO_UTF8( m_pcb->GetLayerName( layer ) ),
                        layerName( layer ).c_str() ); // generic layer name
        }

        fmt::print( outFp, "\n\n" );

        /* output hole lists:
         * 1 - through holes
         * 2 - for partial holes only: by layer starting and ending pair
         * 3 - Non Plated through holes
         */

        bool buildNPTHlist = false; // First pass: build PTH list only

        // in this loop are plated only:
        for( unsigned pair_ndx = 0; pair_ndx < hole_sets.size(); ++pair_ndx )
        {
            const DRILL_SPAN& span = hole_sets[pair_ndx];

            buildHolesList( span, buildNPTHlist );

            if( span.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) && !span.m_IsBackdrill )
            {
                fmt::print( outFp, "Drill file '{}' contains\n",
                            TO_UTF8( getDrillFileName( span, false, m_merge_PTH_NPTH ) ) );

                fmt::print( outFp, "    plated through holes:\n" );
                fmt::print( outFp, "{}", separator );
                totalHoleCount = printToolSummary( outFp, false );
                fmt::print( outFp, "    Total plated holes count {}\n", totalHoleCount );
            }
            else if( span.m_IsBackdrill )
            {
                fmt::print( outFp, "Drill file '{}' contains\n",
                            TO_UTF8( getDrillFileName( span, false, m_merge_PTH_NPTH ) ) );

                fmt::print( outFp, "    backdrill span: '{}' to '{}':\n",
                            TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( span.DrillStartLayer() ) ) ),
                            TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( span.DrillEndLayer() ) ) ) );

                fmt::print( outFp, "{}", separator );
                totalHoleCount = printToolSummary( outFp, false );
                fmt::print( outFp, "    Total backdrilled holes count {}\n", totalHoleCount );
            }
            else
            {
                fmt::print( outFp, "Drill file '{}' contains\n",
                            TO_UTF8( getDrillFileName( span, false, m_merge_PTH_NPTH ) ) );

                fmt::print( outFp, "    holes connecting layer pair: '{} and {}' ({} vias):\n",
                            TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( span.Pair().first ) ) ),
                            TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( span.Pair().second ) ) ),
                            span.Pair().first == F_Cu || span.Pair().second == B_Cu ? "blind" : "buried" );

                fmt::print( outFp, "{}", separator );
                totalHoleCount = printToolSummary( outFp, false );
                fmt::print( outFp, "    Total plated holes count {}\n", totalHoleCount );
            }

            fmt::print( outFp, "\n\n" );
        }

        // NPTHoles. Generate the full list (pads+vias) if PTH and NPTH are merged,
        // or only the NPTH list (which never has vias)
        if( !m_merge_PTH_NPTH )
            buildNPTHlist = true;

        DRILL_SPAN npthSpan( F_Cu, B_Cu, false, buildNPTHlist );

        buildHolesList( npthSpan, buildNPTHlist );

        // nothing wrong with an empty NPTH file in report.
        if( m_merge_PTH_NPTH )
            fmt::print( outFp, "Not plated through holes are merged with plated holes\n" );
        else
            fmt::print( outFp, "Drill file '{}' contains\n",
                        TO_UTF8( getDrillFileName( npthSpan, true, m_merge_PTH_NPTH ) ) );

        fmt::print( outFp, "    unplated through holes:\n" );
        fmt::print( outFp, "{}", separator );
        totalHoleCount = printToolSummary( outFp, true );
        fmt::print( outFp, "    Total unplated holes count {}\n", totalHoleCount );
    }
    catch( const std::system_error& )
    {
        writeError = true;
    }
    catch( const fmt::format_error& )
    {
        writeError = true;
    }

    if( writeError )
    {
        wxString msg = wxString::Format( _( "Created drill report file '%s'" ), aFullFileName );
        aReporter->Report( msg, RPT_SEVERITY_ACTION );
    }

    return !writeError;
}


bool GENDRILL_WRITER_BASE::plotDrillMarks( PLOTTER* aPlotter )
{
    // Plot the drill map:
    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        const HOLE_INFO& hole = m_holeListBuffer[ii];

        // Gives a good line thickness to have a good marker shape:
        aPlotter->SetCurrentLineWidth( getMarkerBestPenSize( hole.m_Hole_Diameter ) );

        // Always plot the drill symbol (for slots identifies the needed cutter!
        aPlotter->Marker( hole.m_Hole_Pos, hole.m_Hole_Diameter, hole.m_Tool_Reference - 1 );

        if( hole.m_Hole_Shape != 0 )
        {
            aPlotter->ThickOval( hole.m_Hole_Pos, hole.m_Hole_Size, hole.m_Hole_Orient, getSketchOvalBestPenSize(),
                                 nullptr );
        }
    }

    aPlotter->SetCurrentLineWidth( PLOTTER::USE_DEFAULT_LINE_WIDTH );

    return true;
}


unsigned GENDRILL_WRITER_BASE::printToolSummary( FILE* out, bool aSummaryNPTH ) const
{
    unsigned totalHoleCount = 0;

    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        const DRILL_TOOL& tool = m_toolListBuffer[ii];

        if( aSummaryNPTH && !tool.m_Hole_NotPlated )
            continue;

        if( !aSummaryNPTH && tool.m_Hole_NotPlated )
            continue;

        // List the tool number assigned to each drill in mm then in inches.
        int tool_number = ii+1;
        fmt::print( 0, "    T{}  {:2.3f}mm  {:2.4f}\"  ", tool_number,
                   diameter_in_mm( tool.m_Diameter ),
                   diameter_in_inches( tool.m_Diameter ) );

        // Now list how many holes and ovals are associated with each drill.
        if( ( tool.m_TotalCount == 1 ) && ( tool.m_OvalCount == 0 ) )
            fmt::print( out, "(1 hole" );
        else if( tool.m_TotalCount == 1 )
            fmt::print( out, "(1 hole)  (with 1 slot" );
        else if( tool.m_OvalCount == 0 )
            fmt::print( out, "({} holes)", tool.m_TotalCount );
        else if( tool.m_OvalCount == 1 )
            fmt::print( out, "({} holes)  (with 1 slot", tool.m_TotalCount );
        else // tool.m_OvalCount > 1
            fmt::print( out, "({} holes)  (with {} slots", tool.m_TotalCount, tool.m_OvalCount );

        if( tool.m_HoleAttribute == HOLE_ATTRIBUTE::HOLE_PAD_CASTELLATED )
            fmt::print( out, ", castellated" );

        if( tool.m_HoleAttribute == HOLE_ATTRIBUTE::HOLE_PAD_PRESSFIT )
            fmt::print( out, ", press-fit" );

        fmt::print( out, ")\n" );

        totalHoleCount += tool.m_TotalCount;
    }

    fmt::print( out, "\n" );

    return totalHoleCount;
}