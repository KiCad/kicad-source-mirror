/*
    * Copyright (C) 2024 KiCad Developers.
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

#include <common.h>
#include <macros.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <zone.h>
#include <advanced_config.h>
#include <geometry/shape_rect.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>

#include <geometry/shape_circle.h>


extern bool SegmentIntersectsBoard( const VECTOR2I& aP1, const VECTOR2I& aP2,
                                    const std::vector<BOARD_ITEM*>&       aBe,
                                    const std::vector<const BOARD_ITEM*>& aDontTestAgainst );
bool        segmentIntersectsArc( const VECTOR2I& p1, const VECTOR2I& p2, const VECTOR2I& center,
                                  double radius, double startAngle, double endAngle );

struct path_connection
{
    VECTOR2D a1;
    VECTOR2D a2;
    double   weight = -1;
    bool     m_show = true;

    bool m_forceA1concavityCheck = false;
    bool m_forceA2concavityCheck = false;


    /** @brief Test if a path is valid 
     *  
     * Check if a paths intersects the board edge or a track
     */
    bool isValid( const BOARD& aBoard, PCB_LAYER_ID aLayer,
                  const std::vector<BOARD_ITEM*>&       aBoardEdges,
                  const std::vector<const BOARD_ITEM*>& aIgnoreForTest, SHAPE_POLY_SET* aOutline,
                  const std::pair<bool, bool>& aTestLocalConcavity )
    {
        if( !aOutline )
            return true; // We keep the segment if there is a problem

        if( !SegmentIntersectsBoard( a1, a2, aBoardEdges, aIgnoreForTest ) )
            return false;

        // The mid point should be inside the board.
        // Tolerance of 100nm.

        VECTOR2I midPoint = ( a1 + a2 ) / 2;
        int      tolerance = 100;

        if( !( aOutline->Contains( midPoint, -1, tolerance )
               || aOutline->PointOnEdge( midPoint, tolerance ) ) )
            return false;

        if( false && ( aTestLocalConcavity.first || aTestLocalConcavity.second ) )
        {
            // Test for local concavity. If it is localy convex, then it will not be a path of interest.

            double extendLine = 1000; // extend line by 1000nm
            // In some cases, the projected point could be on the board edge
            // In such cases, we wan to keep the line.
            // We inflate the polygon to get a small margin for computation/rounding error.
            // We might keep some unnecessary lines, but it's better than loosing the important ones.
            double extendPoly = 100; // extend polygon by 10 nm

            VECTOR2D a( double( a1.x ), double( a1.y ) );
            VECTOR2D b( double( a2.x ), double( a2.y ) );

            VECTOR2D dir( b - a );
            dir = dir * ( extendLine / dir.SquaredEuclideanNorm() );

            SHAPE_POLY_SET outline2 = *aOutline;
            outline2.Inflate( extendPoly, CORNER_STRATEGY::ROUND_ALL_CORNERS, 3 );

            if( aTestLocalConcavity.first && !aOutline->Contains( a - dir, -1, 0 ) )
                return false;

            if( aTestLocalConcavity.second && !aOutline->Contains( b + dir, -1, 0 ) )
                return false;
        }

        SEG segPath( a1, a2 );

        if ( aLayer != Edge_Cuts )
        {
            for( PCB_TRACK* track : aBoard.Tracks() )
            {
                if( !track )
                    continue;

                if( track->Type() == KICAD_T::PCB_TRACE_T && track->IsOnLayer( aLayer ) )
                {
                    std::shared_ptr<SHAPE> sh = track->GetEffectiveShape();

                    if( sh && sh->Type() == SHAPE_TYPE::SH_SEGMENT )
                    {
                        SEG segTrack( track->GetStart(), track->GetEnd() );

                        if( segPath.Intersects( segTrack ) )
                            return false;
                    }
                }
            }
        }
        return true;
    }
};


/* Check if q is inside a rectangle with diagonal [pr] */
bool on_segment( VECTOR2I p, VECTOR2I q, VECTOR2I r )
{
    return ( q.x <= ( p.x > r.x ? p.x : r.x ) && q.x >= ( p.x < r.x ? p.x : r.x )
             && q.y <= ( p.y > r.y ? p.y : r.y ) && q.y >= ( p.y < r.y ? p.y : r.y ) );
}

/* Find the orientation of a triplet ( p, q, r )
    0 -> colinear
    1 -> Clockwise
    2 -> Counterclockwie
    */
int orientation( VECTOR2D p, VECTOR2D q, VECTOR2D r )
{
    float val = ( q.y - p.y ) * ( r.x - q.x ) - ( q.x - p.x ) * ( r.y - q.y );
    if( val == 0 )
        return 0;
    if( val > 0 )
        return 1;
    return 2;
}

//Check if line segments 'p1q1' and 'p2q2' intersect, excluding endpoint overlap

bool segments_intersect( VECTOR2I p1, VECTOR2I q1, VECTOR2I p2, VECTOR2I q2 )
{
    VECTOR2D a1( double( p1.x ), double( p1.y ) );
    VECTOR2D b1( double( q1.x ), double( q1.y ) );
    VECTOR2D a2( double( p2.x ), double( p2.y ) );
    VECTOR2D b2( double( q2.x ), double( q2.y ) );

    int o1 = orientation( a1, b1, a2 );
    int o2 = orientation( a1, b1, b2 );
    int o3 = orientation( a2, b2, a1 );
    int o4 = orientation( a2, b2, b1 );

    if( p1 == p2 || p1 == q2 || q1 == p2 || q1 == q2 )
        return false;

    if( o1 != o2 && o3 != o4 )
        return true;

    if( o1 == 0 && on_segment( p1, p2, q1 ) && p2 != p1 && p2 != q1 )
        return true;

    if( o2 == 0 && on_segment( p1, q2, q1 ) && q2 != p1 && q2 != q1 )
        return true;

    if( o3 == 0 && on_segment( p2, p1, q2 ) && p1 != p2 && p1 != q2 )
        return true;

    if( o4 == 0 && on_segment( p2, q1, q2 ) && q1 != p2 && q1 != q2 )
        return true;

    return false;
}

class GraphConnection;
class GraphNode;
class CreepageGraph;
class CREEP_SHAPE;
class BE_SHAPE;
class BE_SHAPE_POINT;
class BE_SHAPE_ARC;
class BE_SHAPE_CIRCLE;
class CU_SHAPE;
class CU_SHAPE_SEGMENT;
class CU_SHAPE_CIRCLE;
class CU_SHAPE_ARC;

/** @class CREEP_SHAPE
 * 
 *  @brief A class used to represent the shapes for creepage calculation
 */
class CREEP_SHAPE
{
public:
    enum class TYPE
    {
        UNDEFINED = 0,
        POINT,
        CIRCLE,
        ARC
    };
    CREEP_SHAPE() {};

    virtual ~CREEP_SHAPE() {}


    virtual int       GetRadius() const { return 0; };
    virtual EDA_ANGLE GetStartAngle() const { return EDA_ANGLE( 0 ); };
    virtual EDA_ANGLE GetEndAngle() const { return EDA_ANGLE( 0 ); };
    virtual VECTOR2I  GetStartPoint() const { return VECTOR2I( 0, 0 ); };
    virtual VECTOR2I  GetEndPoint() const { return VECTOR2I( 0, 0 ); };
    VECTOR2I          GetPos() const { return m_pos; };
    CREEP_SHAPE::TYPE GetType() const { return m_type; };
    const BOARD_ITEM* GetParent() const { return m_parent; };
    void              SetParent( BOARD_ITEM* aParent ) { m_parent = aParent; };

    virtual void ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const;

    std::vector<path_connection> ReversePaths( const std::vector<path_connection>& aV ) const
    {
        std::vector<path_connection> r;
        r.reserve( aV.size() );

        for( const auto& pc : aV )
        {
            r.emplace_back( pc );
            std::swap( r.back().a1, r.back().a2 );
        }

        return r;
    }

    std::vector<path_connection> Paths( const CREEP_SHAPE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const
    {
        std::vector<path_connection> a;
        return a;
    };
    virtual std::vector<path_connection> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<path_connection> a;
        return a;
    };
    virtual std::vector<path_connection> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<path_connection> a;
        return a;
    };
    virtual std::vector<path_connection> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<path_connection> a;
        return a;
    };
    virtual std::vector<path_connection> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<path_connection> a;
        return a;
    };
    virtual std::vector<path_connection> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<path_connection> a;
        return a;
    };
    virtual std::vector<path_connection> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<path_connection> a;
        return a;
    };

    //virtual std::vector<path_connection> GetPathsCuToBe( CREEP_SHAPE* aShape ) const{ std::vector<path_connection> a; return a;};
    bool IsConductive() { return m_conductive; };


protected:
    bool              m_conductive = false;
    BOARD_ITEM*       m_parent = nullptr;
    VECTOR2I          m_pos = VECTOR2I( 0, 0 );
    CREEP_SHAPE::TYPE m_type = CREEP_SHAPE::TYPE::UNDEFINED;
};


/** @class CU_SHAPE
 * 
 *  @brief Creepage: a conductive shape
 */
class CU_SHAPE : public CREEP_SHAPE
{
public:
    CU_SHAPE() : CREEP_SHAPE() { m_conductive = true; };
};

/** @class BE_SHAPE
 * 
 *  @brief Creepage: a board edge shape
 */
class BE_SHAPE : public CREEP_SHAPE
{
public:
    BE_SHAPE() : CREEP_SHAPE() { m_conductive = false; };
};

