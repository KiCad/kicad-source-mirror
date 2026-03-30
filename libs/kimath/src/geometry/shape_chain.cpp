#include <cstdio>
#include <vector>

#include <math/vector2d.h>
#include <math/box2.h>
#include <geometry/seg.h>


#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_chain.h>


const VECTOR2I SHAPE_CHAIN::NearestPoint( const VECTOR2I& aP, bool aAllowInternalShapePoints ) const
{
    assert( false );
    return VECTOR2I();
}

const VECTOR2I SHAPE_CHAIN::NearestPoint( const SEG& aSeg, int& aDistance ) const
{
    assert( false );
    return VECTOR2I();
}

int SHAPE_CHAIN::NearestShape( const VECTOR2I& aP ) const
{
    assert( false );
    return 0;
}

void SHAPE_CHAIN::Replace( int aStartIndex, int aEndIndex, const SHAPE_CHAIN& aLine )
{
    assert( false );
}

void SHAPE_CHAIN::Replace( int aStartIndex, int aEndIndex, const VECTOR2I& aP )
{
    assert( false );
}

int SHAPE_CHAIN::Split( const VECTOR2I& aP, bool aExact )
{
    assert( false );
    return 0;
}

void SHAPE_CHAIN::Split( const VECTOR2I& aStart, const VECTOR2I& aEnd, SHAPE_CHAIN& aPre, SHAPE_CHAIN& aMid,
                         SHAPE_CHAIN& aPost ) const
{
    assert( false );
}

void SHAPE_CHAIN::Simplify( int aTolerance )
{
    assert( false );
}

int SHAPE_CHAIN::Find( const VECTOR2I& aP, int aThreshold ) const
{
    return 0;
}

bool SHAPE_CHAIN::Collide( const VECTOR2I& aP, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    assert( false );
    return false;
}

bool SHAPE_CHAIN::Collide( const SEG& aSeg, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    assert( false );
    return false;
}

const SHAPE_LINE_CHAIN SHAPE_CHAIN::ToSLC() const
{
    assert( false );
    return SHAPE_LINE_CHAIN();
}

const VECTOR2I SHAPE_CHAIN::PointAlong( int aPathLength ) const
{
    assert( false );
    return VECTOR2I();
}

const BOX2I SHAPE_CHAIN::BBox( int aClearance ) const
{
    BOX2I result;
    
    for( auto sh : m_shapes )
        result.Merge( sh->BBox() );

    return result;
}

void SHAPE_CHAIN::SetWidth( int aWidth )
{
    for( auto sh : m_shapes )
        sh->SetWidth( aWidth );
}
