/*! \file src/line.cpp
    \brief Mainly used for calculating crossings
    \author Klaas Holwerda 
 
    Copyright: 2001-2004 (C) Klaas Holwerda 
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: line.cpp,v 1.6 2009/09/13 18:34:06 titato Exp $
*/

// Standard include files
#include "kbool/booleng.h"

// Include files for forward declarations
#include "kbool/link.h"
#include "kbool/node.h"

// header
#include "kbool/line.h"

#include "kbool/graph.h"
#include "kbool/graphlst.h"

//
// The default constructor
//
kbLine::kbLine( Bool_Engine* GC )
{
    m_GC = GC;
    m_AA = 0.0;
    m_BB = 0.0;
    m_CC = 0.0;
    m_link = 0;
    linecrosslist = NULL;
    m_valid_parameters = false;
}

kbLine::~kbLine()
{
    if ( linecrosslist )
        delete linecrosslist;
}

//
// constructor with a link
//
kbLine::kbLine( kbLink *a_link, Bool_Engine* GC )
{
    m_GC = GC;
    // a_link must exist
    assert( a_link );
    // points may not be equal
    // must be an if statement because if an assert is used there will
    // be a macro expansion error

    //if (a_link->GetBeginNode()->Equal(a_link->GetEndNode(), 1))
    // assert(!a_link);

    linecrosslist = NULL;
    m_link = a_link;
    m_valid_parameters = false;
}



// ActionOnTable1
// This function decide which action must be taken, after PointInLine
// has given the results of two points in relation to a line. See table 1 in the report
//
// input Result_beginnode:
//   Result_endnode :
//       The results can be LEFT_SIDE, RIGHT_SIDE, ON_AREA, IN_AREA
//
// return -1: Illegal combination
//         0: No action, no crosspoints
//         1: Investigate results points in relation to the other line
//         2: endnode is a crosspoint, no further investigation
//         3: beginnode is a crosspoint, no further investigation
//     4: beginnode and endnode are crosspoints, no further investigation
//         5: beginnode is a crosspoint, need further investigation
//         6: endnode is a crosspoint, need further investigation
int kbLine::ActionOnTable1( PointStatus Result_beginnode, PointStatus Result_endnode )
{
    // Action 1.5 beginnode and endnode are crosspoints, no further investigation needed
    if (
        ( Result_beginnode == IN_AREA )
        &&
        ( Result_endnode == IN_AREA )
    )
        return 4;
    // Action 1.1, there are no crosspoints, no action
    if (
        (
            ( Result_beginnode == LEFT_SIDE )
            &&
            ( Result_endnode == LEFT_SIDE )
        )
        ||
        (
            ( Result_beginnode == RIGHT_SIDE )
            &&
            ( Result_endnode == RIGHT_SIDE )
        )
    )
        return 0;
    // Action 1.2, maybe there is a crosspoint, further investigation needed
    if (
        (
            ( Result_beginnode == LEFT_SIDE )
            &&
            (
                ( Result_endnode == RIGHT_SIDE )
                ||
                ( Result_endnode == ON_AREA )
            )
        )
        ||
        (
            ( Result_beginnode == RIGHT_SIDE )
            &&
            (
                ( Result_endnode == LEFT_SIDE )
                ||
                ( Result_endnode == ON_AREA )
            )
        )
        ||
        (
            ( Result_beginnode == ON_AREA )
            &&
            (
                ( Result_endnode == LEFT_SIDE )
                ||
                ( Result_endnode == RIGHT_SIDE )
                ||
                ( Result_endnode == ON_AREA )
            )
        )
    )
        return 1;
    // Action 1.3, there is a crosspoint, no further investigation
    if (
        (
            ( Result_beginnode == LEFT_SIDE )
            ||
            ( Result_beginnode == RIGHT_SIDE )
        )
        &&
        ( Result_endnode == IN_AREA )
    )
        return 2;
    // Action 1.4  there is a crosspoint, no further investigation
    if (
        ( Result_beginnode == IN_AREA )
        &&
        (
            ( Result_endnode == LEFT_SIDE )
            ||
            ( Result_endnode == RIGHT_SIDE )
        )
    )
        return 3;
    // Action 1.6  beginnode is a crosspoint, further investigation needed
    if (
        ( Result_beginnode == IN_AREA )
        &&
        ( Result_endnode == ON_AREA )
    )
        return 5;
    // Action 1.7  endnode is a crosspoint, further investigation needed
    if (
        ( Result_beginnode == ON_AREA )
        &&
        ( Result_endnode == IN_AREA )
    )
        return 6;
    // All other combinations are illegal
    return -1;
}


