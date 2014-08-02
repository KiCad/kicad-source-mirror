/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 *
 * Contact information: E-mail: tor.dokken@sintef.no                      
 * SINTEF ICT, Department of Applied Mathematics,                         
 * P.O. Box 124 Blindern,                                                 
 * 0314 Oslo, Norway.                                                     
 *
 * This file is part of TTL.
 *
 * TTL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version. 
 *
 * TTL is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with TTL. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In accordance with Section 7(b) of the GNU Affero General Public
 * License, a covered work must retain the producer line in every data
 * file that is created or manipulated using TTL.
 *
 * Other Usage
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial activities involving the TTL library without
 * disclosing the source code of your own applications.
 *
 * This file may be used in accordance with the terms contained in a
 * written agreement between you and SINTEF ICT. 
 */

#ifndef _HALF_EDGE_DART_
#define _HALF_EDGE_DART_

#include <ttl/halfedge/hetriang.h>

namespace hed
{
/**
 * \class Dart
 * \brief \b %Dart class for the half-edge data structure.
 *
 * See \ref api for a detailed description of how the member functions
 * should be implemented.
 */
class DART
{
    EDGE_PTR m_edge;

    /// Dart direction: true if dart is counterclockwise in face
    bool m_dir;

public:
    /// Default constructor
    DART()
    {
        m_dir = true;
    }

    /// Constructor
    DART( const EDGE_PTR& aEdge, bool aDir = true )
    {
        m_edge = aEdge;
        m_dir = aDir;
    }

    /// Copy constructor
    DART( const DART& aDart )
    {
        m_edge = aDart.m_edge;
        m_dir  = aDart.m_dir;
    }

    /// Destructor
    ~DART()
    {
    }

    /// Assignment operator
    DART& operator=( const DART& aDart )
    {
        if( this == &aDart )
            return *this;

        m_edge = aDart.m_edge;
        m_dir = aDart.m_dir;

        return *this;
    }

    /// Comparing dart objects
    bool operator==( const DART& aDart ) const
    {
        return ( aDart.m_edge == m_edge && aDart.m_dir == m_dir );
    }

    /// Comparing dart objects
    bool operator!=( const DART& aDart ) const
    {
        return !( aDart == *this );
    }

    /// Maps the dart to a different node
    DART& Alpha0()
    {
        m_dir = !m_dir;
        return *this;
    }

    /// Maps the dart to a different edge
    DART& Alpha1()
    {
        if( m_dir )
        {
            m_edge = m_edge->GetNextEdgeInFace()->GetNextEdgeInFace();
            m_dir = false;
        }
        else
        {
            m_edge = m_edge->GetNextEdgeInFace();
            m_dir = true;
        }

        return *this;
    }

    /// Maps the dart to a different triangle. \b Note: the dart is not changed if it is at the boundary!
    DART& Alpha2()
    {
        if( m_edge->GetTwinEdge() )
        {
            m_edge = m_edge->GetTwinEdge();
            m_dir = !m_dir;
        }

        // else, the dart is at the boundary and should not be changed
        return *this;
    }

    /** @name Utilities not required by TTL */
    //@{
    void Init( const EDGE_PTR& aEdge, bool aDir = true )
    {
        m_edge = aEdge;
        m_dir = aDir;
    }

    double X() const
    {
        return GetNode()->GetX();
    }

    double Y() const
    {
        return GetNode()->GetY();
    }

    bool IsCCW() const
    {
        return m_dir;
    }

    const NODE_PTR& GetNode() const
    {
        return m_dir ? m_edge->GetSourceNode() : m_edge->GetTargetNode();
    }

    const NODE_PTR& GetOppositeNode() const
    {
        return m_dir ? m_edge->GetTargetNode() : m_edge->GetSourceNode();
    }

    EDGE_PTR& GetEdge()
    {
        return m_edge;
    }

    //@} // End of Utilities not required by TTL
};

} // End of hed namespace

#endif