/** @class CU_SHAPE_SEGMENT
 * 
 *  @brief Creepage: a conductive segment
 */
class CU_SHAPE_SEGMENT : public CU_SHAPE
{
public:
    CU_SHAPE_SEGMENT( VECTOR2I aStart, VECTOR2I aEnd, double aWidth = 0 ) : CU_SHAPE()
    {
        m_start = aStart;
        m_end = aEnd;
        m_width = aWidth;
    }

    VECTOR2I GetStart() const { return m_start; };
    VECTOR2I GetEnd() const { return m_end; };
    double   GetWidth() const { return m_width; };

    std::vector<path_connection> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;


private:
    VECTOR2I m_start = VECTOR2I( 0, 0 );
    VECTOR2I m_end = VECTOR2I( 0, 0 );
    double   m_width = 0;
};

/** @class CU_SHAPE_CIRCLE
 * 
 *  @brief Creepage: a conductive circle
 */
class CU_SHAPE_CIRCLE : public CU_SHAPE
{
public:
    CU_SHAPE_CIRCLE( VECTOR2I aPos, double aRadius = 0 ) : CU_SHAPE()
    {
        m_pos = aPos;
        m_radius = aRadius;
    }

    VECTOR2I GetPos() const { return m_pos; };
    int      GetRadius() const override { return m_radius; };

    std::vector<path_connection> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };

    std::vector<path_connection> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

private:
    VECTOR2I m_pos = VECTOR2I( 0, 0 );
    double   m_radius = 1;
};

/** @class CU_SHAPE_ARC
 * 
 *  @brief Creepage: a conductive arc
 */
class CU_SHAPE_ARC : public CU_SHAPE_CIRCLE
{
public:
    CU_SHAPE_ARC( VECTOR2I aPos, double aRadius, EDA_ANGLE aStartAngle, EDA_ANGLE aEndAngle,
                  VECTOR2D aStartPoint, VECTOR2D aEndPoint ) : CU_SHAPE_CIRCLE( aPos, aRadius )
    {
        m_pos = aPos;
        m_type = CREEP_SHAPE::TYPE::ARC;
        m_startAngle = aStartAngle;
        m_endAngle = aEndAngle;
        m_startPoint = aStartPoint;
        m_endPoint = aEndPoint;
        m_radius = aRadius;
    }

    std::vector<path_connection> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;


    EDA_ANGLE GetStartAngle() const override { return m_startAngle; }
    EDA_ANGLE GetEndAngle() const override { return m_endAngle; }
    int       GetRadius() const override { return m_radius; }


    VECTOR2I  GetStartPoint() const override { return m_startPoint; }
    VECTOR2I  GetEndPoint() const override { return m_endPoint; }
    EDA_ANGLE AngleBetweenStartAndEnd( const VECTOR2I aPoint ) const
    {
        EDA_ANGLE angle( aPoint - m_pos );
        while( angle < GetStartAngle() )
            angle += ANGLE_360;
        while( angle > GetEndAngle() + ANGLE_360 )
            angle -= ANGLE_360;

        return angle;
    }
    double GetWidth() const { return m_width; };
    void   SetWidth( double aW ) { m_width = aW; };

private:
    int       m_width = 0;
    VECTOR2I  m_pos = VECTOR2I( 0, 0 );
    double    m_radius = 1;
    EDA_ANGLE m_startAngle = EDA_ANGLE( 0 );
    EDA_ANGLE m_endAngle = EDA_ANGLE( 180 );
    VECTOR2I  m_startPoint = VECTOR2I( 1, 0 );
    VECTOR2I  m_endPoint = VECTOR2I( -1, 0 );
};

/** @class Graphnode
 * 
 *  @brief a node in a @class CreepageGraph
 */
class GraphNode
{
public:
    enum TYPE
    {

        POINT = 0,
        CIRCLE,
        ARC,
        SEGMENT,
        VIRTUAL
    };

    GraphNode( GraphNode::TYPE aType, CREEP_SHAPE* aParent, VECTOR2I aPos = VECTOR2I() )
    {
        m_parent = aParent;
        m_pos = aPos;
        m_type = aType;
        m_connectDirectly = true;
        m_connections = {};
    };

    ~GraphNode() {};


    CREEP_SHAPE*                  m_parent = nullptr;
    std::vector<GraphConnection*> m_connections = {};
    VECTOR2I                      m_pos = VECTOR2I( 0, 0 );
    // Virtual nodes are connected with a 0 weight connection to equivalent net ( same net or netclass )
    bool m_virtual = false;
    bool m_connectDirectly = true;
    int  m_net = -1;

    GraphNode::TYPE m_type;
};

/** @class GraphConnection
 * 
 *  @brief a connection in a @class CreepageGraph
 */
class GraphConnection
{
public:
    GraphConnection( GraphNode* aN1, GraphNode* aN2, const path_connection& aPc ) :
            n1( aN1 ), n2( aN2 )
    {
        m_path = aPc;
    };

    GraphNode*                 n1 = nullptr;
    GraphNode*                 n2 = nullptr;
    path_connection            m_path;

    std::vector<PCB_SHAPE>  GetShapes();
    bool                    forceStraightLigne = false;
};


bool compareShapes( const CREEP_SHAPE* a, const CREEP_SHAPE* b )
{
    if( !a )
        return true;
    if( !b )
        return false;

    if( a->GetType() != b->GetType() )
    {
        return a->GetType() < b->GetType();
    }

    if( a->GetType() == CREEP_SHAPE::TYPE::UNDEFINED )
        return true;

    auto posA = a->GetPos();
    auto posB = b->GetPos();

    if( posA != posB )
    {
        return posA < posB;
    }
    if( a->GetType() == CREEP_SHAPE::TYPE::CIRCLE )
    {
        return a->GetRadius() < b->GetRadius();
    }
    return false;
}

bool areEquivalent( const CREEP_SHAPE* a, const CREEP_SHAPE* b )
{
    if( !a && !b )
    {
        return true;
    }
    if( ( !a && b ) || ( a && !b ) )
    {
        return false;
    }
    if( a->GetType() != b->GetType() )
    {
        return false;
    }
    if( a->GetType() == CREEP_SHAPE::TYPE::POINT )
    {
        return a->GetPos() == b->GetPos();
    }
    if( a->GetType() == CREEP_SHAPE::TYPE::CIRCLE )
    {
        return a->GetPos() == b->GetPos() && ( a->GetRadius() == b->GetRadius() );
    }
    return false;
}


/** @class BE_SHAPE_POINT
 * 
 *  @brief Creepage: a board edge point
 */
class BE_SHAPE_POINT : public BE_SHAPE
{
public:
    BE_SHAPE_POINT( VECTOR2I aPos ) : BE_SHAPE()
    {
        m_pos = aPos;
        m_type = CREEP_SHAPE::TYPE::POINT;
    }

    std::vector<path_connection> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    }
    std::vector<path_connection> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };


    void ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const override;
};

/** @class BE_SHAPE_CIRCLE
 * 
 *  @brief Creepage: a board edge circle
 */
class BE_SHAPE_CIRCLE : public BE_SHAPE
{
public:
    BE_SHAPE_CIRCLE( VECTOR2I aPos = VECTOR2I( 0, 0 ), int aRadius = 0 ) : BE_SHAPE()
    {
        m_pos = aPos;
        m_radius = aRadius;
        m_type = CREEP_SHAPE::TYPE::CIRCLE;
    }

    std::vector<path_connection> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<path_connection> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };


    int  GetRadius() const override { return m_radius; }
    void ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const override;
    void ShortenChildDueToGV( GraphNode* a1, GraphNode* a2, CreepageGraph& aG,
                              double aNormalWeight ) const;


protected:
    int m_radius;
};

/** @class BE_SHAPE_ARC
 * 
 *  @brief Creepage: a board edge arc
 */
class BE_SHAPE_ARC : public BE_SHAPE_CIRCLE
{
public:
    BE_SHAPE_ARC( VECTOR2I aPos, int aRadius, EDA_ANGLE aStartAngle, EDA_ANGLE aEndAngle,
                  VECTOR2D aStartPoint, VECTOR2D aEndPoint ) : BE_SHAPE_CIRCLE( aPos, aRadius )
    {
        m_type = CREEP_SHAPE::TYPE::ARC;
        m_startAngle = aStartAngle;
        m_endAngle = aEndAngle;
        m_startPoint = aStartPoint;
        m_endPoint = aEndPoint;
        m_radius = aRadius;
    }

    void ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const override;


    std::vector<path_connection> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<path_connection> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<path_connection> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };


    EDA_ANGLE GetStartAngle() const override { return m_startAngle; }
    EDA_ANGLE GetEndAngle() const override { return m_endAngle; }
    int       GetRadius() const override { return m_radius; }


    VECTOR2I  GetStartPoint() const override { return m_startPoint; }
    VECTOR2I  GetEndPoint() const override { return m_endPoint; }
    EDA_ANGLE AngleBetweenStartAndEnd( const VECTOR2I aPoint ) const
    {
        EDA_ANGLE angle( aPoint - m_pos );

        while( angle < m_startAngle )
            angle += ANGLE_360;
        while( angle > m_endAngle + ANGLE_360 )
            angle -= ANGLE_360;

        return angle;
    }

    std::pair<bool, bool> IsThereATangentPassingThroughPoint( const BE_SHAPE_POINT aPoint ) const;

protected:
    int       m_radius;
    EDA_ANGLE m_startAngle;
    EDA_ANGLE m_endAngle;
    VECTOR2I  m_startPoint;
    VECTOR2I  m_endPoint;
};


