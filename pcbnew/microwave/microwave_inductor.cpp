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

#include <base_units.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <confirm.h>
#include <dialogs/dialog_text_entry.h>
#include <geometry/geometry_utils.h>
#include <math/util.h>      // for KiROUND
#include <microwave/microwave_tool.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <pcb_edit_frame.h>
#include <validators.h>

/**
 * Function  gen_arc
 * generates an arc using arc approximation by lines:
 * Center aCenter
 * Angle "angle" (in 0.1 deg)
 * @param  aBuffer = a buffer to store points.
 * @param  aStartPoint = starting point of arc.
 * @param  aCenter = arc centre.
 * @param  a_ArcAngle = arc length in 0.1 degrees.
 */
static void gen_arc( std::vector<VECTOR2I>& aBuffer, const VECTOR2I& aStartPoint, const VECTOR2I& aCenter,
                     const EDA_ANGLE& a_ArcAngle )
{
    VECTOR2D first_point = VECTOR2D( aStartPoint ) - aCenter;
    double   radius = first_point.EuclideanNorm();
    int      seg_count = GetArcToSegmentCount( radius, ARC_HIGH_DEF, a_ArcAngle );

    double increment_angle = a_ArcAngle.AsRadians() / seg_count;

    // Creates nb_seg point to approximate arc by segments:
    for( int ii = 1; ii <= seg_count; ii++ )
    {
        double  rot_angle = increment_angle * ii;
        double  fcos = cos( rot_angle );
        double  fsin = sin( rot_angle );
        VECTOR2I currpt;

        // Rotate current point:
        currpt.x = KiROUND( ( first_point.x * fcos + first_point.y * fsin ) );
        currpt.y = KiROUND( ( first_point.y * fcos - first_point.x * fsin ) );

        auto corner = aCenter + currpt;
        aBuffer.push_back( corner );
    }
}


enum class INDUCTOR_S_SHAPE_RESULT
{
    OK,        /// S-shape constructed
    TOO_LONG,  /// Requested length too long
    TOO_SHORT, /// Requested length too short
    NO_REPR,   /// Requested length can't be represented
};


/**
 * Function BuildCornersList_S_Shape
 * Create a path like a S-shaped coil
 * @param  aBuffer =  a buffer where to store points (ends of segments)
 * @param  aStartPoint = starting point of the path
 * @param  aEndPoint = ending point of the path
 * @param  aLength = full length of the path
 * @param  aWidth = segment width
 */
