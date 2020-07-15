/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2020 CERN
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

#ifndef __SHAPE_COMPOUND_H
#define __SHAPE_COMPOUND_H

#include <geometry/shape.h>
#include <math/vector2d.h>
#include <math/box2.h>
#include <list>
#include <vector>

class SHAPE_COMPOUND : public SHAPE
{
   public:
      SHAPE_COMPOUND() :
         SHAPE( SH_COMPOUND ),
         m_dirty( true )
         {}


      SHAPE_COMPOUND( const std::vector<SHAPE*>& aShapes );
      
      SHAPE_COMPOUND( const SHAPE_COMPOUND& aOther );
      ~SHAPE_COMPOUND();
      
      SHAPE_COMPOUND* Clone() const override;
      const std::string Format() const override;

      bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr ) const override;

      bool Collide( const SHAPE* aShape, int aClearance, VECTOR2I* aMTV ) const override
      {
         return SHAPE::Collide( aShape, aClearance, aMTV );
      }

      bool Collide( const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr ) const override
      {
         return SHAPE::Collide( aShape, aClearance, aActual );
      }


      const std::vector<SHAPE*>& Shapes() const
      {
         return m_shapes;
      }

      const BOX2I BBox( int aClearance = 0 ) const override;

      int Distance( const SEG& aSeg ) const;

      void Move ( const VECTOR2I& aVector ) override
      {
         for( auto& item : m_shapes )
            item->Move( aVector );
      }

      void AddShape( SHAPE* aShape )
      {
         m_shapes.push_back( aShape );
         m_dirty = true;
      }

   bool Empty() const
   {
      return m_shapes.empty();
   }

    void Rotate( double aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override;

    bool IsSolid() const override;
    
   private:

      BOX2I m_cachedBBox;
      bool m_dirty;
      std::vector<SHAPE*> m_shapes;
};

#endif // __SHAPE_COMPOUND_H
