/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <tools/sch_edit_tool.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_line_drawing_tool.h>
#include <tools/sch_picker_tool.h>
#include <sch_actions.h>
#include <hotkeys.h>
#include <bitmaps.h>
#include <confirm.h>
#include <eda_doc.h>
#include <base_struct.h>
#include <sch_item_struct.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_text.h>
#include <sch_bitmap.h>
#include <sch_view.h>
#include <sch_line.h>
#include <sch_item_struct.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>
#include <list_operations.h>
#include <eeschema_id.h>
#include <status_popup.h>
#include <wx/gdicmn.h>
#include "sch_drawing_tool.h"

TOOL_ACTION SCH_ACTIONS::move( "eeschema.InteractiveEdit.move",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE ),
        _( "Move" ), _( "Moves the selected item(s)" ),
        move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drag( "eeschema.InteractiveEdit.drag",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DRAG ),
        _( "Drag" ), _( "Drags the selected item(s)" ),
        move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::duplicate( "eeschema.InteractiveEdit.duplicate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ),
        duplicate_xpm );

TOOL_ACTION SCH_ACTIONS::repeatDrawItem( "eeschema.InteractiveEdit.repeatDrawItem",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_REPEAT_LAST ),
        _( "Repeat Last Item" ), _( "Duplicates the last drawn item" ),
        nullptr );

TOOL_ACTION SCH_ACTIONS::rotateCW( "eeschema.InteractiveEdit.rotateCW",
        AS_GLOBAL, 0,
        _( "Rotate Clockwise" ), _( "Rotates selected item(s) clockwise" ),
        rotate_cw_xpm );

TOOL_ACTION SCH_ACTIONS::rotateCCW( "eeschema.InteractiveEdit.rotateCCW",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ROTATE ),
        _( "Rotate" ), _( "Rotates selected item(s) counter-clockwise" ),
        rotate_ccw_xpm );

TOOL_ACTION SCH_ACTIONS::mirrorX( "eeschema.InteractiveEdit.mirrorX",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MIRROR_X ),
        _( "Mirror Around Horizontal Axis" ), _( "Flips selected item(s) from top to bottom" ),
        mirror_h_xpm );

TOOL_ACTION SCH_ACTIONS::mirrorY( "eeschema.InteractiveEdit.mirrorY",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MIRROR_Y ),
        _( "Mirror Around Vertical Axis" ), _( "Flips selected item(s) from left to right" ),
        mirror_v_xpm );

TOOL_ACTION SCH_ACTIONS::properties( "eeschema.InteractiveEdit.properties",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT ),
        _( "Properties..." ), _( "Displays item properties dialog" ),
        edit_xpm );

TOOL_ACTION SCH_ACTIONS::editReference( "eeschema.InteractiveEdit.editReference",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COMPONENT_REFERENCE ),
        _( "Edit Reference..." ), _( "Displays reference field dialog" ),
        edit_comp_ref_xpm );

TOOL_ACTION SCH_ACTIONS::editValue( "eeschema.InteractiveEdit.editValue",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COMPONENT_VALUE ),
        _( "Edit Value..." ), _( "Displays value field dialog" ),
        edit_comp_value_xpm );

TOOL_ACTION SCH_ACTIONS::editFootprint( "eeschema.InteractiveEdit.editFootprint",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COMPONENT_FOOTPRINT ),
        _( "Edit Footprint..." ), _( "Displays footprint field dialog" ),
        edit_comp_footprint_xpm );

TOOL_ACTION SCH_ACTIONS::autoplaceFields( "eeschema.InteractiveEdit.autoplaceFields",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_AUTOPLACE_FIELDS ),
        _( "Autoplace Fields" ), _( "Runs the automatic placement algorithm on the symbol's fields" ),
        autoplace_fields_xpm );

TOOL_ACTION SCH_ACTIONS::convertDeMorgan( "eeschema.InteractiveEdit.convertDeMorgan",
        AS_GLOBAL, 0,
        _( "DeMorgan Conversion" ), _( "Switch between DeMorgan representations" ),
        morgan2_xpm );

TOOL_ACTION SCH_ACTIONS::toShapeSlash( "eeschema.InteractiveEdit.toShapeSlash",
        AS_GLOBAL, 0,
        _( "Set Bus Entry Shape /" ), _( "Change the bus entry shape to /" ),
        change_entry_orient_xpm );

TOOL_ACTION SCH_ACTIONS::toShapeBackslash( "eeschema.InteractiveEdit.toShapeBackslash",
        AS_GLOBAL, 0,
        _( "Set Bus Entry Shape \\" ), _( "Change the bus entry shape to \\" ),
        change_entry_orient_xpm );

TOOL_ACTION SCH_ACTIONS::toLabel( "eeschema.InteractiveEdit.toLabel",
        AS_GLOBAL, 0,
        _( "Change to Label" ), _( "Change existing item to a label" ),
        add_line_label_xpm );

TOOL_ACTION SCH_ACTIONS::toHLabel( "eeschema.InteractiveEdit.toHLabel",
        AS_GLOBAL, 0,
        _( "Change to Hierarchical Label" ), _( "Change existing item to a hierarchical label" ),
        add_hierarchical_label_xpm );

TOOL_ACTION SCH_ACTIONS::toGLabel( "eeschema.InteractiveEdit.toGLabel",
        AS_GLOBAL, 0,
        _( "Change to Global Label" ), _( "Change existing item to a global label" ),
        add_glabel_xpm );

TOOL_ACTION SCH_ACTIONS::toText( "eeschema.InteractiveEdit.toText",
        AS_GLOBAL, 0,
        _( "Change to Text" ), _( "Change existing item to a text comment" ),
        text_xpm );

TOOL_ACTION SCH_ACTIONS::cleanupSheetPins( "eeschema.InteractiveEdit.cleanupSheetPins",
        AS_GLOBAL, 0,
        _( "Cleanup Sheet Pins" ), _( "Delete unreferenced sheet pins" ),
        nullptr );

TOOL_ACTION SCH_ACTIONS::doDelete( "eeschema.InteractiveEdit.doDelete",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DELETE ),
        _( "Delete" ), _( "Deletes selected item(s)" ),
        delete_xpm );