std::vector<path_connection> BE_SHAPE_POINT::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                    double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    double weight = ( this->GetPos() - aS2.GetPos() ).SquaredEuclideanNorm();

    if( weight > aMaxSquaredWeight )
        return result;

    path_connection pc;
    pc.a1 = this->GetPos();
    pc.a2 = aS2.GetPos();
    pc.weight = sqrt( weight );

    result.push_back( pc );
    return result;
}

std::vector<path_connection> BE_SHAPE_POINT::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                    double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
    int                          radius = aS2.GetRadius();
    VECTOR2I                     pointPos = this->GetPos();
    VECTOR2I                     circleCenter = aS2.GetPos();

    if( radius <= 0 )
    {
        return result;
    }

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

    path_connection pc;
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

std::pair<bool, bool>
BE_SHAPE_ARC::IsThereATangentPassingThroughPoint( const BE_SHAPE_POINT aPoint ) const
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
        connectToEndPoint &=
                ( cos( startAngle ) * newPoint.x + sin( startAngle ) * newPoint.y <= R );

    connectToEndPoint |= ( cos( startAngle ) * newPoint.x + sin( startAngle ) * newPoint.y <= R )
                         && ( pointAngle >= endAngle || pointAngle <= startAngle );


    result.second = !connectToEndPoint;
    return result;
}

std::vector<path_connection> BE_SHAPE_POINT::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                    double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
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
        std::vector<path_connection> paths = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

        if( paths.size() > 1 ) // Point to circle creates either 0 or 2 connections
        {
            result.push_back( paths[1] );
        }
    }
    else
    {
        BE_SHAPE_POINT csp1( aS2.GetStartPoint() );

        for( path_connection pc : this->Paths( csp1, aMaxWeight, aMaxSquaredWeight ) )
        {
            result.push_back( pc );
        }
    }
    if( behavesLikeCircle.second )
    {
        BE_SHAPE_CIRCLE              csc( center, radius );
        std::vector<path_connection> paths = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

        if( paths.size() > 1 ) // Point to circle creates either 0 or 2 connections
        {
            result.push_back( paths[0] );
        }
    }
    else
    {
        BE_SHAPE_POINT csp1( aS2.GetEndPoint() );

        for( path_connection pc : this->Paths( csp1, aMaxWeight, aMaxSquaredWeight ) )
        {
            result.push_back( pc );
        }
    }
    return result;
}

std::vector<path_connection> BE_SHAPE_CIRCLE::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
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


    for( path_connection pc : this->Paths( csc, aMaxWeight, aMaxSquaredWeight ) )
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

    for( BE_SHAPE_POINT csp : { csp1, csp2 } )
    {
        for( path_connection pc : this->Paths( csp, aMaxWeight, aMaxSquaredWeight ) )
        {
            if( !segmentIntersectsArc( pc.a1, pc.a2, arcCenter, arcRadius,
                                       arcStartAngle.AsDegrees(), arcEndAngle.AsDegrees() ) )
                result.push_back( pc );
        }
    }


    return result;
}


std::vector<path_connection> BE_SHAPE_ARC::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
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


    for( path_connection pc : this->Paths( BE_SHAPE_CIRCLE( aS2.GetPos(), aS2.GetRadius() ),
                                           aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE pointAngle = aS2.AngleBetweenStartAndEnd( pc.a2 - arcCenter );

        if( pointAngle <= aS2.GetEndAngle() )
            result.push_back( pc );
    }

    for( path_connection pc : BE_SHAPE_CIRCLE( this->GetPos(), this->GetRadius() )
                                      .Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE pointAngle = this->AngleBetweenStartAndEnd( pc.a2 - arcCenter );

        if( pointAngle <= this->GetEndAngle() )
            result.push_back( pc );
    }

    return result;
}


std::vector<path_connection> BE_SHAPE_CIRCLE::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

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


        path_connection pc;
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


        path_connection pc;
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

/** @class CreepageGraph
 * 
 *  @brief A graph with nodes and connections for creepage calculation
 */
class CreepageGraph
{
public:
    CreepageGraph( BOARD& aBoard ) : m_board( aBoard )
    {
        m_boardOutline = nullptr;
        m_creepageTarget = -1;
        m_creepageTargetSquared = -1;
    };
    ~CreepageGraph()
    {
        for( GraphConnection* gc : m_connections )
        {
            if( gc )
            {
                delete gc;
                gc = nullptr;
            }
        }
        for( GraphNode* gn : m_nodes )
            if( gn )
            {
                delete gn;
                gn = nullptr;
            }
        for( CREEP_SHAPE* cs : m_shapeCollection )
            if( cs )
            {
                delete cs;
                cs = nullptr;
            }
    };

    BOARD&                   m_board;
    std::vector<BOARD_ITEM*> m_boardEdge;
    SHAPE_POLY_SET*          m_boardOutline;

    std::vector<GraphNode*>                       m_nodes;
    std::vector<GraphConnection*>                 m_connections;


    void TransformEdgeToCreepShapes();
    void TransformCreepShapesToNodes( std::vector<CREEP_SHAPE*>& aShapes );
    void RemoveDuplicatedShapes();
    // Add a node to the graph. If an equivalent node exists, returns the pointer of the existing node instead
    GraphNode*       AddNode( GraphNode::TYPE aType, CREEP_SHAPE* aParent = nullptr,
                              VECTOR2I aPos = VECTOR2I() );
    GraphNode*       AddNodeVirtual();
    GraphConnection* AddConnection( GraphNode* aN1, GraphNode* aN2, const path_connection& aPc );
    GraphConnection* AddConnection( GraphNode* aN1, GraphNode* aN2 );
    GraphNode*       FindNode( GraphNode::TYPE aType, CREEP_SHAPE* aParent, VECTOR2I aPos );

    void RemoveConnection( GraphConnection*, bool aDelete = false );
    void Trim( double aWeightLimit );
    void Addshape( const SHAPE& aShape, GraphNode* aConnectTo = nullptr,
                   BOARD_ITEM* aParent = nullptr );

    double Solve( GraphNode* aFrom, GraphNode* aTo, std::vector<GraphConnection*>& aResult );

    void                       GeneratePaths( double aMaxWeight, PCB_LAYER_ID aLayer, bool aGenerateBoardEdges = true );
    GraphNode* AddNetElements( int aNetCode, PCB_LAYER_ID aLayer, int aMaxCreepage );

    void   SetTarget( double aTarget );
    double GetTarget() { return m_creepageTarget; };
    int    m_minGrooveWidth = 0;

    std::vector<CREEP_SHAPE*> m_shapeCollection;

private:
    double m_creepageTarget;
    double m_creepageTargetSquared;
};

void CreepageGraph::TransformCreepShapesToNodes( std::vector<CREEP_SHAPE*>& aShapes )
{
    for( CREEP_SHAPE* p1 : aShapes )
    {
        if( !p1 )
            continue;

        switch( p1->GetType() )
        {
        case CREEP_SHAPE::TYPE::POINT: AddNode( GraphNode::TYPE::POINT, p1, p1->GetPos() ); break;
        case CREEP_SHAPE::TYPE::CIRCLE: AddNode( GraphNode::TYPE::CIRCLE, p1, p1->GetPos() ); break;
        case CREEP_SHAPE::TYPE::ARC: AddNode( GraphNode::TYPE::ARC, p1, p1->GetPos() ); break;
        default: break;
        }
    }
}

void CreepageGraph::RemoveDuplicatedShapes()
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

void CreepageGraph::TransformEdgeToCreepShapes()
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
        {
            std::vector<VECTOR2I> points;
            d->DupPolyPointsList( points );

            for( auto p : points )
            {
                BE_SHAPE_POINT* a = new BE_SHAPE_POINT( p );
                m_shapeCollection.push_back( a );
            }
            break;
        }
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
        default: break;
        }
    }
}


std::vector<PCB_SHAPE> GraphConnection::GetShapes()
{
    std::vector<PCB_SHAPE> shapes = std::vector<PCB_SHAPE>();
    int                    lineWidth = m_path.weight / 100;

    if( !m_path.m_show )
        return shapes;

    if( !n1 || !n2 )
        return shapes;

    if( n1->m_type == GraphNode::TYPE::VIRTUAL || n2->m_type == GraphNode::TYPE::VIRTUAL )
    {
        return shapes;
    }

    if( !forceStraightLigne && n1->m_parent && ( n1->m_parent == n2->m_parent )
        && ( n1->m_parent->GetType() == CREEP_SHAPE::TYPE::CIRCLE ) )
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


        s.SetWidth( lineWidth );
        s.SetLayer( Eco1_User );

        shapes.push_back( s );
        return shapes;
    }

    else if( !forceStraightLigne && n1->m_parent && ( n1->m_parent == n2->m_parent )
             && n1->m_parent->GetType() == CREEP_SHAPE::TYPE::ARC )
    {
        BE_SHAPE_ARC* arc = dynamic_cast<BE_SHAPE_ARC*>( n1->m_parent );

        if( !arc )
        {
            PCB_SHAPE s;
            s.SetStart( m_path.a1 );
            s.SetEnd( m_path.a2 );

            s.SetWidth( lineWidth );

            s.SetLayer( Eco1_User );

            shapes.push_back( s );
            return shapes;
        }

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

        s.SetWidth( lineWidth );
        s.SetLayer( Eco1_User );

        shapes.push_back( s );
        return shapes;
    }

    PCB_SHAPE s;
    s.SetStart( m_path.a1 );
    s.SetEnd( m_path.a2 );

    s.SetWidth( lineWidth );

    s.SetLayer( Eco1_User );

    shapes.push_back( s );

    return shapes;
}

