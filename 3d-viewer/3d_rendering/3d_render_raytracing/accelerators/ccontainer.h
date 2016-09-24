/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file  ccontainer.h
 * @brief
 */

#ifndef _CCONTAINER_H_
#define _CCONTAINER_H_

#include "../shapes3D/cobject.h"
#include <list>
#include <vector>

typedef std::list<COBJECT *> LIST_OBJECT;
typedef std::vector<COBJECT *> VECTOR_OBJECT;
typedef std::vector<const COBJECT *> CONST_VECTOR_OBJECT;

class  CGENERICCONTAINER
{
protected:
    CBBOX       m_bbox;
    LIST_OBJECT m_objects;

public:
    CGENERICCONTAINER();

    virtual ~CGENERICCONTAINER();

    void Add( COBJECT *aObject )
    {
        if( aObject ) // Only add if it is a valid pointer
        {
            m_objects.push_back( aObject );
            m_bbox.Union( aObject->GetBBox() );
        }
    }

    void Clear();

    const LIST_OBJECT &GetList() const { return m_objects; }

    void ConvertTo( CONST_VECTOR_OBJECT &aOutVector ) const;

    const CBBOX &GetBBox() const { return m_bbox; }

    virtual bool Intersect( const RAY &aRay, HITINFO &aHitInfo ) const = 0;
    virtual bool IntersectP( const RAY &aRay, float aMaxDistance ) const = 0;

private:
};


class  CCONTAINER : public CGENERICCONTAINER
{
public:
    bool Intersect( const RAY &aRay, HITINFO &aHitInfo ) const override;
    bool IntersectP( const RAY &aRay, float aMaxDistance ) const override;
};

#endif // _CCONTAINER_H_