// ActionOnTable2
// This function decide which action must be taken, after PointInLine
// has given the results of two points in relation to a line. It can only give a
// correct decision if first the relation of the points from the line
// are investigated in relation to the line wich can be constucted from the points.
//
// input Result_beginnode:
//   Result_endnode :
//       The results can be LEFT_SIDE, RIGHT_SIDE, ON_AREA, IN_AREA
//
// return -1: Illegal combination
//         0: No action, no crosspoints
//         1: Calculate crosspoint
//         2: endnode is a crosspoint
//         3: beginnode is a crosspoint
//         4: beginnode and endnode are crosspoints
int kbLine::ActionOnTable2( PointStatus Result_beginnode, PointStatus Result_endnode )
{
    // Action 2.5, beginnode and eindpoint are crosspoints
    if (
        ( Result_beginnode == IN_AREA )
        &&
        ( Result_endnode == IN_AREA )
    )
        return 4;
    // Action 2.1, there are no crosspoints
    if (
        (
            ( Result_beginnode == LEFT_SIDE )
            &&
            (
                ( Result_endnode == LEFT_SIDE )
                ||
                ( Result_endnode == ON_AREA )
            )
        )
        ||
        (
            ( Result_beginnode == RIGHT_SIDE )
            &&
            (
                ( Result_endnode == RIGHT_SIDE )
                ||
                ( Result_endnode == ON_AREA )
            )
        )
        ||
        (
            ( Result_beginnode == ON_AREA )
            &&
            (
                ( Result_endnode == LEFT_SIDE )
                ||
                ( Result_endnode == RIGHT_SIDE )
                ||
                ( Result_endnode == ON_AREA )
            )
        )
    )
        return 0;
    // Action 2.2, there is a real intersection, which must be calculated
    if (
        (
            ( Result_beginnode == LEFT_SIDE )
            &&
            ( Result_endnode == RIGHT_SIDE )
        )
        ||
        (
            ( Result_beginnode == RIGHT_SIDE )
            &&
            ( Result_endnode == LEFT_SIDE )
        )
    )
        return 1;
    // Action 2.3, endnode is a crosspoint
    if (
        (
            ( Result_beginnode == LEFT_SIDE )
            ||
            ( Result_beginnode == RIGHT_SIDE )
            ||
            ( Result_beginnode == ON_AREA )
        )
        &&
        ( Result_endnode == IN_AREA )
    )
        return 2;
    // Action 2.4, beginnode is a crosspoint
    if (
        ( Result_beginnode == IN_AREA )
        &&
        (
            ( Result_endnode == LEFT_SIDE )
            ||
            ( Result_endnode == RIGHT_SIDE )
            ||
            ( Result_endnode == ON_AREA )
        )
    )
        return 3;
    // All other combinations are illegal
    return -1;
}

//
// This fucntion will ad a crossing to this line and the other line
// the crossing will be put in the link, because the line will be destructed
// after use of the variable
//
void kbLine::AddLineCrossing( B_INT X, B_INT Y, kbLine *other_line )
{
    // the other line must exist
    assert( other_line );
    // the links of the lines must exist
    assert( other_line->m_link && m_link );
    other_line->AddCrossing( AddCrossing( X, Y ) );
}

// Calculate the Y when the X is given
//
B_INT kbLine::Calculate_Y( B_INT X )
{
    // link must exist to get info about the nodes
    assert( m_link );

    CalculateLineParameters();
    if ( m_AA != 0 )
    {
         //vertical line is undefined
        assert( m_BB );
        return ( B_INT )( -( m_AA * X + m_CC ) / m_BB );
    }
    else
        // horizontal line
        return m_link->GetBeginNode()->GetY();
}

B_INT kbLine::Calculate_Y_from_X( B_INT X )
{
    // link must exist to get info about the nodes
    assert( m_link );
    assert( m_valid_parameters );

    if ( m_AA != 0 )
        return ( B_INT ) ( ( -( m_AA * X + m_CC ) / m_BB ) + 0.5 );
    else
        // horizontal line
        return m_link->GetBeginNode()->GetY();
}

void kbLine::Virtual_Point( kbLPoint *a_point, double distance )
{
    // link must exist to get info about the nodes
    assert( m_link );
    assert( m_valid_parameters );

    //calculate the distance using the slope of the line
    //and rotate 90 degrees

    a_point->SetY( ( B_INT )( a_point->GetY() + ( distance * -( m_BB ) ) ) );
    a_point->SetX( ( B_INT )( a_point->GetX() - ( distance * m_AA   ) ) );
}



//
// Calculate the lineparameters for the line if nessecary
//
void kbLine::CalculateLineParameters()
{
    // linkmust exist to get beginnode AND endnode
    assert( m_link );

    // if not valid_parameters calculate the parameters
    if ( !m_valid_parameters )
    {
        kbNode * bp, *ep;
        double length;

        // get the begin and endnode via the link
        bp = m_link->GetBeginNode();
        ep = m_link->GetEndNode();

        // bp AND ep may not be the same
        if ( bp == ep )
            assert ( bp != ep );

        m_AA = ( double ) ( ep->GetY() - bp->GetY() ); // A = (Y2-Y1)
        m_BB = ( double ) ( bp->GetX() - ep->GetX() ); // B = (X1-X2)

        // the parameters A end B can now be normalized
        length = sqrt( m_AA * m_AA + m_BB * m_BB );

        if( length == 0 )
            m_GC->error( "length = 0", "CalculateLineParameters" );

        m_AA = ( m_AA / length );
        m_BB = ( m_BB / length );

        m_CC = -( ( m_AA * bp->GetX() ) + ( bp->GetY() * m_BB ) );

        m_valid_parameters = true;
    }
}


// Checks if a line intersect with another line
// inout    Line : another line
//          Marge: optional, standard on MARGE (declared in MISC.CPP)
//
// return true : lines are crossing
//    false: lines are not crossing
//
int kbLine::CheckIntersect ( kbLine * lijn, double Marge )
{
    double distance = 0;
    // link must exist
    assert( m_link );
    // lijn must exist
    assert( lijn );

    // points may not be equal
    // must be an if statement because if an assert is used there will
    // be a macro expansion error
    if ( m_link->GetBeginNode() == m_link->GetEndNode() )
        assert( !m_link );

    int Take_Action1, Take_Action2, Total_Result;
    kbNode *bp, *ep;
    PointStatus Result_beginnode, Result_endnode;

    bp = lijn->m_link->GetBeginNode();
    ep = lijn->m_link->GetEndNode();
    Result_beginnode = PointInLine( bp, distance, Marge );
    Result_endnode   = PointInLine( ep, distance, Marge );
    Take_Action1 = ActionOnTable1( Result_beginnode, Result_endnode );
    switch ( Take_Action1 )
    {
        case 0: Total_Result = false ; break;
        case 1:
        {
            bp = m_link->GetBeginNode();
            ep = m_link->GetEndNode();
            Result_beginnode = lijn->PointInLine( bp, distance, Marge );
            Result_endnode   = lijn->PointInLine( ep, distance, Marge );
            Take_Action2 = ActionOnTable2( Result_beginnode, Result_endnode );
            switch ( Take_Action2 )
            {
                case 0: Total_Result = false; break;
                case 1: case 2: case 3: case 4: Total_Result = true; break;
                default: Total_Result = false; assert( Total_Result );
            }
        }
        ; break; // This break belongs to the switch(Take_Action1)
        case 2: case 3: case 4: case 5: case 6: Total_Result = true; break;
        default: Total_Result = false; assert( Total_Result );
    }
    return Total_Result; //This is the final decision
}