void CREEP_SHAPE::ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const
{
}


void BE_SHAPE_POINT::ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const
{
}

void BE_SHAPE_CIRCLE::ShortenChildDueToGV( GraphNode* a1, GraphNode* a2, CreepageGraph& aG,
                                           double aNormalWeight ) const
{
    EDA_ANGLE angle1 = EDA_ANGLE( a1->m_pos - m_pos );
    EDA_ANGLE angle2 = EDA_ANGLE( a2->m_pos - m_pos );

    while( angle1 < 0 )
        angle1 += ANGLE_360;
    while( angle2 < 0 )
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


    GraphNode* gnt = aG.AddNode( GraphNode::POINT, a1->m_parent, skipPoint );

    path_connection pc;

    pc.a1 = maxAngle == angle2 ? a1->m_pos : a2->m_pos;
    pc.a2 = skipPoint;
    pc.weight = aNormalWeight - aG.m_minGrooveWidth;
    aG.AddConnection( maxAngle == angle2 ? a1 : a2, gnt, pc );

    pc.a1 = skipPoint;
    pc.a2 = maxAngle == angle2 ? a2->m_pos : a1->m_pos;
    pc.weight = aG.m_minGrooveWidth;

    GraphConnection* gc = aG.AddConnection( gnt, maxAngle == angle2 ? a2 : a1, pc );

    if( gc )
        gc->forceStraightLigne = true;
    return;
}

void BE_SHAPE_CIRCLE::ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const
{
    if( !a1 || !a2 )
        return;

    if( m_radius == 0 )
        return;

    VECTOR2D distI( a1->m_pos - a2->m_pos );
    VECTOR2D distD( double( distI.x ), double( distI.y ) );

    double weight = m_radius * 2 * asin( distD.EuclideanNorm() / ( 2.0 * m_radius ) );

    if( ( weight > aG.GetTarget() ) )
        return;

    if( aG.m_minGrooveWidth <= 0 )
    {
        path_connection pc;
        pc.a1 = a1->m_pos;
        pc.a2 = a2->m_pos;
        pc.weight = weight;

        aG.AddConnection( a1, a2, pc );
        return;
    }

    if( weight > aG.m_minGrooveWidth )
    {
        ShortenChildDueToGV( a1, a2, aG, weight );
    }
    // Else well.. this paths will be "shorted" by another one
    return;
}


void BE_SHAPE_ARC::ConnectChildren( GraphNode* a1, GraphNode* a2, CreepageGraph& aG ) const
{
    if( !a1 || !a2 )
        return;

    EDA_ANGLE angle1 = AngleBetweenStartAndEnd( a1->m_pos );
    EDA_ANGLE angle2 = AngleBetweenStartAndEnd( a2->m_pos );

    double weight = abs( m_radius * ( angle2 - angle1 ).AsRadians() );

    if( aG.m_minGrooveWidth <= 0 )
    {
        if( ( weight > aG.GetTarget() ) )
            return;

        path_connection pc;
        pc.a1 = a1->m_pos;
        pc.a2 = a2->m_pos;
        pc.weight = weight;

        aG.AddConnection( a1, a2, pc );
        return;
    }

    if( weight > aG.m_minGrooveWidth )
    {
        ShortenChildDueToGV( a1, a2, aG, weight );
    }
}

void CreepageGraph::SetTarget( double aTarget )
{
    m_creepageTarget = aTarget;
    m_creepageTargetSquared = aTarget * aTarget;
}

bool segmentIntersectsArc( const VECTOR2I& p1, const VECTOR2I& p2, const VECTOR2I& center,
                           double radius, double startAngle, double endAngle )
{
    // Vector from p1 to p2
    VECTOR2I D = p2 - p1;
    // Vector from p1 to center
    VECTOR2I F = p1 - center;

    // Convert to double precision
    VECTOR2D d( D.x, D.y );
    VECTOR2D f( F.x, F.y );

    // Coefficients for the quadratic equation
    double a = d.x * d.x + d.y * d.y;
    double b = 2 * ( f.x * d.x + f.y * d.y );
    double c = ( f.x * f.x + f.y * f.y ) - radius * radius;

    // Discriminant of the quadratic equation
    double discriminant = b * b - 4 * a * c;

    if( discriminant < 0 )
    {
        return false; // No intersection with the circle
    }
    else
    {
        discriminant = sqrt( discriminant );
        double t1 = ( -b - discriminant ) / ( 2 * a );
        double t2 = ( -b + discriminant ) / ( 2 * a );

        for( double tn : { t1, t2 } )
        {
            if( tn < 0 || tn > 1 )
                continue;

            VECTOR2D intersection( p1.x + tn * d.x, p1.y + tn * d.y );
            VECTOR2D toIntersection =
                    VECTOR2D( intersection.x - center.x, intersection.y - center.y );

            EDA_ANGLE angle( toIntersection );

            if( angle < startAngle )
                angle += ANGLE_360;

            if( angle <= endAngle )
                return true;
        }

        return false; // No valid intersection with the arc
    }
}

std::vector<path_connection> CU_SHAPE_SEGMENT::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
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

    path_connection pc;
    pc.a1 = newPoint;
    pc.a2 = pointPos;
    pc.weight = sqrt( weightSquared );

    result.push_back( pc );
    return result;
}


std::vector<path_connection> CU_SHAPE_SEGMENT::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
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
        for( path_connection pc : csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
        {
            result.push_back( pc );
        }
    }
    else if( ( projectedPos1 > length && projectedPos2 > length ) )
    {
        CU_SHAPE_CIRCLE csc( end, halfWidth );
        for( path_connection pc : csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight ) )
        {
            result.push_back( pc );
        }
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
            path_connection pc;
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
        std::vector<path_connection> pcs = csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

        if ( pcs.size() < 2 )
            return result;

        result.push_back( pcs.at( trackSide == 1 ? 1 : 0 ) );


        PointOnTrack = start;
        PointOnTrack += ( end - start ).Resize( projectedPos1 );
        PointOnTrack += ( end - start ).Perpendicular().Resize( halfWidth ) * trackSide;
        PointOnCircle = circleCenter - ( end - start ).Resize( circleRadius );
        weightSquared = ( PointOnCircle - PointOnTrack ).SquaredEuclideanNorm();

        if( weightSquared < aMaxSquaredWeight )
        {
            path_connection pc;
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
        std::vector<path_connection> pcs = csc.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

        if ( pcs.size() < 2 )
            return result;

        result.push_back( pcs.at( trackSide == 1 ? 0 : 1 ) );

        PointOnTrack = start;
        PointOnTrack += ( end - start ).Resize( projectedPos2 );
        PointOnTrack += ( end - start ).Perpendicular().Resize( halfWidth ) * trackSide;
        PointOnCircle = circleCenter + ( end - start ).Resize( circleRadius );
        weightSquared = ( PointOnCircle - PointOnTrack ).SquaredEuclideanNorm();

        if( weightSquared < aMaxSquaredWeight )
        {
            path_connection pc;
            pc.a1 = PointOnTrack;
            pc.a2 = PointOnCircle;
            pc.weight = sqrt( weightSquared );

            result.push_back( pc );
        }
    }

    return result;
}


std::vector<path_connection> CU_SHAPE_SEGMENT::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    BE_SHAPE_CIRCLE bsc( aS2.GetPos(), aS2.GetRadius() );

    for( auto& pc : this->Paths( bsc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < aS2.GetEndAngle() )
        {
            result.push_back( pc );
        }
    }

    if( result.size() < 2 )
    {
        BE_SHAPE_POINT bsp1( aS2.GetStartPoint() );
        BE_SHAPE_POINT bsp2( aS2.GetEndPoint() );

        VECTOR2I  beArcPos = aS2.GetPos();
        int       beArcRadius = aS2.GetRadius();
        EDA_ANGLE beArcStartAngle = aS2.GetStartAngle();
        EDA_ANGLE beArcEndAngle = aS2.GetEndAngle();

        for( auto& pc : this->Paths( bsp1, aMaxWeight, aMaxSquaredWeight ) )
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius,
                                       beArcStartAngle.AsDegrees(), beArcEndAngle.AsDegrees() ) )
                result.push_back( pc );

        for( auto& pc : this->Paths( bsp2, aMaxWeight, aMaxSquaredWeight ) )
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius,
                                       beArcStartAngle.AsDegrees(), beArcEndAngle.AsDegrees() ) )
                result.push_back( pc );
    }

    return result;
}


std::vector<path_connection> CU_SHAPE_CIRCLE::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
    VECTOR2I                     beArcPos = aS2.GetPos();
    int                          beArcRadius = aS2.GetRadius();
    EDA_ANGLE                    beArcStartAngle = aS2.GetStartAngle();
    EDA_ANGLE                    beArcEndAngle = aS2.GetEndAngle();

    BE_SHAPE_CIRCLE bsc( beArcPos, beArcRadius );

    for( auto& pc : this->Paths( bsc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < aS2.GetEndAngle() )
        {
            result.push_back( pc );
        }
    }

    if( result.size() < 2 )
    {
        BE_SHAPE_POINT bsp1( aS2.GetStartPoint() );
        BE_SHAPE_POINT bsp2( aS2.GetEndPoint() );

        for( auto& pc : this->Paths( bsp1, aMaxWeight, aMaxSquaredWeight ) )
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius,
                                       beArcStartAngle.AsDegrees(), beArcEndAngle.AsDegrees() ) )
                result.push_back( pc );

        for( auto& pc : this->Paths( bsp2, aMaxWeight, aMaxSquaredWeight ) )
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius,
                                       beArcStartAngle.AsDegrees(), beArcEndAngle.AsDegrees() ) )
                result.push_back( pc );
    }
    return result;
}

