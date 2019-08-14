/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#include <tool/picker_tool.h>
#include <tools/sch_edit_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>
#include <ee_actions.h>
#include <bitmaps.h>
#include <confirm.h>
#include <base_struct.h>
#include <sch_item.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_text.h>
#include <sch_bitmap.h>
#include <sch_view.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include <status_popup.h>
#include <wx/gdicmn.h>
#include <invoke_sch_dialog.h>
#include <dialogs/dialog_image_editor.h>
#include <dialogs/dialog_edit_line_style.h>
#include <dialogs/dialog_edit_component_in_schematic.h>
#include <dialogs/dialog_sch_edit_sheet_pin.h>
#include <dialogs/dialog_edit_one_field.h>
#include "sch_drawing_tools.h"


char g_lastBusEntryShape = '/';


class SYMBOL_UNIT_MENU : public ACTION_MENU
{
public:
    SYMBOL_UNIT_MENU() :
        ACTION_MENU( true )
    {
        SetIcon( component_select_unit_xpm );
        SetTitle( _( "Symbol Unit" ) );
    }

protected:
    ACTION_MENU* create() const override
    {
        return new SYMBOL_UNIT_MENU();
    }

private:
    void update() override
    {
        EE_SELECTION_TOOL* selTool = getToolManager()->GetTool<EE_SELECTION_TOOL>();
        EE_SELECTION&      selection = selTool->GetSelection();
        SCH_COMPONENT*     component = dynamic_cast<SCH_COMPONENT*>( selection.Front() );

        Clear();

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
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveEdit" )
{
}


using E_C = EE_CONDITIONS;

bool SCH_EDIT_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    SCH_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<SCH_DRAWING_TOOLS>();
    SCH_MOVE_TOOL*     moveTool = m_toolMgr->GetTool<SCH_MOVE_TOOL>();

    wxASSERT_MSG( drawingTools, "eeshema.InteractiveDrawing tool is not available" );

    auto sheetTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->IsCurrentTool( EE_ACTIONS::drawSheet ) );
    };

    auto anyTextTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->IsCurrentTool( EE_ACTIONS::placeLabel )
              || m_frame->IsCurrentTool( EE_ACTIONS::placeGlobalLabel )
              || m_frame->IsCurrentTool( EE_ACTIONS::placeHierLabel )
              || m_frame->IsCurrentTool( EE_ACTIONS::placeSchematicText ) );
    };

    auto duplicateCondition = [] ( const SELECTION& aSel ) {
        if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        return true;
    };

    auto orientCondition = [] ( const SELECTION& aSel ) {
        if( aSel.Empty() )
            return false;

        if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        SCH_ITEM* item = (SCH_ITEM*) aSel.Front();

        if( aSel.GetSize() > 1 )
            return true;

        switch( item->Type() )
        {
        case SCH_MARKER_T:
        case SCH_JUNCTION_T:
        case SCH_NO_CONNECT_T:
        case SCH_LINE_T:
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

    static KICAD_T toLabelTypes[] = { SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, SCH_TEXT_T, EOT };
    auto toLabelCondition = E_C::Count( 1 ) && E_C::OnlyTypes( toLabelTypes );

    static KICAD_T toHLableTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_TEXT_T, EOT };
    auto toHLabelCondition = E_C::Count( 1 ) && E_C::OnlyTypes( toHLableTypes );

    static KICAD_T toGLableTypes[] = { SCH_LABEL_T, SCH_HIER_LABEL_T, SCH_TEXT_T, EOT };
    auto toGLabelCondition = E_C::Count( 1 ) && E_C::OnlyTypes( toGLableTypes );

    static KICAD_T toTextTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, EOT };
    auto toTextlCondition = E_C::Count( 1 ) && E_C::OnlyTypes( toTextTypes );

    static KICAD_T entryTypes[] = { SCH_BUS_WIRE_ENTRY_T, SCH_BUS_BUS_ENTRY_T, EOT };
    auto entryCondition = E_C::MoreThan( 0 ) && E_C::OnlyTypes( entryTypes );

    auto singleComponentCondition = E_C::Count( 1 )    && E_C::OnlyType( SCH_COMPONENT_T );
    auto wireSelectionCondition =   E_C::MoreThan( 0 ) && E_C::OnlyType( SCH_LINE_LOCATE_WIRE_T );
    auto busSelectionCondition =    E_C::MoreThan( 0 ) && E_C::OnlyType( SCH_LINE_LOCATE_BUS_T );
    auto singleSheetCondition =     E_C::Count( 1 )    && E_C::OnlyType( SCH_SHEET_T );

    //
    // Add edit actions to the move tool menu
    //
    if( moveTool )
    {
        CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

        moveMenu.AddSeparator();
        moveMenu.AddItem( EE_ACTIONS::rotateCCW,       orientCondition );
        moveMenu.AddItem( EE_ACTIONS::rotateCW,        orientCondition );
        moveMenu.AddItem( EE_ACTIONS::mirrorX,         orientCondition );
        moveMenu.AddItem( EE_ACTIONS::mirrorY,         orientCondition );
        moveMenu.AddItem( ACTIONS::doDelete,           E_C::NotEmpty );

        moveMenu.AddItem( EE_ACTIONS::properties,      propertiesCondition );
        moveMenu.AddItem( EE_ACTIONS::editReference,   singleComponentCondition );
        moveMenu.AddItem( EE_ACTIONS::editValue,       singleComponentCondition );
        moveMenu.AddItem( EE_ACTIONS::editFootprint,   singleComponentCondition );
        moveMenu.AddItem( EE_ACTIONS::toggleDeMorgan,  E_C::SingleDeMorganSymbol );

        std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu = std::make_shared<SYMBOL_UNIT_MENU>();
        symUnitMenu->SetTool( this );
        m_menu.AddSubMenu( symUnitMenu );
        moveMenu.AddMenu( symUnitMenu.get(), E_C::SingleMultiUnitSymbol, 1 );

        moveMenu.AddSeparator();
        moveMenu.AddItem( ACTIONS::cut,                E_C::IdleSelection );
        moveMenu.AddItem( ACTIONS::copy,               E_C::IdleSelection );
        moveMenu.AddItem( ACTIONS::duplicate,          duplicateCondition );
    }

    //
    // Add editing actions to the drawing tool menu
    //
    CONDITIONAL_MENU& drawMenu = drawingTools->GetToolMenu().GetMenu();

    drawMenu.AddItem( EE_ACTIONS::rotateCCW,        orientCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::rotateCW,         orientCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorX,          orientCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorY,          orientCondition, 200 );

    drawMenu.AddItem( EE_ACTIONS::properties,       propertiesCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::editReference,    singleComponentCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::editValue,        singleComponentCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::editFootprint,    singleComponentCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::autoplaceFields,  singleComponentCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::toggleDeMorgan,   E_C::SingleDeMorganSymbol, 200 );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu2 = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu2->SetTool( drawingTools );
    drawingTools->GetToolMenu().AddSubMenu( symUnitMenu2 );
    drawMenu.AddMenu( symUnitMenu2.get(), E_C::SingleMultiUnitSymbol, 1 );

    drawMenu.AddItem( EE_ACTIONS::editWithLibEdit, singleComponentCondition && E_C::Idle, 200 );

    drawMenu.AddItem( EE_ACTIONS::toShapeSlash,        entryCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::toShapeBackslash,    entryCondition, 200 );
    drawMenu.AddItem( EE_ACTIONS::toLabel,             anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::toHLabel,            anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::toGLabel,            anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::toText,              anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::cleanupSheetPins,    sheetTool && E_C::Idle, 250 );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::rotateCCW,        orientCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::rotateCW,         orientCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorX,          orientCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorY,          orientCondition, 200 );
    selToolMenu.AddItem( ACTIONS::doDelete,            E_C::NotEmpty, 200 );

    selToolMenu.AddItem( EE_ACTIONS::properties,       propertiesCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::editReference,    E_C::SingleSymbol, 200 );
    selToolMenu.AddItem( EE_ACTIONS::editValue,        E_C::SingleSymbol, 200 );
    selToolMenu.AddItem( EE_ACTIONS::editFootprint,    E_C::SingleSymbol, 200 );
    selToolMenu.AddItem( EE_ACTIONS::autoplaceFields,  singleComponentCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::toggleDeMorgan,   E_C::SingleSymbol, 200 );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu3 = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu3->SetTool( m_selectionTool );
    m_selectionTool->GetToolMenu().AddSubMenu( symUnitMenu3 );
    selToolMenu.AddMenu( symUnitMenu3.get(), E_C::SingleMultiUnitSymbol, 1 );

    selToolMenu.AddItem( EE_ACTIONS::editWithLibEdit, singleComponentCondition && E_C::Idle, 200 );

    selToolMenu.AddItem( EE_ACTIONS::toShapeSlash,     entryCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::toShapeBackslash, entryCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::toLabel,          toLabelCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::toHLabel,         toHLabelCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::toGLabel,         toGLabelCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::toText,           toTextlCondition, 200 );
    selToolMenu.AddItem( EE_ACTIONS::cleanupSheetPins, singleSheetCondition, 250 );

    selToolMenu.AddSeparator( 300 );
    selToolMenu.AddItem( ACTIONS::cut,                 E_C::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copy,                E_C::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::paste,               E_C::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::duplicate,           duplicateCondition, 300 );

    return true;
}


