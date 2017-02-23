# Tool Framework #

This document briefly outlines the structure of the tool system in the
GAL canvases.

[TOC]

# Introduction # {#intro}

The GAL (Graphics Abstraction Layer) framework provides a powerful
method of easily adding tools to KiCad. Compared to the older "legacy"
canvas, GAL tools are more flexible, powerful and much easier to write.

A GAL "tool" is a class which provides one or more "actions"
to perform. An action can be a simple one-off action (e.g. "zoom in"
or "flip object"), or an interactive process (e.g. "manually edit
polygon points").

Some examples of tools in the Pcbnew GAL are:

* The selection tool - the "normal" tool. This tool enters a state where
  items can be added to a list of selected objects, which are then made
  available for other tools to act on.
  (pcbnew/tools/selection_tool.cpp, pcbnew/tools/selection_tool.h)
* The edit tool - this tool is active when a component is "picked up",
  and tracks the mouse position to allow the user to move a component.
  Aspects of editing (e.g. flip) are also make available for use by
  hotkeys or other tools.
  (pcbnew/tools/edit_tool.cpp,pcbnew/tools/edit_tool.h)
* The drawing tool - this tool controls the process of drawing graphics
  elements such as line segments and circles.
  (pcbnew/tools/drawing_tool.cpp,pcbnew/tools/drawing_tool.h)
* The zoom tool - allows the user to zoom in and out

# Major parts of a tool # {#major-parts}

There are two main aspects to tools: the actions and the the tool class.

## Tool actions {#tool-actions}

The `TOOL_ACTION` class acts as a handle for the GAL framework to
call on actions provided by tools. Generally, every action, interactive
or not, has a `TOOL_ACTION` instance. This provides:

* A "name", which is of the format `pcbnew.ToolName.actionName`, which
  is used internally to dispatch the action
* A "scope", which determines when the tools is available:
    * `AS_CONTEXT`, when the action is specific to a particular tool. For
      example, `pcbnew.InteractiveDrawing.incWidth` increases the width
      of a line while the line is still being drawn.
    * `AS_GLOBAL`, when the tool can always be invoked, by a hotkey, or
      during the execution of a different tool. For example, the zoom
      actions can be accessed from the selection tool's menu during the
      interactive selection process.
* A "default hotkey", which is used if the user doesn't provide their
  own configuration.
* A "menu item", which is the (translatable) string shown when this tool
  is accessed from a menu.
* A "menu description", which is the string shown in the menu item's
  tooltip and provides a more detailed description if needed.
* An "icon", which is shown in menus and on buttons for the action
* "Flags" which include:
    * `AF_ACTIVATE` which indicates that the tool enters an active state
* A parameter, which allows different actions to call the same function
  with different effects, for example "step left" and "step right".

## The tool class {#tool-class}

GAL tools inherit the `TOOL_BASE` class. A Pcbnew tool will generally
inherit from `PCB_TOOL`, which is a `TOOL_INTERACTIVE`, which is
a `TOOL_BASE`. In the future, Eeschema tools will be developed in a similar
manner.

The tool class for a tool can be fairly lightweight - much of the
functionality is inherited from the tool's base classes. These base
classes provide access to several things, particularly:

* Access to the parent frame (a `wxWindow`, which can be used to
  modify the viewport, set cursors and status bar content, etc.
    * Use the function `getEditFrame<T>()`, where `T` is the frame
      subclass you want. In `PCB_TOOL`, this is likely `PCB_EDIT_FRAME`.
* Access to the `TOOL_MANAGER` which can be used to access other tools'
  actions.
* Access to the "model" (some sort of `EDA_ITEM`) which backs the tool.
    * Access with  `getModel<T>()`. In `PCB_TOOL`, the model type `T` is
     `BOARD`, which can be used to access and modify the PCB content.
* Access to the `KIGFX::VIEW` and `KIGFX::VIEW_CONTROLS`, which are
  used to manipulate the GAL canvas.

The major parts of tool's implementation are the functions used by the
`TOOL_MANAGER` to set up and manage the tool:

* Constructor and destructor to establish whatever class members are
  required.
* The TOOL_BASE class requires a string to be passed for the
  tool name, which normally looks like `pcbnew.ToolName`.
* `Init()` function (optional), which is commonly used to fill in
  a context menu, either belonging to this tool, or access another
  tool's menu and add items to that. This function is called once, when
  the tool is registered with the tool manager.
* `Reset()` function, called when the model (e.g. the `BOARD`) is reloaded,
  when the GAL canvas is switched, and also just after tool registration.
  Any resource claimed from the GAL view or the model must be released
  in this function, as they could become invalid.
* `SetTransitions()` function, which maps tool actions to functions
  within the tool class.
* One or more functions to call when actions are invoked. Many actions
  can invoke the same function if desired. The functions have the
  following signature:
    * int TOOL_CLASS::FunctionName( const TOOL_EVENT& aEvent )
        * Returning 0 means success.
    * These functions are called by the `TOOL_MANAGER` in case an associated
      event arrives (association is created with TOOL_INTERACTIVE::Go() function).
    * These can generally be private, as they are not called directly
      by any other code, but are invoked by the tool manager's coroutine
      framework according to the `SetTransitions()` map.

### Interactive actions {#interactive-actions}

The action handlers for an interactive actions handle repeated actions
from the tool manager in a loop, until an action indicating that the
tool should exit.

Interactive tools also normally indicate that they are active with
a cursor change and by setting a status string.

    int TOOL_NAME::someAction( const TOOL_EVENT& aEvent )
    {
        auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

        // set tool hint and cursor (actually looks like a crosshair)
        frame.SetToolID( ID_PCB_SHOW_1_RATSNEST_BUTT,
                wxCURSOR_PENCIL, _( "Select item to move left" ) );
        getViewControls()->ShowCursor( true );

        // activate the tool, now it will be the first one to receive events
        // you can skip this, if you are writing a handler for a single action
        // (e.g. zoom in), opposed to interactive tool that requires further
        // events to operate (e.g. dragging a component)
        Activate();

        // the main event loop
        while( OPT_TOOL_EVENT evt = Wait() )
        {
            if( evt->IsCancel() || evt->IsActivate() )
            {
                // end of interactive tool
                break;
            }
            else if( evt->IsClick( BUT_LEFT ) )
            {
                // do something here
            }
            // other events...
        }

        // reset the PCB frame to how it was when we got it
        frame.SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );
        getViewControls()->ShowCursor( false );

        return 0;
    }

## The tool menu {#tool-menu}

Top level tools, i.e. tools that the user enters directly, usually
provide their own context menu. Tools that are called only from other
tools' interactive modes add their menu items to those tools' menus.

To use a `TOOL_MENU` in a top level tool, simply add one as a member
and initialise it with a reference to the tools at construction time:

    TOOL_NAME: public PCB_TOOL
    {
    public:
        TOOL_NAME() :
            PCB_TOOL( "pcbnew.MyNewTool" ),
            m_menu( *this )
        {}

    private:
        TOOL_MENU m_menu;
    }

You can then add a menu accessor, or provide a custom function to
allow other tools to add any other actions, or a subset that you
think appropriate.

You can then invoke the menu from an interactive tool loop by
calling `m_menu.ShowContextMenu()`. Clicking on the tool's entry in
this menu will trigger the action - there is no further action
needed in your tool's event loop.


## Commit objects {#commits}

The `COMMIT` class manages changes to `EDA_ITEMS`, which combines
changes on any number of items into a single undo/redo action.
When editing PCBs, changes to the PCB are managed by the derived
`BOARD_COMMIT` class.

This class takes either a `PCB_BASE_FRAME` or a `PCB_TOOL` as an
argument. Using `PCB_TOOL` is more appropriate for a GAL tool, since
there's no need to go though a frame class if not required.

The procedure of a commit is:

* Construct an appropriate `COMMIT` object
* Before modifying any item, add it to the commit with `Modify( item )`
  so that the current item state can be stored as an undo point.
* When adding a new item, call `Add( item )`. Do not delete the added item,
  unless you are going to abort the commit.
* When removing an item, call `Remove( item )`. You should not delete the
  removed item, it will be stored in the undo buffer.
* Finalise the commit with `Push( "Description" )`. If you performed
  no modifications, additions or removals, this is a no-op, so you
  don't need to check if you made any changes before pushing.

If you want to abort a commit, you can just destruct it, without
calling `Push()`. The underlying model won't be updated.

As an example:

    // Construct commit from current PCB_TOOL
    BOARD_COMMIT commit( this );

    BOARD_ITEM* modifiedItem = getSomeItemToModify();

    // tell the commit we're going to change the item
    commit.Modify( modifiedItem );

    // update the item
    modifiedItem->Move( x, y );

    // create a new item
    DRAWSEGMENT* newItem = new DRAWSEGMENT;

    // ... set up item here

    // add to commit
    commit.Add( newItem );

    // update the model and add the undo point
    commit.Push( "Modified one item, added another" );


# Tutorial: Adding a new tool {#tutorial}

Without getting too heavily into the details of how the GAL tool framework
is implemented under the surface, let's look at how you could add a
brand new tool to Pcbnew. Our tool will have the following (rather
useless) functions:

* An interactive tool which will allow the user to select a point,
  choose from the items at that point and then move that item 10mm to
  the left.
* While in this mode, the context menu will have more options:
    * Use of the "normal" canvas zoom and grid options
    * A non-interactive tool which will add a fixed circle at a fixed point.
    * A way to invoke the non-interactive "unfill all zones" tool from
      the PCB_EDITOR_CONTROL tool.

## Declare tool actions {#declare-actions}

The first step is to add tool actions. We will implement two actions
named:

* `Pcbnew.UselessTool.MoveItemLeft` - the interactive tool
* `Pcbnew.UselessTool.FixedCircle` - the non-interactive tool.

The "unfill tool" already exists with the name
`pcbnew.EditorControl.zoneUnfillAll`.

This guide assumes we will be adding a tool to Pcbnew, but the
procedure for other GAL-capable canvases will be similar.

In `pcbnew/tools/pcb_actions.h`, we add the following to the
`PCB_ACTIONS` class, which declares our tools:

    static TOOL_ACTION uselessMoveItemLeft;
    static TOOL_ACTION uselessFixedCircle;

Definitions of actions generally happen in the .cpp of the relevant tool.
It doesn't actually matter where the defintion occurs (the declaration
is enough to use the action), as long as it's linked in the end.
Similar tools should always be defined together.