std::vector<path_connection> CU_SHAPE_ARC::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    CU_SHAPE_CIRCLE csc( this->GetPos(), this->GetRadius() + this->GetWidth() / 2 );

    for( auto& pc : this->Paths( csc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = this->AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < this->GetEndAngle() )
        {
            result.push_back( pc );
        }
        }

        if ( result.size() < 2 )
        {
            CU_SHAPE_CIRCLE csc1( this->GetStartPoint(), this->GetWidth() / 2 );
            CU_SHAPE_CIRCLE csc2( this->GetEndPoint(), this->GetWidth() / 2 );

            for( auto& pc : this->Paths( csc1, aMaxWeight, aMaxSquaredWeight ) )
                result.push_back( pc );

            for( auto& pc : this->Paths( csc2, aMaxWeight, aMaxSquaredWeight ) )
                result.push_back( pc );
        }

    return result;
}


std::vector<path_connection> CU_SHAPE_ARC::Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
    VECTOR2I                     beArcPos = aS2.GetPos();
    int                          beArcRadius = aS2.GetRadius();
    EDA_ANGLE                    beArcStartAngle = aS2.GetStartAngle();
    EDA_ANGLE                    beArcEndAngle = aS2.GetEndAngle();

    BE_SHAPE_CIRCLE bsc( aS2.GetPos(), aS2.GetRadius() );

    for( auto& pc : this->Paths( bsc, aMaxWeight, aMaxSquaredWeight ) )
    {
        EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( pc.a2 );

        if( testAngle < aS2.GetEndAngle() )
        {
            result.push_back( pc );
        }
    }

    if( result.size() < 2 )
    {
        BE_SHAPE_POINT bsp1( aS2.GetStartPoint() );
        BE_SHAPE_POINT bsp2( aS2.GetEndPoint() );

        for( auto& pc : this->Paths( bsp1, aMaxWeight, aMaxSquaredWeight ) )
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius,
                                       beArcStartAngle.AsDegrees(), beArcEndAngle.AsDegrees() ) )
                result.push_back( pc );

        for( auto& pc : this->Paths( bsp2, aMaxWeight, aMaxSquaredWeight ) )
            if( !segmentIntersectsArc( pc.a1, pc.a2, beArcPos, beArcRadius,
                                       beArcStartAngle.AsDegrees(), beArcEndAngle.AsDegrees() ) )
                result.push_back( pc );
    }

    return result;
}


std::vector<path_connection> CU_SHAPE_CIRCLE::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    double   R = this->GetRadius();
    VECTOR2I center = this->GetPos();
    VECTOR2I point = aS2.GetPos();
    double   weight = ( center - point ).EuclideanNorm() - R;

    if( weight > aMaxWeight )
        return result;

    path_connection pc;
    pc.weight = weight;
    pc.a2 = point;
    pc.a1 = center + ( point - center ).Resize( R );

    result.push_back( pc );
    return result;
}


std::vector<path_connection> CU_SHAPE_CIRCLE::Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

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

    path_connection pc;
    pc.weight = weight;
    pc.a1 = ( C2 - C1 ).Resize( R1 ) + C1;
    pc.a2 = ( C1 - C2 ).Resize( R2 ) + C2;
    result.push_back( pc );
    return result;
}


std::vector<path_connection> CU_SHAPE_SEGMENT::Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

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

    path_connection pc;
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


std::vector<path_connection> CU_SHAPE_CIRCLE::Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    VECTOR2I circlePos = this->GetPos();
    VECTOR2I arcPos = aS2.GetPos();

    double circleRadius = this->GetRadius();
    double arcRadius = aS2.GetRadius();

    VECTOR2I startPoint = aS2.GetStartPoint();
    VECTOR2I endPoint = aS2.GetEndPoint();

    CU_SHAPE_CIRCLE csc( arcPos, arcRadius + aS2.GetWidth() / 2 );

    if( ( circlePos - arcPos ).EuclideanNorm() > arcRadius + circleRadius )
    {
        std::vector<path_connection> pcs = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

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

    path_connection* bestPath = nullptr;


    std::vector<path_connection> pcs1 = this->Paths( csc1, aMaxWeight, aMaxSquaredWeight );
    std::vector<path_connection> pcs2 = this->Paths( csc2, aMaxWeight, aMaxSquaredWeight );

    for( path_connection& pc : pcs1 )
    {
        if( !bestPath || ( ( bestPath->weight > pc.weight ) && ( pc.weight > 0 ) ) )
            bestPath = &pc;
    }

    for( path_connection& pc : pcs2 )
    {
        if( !bestPath || ( ( bestPath->weight > pc.weight ) && ( pc.weight > 0 ) ) )
            bestPath = &pc;
    }

    // If the circle center is insde the arc ring

    path_connection pc3;

    if( ( circlePos - arcPos ).SquaredEuclideanNorm() < arcRadius * arcRadius )
    {
        if( circlePos != arcPos ) // The best path is already found otherwise
        {
            EDA_ANGLE testAngle = aS2.AngleBetweenStartAndEnd( circlePos );

            if( testAngle < aS2.GetEndAngle() )
            {
                pc3.weight = arcRadius - ( circlePos - arcPos ).EuclideanNorm() - circleRadius;
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


std::vector<path_connection> CU_SHAPE_SEGMENT::Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    VECTOR2I s_start = this->GetStart();
    VECTOR2I s_end = this->GetEnd();
    double   halfWidth1 = this->GetWidth() / 2;

    VECTOR2I arcPos = aS2.GetPos();
    double   arcRadius = aS2.GetRadius();
    double   halfWidth2 = aS2.GetWidth() / 2;


    CU_SHAPE_CIRCLE csc( arcPos, arcRadius + halfWidth2 );

    std::vector<path_connection> pcs;
    pcs = this->Paths( csc, aMaxWeight, aMaxSquaredWeight );

    if ( pcs.size() < 1 )
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
    path_connection* bestPath = nullptr;


    std::vector<path_connection> pcs1 = this->Paths( csc1, aMaxWeight, aMaxSquaredWeight );

    for( path_connection& pc : pcs1 )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
        {
            bestPath = &pc;
        }
    }

    std::vector<path_connection> pcs2 = this->Paths( csc2, aMaxWeight, aMaxSquaredWeight );

    for( path_connection& pc : pcs2 )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
        {
            bestPath = &pc;
        }
    }

    CU_SHAPE_CIRCLE csc3( s_start, halfWidth1 );
    CU_SHAPE_CIRCLE csc4( s_end, halfWidth1 );

    std::vector<path_connection> pcs3 = csc3.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

    for( path_connection& pc : pcs3 )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
        {
            bestPath = &pc;
        }
    }


    std::vector<path_connection> pcs4 = csc4.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

    for( path_connection& pc : pcs4 )
    {
        if( !bestPath || ( bestPath->weight > pc.weight ) )
        {
            bestPath = &pc;
        }
    }

    if( bestPath )
    {
        result.push_back( *bestPath );
    }

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


std::vector<path_connection> CU_SHAPE_SEGMENT::Paths( const CU_SHAPE_SEGMENT& aS2,
                                                      double                  aMaxWeight,
                                                      double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

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


    path_connection pc;
    pc.a1 = closest1 + ( closest2 - closest1 ).Resize( halfWidth1 );
    pc.a2 = closest2 + ( closest1 - closest2 ).Resize( halfWidth2 );
    pc.weight = sqrt( min_dist ) - halfWidth1 - halfWidth2;

    if( pc.weight <= aMaxWeight )
    {
        result.push_back( pc );
    }
    return result;
}


std::vector<path_connection> CU_SHAPE_CIRCLE::Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                     double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    double   R1 = this->GetRadius();
    double   R2 = aS2.GetRadius();
    VECTOR2I center1 = this->GetPos();
    VECTOR2I center2 = aS2.GetPos();
    double   dist = ( center1 - center2 ).EuclideanNorm();

    if( dist > aMaxWeight || dist == 0 )
    {
        return result;
    }

    double weight = sqrt ( dist*dist - R2 * R2 ) - R1;
    double theta = asin( R2 / dist );
    double psi = acos( R2 / dist );

    if( weight > aMaxWeight )
    {
        return result;
    }

    path_connection pc;
    pc.weight = weight;

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


std::vector<path_connection> CU_SHAPE_ARC::Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;
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
            path_connection pc;
            pc.weight = ( radius - width / 2 ) - ( point - arcCenter ).EuclideanNorm();
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
            nearestPoint = this->GetEndPoint();
        else
            nearestPoint = this->GetStartPoint();

        CU_SHAPE_CIRCLE circle( nearestPoint, width / 2 );
        return circle.Paths( aS2, aMaxWeight, aMaxSquaredWeight );
    }
    return result;
}


