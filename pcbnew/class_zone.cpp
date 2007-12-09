/**********************************/
/* classes to handle copper zones */
/**********************************/

#include "fctsys.h"
#include "wxstruct.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

/************************/
/* class ZONE_CONTAINER */
/************************/

ZONE_CONTAINER::ZONE_CONTAINER (BOARD * parent):
	BOARD_ITEM (parent, TYPEZONE_CONTAINER)
{
    m_NetCode = -1;              // Net number for fast comparisons
}


ZONE_CONTAINER::~ZONE_CONTAINER()
{
}

bool ZONE_CONTAINER::Save( FILE* aFile ) const
{
	return true;
}


/**********************/
/* Class EDGE_ZONE */
/**********************/

/* Constructor */
EDGE_ZONE::EDGE_ZONE( BOARD_ITEM* parent ) :
    DRAWSEGMENT( parent, TYPEEDGEZONE )
{
    m_Width = 2;        // a minimum for visibility, while dragging
}


/* Destructor */
EDGE_ZONE:: ~EDGE_ZONE()
{
}


bool EDGE_ZONE::Save( FILE* aFile ) const
{
    if( GetState( DELETED ) )
        return true;

    int ret = fprintf( aFile, "ZE %d %d %d %d %d %lX %X\n",
            m_Start.x, m_Start.y,
            m_End.x, m_End.y,
            m_Angle,
            m_TimeStamp,
            ReturnStatus()
            );
    
    return (ret > 14 );    
}

