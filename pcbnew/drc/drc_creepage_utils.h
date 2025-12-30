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


#pragma once

#include <unordered_set>

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
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <board.h>


#include <geometry/shape_circle.h>
#include <geometry/rtree.h>


// Simple wrapper for track segment data in the RTree
struct CREEPAGE_TRACK_ENTRY
{
    SEG      segment;
    PCB_LAYER_ID layer;
};

using TRACK_RTREE = RTree<CREEPAGE_TRACK_ENTRY*, int, 2, double>;

extern bool SegmentIntersectsBoard( const VECTOR2I& aP1, const VECTOR2I& aP2,
                                    const std::vector<BOARD_ITEM*>&       aBe,
                                    const std::vector<const BOARD_ITEM*>& aDontTestAgainst,
                                    int                                   aMinGrooveWidth );


struct PATH_CONNECTION
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
     *
     * @param aBoard The board (used as fallback if no track index provided)
     * @param aLayer The layer to check
     * @param aBoardEdges Board edge items
     * @param aIgnoreForTest Items to ignore in intersection tests
     * @param aOutline Board outline polygon
     * @param aTestLocalConcavity Concavity test flags
     * @param aMinGrooveWidth Minimum groove width
     * @param aTrackIndex Optional spatial index for tracks (if nullptr, falls back to linear search)
     */
    bool isValid( const BOARD& aBoard, PCB_LAYER_ID aLayer,
                  const std::vector<BOARD_ITEM*>&       aBoardEdges,
                  const std::vector<const BOARD_ITEM*>& aIgnoreForTest, SHAPE_POLY_SET* aOutline,
                  const std::pair<bool, bool>& aTestLocalConcavity, int aMinGrooveWidth,
                  TRACK_RTREE* aTrackIndex = nullptr ) const
    {
        if( !aOutline )
            return true; // We keep the segment if there is a problem

        if( !SegmentIntersectsBoard( a1, a2, aBoardEdges, aIgnoreForTest, aMinGrooveWidth ) )
            return false;

        // The mid point should be inside the board.
        // Tolerance of 100nm.

        VECTOR2I midPoint = ( a1 + a2 ) / 2;
        int      tolerance = 100;

        bool contained = aOutline->Contains( midPoint, -1, tolerance )
                         || aOutline->PointOnEdge( midPoint, tolerance );

        if( !contained )
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

        if( aLayer != Edge_Cuts )
        {
            SEG segPath( a1, a2 );

            // Prefer RTree search if available
            if( aTrackIndex )
            {
                // Calculate bounding box of the path segment
                int minX = std::min( (int) a1.x, (int) a2.x );
                int minY = std::min( (int) a1.y, (int) a2.y );
                int maxX = std::max( (int) a1.x, (int) a2.x );
                int maxY = std::max( (int) a1.y, (int) a2.y );

                int searchMin[2] = { minX, minY };
                int searchMax[2] = { maxX, maxY };

                bool intersects = false;

                aTrackIndex->Search( searchMin, searchMax,
                        [&]( CREEPAGE_TRACK_ENTRY* entry ) -> bool
                        {
                            if( entry && entry->layer == aLayer )
                            {
                                if( segPath.Intersects( entry->segment ) )
                                {
                                    intersects = true;
                                    return false; // Stop searching
                                }
                            }
                            return true; // Continue searching
                        } );

                if( intersects )
                    return false;
            }
            else
            {
                // Fallback to linear search if no index provided
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
        }
        return true;
    }
};