//
// Get the beginnode from the line
// usage: kbNode *anode = a_line.GetBeginNode()
//
kbNode *kbLine::GetBeginNode()
{
    // link must exist
    assert( m_link );
    return m_link->GetBeginNode();
}


//
// Get the endnode from the line
// usage: kbNode *anode = a_line.GetEndNode()
//
kbNode *kbLine::GetEndNode()
{
    // link must exist
    assert( m_link );
    return m_link->GetEndNode();
}

// Intersects two lines
// input    Line : another line
//          Marge: optional, standard on MARGE
//
// return 0: If there are no crossings
//    1: If there is one crossing
//    2: If there are two crossings
int kbLine::Intersect( kbLine * lijn, double Marge )
{
    double distance = 0;
    // lijn must exist
    assert( lijn );

    // points may not be equal
    // must be an if statement because if an assert is used there will
    // be a macro expansion error
    if ( m_link->GetBeginNode() == m_link->GetEndNode() )
        assert( !m_link );

    kbNode *bp, *ep;
    PointStatus Result_beginnode, Result_endnode;
    int Take_Action1, Take_Action2, Number_of_Crossings = 0;

    // Get the nodes from lijn via the link
    bp = lijn->m_link->GetBeginNode();
    ep = lijn->m_link->GetEndNode();

    Result_beginnode = PointInLine( bp, distance, Marge );
    Result_endnode   = PointInLine( ep, distance, Marge );

    Take_Action1 = ActionOnTable1( Result_beginnode, Result_endnode );

    // The first switch will insert a crosspoint immediatly
    switch ( Take_Action1 )
    {
            // for the cases see the returnvalue of ActionTable1
    case 2: case 6: AddCrossing( ep );
            Number_of_Crossings = 1;
            break;
    case 3: case 5: AddCrossing( bp );
            Number_of_Crossings = 1;
            break;
        case 4:     AddCrossing( bp );
            AddCrossing( ep );
            Number_of_Crossings = 2;
            break;
    }
    // This switch wil investigate the points of this line in relation to lijn
    switch ( Take_Action1 )
    {
            // for the cases see the returnvalue of ActionTable1
case 1: case 5: case 6:
        {
            // Get the nodes from this line via the link
            bp = m_link->GetBeginNode();
            ep = m_link->GetEndNode();
            Result_beginnode = lijn->PointInLine( bp, distance, Marge );
            Result_endnode   = lijn->PointInLine( ep, distance, Marge );
            Take_Action2 = ActionOnTable2( Result_beginnode, Result_endnode );
            switch ( Take_Action2 )
            {
                    // for the cases see the returnvalue of ActionTable2
                case 1:
                {   // begin of scope to calculate the intersection
                    double X, Y, Denominator;
                    CalculateLineParameters();
                    Denominator  = ( m_AA * lijn->m_BB ) - ( lijn->m_AA * m_BB );
                    // Denominator may not be 0
                    assert( Denominator != 0.0 );
                    // Calculate intersection of both linesegments
                    X = ( ( m_BB * lijn->m_CC ) - ( lijn->m_BB * m_CC ) ) / Denominator;
                    Y = ( ( lijn->m_AA * m_CC ) - ( m_AA * lijn->m_CC ) ) / Denominator;

                    //make a decent rounding to B_INT
                    AddLineCrossing( ( B_INT )X, ( B_INT )Y, lijn );
                }   // end of scope to calculate the intersection
                Number_of_Crossings++;
                break;
                case 2: lijn->AddCrossing( ep );
                    Number_of_Crossings++;
                    break;
                case 3: lijn->AddCrossing( bp );
                    Number_of_Crossings++;
                    break;
                case 4: lijn->AddCrossing( bp );
                    lijn->AddCrossing( ep );
                    Number_of_Crossings = 2;
                    break;
            }
        }
        ; break; // This break belongs to the outer switch
    }
    return Number_of_Crossings; //This is de final number of crossings
}


// Intersects two lines there must be a crossing
int kbLine::Intersect_simple( kbLine * lijn )
{
    // lijn must exist
    assert( lijn );

    double X, Y, Denominator;
    Denominator  = ( m_AA * lijn->m_BB ) - ( lijn->m_AA * m_BB );
    // Denominator may not be 0
    if ( Denominator == 0.0 )
        m_GC->error( "colliniar lines", "line" );
    // Calculate intersection of both linesegments
    X = ( ( m_BB * lijn->m_CC ) - ( lijn->m_BB * m_CC ) ) / Denominator;
    Y = ( ( lijn->m_AA * m_CC ) - ( m_AA * lijn->m_CC ) ) / Denominator;
    AddLineCrossing( ( B_INT )X, ( B_INT )Y, lijn );

    return( 0 );
}