In our case, since we're making a new tool, this will be in
`pcbnew/tools/useless_tool.cpp`. If adding actions to existing tools,
the prefix of the tool string (e.g. `"Pcbnew.UselessTool"`) will
be a strong indicator as to where to define the tool.

The tools definitions look like this:

    TOOL_ACTION COMMON_ACTIONS::uselessMoveItemLeft(
            "pcbnew.UselessTool.MoveItemLeft",
            AS_GLOBAL, MD_CTRL + MD_SHIFT + int( 'L' ),
            _( "Move item left" ), _( "Select and move item left" ) );

    TOOL_ACTION COMMON_ACTIONS::uselessFixedCircle(
            "pcbnew.UselessTool.FixedCircle",
            AS_GLOBAL, MD_CTRL + MD_SHIFT + int( 'C' ),
            _( "Fixed circle" ), _( "Add a fixed size circle in a fixed place" ),
            add_circle_xpm );

We have defined hotkeys for each action, and they are both global. This
means you can use `Shift+Ctrl+L` and `Shift-Ctrl-C` to access each tool
respectively.

We defined an icon for one of the tools, which should appear in any
menu the item is added to, along with the given label and explanatory
tooltip.

We now have two actions defined, but they are not connected to anything.
We need to define a functions which implement the right actions.
You can add these to an existing tool (for example `PCB_EDITOR_CONTROL`,
which deals with many general PCB modification operation like zone
filling), or you can write a whole new tool to keep things separate
and give you more scope for adding tool state.

