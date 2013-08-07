/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __TOOL_BASE_H
#define __TOOL_BASE_H

#include <cassert>
#include <base_struct.h>    // for KICAD_T

#include <tool/tool_event.h>
#include <tool/delegate.h>

class EDA_ITEM;
class TOOL_MANAGER;

namespace KiGfx
{
class VIEW;
class VIEW_CONTROLS;
};

enum TOOL_Type
{
    TOOL_Interactive = 0x1,
    TOOL_Batch       = 0x2
};

typedef int TOOL_ID;
typedef DELEGATE<int, TOOL_EVENT&> TOOL_STATE_FUNC;

/**
 * Class TOOL_BASE
 *
 * Base abstract interface for all kinds of tools
 */

class TOOL_BASE 
{
public:

	TOOL_BASE( TOOL_Type aType, TOOL_ID aId, const std::string& aName = std::string( "" ) ) :
		m_type( aType ),
		m_toolId( aId ),
		m_toolName( aName ) {};

	virtual ~TOOL_BASE() {};

	TOOL_Type GetType() const
	{
		return m_type;
	}

	TOOL_ID GetId() const
	{
		return m_toolId;
	}

	const std::string& GetName() const
	{
		return m_toolName;
	}

	TOOL_MANAGER* GetManager()
	{
		return m_toolMgr;
	}
	
protected:
	friend class TOOL_MANAGER;

	/**
	 * Function attachManager()
	 *
	 * Sets the TOOL_MANAGER the tool will belong to.
	 * Called by TOOL_MANAGER::RegisterTool()
	 */
	void attachManager( TOOL_MANAGER *aManager );

	KiGfx::VIEW* getView();
	KiGfx::VIEW_CONTROLS* getViewControls();
	
	/**
	 * Function getEditFrame()
	 * 
	 * Returns the application window object, casted to requested user type, possibly with
	 * run-time type check
	 */
	template<typename T>
	T *getEditFrame()
	{
		return static_cast<T*>( getEditFrameInt() );
	}

	/**
	 * Function getModel()
	 * 
	 * Returns the model object if it matches the requested type.
	 */
	template<typename T> 
	T* getModel( KICAD_T modelType ) 
	{
		EDA_ITEM *m = getModelInt();
//		assert(modelType == m->Type());
		return static_cast<T*>( m );
	}

protected:
	TOOL_Type m_type;
	TOOL_ID m_toolId;
	std::string m_toolName;
	TOOL_MANAGER* m_toolMgr;

private:
	// hide the implementation to avoid spreading half of
	// kicad and wxWidgets headers to the tools that may not need them at all!
	EDA_ITEM* getModelInt();
	wxWindow* getEditFrameInt();
};

#endif