// Intersects two lines there must be a crossing
bool kbLine::Intersect2( kbNode* crossing, kbLine * lijn )
{
    // lijn must exist
    assert( lijn );

    double X, Y, Denominator;
    Denominator  = ( m_AA * lijn->m_BB ) - ( lijn->m_AA * m_BB );
    // Denominator may not be 0
    if ( Denominator == 0.0 )
        return false;
    // Calculate intersection of both linesegments
    X = ( ( m_BB * lijn->m_CC ) - ( lijn->m_BB * m_CC ) ) / Denominator;
    Y = ( ( lijn->m_AA * m_CC ) - ( m_AA * lijn->m_CC ) ) / Denominator;

    crossing->SetX( ( B_INT )X );
    crossing->SetY( ( B_INT )Y );
    return true;
}

//
// test if a point lies in the linesegment. If the point isn't on the line
// the function returns a value that indicates on which side of the
// line the point is (in linedirection from first point to second point
//
// returns LEFT_SIDE, when point lies on the left side of the line
//         RIGHT_SIDE, when point lies on the right side of the line
//         ON_AREA, when point lies on the infinite line within a range
//     IN_AREA, when point lies in the area of the linesegment
//     the returnvalues are declared in (LINE.H)
PointStatus kbLine::PointInLine( kbNode *a_node, double& Distance, double Marge )
{
    Distance = 0;

    //Punt must exist
    assert( a_node );
    // link must exist to get beginnode and endnode via link
    assert( m_link );

    // get the nodes form the line via the link
    kbNode *bp, *ep;
    bp = m_link->GetBeginNode();
    ep = m_link->GetEndNode();

    // both node must exist
    assert( bp && ep );
    // node may not be the same
    assert( bp != ep );

    //quick test if point is begin or endpoint
    if ( a_node == bp || a_node == ep )
        return IN_AREA;

    int Result_of_BBox = false;
    PointStatus Result_of_Online;

    // Checking if point is in bounding-box with marge
    B_INT xmin = bmin( bp->GetX(), ep->GetX() );
    B_INT xmax = bmax( bp->GetX(), ep->GetX() );
    B_INT ymin = bmin( bp->GetY(), ep->GetY() );
    B_INT ymax = bmax( bp->GetY(), ep->GetY() );

    if (  a_node->GetX() >= ( xmin - Marge ) && a_node->GetX() <= ( xmax + Marge ) &&
            a_node->GetY() >= ( ymin - Marge ) && a_node->GetY() <= ( ymax + Marge ) )
        Result_of_BBox = true;

    // Checking if point is on the infinite line
    Result_of_Online = PointOnLine( a_node, Distance, Marge );

    // point in boundingbox of the line and is on the line then the point is IN_AREA
    if ( ( Result_of_BBox ) && ( Result_of_Online == ON_AREA ) )
        return IN_AREA;
    else
        return Result_of_Online;
}


//
// test if a point lies on the line. If the point isn't on the line
// the function returns a value that indicates on which side of the
// line the point is (in linedirection from first point to second point
//
// returns LEFT_SIDE, when point lies on the left side of the line
//         ON_AREA, when point lies on the infinite line within a range
//         RIGHT_SIDE, when point lies on the right side of the line
//     LEFT_SIDE , RIGHT_SIDE , ON_AREA
PointStatus kbLine::PointOnLine( kbNode *a_node, double& Distance, double Marge )
{
    Distance = 0;

    // a_node must exist
    assert( a_node );
    // link must exist to get beginnode and endnode
    assert( m_link );

    // get the nodes from the line via the link
    kbNode *bp, *ep;
    bp = m_link->GetBeginNode();
    ep = m_link->GetEndNode();

    // both node must exist
    assert( bp && ep );
    // node may not be queal
    assert( bp != ep );

    //quick test if point is begin or endpoint
    if ( a_node == bp || a_node == ep )
        return ON_AREA;

    CalculateLineParameters();
    // calculate the distance of a_node in relation to the line
    Distance = ( m_AA * a_node->GetX() ) + ( m_BB * a_node->GetY() ) + m_CC;

    if ( Distance < -Marge )
        return LEFT_SIDE;
    else
    {
        if ( Distance > Marge )
            return RIGHT_SIDE;
        else
            return ON_AREA;
    }
}


//
// Sets lines parameters
// usage: a_line.Set(a_pointer_to_a_link);
//
void kbLine::Set( kbLink *a_link )
{
    // points must exist
    assert( a_link );
    // points may not be equal
    // must be an if statement because if an assert is used there will
    // be a macro expansion error
// if (a_link->GetBeginNode()->Equal(a_link->GetEndNode(),MARGE)) assert(!a_link);

    linecrosslist = NULL;
    m_link = a_link;
    m_valid_parameters = false;
}

kbLink* kbLine::GetLink()
{
    return m_link;
}
//
// makes a line same as these
// usage : line1 = line2;
//
kbLine& kbLine::operator=( const kbLine& a_line )
{
    m_AA = a_line.m_AA;
    m_BB = a_line.m_BB;
    m_CC = a_line.m_CC;
    m_link = a_line.m_link;
    linecrosslist = NULL;
    m_valid_parameters = a_line.m_valid_parameters;
    return *this;
}