TOOL_ACTION SCH_ACTIONS::deleteItemCursor( "eeschema.InteractiveEdit.deleteItemCursor",
        AS_GLOBAL, 0,
        _( "DoDelete Items" ), _( "DoDelete clicked items" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::breakWire( "eeschema.InteractiveEdit.breakWire",
        AS_GLOBAL, 0,
        _( "Break Wire" ), _( "Divide a wire into segments which can be dragged independently" ),
        break_line_xpm );

TOOL_ACTION SCH_ACTIONS::breakBus( "eeschema.InteractiveEdit.breakBus",
        AS_GLOBAL, 0,
        _( "Break Bus" ), _( "Divide a bus into segments which can be dragged independently" ),
        break_line_xpm );


// For adding to or removing from selections
#define QUIET_MODE true


class SYMBOL_UNIT_MENU : public CONTEXT_MENU
{
public:
    SYMBOL_UNIT_MENU()
    {
        SetIcon( component_select_unit_xpm );
        SetTitle( _( "Symbol Unit" ) );
    }


protected:
    CONTEXT_MENU* create() const override
    {
        return new SYMBOL_UNIT_MENU();
    }

private:
    void update() override
    {
        SCH_SELECTION_TOOL* selTool = getToolManager()->GetTool<SCH_SELECTION_TOOL>();
        SELECTION&          selection = selTool->GetSelection();
        SCH_COMPONENT*      component = dynamic_cast<SCH_COMPONENT*>( selection.Front() );

        if( !component )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "no symbol selected" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        int  unit = component->GetUnit();
        auto partRef = component->GetPartRef().lock();

        if( !partRef || partRef->GetUnitCount() < 2 )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "symbol is not multi-unit" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        for( int ii = 0; ii < partRef->GetUnitCount(); ii++ )
        {
            wxString num_unit;
            num_unit.Printf( _( "Unit %s" ), LIB_PART::SubReference( ii + 1, false ) );

            wxMenuItem * item = Append( ID_POPUP_SCH_SELECT_UNIT1 + ii, num_unit, wxEmptyString,
                                        wxITEM_CHECK );
            if( unit == ii + 1 )
                item->Check(true);

            // The ID max for these submenus is ID_POPUP_SCH_SELECT_UNIT_CMP_MAX
            // See eeschema_id to modify this value.
            if( ii >= (ID_POPUP_SCH_SELECT_UNIT_CMP_MAX - ID_POPUP_SCH_SELECT_UNIT1) )
                break;      // We have used all IDs for these submenus
        }
    }
};


SCH_EDIT_TOOL::SCH_EDIT_TOOL() :
        TOOL_INTERACTIVE( "eeschema.InteractiveEdit" ),
        m_selectionTool( nullptr ),
        m_view( nullptr ),
        m_controls( nullptr ),
        m_frame( nullptr ),
        m_menu( *this ),
        m_moveInProgress( false ),
        m_moveOffset( 0, 0 )
{
}


SCH_EDIT_TOOL::~SCH_EDIT_TOOL()
{
}


bool SCH_EDIT_TOOL::Init()
{
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_DRAWING_TOOL* drawingTool = m_toolMgr->GetTool<SCH_DRAWING_TOOL>();

    wxASSERT_MSG( m_selectionTool, "eeshema.InteractiveSelection tool is not available" );
    wxASSERT_MSG( drawingTool, "eeshema.InteractiveDrawing tool is not available" );

    auto activeTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto sheetTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_SHEET_SYMBOL_BUTT );
    };

    auto anyTextTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_LABEL_BUTT
              || m_frame->GetToolId() == ID_GLOBALLABEL_BUTT
              || m_frame->GetToolId() == ID_HIERLABEL_BUTT
              || m_frame->GetToolId() == ID_TEXT_COMMENT_BUTT );
    };

    auto moveCondition = [] ( const SELECTION& aSel ) {
        if( aSel.Empty() )
            return false;

        if( SCH_LINE_DRAWING_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        return true;
    };

    auto orientCondition = [] ( const SELECTION& aSel ) {
        if( aSel.Empty() )
            return false;

        if( SCH_LINE_DRAWING_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        SCH_ITEM* item = (SCH_ITEM*) aSel.Front();

        if( aSel.GetSize() > 1 )
            return true;

        switch( item->Type() )
        {
        case SCH_MARKER_T:
        case SCH_JUNCTION_T:
        case SCH_NO_CONNECT_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
        case SCH_LINE_T:
        case SCH_SHEET_PIN_T:
        case SCH_PIN_T:
            return false;
        default:
            return true;
        }
    };

    auto propertiesCondition = []  ( const SELECTION& aSel ) {
        if( aSel.GetSize() != 1 )
            return false;

        switch( static_cast<EDA_ITEM*>( aSel.Front() )->Type() )
        {
        case SCH_MARKER_T:
        case SCH_JUNCTION_T:
        case SCH_NO_CONNECT_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
        case SCH_LINE_T:
        case SCH_SHEET_PIN_T:
        case SCH_PIN_T:
            return false;
        default:
            return true;
        }
    };

    KICAD_T toLabelTypes[] = { SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, SCH_TEXT_T, EOT };
    auto toLabelCondition = SCH_CONDITIONS::Count( 1 )
                         && SCH_CONDITIONS::OnlyTypes( toLabelTypes );

    KICAD_T toHLableTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_TEXT_T, EOT };
    auto toHLabelCondition = SCH_CONDITIONS::Count( 1 )
                          && SCH_CONDITIONS::OnlyTypes( toHLableTypes);

    KICAD_T toGLableTypes[] = { SCH_LABEL_T, SCH_HIER_LABEL_T, SCH_TEXT_T, EOT };
    auto toGLabelCondition = SCH_CONDITIONS::Count( 1 )
                          && SCH_CONDITIONS::OnlyTypes( toGLableTypes);

    KICAD_T toTextTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, EOT };
    auto toTextlCondition = SCH_CONDITIONS::Count( 1 )
                         && SCH_CONDITIONS::OnlyTypes( toTextTypes);

    KICAD_T entryTypes[] = { SCH_BUS_WIRE_ENTRY_T, SCH_BUS_BUS_ENTRY_T, EOT };
    auto entryCondition = SCH_CONDITIONS::MoreThan( 0 )
                       && SCH_CONDITIONS::OnlyTypes( entryTypes );

    auto singleComponentCondition = SCH_CONDITIONS::Count( 1 )
                                 && SCH_CONDITIONS::OnlyType( SCH_COMPONENT_T );

    auto wireSelectionCondition = SCH_CONDITIONS::MoreThan( 0 )
                               && SCH_CONDITIONS::OnlyType( SCH_LINE_LOCATE_WIRE_T );

    auto busSelectionCondition = SCH_CONDITIONS::MoreThan( 0 )
                              && SCH_CONDITIONS::OnlyType( SCH_LINE_LOCATE_BUS_T );

    auto singleSheetCondition = SCH_CONDITIONS::Count( 1 )
                             && SCH_CONDITIONS::OnlyType( SCH_SHEET_T );

    //
    // Build the edit tool menu (shown when moving or dragging)
    //
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeTool, 1 );

    ctxMenu.AddSeparator( SELECTION_CONDITIONS::NotEmpty );
    ctxMenu.AddItem( SCH_ACTIONS::rotateCCW, orientCondition );
    ctxMenu.AddItem( SCH_ACTIONS::rotateCW,  orientCondition );
    ctxMenu.AddItem( SCH_ACTIONS::mirrorX,   orientCondition );
    ctxMenu.AddItem( SCH_ACTIONS::mirrorY,   orientCondition );
    ctxMenu.AddItem( SCH_ACTIONS::duplicate, moveCondition );
    ctxMenu.AddItem( SCH_ACTIONS::doDelete,  SCH_CONDITIONS::NotEmpty );

    ctxMenu.AddItem( SCH_ACTIONS::properties,      propertiesCondition );
    ctxMenu.AddItem( SCH_ACTIONS::editReference,   singleComponentCondition );
    ctxMenu.AddItem( SCH_ACTIONS::editValue,       singleComponentCondition );
    ctxMenu.AddItem( SCH_ACTIONS::editFootprint,   singleComponentCondition );
    ctxMenu.AddItem( SCH_ACTIONS::convertDeMorgan, SCH_CONDITIONS::SingleDeMorganSymbol );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu->SetTool( this );
    m_menu.AddSubMenu( symUnitMenu );
    ctxMenu.AddMenu( symUnitMenu.get(), false, SCH_CONDITIONS::SingleMultiUnitSymbol, 1 );

    ctxMenu.AddSeparator( SCH_CONDITIONS::IdleSelection );
    ctxMenu.AddItem( SCH_ACTIONS::cut,  SCH_CONDITIONS::IdleSelection );
    ctxMenu.AddItem( SCH_ACTIONS::copy, SCH_CONDITIONS::IdleSelection );

    ctxMenu.AddSeparator( SELECTION_CONDITIONS::NotEmpty, 1000 );
    m_menu.AddStandardSubMenus( m_frame );

    //
    // Add editing actions to the drawing tool menu
    //
    CONDITIONAL_MENU& drawingMenu = drawingTool->GetToolMenu().GetMenu();

    drawingMenu.AddSeparator( SCH_CONDITIONS::NotEmpty, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::rotateCCW, orientCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::rotateCW,  orientCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::mirrorX,   orientCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::mirrorY,   orientCondition, 200 );

    drawingMenu.AddItem( SCH_ACTIONS::properties,      propertiesCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::editReference,   singleComponentCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::editValue,       singleComponentCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::editFootprint,   singleComponentCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::convertDeMorgan, SCH_CONDITIONS::SingleDeMorganSymbol, 200 );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu2 = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu2->SetTool( drawingTool );
    drawingTool->GetToolMenu().AddSubMenu( symUnitMenu2 );
    drawingMenu.AddMenu( symUnitMenu2.get(), false, SCH_CONDITIONS::SingleMultiUnitSymbol, 1 );

    drawingMenu.AddItem( SCH_ACTIONS::toShapeSlash,     entryCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::toShapeBackslash, entryCondition, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::toLabel,          anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::toHLabel,         anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::toGLabel,         anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::toText,           anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::cleanupSheetPins, sheetTool && SCH_CONDITIONS::Idle, 200 );
    drawingMenu.AddItem( SCH_ACTIONS::resizeSheet,      sheetTool && SCH_CONDITIONS::Idle, 200 );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( SCH_ACTIONS::move,             moveCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::drag,             moveCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::rotateCCW,        orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::rotateCW,         orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::mirrorX,          orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::mirrorY,          orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::duplicate,        moveCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::doDelete,         SCH_CONDITIONS::NotEmpty, 200 );

    selToolMenu.AddItem( SCH_ACTIONS::properties,       propertiesCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::editReference,    SCH_CONDITIONS::SingleSymbol, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::editValue,        SCH_CONDITIONS::SingleSymbol, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::editFootprint,    SCH_CONDITIONS::SingleSymbol, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::autoplaceFields,  singleComponentCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::convertDeMorgan,  SCH_CONDITIONS::SingleSymbol, 200 );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu3 = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu3->SetTool( m_selectionTool );
    m_selectionTool->GetToolMenu().AddSubMenu( symUnitMenu3 );
    selToolMenu.AddMenu( symUnitMenu3.get(), false, SCH_CONDITIONS::SingleMultiUnitSymbol, 1 );

    selToolMenu.AddItem( SCH_ACTIONS::toShapeSlash,     entryCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::toShapeBackslash, entryCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::toLabel,          toLabelCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::toHLabel,         toHLabelCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::toGLabel,         toGLabelCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::toText,           toTextlCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::cleanupSheetPins, singleSheetCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::resizeSheet,      singleSheetCondition, 200 );

    selToolMenu.AddSeparator( SCH_CONDITIONS::Idle, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::cut,   SCH_CONDITIONS::IdleSelection, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::copy,  SCH_CONDITIONS::IdleSelection, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::paste, SCH_CONDITIONS::Idle, 200 );

    return true;
}