We will write our own tool to demonstrate the process.

## Add tool class declaration {#declare-tool-class}

Add a new tool class header `pcbnew/tools/useless_tool.h` containing
the following class:

    class USELESS_TOOL : public PCB_TOOL
    {
    public:
        USELESS_TOOL();
        ~USELESS_TOOL();

        ///> React to model/view changes
        void Reset( RESET_REASON aReason ) override;

        ///> Basic initalization
        bool Init() override;

        ///> Bind handlers to corresponding TOOL_ACTIONs
        void SetTransitions() override;

    private:
        ///> 'Move selected left' interactive tool
        int moveLeft( const TOOL_EVENT& aEvent );

        ///> Internal function to perform the move left action
        void moveLeftInt();

        ///> Add a fixed size circle
        int fixedCircle( const TOOL_EVENT& aEvent );

        ///> Menu model displayed by the tool.
        TOOL_MENU m_menu;
    };

## Implement tool class methods {#implement-tool}

In the `pcbnew/tools/useless_tool.cpp`, implement the required methods.
In this file, you might also add free function helpers, other classes,
and so on.

You will need to add this file to the `pcbnew/CMakeLists.txt` to
build it.

Below you will find the contents of useless_tool.cpp:

    #include "useless_tool.h"

    #include <class_draw_panel_gal.h>
    #include <view/view_controls.h>
    #include <view/view.h>
    #include <tool/tool_manager.h>
    #include <board_commit.h>

    // For frame ToolID values
    #include <pcbnew_id.h>

    // For action icons
    #include <bitmaps.h>

    // Items tool can act on
    #include <class_board_item.h>
    #include <class_drawsegment.h>

    // Access to other PCB actions and tools
    #include "pcb_actions.h"
    #include "selection_tool.h"


    /*
     * Tool-specific action defintions
     */
    TOOL_ACTION PCB_ACTIONS::uselessMoveItemLeft(
            "pcbnew.UselessTool.MoveItemLeft",
            AS_GLOBAL, MD_CTRL + MD_SHIFT + int( 'L' ),
            _( "Move item left" ), _( "Select and move item left" ) );

    TOOL_ACTION PCB_ACTIONS::uselessFixedCircle(
            "pcbnew.UselessTool.FixedCircle",
            AS_GLOBAL, MD_CTRL + MD_SHIFT + int( 'C' ),
            _( "Fixed circle" ), _( "Add a fixed size circle in a fixed place" ),
            add_circle_xpm );

    /*
     * USELESS_TOOL implementation
     */

    USELESS_TOOL::USELESS_TOOL() :
            PCB_TOOL( "pcbnew.UselessTool" ),
            m_menu( *this )
    {
    }


    USELESS_TOOL::~USELESS_TOOL()
    {}


    void USELESS_TOOL::Reset( RESET_REASON aReason )
    {
    }


    bool USELESS_TOOL::Init()
    {
        auto& menu = m_menu.GetMenu();

        // add our own tool's action
        menu.AddItem( PCB_ACTIONS::uselessFixedCircle );
        // add the PCB_EDITOR_CONTROL's zone unfill all action
        menu.AddItem( PCB_ACTIONS::zoneUnfillAll );

        // Add standard zoom and grid tool actions
        m_menu.AddStandardSubMenus( *getEditFrame<PCB_BASE_FRAME>() );

        return true;
    }

    void USELESS_TOOL::moveLeftInt()
    {
        // we will call actions on the selection tool to get the current
        // selection. The selection tools will handle item deisambiguation
        SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
        assert( selectionTool );

        // call the actions
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );
        selectionTool->SanitizeSelection();

        const SELECTION& selection = selectionTool->GetSelection();

        // nothing selected, return to event loop
        if( selection.Empty() )
            return;

        BOARD_COMMIT commit( this );

        // iterate BOARD_ITEM* container, moving each item
        for( auto item : selection )
        {
            commit.Modify( item );
            item->Move( wxPoint( -5 * IU_PER_MM, 0 ) );
        }

        // push commit - if selection were empty, this is a no-op
        commit.Push( "Move left" );
    }

    int USELESS_TOOL::moveLeft( const TOOL_EVENT& aEvent )
    {
        auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

        // set tool hint and cursor (actually looks like a crosshair)
        frame.SetToolID( ID_NO_TOOL_SELECTED,
                wxCURSOR_PENCIL, _( "Select item to move left" ) );

        getViewControls()->ShowCursor( true );

        Activate();

        // handle tool events for as long as the tool is active
        while( OPT_TOOL_EVENT evt = Wait() )
        {
            if( evt->IsCancel() || evt->IsActivate() )
            {
                // end of interactive tool
                break;
            }
            else if( evt->IsClick( BUT_RIGHT ) )
            {
                m_menu.ShowContextMenu();
            }
            else if( evt->IsClick( BUT_LEFT ) )
            {
                // invoke the main action logic
                moveLeftInt();

                // keep showing the edit cursor
                getViewControls()->ShowCursor( true );
            }
        }

        // reset the PCB frame to how it was we got it
        frame.SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );
        getViewControls()->ShowCursor( false );

        // exit action
        return 0;
    }


    int USELESS_TOOL::fixedCircle( const TOOL_EVENT& aEvent )
    {
        // new circle to add (ideally use a smart pointer)
        DRAWSEGMENT* circle = new DRAWSEGMENT;

        // Set the circle attributes
        circle->SetShape( S_CIRCLE );
        circle->SetWidth( 5 * IU_PER_MM );
        circle->SetStart( wxPoint( 50 * IU_PER_MM, 50 * IU_PER_MM ) );
        circle->SetEnd( wxPoint( 80 * IU_PER_MM, 80 * IU_PER_MM ) );
        circle->SetLayer(  LAYER_ID::F_SilkS );

        // commit the circle to the BOARD
        BOARD_COMMIT commit( this );
        commit.Add( circle );
        commit.Push( _( "Draw a circle" ) );

        return 0;
    }


    void USELESS_TOOL::SetTransitions()
    {
        Go( &USELESS_TOOL::fixedCircle, PCB_ACTIONS::uselessFixedCircle.MakeEvent() );
        Go( &USELESS_TOOL::moveLeft,    PCB_ACTIONS::uselessMoveItemLeft.MakeEvent() );
    }