kbNode* kbLine::OffsetContour( kbLine* const nextline, kbNode* _last_ins, double factor, kbGraph *shape )
{
    kbLink * offs_currentlink;
    kbLine  offs_currentline( m_GC );
    kbLink* offs_nextlink;
    kbLine  offs_nextline( m_GC );
    kbNode* offs_end;

    kbNode* offs_bgn_next;
    kbNode* offs_end_next;

    // make a node from this point
    offs_end = new kbNode( GetEndNode(), m_GC );
    Virtual_Point( offs_end, factor );
    offs_currentlink = new kbLink( 0, _last_ins, offs_end, m_GC );
    offs_currentline.Set( offs_currentlink );

    offs_bgn_next = new kbNode( nextline->m_link->GetBeginNode(), m_GC );
    nextline->Virtual_Point( offs_bgn_next, factor );

    offs_end_next = new kbNode( nextline->m_link->GetEndNode(), m_GC );
    nextline->Virtual_Point( offs_end_next, factor );

    offs_nextlink = new kbLink( 0, offs_bgn_next, offs_end_next, m_GC );
    offs_nextline.Set( offs_nextlink );

    offs_currentline.CalculateLineParameters();
    offs_nextline.CalculateLineParameters();
    offs_currentline.Intersect2( offs_end, &offs_nextline );

    // make a link between the current and the previous and add this to kbGraph
    shape->AddLink( offs_currentlink );

    delete offs_nextlink;

    return( offs_end );
}


kbNode* kbLine::OffsetContour_rounded( kbLine* const nextline, kbNode* _last_ins, double factor, kbGraph *shape )
{
    kbLink * offs_currentlink;
    kbLine  offs_currentline( m_GC );
    kbLink* offs_nextlink;
    kbLine  offs_nextline( m_GC );
    kbNode* offs_end;
    kbNode* medial_axes_point = new kbNode( m_GC );
    kbNode* bu_last_ins = new kbNode( _last_ins, m_GC );

    kbNode* offs_bgn_next;
    kbNode* offs_end_next;

    // make a node from this point
    offs_end = new kbNode( GetEndNode(), m_GC );

    *_last_ins = *GetBeginNode();
    Virtual_Point( _last_ins, factor );
    Virtual_Point( offs_end, factor );
    offs_currentlink = new kbLink( 0, _last_ins, offs_end, m_GC );
    offs_currentline.Set( offs_currentlink );

    offs_bgn_next = new kbNode( nextline->m_link->GetBeginNode(), m_GC );
    nextline->Virtual_Point( offs_bgn_next, factor );

    offs_end_next = new kbNode( nextline->m_link->GetEndNode(), m_GC );
    nextline->Virtual_Point( offs_end_next, factor );

    offs_nextlink = new kbLink( 0, offs_bgn_next, offs_end_next, m_GC );
    offs_nextline.Set( offs_nextlink );

    offs_currentline.CalculateLineParameters();
    offs_nextline.CalculateLineParameters();
    offs_currentline.Intersect2( medial_axes_point, &offs_nextline );

    double result_offs = sqrt( pow( ( double )GetEndNode()->GetY() - medial_axes_point->GetY(), 2 ) +
                               pow( ( double )GetEndNode()->GetX() - medial_axes_point->GetX(), 2 ) );

    if ( result_offs < fabs( m_GC->GetRoundfactor() * factor ) )
    {
        *_last_ins = *bu_last_ins;
        *offs_end = *medial_axes_point;
        delete medial_axes_point;
        delete bu_last_ins;
        // make a link between the current and the previous and add this to kbGraph
        delete offs_nextlink;
        shape->AddLink( offs_currentlink );
        return( offs_end );
    }
    else
    { //let us create a circle
        *_last_ins = *bu_last_ins;
        delete medial_axes_point;
        delete bu_last_ins;
        kbNode* endarc = new kbNode( offs_bgn_next, m_GC );
        shape->AddLink( offs_currentlink );
        delete offs_nextlink;
        shape->CreateArc( GetEndNode(), &offs_currentline, endarc, fabs( factor ), m_GC->GetInternalCorrectionAber() );
        return( endarc );
    }
}


bool kbLine::OkeForContour( kbLine* const nextline, double factor, kbNode* LastLeft, kbNode* LastRight, LinkStatus& _outproduct )
{
    assert( m_link );
    assert( m_valid_parameters );
    assert( nextline->m_link );
    assert( nextline->m_valid_parameters );

    factor = fabs( factor );

// PointStatus status=ON_AREA;
    double distance = 0;

    kbNode offs_end_next( nextline->m_link->GetEndNode(), m_GC );

    _outproduct = m_link->OutProduct( nextline->m_link, m_GC->GetAccur() );

    switch ( _outproduct )
    {
            // current line lies on  leftside of prev line
        case IS_RIGHT :
        {
            nextline->Virtual_Point( &offs_end_next, -factor );

            // status=
            nextline->PointOnLine( LastRight, distance, m_GC->GetAccur() );
            if ( distance > factor )
            {
                PointOnLine( &offs_end_next, distance, m_GC->GetAccur() );
                if ( distance > factor )
                    return( true );
            }
        }
        break;
        // current line lies on  rightside of prev line
        case IS_LEFT :
        {
            nextline->Virtual_Point( &offs_end_next, factor );

            // status=
            nextline->PointOnLine( LastLeft, distance, m_GC->GetAccur() );
            if ( distance < -factor )
            {
                PointOnLine( &offs_end_next, distance, m_GC->GetAccur() );
                if ( distance < -factor )
                    return( true );
            }
        }
        break;
        // current line  lies on prev line
        case IS_ON  :
        {
            return( true );
        }
    }//end switch

    return( false );
}


