/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <view/view_group.h>
#include <pcb_view.h>
#include <pcb_draw_panel_gal.h>

#include <functional>
#include <tool/tool_menu.h>

/**
 * Class PCB_TOOL_BASE
 *
 * A tool operating on a BOARD object
**/

class PCB_TOOL_BASE;
class PCB_EDIT_FRAME;
class PCB_DISPLAY_OPTIONS;
class PCBNEW_SELECTION;

struct INTERACTIVE_PLACER_BASE
{
    virtual std::unique_ptr<BOARD_ITEM> CreateItem() = 0;
    virtual void SnapItem( BOARD_ITEM *aItem );
    virtual bool PlaceItem( BOARD_ITEM *aItem, BOARD_COMMIT& aCommit );

    PCB_BASE_EDIT_FRAME* m_frame;
    BOARD* m_board;
    int m_modifiers;
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
        m_editModules( false ) 
    {};

    /**
     * Constructor
     *
     * Creates a tool with given name. The name must be unique. */
    PCB_TOOL_BASE( const std::string& aName ) :
        TOOL_INTERACTIVE ( aName ),
        m_editModules( false ) 
    {};

    virtual ~PCB_TOOL_BASE() {};

    virtual bool Init() override;
    virtual void Reset( RESET_REASON aReason ) override;

    /**
     * Function SetEditModules()
     *
     * Toggles edit module mode. When enabled, one may select parts of modules individually
     * (graphics, pads, etc.), so they can be modified.
     * @param aEnabled decides if the mode should be enabled.
     */
    void SetEditModules( bool aEnabled )
    {
        m_editModules = aEnabled;
    }

    bool EditingModules() const
    {
        return m_editModules;
    }

protected:

    enum INTERACTIVE_PLACEMENT_OPTIONS {
        IPO_ROTATE = 1,
        IPO_FLIP = 2,
        IPO_SINGLE_CLICK = 4,
        IPO_REPEAT = 8
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
    void doInteractiveItemPlacement( const std::string& aTool, INTERACTIVE_PLACER_BASE *aPlacer,
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
    
    PCB_BASE_EDIT_FRAME* frame() const 
    { 
        return getEditFrame<PCB_BASE_EDIT_FRAME>(); 
    }
    
    BOARD* board() const { return getModel<BOARD>(); }
    
    MODULE* module() const
    {
        return board()->GetFirstModule();
    }
    
    PCB_DISPLAY_OPTIONS* displayOptions() const;
    PCB_DRAW_PANEL_GAL* canvas() const;
    const PCBNEW_SELECTION& selection() const;
    PCBNEW_SELECTION& selection();

    bool m_editModules;
};

#endif