const KICAD_T rotatableItems[] = {
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_BITMAP_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_LINE_T,
    SCH_JUNCTION_T,
    EOT
};


int SCH_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( rotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   rotPoint;
    bool      clockwise = ( aEvent.Matches( EE_ACTIONS::rotateCW.MakeEvent() ) );
    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.Front() );
    bool      connections = false;
    bool      moving = item->IsMoving();

    if( selection.GetSize() == 1 )
    {
        if( !moving )
            saveCopyInUndoList( item, UR_CHANGED );

        for( int i = 0; clockwise ? i < 1 : i < 3; ++i )
        {
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

            case SCH_SHEET_PIN_T:
            {
                // Rotate pin within parent sheet
                SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( item );
                SCH_SHEET*     sheet = pin->GetParent();
                pin->Rotate( sheet->GetBoundingBox().GetCenter() );
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
                // Rotate the sheet on itself. Sheets do not have an anchor point.
                rotPoint = m_frame->GetNearestGridPosition( item->GetBoundingBox().Centre() );
                item->Rotate( rotPoint );
                break;

            default:
                break;
            }
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

            for( int i = 0; clockwise ? i < 1 : i < 3; ++i )
            {
                if( item->Type() == SCH_LINE_T )
                {
                    SCH_LINE* line = (SCH_LINE*) item;

                    if( item->GetFlags() & STARTPOINT )
                        line->RotateStart( rotPoint );

                    if( item->GetFlags() & ENDPOINT )
                        line->RotateEnd( rotPoint );
                }
                else if( item->Type() == SCH_SHEET_PIN_T )
                {
                    if( item->GetParent()->IsSelected() )
                    {
                        // parent will rotate us
                    }
                    else
                    {
                        // rotate within parent
                        SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( item );
                        SCH_SHEET*     sheet = pin->GetParent();

                        pin->Rotate( sheet->GetBoundingBox().GetCenter() );
                    }
                }
                else
                {
                    item->Rotate( rotPoint );
                }
            }

            connections |= item->IsConnectable();
            m_frame->RefreshItem( item );
        }
    }

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    if( item->IsMoving() )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        if( connections )
            m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    return 0;
}