void SCH_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
    {
        m_moveInProgress = false;
        m_moveOffset = { 0, 0 };

        // Init variables used by every drawing tool
        m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
        m_controls = getViewControls();
        m_frame = getEditFrame<SCH_EDIT_FRAME>();
    }
}


int SCH_EDIT_TOOL::Main( const TOOL_EVENT& aEvent )
{
    const KICAD_T movableItems[] =
    {
        SCH_MARKER_T,
        SCH_JUNCTION_T,
        SCH_NO_CONNECT_T,
        SCH_BUS_BUS_ENTRY_T,
        SCH_BUS_WIRE_ENTRY_T,
        SCH_LINE_T,
        SCH_BITMAP_T,
        SCH_TEXT_T,
        SCH_LABEL_T,
        SCH_GLOBAL_LABEL_T,
        SCH_HIER_LABEL_T,
        SCH_FIELD_T,
        SCH_COMPONENT_T,
        SCH_SHEET_PIN_T,
        SCH_SHEET_T,
        EOT
    };

    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->SetSnapping( true );
    VECTOR2I originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    SELECTION& selection = m_selectionTool->RequestSelection( movableItems );
    EDA_ITEMS  dragAdditions;
    bool       unselect = selection.IsHover();

    if( selection.Empty() )
        return 0;

    if( aEvent.IsAction( &SCH_ACTIONS::move ) )
        m_frame->SetToolID( ID_SCH_MOVE, wxCURSOR_DEFAULT, _( "Move Items" ) );
    else
        m_frame->SetToolID( ID_SCH_DRAG, wxCURSOR_DEFAULT, _( "Drag Items" ) );

    Activate();
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool restore_state = false;
    bool chain_commands = false;
    OPT_TOOL_EVENT evt = aEvent;
    VECTOR2I prevPos;

    if( m_moveInProgress )
    {
        // User must have switched from move to drag or vice-versa.  Reset the moved items
        // so we can start again with the current m_isDragOperation and m_moveOffset.
        m_frame->RollbackSchematicFromUndo();
        m_selectionTool->RemoveItemsFromSel( &dragAdditions, QUIET_MODE );
        m_moveInProgress = false;
        // And give it a kick so it doesn't have to wait for the first mouse movement to
        // refresh.
        m_toolMgr->RunAction( SCH_ACTIONS::refreshPreview );
        return 0;
    }

    // Main loop: keep receiving events
    do
    {
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

        if( evt->IsAction( &SCH_ACTIONS::move ) || evt->IsAction( &SCH_ACTIONS::drag )
                || evt->IsMotion() || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &SCH_ACTIONS::refreshPreview ) )
        {
            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                //------------------------------------------------------------------------
                // Setup a drag or a move
                //
                for( SCH_ITEM* it = m_frame->GetScreen()->GetDrawItems(); it; it = it->Next() )
                {
                    if( it->IsSelected() )
                        it->SetFlags( STARTPOINT | ENDPOINT | SELECTEDNODE );
                    else
                        it->ClearFlags( STARTPOINT | ENDPOINT | SELECTEDNODE );
                }

                // Add connections to the selection for a drag.
                //
                if( m_frame->GetToolId() == ID_SCH_DRAG )
                {
                    for( EDA_ITEM* item : selection )
                    {
                        if( static_cast<SCH_ITEM*>( item )->IsConnectable() )
                        {
                            std::vector<wxPoint> connections;
                            static_cast<SCH_ITEM*>( item )->GetConnectionPoints( connections );

                            for( wxPoint point : connections )
                                getConnectedDragItems( (SCH_ITEM*) item, point, dragAdditions );
                        }
                    }

                    m_selectionTool->AddItemsToSel( &dragAdditions, QUIET_MODE );

                    for( EDA_ITEM* item : dragAdditions )
                        saveCopyInUndoList( (SCH_ITEM*) item, UR_CHANGED, true );
                }

                // Mark the edges of the block with dangling flags for a move.
                //
                if( m_frame->GetToolId() == ID_SCH_MOVE )
                {
                    std::vector<DANGLING_END_ITEM> internalPoints;

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->GetEndPoints( internalPoints );

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->UpdateDanglingState( internalPoints );
                }

                // Generic setup
                //
                bool first = true;
                for( EDA_ITEM* item : selection )
                {
                    if( item->IsNew() || ( item->GetParent() && item->GetParent()->IsSelected() ) )
                    {
                        // already saved to undo
                    }
                    else
                    {
                        saveCopyInUndoList( (SCH_ITEM*) item, UR_CHANGED, !first );
                        first = false;
                    }

                    // Apply any initial offset in case we're coming from a previous command.
                    //
                    moveItem( (SCH_ITEM*) item, m_moveOffset, m_frame->GetToolId() == ID_SCH_DRAG );
                }

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    VECTOR2I delta = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( int i = 0; i < selection.GetSize(); ++i )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                        // Don't double move pins, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        moveItem( item, delta, m_frame->GetToolId() == ID_SCH_DRAG );
                        updateView( item );
                    }

                    selection.SetReferencePoint( m_cursor );
                }
                else if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin,
                    // so the movement vector can be computed later
                    updateModificationPoint( selection );
                    m_cursor = originalCursorPos;
                }
                else
                {
                    updateModificationPoint( selection );
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_cursor = controls->GetCursorPosition();
            VECTOR2I delta( m_cursor - prevPos );
            selection.SetReferencePoint( m_cursor );

            m_moveOffset += delta;
            prevPos = m_cursor;

            for( EDA_ITEM* item : selection )
            {
                // Don't double move pins, fields, etc.
                if( item->GetParent() && item->GetParent()->IsSelected() )
                    continue;

                moveItem( (SCH_ITEM*) item, delta, m_frame->GetToolId() == ID_SCH_DRAG );
                updateView( item );
            }

            m_frame->UpdateMsgPanel();
        }
        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

            if( m_moveInProgress )
                restore_state = true;

            break;
        }
        //------------------------------------------------------------------------
        // Handle TOOL_ACTION special cases
        //
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }
        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &SCH_ACTIONS::doDelete ) )
            {
                // Exit on a remove operation; there is no further processing for removed items.
                break;
            }
            else if( evt->IsAction( &SCH_ACTIONS::duplicate ) )
            {
                if( selection.Front()->IsNew() )
                {
                    // This doesn't really make sense; we'll just end up dragging a stack of
                    // objects so Duplicate() is going to ignore this and we'll just carry on.
                    continue;
                }

                // Move original back and exit.  The duplicate will run in its own loop.
                restore_state = true;
                unselect = false;
                chain_commands = true;
                break;
            }
            else if( evt->Action() == TA_CONTEXT_MENU_CHOICE )
            {
                if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                    && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_CMP_MAX )
                {
                    SCH_COMPONENT* component = dynamic_cast<SCH_COMPONENT*>( selection.Front() );
                    int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                    if( component )
                    {
                        m_frame->SelectUnit( component, unit );
                        m_toolMgr->RunAction( SCH_ACTIONS::refreshPreview );
                    }
                }
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection );
        }
        //------------------------------------------------------------------------
        // Handle drop
        //
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            break; // Finish
        }

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    if( !chain_commands )
        m_moveOffset = { 0, 0 };

    m_moveInProgress = false;
    m_frame->SetNoToolSelected();

    selection.ClearReferencePoint();

    for( auto item : selection )
        item->ClearFlags( IS_MOVED );

    if( unselect )
        m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
    else
        m_selectionTool->RemoveItemsFromSel( &dragAdditions, QUIET_MODE );

    if( restore_state )
    {
        m_frame->RollbackSchematicFromUndo();
    }
    else
    {
        m_frame->CheckConnections( selection, true );
        m_frame->SchematicCleanUp( true );
        m_frame->TestDanglingEnds();
        m_frame->OnModify();
    }

    return 0;
}