std::vector<path_connection> CU_SHAPE_ARC::Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                  double aMaxSquaredWeight ) const
{
    std::vector<path_connection> result;

    double R1 = this->GetRadius();
    double R2 = aS2.GetRadius();

    VECTOR2I C1 = this->GetPos();
    VECTOR2I C2 = aS2.GetPos();

    path_connection bestPath;
    bestPath.weight = std::numeric_limits<double>::infinity();
    CU_SHAPE_CIRCLE csc1( C1, R1 + this->GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc2( C2, R2 + aS2.GetWidth() / 2 );

    CU_SHAPE_CIRCLE csc3( this->GetStartPoint(), this->GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc4( this->GetEndPoint(), this->GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc5( aS2.GetStartPoint(), aS2.GetWidth() / 2 );
    CU_SHAPE_CIRCLE csc6( aS2.GetEndPoint(), aS2.GetWidth() / 2 );

    std::vector<path_connection> pcs0 = csc1.Paths( csc2, aMaxWeight, aMaxSquaredWeight );

    std::vector<path_connection> pcs1 = this->Paths( csc2, aMaxWeight, aMaxSquaredWeight );
    std::vector<path_connection> pcs2 = csc1.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

    std::vector<path_connection> pcs3 = this->Paths( csc5, aMaxWeight, aMaxSquaredWeight );
    std::vector<path_connection> pcs4 = this->Paths( csc6, aMaxWeight, aMaxSquaredWeight );

    std::vector<path_connection> pcs5 = csc3.Paths( aS2, aMaxWeight, aMaxSquaredWeight );
    std::vector<path_connection> pcs6 = csc4.Paths( aS2, aMaxWeight, aMaxSquaredWeight );

    for( std::vector<path_connection> pcs : { pcs0, pcs1, pcs2 } )
    {
        for( path_connection& pc : pcs )
        {
            EDA_ANGLE testAngle1 = this->AngleBetweenStartAndEnd( pc.a1 );
            EDA_ANGLE testAngle2 = aS2.AngleBetweenStartAndEnd( pc.a2 );

            if( ( testAngle1 < this->GetEndAngle() ) && ( testAngle2 < aS2.GetEndAngle() )
                && ( bestPath.weight > pc.weight ) )
            {
                bestPath = pc;
            }
        }
    }

    for( std::vector<path_connection> pcs : { pcs3, pcs4, pcs5, pcs6 } )
    {
        for( path_connection& pc : pcs )
        {
            if( bestPath.weight > pc.weight )
            {
                bestPath = pc;
            }
        }
    }

    if( bestPath.weight != std::numeric_limits<double>::infinity() )
    {
        result.push_back( bestPath );
    }

    return result;
}


bool segmentIntersectsCircle( VECTOR2I p1, VECTOR2I p2, VECTOR2I center, double radius )
{
    // Vector from p1 to p2
    VECTOR2I D = p2 - p1;
    // Vector from p1 to center
    VECTOR2I F = p1 - center;

    VECTOR2D d( D.x, D.y );
    VECTOR2D f( F.x, F.y );

    double a = d.x * d.x + d.y * d.y;
    double b = 2 * ( f.x * d.x + f.y * d.y );
    double c = ( f.x * f.x + f.y * f.y ) - radius * radius;

    double discriminant = b * b - 4 * a * c;

    if( discriminant < 0 )
    {
        return false; // No intersection
    }
    else
    {
        discriminant = sqrt( discriminant );
        double t1 = ( -b - discriminant ) / ( 2 * a );
        double t2 = ( -b + discriminant ) / ( 2 * a );

        if( ( t1 >= 0 && t1 <= 1 ) || ( t2 >= 0 && t2 <= 1 ) )
        {
            return true; // Intersection within the segment
        }

        return false;
    }
}

bool SegmentIntersectsBoard( const VECTOR2I& aP1, const VECTOR2I& aP2,
                             const std::vector<BOARD_ITEM*>&       aBe,
                             const std::vector<const BOARD_ITEM*>& aDontTestAgainst )
{
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
            if( segments_intersect( aP1, aP2, d->GetStart(), d->GetEnd() ) )
                return false;
            break;

        case SHAPE_T::RECTANGLE:
        {
            VECTOR2I c1 = d->GetStart();
            VECTOR2I c2( d->GetStart().x, d->GetEnd().y );
            VECTOR2I c3 = d->GetEnd();
            VECTOR2I c4( d->GetEnd().x, d->GetStart().y );

            if( segments_intersect( aP1, aP2, c1, c2 ) || segments_intersect( aP1, aP2, c2, c3 )
                || segments_intersect( aP1, aP2, c3, c4 )
                || segments_intersect( aP1, aP2, c4, c1 ) )
            {
                return false;
            }
            break;
        }
        case SHAPE_T::POLY:
        {
            std::vector<VECTOR2I> points;
            d->DupPolyPointsList( points );

            if ( points.size() < 2 )
                break;
            VECTOR2I prevPoint = points.back();

            for( auto p : points )
            {
                if( segments_intersect( aP1, aP2, prevPoint, p ) )
                    return false;
                prevPoint = p;
            }
            break;
        }
        case SHAPE_T::CIRCLE:
        {
            VECTOR2I center = d->GetCenter();
            double   radius = d->GetRadius();

            if( segmentIntersectsCircle( aP1, aP2, center, radius ) )
                return false;

            break;
        }


        case SHAPE_T::ARC:
        {
            VECTOR2I center = d->GetCenter();
            double   radius = d->GetRadius();

            EDA_ANGLE A, B;
            d->CalcArcAngles( A, B );

            if( segmentIntersectsArc( aP1, aP2, center, radius, A.AsDegrees(), B.AsDegrees() ) )
                return false;

            break;
        }


        default: break;
        }
    }
    return true;
}

bool CheckPathValidity( VECTOR2I aP1, VECTOR2I aP2, std::vector<BOARD_ITEM*> aBe,
                        std::vector<const BOARD_ITEM*> aDontTestAgainst )
{
    return false;
}

std::vector<path_connection> GetPaths( CREEP_SHAPE* aS1, CREEP_SHAPE* aS2, double aMaxWeight )
{
    double                       maxWeight = aMaxWeight;
    double                       maxWeightSquared = maxWeight * maxWeight;
    std::vector<path_connection> result;

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
        return bearc1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( cuarc2 && becircle1 )
        return becircle1->Paths( *bearc2, maxWeight, maxWeightSquared );
    if( cuarc2 && bepoint1 )
        return bepoint1->Paths( *bearc2, maxWeight, maxWeightSquared );
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

double
CreepageGraph::Solve( GraphNode* aFrom, GraphNode* aTo,
                      std::vector<GraphConnection*>& aResult ) // Change to vector of pointers
{
    if( !aFrom || !aTo )
        return 0;

    if( aFrom == aTo )
        return 0;

    // Dijkstra's algorithm for shortest path
    std::unordered_map<GraphNode*, double>     distances;
    std::unordered_map<GraphNode*, GraphNode*> previous;

    auto cmp = [&distances]( GraphNode* left, GraphNode* right )
    {
        if( distances[left] == distances[right] )
            return left > right; // Compare addresses to avoid ties.
        return distances[left] > distances[right];
    };
    std::priority_queue<GraphNode*, std::vector<GraphNode*>, decltype( cmp )> pq( cmp );

    // Initialize distances to infinity for all nodes except the starting node
    for( GraphNode* node : m_nodes )
    {
        if( node != nullptr )
            distances[node] = std::numeric_limits<double>::infinity(); // Set to infinity
    }
    distances[aFrom] = 0.0;
    distances[aTo] = std::numeric_limits<double>::infinity();
    pq.push( aFrom );

    // Dijkstra's main loop
    while( !pq.empty() )
    {
        GraphNode* current = pq.top();
        pq.pop();

        if( current == aTo )
        {
            break; // Shortest path found
        }

        // Traverse neighbors
        for( GraphConnection* connection : current->m_connections )
        {
            GraphNode* neighbor = connection->n1 == current ? connection->n2 : connection->n1;

            if( !neighbor )
                continue;

            double alt = distances[current]
                         + connection->m_path.weight; // Calculate alternative path cost

            if( alt < distances[neighbor] )
            {
                distances[neighbor] = alt;
                previous[neighbor] = current;
                pq.push( neighbor );
            }
        }
    }

    double pathWeight = distances[aTo];

    // If aTo is unreachable, return infinity
    if( pathWeight == std::numeric_limits<double>::infinity() )
    {
        return std::numeric_limits<double>::infinity();
    }

    // Trace back the path from aTo to aFrom
    GraphNode* step = aTo;

    while( step != aFrom )
    {
        GraphNode* prevNode = previous[step];
        for( GraphConnection* connection : step->m_connections )
        {
            if( ( connection->n1 == prevNode && connection->n2 == step )
                || ( connection->n1 == step && connection->n2 == prevNode ) )
            {
                aResult.push_back( connection );
                break;
            }
        }
        step = prevNode;
    }

    return pathWeight;
}

void CreepageGraph::Addshape( const SHAPE& aShape, GraphNode* aConnectTo, BOARD_ITEM* aParent )
{
    CREEP_SHAPE* newshape = nullptr;

    if( !aConnectTo )
        return;

    switch( aShape.Type() )
    {
    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT& segment = dynamic_cast<const SHAPE_SEGMENT&>( aShape );
        CU_SHAPE_SEGMENT*    cuseg =
                new CU_SHAPE_SEGMENT( segment.GetSeg().A, segment.GetSeg().B, segment.GetWidth() );
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
                if ( !( ( subshape->Type() == SH_RECT ) && ( nbShapes == 5 ) ))
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

        for( auto point : lineChain.CPoints() )
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
    default: break;
    }

    if( !newshape )
        return;

    GraphNode* gnShape = nullptr;

    newshape->SetParent( aParent );

    switch( aShape.Type() )
    {
    case SH_SEGMENT: gnShape = AddNode( GraphNode::SEGMENT, newshape, newshape->GetPos() ); break;
    case SH_CIRCLE: gnShape = AddNode( GraphNode::CIRCLE, newshape, newshape->GetPos() ); break;
    case SH_ARC: gnShape = AddNode( GraphNode::ARC, newshape, newshape->GetPos() ); break;
    default: break;
    }

    if( gnShape )
    {
        m_shapeCollection.push_back( newshape );
        gnShape->m_net = aConnectTo->m_net;
        GraphConnection* gc = AddConnection( gnShape, aConnectTo );

        if( gc )
            gc->m_path.m_show = false;
    }
    else
    {
        delete newshape;
        newshape = nullptr;
    }
}

void CreepageGraph::GeneratePaths( double aMaxWeight, PCB_LAYER_ID aLayer, bool aGenerateBoardEdges )
{
    std::vector<GraphNode*> nodes1 = m_nodes;
    std::vector<GraphNode*> nodes2 = m_nodes;


    for( GraphNode* gn1 : nodes1 )
    {
        nodes2.erase( nodes2.begin() );

        if( !gn1 )
            continue;

        if( !gn1->m_parent )
            continue;

        if( !gn1->m_connectDirectly )
            continue;

        if( gn1->m_type == GraphNode::TYPE::VIRTUAL )
            continue;


        for( GraphNode* gn2 : nodes2 )
        {
            if( !gn2 )
                continue;

            if( !gn2->m_parent )
                continue;

            if( gn1->m_parent == gn2->m_parent )
                continue;

            if( !gn2->m_connectDirectly )
                continue;

            if( gn2->m_type == GraphNode::TYPE::VIRTUAL )
                continue;

            if( !aGenerateBoardEdges && !gn1->m_parent->IsConductive()
                && !gn2->m_parent->IsConductive() )
                continue;

            if( ( gn1->m_net == gn2->m_net ) && ( gn1->m_parent->IsConductive() )
                && ( gn2->m_parent->IsConductive() ) )
                continue;

            for( path_connection pc : GetPaths( gn1->m_parent, gn2->m_parent, aMaxWeight ) )
            {
                std::vector<const BOARD_ITEM*> IgnoreForTest;
                IgnoreForTest.push_back( gn1->m_parent->GetParent() );
                IgnoreForTest.push_back( gn2->m_parent->GetParent() );

                if( !pc.isValid( m_board, aLayer, m_boardEdge, IgnoreForTest, m_boardOutline,
                                 { false, true } ) )
                    continue;

                GraphNode* connect1;
                GraphNode* connect2;

                if( gn1->m_parent->GetType() == CREEP_SHAPE::TYPE::POINT )
                {
                    connect1 = gn1;
                }
                else
                {
                    GraphNode* gnt = AddNode( GraphNode::POINT, gn1->m_parent, pc.a1 );
                    gnt->m_connectDirectly = false;

                    if( gn1->m_parent->IsConductive() )
                    {
                        GraphConnection* gc = AddConnection( gn1, gnt );

                        if( gc )
                            gc->m_path.m_show = false;
                    }
                    connect1 = gnt;
                }

                if( gn2->m_parent->GetType() == CREEP_SHAPE::TYPE::POINT )
                {
                    connect2 = gn2;
                }
                else
                {
                    GraphNode* gnt = AddNode( GraphNode::POINT, gn2->m_parent, pc.a2 );
                    gnt->m_connectDirectly = false;

                    if( gn2->m_parent->IsConductive() )
                    {
                        GraphConnection* gc = AddConnection( gn2, gnt );

                        if( gc )
                            gc->m_path.m_show = false;
                    }
                    connect2 = gnt;
                }
                AddConnection( connect1, connect2, pc );
            }
        }
    }
}


void CreepageGraph::Trim( double aWeightLimit )
{
    std::vector<GraphConnection*> toRemove;

    // Collect connections to remove
    for( auto& gc : m_connections )
    {
        if( gc && ( gc->m_path.weight > aWeightLimit ) )
        {
            toRemove.push_back( gc );
        }
    }

    // Remove collected connections
    for( const auto& gc : toRemove )
    {
        RemoveConnection( gc );
    }
}

void CreepageGraph::RemoveConnection( GraphConnection* aGc, bool aDelete )
{
    if( !aGc )
        return;

    for( GraphNode* gn : { aGc->n1, aGc->n2 } )
    {
        if( gn )
        {
            auto& nConns = gn->m_connections;
            nConns.erase( std::remove( nConns.begin(), nConns.end(), aGc ), nConns.end() );

            if( nConns.empty() && aDelete )
            {
                auto it = std::find_if( m_nodes.begin(), m_nodes.end(),
                                        [&gn]( const GraphNode* node )
                                        {
                                            return node == gn;
                                        } );
                if( it != m_nodes.end() )
                {
                    m_nodes.erase( it );
                    delete *it;
                }
            }
        }
    }

    if( aDelete )
    {
        // Remove the connection from the graph's connections
        m_connections.erase( std::remove( m_connections.begin(), m_connections.end(), aGc ),
                             m_connections.end() );
        delete aGc;
    }
}


GraphNode* CreepageGraph::AddNode( GraphNode::TYPE aType, CREEP_SHAPE* parent, VECTOR2I pos )
{
    GraphNode* gn = FindNode( aType, parent, pos );
    if( gn )
        return gn;

    gn = new GraphNode( aType, parent, pos );
    m_nodes.push_back( gn );
    return gn;
}

GraphNode* CreepageGraph::AddNodeVirtual()
{
    //Virtual nodes are always unique, do not try to find them
    GraphNode* gn = new GraphNode( GraphNode::TYPE::VIRTUAL, nullptr );
    m_nodes.push_back( gn );
    return gn;
}


GraphConnection* CreepageGraph::AddConnection( GraphNode* aN1, GraphNode* aN2,
                                               const path_connection& aPc )
{
    if( !aN1 || !aN2 )
        return nullptr;

    GraphConnection* gc = new GraphConnection( aN1, aN2, aPc );
    m_connections.push_back( gc );
    aN1->m_connections.push_back( gc );
    aN2->m_connections.push_back( gc );

    return gc;
}

GraphConnection* CreepageGraph::AddConnection( GraphNode* aN1, GraphNode* aN2 )
{
    if( !aN1 || !aN2 )
        return nullptr;

    path_connection pc;
    pc.a1 = aN1->m_pos;
    pc.a2 = aN2->m_pos;
    pc.weight = 0;

    return AddConnection( aN1, aN2, pc );
}

GraphNode* CreepageGraph::FindNode( GraphNode::TYPE aType, CREEP_SHAPE* aParent, VECTOR2I aPos )
{
    for( GraphNode* gn : m_nodes )
    {
        if( aPos == gn->m_pos && aParent == gn->m_parent && aType == gn->m_type )
        {
            return gn;
        }
    }
    return nullptr;
}

/*
        Physical creepage tests.

        Errors generated:
        - DRCE_CREEPAGE
    */

class DRC_TEST_PROVIDER_CREEPAGE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_CREEPAGE() : DRC_TEST_PROVIDER_CLEARANCE_BASE() {}

    virtual ~DRC_TEST_PROVIDER_CREEPAGE() {}

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "creepage" ); };

    virtual const wxString GetDescription() const override { return wxT( "Tests creepage" ); }

    double GetMaxConstraint( const std::vector<int>& aNetCodes );

