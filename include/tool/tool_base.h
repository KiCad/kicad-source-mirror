/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef TOOL_BASE_H
#define TOOL_BASE_H

#include <cassert>
#include <functional>
#include <string>
#include <wx/debug.h>

class EDA_ITEM;
class TOOL_EVENT;
class TOOL_MANAGER;
class TOOLS_HOLDER;

namespace KIGFX
{
class VIEW;
class VIEW_CONTROLS;
}

enum TOOL_TYPE
{
    /// Tool that interacts with the user
    INTERACTIVE = 0x01,

    /// Tool that runs in the background without any user intervention
    BATCH       = 0x02
};

/// Unique identifier for tools
typedef int TOOL_ID;

using TOOL_STATE_FUNC = std::function<int(const TOOL_EVENT&)>;


/**
 * Base abstract interface for all kinds of tools.
 */

class TOOL_BASE
{
public:
    TOOL_BASE( TOOL_TYPE aType, TOOL_ID aId, const std::string& aName = std::string( "" ) ) :
        m_type( aType ),
        m_toolId( aId ),
        m_toolName( aName ),
        m_toolMgr( nullptr ) {};

    virtual ~TOOL_BASE() {};

    /// Determine the reason of reset for a tool.
    enum RESET_REASON
    {
        RUN,                ///< Tool is invoked after being inactive
        MODEL_RELOAD,       ///< Model changes (the sheet for a schematic)
        SUPERMODEL_RELOAD,  ///< For schematics, the entire schematic changed, not just the sheet
        GAL_SWITCH,         ///< Rendering engine changes
        REDRAW,             ///< Full drawing refresh
        SHUTDOWN            ///< Tool is being shut down
    };

    /**
     * Init() is called once upon a registration of the tool.
     *
     * @return True if the initialization went fine, false - otherwise.
     */
    virtual bool Init()
    {
        return true;
    }

    /**
     * Bring the tool to a known, initial state.
     *
     * If the tool claimed anything from the model or the view, it must release it when its
     * reset.
     * @param aReason contains information about the reason of tool reset.
     */
    virtual void Reset( RESET_REASON aReason ) = 0;

    /**
     * Return the type of the tool.
     *
     * @return The type of the tool.
     */
    TOOL_TYPE GetType() const
    {
        return m_type;
    }

    /**
     * Return the unique identifier of the tool.
     *
     * The identifier is set by an instance of #TOOL_MANAGER.
     *
     * @return Identifier of the tool.
     */
    TOOL_ID GetId() const
    {
        return m_toolId;
    }

    /**
     * Return the name of the tool.
     *
     * Tool names are expected to obey the format: application.ToolName (eg.
     * pcbnew.InteractiveSelection).
     *
     * @return The name of the tool.
     */
    const std::string& GetName() const
    {
        return m_toolName;
    }

    /**
     * Return the instance of #TOOL_MANAGER that takes care of the tool.
     *
     * @return Instance of the #TOOL_MANAGER or NULL if there is no associated tool manager.
     */
    TOOL_MANAGER* GetManager() const
    {
        return m_toolMgr;
    }

    //TOOL_SETTINGS& GetAdapter();

    bool IsToolActive() const;

protected:
    friend class TOOL_MANAGER;

    /**
     * Set the #TOOL_MANAGER the tool will belong to.
     *
     * Called by #TOOL_MANAGER::RegisterTool()
     */
    void attachManager( TOOL_MANAGER* aManager );

    /**
     * Returns the instance of #VIEW object used in the application. It allows tools to draw.
     *
     * @return The instance of VIEW.
     */
    KIGFX::VIEW* getView() const;

    /**
     * Return the instance of VIEW_CONTROLS object used in the application.
     *
     * It allows tools to read & modify user input and its settings (eg. show cursor, enable
     * snapping to grid, etc.).
     *
     * @return The instance of VIEW_CONTROLS.
     */
    KIGFX::VIEW_CONTROLS* getViewControls() const;

    /**
     * Return the application window object, casted to requested user type.
     */
    template <typename T>
    T* getEditFrame() const
    {
#if !defined( QA_TEST )   // Dynamic casts give the linker a seizure in the test framework
        wxASSERT( dynamic_cast<T*>( getToolHolderInternal() ) );
#endif
        return static_cast<T*>( getToolHolderInternal() );
    }

    /**
     * Return the model object if it matches the requested type.
     */
    template <typename T>
    T* getModel() const
    {
        EDA_ITEM* m = getModelInternal();
#if !defined( QA_TEST )   // Dynamic casts give the linker a seizure in the test framework
        wxASSERT( dynamic_cast<T*>( m ) );
#endif
        return static_cast<T*>( m );
    }

private:
    // hide the implementation to avoid spreading half of kicad and wxWidgets headers to the tools
    // that may not need them at all!
    EDA_ITEM* getModelInternal() const;
    TOOLS_HOLDER* getToolHolderInternal() const;

protected:
    TOOL_TYPE     m_type;
    TOOL_ID       m_toolId;       ///< Unique id, assigned by a TOOL_MANAGER instance.

    /// Names are expected to obey the format application.ToolName (eg.
    /// pcbnew.InteractiveSelection).
    std::string   m_toolName;
    TOOL_MANAGER* m_toolMgr;
};

#endif  // TOOL_BASE_H