int SCH_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( rotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   mirrorPoint;
    bool      xAxis = ( aEvent.Matches( EE_ACTIONS::mirrorX.MakeEvent() ) );
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

        case SCH_SHEET_PIN_T:
        {
            // mirror within parent sheet
            SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( item );
            SCH_SHEET*     sheet = pin->GetParent();

            if( xAxis )
                pin->MirrorX( sheet->GetBoundingBox().GetCenter().y );
            else
                pin->MirrorY( sheet->GetBoundingBox().GetCenter().x );

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
        mirrorPoint = (wxPoint)selection.GetReferencePoint();

        for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
        {
            item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

            if( !moving )
                saveCopyInUndoList( item, UR_CHANGED, ii > 0 );

            if( item->Type() == SCH_SHEET_PIN_T )
            {
                if( item->GetParent()->IsSelected() )
                {
                    // parent will mirror us
                }
                else
                {
                    // mirror within parent sheet
                    SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( item );
                    SCH_SHEET*     sheet = pin->GetParent();

                    if( xAxis )
                        pin->MirrorX( sheet->GetBoundingBox().GetCenter().y );
                    else
                        pin->MirrorY( sheet->GetBoundingBox().GetCenter().x );
                }
            }
            else
            {
                if( xAxis )
                    item->MirrorX( mirrorPoint.y );
                else
                    item->MirrorY( mirrorPoint.x );
            }

            connections |= item->IsConnectable();
            m_frame->RefreshItem( item );
        }
    }

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    if( item->IsMoving() )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        if( connections )
            m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    return 0;
}


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


int SCH_EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( duplicatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    // Doing a duplicate of a new object doesn't really make any sense; we'd just end
    // up dragging around a stack of objects...
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

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    m_toolMgr->RunAction( EE_ACTIONS::addItemsToSel, true, &newItems );
    m_toolMgr->RunAction( EE_ACTIONS::move, false );

    return 0;
}