void SCH_EDIT_TOOL::getConnectedDragItems( SCH_ITEM* aItem, wxPoint aPoint, EDA_ITEMS& aList )
{
    for( SCH_ITEM* test = m_frame->GetScreen()->GetDrawItems(); test; test = test->Next() )
    {
        if( test->IsSelected() || !test->IsConnectable() || !test->CanConnect( aItem ) )
            continue;

        switch( test->Type() )
        {
        default:
        case SCH_LINE_T:
        {
            // Select wires/busses that are connected at one end and/or the other.  Any
            // unconnected ends must be flagged (STARTPOINT or ENDPOINT).
            SCH_LINE* line = (SCH_LINE*) test;

            if( line->GetStartPoint() == aPoint )
            {
                if( !( line->GetFlags() & SELECTEDNODE ) )
                    aList.push_back( line );

                line->SetFlags( STARTPOINT | SELECTEDNODE );
            }
            else if( line->GetEndPoint() == aPoint )
            {
                if( !( line->GetFlags() & SELECTEDNODE ) )
                    aList.push_back( line );

                line->SetFlags( ENDPOINT | SELECTEDNODE );
            }
            break;
        }

        case SCH_SHEET_T:
            // Dragging a sheet just because it's connected to something else feels a bit like
            // the tail wagging the dog, but this could be moved down to the next case.
            break;

        case SCH_COMPONENT_T:
        case SCH_NO_CONNECT_T:
        case SCH_JUNCTION_T:
            // Select connected items that have no wire between them.
            if( aItem->Type() != SCH_LINE_T && test->IsConnected( aPoint ) )
               aList.push_back( test );

            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            // Select labels and bus entries that are connected to a wire being moved.
            if( aItem->Type() == SCH_LINE_T )
            {
                std::vector<wxPoint> connections;
                test->GetConnectionPoints( connections );

                for( wxPoint& point : connections )
                {
                    if( aItem->HitTest( point ) )
                        aList.push_back( test );
                }
            }
            break;
        }
    }
}