bool kbLine::Create_Ring_Shape( kbLine* nextline, kbNode** _last_ins_left, kbNode** _last_ins_right, double factor, kbGraph *shape )
{
    kbNode * _current;
    LinkStatus _outproduct = IS_ON;

    if ( OkeForContour( nextline, factor, *_last_ins_left, *_last_ins_right, _outproduct ) )
    {
        switch ( _outproduct )
        {
                // Line 2 lies on  leftside of this line
            case IS_RIGHT :
            {
                *_last_ins_left  = OffsetContour_rounded( nextline, *_last_ins_left, factor, shape );
                *_last_ins_right = OffsetContour( nextline, *_last_ins_right, -factor, shape );
            }
            break;
            case IS_LEFT :
            {
                *_last_ins_left  = OffsetContour( nextline, *_last_ins_left, factor, shape );
                *_last_ins_right = OffsetContour_rounded( nextline, *_last_ins_right, -factor, shape );

            }
            break;
            // Line 2 lies on this line
            case IS_ON  :
            {
                // make a node from this point
                _current = new kbNode( m_link->GetEndNode(), m_GC );
                Virtual_Point( _current, factor );

                // make a link between the current and the previous and add this to kbGraph
                shape->AddLink( *_last_ins_left, _current );
                *_last_ins_left = _current;

                _current = new kbNode( m_link->GetEndNode(), m_GC );
                Virtual_Point( _current, -factor );

                shape->AddLink( *_last_ins_right, _current );
                *_last_ins_right = _current;
            }
            break;
        }//end switch
        return( true );
    }
    /* else
     {
      switch (_outproduct)
      {
       // Line 2 lies on  leftside of this line
       case IS_RIGHT :
       {
        *_last_ins_left  =OffsetContour_rounded(nextline,*_last_ins_left,factor,Ishape);
        *_last_ins_right =OffsetContour(nextline,*_last_ins_right,-factor,Ishape);
       }
       break;
       case IS_LEFT :
       {
        *_last_ins_left  =OffsetContour(nextline,*_last_ins_left,factor,Ishape);
        *_last_ins_right =OffsetContour_rounded(nextline,*_last_ins_right,-factor,Ishape);
     
       }
       break;
       // Line 2 lies on this line
       case IS_ON  :
       {
        // make a node from this point
        _current = new kbNode(m_link->GetEndNode());
        Virtual_Point(_current,factor);
     
        // make a link between the current and the previous and add this to kbGraph
        Ishape->AddLink(*_last_ins_left, _current);
        *_last_ins_left=_current;
     
        _current = new kbNode(m_link->GetEndNode());
        Virtual_Point(_current,-factor);
     
        Ishape->AddLink(*_last_ins_right, _current);
        *_last_ins_right=_current;
       }
       break;
      }//end switch
      return(true);
     }
    */
    return( false );
}


void kbLine::Create_Begin_Shape( kbLine* nextline, kbNode** _last_ins_left, kbNode** _last_ins_right, double factor, kbGraph *shape )
{
    factor = fabs( factor );
    LinkStatus _outproduct;
    _outproduct = m_link->OutProduct( nextline->m_link, m_GC->GetAccur() );

    switch ( _outproduct )
    {
        case IS_RIGHT :
        {
            *_last_ins_left = new kbNode( m_link->GetEndNode(), m_GC );

            Virtual_Point( *_last_ins_left, factor );

            *_last_ins_right = new kbNode( nextline->m_link->GetBeginNode(), m_GC );
            nextline->Virtual_Point( *_last_ins_right, -factor );

            shape->AddLink( *_last_ins_left, *_last_ins_right );

            *_last_ins_left = OffsetContour_rounded( nextline, *_last_ins_left, factor, shape );
        }
        break;
        case IS_LEFT :
        {
            *_last_ins_left = new kbNode( nextline->m_link->GetBeginNode(), m_GC );
            nextline->Virtual_Point( *_last_ins_left, factor );

            *_last_ins_right = new kbNode( m_link->GetEndNode(), m_GC );
            Virtual_Point( *_last_ins_right, -factor );

            shape->AddLink( *_last_ins_left, *_last_ins_right );

            *_last_ins_right = OffsetContour_rounded( nextline, *_last_ins_right, -factor, shape );
        }
        break;
        // Line 2 lies on this line
        case IS_ON  :
        {
            *_last_ins_left = new kbNode( nextline->m_link->GetBeginNode(), m_GC );
            Virtual_Point( *_last_ins_left, factor );

            *_last_ins_right = new kbNode( nextline->m_link->GetBeginNode(), m_GC );
            Virtual_Point( *_last_ins_right, -factor );

            shape->AddLink( *_last_ins_left, *_last_ins_right );
        }
        break;
    }//end switch

}

void kbLine::Create_End_Shape( kbLine* nextline, kbNode* _last_ins_left, kbNode* _last_ins_right, double factor, kbGraph *shape )
{
    kbNode * _current;
    factor = fabs( factor );
    LinkStatus _outproduct;
    _outproduct = m_link->OutProduct( nextline->m_link, m_GC->GetAccur() );

    switch ( _outproduct )
    {
        case IS_RIGHT :
        {
            _current = new kbNode( m_link->GetEndNode(), m_GC );
            Virtual_Point( _current, -factor );
            shape->AddLink( _last_ins_right, _current );
            _last_ins_right = _current;

            _last_ins_left = OffsetContour_rounded( nextline, _last_ins_left, factor, shape );
            shape->AddLink( _last_ins_left, _last_ins_right );
        }
        break;
        case IS_LEFT :
        {
            _current = new kbNode( m_link->GetEndNode(), m_GC );
            Virtual_Point( _current, factor );
            shape->AddLink( _last_ins_left, _current );
            _last_ins_left = _current;

            _last_ins_right = OffsetContour_rounded( nextline, _last_ins_right, -factor, shape );
            shape->AddLink( _last_ins_right, _last_ins_left );
        }
        break;
        // Line 2 lies on this line
        case IS_ON  :
        {
            _current = new kbNode( m_link->GetEndNode(), m_GC );
            Virtual_Point( _current, factor );
            shape->AddLink( _last_ins_left, _current );
            _last_ins_left = _current;

            _current = new kbNode( m_link->GetEndNode(), m_GC );
            Virtual_Point( _current, -factor );
            shape->AddLink( _last_ins_right, _current );
            _last_ins_right = _current;

            shape->AddLink( _last_ins_left, _last_ins_right );
        }
        break;
    }//end switch

}