int SCH_EDIT_TOOL::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    SCH_ITEM* sourceItem = m_frame->GetRepeatItem();

    if( !sourceItem )
        return 0;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

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
        m_toolMgr->RunAction( EE_ACTIONS::move, true );

    newItem->ClearFlags();

    if( newItem->IsConnectable() )
    {
        auto selection = m_selectionTool->GetSelection();

        m_toolMgr->RunAction( EE_ACTIONS::addNeededJunctions, true, &selection );
        m_frame->SchematicCleanUp();
        m_frame->TestDanglingEnds();
    }

    // newItem newItem, now that it has been moved, thus saving new position.
    m_frame->SaveCopyForRepeatItem( newItem );

    return 0;
}


static KICAD_T deletableItems[] =
{
    SCH_MARKER_T,
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
    SCH_SHEET_PIN_T,
    SCH_COMPONENT_T,
    SCH_BITMAP_T,
    EOT
};


int SCH_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*          screen = m_frame->GetScreen();
    auto                 items = m_selectionTool->RequestSelection( deletableItems ).GetItems();
    bool                 appendToUndo = false;
    std::vector<wxPoint> pts;

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    for( EDA_ITEM* item : items )
        item->ClearFlags( STRUCT_DELETED );

    for( EDA_ITEM* item : items )
    {
        if( item->Type() == SCH_JUNCTION_T )
        {
            // clean up junctions at the end
        }
        else
        {
            item->SetFlags( STRUCT_DELETED );
            saveCopyInUndoList( item, UR_DELETED, appendToUndo );
            appendToUndo = true;

            SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item );

            if( sch_item && sch_item->IsConnectable() )
                sch_item->GetConnectionPoints( pts );

            updateView( item );

            if( item->Type() == SCH_SHEET_PIN_T )
                static_cast<SCH_SHEET*>( item->GetParent() )->RemovePin( (SCH_SHEET_PIN*) item );
            else
                m_frame->RemoveFromScreen( item );
        }
    }

    for( auto point : pts )
    {
        SCH_ITEM* junction = screen->GetItem( point, 0, SCH_JUNCTION_T );
        if( junction && !screen->IsJunctionNeeded( point ) )
            m_frame->DeleteJunction( junction, appendToUndo );
    }

    m_frame->TestDanglingEnds();

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int SCH_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    std::string  tool = aEvent.GetCommandStr().get();
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    m_pickerItem = nullptr;

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( wxStockCursor( wxCURSOR_BULLSEYE ) );

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPosition ) -> bool
        {
            if( m_pickerItem )
            {
                SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( m_pickerItem );

                if( sch_item && sch_item->IsLocked() )
                {
                    STATUS_TEXT_POPUP statusPopup( m_frame );
                    statusPopup.SetText( _( "Item locked." ) );
                    statusPopup.PopupFor( 2000 );
                    statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                    return true;
                }

                EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
                selectionTool->UnbrightenItem( m_pickerItem );
                selectionTool->AddItemToSel( m_pickerItem, true );
                m_toolMgr->RunAction( ACTIONS::doDelete, true );
                m_pickerItem = nullptr;
            }

            return true;
        } );

    picker->SetMotionHandler(
        [this] ( const VECTOR2D& aPos )
        {
            EE_COLLECTOR collector;
            collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
            collector.Collect( m_frame->GetScreen()->GetDrawItems(), deletableItems, (wxPoint) aPos );

            EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
            selectionTool->GuessSelectionCandidates( collector, aPos );

            EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

            if( m_pickerItem != item )
            {
                if( m_pickerItem )
                    selectionTool->UnbrightenItem( m_pickerItem );

                m_pickerItem = item;

                if( m_pickerItem )
                    selectionTool->BrightenItem( m_pickerItem );
            }
        } );

    picker->SetFinalizeHandler(
        [this] ( const int& aFinalState )
        {
            if( m_pickerItem )
                m_toolMgr->GetTool<EE_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


void SCH_EDIT_TOOL::editComponentFieldText( SCH_FIELD* aField )
{
    SCH_COMPONENT* component = (SCH_COMPONENT*) aField->GetParent();

    // Save old component in undo list if not already in edit, or moving.
    if( aField->GetEditFlags() == 0 )    // i.e. not edited, or moved
        m_frame->SaveCopyInUndoList( component, UR_CHANGED );

    wxString title = wxString::Format( _( "Edit %s Field" ), aField->GetName() );

    DIALOG_SCH_EDIT_ONE_FIELD dlg( m_frame, title, aField );

    // The footprint field dialog can invoke a KIWAY_PLAYER so we must use a quasi-modal
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    dlg.UpdateField( aField, g_CurrentSheet );

    if( m_frame->GetAutoplaceFields() )
        component->AutoAutoplaceFields( m_frame->GetScreen() );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->RefreshItem( aField );
    m_frame->OnModify();
}


int SCH_EDIT_TOOL::EditField( const TOOL_EVENT& aEvent )
{
    static KICAD_T Nothing[]        = { EOT };
    static KICAD_T CmpOrReference[] = { SCH_FIELD_LOCATE_REFERENCE_T, SCH_COMPONENT_T, EOT };
    static KICAD_T CmpOrValue[]     = { SCH_FIELD_LOCATE_VALUE_T,     SCH_COMPONENT_T, EOT };
    static KICAD_T CmpOrFootprint[] = { SCH_FIELD_LOCATE_FOOTPRINT_T, SCH_COMPONENT_T, EOT };

    KICAD_T* filter = Nothing;

    if( aEvent.IsAction( &EE_ACTIONS::editReference ) )
        filter = CmpOrReference;
    else if( aEvent.IsAction( &EE_ACTIONS::editValue ) )
        filter = CmpOrValue;
    else if( aEvent.IsAction( &EE_ACTIONS::editFootprint ) )
        filter = CmpOrFootprint;

    EE_SELECTION& selection = m_selectionTool->RequestSelection( filter );

    if( selection.Empty() )
        return 0;

    SCH_ITEM* item = (SCH_ITEM*) selection.Front();

    if( item->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) item;

        if( aEvent.IsAction( &EE_ACTIONS::editReference ) )
            editComponentFieldText( component->GetField( REFERENCE ) );
        else if( aEvent.IsAction( &EE_ACTIONS::editValue ) )
            editComponentFieldText( component->GetField( VALUE ) );
        else if( aEvent.IsAction( &EE_ACTIONS::editFootprint ) )
            editComponentFieldText( component->GetField( FOOTPRINT ) );
    }
    else if( item->Type() == SCH_FIELD_T )
    {
        editComponentFieldText( (SCH_FIELD*) item );
    }

    return 0;
}


int SCH_EDIT_TOOL::AutoplaceFields( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );

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


int SCH_EDIT_TOOL::UpdateFields( const TOOL_EVENT& aEvent )
{
    std::list<SCH_COMPONENT*> components;

    for( SCH_ITEM* item = m_frame->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_COMPONENT_T )
            components.push_back( static_cast<SCH_COMPONENT*>( item ) );
    }

    if( InvokeDialogUpdateFields( m_frame, components, true ) == wxID_OK )
        m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDIT_TOOL::ConvertDeMorgan( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );

    if( selection.Empty() )
        return 0;

    SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();

    if( aEvent.IsAction( &EE_ACTIONS::showDeMorganStandard )
            && component->GetConvert() == LIB_ITEM::LIB_CONVERT::BASE )
        return 0;

    if( aEvent.IsAction( &EE_ACTIONS::showDeMorganAlternate )
            && component->GetConvert() != LIB_ITEM::LIB_CONVERT::DEMORGAN )
        return 0;

    if( !component->IsNew() )
        m_frame->SaveCopyInUndoList( component, UR_CHANGED );

    m_frame->ConvertPart( component );

    if( component->IsNew() )
        m_toolMgr->RunAction( ACTIONS::refreshPreview );

    return 0;
}