void SCH_EDIT_TOOL::moveItem( SCH_ITEM* aItem, VECTOR2I aDelta, bool isDrag )
{
    switch( aItem->Type() )
    {
    case SCH_LINE_T:
        if( aItem->GetFlags() & STARTPOINT )
            static_cast<SCH_LINE*>( aItem )->MoveStart( (wxPoint) aDelta );

        if( aItem->GetFlags() & ENDPOINT )
            static_cast<SCH_LINE*>( aItem )->MoveEnd( (wxPoint) aDelta );

        break;

    case SCH_PIN_T:
    case SCH_FIELD_T:
        aItem->Move( wxPoint( aDelta.x, -aDelta.y ) );
        break;

    default:
        aItem->Move( (wxPoint) aDelta );
        break;
    }

    aItem->SetFlags( IS_MOVED );
}


bool SCH_EDIT_TOOL::updateModificationPoint( SELECTION& aSelection )
{
    if( m_moveInProgress && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        SCH_ITEM* item =  static_cast<SCH_ITEM*>( aSelection.Front() );

        // For some items, moving the cursor to anchor is not good (for instance large
        // hierarchical sheets or components can have the anchor outside the view)
        if( item->IsMovableFromAnchorPoint() )
        {
            wxPoint pos = item->GetPosition();
            aSelection.SetReferencePoint( pos );

            return true;
        }
    }

    // ...otherwise modify items with regard to the grid-snapped cursor position
    m_cursor = getViewControls()->GetCursorPosition( true );
    aSelection.SetReferencePoint( m_cursor );

    return true;
}


int SCH_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::RotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   rotPoint;
    bool      clockwise = ( aEvent.Matches( SCH_ACTIONS::rotateCW.MakeEvent() ) );
    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.Front() );
    bool      connections = false;
    bool      moving = item->IsMoving();

    if( selection.GetSize() == 1 )
    {
        if( !moving )
            saveCopyInUndoList( item, UR_CHANGED );

        switch( item->Type() )
        {
        case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

            if( clockwise )
                component->SetOrientation( CMP_ROTATE_CLOCKWISE );
            else
                component->SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );

            if( m_frame->GetAutoplaceFields() )
                component->AutoAutoplaceFields( m_frame->GetScreen() );

            break;
        }

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_TEXT* textItem = static_cast<SCH_TEXT*>( item );
            textItem->SetLabelSpinStyle( ( textItem->GetLabelSpinStyle() + 1 ) & 3 );
            break;
        }

        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T:
            item->Rotate( item->GetPosition() );
            break;

        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

            if( field->GetTextAngle() == TEXT_ANGLE_HORIZ )
                field->SetTextAngle( TEXT_ANGLE_VERT );
            else
                field->SetTextAngle( TEXT_ANGLE_HORIZ );

            // Now that we're moving a field, they're no longer autoplaced.
            if( item->GetParent()->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT *parent = static_cast<SCH_COMPONENT*>( item->GetParent() );
                parent->ClearFieldsAutoplaced();
            }

            break;
        }

        case SCH_BITMAP_T:
            item->Rotate( item->GetPosition() );

            // The bitmap is cached in Opengl: clear the cache to redraw
            getView()->RecacheAllItems();
            break;

        case SCH_SHEET_T:
            // Rotate the sheet on itself. Sheets do not have a anchor point.
            rotPoint = m_frame->GetNearestGridPosition( item->GetBoundingBox().Centre() );

            for( int i = 0; clockwise ? i < 1 : i < 3; ++i )
                item->Rotate( rotPoint );

            break;

        default:
            break;
        }

        connections = item->IsConnectable();
        m_frame->RefreshItem( item );
    }
    else if( selection.GetSize() > 1 )
    {
        rotPoint = m_frame->GetNearestGridPosition( (wxPoint)selection.GetCenter() );

        for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
        {
            item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

            if( !moving )
                saveCopyInUndoList( item, UR_CHANGED, ii > 0 );

            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE* line = (SCH_LINE*) item;

                if( item->GetFlags() & STARTPOINT )
                    line->RotateStart( rotPoint );

                if( item->GetFlags() & ENDPOINT )
                    line->RotateEnd( rotPoint );
            }
            else
            {
                item->Rotate( rotPoint );
            }

            connections |= item->IsConnectable();
            m_frame->RefreshItem( item );
        }
    }

    if( !item->IsMoving() )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

        if( connections )
            m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    return 0;
}