//
// Generate from the found crossings a part of the kbGraph
//
bool kbLine::ProcessCrossings( TDLI<kbLink>* _LI )
{
    kbNode * last; kbLink *dummy;
// assert (beginnode && endnode);

    if ( !linecrosslist ) return false;

    if ( linecrosslist->empty() ) return false;
    if ( linecrosslist->count() > 1 ) SortLineCrossings();
    m_link->GetEndNode()->RemoveLink( m_link );
    last = m_link->GetEndNode();
    // Make new links :
    while ( !linecrosslist->empty() )
    {
        dummy = new kbLink( m_link->GetGraphNum(), ( kbNode* ) linecrosslist->tailitem(), last, m_GC );
        dummy->SetBeenHere();
        dummy->SetGroup( m_link->Group() );
        _LI->insbegin( dummy );
        last = ( kbNode* )linecrosslist->tailitem();
        linecrosslist->removetail();
    }
    // Recycle this link :
    last->AddLink( m_link );
    m_link->SetEndNode( last );
    delete linecrosslist;
    linecrosslist = NULL;
    return true;
}

/*
// Sorts the links on the X values
int NodeXYsorter(kbNode* a, kbNode* b)
{
 if ( a->GetX() < b->GetX())
  return(1);
 if ( a->GetX() > b->GetX())
  return(-1);
   //they are eqaul in x
 if ( a->GetY() < b->GetY())
  return(-1);
 if ( a->GetY() > b->GetY())
  return(1);
   //they are eqaul in y
 return(0);
}
 
//
// Generate from the found crossings a part of the graph
// this routine is used in combination with the scanbeam class
// the this link most stay at the same place in the sorted graph
// The link is split into peaces wich are inserted sorted into the graph
// on beginnode.
// The mostleft link most become the new link for the beam record
// therefore the mostleft new/old link is returned to become the beam record link
// also the part returned needs to have the bin flag set to the original value it had in the beam
kbLink* kbLine::ProcessCrossingsSmart(TDLI<kbLink>* _LI)
{
   kbNode *lastinserted;
   kbLink *new_link;
   kbLink *returnlink;
 assert (beginnode && endnode);
 if (!linecrosslist) return this;
 
 if (linecrosslist->empty()) return this;
 if (linecrosslist->count()>1)
   {
    SortLineCrossings();
   }
 int inbeam;
 
   //most left at the beginnode or endnode
   if (NodeXYsorter(beginnode,endnode)==1)
   {
      //re_use this link
      endnode->RemoveLink(this);
      linecrosslist->insend(endnode); //the last link to create is towards this node
      endnode=(kbNode*) linecrosslist->headitem();
      endnode->AddLink(this);
      inbeam=NodeXYsorter(_LI->item()->beginnode,beginnode);
      switch (inbeam)
      {
        case -1:
        case 0:
            bin=true;
            break;
        case 1:
            bin=false;
            break;
      }
      returnlink=this;
 
      lastinserted=endnode;
      linecrosslist->removehead();
      // Make new links starting at endnode
      while (!linecrosslist->empty())
      {
         new_link=new kbLink(graphnum,lastinserted,(kbNode*) linecrosslist->headitem());
 
         new_link->group=group;
         int inbeam=NodeXYsorter(_LI->item()->beginnode,lastinserted);
         switch (inbeam)
         {
           case -1:
               {
                 double x,y,xl,yl;
                 char buf[80];
                 x=((kbNode*)(linecrosslist->headitem()))->GetX();
                 y=((kbNode*)(linecrosslist->headitem()))->GetY();
       xl=_LI->item()->beginnode->GetX();
                 yl=_LI->item()->beginnode->GetY();
                 sprintf(buf," x=%f , y=%f inserted before %f,%f",x,y,xl,yl);
                 _messagehandler->info(buf,"scanbeam");
             new_link->bin=true;
               }
               break;
           case 0:
             new_link->bin=true;
             returnlink=new_link;
               break;
           case 1:
             new_link->bin=false;
               break;
         }
 
         //insert a link into the graph that is already sorted on beginnodes of the links.
         //starting at a given position
         // if empty then just insert
 
         if (_LI->empty())
            _LI->insend(new_link);
         else
         {
            // put new item left of the one that is bigger are equal
            int i=0;
            int insert=0;
            while(!_LI->hitroot())
            {
               if ((insert=linkXYsorter(new_link,_LI->item()))!=-1)
                  break;
               (*_LI)++;
               i++;
            }
 
            _LI->insbefore_unsave(new_link);
            if (insert==0 && _LI->item()->beginnode!=new_link->beginnode)
             //the begin nodes are equal but not the same merge them into one node
            {  kbNode* todelete=_LI->item()->beginnode;
     new_link->beginnode->Merge(todelete);
     delete todelete;
            }
 
            //set back iter
            (*_LI) << (i+1);
         }
 
         lastinserted=(kbNode*)linecrosslist->headitem();
         linecrosslist->removehead();
      }
   }
   else
   {
      //re_use this link
      endnode->RemoveLink(this);
      linecrosslist->insend(endnode); //the last link to create is towards this node
      endnode=(kbNode*) linecrosslist->headitem();
      endnode->AddLink(this);
      inbeam=NodeXYsorter(_LI->item()->beginnode,endnode);
      switch (inbeam)
      {
        case -1:
        case 0:
            bin=true;
            break;
        case 1:
            bin=false;
            break;
      }
      returnlink=this;
 
      lastinserted=endnode;
      linecrosslist->removehead();
 
      // Make new links starting at endnode
      while (!linecrosslist->empty())
      {
         new_link=new kbLink(graphnum,lastinserted,(kbNode*) linecrosslist->headitem());
         new_link->group=group;
 
         inbeam=NodeXYsorter(_LI->item()->beginnode,(kbNode*) linecrosslist->headitem());
         switch (inbeam)
         {
           case -1:
           case 0:
             new_link->bin=true;
               break;
           case 1:
             new_link->bin=false;
               break;
         }
         inbeam=NodeXYsorter(_LI->item()->beginnode,lastinserted);
         switch (inbeam)
         {
           case -1:
               {
                 double x,y,xl,yl;
                 char buf[80];
                 x=lastinserted->GetX();
                 y=lastinserted->GetY();
       xl=_LI->item()->beginnode->GetX();
                 yl=_LI->item()->beginnode->GetY();
                 sprintf(buf," x=%f , y=%f inserted before %f,%f",x,y,xl,yl);
                 _messagehandler->info(buf,"scanbeam");
               }
               break;
           case 0:
               break;
           case 1:
             returnlink=new_link;
               break;
         }
 
         //insert a link into the graph that is already sorted on beginnodes of the links.
         //starting at a given position
         // if empty then just insert
 
         if (_LI->empty())
            _LI->insend(new_link);
         else
         {
            // put new item left of the one that is bigger are equal
            int i=0;
            int insert=0;
            while(!_LI->hitroot())
            {
               if ((insert=linkXYsorter(new_link,_LI->item()))!=-1)
                  break;
               (*_LI)++;
               i++;
            }
 
            _LI->insbefore_unsave(new_link);
            if (insert==0 && _LI->item()->beginnode!=new_link->beginnode)
             //the begin nodes are equal but not the same merge them into one node
            {  kbNode* todelete=_LI->item()->beginnode;
     new_link->beginnode->Merge(todelete);
     delete todelete;
            }
            //set back iter
            (*_LI) << (i+1);
         }
 
         lastinserted=(kbNode*)linecrosslist->headitem();
         linecrosslist->removehead();
      }
   }
 delete linecrosslist;
 linecrosslist=NULL;
 
   return returnlink;
}
*/