static INDUCTOR_S_SHAPE_RESULT BuildCornersList_S_Shape( std::vector<VECTOR2I>& aBuffer,
                                                         const VECTOR2I& aStartPoint, const VECTOR2I& aEndPoint,
                                                         int aLength, int aWidth )
{
    /*
     * We must determine:
     * segm_count = number of segments perpendicular to the direction
     * segm_len = length of a strand
     * radius = radius of rounded parts of the coil
     * stubs_len = length of the 2 stubs( segments parallel to the direction)
     *         connecting the start point to the start point of the S shape
     *         and the ending point to the end point of the S shape
     * The equations are (assuming the area size of the entire shape is Size:
     * Size.x = 2 * radius + segm_len
     * Size.y = (segm_count + 2 ) * 2 * radius + 2 * stubs_len
     * aInductorPattern.m_length = 2 * delta // connections to the coil
     *             + (segm_count-2) * segm_len      // length of the strands except 1st and last
     *             + (segm_count) * (PI * radius)   // length of rounded
     * segm_len + / 2 - radius * 2)                 // length of 1st and last bit
     *
     * The constraints are:
     * segm_count >= 2
     * radius < m_Size.x
     * Size.y = (radius * 4) + (2 * stubs_len)
     * segm_len > radius * 2
     *
     * The calculation is conducted in the following way:
     * first:
     * segm_count = 2
     * radius = 4 * Size.x (arbitrarily fixed value)
     * Then:
     * Increasing the number of segments to the desired length
     * (radius decreases if necessary)
     */
    wxPoint size;

    // This scale factor adjusts the arc length to handle
    // the arc to segment approximation.
    // because we use SEGM_COUNT_PER_360DEG segment to approximate a circle,
    // the trace len must be corrected when calculated using arcs
    // this factor adjust calculations and must be changed if SEGM_COUNT_PER_360DEG is modified
    // because trace using segment is shorter the corresponding arc
    // ADJUST_SIZE is the ratio between tline len and the arc len for an arc
    // of 360/ADJUST_SIZE angle
    #define ADJUST_SIZE 0.988

    auto      pt  = aEndPoint - aStartPoint;
    EDA_ANGLE angle( pt );
    int       min_len = pt.EuclideanNorm();
    int       segm_len = 0;       // length of segments
    int       full_len;           // full len of shape (sum of length of all segments + arcs)

    angle = -angle;

    /*
     * Note: calculations are made for a vertical coil (more easy calculations)
     * and after points are rotated to their actual position
     * So the main direction is the Y axis.
     * the 2 stubs are on the Y axis
     * the others segments are parallel to the X axis.
     */

    // Calculate the size of area (for a vertical shape)
    size.x = min_len / 2;
    size.y = min_len;

    // Choose a reasonable starting value for the radius of the arcs.
    int radius = std::min( aWidth * 5, size.x / 4 );

    int segm_count;     // number of full len segments
                        // the half size segments (first and last segment) are not counted here
    int stubs_len = 0;  // length of first or last segment (half size of others segments)

    for( segm_count = 0; ; segm_count++ )
    {
        stubs_len = ( size.y - ( radius * 2 * (segm_count + 2 ) ) ) / 2;

        if( stubs_len < size.y / 10 ) // Reduce radius.
        {
            stubs_len = size.y / 10;
            radius    = ( size.y - (2 * stubs_len) ) / ( 2 * (segm_count + 2) );

            if( radius < aWidth ) // Radius too small.
            {
                // Unable to create line: Requested length value is too large for room
                return INDUCTOR_S_SHAPE_RESULT::TOO_LONG;
            }
        }

        segm_len  = size.x - ( radius * 2 );
        full_len  = 2 * stubs_len;               // Length of coil connections.
        full_len += segm_len * segm_count;       // Length of full length segments.
        full_len += KiROUND( ( segm_count + 2 ) * M_PI * ADJUST_SIZE * radius );    // Ard arcs len
        full_len += segm_len - (2 * radius);     // Length of first and last segments
                                                 // (half size segments len = segm_len/2 - radius).

        if( full_len >= aLength )
            break;
    }

    // Adjust len by adjusting segm_len:
    int delta_size = full_len - aLength;

    // reduce len of the segm_count segments + 2 half size segments (= 1 full size segment)
    segm_len -= delta_size / (segm_count + 1);

    // at this point, it could still be that the requested length is too
    // short (because 4 quarter-circles are too long)
    // to fix this is a relatively complex numerical problem which probably
    // needs a refactor in this area. For now, just reject these cases:
    {
        const int min_total_length = 2 * stubs_len + 2 * M_PI * ADJUST_SIZE * radius;
        if( min_total_length > aLength )
        {
            // we can't express this inductor with 90-deg arcs of this radius
            return INDUCTOR_S_SHAPE_RESULT::TOO_SHORT;
        }
    }

    if( segm_len - 2 * radius < 0 )
    {
        // we can't represent this exact requested length with this number
        // of segments (using the current algorithm). This stems from when
        // you add a segment, you also add another half-circle, so there's a
        // little bit of "dead" space.
        // It's a bit ugly to just reject the input, as it might be possible
        // to tweak the radius, but, again, that probably needs a refactor.
        return INDUCTOR_S_SHAPE_RESULT::NO_REPR;
    }

    // Generate first line (the first stub) and first arc (90 deg arc)
    pt = aStartPoint;
    aBuffer.push_back( pt );
    pt.y += stubs_len;
    aBuffer.push_back( pt );

    auto centre = pt;
    centre.x -= radius;
    gen_arc( aBuffer, pt, centre, -ANGLE_90 );
    pt = aBuffer.back();

    int half_size_seg_len = segm_len / 2 - radius;

    if( half_size_seg_len )
    {
        pt.x -= half_size_seg_len;
        aBuffer.push_back( pt );
    }

    // Create shape.
    int ii;
    int sign = 1;
    segm_count += 1;    // increase segm_count to create the last half_size segment

    for( ii = 0; ii < segm_count; ii++ )
    {
        if( ii & 1 ) // odd order arcs are greater than 0
            sign = -1;
        else
            sign = 1;

        centre    = pt;
        centre.y += radius;
        gen_arc( aBuffer, pt, centre, ANGLE_180 * sign );
        pt    = aBuffer.back();
        pt.x += segm_len * sign;
        aBuffer.push_back( pt );
    }

    // The last point is false:
    // it is the end of a full size segment, but must be
    // the end of the second half_size segment. Change it.
    sign *= -1;
    aBuffer.back().x = aStartPoint.x + radius * sign;

    // create last arc
    pt        = aBuffer.back();
    centre    = pt;
    centre.y += radius;
    gen_arc( aBuffer, pt, centre, ANGLE_90 * sign );

    // Rotate point
    angle += ANGLE_90;

    for( unsigned jj = 0; jj < aBuffer.size(); jj++ )
        RotatePoint( aBuffer[jj], aStartPoint, angle );

    // push last point (end point)
    aBuffer.push_back( aEndPoint );

    return INDUCTOR_S_SHAPE_RESULT::OK;
}