int SCH_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::RotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   mirrorPoint;
    bool      xAxis = ( aEvent.Matches( SCH_ACTIONS::mirrorX.MakeEvent() ) );
    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.Front() );
    bool      connections = false;
    bool      moving = item->IsMoving();

    if( selection.GetSize() == 1 )
    {
        if( !moving )
            saveCopyInUndoList( item, UR_CHANGED );

        switch( item->Type() )
        {
        case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

            if( xAxis )
                component->SetOrientation( CMP_MIRROR_X );
            else
                component->SetOrientation( CMP_MIRROR_Y );

            if( m_frame->GetAutoplaceFields() )
                component->AutoAutoplaceFields( m_frame->GetScreen() );

            break;
        }

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_TEXT* textItem = static_cast<SCH_TEXT*>( item );
            int       spin = textItem->GetLabelSpinStyle();

            if( xAxis && spin % 2 )
                textItem->SetLabelSpinStyle( ( spin + 2 ) % 4 );
            else if ( !xAxis && !( spin % 2 ) )
                textItem->SetLabelSpinStyle( ( spin + 2 ) % 4 );
            break;
        }

        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T:
            if( xAxis )
                item->MirrorX( item->GetPosition().y );
            else
                item->MirrorY( item->GetPosition().x );
            break;

        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

            if( xAxis )
                field->SetVertJustify( (EDA_TEXT_VJUSTIFY_T)-field->GetVertJustify() );
            else
                field->SetHorizJustify( (EDA_TEXT_HJUSTIFY_T)-field->GetHorizJustify() );

            // Now that we're re-justifying a field, they're no longer autoplaced.
            if( item->GetParent()->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT *parent = static_cast<SCH_COMPONENT*>( item->GetParent() );
                parent->ClearFieldsAutoplaced();
            }

            break;
        }

        case SCH_BITMAP_T:
            if( xAxis )
                item->MirrorX( item->GetPosition().y );
            else
                item->MirrorY( item->GetPosition().x );

            // The bitmap is cached in Opengl: clear the cache to redraw
            getView()->RecacheAllItems();
            break;

        case SCH_SHEET_T:
            // Mirror the sheet on itself. Sheets do not have a anchor point.
            mirrorPoint = m_frame->GetNearestGridPosition( item->GetBoundingBox().Centre() );

            if( xAxis )
                item->MirrorX( mirrorPoint.y );
            else
                item->MirrorY( mirrorPoint.x );

            break;

        default:
            break;
        }

        connections = item->IsConnectable();
        m_frame->RefreshItem( item );
    }
    else if( selection.GetSize() > 1 )
    {
        mirrorPoint = m_frame->GetNearestGridPosition( (wxPoint)selection.GetCenter() );

        for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
        {
            item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

            if( !moving )
                saveCopyInUndoList( item, UR_CHANGED, ii > 0 );

            if( xAxis )
                item->MirrorX( mirrorPoint.y );
            else
                item->MirrorY( mirrorPoint.x );

            connections |= item->IsConnectable();
            m_frame->RefreshItem( item );
        }
    }

    if( !item->IsMoving() )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

        if( connections )
            m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    return 0;
}


int SCH_EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    static KICAD_T duplicatableItems[] =
    {
        SCH_JUNCTION_T,
        SCH_LINE_T,
        SCH_BUS_BUS_ENTRY_T,
        SCH_BUS_WIRE_ENTRY_T,
        SCH_TEXT_T,
        SCH_LABEL_T,
        SCH_GLOBAL_LABEL_T,
        SCH_HIER_LABEL_T,
        SCH_NO_CONNECT_T,
        SCH_SHEET_T,
        SCH_COMPONENT_T,
        EOT
    };

    SELECTION& selection = m_selectionTool->RequestSelection( duplicatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    // This doesn't really make sense; we'll just end up dragging a stack of objects...
    if( selection.Front()->IsNew() )
        return 0;

    EDA_ITEMS newItems;

    // Keep track of existing sheet paths. Duplicating a selection can modify this list
    bool copiedSheets = false;
    SCH_SHEET_LIST initial_sheetpathList( g_RootSheet );

    for( unsigned ii = 0; ii < selection.GetSize(); ++ii )
    {
        SCH_ITEM* oldItem = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );
        SCH_ITEM* newItem = DuplicateItem( oldItem );
        newItem->SetFlags( IS_NEW );
        newItems.push_back( newItem );
        saveCopyInUndoList( newItem, UR_NEW, ii > 0 );

        switch( newItem->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_NO_CONNECT_T:
            newItem->SetParent( m_frame->GetScreen() );
            m_frame->AddToScreen( newItem );
            break;

        case SCH_SHEET_T:
        {
            SCH_SHEET* sheet = (SCH_SHEET*) newItem;
            // Duplicate sheet names and sheet time stamps are not valid.  Use a time stamp
            // based sheet name and update the time stamp for each sheet in the block.
            timestamp_t timeStamp = GetNewTimeStamp();

            sheet->SetName( wxString::Format( wxT( "sheet%8.8lX" ), (unsigned long)timeStamp ) );
            sheet->SetTimeStamp( timeStamp );

            sheet->SetParent( m_frame->GetScreen() );
            m_frame->AddToScreen( sheet );

            copiedSheets = true;
            break;
        }

        case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) newItem;

            component->SetTimeStamp( GetNewTimeStamp() );
            component->ClearAnnotation( NULL );

            component->SetParent( m_frame->GetScreen() );
            m_frame->AddToScreen( component );
            break;
        }

        default:
            break;
        }
    }

    if( copiedSheets )
    {
        // We clear annotation of new sheet paths.
        // Annotation of new components added in current sheet is already cleared.
        SCH_SCREENS screensList( g_RootSheet );
        screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
        m_frame->SetSheetNumberAndCount();
    }

    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
    m_toolMgr->RunAction( SCH_ACTIONS::addItemsToSel, true, &newItems );

    TOOL_EVENT evt = SCH_ACTIONS::move.MakeEvent();
    Main( evt );

    return 0;
}


