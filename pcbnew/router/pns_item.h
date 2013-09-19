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

#ifndef __PNS_ITEM_H
#define __PNS_ITEM_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_layerset.h"

class BOARD_ITEM;
class PNS_NODE;

/**
 * Class PNS_ITEM
 *
 * Base class for PNS router board items. Implements the shared properties of all PCB items -
 * net, spanned layers, geometric shape & refererence to owning model.
 */
 
class PNS_ITEM 
{
public:

	static const int UnusedNet = INT_MAX;

	///> Supported item types
	enum PnsKind {
		SOLID 		= 1,
		LINE 		= 2,
		JOINT 		= 4,
		SEGMENT 	= 8,
		VIA 		= 16,
		ANY 		= 0xff
	};

	PNS_ITEM(PnsKind aKind)
	{	
		m_net = UnusedNet;
		m_movable = true;
		m_kind = aKind;
		m_parent = NULL;
		m_world = NULL;
		m_owner = NULL;
	}

	PNS_ITEM( const PNS_ITEM& aOther )
	{
		m_layers = aOther.m_layers;
		m_net = aOther.m_net;
		m_movable = aOther.m_movable;
		m_kind = aOther.m_kind;
		m_world = aOther.m_world;
		m_parent = aOther.m_parent;
		m_owner = NULL;
	}

	virtual ~PNS_ITEM();
	
	virtual PNS_ITEM *Clone() const = 0;

	///> Returns a convex polygon "hull" of a the item, that is used as the walkaround
	///  path. 
	///  aClearance defines how far from the body of the item the hull should be,
	///  aWalkaroundThickness is the width of the line that walks around this hull.
	virtual const SHAPE_LINE_CHAIN Hull(int aClearance = 0, int aWalkaroundThickness = 0) const 
	{
		return SHAPE_LINE_CHAIN();
	};


	
	PnsKind GetKind() const 		{  return m_kind; }
	bool OfKind( int aKind ) const 	{ return (aKind & m_kind) != 0; }
	
	const std::string GetKindStr() const;
	
	///> Gets/Sets the corresponding parent object in the host application's model (pcbnew) 
	void SetParent(BOARD_ITEM *aParent) { m_parent = aParent; }
	BOARD_ITEM *GetParent() const 	{  return m_parent;  }

	///> Net accessors
	int GetNet() const 				{ return m_net; }
	void SetNet(int aNet) 			{ m_net = aNet; }

	///> Layers accessors
	const PNS_LAYERSET& GetLayers() const 				{	return m_layers;	}
	void SetLayers ( const PNS_LAYERSET& aLayers ) 		{ m_layers = aLayers; }
	void SetLayer ( int aLayer ) 						{ m_layers = PNS_LAYERSET (aLayer, aLayer); }

	///> Ownership management. An item can belong to a single PNS_NODE or stay unowned.
	void SetOwner (PNS_NODE *aOwner) 			{ m_owner = aOwner; }
	bool BelongsTo (PNS_NODE *aNode) const 		{ return m_owner == aNode; }
	PNS_NODE *GetOwner() const 					{ return m_owner; }

	///> Sets the world that is used for collision resolution.
	void SetWorld (PNS_NODE *aWorld) 			{ m_world = aWorld; }
	PNS_NODE *GetWorld() const 					{ return m_world; }


	///> Collision function. Checks if the item aOther is closer to us than
	/// aClearance and returns true if so. It can also calculate a minimum translation vector that resolves the
	/// collision if needed.
	virtual bool Collide( const PNS_ITEM *aOther, int aClearance, bool aNeedMTV, VECTOR2I& aMTV ) const;

	///> A shortcut without MTV calculation
	bool Collide( const PNS_ITEM *aOther, int aClearance ) const
	{
		VECTOR2I dummy;
		return Collide(aOther, aClearance, false, dummy);
	}
	
	///> Returns the geometric shape of the item
	virtual	const SHAPE* GetShape() const { 
		return NULL;
	}

private:
	bool collideSimple ( const PNS_ITEM *aOther, int aClearance, bool aNeedMTV, VECTOR2I& aMTV ) const;

protected:

	PnsKind m_kind;
	
	BOARD_ITEM *m_parent;
	PNS_NODE *m_world;
	PNS_NODE *m_owner;
	PNS_LAYERSET m_layers;

	bool m_movable;
	int m_net;
};

#endif // __PNS_ITEM_H