int SCH_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::EditableItems );

    if( selection.Empty() )
        return 0;

    SCH_ITEM* item = (SCH_ITEM*) selection.Front();

    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) item;
        DIALOG_EDIT_COMPONENT_IN_SCHEMATIC dlg( m_frame, component );

        // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
        // frame. Therefore this dialog as a modal frame parent, MUST be run under
        // quasimodal mode for the quasimodal frame support to work.  So don't use
        // the QUASIMODAL macros here.
        if( dlg.ShowQuasiModal() == wxID_OK )
        {
            if( m_frame->GetAutoplaceFields() )
                component->AutoAutoplaceFields( m_frame->GetScreen() );

            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->OnModify();
        }
    }
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
        {
            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->GetCanvas()->Refresh();
        }

        break;
    }

    case SCH_SHEET_PIN_T:
    {
        SCH_SHEET_PIN* pin = (SCH_SHEET_PIN*) item;
        DIALOG_SCH_EDIT_SHEET_PIN dlg( m_frame, pin );

        if( dlg.ShowModal() == wxID_OK )
        {
            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->OnModify();
        }
    }
        break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
        if( InvokeDialogLabelEditor( m_frame, (SCH_TEXT*) item ) == wxID_OK )
        {
            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->OnModify();
        }

        break;

    case SCH_FIELD_T:
        editComponentFieldText( (SCH_FIELD*) item );
        break;

    case SCH_BITMAP_T:
    {
        SCH_BITMAP*         bitmap = (SCH_BITMAP*) item;
        DIALOG_IMAGE_EDITOR dlg( m_frame, bitmap->GetImage() );

        if( dlg.ShowModal() == wxID_OK )
        {
            // save old image in undo list if not already in edit
            if( bitmap->GetEditFlags() == 0 )
                m_frame->SaveCopyInUndoList( bitmap, UR_CHANGED );

            dlg.TransferToImage( bitmap->GetImage() );

            // The bitmap is cached in Opengl: clear the cache in case it has become invalid
            getView()->RecacheAllItems();
            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->OnModify();
        }
    }
        break;

    case SCH_LINE_T:
    {
        SCH_LINE* line = (SCH_LINE*) item;

        // We purposely disallow editing everything except graphic lines
        if( line->GetLayer() != LAYER_NOTES )
            break;

        DIALOG_EDIT_LINE_STYLE dlg( m_frame, line );

        if( dlg.ShowModal() == wxID_OK )
        {
            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->OnModify();
        }
    }
        break;

    case SCH_MARKER_T:        // These items have no properties to edit
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
        break;

    default:                // Unexpected item
        wxFAIL_MSG( wxString( "Cannot edit schematic item type " ) + item->GetClass() );
    }

    updateView( item );

    if( selection.IsHover() )
    {
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    }

    return 0;
}