int SCH_EDIT_TOOL::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    SCH_ITEM* sourceItem = m_frame->GetRepeatItem();

    if( !sourceItem )
        return 0;

    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

    SCH_ITEM* newItem = (SCH_ITEM*) sourceItem->Clone();
    bool      performDrag = false;

    // If cloning a component then put into 'move' mode.
    if( newItem->Type() == SCH_COMPONENT_T )
    {
        ( (SCH_COMPONENT*) newItem )->SetTimeStamp( GetNewTimeStamp() );

        newItem->Move( (wxPoint)m_controls->GetCursorPosition( true ) - newItem->GetPosition() );
        performDrag = true;
    }
    else
    {
        if( newItem->CanIncrementLabel() )
            ( (SCH_TEXT*) newItem )->IncrementLabel( m_frame->GetRepeatDeltaLabel() );

        newItem->Move( m_frame->GetRepeatStep() );
    }

    newItem->SetFlags( IS_NEW );
    m_frame->AddToScreen( newItem );
    m_frame->SaveCopyInUndoList( newItem, UR_NEW );

    m_selectionTool->AddItemToSel( newItem );

    if( performDrag )
    {
        TOOL_EVENT evt = SCH_ACTIONS::move.MakeEvent();
        Main( evt );
    }

    newItem->ClearFlags();

    if( newItem->IsConnectable() )
        m_frame->TestDanglingEnds();

    // newItem newItem, now that it has been moved, thus saving new position.
    m_frame->SetRepeatItem( newItem );

    return 0;
}


int SCH_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    std::vector<SCH_ITEM*> items;
    // get a copy instead of reference (we're going to clear the selection before removing items)
    SELECTION              selectionCopy = m_selectionTool->RequestSelection();

    if( selectionCopy.Empty() )
        return 0;

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

    for( unsigned ii = 0; ii < selectionCopy.GetSize(); ii++ )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selectionCopy.GetItem( ii ) );

        // Junctions, in particular, may have already been deleted if deleting wires made
        // them redundant
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        m_frame->DeleteItem( item, ii > 0 );
    }

    m_frame->SetRepeatItem( nullptr );
    m_frame->TestDanglingEnds();

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


static bool deleteItem( SCH_EDIT_FRAME* aFrame, const VECTOR2D& aPosition )
{
    SCH_SELECTION_TOOL* selectionTool = aFrame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    wxCHECK( selectionTool, false );

    aFrame->GetToolManager()->RunAction( SCH_ACTIONS::clearSelection, true );

    SCH_ITEM* item = selectionTool->SelectPoint( aPosition );

    if( item )
    {
        if( item->IsLocked() )
        {
            STATUS_TEXT_POPUP statusPopup( aFrame );
            statusPopup.SetText( _( "Item locked." ) );
            statusPopup.Expire( 2000 );
            statusPopup.Popup();
            statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
        }
        else
        {
            aFrame->GetToolManager()->RunAction( SCH_ACTIONS::doDelete, true );
        }
    }

    return true;
}


int SCH_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    Activate();

    SCH_PICKER_TOOL* picker = m_toolMgr->GetTool<SCH_PICKER_TOOL>();
    wxCHECK( picker, 0 );

    m_frame->SetToolID( ID_SCHEMATIC_DELETE_ITEM_BUTT, wxCURSOR_BULLSEYE, _( "DoDelete item" ) );
    picker->SetClickHandler( std::bind( deleteItem, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


int SCH_EDIT_TOOL::EditField( const TOOL_EVENT& aEvent )
{
    static KICAD_T Nothing[]        = { EOT };
    static KICAD_T CmpOrReference[] = { SCH_FIELD_LOCATE_REFERENCE_T, SCH_COMPONENT_T, EOT };
    static KICAD_T CmpOrValue[]     = { SCH_FIELD_LOCATE_VALUE_T,     SCH_COMPONENT_T, EOT };
    static KICAD_T CmpOrFootprint[] = { SCH_FIELD_LOCATE_FOOTPRINT_T, SCH_COMPONENT_T, EOT };

    KICAD_T* filter = Nothing;

    if( aEvent.IsAction( &SCH_ACTIONS::editReference ) )
        filter = CmpOrReference;
    else if( aEvent.IsAction( &SCH_ACTIONS::editValue ) )
        filter = CmpOrValue;
    else if( aEvent.IsAction( &SCH_ACTIONS::editFootprint ) )
        filter = CmpOrFootprint;

    SELECTION& selection = m_selectionTool->RequestSelection( filter );

    if( selection.Empty() )
        return 0;

    SCH_ITEM* item = (SCH_ITEM*) selection.Front();

    if( item->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) item;

        if( aEvent.IsAction( &SCH_ACTIONS::editReference ) )
            m_frame->EditComponentFieldText( component->GetField( REFERENCE ) );
        else if( aEvent.IsAction( &SCH_ACTIONS::editValue ) )
            m_frame->EditComponentFieldText( component->GetField( VALUE ) );
        else if( aEvent.IsAction( &SCH_ACTIONS::editFootprint ) )
            m_frame->EditComponentFieldText( component->GetField( FOOTPRINT ) );
    }
    else if( item->Type() == SCH_FIELD_T )
    {
        m_frame->EditComponentFieldText( (SCH_FIELD*) item );
    }

    return 0;
}


int SCH_EDIT_TOOL::AutoplaceFields( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::ComponentsOnly );

    if( selection.Empty() )
        return 0;

    SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();

    if( !component->IsNew() )
        m_frame->SaveCopyInUndoList( component, UR_CHANGED );

    component->AutoplaceFields( m_frame->GetScreen(), /* aManual */ true );

    updateView( component );
    m_frame->OnModify();

    return 0;
}


int SCH_EDIT_TOOL::ConvertDeMorgan( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::ComponentsOnly );

    if( selection.Empty() )
        return 0;

    SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();

    if( component->IsNew() )
        m_toolMgr->RunAction( SCH_ACTIONS::refreshPreview );
    else
        m_frame->SaveCopyInUndoList( component, UR_CHANGED );

    m_frame->ConvertPart( component );

    return 0;
}