class GRAPH_CONNECTION;
class GRAPH_NODE;
class CREEPAGE_GRAPH;
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

    CREEP_SHAPE()
    {
        m_conductive = false;
        m_parent = nullptr;
        m_pos = VECTOR2I( 0, 0 );
        m_type = CREEP_SHAPE::TYPE::UNDEFINED;
    };

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

    virtual void ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                                  CREEPAGE_GRAPH& aG ) const;

    std::vector<PATH_CONNECTION> ReversePaths( const std::vector<PATH_CONNECTION>& aV ) const
    {
        std::vector<PATH_CONNECTION> r;
        r.reserve( aV.size() );

        for( const auto& pc : aV )
        {
            r.emplace_back( pc );
            std::swap( r.back().a1, r.back().a2 );
        }

        return r;
    }

    std::vector<PATH_CONNECTION> Paths( const CREEP_SHAPE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const
    {
        std::vector<PATH_CONNECTION> a;
        return a;
    };

    virtual std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<PATH_CONNECTION> a;
        return a;
    };

    virtual std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<PATH_CONNECTION> a;
        return a;
    };

    virtual std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<PATH_CONNECTION> a;
        return a;
    };

    virtual std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<PATH_CONNECTION> a;
        return a;
    };

    virtual std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<PATH_CONNECTION> a;
        return a;
    };

    virtual std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                                double aMaxSquaredWeight ) const
    {
        std::vector<PATH_CONNECTION> a;
        return a;
    };

    //virtual std::vector<PATH_CONNECTION> GetPathsCuToBe( CREEP_SHAPE* aShape ) const{ std::vector<PATH_CONNECTION> a; return a;};
    bool IsConductive() { return m_conductive; };

protected:
    bool              m_conductive;
    BOARD_ITEM*       m_parent;
    VECTOR2I          m_pos;
    CREEP_SHAPE::TYPE m_type;
};


/** @class CU_SHAPE
 *
 *  @brief Creepage: a conductive shape
 */
class CU_SHAPE : public CREEP_SHAPE
{
public:
    CU_SHAPE() :
            CREEP_SHAPE()
    {
        m_conductive = true;
    };
};

/** @class BE_SHAPE
 *
 *  @brief Creepage: a board edge shape
 */
class BE_SHAPE : public CREEP_SHAPE
{
public:
    BE_SHAPE() :
            CREEP_SHAPE()
    {
        m_conductive = false;
    };
};

/** @class CU_SHAPE_SEGMENT
 *
 *  @brief Creepage: a conductive segment
 */
class CU_SHAPE_SEGMENT : public CU_SHAPE
{
public:
    CU_SHAPE_SEGMENT( VECTOR2I aStart, VECTOR2I aEnd, double aWidth = 0 ) :
            CU_SHAPE()
    {
        m_start = aStart;
        m_end = aEnd;
        m_width = aWidth;
    }

    VECTOR2I GetStart() const { return m_start; };
    VECTOR2I GetEnd() const { return m_end; };
    double   GetWidth() const { return m_width; };

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
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
    CU_SHAPE_CIRCLE( VECTOR2I aPos, double aRadius = 0 ) :
            CU_SHAPE()
    {
        m_pos = aPos;
        m_radius = aRadius;
    }

    VECTOR2I GetPos() const { return m_pos; };
    int      GetRadius() const override { return m_radius; };

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;
    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

protected:
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
                  VECTOR2D aStartPoint, VECTOR2D aEndPoint ) :
            CU_SHAPE_CIRCLE( aPos, aRadius ),
            m_startAngle( aStartAngle ),
            m_endAngle( aEndAngle ),
            m_startPoint( aStartPoint ),
            m_endPoint( aEndPoint )
    {
        m_type = CREEP_SHAPE::TYPE::ARC;
        m_width = 0;
    }

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
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
    int       m_width;
    EDA_ANGLE m_startAngle;
    EDA_ANGLE m_endAngle;
    VECTOR2I  m_startPoint;
    VECTOR2I  m_endPoint;
};

/** @class GRAPH_NODE
 *
 *  @brief a node in a @class CREEPAGE_GRAPH
 */
class GRAPH_NODE
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

    GRAPH_NODE( GRAPH_NODE::TYPE aType, CREEP_SHAPE* aParent, const VECTOR2I& aPos = VECTOR2I() ) :
            m_parent( aParent ),
            m_pos( aPos ),
            m_type( aType )
    {
        m_node_conns = {};
        m_virtual = false;
        m_connectDirectly = true;
        m_net = -1;
    };

    ~GRAPH_NODE() {};