int SCH_EDIT_TOOL::ChangeShape( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->GetSelection();
    char          shape = aEvent.Parameter<char>();

    for( unsigned int i = 0; i < selection.GetSize(); ++i )
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
    KICAD_T       convertTo = aEvent.Parameter<KICAD_T>();
    KICAD_T       allTextTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, SCH_TEXT_T, EOT };
    EE_SELECTION& selection = m_selectionTool->RequestSelection( allTextTypes );

    for( unsigned int i = 0; i < selection.GetSize(); ++i )
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
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::SheetsOnly );
    SCH_SHEET*    sheet = (SCH_SHEET*) selection.Front();

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


void SCH_EDIT_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TOOL::Duplicate,          ACTIONS::duplicate.MakeEvent() );
    Go( &SCH_EDIT_TOOL::RepeatDrawItem,     EE_ACTIONS::repeatDrawItem.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorX.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorY.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DoDelete,           ACTIONS::doDelete.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DeleteItemCursor,   ACTIONS::deleteTool.MakeEvent() );

    Go( &SCH_EDIT_TOOL::Properties,         EE_ACTIONS::properties.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          EE_ACTIONS::editReference.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          EE_ACTIONS::editValue.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          EE_ACTIONS::editFootprint.MakeEvent() );
    Go( &SCH_EDIT_TOOL::AutoplaceFields,    EE_ACTIONS::autoplaceFields.MakeEvent() );
    Go( &SCH_EDIT_TOOL::UpdateFields,       EE_ACTIONS::updateFieldsFromLibrary.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ConvertDeMorgan,    EE_ACTIONS::toggleDeMorgan.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ConvertDeMorgan,    EE_ACTIONS::showDeMorganStandard.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ConvertDeMorgan,    EE_ACTIONS::showDeMorganAlternate.MakeEvent() );

    Go( &SCH_EDIT_TOOL::ChangeShape,        EE_ACTIONS::toShapeSlash.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeShape,        EE_ACTIONS::toShapeBackslash.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toHLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toGLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toText.MakeEvent() );

    Go( &SCH_EDIT_TOOL::BreakWire,          EE_ACTIONS::breakWire.MakeEvent() );
    Go( &SCH_EDIT_TOOL::BreakWire,          EE_ACTIONS::breakBus.MakeEvent() );

    Go( &SCH_EDIT_TOOL::CleanupSheetPins,   EE_ACTIONS::cleanupSheetPins.MakeEvent() );
    Go( &SCH_EDIT_TOOL::GlobalEdit,         EE_ACTIONS::editTextAndGraphics.MakeEvent() );
}