private:
    int testCreepage();
    int testCreepage( CreepageGraph& aGraph, int aNetCodeA, int aNetCodeB, PCB_LAYER_ID aLayer );

    void CollectBoardEdges( std::vector<BOARD_ITEM*>& aVector );
    void CollectNetCodes( std::vector<int>& aVector );
};


bool DRC_TEST_PROVIDER_CREEPAGE::Run()
{
    if( !ADVANCED_CFG::GetCfg().m_EnableCreepageDRC )
    {
        return true;
    }

    m_board = m_drcEngine->GetBoard();

    //int errorMax = m_board->GetDesignSettings().m_MaxError;

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_CREEPAGE ) )
    {
        if( !reportPhase( _( "Checking creepage..." ) ) )
            return false; // DRC cancelled

        testCreepage();
    }
    return !m_drcEngine->IsCancelled();
}


GraphNode* FindInGraphNodes( GraphNode* aNode, std::vector<GraphNode*>& aGraph )
{
    if( !aNode )
        return nullptr;

    for( GraphNode* gn : aGraph )
    {
        if( aNode->m_pos == gn->m_pos )
        {
            return gn;
        }
    }
    return nullptr;
}

GraphNode* CreepageGraph::AddNetElements( int aNetCode, PCB_LAYER_ID aLayer, int aMaxCreepage )
{
    GraphNode* virtualNode = AddNodeVirtual();
    virtualNode->m_net = aNetCode;

    for( FOOTPRINT* footprint : m_board.Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetNetCode() != aNetCode )
                continue;

            std::shared_ptr<SHAPE> padShape = pad->GetEffectiveShape( aLayer );

            if( padShape )
            {
                Addshape( *padShape, virtualNode, pad );
            }
        }
    }

    for( PCB_TRACK* track : m_board.Tracks() )
    {
        if( !track )
            continue;

        if( track->GetNetCode() != aNetCode )
            continue;

        if( !track->IsOnLayer( aLayer ) )
            continue;

        if( track->GetEffectiveShape() == nullptr )
            continue;

        Addshape( *( track->GetEffectiveShape() ), virtualNode, track );
    }


    for( ZONE* zone : m_board.Zones() )
    {
        if( !zone )
            continue;

        if( zone->GetNetCode() != aNetCode )
            continue;

        if( zone->GetEffectiveShape( aLayer ) == nullptr )
            continue;

        Addshape( *( zone->GetEffectiveShape( aLayer ) ), virtualNode, zone );
    }

    const DRAWINGS drawings = m_board.Drawings();

    for( BOARD_ITEM* drawing : drawings )
    {
        if( !drawing )
            continue;

        if( !drawing->IsConnected() )
            continue;

        BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( drawing );

        if( !bci )
            continue;

        if( bci->GetNetCode() != aNetCode )
            continue;

        if( bci->IsOnLayer( aLayer ) )
        {
            Addshape( *( bci->GetEffectiveShape() ), virtualNode, bci );
        }
    }


    return virtualNode;
}