public:
    CREEP_SHAPE*                                m_parent;
    std::set<std::shared_ptr<GRAPH_CONNECTION>> m_node_conns;
    VECTOR2I                                    m_pos;

    // Virtual nodes are connected with a 0 weight connection to equivalent net ( same net or netclass )
    bool m_virtual ;
    bool m_connectDirectly;
    int  m_net;

    GRAPH_NODE::TYPE m_type;
};

/** @class GRAPH_CONNECTION
 *
 *  @brief a connection in a @class CREEPAGE_GRAPH
 */
class GRAPH_CONNECTION
{
public:
    GRAPH_CONNECTION( std::shared_ptr<GRAPH_NODE>& aN1, std::shared_ptr<GRAPH_NODE>& aN2,
                      const PATH_CONNECTION& aPc ) :
            n1( aN1 ),
            n2( aN2 )
    {
        m_path = aPc;
        m_forceStraightLine = false;
    };

    void GetShapes( std::vector<PCB_SHAPE>& aShapes );

public:
    std::shared_ptr<GRAPH_NODE> n1;
    std::shared_ptr<GRAPH_NODE> n2;
    PATH_CONNECTION             m_path;
    bool                        m_forceStraightLine;
};


/** @class BE_SHAPE_POINT
 *
 *  @brief Creepage: a board edge point
 */
class BE_SHAPE_POINT : public BE_SHAPE
{
public:
    BE_SHAPE_POINT( VECTOR2I aPos ) :
            BE_SHAPE()
    {
        m_pos = aPos;
        m_type = CREEP_SHAPE::TYPE::POINT;
    }

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    }

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };


    void ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                          CREEPAGE_GRAPH& aG ) const override;
};

/** @class BE_SHAPE_CIRCLE
 *
 *  @brief Creepage: a board edge circle
 */
class BE_SHAPE_CIRCLE : public BE_SHAPE
{
public:
    BE_SHAPE_CIRCLE( VECTOR2I aPos = VECTOR2I( 0, 0 ), int aRadius = 0 ) :
            BE_SHAPE()
    {
        m_pos = aPos;
        m_radius = aRadius;
        m_type = CREEP_SHAPE::TYPE::CIRCLE;
    }

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };


    int  GetRadius() const override { return m_radius; }

    void ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                          CREEPAGE_GRAPH& aG ) const override;

    void ShortenChildDueToGV( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                              CREEPAGE_GRAPH& aG, double aNormalWeight ) const;


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
                  VECTOR2D aStartPoint, VECTOR2D aEndPoint ) :
            BE_SHAPE_CIRCLE( aPos, aRadius ),
            m_startAngle( aStartAngle ), m_endAngle( aEndAngle ),
            m_startPoint( aStartPoint ), m_endPoint( aEndPoint )
    {
        m_type = CREEP_SHAPE::TYPE::ARC;
    }

    void ConnectChildren( std::shared_ptr<GRAPH_NODE>& a1, std::shared_ptr<GRAPH_NODE>& a2,
                          CREEPAGE_GRAPH& aG ) const override;


    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_POINT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<PATH_CONNECTION> Paths( const BE_SHAPE_ARC& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override;

    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_SEGMENT& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_CIRCLE& aS2, double aMaxWeight,
                                        double aMaxSquaredWeight ) const override
    {
        return ReversePaths( aS2.Paths( *this, aMaxWeight, aMaxSquaredWeight ) );
    };
    std::vector<PATH_CONNECTION> Paths( const CU_SHAPE_ARC& aS2, double aMaxWeight,
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
    EDA_ANGLE m_startAngle;
    EDA_ANGLE m_endAngle;
    VECTOR2I  m_startPoint;
    VECTOR2I  m_endPoint;
};


/** @class CREEPAGE_GRAPH
 *
 *  @brief A graph with nodes and connections for creepage calculation
 */
class CREEPAGE_GRAPH
{
public:
    CREEPAGE_GRAPH( BOARD& aBoard ) :
            m_board( aBoard )
    {
        m_boardOutline = nullptr;
        m_minGrooveWidth = 0;
        m_creepageTarget = -1;
        m_creepageTargetSquared = -1;
    };

