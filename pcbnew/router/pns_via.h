/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#ifndef __PNS_VIA_H
#define __PNS_VIA_H

#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>

#include "pns_item.h"

class PNS_NODE;

class PNS_VIA : public PNS_ITEM 
{
	public:
		PNS_VIA( ):
			PNS_ITEM (VIA) {};

		PNS_VIA( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers, int aDiameter, int aNet = -1) :
			PNS_ITEM (VIA) {
				SetNet(aNet);
				SetLayers(aLayers);
				m_pos = aPos;
				m_diameter = aDiameter;
				m_shape = SHAPE_CIRCLE(aPos, aDiameter/2);
			};


		PNS_VIA(const PNS_VIA& b) : PNS_ITEM(VIA)
		{
			SetNet(b.GetNet());
			SetLayers(b.GetLayers());
			m_pos = b.m_pos;
			m_diameter = b.m_diameter;
			m_shape = SHAPE_CIRCLE(m_pos, m_diameter/2);
		}

		const VECTOR2I& GetPos() const
		{
			return m_pos;
		}

		void SetPos( const VECTOR2I& aPos ) 
		{
			m_pos = aPos;
			m_shape.SetCenter(aPos);
		}

		int GetDiameter() const
		{
			return m_diameter;
		}

		void SetDiameter(int aDiameter)
		{
			m_diameter = aDiameter;
			m_shape.SetRadius(m_diameter/2);
		}

		int GetDrill() const
		{
			return m_drill;
		}

		void SetDrill(int aDrill)
		{
			m_drill = aDrill;
		}

		bool PushoutForce ( PNS_NODE *aNode, const VECTOR2I &aDirection, VECTOR2I& aForce, bool aSolidsOnly = true, int aMaxIterations = 10);

		const SHAPE *GetShape() const
		{
			return &m_shape;
		}

		PNS_VIA *Clone() const
		{
			PNS_VIA *v = new PNS_VIA();

			v->SetNet(GetNet());
			v->SetLayers(GetLayers());
			v->m_pos = m_pos;
			v->m_diameter = m_diameter;
			v->m_shape = SHAPE_CIRCLE(m_pos, m_diameter/2);
		
			return v;
		}

		const SHAPE_LINE_CHAIN Hull(int aClearance = 0, int aWalkaroundThickness = 0) const;

	private:
		
		int m_diameter;
		int m_drill;
		VECTOR2I m_pos;
		SHAPE_CIRCLE m_shape;
};

#endif