## Register the tool {#register-tool}

The last step is to register the tool in the tool manager.

This is done by adding a new instance of the tool to the
`registerAllTools()` function in `pcbnew/tools/tools_common.cpp`.

    // add your tool header
    #include <tools/useless_tool.h>

    void registerAllTools( TOOL_MANAGER *aToolManager )
    {
        ....
        aToolManager->RegisterTool( new USELESS_TOOL );
        ....
    }

If your new tool applies in the module editor, you also need to do this
in `FOOTPRINT_EDIT_FRAME::setupTools()`. Generally, each kind of
`EDA_DRAW_FRAME` that can use GAL will have a place to do this.

## Build and run {#tutorial-summary}

When this is all done, you should have modified the following files:

* `pcbnew/tools/common_actions.h` - action declarations
* `pcbnew/tools/useless_tool.h` - tool header
* `pcbnew/tools/useless_tool.cpp` - action definitions and tool implementation
* `pcbnew/tools/tools_common.cpp` - registration of the tool
* `pcbnew/CMakeLists.txt` - for building the new .cpp files

When you run Pcbnew, you should be able to press `Shift+Ctrl+L` to
enter the "move item left" tool - the cursor will change to a crosshair
and "Select item to move left" appears in the bottom right corner.

When you right-click, you get a menu, which contains an entry for
our "create fixed circle" tool and one for the existing "unfill all
zones" tool which we added to the menu. You can also use `Shift+Ctrl+C`
to access the fixed circle action.

Congratulations, you have just created your first KiCad tool!