    ~CREEPAGE_GRAPH()
    {
        for( CREEP_SHAPE* cs : m_shapeCollection )
        {
            if( cs )
            {
                delete cs;
                cs = nullptr;
            }
        }

        // Clear out the circular shared pointer references
        for( std::shared_ptr<GRAPH_NODE>& n : m_nodes )
        {
            if( n )
            {
                n->m_node_conns.clear();
                n = nullptr;
            }
        }

        for( std::shared_ptr<GRAPH_CONNECTION>& c : m_connections )
        {
            if( c )
                c = nullptr;
        }
    };

    void TransformEdgeToCreepShapes();
    void TransformCreepShapesToNodes(std::vector<CREEP_SHAPE*>& aShapes);
    void RemoveDuplicatedShapes();

    // Add a node to the graph. If an equivalent node exists, returns the pointer of the existing node instead
    std::shared_ptr<GRAPH_NODE> AddNode( GRAPH_NODE::TYPE aType, CREEP_SHAPE* aParent = nullptr,
                                         const VECTOR2I& aPos = VECTOR2I() );

    std::shared_ptr<GRAPH_NODE> AddNodeVirtual();

    std::shared_ptr<GRAPH_CONNECTION> AddConnection( std::shared_ptr<GRAPH_NODE>& aN1,
                                                     std::shared_ptr<GRAPH_NODE>& aN2,
                                                     const PATH_CONNECTION&      aPc );

    std::shared_ptr<GRAPH_CONNECTION> AddConnection( std::shared_ptr<GRAPH_NODE>& aN1,
                                                     std::shared_ptr<GRAPH_NODE>& aN2 );

    std::shared_ptr<GRAPH_NODE> FindNode( GRAPH_NODE::TYPE aType, CREEP_SHAPE* aParent,
                                          const VECTOR2I& aPos );

    void RemoveConnection( const std::shared_ptr<GRAPH_CONNECTION>&, bool aDelete = false );

    void Trim( double aWeightLimit );

    void Addshape( const SHAPE& aShape, std::shared_ptr<GRAPH_NODE>& aConnectTo,
                   BOARD_ITEM* aParent = nullptr );

    double Solve( std::shared_ptr<GRAPH_NODE>& aFrom, std::shared_ptr<GRAPH_NODE>& aTo,
                  std::vector<std::shared_ptr<GRAPH_CONNECTION>>& aResult );

    void GeneratePaths( double aMaxWeight, PCB_LAYER_ID aLayer );

    std::shared_ptr<GRAPH_NODE> AddNetElements( int aNetCode, PCB_LAYER_ID aLayer, int aMaxCreepage );

    void   SetTarget( double aTarget );
    double GetTarget() { return m_creepageTarget; };

    struct GraphNodeHash
    {
        std::size_t operator()(const std::shared_ptr<GRAPH_NODE>& node) const
        {
            return hash_val(node->m_type, node->m_parent, node->m_pos.x, node->m_pos.y);
        }
    };

    struct GraphNodeEqual
    {
        bool operator()(const std::shared_ptr<GRAPH_NODE>& lhs, const std::shared_ptr<GRAPH_NODE>& rhs) const
        {
            return lhs->m_type == rhs->m_type && lhs->m_parent == rhs->m_parent && lhs->m_pos == rhs->m_pos;
        }
    };

public:
    BOARD&                                         m_board;
    std::vector<BOARD_ITEM*>                       m_boardEdge;
    SHAPE_POLY_SET*                                m_boardOutline;
    std::vector<std::shared_ptr<GRAPH_NODE>>       m_nodes;
    std::vector<std::shared_ptr<GRAPH_CONNECTION>> m_connections;
    std::vector<CREEP_SHAPE*>                      m_shapeCollection;

    // This is a duplicate of m_nodes, but it is used to quickly find a node rather than iterating through m_nodes
    std::unordered_set<std::shared_ptr<GRAPH_NODE>, GraphNodeHash, GraphNodeEqual> m_nodeset;

    int m_minGrooveWidth;

private:
    double m_creepageTarget;
    double m_creepageTargetSquared;
};