static int NODE_X_ASCENDING_L ( kbNode* a, kbNode* b )
{
    if( b->GetX() > a->GetX() ) return( 1 );
    else
        if( b->GetX() == a->GetX() ) return( 0 );

    return( -1 );
}

static int NODE_X_DESCENDING_L( kbNode* a, kbNode* b )
{
    if( a->GetX() > b->GetX() ) return( 1 );
    else
        if( a->GetX() == b->GetX() ) return( 0 );

    return( -1 );
}

static int NODE_Y_ASCENDING_L ( kbNode* a, kbNode* b )
{
    if( b->GetY() > a->GetY() ) return( 1 );
    else
        if( b->GetY() == a->GetY() ) return( 0 );
    return( -1 );
}

static int NODE_Y_DESCENDING_L( kbNode* a, kbNode* b )
{
    if( a->GetY() > b->GetY() ) return( 1 );
    else
        if( a->GetY() == b->GetY() ) return( 0 );

    return( -1 );
}

//
// This function finds out which sortfunction to use with sorting
// the crossings.
//
void kbLine::SortLineCrossings()
{
    TDLI<kbNode> I( linecrosslist );

    B_INT dx, dy;
    dx = babs( m_link->GetEndNode()->GetX() - m_link->GetBeginNode()->GetX() );
    dy = babs( m_link->GetEndNode()->GetY() - m_link->GetBeginNode()->GetY() );
    if ( dx > dy )
    { // thislink is more horizontal then vertical
        if ( m_link->GetEndNode()->GetX() > m_link->GetBeginNode()->GetX() )
            I.mergesort( NODE_X_ASCENDING_L );
        else
            I.mergesort( NODE_X_DESCENDING_L );
    }
    else
    { // this link is more vertical then horizontal
        if ( m_link->GetEndNode()->GetY() > m_link->GetBeginNode()->GetY() )
            I.mergesort( NODE_Y_ASCENDING_L );
        else
            I.mergesort( NODE_Y_DESCENDING_L );
    }
}

//
// Adds a cross Node to this. a_node may not be deleted before processing the crossings
//
void kbLine::AddCrossing( kbNode *a_node )
{
    if ( a_node == m_link->GetBeginNode() || a_node == m_link->GetEndNode() ) return;


    if ( !linecrosslist )
    {
        linecrosslist = new DL_List<void*>();
        linecrosslist->insend( a_node );
    }
    else
    {
        TDLI<kbNode> I( linecrosslist );
        if ( !I.has( a_node ) )
            I.insend( a_node );
    }
}

//
// see above
//
kbNode* kbLine::AddCrossing( B_INT X, B_INT Y )
{
    kbNode * result = new kbNode( X, Y, m_GC );
    AddCrossing( result );
    return result;
}

DL_List<void*>* kbLine::GetCrossList()
{
    if ( linecrosslist )
        return linecrosslist;
    return NULL;
}

bool kbLine::CrossListEmpty()
{
    if ( linecrosslist )
        return linecrosslist->empty();
    return true;
}

/*
bool kbLine::HasInCrossList(kbNode *n)
{
 if(linecrosslist!=NULL)
 {
  TDLI<kbNode> I(linecrosslist);
  return I.has(n);
 }
 return false;
}
*/