int DRC_TEST_PROVIDER_CREEPAGE::testCreepage( CreepageGraph& aGraph, int aNetCodeA, int aNetCodeB,
                                              PCB_LAYER_ID aLayer )
{
    ZONE bci1( m_board );
    ZONE bci2( m_board );
    bci1.SetNetCode( aNetCodeA );
    bci2.SetNetCode( aNetCodeB );
    bci1.SetLayer( aLayer );
    bci2.SetLayer( aLayer );


    DRC_CONSTRAINT constraint;
    constraint = m_drcEngine->EvalRules( CREEPAGE_CONSTRAINT, &bci1, &bci2, aLayer );
    double creepageValue = constraint.Value().Min();
    aGraph.SetTarget( creepageValue );

    if( creepageValue <= 0 )
        return 0;

    // Let's make a quick "clearance test" 
    NETINFO_ITEM* netA = m_board->FindNet( aNetCodeA );
    NETINFO_ITEM* netB = m_board->FindNet( aNetCodeB );

    if ( !netA || !netB )
        return 0;

    if ( netA->GetBoundingBox().Distance( netB->GetBoundingBox() ) > creepageValue )
        return 0;

    GraphNode* NetA = aGraph.AddNetElements( aNetCodeA, aLayer, creepageValue );
    GraphNode* NetB = aGraph.AddNetElements( aNetCodeB, aLayer, creepageValue );


    aGraph.GeneratePaths( creepageValue, aLayer );

    std::vector<GraphNode*> nodes1 = aGraph.m_nodes;
    std::vector<GraphNode*> nodes2 = aGraph.m_nodes;

    alg::for_all_pairs( aGraph.m_nodes.begin(), aGraph.m_nodes.end(),
                        [&]( GraphNode* aN1, GraphNode* aN2 )
                        {
                            if( aN1 == aN2 )
                                return;

                            if( !aN1 || !aN2 )
                                return;

                            if( !( aN1->m_parent ) || !( aN2->m_parent ) )
                                return;

                            if( ( aN1->m_parent ) != ( aN2->m_parent ) )
                                return;


                            if( aN1->m_parent->IsConductive() )
                                return;

                            if( aN1->m_connectDirectly || aN2->m_connectDirectly )
                                return;

                            // We are only looking for points on circles and arcs

                            if( aN1->m_type != GraphNode::POINT )
                                return;

                            if( aN2->m_type != GraphNode::POINT )
                                return;

                            aN1->m_parent->ConnectChildren( aN1, aN2, aGraph );
                        } );

    std::vector<GraphConnection*> shortestPath;
    shortestPath.clear();
    double distance = aGraph.Solve( NetA, NetB, shortestPath );


    if( ( shortestPath.size() > 0 ) && ( distance - creepageValue < 0 ) )
    {
        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CREEPAGE );
        wxString msg = formatMsg( _( "(%s creepage %s; actual %s)" ), constraint.GetName(),
                                  creepageValue, distance );
        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
        drce->SetViolatingRule( constraint.GetParentRule() );

        if( shortestPath.size() >= 4 && shortestPath[1]->n1 && shortestPath[1]->n2 )
            drce->SetItems( shortestPath[1]->n1->m_parent->GetParent(),
                            shortestPath[shortestPath.size() - 2]->n2->m_parent->GetParent() );

        // m_commit is protected and cannot be sent to the lambda function
        BOARD_COMMIT* aCommit = m_commit;

        std::vector<PCB_SHAPE> shortestPathShapes;

        for( GraphConnection* gc : shortestPath )
        {
            if( !gc )
                continue;

            for( PCB_SHAPE sh : gc->GetShapes() )
                shortestPathShapes.push_back( sh );
        }

        m_drcEngine->SetViolationHandler(
                [aCommit, shortestPathShapes]( const std::shared_ptr<DRC_ITEM>& aItem,
                                               VECTOR2I aPos, int aReportLayer )
                {
                    PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aReportLayer );

                    if( !aCommit )
                        return;

                    marker->SetShapes( std::move( shortestPathShapes ) );
                    aCommit->Add( marker );
                } );

        reportViolation( drce, shortestPath[1]->m_path.a2, aLayer );
        
    }
    shortestPath.clear();

    return 1;
}

double DRC_TEST_PROVIDER_CREEPAGE::GetMaxConstraint( const std::vector<int>& aNetCodes )
{
    double         maxConstraint = 0;
    DRC_CONSTRAINT constraint;

    ZONE bci1( m_board );
    ZONE bci2( m_board );


    alg::for_all_pairs( aNetCodes.begin(), aNetCodes.end(),
                        [&]( int aNet1, int aNet2 )
                        {
                            if( aNet1 == aNet2 )
                                return;

                            bci1.SetNetCode( aNet1 );
                            bci2.SetNetCode( aNet2 );

                            for( PCB_LAYER_ID layer : LSET::AllCuMask().CuStack() )
                            {
                                bci1.SetLayer( layer );
                                bci2.SetLayer( layer );
                                constraint = m_drcEngine->EvalRules( CREEPAGE_CONSTRAINT, &bci1,
                                                                     &bci2, layer );
                                double value = constraint.Value().Min();
                                maxConstraint = value > maxConstraint ? value : maxConstraint;
                            }
                        } );

    return maxConstraint;
}

void DRC_TEST_PROVIDER_CREEPAGE::CollectNetCodes( std::vector<int>& aVector )
{
    NETCODES_MAP nets = m_board->GetNetInfo().NetsByNetcode();

    for( auto it = nets.begin(); it != nets.end(); it++ )
    {
        aVector.push_back( it->first );
    }
}

void DRC_TEST_PROVIDER_CREEPAGE::CollectBoardEdges( std::vector<BOARD_ITEM*>& aVector )
{
    if( !m_board )
        return;

    for( BOARD_ITEM* drawing : m_board->Drawings() )
    {
        if( !drawing )
            continue;

        if( drawing->IsOnLayer( Edge_Cuts ) )
        {
            aVector.push_back( drawing );
        }
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        if( !fp )
            continue;

        for( BOARD_ITEM* drawing : fp->GraphicalItems() )
        {
            if( !drawing )
                continue;

            if( drawing->IsOnLayer( Edge_Cuts ) )
            {
                aVector.push_back( drawing );
            }
        }
    }

    for( const PAD* p : m_board->GetPads() )
    {
        if( !p )
            continue;


        if( p->GetAttribute() != PAD_ATTRIB::NPTH )
            continue;

        PCB_SHAPE* s = new PCB_SHAPE( NULL, SHAPE_T::CIRCLE );
        s->SetRadius( p->GetDrillSize().x / 2 );
        s->SetPosition( p->GetPosition() );
        aVector.push_back( s );
    }
}

int DRC_TEST_PROVIDER_CREEPAGE::testCreepage()
{
    if( !m_board )
        return -1;

    std::vector<int> netcodes;

    this->CollectNetCodes( netcodes );
    double maxConstraint = GetMaxConstraint( netcodes );

    if( maxConstraint <= 0 )
        return 0;

    SHAPE_POLY_SET outline;

    if( !m_board->GetBoardPolygonOutlines( outline ) )
        return -1;

    const DRAWINGS drawings = m_board->Drawings();
    CreepageGraph  graph( *m_board );
    graph.m_boardOutline = &outline;

    this->CollectBoardEdges( graph.m_boardEdge );
    graph.TransformEdgeToCreepShapes();
    graph.RemoveDuplicatedShapes();
    graph.TransformCreepShapesToNodes( graph.m_shapeCollection );

    graph.GeneratePaths( maxConstraint, Edge_Cuts );


    int    beNodeSize = graph.m_nodes.size();
    int    beConnectionsSize = graph.m_connections.size();
    bool prevTestChangedGraph = false;

    alg::for_all_pairs( netcodes.begin(), netcodes.end(),
                        [&]( int aNet1, int aNet2 )
                        {
                            if( aNet1 == aNet2 )
                                return;

                            for( PCB_LAYER_ID layer : LSET::AllCuMask().CuStack() )
                            {
                                if( !m_board->IsLayerEnabled( layer ) )
                                    continue;

                                if ( prevTestChangedGraph )
                                {
                                    size_t vectorSize = graph.m_connections.size();

                                    for( size_t i = beConnectionsSize; i < vectorSize; i++ )
                                    {
                                        // We need to remove the connection from its endpoints' lists.
                                        graph.RemoveConnection( graph.m_connections[i], false );
                                        delete graph.m_connections[i];
                                        graph.m_connections[i] = nullptr;
                                    }
                                    graph.m_connections.resize( beConnectionsSize );

                                    vectorSize = graph.m_nodes.size();

                                    for( size_t i = beNodeSize; i < vectorSize; i++ )
                                    {
                                        delete graph.m_nodes[i];
                                        graph.m_nodes[i] = nullptr;
                                    }
                                    graph.m_nodes.resize( beNodeSize );
                                }

                                prevTestChangedGraph = testCreepage( graph, aNet1, aNet2, layer );
                            }
                        } );

    return 1;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CREEPAGE> dummy;
}