void MICROWAVE_TOOL::createInductorBetween( const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_EDIT_FRAME& editFrame = *getEditFrame<PCB_EDIT_FRAME>();

    MICROWAVE_INDUCTOR_PATTERN pattern;

    pattern.m_Width = board()->GetDesignSettings().GetCurrentTrackWidth();

    pattern.m_Start = { aStart.x, aStart.y };
    pattern.m_End = { aEnd.x, aEnd.y };

    wxString errorMessage;

    auto inductorFP = std::unique_ptr<FOOTPRINT>( createMicrowaveInductor( pattern, errorMessage ) );

    // on any error, report if we can
    if ( !inductorFP || !errorMessage.IsEmpty() )
    {
        if ( !errorMessage.IsEmpty() )
            editFrame.ShowInfoBarError( errorMessage );
    }
    else
    {
        // at this point, we can save the footprint
        m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, inductorFP.get() );

        BOARD_COMMIT commit( this );
        commit.Add( inductorFP.release() );
        commit.Push( _("Add Microwave Inductor" ) );
    }
}


FOOTPRINT* MICROWAVE_TOOL::createMicrowaveInductor( MICROWAVE_INDUCTOR_PATTERN& aInductorPattern,
                                                    wxString& aErrorMessage )
{
    /* Build a microwave inductor footprint.
     * - Length Mself.lng
     * - Extremities Mself.m_Start and Mself.m_End
     * We must determine:
     * Mself.nbrin = number of segments perpendicular to the direction
     * (The coil nbrin will demicercles + 1 + 2 1 / 4 circle)
     * Mself.lbrin = length of a strand
     * Mself.radius = radius of rounded parts of the coil
     * Mself.delta = segments extremities connection between him and the coil even
     *
     * The equations are
     * Mself.m_Size.x = 2 * Mself.radius + Mself.lbrin
     * Mself.m_Size.y * Mself.delta = 2 + 2 * Mself.nbrin * Mself.radius
     * Mself.lng = 2 * Mself.delta / / connections to the coil
     + (Mself.nbrin-2) * Mself.lbrin / / length of the strands except 1st and last
     + (Mself.nbrin 1) * (PI * Mself.radius) / / length of rounded
     * Mself.lbrin + / 2 - Melf.radius * 2) / / length of 1st and last bit
     *
     * The constraints are:
     * Nbrin >= 2
     * Mself.radius < Mself.m_Size.x
     * Mself.m_Size.y = Mself.radius * 4 + 2 * Mself.raccord
     * Mself.lbrin> Mself.radius * 2
     *
     * The calculation is conducted in the following way:
     * Initially:
     * Nbrin = 2
     * Radius = 4 * m_Size.x (arbitrarily fixed value)
     * Then:
     * Increasing the number of segments to the desired length
     * (Radius decreases if necessary)
     */

    PAD*            pad;
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();

    VECTOR2I pt = aInductorPattern.m_End - aInductorPattern.m_Start;
    int      min_len = pt.EuclideanNorm();
    aInductorPattern.m_Length = min_len;

    // Enter the desired length.
    wxString             msg = editFrame->StringFromValue( aInductorPattern.m_Length );
    WX_TEXT_ENTRY_DIALOG dlg( editFrame, _( "Length of track:" ), _( "Create Microwave Footprint" ), msg );

    // TODO: why is this QuasiModal?
    if( dlg.ShowQuasiModal() != wxID_OK )
        return nullptr; // canceled by user

    aInductorPattern.m_Length = editFrame->ValueFromString( dlg.GetValue() );

    // Control values (ii = minimum length)
    if( aInductorPattern.m_Length < min_len )
    {
        aErrorMessage = _( "Requested length < minimum length" );
        return nullptr;
    }

    // Calculate the elements.
    std::vector<VECTOR2I>         buffer;
    const INDUCTOR_S_SHAPE_RESULT res = BuildCornersList_S_Shape( buffer, aInductorPattern.m_Start,
                                                                  aInductorPattern.m_End,
                                                                  aInductorPattern.m_Length,
                                                                  aInductorPattern.m_Width );

    switch( res )
    {
    case INDUCTOR_S_SHAPE_RESULT::TOO_LONG:
        aErrorMessage = _( "Requested length too large" );
        return nullptr;
    case INDUCTOR_S_SHAPE_RESULT::TOO_SHORT:
        aErrorMessage = _( "Requested length too small" );
        return nullptr;
    case INDUCTOR_S_SHAPE_RESULT::NO_REPR:
        aErrorMessage = _( "Requested length can't be represented" );
        return nullptr;
    case INDUCTOR_S_SHAPE_RESULT::OK:
        break;
    }

    // Generate footprint. the value is also used as footprint name.
    msg = wxT( "L" );
    WX_TEXT_ENTRY_DIALOG cmpdlg( editFrame, _( "Component value:" ), _( "Create Microwave Footprint" ), msg );
    cmpdlg.SetTextValidator( FOOTPRINT_NAME_VALIDATOR( &msg ) );

    // TODO: why is this QuasiModal?
    if( ( cmpdlg.ShowQuasiModal() != wxID_OK ) || msg.IsEmpty() )
        return nullptr;    //  Aborted by user

    FOOTPRINT* footprint = editFrame->CreateNewFootprint( msg, wxEmptyString );

    footprint->SetFPID( LIB_ID( wxEmptyString, wxT( "mw_inductor" ) ) );
    footprint->SetAttributes( FP_EXCLUDE_FROM_POS_FILES | FP_EXCLUDE_FROM_BOM );
    footprint->ClearFlags();
    footprint->SetPosition( aInductorPattern.m_End );

    // Generate segments
    for( unsigned jj = 1; jj < buffer.size(); jj++ )
    {
        PCB_SHAPE* seg = new PCB_SHAPE( footprint, SHAPE_T::SEGMENT );
        seg->SetStart( buffer[jj - 1] );
        seg->SetEnd( buffer[jj] );
        seg->SetStroke( STROKE_PARAMS( aInductorPattern.m_Width, LINE_STYLE::SOLID ) );
        seg->SetLayer( footprint->GetLayer() );
        footprint->Add( seg );
    }

    // Place a pad on each end of coil.
    pad = new PAD( footprint );

    footprint->Add( pad );

    pad->SetNumber( wxT( "1" ) );
    pad->SetPosition( aInductorPattern.m_End );

    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( aInductorPattern.m_Width, aInductorPattern.m_Width ) );

    pad->SetLayerSet( LSET( { footprint->GetLayer() } ) );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );

    PAD* newpad = new PAD( *pad );
    const_cast<KIID&>( newpad->m_Uuid ) = KIID();

    footprint->Add( newpad );

    pad = newpad;
    pad->SetNumber( wxT( "2" ) );
    pad->SetPosition( aInductorPattern.m_Start );

    // Modify text positions.
    VECTOR2I refPos( ( aInductorPattern.m_Start.x + aInductorPattern.m_End.x ) / 2,
                     ( aInductorPattern.m_Start.y + aInductorPattern.m_End.y ) / 2 );

    VECTOR2I valPos = refPos;

    refPos.y -= footprint->Reference().GetTextSize().y;
    footprint->Reference().SetPosition( refPos );
    valPos.y += footprint->Value().GetTextSize().y;
    footprint->Value().SetPosition( valPos );

    return footprint;
}
