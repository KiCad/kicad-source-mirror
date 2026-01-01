/*
    * Copyright The KiCad Developers.
    * Copyright (C) 2024 Fabien Corona f.corona<at>laposte.net
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

#include <drc/drc_creepage_utils.h>
#include <geometry/intersection.h>
#include <thread_pool.h>


bool segmentIntersectsArc( const VECTOR2I& p1, const VECTOR2I& p2, const VECTOR2I& center,
                           double radius, EDA_ANGLE startAngle, EDA_ANGLE endAngle,
                           std::vector<VECTOR2I>* aIntersectionPoints = nullptr )
{
    SEG       segment( p1, p2 );
    VECTOR2I  startPoint( radius * cos( startAngle.AsRadians() ), radius * sin( startAngle.AsRadians() ) );
    SHAPE_ARC arc( center, startPoint + center, endAngle - startAngle );

    INTERSECTABLE_GEOM geom1 = segment;
    INTERSECTABLE_GEOM geom2 = arc;

    if( aIntersectionPoints )
    {
        size_t startCount = aIntersectionPoints->size();

        INTERSECTION_VISITOR visitor( geom2, *aIntersectionPoints );
        std::visit( visitor, geom1 );

        return aIntersectionPoints->size() > startCount;
    }
    else
    {
        std::vector<VECTOR2I> intersectionPoints;

        INTERSECTION_VISITOR visitor( geom2, intersectionPoints );
        std::visit( visitor, geom1 );

        return intersectionPoints.size() > 0;
    }
}


//Check if line segments 'p1q1' and 'p2q2' intersect, excluding endpoint overlap

bool segments_intersect( const VECTOR2I& p1, const VECTOR2I& q1, const VECTOR2I& p2, const VECTOR2I& q2,
                         std::vector<VECTOR2I>& aIntersectionPoints )
{
    if( p1 == p2 || p1 == q2 || q1 == p2 || q1 == q2 )
        return false;

    SEG segment1( p1, q1 );
    SEG segment2( p2, q2 );

    INTERSECTABLE_GEOM geom1 = segment1;
    INTERSECTABLE_GEOM geom2 = segment2;

    size_t startCount = aIntersectionPoints.size();

    INTERSECTION_VISITOR visitor( geom2, aIntersectionPoints );
    std::visit( visitor, geom1 );

    return aIntersectionPoints.size() > startCount;
}


bool compareShapes( const CREEP_SHAPE* a, const CREEP_SHAPE* b )
{
    if( !a )
        return true;

    if( !b )
        return false;

    if( a->GetType() != b->GetType() )
        return a->GetType() < b->GetType();

    if( a->GetType() == CREEP_SHAPE::TYPE::UNDEFINED )
        return true;

    if( a->GetPos() != b->GetPos() )
        return a->GetPos() < b->GetPos();

    if( a->GetType() == CREEP_SHAPE::TYPE::CIRCLE )
        return a->GetRadius() < b->GetRadius();

    return false;
}


bool areEquivalent( const CREEP_SHAPE* a, const CREEP_SHAPE* b )
{
    if( !a && !b )
        return true;

    if( !a || !b )
        return false;

    if( a->GetType() != b->GetType() )
        return false;

    if( a->GetType() == CREEP_SHAPE::TYPE::POINT )
        return a->GetPos() == b->GetPos();

    if( a->GetType() == CREEP_SHAPE::TYPE::CIRCLE )
        return a->GetPos() == b->GetPos() && ( a->GetRadius() == b->GetRadius() );

    return false;
}


std::vector<PATH_CONNECTION> BE_SHAPE_POINT::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                    double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    double weight = ( this->GetPos() - aS2.GetPos() ).SquaredEuclideanNorm();

    if( weight > aMaxSquaredWeight )
        return result;

    PATH_CONNECTION pc;
    pc.a1 = this->GetPos();
    pc.a2 = aS2.GetPos();
    pc.weight = sqrt( weight );

    result.push_back( pc );
    return result;
}


std::vector<PATH_CONNECTION> BE_SHAPE_POINT::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                    double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    int                          radius = aS2.GetRadius();
    VECTOR2I                     pointPos = this->GetPos();
    VECTOR2I                     circleCenter = aS2.GetPos();

    if( radius <= 0 )
        return result;

    double pointToCenterDistanceSquared = ( pointPos - circleCenter ).SquaredEuclideanNorm();
    double weightSquared = pointToCenterDistanceSquared - (float) radius * (float) radius;

    if( weightSquared > aMaxSquaredWeight )
        return result;

    VECTOR2D direction1 = VECTOR2D( pointPos.x - circleCenter.x, pointPos.y - circleCenter.y );
    direction1 = direction1.Resize( 1 );

    VECTOR2D direction2 = direction1.Perpendicular();

    double radiusSquared = double( radius ) * double( radius );

    double distance = sqrt( pointToCenterDistanceSquared );
    double value1 = radiusSquared / distance;
    double value2 = sqrt( radiusSquared - value1 * value1 );

    VECTOR2D resultPoint;

    PATH_CONNECTION pc;
    pc.a1 = pointPos;
    pc.weight = sqrt( weightSquared );

    resultPoint = direction1 * value1 + direction2 * value2 + circleCenter;
    pc.a2.x = int( resultPoint.x );
    pc.a2.y = int( resultPoint.y );
    result.push_back( pc );

    resultPoint = direction1 * value1 - direction2 * value2 + circleCenter;
    pc.a2.x = int( resultPoint.x );
    pc.a2.y = int( resultPoint.y );
    result.push_back( pc );

    return result;
}


std::pair<bool, bool> BE_SHAPE_ARC::IsThereATangentPassingThroughPoint( const BE_SHAPE_POINT aPoint ) const
{
    std::pair<bool, bool> result;
    double                R = m_radius;

    VECTOR2I newPoint = aPoint.GetPos() - m_pos;

    if( newPoint.SquaredEuclideanNorm() <= R * R )
    {
        // If the point is inside the arc
        result.first = false;
        result.second = false;
        return result;
    }

    EDA_ANGLE testAngle = AngleBetweenStartAndEnd( aPoint.GetPos() );

    double startAngle = m_startAngle.AsRadians();
    double endAngle = m_endAngle.AsRadians();
    double pointAngle = testAngle.AsRadians();

    bool greaterThan180 = ( m_endAngle - m_startAngle ) > EDA_ANGLE( 180 );
    bool connectToEndPoint;

    connectToEndPoint = ( cos( startAngle ) * newPoint.x + sin( startAngle ) * newPoint.y >= R );

    if( greaterThan180 )
        connectToEndPoint &= ( cos( endAngle ) * newPoint.x + sin( endAngle ) * newPoint.y <= R );

    connectToEndPoint |= ( cos( endAngle ) * newPoint.x + sin( endAngle ) * newPoint.y <= R )
                         && ( pointAngle >= endAngle || pointAngle <= startAngle );

    result.first = !connectToEndPoint;

    connectToEndPoint = ( cos( endAngle ) * newPoint.x + sin( endAngle ) * newPoint.y >= R );

    if( greaterThan180 )
        connectToEndPoint &= ( cos( startAngle ) * newPoint.x + sin( startAngle ) * newPoint.y <= R );

    connectToEndPoint |= ( cos( startAngle ) * newPoint.x + sin( startAngle ) * newPoint.y <= R )
                         && ( pointAngle >= endAngle || pointAngle <= startAngle );

    result.second = !connectToEndPoint;
    return result;
}


std::vector<PATH_CONNECTION> BE_SHAPE_POINT::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                    double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     center = aS2.GetPos();
    double                       radius = aS2.GetRadius();

    // First path tries to connect to start point
    // Second path tries to connect to end point
    std::pair<bool, bool> behavesLikeCircle;
    behavesLikeCircle = aS2.IsThereATangentPassingThroughPoint( *this );

    if( behavesLikeCircle.first && behavesLikeCircle.second )
    {
        BE_SHAPE_CIRCLE csc( center, radius );
        return this->Paths( csc, aMaxWeight, aMaxSquaredWeight );
    }

    if( behavesLikeCircle.first )
    {
        BE_SHAPE_CIRCLE              csc( center, radius );
        std::vector<PATH_CONNECTION> paths = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

        if( paths.size() > 1 ) // Point to circle creates either 0 or 2 connections
            result.push_back( paths[1] );
    }
    else
    {
        BE_SHAPE_POINT csp1( aS2.GetStartPoint() );

        for( const PATH_CONNECTION& pc : this->Paths( csp1, aMaxWeight, aMaxSquaredWeight ) )
            result.push_back( pc );
    }

    if( behavesLikeCircle.second )
    {
        BE_SHAPE_CIRCLE              csc( center, radius );
        std::vector<PATH_CONNECTION> paths = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

        if( paths.size() > 1 ) // Point to circle creates either 0 or 2 connections
            result.push_back( paths[0] );
    }
    else
    {
        BE_SHAPE_POINT csp1( aS2.GetEndPoint() );

        for( const PATH_CONNECTION& pc : this->Paths( csp1, aMaxWeight, aMaxSquaredWeight ) )
            result.push_back( pc );
    }

    return result;
}

std::vector<PATH_CONNECTION> BE_SHAPE_CIRCLE::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     circleCenter = this->GetPos();
    double                       circleRadius = this->GetRadius();
    VECTOR2I                     arcCenter = aS2.GetPos();
    double                       arcRadius = aS2.GetRadius();
    EDA_ANGLE                    arcStartAngle = aS2.GetStartAngle();
    EDA_ANGLE                    arcEndAngle = aS2.GetEndAngle();

    double centerDistance = ( circleCenter - arcCenter ).EuclideanNorm();

    if( centerDistance + arcRadius < circleRadius )
    {
        // The arc is inside the circle
        return result;
    }

    BE_SHAPE_POINT  csp1( aS2.GetStartPoint() );
    BE_SHAPE_POINT  csp2( aS2.GetEndPoint() );
    BE_SHAPE_CIRCLE csc( arcCenter, arcRadius );

    for( const PATH_CONNECTION& pc : this->Paths( csc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE pointAngle = aS2.AngleBetweenStartAndEnd( pc.a2 - arcCenter );

        if( pointAngle <= aS2.GetEndAngle() )
            result.push_back( pc );
    }

    if( result.size() == 4 )
    {
        // It behaved as a circle
        return result;
    }

    for( const BE_SHAPE_POINT& csp : { csp1, csp2 } )
    {
        for( const PATH_CONNECTION& pc : this->Paths( csp, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, arcCenter, arcRadius, arcStartAngle, arcEndAngle ) )
                result.push_back( pc );
        }
    }

    return result;
}


std::vector<PATH_CONNECTION> BE_SHAPE_ARC::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     circleCenter = this->GetPos();
    double                       circleRadius = this->GetRadius();
    VECTOR2I                     arcCenter = aS2.GetPos();
    double                       arcRadius = aS2.GetRadius();

    double centerDistance = ( circleCenter - arcCenter ).EuclideanNorm();

    if( centerDistance + arcRadius < circleRadius )
    {
        // The arc is inside the circle
        return result;
    }

    BE_SHAPE_POINT  csp1( aS2.GetStartPoint() );
    BE_SHAPE_POINT  csp2( aS2.GetEndPoint() );
    BE_SHAPE_CIRCLE csc( arcCenter, arcRadius );


    for( const PATH_CONNECTION& pc : this->Paths( BE_SHAPE_CIRCLE( aS2.GetPos(), aS2.GetRadius() ),
                                                  aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE pointAngle = aS2.AngleBetweenStartAndEnd( pc.a2 - arcCenter );

        if( pointAngle <= aS2.GetEndAngle() )
            result.push_back( pc );
    }

    for( const PATH_CONNECTION& pc : BE_SHAPE_CIRCLE( this->GetPos(), this->GetRadius() )
                                          .Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE pointAngle = this->AngleBetweenStartAndEnd( pc.a2 - arcCenter );

        if( pointAngle <= this->GetEndAngle() )
            result.push_back( pc );
    }

    return result;
}


std::vector<PATH_CONNECTION> BE_SHAPE_CIRCLE::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    VECTOR2I p1 = this->GetPos();
    VECTOR2I p2 = aS2.GetPos();

    VECTOR2D distSquared( double( ( p2 - p1 ).x ), double( ( p2 - p1 ).y ) );
    double   weightSquared = distSquared.SquaredEuclideanNorm();

    double R1 = this->GetRadius();
    double R2 = aS2.GetRadius();

    double Rdiff = abs( R1 - R2 );
    double Rsum = R1 + R2;

    // "Straight" paths
    double weightSquared1 = weightSquared - Rdiff * Rdiff;
    // "Crossed" paths
    double weightSquared2 = weightSquared - Rsum * Rsum;

    if( weightSquared1 <= aMaxSquaredWeight )
    {
        VECTOR2D direction1 = VECTOR2D( p2.x - p1.x, p2.y - p1.y );
        direction1 = direction1.Resize( 1 );
        VECTOR2D direction2 = direction1.Perpendicular();

        double D = sqrt( weightSquared );
        double ratio1 = ( R1 - R2 ) / D;
        double ratio2 = sqrt( 1 - ratio1 * ratio1 );


        PATH_CONNECTION pc;
        pc.weight = sqrt( weightSquared1 );

        pc.a1 = p1 + direction1 * R1 * ratio1 + direction2 * R1 * ratio2;
        pc.a2 = p2 + direction1 * R2 * ratio1 + direction2 * R2 * ratio2;

        result.push_back( pc );

        pc.a1 = p1 + direction1 * R1 * ratio1 - direction2 * R1 * ratio2;
        pc.a2 = p2 + direction1 * R2 * ratio1 - direction2 * R2 * ratio2;

        result.push_back( pc );
    }
    if( weightSquared2 <= aMaxSquaredWeight )
    {
        VECTOR2D direction1 = VECTOR2D( p2.x - p1.x, p2.y - p1.y );
        direction1 = direction1.Resize( 1 );
        VECTOR2D direction2 = direction1.Perpendicular();

        double D = sqrt( weightSquared );
        double ratio1 = ( R1 + R2 ) / D;
        double ratio2 = sqrt( 1 - ratio1 * ratio1 );


        PATH_CONNECTION pc;
        pc.weight = sqrt( weightSquared2 );

        pc.a1 = p1 + direction1 * R1 * ratio1 + direction2 * R1 * ratio2;
        pc.a2 = p2 - direction1 * R2 * ratio1 - direction2 * R2 * ratio2;

        result.push_back( pc );

        pc.a1 = p1 + direction1 * R1 * ratio1 - direction2 * R1 * ratio2;
        pc.a2 = p2 - direction1 * R2 * ratio1 + direction2 * R2 * ratio2;

        result.push_back( pc );
    }

    return result;
}


void CREEPAGE_GRAPH::TransformCreepShapesToNodes( std::vector<CREEP_SHAPE*>& aShapes )
{
    for( CREEP_SHAPE* p1 : aShapes )
    {
        if( !p1 )
            continue;

        switch( p1->GetType() )
        {
        case CREEP_SHAPE::TYPE::POINT:  AddNode( GRAPH_NODE::TYPE::POINT, p1, p1->GetPos() );  break;
        case CREEP_SHAPE::TYPE::CIRCLE: AddNode( GRAPH_NODE::TYPE::CIRCLE, p1, p1->GetPos() ); break;
        case CREEP_SHAPE::TYPE::ARC:    AddNode( GRAPH_NODE::TYPE::ARC, p1, p1->GetPos() );    break;
        default:                                                                               break;
        }
    }
}

void CREEPAGE_GRAPH::RemoveDuplicatedShapes()
{
    // Sort the vector
    sort( m_shapeCollection.begin(), m_shapeCollection.end(), compareShapes );
    std::vector<CREEP_SHAPE*> newVector;

    size_t i = 0;

    for( i = 0; i < m_shapeCollection.size() - 1; i++ )
    {
        if( m_shapeCollection[i] == nullptr )
            continue;

        if( areEquivalent( m_shapeCollection[i], m_shapeCollection[i + 1] ) )
        {
            delete m_shapeCollection[i];
            m_shapeCollection[i] = nullptr;
        }
        else
        {
            newVector.push_back( m_shapeCollection[i] );
        }
    }

    if( m_shapeCollection[i] )
        newVector.push_back( m_shapeCollection[i] );

    std::swap( m_shapeCollection, newVector );
}

void CREEPAGE_GRAPH::TransformEdgeToCreepShapes()
{
    for( BOARD_ITEM* drawing : m_boardEdge )
    {
        PCB_SHAPE* d = dynamic_cast<PCB_SHAPE*>( drawing );

        if( !d )
            continue;

        switch( d->GetShape() )
        {
        case SHAPE_T::SEGMENT:
        {
            BE_SHAPE_POINT* a = new BE_SHAPE_POINT( d->GetStart() );
            m_shapeCollection.push_back( a );
            a = new BE_SHAPE_POINT( d->GetEnd() );
            m_shapeCollection.push_back( a );
            break;
        }

        case SHAPE_T::RECTANGLE:
        {
            BE_SHAPE_POINT* a = new BE_SHAPE_POINT( d->GetStart() );
            m_shapeCollection.push_back( a );
            a = new BE_SHAPE_POINT( d->GetEnd() );
            m_shapeCollection.push_back( a );
            a = new BE_SHAPE_POINT( VECTOR2I( d->GetEnd().x, d->GetStart().y ) );
            m_shapeCollection.push_back( a );
            a = new BE_SHAPE_POINT( VECTOR2I( d->GetStart().x, d->GetEnd().y ) );
            m_shapeCollection.push_back( a );
            break;
        }

        case SHAPE_T::POLY:
            for( const VECTOR2I& p : d->GetPolyPoints() )
            {
                BE_SHAPE_POINT* a = new BE_SHAPE_POINT( p );
                m_shapeCollection.push_back( a );
            }

            break;

        case SHAPE_T::CIRCLE:
        {
            BE_SHAPE_CIRCLE* a = new BE_SHAPE_CIRCLE( d->GetCenter(), d->GetRadius() );
            a->SetParent( d );
            m_shapeCollection.push_back( a );
            break;
        }

        case SHAPE_T::ARC:
        {
            // If the arc is not locally convex, only use the endpoints
            double   tolerance = 10;
            VECTOR2D center( double( d->GetCenter().x ), double( d->GetCenter().y ) );
            VECTOR2D mid( double( d->GetArcMid().x ), double( d->GetArcMid().y ) );
            VECTOR2D dir( mid - center );
            dir = dir / d->GetRadius() * ( d->GetRadius() - tolerance );

            EDA_ANGLE alpha, beta;
            d->CalcArcAngles( alpha, beta );
            BE_SHAPE_ARC* a = new BE_SHAPE_ARC( d->GetCenter(), d->GetRadius(), alpha, beta,
                                                d->GetStart(), d->GetEnd() );
            a->SetParent( d );

            m_shapeCollection.push_back( a );
            break;
        }

        default:
            break;
        }
    }
}


void GRAPH_CONNECTION::GetShapes( std::vector<PCB_SHAPE>& aShapes )
{
    if( !m_path.m_show )
        return;

    if( !n1 || !n2 )
        return;

    if( n1->m_type == GRAPH_NODE::TYPE::VIRTUAL || n2->m_type == GRAPH_NODE::TYPE::VIRTUAL )
        return;

    if( !m_forceStraightLine && n1->m_parent
            && n1->m_parent == n2->m_parent
            && n1->m_parent->GetType() == CREEP_SHAPE::TYPE::CIRCLE )
    {
        VECTOR2I  center = n1->m_parent->GetPos();
        VECTOR2I  R1 = n1->m_pos - center;
        VECTOR2I  R2 = n2->m_pos - center;
        PCB_SHAPE s( nullptr, SHAPE_T::ARC );

        if( R1.Cross( R2 ) > 0 )
        {
            s.SetStart( n1->m_pos );
            s.SetEnd( n2->m_pos );
        }
        else
        {
            s.SetStart( n2->m_pos );
            s.SetEnd( n1->m_pos );
        }

        s.SetCenter( center );
        aShapes.push_back( s );
        return;
    }

    if( !m_forceStraightLine && n1->m_parent
            && n1->m_parent == n2->m_parent
            && n1->m_parent->GetType() == CREEP_SHAPE::TYPE::ARC )
    {
        if( BE_SHAPE_ARC* arc = dynamic_cast<BE_SHAPE_ARC*>( n1->m_parent ) )
        {
            VECTOR2I  center = arc->GetPos();
            VECTOR2I  R1 = n1->m_pos - center;
            VECTOR2I  R2 = n2->m_pos - center;
            PCB_SHAPE s( nullptr, SHAPE_T::ARC );

            if( R1.Cross( R2 ) > 0 )
            {
                s.SetStart( n1->m_pos );
                s.SetEnd( n2->m_pos );
            }
            else
            {
                s.SetStart( n2->m_pos );
                s.SetEnd( n1->m_pos );
            }

            s.SetCenter( center );

            //Check that we are on the correct side of the arc.
            VECTOR2I  mid = s.GetArcMid();
            EDA_ANGLE midAngle = arc->AngleBetweenStartAndEnd( mid );

            if( midAngle > arc->GetEndAngle() )
            {
                VECTOR2I tmp;
                tmp = s.GetStart();
                s.SetStart( s.GetEnd() );
                s.SetEnd( tmp );
                s.SetCenter( center );
            }

            aShapes.push_back( s );
            return;
        }
    }

    PCB_SHAPE s( nullptr, SHAPE_T::SEGMENT );
    s.SetStart( m_path.a1 );
    s.SetEnd( m_path.a2 );
    aShapes.push_back( s );
}


void CREEP_SHAPE::ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>&,
                                   CREEPAGE_GRAPH&              aG ) const
{
}


void BE_SHAPE_POINT::ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>&,
                                      CREEPAGE_GRAPH&              aG ) const
{
}


void BE_SHAPE_CIRCLE::ShortenChildDueToGV( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                                           CREEPAGE_GRAPH& aG, double aNormalWeight ) const
{
    EDA_ANGLE angle1 = EDA_ANGLE( a1->m_pos - m_pos );
    EDA_ANGLE angle2 = EDA_ANGLE( a2->m_pos - m_pos );

    while( angle1 < ANGLE_0 )
        angle1 += ANGLE_360;
    while( angle2 < ANGLE_0 )
        angle2 += ANGLE_360;
    while( angle1 > ANGLE_360 )
        angle1 -= ANGLE_360;
    while( angle2 > ANGLE_360 )
        angle2 -= ANGLE_360;

    EDA_ANGLE maxAngle = angle1 > angle2 ? angle1 : angle2;
    EDA_ANGLE skipAngle =
            EDA_ANGLE( asin( float( aG.m_minGrooveWidth ) / ( 2 * m_radius ) ), RADIANS_T );
    skipAngle += skipAngle; // Cannot multiply EDA_ANGLE by scalar, but this really is angle *2
    EDA_ANGLE pointAngle = maxAngle - skipAngle;

    VECTOR2I skipPoint = m_pos;
    skipPoint.x += m_radius * cos( pointAngle.AsRadians() );
    skipPoint.y += m_radius * sin( pointAngle.AsRadians() );

    std::shared_ptr<GRAPH_NODE> gnt = aG.AddNode( GRAPH_NODE::POINT, a1->m_parent, skipPoint );

    PATH_CONNECTION pc;

    pc.a1 = maxAngle == angle2 ? a1->m_pos : a2->m_pos;
    pc.a2 = skipPoint;
    pc.weight = aNormalWeight - aG.m_minGrooveWidth;
    aG.AddConnection( maxAngle == angle2 ? a1 : a2, gnt, pc );

    pc.a1 = skipPoint;
    pc.a2 = maxAngle == angle2 ? a2->m_pos : a1->m_pos;
    pc.weight = aG.m_minGrooveWidth;

    std::shared_ptr<GRAPH_CONNECTION> gc = aG.AddConnection( gnt, maxAngle == angle2 ? a2 : a1, pc );

    if( gc )
        gc->m_forceStraightLine = true;
}


void BE_SHAPE_CIRCLE::ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                                       CREEPAGE_GRAPH& aG ) const
{
    if( !a1 || !a2 )
        return;

    if( m_radius == 0 )
        return;

    VECTOR2D distI( a1->m_pos - a2->m_pos );
    VECTOR2D distD( double( distI.x ), double( distI.y ) );

    double weight = m_radius * 2 * asin( distD.EuclideanNorm() / ( 2.0 * m_radius ) );

    if( weight > aG.GetTarget() )
        return;

    if( aG.m_minGrooveWidth <= 0 )
    {
        PATH_CONNECTION pc;
        pc.a1 = a1->m_pos;
        pc.a2 = a2->m_pos;
        pc.weight = std::max( weight, 0.0 );

        aG.AddConnection( a1, a2, pc );
        return;
    }

    if( weight > aG.m_minGrooveWidth )
        ShortenChildDueToGV( a1, a2, aG, weight );
    // Else well.. this paths will be "shorted" by another one
}


void BE_SHAPE_ARC::ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                                    CREEPAGE_GRAPH& aG ) const
{
    if( !a1 || !a2 )
        return;

    EDA_ANGLE angle1 = AngleBetweenStartAndEnd( a1->m_pos );
    EDA_ANGLE angle2 = AngleBetweenStartAndEnd( a2->m_pos );

    double weight = abs( m_radius * ( angle2 - angle1 ).AsRadians() );

    if( true || aG.m_minGrooveWidth <= 0 )
    {
        if( ( weight > aG.GetTarget() ) )
            return;

        PATH_CONNECTION pc;
        pc.a1 = a1->m_pos;
        pc.a2 = a2->m_pos;
        pc.weight = weight;

        aG.AddConnection( a1, a2, pc );
        return;
    }

    if( weight > aG.m_minGrooveWidth )
        ShortenChildDueToGV( a1, a2, aG, weight );
}


void CREEPAGE_GRAPH::SetTarget( double aTarget )
{
    m_creepageTarget = aTarget;
    m_creepageTargetSquared = aTarget * aTarget;
}


std::vector<PATH_CONNECTION> CU_SHAPE_SEGMENT::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     start = this->GetStart();
    VECTOR2I                     end = this->GetEnd();
    double                       halfWidth = this->GetWidth() / 2;
    EDA_ANGLE                    trackAngle( end - start );
    VECTOR2I                     pointPos = aS2.GetPos();

    double length = ( start - end ).EuclideanNorm();
    double projectedPos = cos( trackAngle.AsRadians() ) * ( pointPos.x - start.x )
                          + sin( trackAngle.AsRadians() ) * ( pointPos.y - start.y );

    VECTOR2I newPoint;

    if( projectedPos <= 0 )
    {
        newPoint = start + ( pointPos - start ).Resize( halfWidth );
    }
    else if( projectedPos >= length )
    {
        newPoint = end + ( pointPos - end ).Resize( halfWidth );
    }
    else
    {
        double posOnSegment = ( start - pointPos ).SquaredEuclideanNorm()
                              - ( end - pointPos ).SquaredEuclideanNorm();
        posOnSegment = posOnSegment / ( 2 * length ) + length / 2;

        newPoint = start + ( end - start ).Resize( posOnSegment );
        newPoint += ( pointPos - newPoint ).Resize( halfWidth );
    }

    double weightSquared = ( pointPos - newPoint ).SquaredEuclideanNorm();

    if( weightSquared > aMaxSquaredWeight )
        return result;

    PATH_CONNECTION pc;
    pc.a1 = newPoint;
    pc.a2 = pointPos;
    pc.weight = sqrt( weightSquared );

    result.push_back( pc );
    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_SEGMENT::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     start = this->GetStart();
    VECTOR2I                     end = this->GetEnd();
    double                       halfWidth = this->GetWidth() / 2;

    double    circleRadius = aS2.GetRadius();
    VECTOR2I  circleCenter = aS2.GetPos();
    double    length = ( start - end ).EuclideanNorm();
    EDA_ANGLE trackAngle( end - start );

    double   weightSquared = std::numeric_limits<double>::infinity();
    VECTOR2I PointOnTrack, PointOnCircle;

    // There are two possible paths
    // First the one on the side of the start of the track.
    double projectedPos1 = cos( trackAngle.AsRadians() ) * ( circleCenter.x - start.x )
                           + sin( trackAngle.AsRadians() ) * ( circleCenter.y - start.y );
    double projectedPos2 = projectedPos1 + circleRadius;
    projectedPos1 = projectedPos1 - circleRadius;

    double trackSide = ( end - start ).Cross( circleCenter - start ) > 0 ? 1 : -1;

    if( ( projectedPos1 < 0 && projectedPos2 < 0 ) )
    {
        CU_SHAPE_CIRCLE csc( start, halfWidth );
        for( PATH_CONNECTION pc : csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
        {
            result.push_back( pc );
        }
    }
    else if( ( projectedPos1 > length && projectedPos2 > length ) )
    {
        CU_SHAPE_CIRCLE csc( end, halfWidth );

        for( const PATH_CONNECTION& pc : csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
            result.push_back( pc );
    }

    else if( ( projectedPos1 >= 0 ) && ( projectedPos1 <= length ) && ( projectedPos2 >= 0 )
             && ( projectedPos2 <= length ) )
    {
        // Both point connects to the segment part of the track
        PointOnTrack = start;
        PointOnTrack += ( end - start ).Resize( projectedPos1 );
        PointOnTrack += ( end - start ).Perpendicular().Resize( halfWidth ) * trackSide;
        PointOnCircle = circleCenter - ( end - start ).Resize( circleRadius );
        weightSquared = ( PointOnCircle - PointOnTrack ).SquaredEuclideanNorm();

        if( weightSquared < aMaxSquaredWeight )
        {
            PATH_CONNECTION pc;
            pc.a1 = PointOnTrack;
            pc.a2 = PointOnCircle;
            pc.weight = sqrt( weightSquared );

            result.push_back( pc );

            PointOnTrack = start;
            PointOnTrack += ( end - start ).Resize( projectedPos2 );
            PointOnTrack += ( end - start ).Perpendicular().Resize( halfWidth ) * trackSide;
            PointOnCircle = circleCenter + ( end - start ).Resize( circleRadius );


            pc.a1 = PointOnTrack;
            pc.a2 = PointOnCircle;

            result.push_back( pc );
        }
    }
    else if( ( ( projectedPos1 >= 0 ) && ( projectedPos1 <= length ) )
             && ( ( projectedPos2 > length ) || projectedPos2 < 0 ) )
    {
        CU_SHAPE_CIRCLE              csc( end, halfWidth );
        std::vector<PATH_CONNECTION> pcs = csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

        if( pcs.size() < 2 )
            return result;

        result.push_back( pcs.at( trackSide == 1 ? 1 : 0 ) );


        PointOnTrack = start;
        PointOnTrack += ( end - start ).Resize( projectedPos1 );
        PointOnTrack += ( end - start ).Perpendicular().Resize( halfWidth ) * trackSide;
        PointOnCircle = circleCenter - ( end - start ).Resize( circleRadius );
        weightSquared = ( PointOnCircle - PointOnTrack ).SquaredEuclideanNorm();

        if( weightSquared < aMaxSquaredWeight )
        {
            PATH_CONNECTION pc;
            pc.a1 = PointOnTrack;
            pc.a2 = PointOnCircle;
            pc.weight = sqrt( weightSquared );

            result.push_back( pc );
        }
    }
    else if( ( ( projectedPos2 >= 0 ) && ( projectedPos2 <= length ) )
             && ( ( projectedPos1 > length ) || projectedPos1 < 0 ) )
    {
        CU_SHAPE_CIRCLE              csc( start, halfWidth );
        std::vector<PATH_CONNECTION> pcs = csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

        if( pcs.size() < 2 )
            return result;

        result.push_back( pcs.at( trackSide == 1 ? 0 : 1 ) );

        PointOnTrack = start;
        PointOnTrack += ( end - start ).Resize( projectedPos2 );
        PointOnTrack += ( end - start ).Perpendicular().Resize( halfWidth ) * trackSide;
        PointOnCircle = circleCenter + ( end - start ).Resize( circleRadius );
        weightSquared = ( PointOnCircle - PointOnTrack ).SquaredEuclideanNorm();

        if( weightSquared < aMaxSquaredWeight )
        {
            PATH_CONNECTION pc;
            pc.a1 = PointOnTrack;
            pc.a2 = PointOnCircle;
            pc.weight = sqrt( weightSquared );

            result.push_back( pc );
        }
    }

    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_SEGMENT::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    BE_SHAPE_CIRCLE bsc( aS2.GetPos(), aS2.GetRadius() );

    for( const PATH_CONNECTION& pc : this->Paths( bsc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < aS2.GetEndAngle() )
            result.push_back( pc );
    }

    if( result.size() < 2 )
    {
        BE_SHAPE_POINT bsp1( aS2.GetStartPoint() );
        BE_SHAPE_POINT bsp2( aS2.GetEndPoint() );

        VECTOR2I  beArcPos = aS2.GetPos();
        int       beArcRadius = aS2.GetRadius();
        EDA_ANGLE beArcStartAngle = aS2.GetStartAngle();
        EDA_ANGLE beArcEndAngle = aS2.GetEndAngle();

        for( const PATH_CONNECTION& pc : this->Paths( bsp1, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius, beArcStartAngle, beArcEndAngle ) )
                result.push_back( pc );
        }

        for( const PATH_CONNECTION& pc : this->Paths( bsp2, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius, beArcStartAngle, beArcEndAngle ) )
                result.push_back( pc );
        }
    }

    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_CIRCLE::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     beArcPos = aS2.GetPos();
    int                          beArcRadius = aS2.GetRadius();
    EDA_ANGLE                    beArcStartAngle = aS2.GetStartAngle();
    EDA_ANGLE                    beArcEndAngle = aS2.GetEndAngle();

    BE_SHAPE_CIRCLE bsc( beArcPos, beArcRadius );

    for( const PATH_CONNECTION& pc : this->Paths( bsc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < aS2.GetEndAngle() )
            result.push_back( pc );
    }

    if( result.size() < 2 )
    {
        BE_SHAPE_POINT bsp1( aS2.GetStartPoint() );
        BE_SHAPE_POINT bsp2( aS2.GetEndPoint() );

        for( const PATH_CONNECTION& pc : this->Paths( bsp1, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius, beArcStartAngle, beArcEndAngle ) )
                result.push_back( pc );
        }

        for( const PATH_CONNECTION& pc : this->Paths( bsp2, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius, beArcStartAngle, beArcEndAngle ) )
                result.push_back( pc );
        }

    }
    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_ARC::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    CU_SHAPE_CIRCLE csc( this->GetPos(), this->GetRadius() + this->GetWidth() / 2 );

    for( const PATH_CONNECTION& pc : this->Paths( csc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = this->AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < this->GetEndAngle() )
            result.push_back( pc );
    }

    if( result.size() < 2 )
    {
        CU_SHAPE_CIRCLE csc1( this->GetStartPoint(), this->GetWidth() / 2 );
        CU_SHAPE_CIRCLE csc2( this->GetEndPoint(), this->GetWidth() / 2 );

        for( const PATH_CONNECTION& pc : this->Paths( csc1, aMaxWeight, aMaxSquaredWeight ) )
            result.push_back( pc );

        for( const PATH_CONNECTION& pc : this->Paths( csc2, aMaxWeight, aMaxSquaredWeight ) )
            result.push_back( pc );
    }

    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_ARC::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     beArcPos = aS2.GetPos();
    int                          beArcRadius = aS2.GetRadius();
    EDA_ANGLE                    beArcStartAngle = aS2.GetStartAngle();
    EDA_ANGLE                    beArcEndAngle = aS2.GetEndAngle();

    BE_SHAPE_CIRCLE bsc( aS2.GetPos(), aS2.GetRadius() );

    for( const PATH_CONNECTION& pc : this->Paths( bsc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < aS2.GetEndAngle() )
            result.push_back( pc );
    }

    if( result.size() < 2 )
    {
        BE_SHAPE_POINT bsp1( aS2.GetStartPoint() );
        BE_SHAPE_POINT bsp2( aS2.GetEndPoint() );

        for( const PATH_CONNECTION& pc : this->Paths( bsp1, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius, beArcStartAngle, beArcEndAngle ) )
                result.push_back( pc );
        }

        for( const PATH_CONNECTION& pc : this->Paths( bsp2, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius, beArcStartAngle, beArcEndAngle ) )
                result.push_back( pc );
        }
    }

    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_CIRCLE::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    double   R = this->GetRadius();
    VECTOR2I center = this->GetPos();
    VECTOR2I point = aS2.GetPos();
    double   weight = ( center - point ).EuclideanNorm() - R;

    if( weight > aMaxWeight )
        return result;

    PATH_CONNECTION pc;
    pc.weight = std::max( weight, 0.0 );
    pc.a2 = point;
    pc.a1 = center + ( point - center ).Resize( R );

    result.push_back( pc );
    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_CIRCLE::Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    double   R1 = this->GetRadius();
    double   R2 = aS2.GetRadius();
    VECTOR2I C1 = this->GetPos();
    VECTOR2I C2 = aS2.GetPos();

    if( ( C1 - C2 ).SquaredEuclideanNorm() < ( R1 - R2 ) * ( R1 - R2 ) )
    {
        // One of the circles is inside the other
        return result;
    }

    double weight = ( C1 - C2 ).EuclideanNorm() - R1 - R2;

    if( weight > aMaxWeight || weight < 0 )
        return result;

    PATH_CONNECTION pc;
    pc.weight = std::max( weight, 0.0 );
    pc.a1 = ( C2 - C1 ).Resize( R1 ) + C1;
    pc.a2 = ( C1 - C2 ).Resize( R2 ) + C2;
    result.push_back( pc );
    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_SEGMENT::Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    VECTOR2I s_start = this->GetStart();
    VECTOR2I s_end = this->GetEnd();
    double   halfWidth = this->GetWidth() / 2;

    EDA_ANGLE trackAngle( s_end - s_start );
    VECTOR2I  pointPos = aS2.GetPos();

    double length = ( s_start - s_end ).EuclideanNorm();
    double projectedPos = cos( trackAngle.AsRadians() ) * ( pointPos.x - s_start.x )
                          + sin( trackAngle.AsRadians() ) * ( pointPos.y - s_start.y );

    if( ( projectedPos <= 0 ) || ( s_start == s_end ) )
    {
        CU_SHAPE_CIRCLE csc( s_start, halfWidth );
        return csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight );
    }

    if( projectedPos >= length )
    {
        CU_SHAPE_CIRCLE csc( s_end, halfWidth );
        return csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight );
    }

    double radius = aS2.GetRadius();
    double trackSide = ( s_end - s_start ).Cross( pointPos - s_start ) > 0 ? 1 : -1;

    PATH_CONNECTION pc;
    pc.a1 = s_start + ( s_end - s_start ).Resize( projectedPos )
            + ( s_end - s_start ).Perpendicular().Resize( halfWidth ) * trackSide;
    pc.a2 = ( pc.a1 - pointPos ).Resize( radius ) + pointPos;
    pc.weight = ( pc.a2 - pc.a1 ).SquaredEuclideanNorm();

    if( pc.weight <= aMaxSquaredWeight )
    {
        pc.weight = sqrt( pc.weight );
        result.push_back( pc );
    }

    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_CIRCLE::Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    VECTOR2I circlePos = this->GetPos();
    VECTOR2I arcPos = aS2.GetPos();

    double circleRadius = this->GetRadius();
    double arcRadius = aS2.GetRadius();

    VECTOR2I startPoint = aS2.GetStartPoint();
    VECTOR2I endPoint = aS2.GetEndPoint();

    CU_SHAPE_CIRCLE csc( arcPos, arcRadius + aS2.GetWidth() / 2 );

    if( ( circlePos - arcPos ).EuclideanNorm() > arcRadius + circleRadius )
    {
        const std::vector<PATH_CONNECTION>& pcs = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

        if( pcs.size() == 1 )
        {
            EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( pcs[0].a2 );

            if( testAngle < aS2.GetEndAngle() )
            {
                result.push_back( pcs[0] );
                return result;
            }
        }
    }

    CU_SHAPE_CIRCLE csc1( startPoint, aS2.GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc2( endPoint, aS2.GetWidth() / 2 );

    PATH_CONNECTION* bestPath = nullptr;


    std::vector<PATH_CONNECTION> pcs1 = this->Paths( csc1, aMaxWeight, aMaxSquaredWeight );
    std::vector<PATH_CONNECTION> pcs2 = this->Paths( csc2, aMaxWeight, aMaxSquaredWeight );

    for( PATH_CONNECTION& pc : pcs1 )
    {
        if( !bestPath || ( ( bestPath->weight > pc.weight ) && ( pc.weight > 0 ) ) )
            bestPath = &pc;
    }

    for( PATH_CONNECTION& pc : pcs2 )
    {
        if( !bestPath || ( ( bestPath->weight > pc.weight ) && ( pc.weight > 0 ) ) )
            bestPath = &pc;
    }

    // If the circle center is insde the arc ring

    PATH_CONNECTION pc3;

    if( ( circlePos - arcPos ).SquaredEuclideanNorm() < arcRadius * arcRadius )
    {
        if( circlePos != arcPos ) // The best path is already found otherwise
        {
            EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( circlePos );

            if( testAngle < aS2.GetEndAngle() )
            {
                pc3.weight = std::max( arcRadius - ( circlePos - arcPos ).EuclideanNorm() - circleRadius, 0.0 );
                pc3.a1 = circlePos + ( circlePos - arcPos ).Resize( circleRadius );
                pc3.a2 = arcPos + ( circlePos - arcPos ).Resize( arcRadius - aS2.GetWidth() / 2 );

                if( !bestPath || ( ( bestPath->weight > pc3.weight ) && ( pc3.weight > 0 ) ) )
                    bestPath = &pc3;
            }
        }
    }

    if( bestPath && bestPath->weight > 0 )
    {
        result.push_back( *bestPath );
    }

    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_SEGMENT::Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    VECTOR2I s_start = this->GetStart();
    VECTOR2I s_end = this->GetEnd();
    double   halfWidth1 = this->GetWidth() / 2;

    VECTOR2I arcPos = aS2.GetPos();
    double   arcRadius = aS2.GetRadius();
    double   halfWidth2 = aS2.GetWidth() / 2;


    CU_SHAPE_CIRCLE csc( arcPos, arcRadius + halfWidth2 );

    std::vector<PATH_CONNECTION> pcs;
    pcs = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

    if( pcs.size() < 1 )
        return result;

    VECTOR2I  circlePoint;
    EDA_ANGLE testAngle;

    if( pcs.size() > 0 )
    {
        circlePoint = pcs[0].a1;
        testAngle = ( aS2.AngleBetweenStartAndEnd( pcs[0].a1 ) );
    }

    if( testAngle < aS2.GetEndAngle() && pcs.size() > 0 )
    {
        result.push_back( pcs[0] );
        return result;
    }

    CU_SHAPE_CIRCLE  csc1( aS2.GetStartPoint(), halfWidth2 );
    CU_SHAPE_CIRCLE  csc2( aS2.GetEndPoint(), halfWidth2 );
    PATH_CONNECTION* bestPath = nullptr;

    for( PATH_CONNECTION& pc : this->Paths( csc1, aMaxWeight, aMaxSquaredWeight ) )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
            bestPath = &pc;
    }

    for( PATH_CONNECTION& pc : this->Paths( csc2, aMaxWeight, aMaxSquaredWeight ) )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
            bestPath = &pc;
    }

    CU_SHAPE_CIRCLE csc3( s_start, halfWidth1 );
    CU_SHAPE_CIRCLE csc4( s_end, halfWidth1 );

    for( PATH_CONNECTION& pc : csc3.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
            bestPath = &pc;
    }


    for( PATH_CONNECTION& pc : csc4.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
            bestPath = &pc;
    }

    if( bestPath )
        result.push_back( *bestPath );

    return result;
}

// Function to compute the projection of point P onto the line segment AB
VECTOR2I closestPointOnSegment( const VECTOR2I& A, const VECTOR2I& B, const VECTOR2I& P )
{
    if( A == B )
        return A;
    if( A == P )
        return A;

    VECTOR2I AB = B - A;
    VECTOR2I AP = P - A;

    double t = float( AB.Dot( AP ) ) / float( AB.SquaredEuclideanNorm() );

    // Clamp t to the range [0, 1] to restrict the projection to the segment
    t = std::max( 0.0, std::min( 1.0, t ) );

    return A + ( AB * t );
}


std::vector<PATH_CONNECTION> CU_SHAPE_SEGMENT::Paths( const CU_SHAPE_SEGMENT& aS2,
                                                      double                  aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    VECTOR2I A( this->GetStart() );
    VECTOR2I B( this->GetEnd() );
    double   halfWidth1 = this->GetWidth() / 2;


    VECTOR2I C( aS2.GetStart() );
    VECTOR2I D( aS2.GetEnd() );
    double   halfWidth2 = aS2.GetWidth() / 2;

    VECTOR2I P1 = closestPointOnSegment( A, B, C );
    VECTOR2I P2 = closestPointOnSegment( A, B, D );
    VECTOR2I P3 = closestPointOnSegment( C, D, A );
    VECTOR2I P4 = closestPointOnSegment( C, D, B );

    // Calculate all possible squared distances between the segments
    double dist1 = ( P1 - C ).SquaredEuclideanNorm();
    double dist2 = ( P2 - D ).SquaredEuclideanNorm();
    double dist3 = ( P3 - A ).SquaredEuclideanNorm();
    double dist4 = ( P4 - B ).SquaredEuclideanNorm();

    // Find the minimum squared distance and update closest points
    double   min_dist = dist1;
    VECTOR2I closest1 = P1;
    VECTOR2I closest2 = C;

    if( dist2 < min_dist )
    {
        min_dist = dist2;
        closest1 = P2;
        closest2 = D;
    }

    if( dist3 < min_dist )
    {
        min_dist = dist3;
        closest1 = A;
        closest2 = P3;
    }

    if( dist4 < min_dist )
    {
        min_dist = dist4;
        closest1 = B;
        closest2 = P4;
    }


    PATH_CONNECTION pc;
    pc.a1 = closest1 + ( closest2 - closest1 ).Resize( halfWidth1 );
    pc.a2 = closest2 + ( closest1 - closest2 ).Resize( halfWidth2 );
    pc.weight = std::max( sqrt( min_dist ) - halfWidth1 - halfWidth2, 0.0 );

    if( pc.weight <= aMaxWeight )
        result.push_back( pc );

    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_CIRCLE::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    double   R1 = this->GetRadius();
    double   R2 = aS2.GetRadius();
    VECTOR2I center1 = this->GetPos();
    VECTOR2I center2 = aS2.GetPos();
    double   dist = ( center1 - center2 ).EuclideanNorm();

    if( dist > aMaxWeight || dist == 0 )
        return result;

    double weight = sqrt( dist * dist - R2 * R2 ) - R1;
    double theta = asin( R2 / dist );
    double psi = acos( R2 / dist );

    if( weight > aMaxWeight )
        return result;

    PATH_CONNECTION pc;
    pc.weight = std::max( weight, 0.0 );

    double circleAngle = EDA_ANGLE( center2 - center1 ).AsRadians();

    VECTOR2I pStart;
    VECTOR2I pEnd;

    pStart = VECTOR2I( R1 * cos( theta + circleAngle ), R1 * sin( theta + circleAngle ) );
    pStart += center1;
    pEnd = VECTOR2I( -R2 * cos( psi - circleAngle ), R2 * sin( psi - circleAngle ) );
    pEnd += center2;

    pc.a1 = pStart;
    pc.a2 = pEnd;
    result.push_back( pc );

    pStart = VECTOR2I( R1 * cos( -theta + circleAngle ), R1 * sin( -theta + circleAngle ) );
    pStart += center1;
    pEnd = VECTOR2I( -R2 * cos( -psi - circleAngle ), R2 * sin( -psi - circleAngle ) );
    pEnd += center2;

    pc.a1 = pStart;
    pc.a2 = pEnd;

    result.push_back( pc );
    return result;
}


std::vector<PATH_CONNECTION> CU_SHAPE_ARC::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;
    VECTOR2I                     point = aS2.GetPos();
    VECTOR2I                     arcCenter = this->GetPos();

    double radius = this->GetRadius();
    double width = this->GetWidth();

    EDA_ANGLE angle( point - arcCenter );

    while( angle < this->GetStartAngle() )
        angle += ANGLE_360;
    while( angle > this->GetEndAngle() + ANGLE_360 )
        angle -= ANGLE_360;

    if( angle < this->GetEndAngle() )
    {
        if( ( point - arcCenter ).SquaredEuclideanNorm() > radius * radius )
        {
            CU_SHAPE_CIRCLE circle( arcCenter, radius + width / 2 );
            return circle.Paths( aS2, aMaxWeight, aMaxSquaredWeight );
        }
        else
        {
            PATH_CONNECTION pc;
            pc.weight = std::max( ( radius - width / 2 ) - ( point - arcCenter ).EuclideanNorm(), 0.0 );
            pc.a1 = ( point - arcCenter ).Resize( radius - width / 2 ) + arcCenter;
            pc.a2 = point;

            if( pc.weight > 0 && pc.weight < aMaxWeight )
                result.push_back( pc );

            return result;
        }
    }
    else
    {
        VECTOR2I nearestPoint;

        if( ( point - this->GetStartPoint() ).SquaredEuclideanNorm()
                > ( point - this->GetEndPoint() ).SquaredEuclideanNorm() )
        {
            nearestPoint = this->GetEndPoint();
        }
        else
        {
            nearestPoint = this->GetStartPoint();
        }

        CU_SHAPE_CIRCLE circle( nearestPoint, width / 2 );
        return circle.Paths( aS2, aMaxWeight, aMaxSquaredWeight );
    }
}


std::vector<PATH_CONNECTION> CU_SHAPE_ARC::Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<PATH_CONNECTION> result;

    double R1 = this->GetRadius();
    double R2 = aS2.GetRadius();

    VECTOR2I C1 = this->GetPos();
    VECTOR2I C2 = aS2.GetPos();

    PATH_CONNECTION bestPath;
    bestPath.weight = std::numeric_limits<double>::infinity();
    CU_SHAPE_CIRCLE csc1( C1, R1 + this->GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc2( C2, R2 + aS2.GetWidth() / 2 );

    CU_SHAPE_CIRCLE csc3( this->GetStartPoint(), this->GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc4( this->GetEndPoint(), this->GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc5( aS2.GetStartPoint(), aS2.GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc6( aS2.GetEndPoint(), aS2.GetWidth() / 2 );

    for( const std::vector<PATH_CONNECTION>& pcs : { csc1.Paths( csc2, aMaxWeight, aMaxSquaredWeight ),
                                                     this->Paths( csc2, aMaxWeight, aMaxSquaredWeight ),
                                                     csc1.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) } )
    {
        for( const PATH_CONNECTION& pc : pcs )
        {
            EDA_ANGLE testAngle1 = this->AngleBetweenStartAndEnd( pc.a1 );
            EDA_ANGLE testAngle2 = aS2.AngleBetweenStartAndEnd( pc.a2 );

            if( testAngle1 < this->GetEndAngle() && testAngle2 < aS2.GetEndAngle() && bestPath.weight > pc.weight )
                bestPath = pc;
        }
    }

    for( const std::vector<PATH_CONNECTION>& pcs : { this->Paths( csc5, aMaxWeight, aMaxSquaredWeight ),
                                                     this->Paths( csc6, aMaxWeight, aMaxSquaredWeight ),
                                                     csc3.Paths( aS2, aMaxWeight, aMaxSquaredWeight ),
                                                     csc4.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) } )
    {
        for( const PATH_CONNECTION& pc : pcs )
        {
            if( bestPath.weight > pc.weight )
                bestPath = pc;
        }
    }

    if( bestPath.weight != std::numeric_limits<double>::infinity() )
        result.push_back( bestPath );

    return result;
}


bool segmentIntersectsCircle( const VECTOR2I& p1, const VECTOR2I& p2, const VECTOR2I& center, double radius,
                              std::vector<VECTOR2I>* aIntersectPoints )
{
    SEG    segment( p1, p2 );
    CIRCLE circle( center, radius );

    std::vector<VECTOR2I> intersectionPoints;
    INTERSECTABLE_GEOM    geom1 = segment;
    INTERSECTABLE_GEOM    geom2 = circle;

    INTERSECTION_VISITOR visitor( geom2, intersectionPoints );
    std::visit( visitor, geom1 );

    if( aIntersectPoints )
    {
        for( VECTOR2I& point : intersectionPoints )
            aIntersectPoints->push_back( point );
    }

    return intersectionPoints.size() > 0;
}

bool SegmentIntersectsBoard( const VECTOR2I& aP1, const VECTOR2I& aP2,
                             const std::vector<BOARD_ITEM*>&       aBe,
                             const std::vector<const BOARD_ITEM*>& aDontTestAgainst,
                             int                                   aMinGrooveWidth )
{
    std::vector<VECTOR2I> intersectionPoints;
    bool TestGrooveWidth = aMinGrooveWidth > 0;

    for( BOARD_ITEM* be : aBe )
    {
        if( count( aDontTestAgainst.begin(), aDontTestAgainst.end(), be ) > 0 )
            continue;

        PCB_SHAPE* d = static_cast<PCB_SHAPE*>( be );
        if( !d )
            continue;

        switch( d->GetShape() )
        {
        case SHAPE_T::SEGMENT:
        {
            bool intersects = segments_intersect( aP1, aP2, d->GetStart(), d->GetEnd(), intersectionPoints );

            if( intersects && !TestGrooveWidth )
                return false;

            break;
        }

        case SHAPE_T::RECTANGLE:
        {
            VECTOR2I c1 = d->GetStart();
            VECTOR2I c2( d->GetStart().x, d->GetEnd().y );
            VECTOR2I c3 = d->GetEnd();
            VECTOR2I c4( d->GetEnd().x, d->GetStart().y );

            bool intersects = false;
            intersects |= segments_intersect( aP1, aP2, c1, c2, intersectionPoints );
            intersects |= segments_intersect( aP1, aP2, c2, c3, intersectionPoints );
            intersects |= segments_intersect( aP1, aP2, c3, c4, intersectionPoints );
            intersects |= segments_intersect( aP1, aP2, c4, c1, intersectionPoints );

            if( intersects && !TestGrooveWidth )
                return false;

            break;
        }

        case SHAPE_T::POLY:
        {
            std::vector<VECTOR2I> points = d->GetPolyPoints();

            if( points.size() < 2 )
                break;

            VECTOR2I prevPoint = points.back();

            bool intersects = false;

            for( const VECTOR2I& p : points )
            {
                intersects |= segments_intersect( aP1, aP2, prevPoint, p, intersectionPoints );
                prevPoint = p;
            }

            if( intersects && !TestGrooveWidth )
                return false;

            break;
        }

        case SHAPE_T::CIRCLE:
        {
            VECTOR2I center = d->GetCenter();
            double   radius = d->GetRadius();

            bool intersects = segmentIntersectsCircle( aP1, aP2, center, radius, &intersectionPoints );

            if( intersects && !TestGrooveWidth )
                return false;

            break;
        }

        case SHAPE_T::ARC:
        {
            VECTOR2I center = d->GetCenter();
            double   radius = d->GetRadius();

            EDA_ANGLE A, B;
            d->CalcArcAngles( A, B );

            bool intersects = segmentIntersectsArc( aP1, aP2, center, radius, A, B, &intersectionPoints );

            if( intersects && !TestGrooveWidth )
                return false;

            break;
        }

        default:
            break;
        }
    }

    if( intersectionPoints.size() <= 0 )
        return true;

    if( intersectionPoints.size() % 2 != 0 )
        return false; // Should not happen if the start and end are both on the board

    int minx = intersectionPoints[0].x;
    int maxx = intersectionPoints[0].x;
    int miny = intersectionPoints[0].y;
    int maxy = intersectionPoints[0].y;

    for( const VECTOR2I& v : intersectionPoints )
    {
        minx = v.x < minx ? v.x : minx;
        maxx = v.x > maxx ? v.x : maxx;
        miny = v.x < miny ? v.x : miny;
        maxy = v.x > maxy ? v.x : maxy;
    }

    if( abs( maxx - minx ) > abs( maxy - miny ) )
    {
        std::sort( intersectionPoints.begin(), intersectionPoints.end(),
                   []( const VECTOR2I& a, const VECTOR2I& b )
                   {
                       return a.x > b.x;
                   } );
    }
    else
    {
        std::sort( intersectionPoints.begin(), intersectionPoints.end(),
                   []( const VECTOR2I& a, const VECTOR2I& b )
                   {
                       return a.y > b.y;
                   } );
    }

    int GVSquared = aMinGrooveWidth * aMinGrooveWidth;

    for( size_t i = 0; i < intersectionPoints.size(); i += 2 )
    {
        if( intersectionPoints[i].SquaredDistance( intersectionPoints[i + 1] ) > GVSquared )
            return false;
    }

    return true;
}


std::vector<PATH_CONNECTION> GetPaths( CREEP_SHAPE* aS1, CREEP_SHAPE* aS2, double aMaxWeight )
{
    double                       maxWeight = aMaxWeight;
    double                       maxWeightSquared = maxWeight * maxWeight;
    std::vector<PATH_CONNECTION> result;

    CU_SHAPE_SEGMENT* cusegment1 = dynamic_cast<CU_SHAPE_SEGMENT*>( aS1 );
    CU_SHAPE_SEGMENT* cusegment2 = dynamic_cast<CU_SHAPE_SEGMENT*>( aS2 );
    CU_SHAPE_CIRCLE*  cucircle1 = dynamic_cast<CU_SHAPE_CIRCLE*>( aS1 );
    CU_SHAPE_CIRCLE*  cucircle2 = dynamic_cast<CU_SHAPE_CIRCLE*>( aS2 );
    CU_SHAPE_ARC*     cuarc1 = dynamic_cast<CU_SHAPE_ARC*>( aS1 );
    CU_SHAPE_ARC*     cuarc2 = dynamic_cast<CU_SHAPE_ARC*>( aS2 );


    BE_SHAPE_POINT*  bepoint1 = dynamic_cast<BE_SHAPE_POINT*>( aS1 );
    BE_SHAPE_POINT*  bepoint2 = dynamic_cast<BE_SHAPE_POINT*>( aS2 );
    BE_SHAPE_CIRCLE* becircle1 = dynamic_cast<BE_SHAPE_CIRCLE*>( aS1 );
    BE_SHAPE_CIRCLE* becircle2 = dynamic_cast<BE_SHAPE_CIRCLE*>( aS2 );
    BE_SHAPE_ARC*    bearc1 = dynamic_cast<BE_SHAPE_ARC*>( aS1 );
    BE_SHAPE_ARC*    bearc2 = dynamic_cast<BE_SHAPE_ARC*>( aS2 );

    // Cu to Cu

    if( cuarc1 && cuarc2 )
        return cuarc1->Paths( *cuarc2, maxWeight, maxWeightSquared );
    if( cuarc1 && cucircle2 )
        return cuarc1->Paths( *cucircle2, maxWeight, maxWeightSquared );
    if( cuarc1 && cusegment2 )
        return cuarc1->Paths( *cusegment2, maxWeight, maxWeightSquared );
    if( cucircle1 && cuarc2 )
        return cucircle1->Paths( *cuarc2, maxWeight, maxWeightSquared );
    if( cucircle1 && cucircle2 )
        return cucircle1->Paths( *cucircle2, maxWeight, maxWeightSquared );
    if( cucircle1 && cusegment2 )
        return cucircle1->Paths( *cusegment2, maxWeight, maxWeightSquared );
    if( cusegment1 && cuarc2 )
        return cusegment1->Paths( *cuarc2, maxWeight, maxWeightSquared );
    if( cusegment1 && cucircle2 )
        return cusegment1->Paths( *cucircle2, maxWeight, maxWeightSquared );
    if( cusegment1 && cusegment2 )
        return cusegment1->Paths( *cusegment2, maxWeight, maxWeightSquared );


    // Cu to Be

    if( cuarc1 && bearc2 )
        return cuarc1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( cuarc1 && becircle2 )
        return cuarc1->Paths( *becircle2, maxWeight, maxWeightSquared );
    if( cuarc1 && bepoint2 )
        return cuarc1->Paths( *bepoint2, maxWeight, maxWeightSquared );
    if( cucircle1 && bearc2 )
        return cucircle1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( cucircle1 && becircle2 )
        return cucircle1->Paths( *becircle2, maxWeight, maxWeightSquared );
    if( cucircle1 && bepoint2 )
        return cucircle1->Paths( *bepoint2, maxWeight, maxWeightSquared );
    if( cusegment1 && bearc2 )
        return cusegment1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( cusegment1 && becircle2 )
        return cusegment1->Paths( *becircle2, maxWeight, maxWeightSquared );
    if( cusegment1 && bepoint2 )
        return cusegment1->Paths( *bepoint2, maxWeight, maxWeightSquared );

    // Reversed

    if( cuarc2 && bearc1 )
        return bearc1->Paths( *cuarc2, maxWeight, maxWeightSquared );
    if( cuarc2 && becircle1 )
        return becircle1->Paths( *cuarc2, maxWeight, maxWeightSquared );
    if( cuarc2 && bepoint1 )
        return bepoint1->Paths( *cuarc2, maxWeight, maxWeightSquared );
    if( cucircle2 && bearc1 )
        return bearc1->Paths( *cucircle2, maxWeight, maxWeightSquared );
    if( cucircle2 && becircle1 )
        return becircle1->Paths( *cucircle2, maxWeight, maxWeightSquared );
    if( cucircle2 && bepoint1 )
        return bepoint1->Paths( *cucircle2, maxWeight, maxWeightSquared );
    if( cusegment2 && bearc1 )
        return bearc1->Paths( *cusegment2, maxWeight, maxWeightSquared );
    if( cusegment2 && becircle1 )
        return becircle1->Paths( *cusegment2, maxWeight, maxWeightSquared );
    if( cusegment2 && bepoint1 )
        return bepoint1->Paths( *cusegment2, maxWeight, maxWeightSquared );


    // Be to Be

    if( bearc1 && bearc2 )
        return bearc1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( bearc1 && becircle2 )
        return bearc1->Paths( *becircle2, maxWeight, maxWeightSquared );
    if( bearc1 && bepoint2 )
        return bearc1->Paths( *bepoint2, maxWeight, maxWeightSquared );
    if( becircle1 && bearc2 )
        return becircle1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( becircle1 && becircle2 )
        return becircle1->Paths( *becircle2, maxWeight, maxWeightSquared );
    if( becircle1 && bepoint2 )
        return becircle1->Paths( *bepoint2, maxWeight, maxWeightSquared );
    if( bepoint1 && bearc2 )
        return bepoint1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( bepoint1 && becircle2 )
        return bepoint1->Paths( *becircle2, maxWeight, maxWeightSquared );
    if( bepoint1 && bepoint2 )
        return bepoint1->Paths( *bepoint2, maxWeight, maxWeightSquared );

    return result;
}

double CREEPAGE_GRAPH::Solve( std::shared_ptr<GRAPH_NODE>& aFrom, std::shared_ptr<GRAPH_NODE>& aTo,
                              std::vector<std::shared_ptr<GRAPH_CONNECTION>>& aResult ) // Change to vector of pointers
{
    if( !aFrom || !aTo )
        return 0;

    if( aFrom == aTo )
        return 0;

    // Dijkstra's algorithm for shortest path
    std::unordered_map<GRAPH_NODE*, double>     distances;
    std::unordered_map<GRAPH_NODE*, GRAPH_NODE*> previous;

    auto cmp = [&distances]( GRAPH_NODE* left, GRAPH_NODE* right )
    {
        double distLeft = distances[left];
        double distRight = distances[right];

        if( distLeft == distRight )
            return left > right; // Compare addresses to avoid ties.
        return distLeft > distRight;
    };
    std::priority_queue<GRAPH_NODE*, std::vector<GRAPH_NODE*>, decltype( cmp )> pq( cmp );

    // Initialize distances to infinity for all nodes except the starting node
    for( const std::shared_ptr<GRAPH_NODE>& node : m_nodes )
    {
        if( node != nullptr )
            distances[node.get()] = std::numeric_limits<double>::infinity(); // Set to infinity
    }

    distances[aFrom.get()] = 0.0;
    distances[aTo.get()] = std::numeric_limits<double>::infinity();
    pq.push( aFrom.get() );

    // Dijkstra's main loop
    while( !pq.empty() )
    {
        GRAPH_NODE* current = pq.top();
        pq.pop();

        if( current == aTo.get() )
        {
            break; // Shortest path found
        }

        // Traverse neighbors
        for( const std::shared_ptr<GRAPH_CONNECTION>& connection : current->m_node_conns )
        {
            GRAPH_NODE* neighbor = ( connection->n1 ).get() == current ? ( connection->n2 ).get()
                                                                       : ( connection->n1 ).get();

            if( !neighbor )
                continue;

            // Ignore connections with negative weights as Dijkstra doesn't support them.
            if( connection->m_path.weight < 0.0 )
            {
                wxLogTrace( "CREEPAGE", "Negative weight connection found. Ignoring connection." );
                continue;
            }

            double alt = distances[current] + connection->m_path.weight; // Calculate alternative path cost

            if( alt < distances[neighbor] )
            {
                distances[neighbor] = alt;
                previous[neighbor] = current;
                pq.push( neighbor );
            }
        }
    }

    double pathWeight = distances[aTo.get()];

    // If aTo is unreachable, return infinity
    if( pathWeight == std::numeric_limits<double>::infinity() )
        return std::numeric_limits<double>::infinity();

    // Trace back the path from aTo to aFrom
    GRAPH_NODE* step = aTo.get();

    while( step != aFrom.get() )
    {
        GRAPH_NODE* prevNode = previous[step];

        for( const std::shared_ptr<GRAPH_CONNECTION>& node_conn : step->m_node_conns )
        {
            if( ( ( node_conn->n1 ).get() == prevNode && ( node_conn->n2 ).get() == step )
                || ( ( node_conn->n1 ).get() == step && ( node_conn->n2 ).get() == prevNode ) )
            {
                aResult.push_back( node_conn );
                break;
            }
        }
        step = prevNode;
    }

    return pathWeight;
}

void CREEPAGE_GRAPH::Addshape( const SHAPE& aShape, std::shared_ptr<GRAPH_NODE>& aConnectTo,
                               BOARD_ITEM* aParent )
{
    CREEP_SHAPE* newshape = nullptr;

    if( !aConnectTo )
        return;

    switch( aShape.Type() )
    {
    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT& segment = dynamic_cast<const SHAPE_SEGMENT&>( aShape );
        CU_SHAPE_SEGMENT*    cuseg = new CU_SHAPE_SEGMENT( segment.GetSeg().A, segment.GetSeg().B,
                                                           segment.GetWidth() );
        newshape = dynamic_cast<CREEP_SHAPE*>( cuseg );
        break;
    }

    case SH_CIRCLE:
    {
        const SHAPE_CIRCLE& circle = dynamic_cast<const SHAPE_CIRCLE&>( aShape );
        CU_SHAPE_CIRCLE* cucircle = new CU_SHAPE_CIRCLE( circle.GetCenter(), circle.GetRadius() );
        newshape = dynamic_cast<CREEP_SHAPE*>( cucircle );
        break;
    }

    case SH_ARC:
    {
        const SHAPE_ARC& arc = dynamic_cast<const SHAPE_ARC&>( aShape );
        EDA_ANGLE        alpha, beta;
        VECTOR2I         start, end;

        EDA_SHAPE edaArc( SHAPE_T::ARC, 0, FILL_T::NO_FILL );

        if( arc.IsClockwise() )
        {
            edaArc.SetArcGeometry( arc.GetP0(), arc.GetArcMid(), arc.GetP1() );
            start = arc.GetP0();
            end = arc.GetP1();
        }
        else
        {
            edaArc.SetArcGeometry( arc.GetP1(), arc.GetArcMid(), arc.GetP0() );
            start = arc.GetP1();
            end = arc.GetP0();
        }

        edaArc.CalcArcAngles( alpha, beta );

        CU_SHAPE_ARC* cuarc = new CU_SHAPE_ARC( edaArc.getCenter(), edaArc.GetRadius(), alpha, beta,
                                                arc.GetP0(), arc.GetP1() );
        cuarc->SetWidth( arc.GetWidth() );
        newshape = dynamic_cast<CREEP_SHAPE*>( cuarc );
        break;
    }

    case SH_COMPOUND:
    {
        int nbShapes = static_cast<const SHAPE_COMPOUND*>( &aShape )->Shapes().size();
        for( const SHAPE* subshape : ( static_cast<const SHAPE_COMPOUND*>( &aShape )->Shapes() ) )
        {
            if( subshape )
            {
                // We don't want to add shape for the inner rectangle of rounded rectangles
                if( !( ( subshape->Type() == SH_RECT ) && ( nbShapes == 5 ) ) )
                    Addshape( *subshape, aConnectTo, aParent );
            }
        }
        break;
    }

    case SH_POLY_SET:
    {
        const SHAPE_POLY_SET& polySet = dynamic_cast<const SHAPE_POLY_SET&>( aShape );

        for( auto it = polySet.CIterateSegmentsWithHoles(); it; it++ )
        {
            const SEG     object = *it;
            SHAPE_SEGMENT segment( object.A, object.B );
            Addshape( segment, aConnectTo, aParent );
        }
        break;
    }

    case SH_LINE_CHAIN:
    {
        const SHAPE_LINE_CHAIN& lineChain = dynamic_cast<const SHAPE_LINE_CHAIN&>( aShape );

        VECTOR2I prevPoint = lineChain.CLastPoint();

        for( const VECTOR2I& point : lineChain.CPoints() )
        {
            SHAPE_SEGMENT segment( point, prevPoint );
            prevPoint = point;
            Addshape( segment, aConnectTo, aParent );
        }

        break;
    }

    case SH_RECT:
    {
        const SHAPE_RECT& rect = dynamic_cast<const SHAPE_RECT&>( aShape );

        VECTOR2I point0 = rect.GetPosition();
        VECTOR2I point1 = rect.GetPosition() + VECTOR2I( rect.GetSize().x, 0 );
        VECTOR2I point2 = rect.GetPosition() + rect.GetSize();
        VECTOR2I point3 = rect.GetPosition() + VECTOR2I( 0, rect.GetSize().y );

        Addshape( SHAPE_SEGMENT( point0, point1 ), aConnectTo, aParent );
        Addshape( SHAPE_SEGMENT( point1, point2 ), aConnectTo, aParent );
        Addshape( SHAPE_SEGMENT( point2, point3 ), aConnectTo, aParent );
        Addshape( SHAPE_SEGMENT( point3, point0 ), aConnectTo, aParent );
        break;
    }

    default:
        break;
    }

    if( !newshape )
        return;

    std::shared_ptr<GRAPH_NODE> gnShape = nullptr;

    newshape->SetParent( aParent );

    switch( aShape.Type() )
    {
    case SH_SEGMENT: gnShape = AddNode( GRAPH_NODE::SEGMENT, newshape, newshape->GetPos() ); break;
    case SH_CIRCLE:  gnShape = AddNode( GRAPH_NODE::CIRCLE, newshape, newshape->GetPos() );  break;
    case SH_ARC:     gnShape = AddNode( GRAPH_NODE::ARC, newshape, newshape->GetPos() );     break;
    default:                                                                                 break;
    }

    if( gnShape )
    {
        m_shapeCollection.push_back( newshape );
        gnShape->m_net = aConnectTo->m_net;
        std::shared_ptr<GRAPH_CONNECTION> gc = AddConnection( gnShape, aConnectTo );

        if( gc )
            gc->m_path.m_show = false;
    }
    else
    {
        delete newshape;
        newshape = nullptr;
    }
}

void CREEPAGE_GRAPH::GeneratePaths( double aMaxWeight, PCB_LAYER_ID aLayer )
{
    std::vector<std::shared_ptr<GRAPH_NODE>> nodes;
    std::mutex                               nodes_lock;
    thread_pool&                             tp = GetKiCadThreadPool();

    TRACK_RTREE trackIndex;
    std::vector<CREEPAGE_TRACK_ENTRY*> trackEntries; // For cleanup

    if( aLayer != Edge_Cuts )
    {
        for( PCB_TRACK* track : m_board.Tracks() )
        {
            if( track && track->Type() == KICAD_T::PCB_TRACE_T && track->IsOnLayer( aLayer ) )
            {
                std::shared_ptr<SHAPE> sh = track->GetEffectiveShape();

                if( sh && sh->Type() == SHAPE_TYPE::SH_SEGMENT )
                {
                    CREEPAGE_TRACK_ENTRY* entry = new CREEPAGE_TRACK_ENTRY();
                    entry->segment = SEG( track->GetStart(), track->GetEnd() );
                    entry->layer = aLayer;

                    BOX2I bbox = track->GetBoundingBox();
                    int   minCoords[2] = { bbox.GetX(), bbox.GetY() };
                    int   maxCoords[2] = { bbox.GetRight(), bbox.GetBottom() };
                    trackIndex.Insert( minCoords, maxCoords, entry );
                    trackEntries.push_back( entry );
                }
            }
        }
    }

    std::copy_if( m_nodes.begin(), m_nodes.end(), std::back_inserter( nodes ),
            [&]( const std::shared_ptr<GRAPH_NODE>& gn )
            {
                return gn && gn->m_parent && gn->m_connectDirectly && ( gn->m_type != GRAPH_NODE::TYPE::VIRTUAL );
            } );

    std::sort( nodes.begin(), nodes.end(),
            []( const std::shared_ptr<GRAPH_NODE>& gn1, const std::shared_ptr<GRAPH_NODE>& gn2 )
            {
                return gn1->m_parent < gn2->m_parent
                        || ( gn1->m_parent == gn2->m_parent && gn1->m_net < gn2->m_net );
            } );

    // Build parent -> net -> nodes mapping for efficient filtering
    // Also cache bounding boxes for early spatial filtering
    std::unordered_map<const BOARD_ITEM*, std::unordered_map<int, std::vector<std::shared_ptr<GRAPH_NODE>>>> parent_net_groups;
    std::unordered_map<const BOARD_ITEM*, BOX2I> parent_bboxes;
    std::vector<const BOARD_ITEM*> parent_keys;

    for( const auto& gn : nodes )
    {
        const BOARD_ITEM* parent = gn->m_parent->GetParent();

        if( parent_net_groups[parent].empty() )
        {
            parent_keys.push_back( parent );
            if( parent )
                parent_bboxes[parent] = parent->GetBoundingBox();
        }

        parent_net_groups[parent][gn->m_net].push_back( gn );
    }

    // Generate work items using parent-level spatial indexing
    std::vector<std::pair<std::shared_ptr<GRAPH_NODE>, std::shared_ptr<GRAPH_NODE>>> work_items;

    // Use RTree for spatial indexing of parent bounding boxes
    // Expand each bbox by maxWeight to find potentially overlapping parents

    int64_t maxDist = static_cast<int64_t>( aMaxWeight );

    struct ParentEntry
    {
        const BOARD_ITEM* parent;
        BOX2I             bbox;
    };

    RTree<ParentEntry*, int, 2, double> parentIndex;
    std::vector<ParentEntry>            parentEntries;

    // Build list of non-null parents first
    for( const auto* parent : parent_keys )
    {
        if( parent )
        {
            ParentEntry entry;
            entry.parent = parent;
            entry.bbox = parent_bboxes[parent];
            parentEntries.push_back( entry );
        }
    }

    // Insert into RTree
    for( ParentEntry& entry : parentEntries )
    {
        int minCoords[2] = { entry.bbox.GetLeft(), entry.bbox.GetTop() };
        int maxCoords[2] = { entry.bbox.GetRight(), entry.bbox.GetBottom() };
        parentIndex.Insert( minCoords, maxCoords, &entry );
    }

    // Parallelize parent pair search using thread pool
    std::mutex work_items_lock;

    auto searchParent = [&]( size_t i ) -> bool
    {
        const ParentEntry& entry1 = parentEntries[i];
        const BOARD_ITEM*  parent1 = entry1.parent;
        BOX2I              bbox1 = entry1.bbox;

        std::vector<std::pair<std::shared_ptr<GRAPH_NODE>, std::shared_ptr<GRAPH_NODE>>> localWorkItems;

        // Search for parents within maxDist of bbox1
        int searchMin[2] = { bbox1.GetLeft() - (int) maxDist, bbox1.GetTop() - (int) maxDist };
        int searchMax[2] = { bbox1.GetRight() + (int) maxDist, bbox1.GetBottom() + (int) maxDist };

        parentIndex.Search( searchMin, searchMax,
                [&]( ParentEntry* entry2 ) -> bool
                {
                    const BOARD_ITEM* parent2 = entry2->parent;

                    // Only process if parent1 < parent2 to avoid duplicates
                    if( parent1 >= parent2 )
                        return true;

                    // Precise bbox distance check
                    BOX2I bbox2 = entry2->bbox;

                    int64_t bboxDistX = 0;
                    if( bbox2.GetLeft() > bbox1.GetRight() )
                        bboxDistX = bbox2.GetLeft() - bbox1.GetRight();
                    else if( bbox1.GetLeft() > bbox2.GetRight() )
                        bboxDistX = bbox1.GetLeft() - bbox2.GetRight();

                    int64_t bboxDistY = 0;
                    if( bbox2.GetTop() > bbox1.GetBottom() )
                        bboxDistY = bbox2.GetTop() - bbox1.GetBottom();
                    else if( bbox1.GetTop() > bbox2.GetBottom() )
                        bboxDistY = bbox1.GetTop() - bbox2.GetBottom();

                    int64_t bboxDistSq = bboxDistX * bboxDistX + bboxDistY * bboxDistY;
                    if( bboxDistSq > maxDist * maxDist )
                        return true;

                    // Get nodes for both parents (thread-safe reads from const map)
                    auto it1 = parent_net_groups.find( parent1 );
                    auto it2 = parent_net_groups.find( parent2 );

                    if( it1 == parent_net_groups.end() || it2 == parent_net_groups.end() )
                        return true;

                    for( const auto& [net1, nodes1] : it1->second )
                    {
                        for( const auto& [net2, nodes2] : it2->second )
                        {
                            // Skip same net if both are conductive
                            if( net1 == net2 && !nodes1.empty() && !nodes2.empty() )
                            {
                                if( nodes1[0]->m_parent->IsConductive()
                                    && nodes2[0]->m_parent->IsConductive() )
                                    continue;
                            }

                            for( const auto& gn1 : nodes1 )
                            {
                                for( const auto& gn2 : nodes2 )
                                {
                                    VECTOR2I pos1 = gn1->m_parent->GetPos();
                                    VECTOR2I pos2 = gn2->m_parent->GetPos();
                                    int      r1 = gn1->m_parent->GetRadius();
                                    int      r2 = gn2->m_parent->GetRadius();

                                    int64_t centerDistSq = ( pos1 - pos2 ).SquaredEuclideanNorm();
                                    double  threshold = aMaxWeight + r1 + r2;
                                    double  thresholdSq = threshold * threshold;

                                    if( (double) centerDistSq > thresholdSq )
                                        continue;

                                    localWorkItems.push_back( { gn1, gn2 } );
                                }
                            }
                        }
                    }

                    return true;
                } );

        // Merge local results into global
        if( !localWorkItems.empty() )
        {
            std::lock_guard<std::mutex> lock( work_items_lock );
            work_items.insert( work_items.end(), localWorkItems.begin(), localWorkItems.end() );
        }

        return true;
    };

    // Use thread pool if there are enough parents
    if( parentEntries.size() > 100 && tp.get_tasks_total() < tp.get_thread_count() - 4 )
    {
        auto ret = tp.submit_loop( 0, parentEntries.size(), searchParent );

        for( auto& r : ret )
        {
            if( r.valid() )
                r.wait();
        }
    }
    else
    {
        for( size_t i = 0; i < parentEntries.size(); ++i )
            searchParent( i );
    }

    auto processWorkItems =
            [&]( size_t idx ) -> bool
            {
                auto& [gn1, gn2] = work_items[idx];

                // Distance filtering already done during work item creation
                CREEP_SHAPE* shape1 = gn1->m_parent;
                CREEP_SHAPE* shape2 = gn2->m_parent;

                for( const PATH_CONNECTION& pc : GetPaths( shape1, shape2, aMaxWeight ) )
                {
                    std::vector<const BOARD_ITEM*> IgnoreForTest =
                    {
                        gn1->m_parent->GetParent(), gn2->m_parent->GetParent()
                    };

                    bool valid = pc.isValid( m_board, aLayer, m_boardEdge, IgnoreForTest, m_boardOutline,
                                     { false, true }, m_minGrooveWidth, &trackIndex );

                    if( !valid )
                    {
                        continue;
                    }

                    std::shared_ptr<GRAPH_NODE> connect1 = gn1, connect2 = gn2;
                    std::lock_guard<std::mutex> lock( nodes_lock );

                    // Handle non-point node1
                    if( gn1->m_parent->GetType() != CREEP_SHAPE::TYPE::POINT )
                    {
                        auto gnt1 = AddNode( GRAPH_NODE::POINT, gn1->m_parent, pc.a1 );
                        gnt1->m_connectDirectly = false;
                        connect1 = gnt1;

                        if( gn1->m_parent->IsConductive() )
                        {
                            if( std::shared_ptr<GRAPH_CONNECTION> gc = AddConnection( gn1, gnt1 ) )
                                gc->m_path.m_show = false;
                        }
                    }

                    // Handle non-point node2
                    if( gn2->m_parent->GetType() != CREEP_SHAPE::TYPE::POINT )
                    {
                        auto gnt2 = AddNode( GRAPH_NODE::POINT, gn2->m_parent, pc.a2 );
                        gnt2->m_connectDirectly = false;
                        connect2 = gnt2;

                        if( gn2->m_parent->IsConductive() )
                        {
                            if( std::shared_ptr<GRAPH_CONNECTION> gc = AddConnection( gn2, gnt2 ) )
                                gc->m_path.m_show = false;
                        }
                    }

                    AddConnection( connect1, connect2, pc );
                }

                return true;
            };

    // If the number of tasks is high enough, this indicates that the calling process
    // has already parallelized the work, so we can process all items in one go.
    if( tp.get_tasks_total() >= tp.get_thread_count() - 4 )
    {
        for( size_t ii = 0; ii < work_items.size(); ii++ )
            processWorkItems( ii );
    }
    else
    {
        auto ret = tp.submit_loop( 0, work_items.size(), processWorkItems );

        for( size_t ii = 0; ii < ret.size(); ii++ )
        {
            auto& r = ret[ii];

            if( !r.valid() )
                continue;

            while( r.wait_for( std::chrono::milliseconds( 100 ) ) != std::future_status::ready ){}
        }
    }

    // Clean up track entries
    for( CREEPAGE_TRACK_ENTRY* entry : trackEntries )
        delete entry;
}


void CREEPAGE_GRAPH::Trim( double aWeightLimit )
{
    std::vector<std::shared_ptr<GRAPH_CONNECTION>> toRemove;

    // Collect connections to remove
    for( std::shared_ptr<GRAPH_CONNECTION>& gc : m_connections )
    {
        if( gc && ( gc->m_path.weight > aWeightLimit ) )
            toRemove.push_back( gc );
    }

    // Remove collected connections
    for( const std::shared_ptr<GRAPH_CONNECTION>& gc : toRemove )
        RemoveConnection( gc );
}


void CREEPAGE_GRAPH::RemoveConnection( const std::shared_ptr<GRAPH_CONNECTION>& aGc, bool aDelete )
{
    if( !aGc )
        return;

    for( std::shared_ptr<GRAPH_NODE> gn : { aGc->n1, aGc->n2 } )
    {
        if( gn )
        {
            gn->m_node_conns.erase( aGc );

            if( gn->m_node_conns.empty() && aDelete )
            {
                auto it = std::find_if( m_nodes.begin(), m_nodes.end(),
                                        [&gn]( const std::shared_ptr<GRAPH_NODE>& node )
                                        {
                                            return node.get() == gn.get();
                                        } );

                if( it != m_nodes.end() )
                    m_nodes.erase( it );

                m_nodeset.erase( gn );
            }
        }
    }

    if( aDelete )
    {
        // Remove the connection from the graph's connections
        m_connections.erase( std::remove( m_connections.begin(), m_connections.end(), aGc ),
                             m_connections.end() );
    }
}


std::shared_ptr<GRAPH_NODE> CREEPAGE_GRAPH::AddNode( GRAPH_NODE::TYPE aType, CREEP_SHAPE* parent,
                                                     const VECTOR2I& pos )
{
    std::shared_ptr<GRAPH_NODE> gn = FindNode( aType, parent, pos );

    if( gn )
        return gn;

    gn = std::make_shared<GRAPH_NODE>( aType, parent, pos );
    m_nodes.push_back( gn );
    m_nodeset.insert( gn );
    return gn;
}


std::shared_ptr<GRAPH_NODE> CREEPAGE_GRAPH::AddNodeVirtual()
{
    //Virtual nodes are always unique, do not try to find them
    std::shared_ptr<GRAPH_NODE> gn = std::make_shared<GRAPH_NODE>( GRAPH_NODE::TYPE::VIRTUAL, nullptr );
    m_nodes.push_back( gn );
    m_nodeset.insert( gn );
    return gn;
}


std::shared_ptr<GRAPH_CONNECTION> CREEPAGE_GRAPH::AddConnection( std::shared_ptr<GRAPH_NODE>& aN1,
                                                                 std::shared_ptr<GRAPH_NODE>& aN2,
                                                                 const PATH_CONNECTION&       aPc )
{
    if( !aN1 || !aN2 )
        return nullptr;

    wxASSERT_MSG( ( aN1 != aN2 ), "Creepage: a connection connects a node to itself" );

    std::shared_ptr<GRAPH_CONNECTION> gc = std::make_shared<GRAPH_CONNECTION>( aN1, aN2, aPc );
    m_connections.push_back( gc );
    aN1->m_node_conns.insert( gc );
    aN2->m_node_conns.insert( gc );

    return gc;
}


std::shared_ptr<GRAPH_CONNECTION> CREEPAGE_GRAPH::AddConnection( std::shared_ptr<GRAPH_NODE>& aN1,
                                                                 std::shared_ptr<GRAPH_NODE>& aN2 )
{
    if( !aN1 || !aN2 )
        return nullptr;

    PATH_CONNECTION pc;
    pc.a1 = aN1->m_pos;
    pc.a2 = aN2->m_pos;
    pc.weight = 0;

    return AddConnection( aN1, aN2, pc );
}


std::shared_ptr<GRAPH_NODE> CREEPAGE_GRAPH::FindNode( GRAPH_NODE::TYPE aType, CREEP_SHAPE* aParent,
                                                      const VECTOR2I& aPos )
{
    auto it = m_nodeset.find( std::make_shared<GRAPH_NODE>( aType, aParent, aPos ) );

    if( it != m_nodeset.end() )
        return *it;

    return nullptr;
}


std::shared_ptr<GRAPH_NODE> CREEPAGE_GRAPH::AddNetElements( int aNetCode, PCB_LAYER_ID aLayer,
                                                            int aMaxCreepage )
{
    std::shared_ptr<GRAPH_NODE> virtualNode = AddNodeVirtual();
    virtualNode->m_net = aNetCode;

    for( FOOTPRINT* footprint : m_board.Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetNetCode() != aNetCode || !pad->IsOnLayer( aLayer ) )
                continue;

            if( std::shared_ptr<SHAPE> padShape = pad->GetEffectiveShape( aLayer ) )
                Addshape( *padShape, virtualNode, pad );
        }
    }

    for( PCB_TRACK* track : m_board.Tracks() )
    {
        if( track->GetNetCode() != aNetCode || !track->IsOnLayer( aLayer ) )
            continue;

        if( std::shared_ptr<SHAPE> shape = track->GetEffectiveShape() )
            Addshape( *shape, virtualNode, track );
    }


    for( ZONE* zone : m_board.Zones() )
    {
        if( zone->GetNetCode() != aNetCode || !zone->IsOnLayer( aLayer ) )
            continue;

        if( std::shared_ptr<SHAPE> shape = zone->GetEffectiveShape( aLayer ) )
            Addshape( *shape, virtualNode, zone );
    }

    const DRAWINGS drawings = m_board.Drawings();

    for( BOARD_ITEM* drawing : drawings )
    {
        if( drawing->IsConnected() )
        {
            BOARD_CONNECTED_ITEM* bci = static_cast<BOARD_CONNECTED_ITEM*>( drawing );

            if( bci->GetNetCode() != aNetCode || !bci->IsOnLayer( aLayer ) )
                continue;

            if( std::shared_ptr<SHAPE> shape = bci->GetEffectiveShape() )
                Addshape( *shape, virtualNode, bci );
        }
    }


    return virtualNode;
}
