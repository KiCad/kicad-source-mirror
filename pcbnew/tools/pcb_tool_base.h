/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_TOOL_BASE_H
#define PCB_TOOL_BASE_H

#include <string>

#include <tool/tool_interactive.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <view/view_group.h>
#include <pcb_view.h>
#include <pcb_draw_panel_gal.h>
#include <pcbnew_settings.h>
#include <preview_items/two_point_geom_manager.h>
#include <functional>
#include <tool/tool_menu.h>

/**
 * PCB_TOOL_BASE
 *
 * A tool operating on a BOARD object
**/

class PCB_TOOL_BASE;
class PCB_EDIT_FRAME;
class PCB_DISPLAY_OPTIONS;
class PCB_SELECTION;

struct INTERACTIVE_PLACER_BASE
{
    virtual ~INTERACTIVE_PLACER_BASE()
    {
    }

    virtual std::unique_ptr<BOARD_ITEM> CreateItem() = 0;

    virtual void SnapItem( BOARD_ITEM *aItem );

    virtual bool PlaceItem( BOARD_ITEM *aItem, BOARD_COMMIT& aCommit );

    PCB_BASE_EDIT_FRAME* m_frame;
    BOARD*               m_board;
    int                  m_modifiers;
};


class PCB_TOOL_BASE : public TOOL_INTERACTIVE
{
public:
    /**
     * Constructor
     *
     * Creates a tool with given id & name. The name must be unique. */
    PCB_TOOL_BASE( TOOL_ID aId, const std::string& aName ) :
            TOOL_INTERACTIVE ( aId, aName ),
            m_isFootprintEditor( false ),
            m_isBoardEditor( false )
    {};

    /**
     * Constructor
     *
     * Creates a tool with given name. The name must be unique. */
    PCB_TOOL_BASE( const std::string& aName ) :
            TOOL_INTERACTIVE ( aName ),
            m_isFootprintEditor( false ),
            m_isBoardEditor( false )
    {};

    virtual ~PCB_TOOL_BASE() {};

    virtual bool Init() override;
    virtual void Reset( RESET_REASON aReason ) override;

    /**
     * Function SetIsFootprintEditor()
     *
     * Toggles edit footprint mode. When enabled, one may select parts of footprints individually
     * (graphics, pads, etc.), so they can be modified.
     * @param aEnabled decides if the mode should be enabled.
     */
    void SetIsFootprintEditor( bool aEnabled ) { m_isFootprintEditor = aEnabled; }
    bool IsFootprintEditor() const { return m_isFootprintEditor; }

    void SetIsBoardEditor( bool aEnabled ) { m_isBoardEditor = aEnabled; }
    bool IsBoardEditor() const { return m_isBoardEditor; }

    /**
     * Should the tool use its 45° mode option?
     * @return True if set to use 45°
     */
    virtual bool Is45Limited() const;

    /**
     * Should the tool limit drawing to horizontal and vertical only?
     */
    virtual bool Is90Limited() const;

    /**
     * Get the current angle snapping mode.
     */
    LEADER_MODE GetAngleSnapMode() const;

protected:
    /**
     * Options for placing items interactively.
     */
    enum INTERACTIVE_PLACEMENT_OPTIONS {
        /// Handle the rotate action in the loop by calling the item's rotate method
        IPO_ROTATE       = 0x01,

        /// Handle flip action in the loop by calling the item's flip method
        IPO_FLIP         = 0x02,

        /// Create an item immediately on placement starting, otherwise show the pencil cursor
        /// until the item is created
        IPO_SINGLE_CLICK = 0x04,

        /// Allow repeat placement of the item
        IPO_REPEAT       = 0x08
    };

    /**
     * Helper function for performing a common interactive idiom:
     * wait for a left click, place an item there (perhaps with a
     * dialog or other user interaction), then have it move with
     * the mouse and respond to rotate/flip, etc
     *
     * More complex interactive processes are not supported here, you
     * should implement a customised event loop for those.
     *
     * @param aItemCreator the callable that will attempt to create the item
     * @param aCommitMessage the message used on a successful commit
     */
    void doInteractiveItemPlacement( const TOOL_EVENT& aTool, INTERACTIVE_PLACER_BASE* aPlacer,
                                     const wxString& aCommitMessage,
                                     int aOptions = IPO_ROTATE | IPO_FLIP | IPO_REPEAT );

    virtual void setTransitions() override;


    KIGFX::PCB_VIEW* view() const
    {
        return static_cast<KIGFX::PCB_VIEW*>( getView() );
    }

    KIGFX::VIEW_CONTROLS* controls() const
    {
        return getViewControls();
    }

    template<class T = PCB_BASE_EDIT_FRAME>
    T* frame() const
    {
        return getEditFrame<T>();
    }

    BOARD* board() const { return getModel<BOARD>(); }

    FOOTPRINT* footprint() const
    {
        return board()->GetFirstFootprint();
    }

    PCBNEW_SETTINGS::DISPLAY_OPTIONS& displayOptions() const;

    PCB_DRAW_PANEL_GAL* canvas() const;

    const PCB_SELECTION& selection() const;

    PCB_SELECTION& selection();

protected:
    bool m_isFootprintEditor;
    bool m_isBoardEditor;
};

#endif
