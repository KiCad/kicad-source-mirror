/**
 * @file class_via_dimension.h
 * @brief Class via dimension.
 */

#ifndef CLASS_VIA_DIMENSION_H
#define CLASS_VIA_DIMENSION_H

#include "lengthpcb.h"
/** a small helper class to handle a stock of specific vias diameter and drill pair
 * in the BOARD class
 */
class VIA_DIMENSION
{
public:
    LENGTH_PCB m_Diameter;     // <= 0 means use Netclass via diameter
    LENGTH_PCB m_Drill;        // <= 0 means use Netclass via drill

    VIA_DIMENSION()
    {
        m_Diameter = FROM_LEGACY_LU( 0 );
        m_Drill    = FROM_LEGACY_LU( 0 );
    }


    bool operator ==( const VIA_DIMENSION& other ) const
    {
        return (m_Diameter == other.m_Diameter) && (m_Drill == other.m_Drill);
    }


    bool operator <( const VIA_DIMENSION& other ) const
    {
        if( m_Diameter != other.m_Diameter )
            return m_Diameter < other.m_Diameter;

        return m_Drill < other.m_Drill;
    }
};

#endif /* CLASS_VIA_DIMENSION_H */