int SCH_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::EditableItems );

    if( selection.Empty() )
        return 0;

    SCH_ITEM* item = (SCH_ITEM*) selection.Front();

    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
        m_frame->EditComponent( (SCH_COMPONENT*) item );
        break;

    case SCH_SHEET_T:
    {
        bool doClearAnnotation;
        bool doRefresh = false;
        // Keep track of existing sheet paths. EditSheet() can modify this list
        SCH_SHEET_LIST initial_sheetpathList( g_RootSheet );

        doRefresh = m_frame->EditSheet( (SCH_SHEET*) item, g_CurrentSheet, &doClearAnnotation );

        if( doClearAnnotation )     // happens when the current sheet load a existing file
        {                           // we must clear "new" components annotation
            SCH_SCREENS screensList( g_RootSheet );
            // We clear annotation of new sheet paths here:
            screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
            // Clear annotation of g_CurrentSheet itself, because its sheetpath
            // is not a new path, but components managed by its sheet path must have
            // their annotation cleared, becuase they are new:
            ((SCH_SHEET*) item)->GetScreen()->ClearAnnotation( g_CurrentSheet );
        }

        if( doRefresh )
            m_frame->GetCanvas()->Refresh();

        break;
    }

    case SCH_SHEET_PIN_T:
        m_frame->EditSheetPin( (SCH_SHEET_PIN*) item, true );
        break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
        m_frame->EditSchematicText( (SCH_TEXT*) item );
        break;

    case SCH_FIELD_T:
        m_frame->EditComponentFieldText( (SCH_FIELD*) item );
        break;

    case SCH_BITMAP_T:
        if( m_frame->EditImage( (SCH_BITMAP*) item ) )
        {
            // The bitmap is cached in Opengl: clear the cache in case it has become invalid
            getView()->RecacheAllItems();
        }

        break;

    case SCH_LINE_T:
        m_frame->EditLine( (SCH_LINE*) item, true );
        break;

    case SCH_MARKER_T:        // These items have no properties to edit
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
        break;

    default:                // Unexpected item
        wxFAIL_MSG( wxString( "Cannot edit schematic item type " ) + item->GetClass() );
    }

    updateView( item );

    return 0;
}


int SCH_EDIT_TOOL::ChangeShape( const TOOL_EVENT& aEvent )
{
    SELECTION selection = m_selectionTool->GetSelection();
    char shape;

    if( aEvent.IsAction( &SCH_ACTIONS::toShapeSlash ) )
        shape = '/';
    else if( aEvent.IsAction( &SCH_ACTIONS::toShapeBackslash ) )
        shape = '\\';
    else
        return 0;

    for( int i = 0; i < selection.GetSize(); ++i )
    {
        SCH_BUS_ENTRY_BASE* entry = dynamic_cast<SCH_BUS_ENTRY_BASE*>( selection.GetItem( i ) );

        if( entry )
        {
            if( entry->GetEditFlags() == 0 )
                m_frame->SaveCopyInUndoList( entry, UR_CHANGED );

            entry->SetBusEntryShape( shape );
            m_frame->TestDanglingEnds();

            updateView( entry );
            m_frame->OnModify( );
        }
    }

    g_lastBusEntryShape = shape;

    return 0;
}


int SCH_EDIT_TOOL::ChangeTextType( const TOOL_EVENT& aEvent )
{
    KICAD_T allTextTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, SCH_TEXT_T, EOT };
    SELECTION selection = m_selectionTool->RequestSelection( allTextTypes );
    KICAD_T convertTo;

    if( aEvent.IsAction( &SCH_ACTIONS::toLabel ) )
        convertTo = SCH_LABEL_T;
    else if( aEvent.IsAction( &SCH_ACTIONS::toHLabel ) )
        convertTo = SCH_HIER_LABEL_T;
    else if( aEvent.IsAction( &SCH_ACTIONS::toGLabel ) )
        convertTo = SCH_GLOBAL_LABEL_T;
    else if( aEvent.IsAction( &SCH_ACTIONS::toText ) )
        convertTo = SCH_TEXT_T;
    else
        return 0;

    for( int i = 0; i < selection.GetSize(); ++i )
    {
        SCH_TEXT* text = dynamic_cast<SCH_TEXT*>( selection.GetItem( i ) );

        if( text )
            m_frame->ConvertTextType( text, convertTo );
    }

    return 0;
}


int SCH_EDIT_TOOL::BreakWire( const TOOL_EVENT& aEvent )
{
    VECTOR2I cursorPos = m_controls->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );

    if( m_frame->BreakSegments( (wxPoint) cursorPos ) )
    {
        m_frame->TestDanglingEnds();

        m_frame->OnModify();
        m_frame->GetCanvas()->Refresh();
    }

    return 0;
}


int SCH_EDIT_TOOL::CleanupSheetPins( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::SheetsOnly );
    SCH_SHEET* sheet = (SCH_SHEET*) selection.Front();

    if( !sheet )
        return 0;

    if( !sheet->HasUndefinedPins() )
    {
        DisplayInfoMessage( m_frame, _( "There are no unreferenced pins in this sheet to remove." ) );
        return 0;
    }

    if( !IsOK( m_frame, _( "Do you wish to delete the unreferenced pins from this sheet?" ) ) )
        return 0;

    m_frame->SaveCopyInUndoList( sheet, UR_CHANGED );

    sheet->CleanupSheet();

    updateView( sheet );
    m_frame->OnModify();

    return 0;
}


void SCH_EDIT_TOOL::updateView( EDA_ITEM* aItem )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent() );

    getView()->Update( aItem );
}


void SCH_EDIT_TOOL::saveCopyInUndoList( SCH_ITEM* aItem, UNDO_REDO_T aType, bool aAppend )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        m_frame->SaveCopyInUndoList( (SCH_ITEM*)aItem->GetParent(), aType, aAppend );
    else
        m_frame->SaveCopyInUndoList( aItem, aType, aAppend );
}


void SCH_EDIT_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TOOL::Main,               SCH_ACTIONS::move.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Main,               SCH_ACTIONS::drag.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Duplicate,          SCH_ACTIONS::duplicate.MakeEvent() );
    Go( &SCH_EDIT_TOOL::RepeatDrawItem,     SCH_ACTIONS::repeatDrawItem.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorX.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorY.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DoDelete,           SCH_ACTIONS::doDelete.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DeleteItemCursor,   SCH_ACTIONS::deleteItemCursor.MakeEvent() );

    Go( &SCH_EDIT_TOOL::Properties,         SCH_ACTIONS::properties.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editReference.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editValue.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editFootprint.MakeEvent() );
    Go( &SCH_EDIT_TOOL::AutoplaceFields,    SCH_ACTIONS::autoplaceFields.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ConvertDeMorgan,    SCH_ACTIONS::convertDeMorgan.MakeEvent() );

    Go( &SCH_EDIT_TOOL::ChangeShape,        SCH_ACTIONS::toShapeSlash.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeShape,        SCH_ACTIONS::toShapeBackslash.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toHLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toGLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toText.MakeEvent() );

    Go( &SCH_EDIT_TOOL::BreakWire,          SCH_ACTIONS::breakWire.MakeEvent() );
    Go( &SCH_EDIT_TOOL::BreakWire,          SCH_ACTIONS::breakBus.MakeEvent() );

    Go( &SCH_EDIT_TOOL::CleanupSheetPins,   SCH_ACTIONS::cleanupSheetPins.MakeEvent() );
}
