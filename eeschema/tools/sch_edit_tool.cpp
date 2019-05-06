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
#include <tools/sch_wire_bus_tool.h>
#include <tools/sch_picker_tool.h>
#include <tools/sch_move_tool.h>
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
#include <eeschema_id.h>
#include <status_popup.h>
#include <wx/gdicmn.h>
#include "sch_drawing_tool.h"


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
        m_frame( nullptr ),
        m_menu( *this )
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
    SCH_MOVE_TOOL* moveTool = m_toolMgr->GetTool<SCH_MOVE_TOOL>();

    wxASSERT_MSG( m_selectionTool, "eeshema.InteractiveSelection tool is not available" );
    wxASSERT_MSG( drawingTool, "eeshema.InteractiveDrawing tool is not available" );

    auto sheetTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_SHEET_SYMBOL_BUTT );
    };

    auto anyTextTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_LABEL_BUTT
              || m_frame->GetToolId() == ID_GLOBALLABEL_BUTT
              || m_frame->GetToolId() == ID_HIERLABEL_BUTT
              || m_frame->GetToolId() == ID_TEXT_COMMENT_BUTT );
    };

    auto duplicateCondition = [] ( const SELECTION& aSel ) {
        if( SCH_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        return true;
    };

    auto orientCondition = [] ( const SELECTION& aSel ) {
        if( aSel.Empty() )
            return false;

        if( SCH_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
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
    // Add edit actions to the move tool menu
    //
    if( moveTool )
    {
        CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

        moveMenu.AddSeparator( SELECTION_CONDITIONS::NotEmpty );
        moveMenu.AddItem( SCH_ACTIONS::rotateCCW,       orientCondition );
        moveMenu.AddItem( SCH_ACTIONS::rotateCW,        orientCondition );
        moveMenu.AddItem( SCH_ACTIONS::mirrorX,         orientCondition );
        moveMenu.AddItem( SCH_ACTIONS::mirrorY,         orientCondition );
        moveMenu.AddItem( SCH_ACTIONS::duplicate,       duplicateCondition );
        moveMenu.AddItem( SCH_ACTIONS::doDelete,        SCH_CONDITIONS::NotEmpty );

        moveMenu.AddItem( SCH_ACTIONS::properties,      propertiesCondition );
        moveMenu.AddItem( SCH_ACTIONS::editReference,   singleComponentCondition );
        moveMenu.AddItem( SCH_ACTIONS::editValue,       singleComponentCondition );
        moveMenu.AddItem( SCH_ACTIONS::editFootprint,   singleComponentCondition );
        moveMenu.AddItem( SCH_ACTIONS::convertDeMorgan, SCH_CONDITIONS::SingleDeMorganSymbol );

        std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu = std::make_shared<SYMBOL_UNIT_MENU>();
        symUnitMenu->SetTool( this );
        m_menu.AddSubMenu( symUnitMenu );
        moveMenu.AddMenu( symUnitMenu.get(), false, SCH_CONDITIONS::SingleMultiUnitSymbol, 1 );

        moveMenu.AddSeparator( SCH_CONDITIONS::IdleSelection );
        moveMenu.AddItem( SCH_ACTIONS::cut,  SCH_CONDITIONS::IdleSelection );
        moveMenu.AddItem( SCH_ACTIONS::copy, SCH_CONDITIONS::IdleSelection );
    }

    //
    // Add editing actions to the drawing tool menu
    //
    CONDITIONAL_MENU& drawMenu = drawingTool->GetToolMenu().GetMenu();

    drawMenu.AddSeparator( SCH_CONDITIONS::NotEmpty, 200 );
    drawMenu.AddItem( SCH_ACTIONS::rotateCCW,       orientCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::rotateCW,        orientCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::mirrorX,         orientCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::mirrorY,         orientCondition, 200 );

    drawMenu.AddItem( SCH_ACTIONS::properties,      propertiesCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::editReference,   singleComponentCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::editValue,       singleComponentCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::editFootprint,   singleComponentCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::convertDeMorgan, SCH_CONDITIONS::SingleDeMorganSymbol, 200 );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu2 = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu2->SetTool( drawingTool );
    drawingTool->GetToolMenu().AddSubMenu( symUnitMenu2 );
    drawMenu.AddMenu( symUnitMenu2.get(), false, SCH_CONDITIONS::SingleMultiUnitSymbol, 1 );

    drawMenu.AddItem( SCH_ACTIONS::toShapeSlash,     entryCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toShapeBackslash, entryCondition, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toLabel,          anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toHLabel,         anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toGLabel,         anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toText,           anyTextTool && SCH_CONDITIONS::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::cleanupSheetPins, sheetTool && SCH_CONDITIONS::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::resizeSheet,      sheetTool && SCH_CONDITIONS::Idle, 200 );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( SCH_ACTIONS::rotateCCW,        orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::rotateCW,         orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::mirrorX,          orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::mirrorY,          orientCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::duplicate,        duplicateCondition, 200 );
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
    selToolMenu.AddItem( SCH_ACTIONS::cut,              SCH_CONDITIONS::IdleSelection, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::copy,             SCH_CONDITIONS::IdleSelection, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::paste,            SCH_CONDITIONS::Idle, 200 );

    return true;
}


void SCH_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
    {
        // Init variables used by every drawing tool
        m_frame = getEditFrame<SCH_EDIT_FRAME>();
    }
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
        SCH_ITEM* newItem = oldItem->Duplicate();
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
    m_toolMgr->RunAction( SCH_ACTIONS::move );

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

        wxPoint cursorPos = (wxPoint) getViewControls()->GetCursorPosition( true );
        newItem->Move( cursorPos - newItem->GetPosition() );
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
        m_toolMgr->RunAction( SCH_ACTIONS::move, true );

    newItem->ClearFlags();

    if( newItem->IsConnectable() )
        m_frame->TestDanglingEnds();

    // newItem newItem, now that it has been moved, thus saving new position.
    m_frame->SetRepeatItem( newItem );

    return 0;
}


int SCH_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*  screen = m_frame->GetScreen();
    auto         items = m_selectionTool->RequestSelection().GetItems();
    bool         appendToUndo = false;

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

    for( EDA_ITEM* item : items )
    {
        // Junctions, in particular, may have already been deleted if deleting wires made
        // them redundant
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
        {
            m_frame->DeleteJunction( (SCH_ITEM*) item, appendToUndo );
            appendToUndo = true;
        }
        else
        {
            item->SetFlags( STRUCT_DELETED );
            saveCopyInUndoList( item, UR_DELETED, appendToUndo );
            appendToUndo = true;

            updateView( item );

            SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item );

            if( sch_item && sch_item->IsConnectable() )
            {
                std::vector< wxPoint > pts;
                sch_item->GetConnectionPoints( pts );

                for( auto point : pts )
                {
                    SCH_ITEM* junction = screen->GetItem( point, 0, SCH_JUNCTION_T );
                    if( junction && !screen->IsJunctionNeeded( point ) )
                        m_frame->DeleteJunction( junction, appendToUndo );
                }
            }

            if( item->Type() == SCH_SHEET_PIN_T )
                static_cast<SCH_SHEET*>( item->GetParent() )->RemovePin( (SCH_SHEET_PIN*) item );
            else
                m_frame->RemoveFromScreen( item );
        }
    }

    m_frame->SetRepeatItem( nullptr );
    m_frame->TestDanglingEnds();

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


static bool deleteItem( SCH_BASE_FRAME* aFrame, const VECTOR2D& aPosition )
{
    SCH_SELECTION_TOOL* selectionTool = aFrame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    wxCHECK( selectionTool, false );

    aFrame->GetToolManager()->RunAction( SCH_ACTIONS::clearSelection, true );

    EDA_ITEM* item = selectionTool->SelectPoint( aPosition );
    SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item );

    if( sch_item && sch_item->IsLocked() )
    {
        STATUS_TEXT_POPUP statusPopup( aFrame );
        statusPopup.SetText( _( "Item locked." ) );
        statusPopup.Expire( 2000 );
        statusPopup.Popup();
        statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
        return true;
    }

    if( item )
        aFrame->GetToolManager()->RunAction( SCH_ACTIONS::doDelete, true );

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
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );

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


void SCH_EDIT_TOOL::saveCopyInUndoList( EDA_ITEM* aItem, UNDO_REDO_T aType, bool aAppend )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        m_frame->SaveCopyInUndoList( (SCH_ITEM*) aItem->GetParent(), UR_CHANGED, aAppend );
    else
        m_frame->SaveCopyInUndoList( (SCH_ITEM*) aItem, aType, aAppend );
}


void SCH_EDIT_TOOL::setTransitions()
{
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
