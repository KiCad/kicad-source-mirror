/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiway.h>
#include <tool/picker_tool.h>
#include <tools/sch_edit_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>
#include <tools/sch_drawing_tools.h>
#include <ee_actions.h>
#include <confirm.h>
#include <string_utils.h>
#include <sch_item.h>
#include <sch_symbol.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <sch_bitmap.h>
#include <sch_view.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <sch_commit.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <eeschema_id.h>
#include <dialogs/dialog_change_symbols.h>
#include <dialogs/dialog_image_properties.h>
#include <dialogs/dialog_line_properties.h>
#include <dialogs/dialog_wire_bus_properties.h>
#include <dialogs/dialog_symbol_properties.h>
#include <dialogs/dialog_sheet_pin_properties.h>
#include <dialogs/dialog_field_properties.h>
#include <dialogs/dialog_junction_props.h>
#include <dialogs/dialog_shape_properties.h>
#include <dialogs/dialog_label_properties.h>
#include <dialogs/dialog_text_properties.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>
#include <core/kicad_algo.h>
#include <wx/textdlg.h>
#include <project/net_settings.h>

class SYMBOL_UNIT_MENU : public ACTION_MENU
{
public:
    SYMBOL_UNIT_MENU() :
        ACTION_MENU( true )
    {
        SetIcon( BITMAPS::component_select_unit );
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
        SCH_SYMBOL*        symbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );

        Clear();

        if( !symbol )
        {
            Append( ID_POPUP_SCH_SELECT_UNIT_CMP, _( "no symbol selected" ), wxEmptyString );
            Enable( ID_POPUP_SCH_SELECT_UNIT_CMP, false );
            return;
        }

        int  unit = symbol->GetUnit();

        if( !symbol->GetLibSymbolRef() || symbol->GetLibSymbolRef()->GetUnitCount() < 2 )
        {
            Append( ID_POPUP_SCH_SELECT_UNIT_CMP, _( "symbol is not multi-unit" ), wxEmptyString );
            Enable( ID_POPUP_SCH_SELECT_UNIT_CMP, false );
            return;
        }

        for( int ii = 0; ii < symbol->GetLibSymbolRef()->GetUnitCount(); ii++ )
        {
            wxString num_unit;
            num_unit.Printf( _( "Unit %s" ), LIB_SYMBOL::SubReference( ii + 1, false ) );

            wxMenuItem * item = Append( ID_POPUP_SCH_SELECT_UNIT1 + ii, num_unit, wxEmptyString,
                                        wxITEM_CHECK );
            if( unit == ii + 1 )
                item->Check(true);

            // The ID max for these submenus is ID_POPUP_SCH_SELECT_UNIT_SYM_MAX
            // See eeschema_id to modify this value.
            if( ii >= (ID_POPUP_SCH_SELECT_UNIT_SYM_MAX - ID_POPUP_SCH_SELECT_UNIT1) )
                break;      // We have used all IDs for these submenus
        }
    }
};


SCH_EDIT_TOOL::SCH_EDIT_TOOL() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveEdit" )
{
    m_pickerItem = nullptr;
}


using E_C = EE_CONDITIONS;

bool SCH_EDIT_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    SCH_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<SCH_DRAWING_TOOLS>();
    SCH_MOVE_TOOL*     moveTool = m_toolMgr->GetTool<SCH_MOVE_TOOL>();

    wxASSERT_MSG( drawingTools, "eeshema.InteractiveDrawing tool is not available" );

    auto hasElements =
            [this]( const SELECTION& aSel )
            {
                return !m_frame->GetScreen()->Items().empty();
            };

    auto sheetHasUndefinedPins =
            []( const SELECTION& aSel )
            {
                if( aSel.Size() == 1 && aSel.Front()->Type() == SCH_SHEET_T )
                    return static_cast<SCH_SHEET*>( aSel.Front() )->HasUndefinedPins();

                return false;
            };

    auto sheetSelection = E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_SHEET_T } );

    auto haveHighlight =
            [&]( const SELECTION& sel )
            {
                SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

                return editFrame && !editFrame->GetHighlightedConnection().IsEmpty();
            };

    auto anyTextTool =
            [this]( const SELECTION& aSel )
            {
                return ( m_frame->IsCurrentTool( EE_ACTIONS::placeLabel )
                      || m_frame->IsCurrentTool( EE_ACTIONS::placeClassLabel )
                      || m_frame->IsCurrentTool( EE_ACTIONS::placeGlobalLabel )
                      || m_frame->IsCurrentTool( EE_ACTIONS::placeHierLabel )
                      || m_frame->IsCurrentTool( EE_ACTIONS::placeSchematicText ) );
            };

    auto duplicateCondition =
            []( const SELECTION& aSel )
            {
                if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
                    return false;

                return true;
            };

    auto orientCondition =
            []( const SELECTION& aSel )
            {
                if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
                    return false;

                return SELECTION_CONDITIONS::HasTypes( SCH_EDIT_TOOL::RotatableItems )( aSel );
            };

    auto propertiesCondition =
            [&]( const SELECTION& aSel )
            {
                if( aSel.GetSize() == 0 )
                {
                    if( getView()->IsLayerVisible( LAYER_SCHEMATIC_DRAWINGSHEET ) )
                    {
                        DS_PROXY_VIEW_ITEM* ds = m_frame->GetCanvas()->GetView()->GetDrawingSheet();
                        VECTOR2D            cursor = getViewControls()->GetCursorPosition( false );

                        if( ds && ds->HitTestDrawingSheetItems( getView(), cursor ) )
                            return true;
                    }

                    return false;
                }

                SCH_ITEM*           firstItem   = dynamic_cast<SCH_ITEM*>( aSel.Front() );
                const EE_SELECTION* eeSelection = dynamic_cast<const EE_SELECTION*>( &aSel );

                if( !firstItem || !eeSelection )
                    return false;

                switch( firstItem->Type() )
                {
                case SCH_SYMBOL_T:
                case SCH_SHEET_T:
                case SCH_SHEET_PIN_T:
                case SCH_TEXT_T:
                case SCH_TEXTBOX_T:
                case SCH_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                case SCH_HIER_LABEL_T:
                case SCH_DIRECTIVE_LABEL_T:
                case SCH_FIELD_T:
                case SCH_SHAPE_T:
                case SCH_BITMAP_T:
                    return aSel.GetSize() == 1;

                case SCH_LINE_T:
                case SCH_BUS_WIRE_ENTRY_T:
                case SCH_JUNCTION_T:
                    if( std::all_of( aSel.Items().begin(), aSel.Items().end(),
                            [&]( const EDA_ITEM* item )
                            {
                                return item->Type() == SCH_LINE_T
                                        && static_cast<const SCH_LINE*>( item )->IsGraphicLine();
                            } ) )
                    {
                        return true;
                    }
                    else if( std::all_of( aSel.Items().begin(), aSel.Items().end(),
                            [&]( const EDA_ITEM* item )
                            {
                                return item->Type() == SCH_JUNCTION_T;
                            } ) )
                    {
                        return true;
                    }
                    else if( std::all_of( aSel.Items().begin(), aSel.Items().end(),
                            [&]( const EDA_ITEM* item )
                            {
                                const SCH_ITEM* schItem = dynamic_cast<const SCH_ITEM*>( item );

                                wxCHECK( schItem, false );

                                return ( schItem->HasLineStroke() && schItem->IsConnectable() )
                                        || item->Type() == SCH_JUNCTION_T;
                            } ) )
                    {
                        return true;
                    }

                    return false;

                default:
                    return false;
                }
            };

    auto autoplaceCondition =
            []( const SELECTION& aSel )
            {
                for( const EDA_ITEM* item : aSel )
                {
                    if( item->IsType( EE_COLLECTOR::FieldOwners ) )
                        return true;
                }

                return false;
            };

    // allTextTypes does not include SCH_SHEET_PIN_T because one cannot convert other
    // types to/from this type, living only in a SHEET
    static std::vector<KICAD_T> allTextTypes = { SCH_LABEL_T,
                                                 SCH_DIRECTIVE_LABEL_T,
                                                 SCH_GLOBAL_LABEL_T,
                                                 SCH_HIER_LABEL_T,
                                                 SCH_TEXT_T,
                                                 SCH_TEXTBOX_T };

    auto toChangeCondition = ( E_C::OnlyTypes( allTextTypes ) );

    auto toLabelCondition = ( E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_DIRECTIVE_LABEL_T,
                                                                   SCH_GLOBAL_LABEL_T,
                                                                   SCH_HIER_LABEL_T,
                                                                   SCH_TEXT_T,
                                                                   SCH_TEXTBOX_T } ) )
                                || ( E_C::MoreThan( 1 ) && E_C::OnlyTypes( allTextTypes ) );

    auto toCLabelCondition = ( E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_LABEL_T,
                                                                    SCH_HIER_LABEL_T,
                                                                    SCH_GLOBAL_LABEL_T,
                                                                    SCH_TEXT_T,
                                                                    SCH_TEXTBOX_T } ) )
                                || ( E_C::MoreThan( 1 ) && E_C::OnlyTypes( allTextTypes ) );

    auto toHLabelCondition = ( E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_LABEL_T,
                                                                    SCH_DIRECTIVE_LABEL_T,
                                                                    SCH_GLOBAL_LABEL_T,
                                                                    SCH_TEXT_T,
                                                                    SCH_TEXTBOX_T } ) )
                                || ( E_C::MoreThan( 1 ) && E_C::OnlyTypes( allTextTypes ) );

    auto toGLabelCondition = ( E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_LABEL_T,
                                                                    SCH_DIRECTIVE_LABEL_T,
                                                                    SCH_HIER_LABEL_T,
                                                                    SCH_TEXT_T,
                                                                    SCH_TEXTBOX_T } ) )
                                || ( E_C::MoreThan( 1 ) && E_C::OnlyTypes( allTextTypes ) );

    auto toTextCondition = ( E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_LABEL_T,
                                                                  SCH_DIRECTIVE_LABEL_T,
                                                                  SCH_GLOBAL_LABEL_T,
                                                                  SCH_HIER_LABEL_T,
                                                                  SCH_TEXTBOX_T } ) )
                                || ( E_C::MoreThan( 1 ) && E_C::OnlyTypes( allTextTypes ) );

    auto toTextBoxCondition = ( E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_LABEL_T,
                                                                     SCH_DIRECTIVE_LABEL_T,
                                                                     SCH_GLOBAL_LABEL_T,
                                                                     SCH_HIER_LABEL_T,
                                                                     SCH_TEXT_T } ) )
                                   || ( E_C::MoreThan( 1 ) && E_C::OnlyTypes( allTextTypes ) );

    auto entryCondition = E_C::MoreThan( 0 ) && E_C::OnlyTypes( { SCH_BUS_WIRE_ENTRY_T,
                                                                  SCH_BUS_BUS_ENTRY_T} );

    auto singleSheetCondition =  E_C::Count( 1 ) && E_C::OnlyTypes( { SCH_SHEET_T } );

    //
    // Add edit actions to the move tool menu
    //
    if( moveTool )
    {
        CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

        moveMenu.AddSeparator();

        CONDITIONAL_MENU* transformMoveSubMenu = new CONDITIONAL_MENU( moveTool );
        transformMoveSubMenu->SetTitle( _( "Transform Selection" ) );
        moveMenu.AddMenu( transformMoveSubMenu, orientCondition, 200 );

        transformMoveSubMenu->AddItem( EE_ACTIONS::rotateCCW,        orientCondition, 200 );
        transformMoveSubMenu->AddItem( EE_ACTIONS::rotateCW,         orientCondition, 200 );
        transformMoveSubMenu->AddItem( EE_ACTIONS::mirrorV,          orientCondition, 200 );
        transformMoveSubMenu->AddItem( EE_ACTIONS::mirrorH,          orientCondition, 200 );

        {
            CONDITIONAL_MENU *attribMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribMoveSubMenu->SetTitle( _( "Attributes" ) );
            moveMenu.AddMenu( attribMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ), 200 );

            {
                CONDITIONAL_MENU *attribSimMoveSubMenu = new CONDITIONAL_MENU( moveTool );
                attribSimMoveSubMenu->SetTitle( _( "Simulation" ) );
                attribMoveSubMenu->AddMenu( attribSimMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
                attribSimMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromSimulation,
                        E_C::ShowAlways );
                attribSimMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromSimulation,
                        E_C::ShowAlways );
                attribSimMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromSimulation,
                        E_C::ShowAlways );
            }

            {
                CONDITIONAL_MENU *attribBOMMoveSubMenu = new CONDITIONAL_MENU( moveTool );
                attribBOMMoveSubMenu->SetTitle( _( "Bill of Materials" ) );
                attribMoveSubMenu->AddMenu( attribBOMMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
                attribBOMMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromBOM, E_C::ShowAlways );
                attribBOMMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromBOM, E_C::ShowAlways );
                attribBOMMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromBOM, E_C::ShowAlways );
            }

            {
                CONDITIONAL_MENU *attribBoardMoveSubMenu = new CONDITIONAL_MENU( moveTool );
                attribBoardMoveSubMenu->SetTitle( _( "Exclude from board" ) );
                attribMoveSubMenu->AddMenu( attribBoardMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
                attribBoardMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromBoard,
                        E_C::ShowAlways );
                attribBoardMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromBoard,
                        E_C::ShowAlways );
                attribBoardMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromBoard,
                        E_C::ShowAlways );
            }

            {
                CONDITIONAL_MENU *attribDNPMoveSubMenu = new CONDITIONAL_MENU( moveTool );
                attribDNPMoveSubMenu->SetTitle( _( "Do not populate" ) );
                attribMoveSubMenu->AddMenu( attribDNPMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
                attribDNPMoveSubMenu->AddItem( EE_ACTIONS::setDNP, E_C::ShowAlways );
                attribDNPMoveSubMenu->AddItem( EE_ACTIONS::unsetDNP, E_C::ShowAlways );
                attribDNPMoveSubMenu->AddItem( EE_ACTIONS::toggleDNP, E_C::ShowAlways );
            }
        }
        moveMenu.AddItem( EE_ACTIONS::swap,            SELECTION_CONDITIONS::MoreThan( 1 ) );

        moveMenu.AddItem( EE_ACTIONS::properties,      propertiesCondition );

        CONDITIONAL_MENU* editMoveItemSubMenu = new CONDITIONAL_MENU(moveTool);
        editMoveItemSubMenu->SetTitle( _( "Edit Main Fields" ) );
        editMoveItemSubMenu->SetIcon( BITMAPS::right );
        moveMenu.AddMenu( editMoveItemSubMenu, E_C::SingleSymbol );

        editMoveItemSubMenu->AddItem( EE_ACTIONS::editReference,   E_C::SingleSymbol );
        editMoveItemSubMenu->AddItem( EE_ACTIONS::editValue,       E_C::SingleSymbol );
        editMoveItemSubMenu->AddItem( EE_ACTIONS::editFootprint,   E_C::SingleSymbol );

        std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu = std::make_shared<SYMBOL_UNIT_MENU>();
        symUnitMenu->SetTool( this );
        m_menu.RegisterSubMenu( symUnitMenu );
        moveMenu.AddMenu( symUnitMenu.get(), E_C::SingleMultiUnitSymbol, 1 );

        moveMenu.AddSeparator();
        moveMenu.AddItem( ACTIONS::cut,                E_C::IdleSelection );
        moveMenu.AddItem( ACTIONS::copy,               E_C::IdleSelection );
        moveMenu.AddItem( ACTIONS::doDelete,           E_C::NotEmpty );
        moveMenu.AddItem( ACTIONS::duplicate,          duplicateCondition );
    }

    //
    // Add editing actions to the drawing tool menu
    //
    CONDITIONAL_MENU& drawMenu = drawingTools->GetToolMenu().GetMenu();

    drawMenu.AddItem( EE_ACTIONS::clearHighlight,   haveHighlight && EE_CONDITIONS::Idle, 1 );
    drawMenu.AddSeparator(                          haveHighlight && EE_CONDITIONS::Idle, 1 );

    drawMenu.AddItem( EE_ACTIONS::enterSheet,       sheetSelection && EE_CONDITIONS::Idle, 1 );
    drawMenu.AddSeparator(                          sheetSelection && EE_CONDITIONS::Idle, 1 );

    CONDITIONAL_MENU* transformDrawSubMenu = new CONDITIONAL_MENU( drawingTools );
    transformDrawSubMenu->SetTitle( _( "Transform Selection" ) );
    drawMenu.AddMenu( transformDrawSubMenu, orientCondition, 200 );

    transformDrawSubMenu->AddItem( EE_ACTIONS::rotateCCW,        orientCondition, 200 );
    transformDrawSubMenu->AddItem( EE_ACTIONS::rotateCW,         orientCondition, 200 );
    transformDrawSubMenu->AddItem( EE_ACTIONS::mirrorV,          orientCondition, 200 );
    transformDrawSubMenu->AddItem( EE_ACTIONS::mirrorH,          orientCondition, 200 );

    {
        CONDITIONAL_MENU *attribMoveSubMenu = new CONDITIONAL_MENU( moveTool );
        attribMoveSubMenu->SetTitle( _( "Attributes" ) );
        drawMenu.AddMenu( attribMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ), 200 );

        {
            CONDITIONAL_MENU *attribSimMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribSimMoveSubMenu->SetTitle( _( "Simulation" ) );
            attribMoveSubMenu->AddMenu( attribSimMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribSimMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromSimulation, E_C::ShowAlways );
            attribSimMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromSimulation,
                    E_C::ShowAlways );
            attribSimMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromSimulation,
                    E_C::ShowAlways );
        }

        {
            CONDITIONAL_MENU *attribBOMMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribBOMMoveSubMenu->SetTitle( _( "Bill of Materials" ) );
            attribMoveSubMenu->AddMenu( attribBOMMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribBOMMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromBOM, E_C::ShowAlways );
            attribBOMMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromBOM, E_C::ShowAlways );
            attribBOMMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromBOM, E_C::ShowAlways );
        }

        {
            CONDITIONAL_MENU *attribBoardMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribBoardMoveSubMenu->SetTitle( _( "Exclude from board" ) );
            attribMoveSubMenu->AddMenu( attribBoardMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribBoardMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromBoard, E_C::ShowAlways );
            attribBoardMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromBoard, E_C::ShowAlways );
            attribBoardMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromBoard, E_C::ShowAlways );
        }

        {
            CONDITIONAL_MENU *attribDNPMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribDNPMoveSubMenu->SetTitle( _( "Do not populate" ) );
            attribMoveSubMenu->AddMenu( attribDNPMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribDNPMoveSubMenu->AddItem( EE_ACTIONS::setDNP, E_C::ShowAlways );
            attribDNPMoveSubMenu->AddItem( EE_ACTIONS::unsetDNP, E_C::ShowAlways );
            attribDNPMoveSubMenu->AddItem( EE_ACTIONS::toggleDNP, E_C::ShowAlways );
        }
    }

    drawMenu.AddItem( EE_ACTIONS::properties,       propertiesCondition, 200 );

    CONDITIONAL_MENU* editDrawItemSubMenu = new CONDITIONAL_MENU( drawingTools );
    editDrawItemSubMenu->SetTitle( _( "Edit Main Fields" ) );
    editDrawItemSubMenu->SetIcon( BITMAPS::right );
    drawMenu.AddMenu( editDrawItemSubMenu, E_C::SingleSymbol, 200 );

    editDrawItemSubMenu->AddItem( EE_ACTIONS::editReference,    E_C::SingleSymbol, 200 );
    editDrawItemSubMenu->AddItem( EE_ACTIONS::editValue,        E_C::SingleSymbol, 200 );
    editDrawItemSubMenu->AddItem( EE_ACTIONS::editFootprint,    E_C::SingleSymbol, 200 );


    drawMenu.AddItem( EE_ACTIONS::autoplaceFields,              autoplaceCondition, 200 );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu2 = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu2->SetTool( drawingTools );
    drawingTools->GetToolMenu().RegisterSubMenu( symUnitMenu2 );
    drawMenu.AddMenu( symUnitMenu2.get(), E_C::SingleMultiUnitSymbol, 1 );

    drawMenu.AddItem( EE_ACTIONS::editWithLibEdit,  E_C::SingleSymbolOrPower && E_C::Idle, 200 );

    drawMenu.AddItem( EE_ACTIONS::toLabel,          anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::toHLabel,         anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::toGLabel,         anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::toText,           anyTextTool && E_C::Idle, 200 );
    drawMenu.AddItem( EE_ACTIONS::toTextBox,        anyTextTool && E_C::Idle, 200 );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    CONDITIONAL_MENU* transformSelSubMenu = new CONDITIONAL_MENU( m_selectionTool );
    transformSelSubMenu->SetTitle( _( "Transform Selection" ) );
    selToolMenu.AddMenu( transformSelSubMenu, orientCondition, 200 );

    transformSelSubMenu->AddItem( EE_ACTIONS::rotateCCW,        orientCondition, 200 );
    transformSelSubMenu->AddItem( EE_ACTIONS::rotateCW,         orientCondition, 200 );
    transformSelSubMenu->AddItem( EE_ACTIONS::mirrorV,          orientCondition, 200 );
    transformSelSubMenu->AddItem( EE_ACTIONS::mirrorH,          orientCondition, 200 );

    {
        CONDITIONAL_MENU *attribMoveSubMenu = new CONDITIONAL_MENU( moveTool );
        attribMoveSubMenu->SetTitle( _( "Attributes" ) );
        selToolMenu.AddMenu( attribMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ), 200 );

        {
            CONDITIONAL_MENU *attribSimMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribSimMoveSubMenu->SetTitle( _( "Simulation" ) );
            attribMoveSubMenu->AddMenu( attribSimMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribSimMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromSimulation, E_C::ShowAlways );
            attribSimMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromSimulation,
                    E_C::ShowAlways );
            attribSimMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromSimulation,
                    E_C::ShowAlways );
        }

        {
            CONDITIONAL_MENU *attribBOMMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribBOMMoveSubMenu->SetTitle( _( "Bill of Materials" ) );
            attribMoveSubMenu->AddMenu( attribBOMMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribBOMMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromBOM, E_C::ShowAlways );
            attribBOMMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromBOM, E_C::ShowAlways );
            attribBOMMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromBOM, E_C::ShowAlways );
        }

        {
            CONDITIONAL_MENU *attribBoardMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribBoardMoveSubMenu->SetTitle( _( "Exclude from board" ) );
            attribMoveSubMenu->AddMenu( attribBoardMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribBoardMoveSubMenu->AddItem( EE_ACTIONS::setExcludeFromBoard, E_C::ShowAlways );
            attribBoardMoveSubMenu->AddItem( EE_ACTIONS::unsetExcludeFromBoard, E_C::ShowAlways );
            attribBoardMoveSubMenu->AddItem( EE_ACTIONS::toggleExcludeFromBoard, E_C::ShowAlways );
        }

        {
            CONDITIONAL_MENU *attribDNPMoveSubMenu = new CONDITIONAL_MENU( moveTool );
            attribDNPMoveSubMenu->SetTitle( _( "Do not populate" ) );
            attribMoveSubMenu->AddMenu( attribDNPMoveSubMenu, E_C::HasType( SCH_SYMBOL_T ) );
            attribDNPMoveSubMenu->AddItem( EE_ACTIONS::setDNP, E_C::ShowAlways );
            attribDNPMoveSubMenu->AddItem( EE_ACTIONS::unsetDNP, E_C::ShowAlways );
            attribDNPMoveSubMenu->AddItem( EE_ACTIONS::toggleDNP, E_C::ShowAlways );
        }
    }

    selToolMenu.AddItem( EE_ACTIONS::swap,             SELECTION_CONDITIONS::MoreThan( 1 ) );
    selToolMenu.AddItem( EE_ACTIONS::properties,       propertiesCondition, 200 );

    CONDITIONAL_MENU* editSelItemSubMenu = new CONDITIONAL_MENU( m_selectionTool );
    editSelItemSubMenu->SetTitle( _( "Edit Main Fields" ) );
    editSelItemSubMenu->SetIcon( BITMAPS::right );
    selToolMenu.AddMenu( editSelItemSubMenu, E_C::SingleSymbol, 200 );

    editSelItemSubMenu->AddItem( EE_ACTIONS::editReference,    E_C::SingleSymbol, 200 );
    editSelItemSubMenu->AddItem( EE_ACTIONS::editValue,        E_C::SingleSymbol, 200 );
    editSelItemSubMenu->AddItem( EE_ACTIONS::editFootprint,    E_C::SingleSymbol, 200 );

    selToolMenu.AddItem( EE_ACTIONS::autoplaceFields,          autoplaceCondition, 200 );

    std::shared_ptr<SYMBOL_UNIT_MENU> symUnitMenu3 = std::make_shared<SYMBOL_UNIT_MENU>();
    symUnitMenu3->SetTool( m_selectionTool );
    m_selectionTool->GetToolMenu().RegisterSubMenu( symUnitMenu3 );
    selToolMenu.AddMenu( symUnitMenu3.get(), E_C::SingleMultiUnitSymbol, 1 );

    selToolMenu.AddItem( EE_ACTIONS::editWithLibEdit,  E_C::SingleSymbolOrPower && E_C::Idle, 200 );
    selToolMenu.AddItem( EE_ACTIONS::changeSymbol,     E_C::SingleSymbolOrPower, 200 );
    selToolMenu.AddItem( EE_ACTIONS::updateSymbol,     E_C::SingleSymbolOrPower, 200 );
    selToolMenu.AddItem( EE_ACTIONS::changeSymbols,    E_C::MultipleSymbolsOrPower, 200 );
    selToolMenu.AddItem( EE_ACTIONS::updateSymbols,    E_C::MultipleSymbolsOrPower, 200 );

    CONDITIONAL_MENU* convertToSubMenu = new CONDITIONAL_MENU( m_selectionTool );
    convertToSubMenu->SetTitle( _( "Change To" ) );
    convertToSubMenu->SetIcon( BITMAPS::right );
    selToolMenu.AddMenu( convertToSubMenu,             toChangeCondition, 200 );

    convertToSubMenu->AddItem( EE_ACTIONS::toLabel,    toLabelCondition, 200 );
    convertToSubMenu->AddItem( EE_ACTIONS::toCLabel,   toCLabelCondition, 200 );
    convertToSubMenu->AddItem( EE_ACTIONS::toHLabel,   toHLabelCondition, 200 );
    convertToSubMenu->AddItem( EE_ACTIONS::toGLabel,   toGLabelCondition, 200 );
    convertToSubMenu->AddItem( EE_ACTIONS::toText,     toTextCondition, 200 );
    convertToSubMenu->AddItem( EE_ACTIONS::toTextBox,  toTextBoxCondition, 200 );

    selToolMenu.AddItem( EE_ACTIONS::cleanupSheetPins, sheetHasUndefinedPins, 250 );

    selToolMenu.AddSeparator( 300 );
    selToolMenu.AddItem( ACTIONS::cut,                 E_C::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copy,                E_C::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::paste,               E_C::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::pasteSpecial,        E_C::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::doDelete,            E_C::NotEmpty, 300 );
    selToolMenu.AddItem( ACTIONS::duplicate,           duplicateCondition, 300 );

    selToolMenu.AddSeparator( 400 );
    selToolMenu.AddItem( ACTIONS::selectAll,           hasElements, 400 );

    return true;
}


const std::vector<KICAD_T> SCH_EDIT_TOOL::RotatableItems = {
    SCH_SHAPE_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_BITMAP_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_LINE_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T
};


int SCH_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    bool          clockwise = ( aEvent.Matches( EE_ACTIONS::rotateCW.MakeEvent() ) );
    EE_SELECTION& selection = m_selectionTool->RequestSelection( RotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    SCH_ITEM*   head = nullptr;
    int         principalItemCount = 0;  // User-selected items (as opposed to connected wires)
    VECTOR2I    rotPoint;
    bool        moving = false;
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

        if( item->HasFlag( SELECTED_BY_DRAG ) )
            continue;

        principalItemCount++;

        if( !head )
            head = item;
    }

    if( head && head->IsMoving() )
        moving = true;

    if( principalItemCount == 1 )
    {
        if( moving && selection.HasReferencePoint() )
            rotPoint = selection.GetReferencePoint();
        else if( head->IsConnectable() )
            rotPoint = head->GetPosition();
        else
            rotPoint = m_frame->GetNearestHalfGridPosition( head->GetBoundingBox().GetCenter() );

        if( !moving )
            commit->Modify( head, m_frame->GetScreen() );

        switch( head->Type() )
        {
        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( head );

            for( int i = 0; clockwise ? i < 3 : i < 1; ++i )
                symbol->Rotate( rotPoint );

            if( m_frame->eeconfig()->m_AutoplaceFields.enable )
                symbol->AutoAutoplaceFields( m_frame->GetScreen() );

            break;
        }

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
        {
            SCH_TEXT* textItem = static_cast<SCH_TEXT*>( head );
            textItem->Rotate90( clockwise );
            break;
        }

        case SCH_SHEET_PIN_T:
        {
            // Rotate pin within parent sheet
            SCH_SHEET_PIN* pin   = static_cast<SCH_SHEET_PIN*>( head );
            SCH_SHEET*     sheet = pin->GetParent();

            for( int i = 0; clockwise ? i < 3 : i < 1; ++i )
                pin->Rotate( sheet->GetBodyBoundingBox().GetCenter() );

            break;
        }

        case SCH_LINE_T:
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( head );

            // Equal checks for both and neither. We need this because on undo
            // the item will have both flags cleared, but will be selected, so it is possible
            // for the user to get a selected line with neither endpoint selected. We
            // set flags to make sure Rotate() works when we call it.
            if( line->HasFlag( STARTPOINT ) == line->HasFlag( ENDPOINT ) )
            {
                line->SetFlags( STARTPOINT | ENDPOINT );

                // When we allow off grid items, the rotPoint should be set to the midpoint
                // of the line to allow rotation around the center, and the next if
                // should become an else-if
            }

            if( line->HasFlag( STARTPOINT ) )
                rotPoint = line->GetEndPoint();
            else if( line->HasFlag( ENDPOINT ) )
                rotPoint = line->GetStartPoint();
        }

            KI_FALLTHROUGH;
        case SCH_JUNCTION_T:
        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T:
            for( int i = 0; clockwise ? i < 3 : i < 1; ++i )
                head->Rotate( rotPoint );

            break;

        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( head );

            if( field->GetTextAngle().IsHorizontal() )
                field->SetTextAngle( ANGLE_VERTICAL );
            else
                field->SetTextAngle( ANGLE_HORIZONTAL );

            // Now that we're moving a field, they're no longer autoplaced.
            static_cast<SCH_ITEM*>( head->GetParent() )->ClearFieldsAutoplaced();

            break;
        }

        case SCH_SHAPE_T:
        case SCH_TEXTBOX_T:
            for( int i = 0; clockwise ? i < 1 : i < 3; ++i )
                head->Rotate( rotPoint );

            break;

        case SCH_BITMAP_T:
            for( int i = 0; clockwise ? i < 3 : i < 1; ++i )
                head->Rotate( rotPoint );

            // The bitmap is cached in Opengl: clear the cache to redraw
            getView()->RecacheAllItems();
            break;

        case SCH_SHEET_T:
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( head );
            rotPoint = m_frame->GetNearestHalfGridPosition( sheet->GetRotationCenter() );

            // Rotate the sheet on itself. Sheets do not have an anchor point.
            for( int i = 0; clockwise ? i < 3 : i < 1; ++i )
                sheet->Rotate( rotPoint );

            break;
        }

        default:
            UNIMPLEMENTED_FOR( head->GetClass() );
        }

        m_frame->UpdateItem( head, false, true );
    }
    else
    {
        if( moving && selection.HasReferencePoint() )
            rotPoint = selection.GetReferencePoint();
        else
            rotPoint = m_frame->GetNearestHalfGridPosition( selection.GetCenter() );
    }

    for( EDA_ITEM* edaItem : selection )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );

        // We've already rotated the user selected item if there was only one.  We're just
        // here to rotate the ends of wires that were attached to it.
        if( principalItemCount == 1 && !item->HasFlag( SELECTED_BY_DRAG ) )
            continue;

        if( !moving )
            commit->Modify( item, m_frame->GetScreen() );

        for( int i = 0; clockwise ? i < 3 : i < 1; ++i )
        {
            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE* line = (SCH_LINE*) item;

                // If we are rotating more than one item, we do not have start/end
                // points separately selected
                if( item->HasFlag( STARTPOINT ) )
                    line->RotateStart( rotPoint );

                if( item->HasFlag( ENDPOINT ) )
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

                    pin->Rotate( sheet->GetBodyBoundingBox().GetCenter() );
                }
            }
            else if( item->Type() == SCH_FIELD_T )
            {
                if( item->GetParent()->IsSelected() )
                {
                    // parent will rotate us
                }
                else
                {
                    SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

                    field->Rotate( rotPoint );

                    if( field->GetTextAngle().IsHorizontal() )
                        field->SetTextAngle( ANGLE_VERTICAL );
                    else
                        field->SetTextAngle( ANGLE_HORIZONTAL );

                    // Now that we're moving a field, they're no longer autoplaced.
                    static_cast<SCH_ITEM*>( field->GetParent() )->ClearFieldsAutoplaced();
                }
            }
            else
            {
                item->Rotate( rotPoint );
            }
        }

        m_frame->UpdateItem( item, false, true );
        updateItem( item, true );
    }

    if( moving )
    {
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else
    {
        EE_SELECTION selectionCopy = selection;

        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

        SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
        lwbTool->TrimOverLappingWires( commit, &selectionCopy );
        lwbTool->AddJunctionsIfNeeded( commit, &selectionCopy );

        m_frame->SchematicCleanUp( commit );

        if( !localCommit.Empty() )
            localCommit.Push( _( "Rotate" ) );
    }

    return 0;
}


int SCH_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( RotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    bool        vertical = ( aEvent.Matches( EE_ACTIONS::mirrorV.MakeEvent() ) );
    SCH_ITEM*   item = static_cast<SCH_ITEM*>( selection.Front() );
    bool        connections = false;
    bool        moving = item->IsMoving();
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    if( selection.GetSize() == 1 )
    {
        if( !moving )
            commit->Modify( item, m_frame->GetScreen() );

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( vertical )
                symbol->SetOrientation( SYM_MIRROR_X );
            else
                symbol->SetOrientation( SYM_MIRROR_Y );

            symbol->ClearFieldsAutoplaced();
            break;
        }

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
        {
            SCH_TEXT* textItem = static_cast<SCH_TEXT*>( item );
            textItem->MirrorSpinStyle( !vertical );
            break;
        }

        case SCH_SHEET_PIN_T:
        {
            // mirror within parent sheet
            SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( item );
            SCH_SHEET*     sheet = pin->GetParent();

            if( vertical )
                pin->MirrorVertically( sheet->GetBoundingBox().GetCenter().y );
            else
                pin->MirrorHorizontally( sheet->GetBoundingBox().GetCenter().x );

            break;
        }

        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

            if( vertical )
                field->SetVertJustify( TO_VJUSTIFY( -field->GetVertJustify() ) );
            else
                field->SetHorizJustify( TO_HJUSTIFY( -field->GetHorizJustify() ) );

            // Now that we're re-justifying a field, they're no longer autoplaced.
            static_cast<SCH_ITEM*>( field->GetParent() )->ClearFieldsAutoplaced();

            break;
        }

        case SCH_BITMAP_T:
            if( vertical )
                item->MirrorVertically( item->GetPosition().y );
            else
                item->MirrorHorizontally( item->GetPosition().x );

            // The bitmap is cached in Opengl: clear the cache to redraw
            getView()->RecacheAllItems();
            break;

        case SCH_SHEET_T:
        {
            // Mirror the sheet on itself. Sheets do not have a anchor point.
            VECTOR2I mirrorPoint = m_frame->GetNearestHalfGridPosition( item->GetBoundingBox().Centre() );

            if( vertical )
                item->MirrorVertically( mirrorPoint.y );
            else
                item->MirrorHorizontally( mirrorPoint.x );

            break;
        }

        default:
            if( vertical )
                item->MirrorVertically( item->GetPosition().y );
            else
                item->MirrorHorizontally( item->GetPosition().x );

            break;
        }

        connections = item->IsConnectable();
        m_frame->UpdateItem( item, false, true );
    }
    else if( selection.GetSize() > 1 )
    {
        VECTOR2I mirrorPoint = m_frame->GetNearestHalfGridPosition( selection.GetCenter() );

        for( EDA_ITEM* edaItem : selection )
        {
            item = static_cast<SCH_ITEM*>( edaItem );

            if( !moving )
                commit->Modify( item, m_frame->GetScreen() );

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

                    if( vertical )
                        pin->MirrorVertically( sheet->GetBoundingBox().GetCenter().y );
                    else
                        pin->MirrorHorizontally( sheet->GetBoundingBox().GetCenter().x );
                }
            }
            else if( item->Type() == SCH_FIELD_T )
            {
                SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

                if( vertical )
                    field->SetVertJustify( TO_VJUSTIFY( -field->GetVertJustify() ) );
                else
                    field->SetHorizJustify( TO_HJUSTIFY( -field->GetHorizJustify() ) );

                // Now that we're re-justifying a field, they're no longer autoplaced.
                static_cast<SCH_ITEM*>( field->GetParent() )->ClearFieldsAutoplaced();
            }
            else
            {
                if( vertical )
                    item->MirrorVertically( mirrorPoint.y );
                else
                    item->MirrorHorizontally( mirrorPoint.x );
            }

            connections |= item->IsConnectable();
            m_frame->UpdateItem( item, false, true );
        }
    }

    // Update R-Tree for modified items
    for( EDA_ITEM* selected : selection )
        updateItem( selected, true );

    if( item->IsMoving() )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else
    {
        EE_SELECTION selectionCopy = selection;

        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

        if( connections )
        {
            SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
            lwbTool->TrimOverLappingWires( commit, &selectionCopy );
            lwbTool->AddJunctionsIfNeeded( commit, &selectionCopy );

            m_frame->SchematicCleanUp( commit );
        }

        if( !localCommit.Empty() )
            localCommit.Push( _( "Mirror" ) );
    }

    return 0;
}


const std::vector<KICAD_T> swappableItems = {
    SCH_SHAPE_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_LABEL_T,
    SCH_SHEET_PIN_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_T,
    SCH_BITMAP_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T
};


int SCH_EDIT_TOOL::Swap( const TOOL_EVENT& aEvent )
{
    EE_SELECTION&          selection = m_selectionTool->RequestSelection( swappableItems );
    std::vector<EDA_ITEM*> sorted = selection.GetItemsSortedBySelectionOrder();

    // Sheet pins are special, we need to make sure if we have any sheet pins,
    // that we only have sheet pins, and that they have the same parent
    if( selection.CountType( SCH_SHEET_PIN_T ) > 0 )
    {
        if( !selection.OnlyContains( { SCH_SHEET_PIN_T } ) )
            return 0;

        SCH_SHEET_PIN* firstPin = static_cast<SCH_SHEET_PIN*>( selection.Front() );
        SCH_SHEET*     parent = firstPin->GetParent();

        for( EDA_ITEM* item : selection )
        {
            SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( item );

            if( pin->GetParent() != parent )
                return 0;
        }
    }

    if( selection.Size() < 2 )
        return 0;

    bool isMoving    = selection.Front()->IsMoving();
    bool appendUndo  = isMoving;
    bool connections = false;

    SCH_SCREEN* screen = this->m_frame->GetScreen();

    for( size_t i = 0; i < sorted.size() - 1; i++ )
    {
        SCH_ITEM* a = static_cast<SCH_ITEM*>( sorted[i] );
        SCH_ITEM* b = static_cast<SCH_ITEM*>( sorted[( i + 1 ) % sorted.size()] );

        VECTOR2I aPos = a->GetPosition(), bPos = b->GetPosition();
        std::swap( aPos, bPos );

        saveCopyInUndoList( a, UNDO_REDO::CHANGED, appendUndo );
        appendUndo = true;
        saveCopyInUndoList( b, UNDO_REDO::CHANGED, appendUndo );

        // Sheet pins need to have their sides swapped before we change their
        // positions
        if( a->Type() == SCH_SHEET_PIN_T )
        {
            SCH_SHEET_PIN* aPin = static_cast<SCH_SHEET_PIN*>( a );
            SCH_SHEET_PIN* bPin = static_cast<SCH_SHEET_PIN*>( b );
            SHEET_SIDE     aSide = aPin->GetSide(), bSide = bPin->GetSide();
            std::swap( aSide, bSide );
            aPin->SetSide( aSide );
            bPin->SetSide( bSide );
        }

        a->SetPosition( aPos );
        b->SetPosition( bPos );

        if( a->Type() == b->Type() )
        {
            switch( a->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            case SCH_DIRECTIVE_LABEL_T:
                m_frame->AutoRotateItem( screen, a );
                m_frame->AutoRotateItem( screen, b );
                break;
            case SCH_SYMBOL_T:
            {
                SCH_SYMBOL* aSymbol = static_cast<SCH_SYMBOL*>( a );
                SCH_SYMBOL* bSymbol = static_cast<SCH_SYMBOL*>( b );
                int aOrient = aSymbol->GetOrientation(), bOrient = bSymbol->GetOrientation();
                std::swap( aOrient, bOrient );
                aSymbol->SetOrientation( aOrient );
                bSymbol->SetOrientation( bOrient );
                break;
            }
            default: break;
            }
        }

        connections |= a->IsConnectable();
        connections |= b->IsConnectable();
        m_frame->UpdateItem( a, false, true );
        m_frame->UpdateItem( b, false, true );
    }

    // Update R-Tree for modified items
    for( EDA_ITEM* selected : selection )
        updateItem( selected, true );

    if( isMoving )
    {
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

        if( connections )
            m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    return 0;
}


int SCH_EDIT_TOOL::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    const std::vector<std::unique_ptr<SCH_ITEM>>& sourceItems = m_frame->GetRepeatItems();

    if( sourceItems.empty() )
        return 0;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    SCH_COMMIT   commit( m_toolMgr );
    EE_SELECTION newItems;

    for( const std::unique_ptr<SCH_ITEM>& item : sourceItems )
    {
        SCH_ITEM*          newItem = item->Duplicate();
        EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
        bool               restore_state = false;

        if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( newItem ) )
        {
            // If incrementing tries to go below zero, tell user why the value is repeated

            if( !label->IncrementLabel( cfg->m_Drawing.repeat_label_increment ) )
                m_frame->ShowInfoBarWarning( _( "Label value cannot go below zero" ), true );
        }

        // If cloning a symbol then put into 'move' mode.
        if( newItem->Type() == SCH_SYMBOL_T )
        {
            VECTOR2I cursorPos = getViewControls()->GetCursorPosition( true );
            newItem->Move( cursorPos - newItem->GetPosition() );
        }
        else
        {
            newItem->Move( VECTOR2I( schIUScale.MilsToIU( cfg->m_Drawing.default_repeat_offset_x ),
                                     schIUScale.MilsToIU( cfg->m_Drawing.default_repeat_offset_y ) ) );
        }

        m_toolMgr->RunAction<EDA_ITEM*>( EE_ACTIONS::addItemToSel, newItem );
        newItem->SetFlags( IS_NEW );
        m_frame->AddToScreen( newItem, m_frame->GetScreen() );
        commit.Added( newItem, m_frame->GetScreen() );

        if( newItem->Type() == SCH_SYMBOL_T )
        {
            EESCHEMA_SETTINGS::PANEL_ANNOTATE& annotate = m_frame->eeconfig()->m_AnnotatePanel;
            SCHEMATIC_SETTINGS&                projSettings = m_frame->Schematic().Settings();
            int                                annotateStartNum = projSettings.m_AnnotateStartNum;

            if( annotate.automatic )
            {
                static_cast<SCH_SYMBOL*>( newItem )->ClearAnnotation( nullptr, false );
                NULL_REPORTER reporter;
                m_frame->AnnotateSymbols( &commit, ANNOTATE_SELECTION,
                                          (ANNOTATE_ORDER_T) annotate.sort_order,
                                          (ANNOTATE_ALGO_T) annotate.method, true /* recursive */,
                                          annotateStartNum, false, false, reporter );
            }

            restore_state = !m_toolMgr->RunSynchronousAction( EE_ACTIONS::move, &commit );
        }

        if( restore_state )
        {
            commit.Revert();
        }
        else
        {
            newItems.Add( newItem );

            SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
            lwbTool->TrimOverLappingWires( &commit, &newItems );
            lwbTool->AddJunctionsIfNeeded( &commit, &newItems );

            m_frame->SchematicCleanUp( &commit );
            commit.Push( _( "Repeat Item" ) );
        }

    }

    if( !newItems.Empty() )
        m_frame->SaveCopyForRepeatItem( static_cast<SCH_ITEM*>( newItems[0] ) );

    for( size_t ii = 1; ii < newItems.GetSize(); ++ii )
        m_frame->AddCopyForRepeatItem( static_cast<SCH_ITEM*>( newItems[ii] ) );

    return 0;
}


static std::vector<KICAD_T> deletableItems =
{
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_LINE_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_SHAPE_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_NO_CONNECT_T,
    SCH_SHEET_T,
    SCH_SHEET_PIN_T,
    SCH_SYMBOL_T,
    SCH_FIELD_T, // Will be hidden
    SCH_BITMAP_T
};


int SCH_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*           screen = m_frame->GetScreen();
    std::deque<EDA_ITEM*> items = m_selectionTool->RequestSelection( deletableItems ).GetItems();
    SCH_COMMIT            commit( m_toolMgr );
    std::vector<VECTOR2I> pts;
    bool                  updateHierarchy = false;

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    for( EDA_ITEM* item : items )
        item->ClearFlags( STRUCT_DELETED );

    for( EDA_ITEM* item : items )
    {
        SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item );

        if( !sch_item )
            continue;

        if( sch_item->IsConnectable() )
        {
            std::vector<VECTOR2I> tmp_pts = sch_item->GetConnectionPoints();
            pts.insert( pts.end(), tmp_pts.begin(), tmp_pts.end() );
        }

        if( sch_item->Type() == SCH_JUNCTION_T )
        {
            sch_item->SetFlags( STRUCT_DELETED );
            // clean up junctions at the end
        }
        else if( sch_item->Type() == SCH_SHEET_PIN_T )
        {
            SCH_SHEET_PIN* pin = (SCH_SHEET_PIN*) sch_item;
            SCH_SHEET*     sheet = pin->GetParent();

            if( !alg::contains( items, sheet ) )
            {
                commit.Modify( sheet, m_frame->GetScreen() );
                sheet->RemovePin( pin );
            }
        }
        else if( sch_item->Type() == SCH_FIELD_T )
        {
            commit.Modify( item, m_frame->GetScreen() );
            static_cast<SCH_FIELD*>( sch_item )->SetVisible( false );
        }
        else
        {
            sch_item->SetFlags( STRUCT_DELETED );
            commit.Remove( item, m_frame->GetScreen() );
            updateHierarchy |= ( sch_item->Type() == SCH_SHEET_T );
        }
    }

    for( const VECTOR2I& point : pts )
    {
        SCH_ITEM* junction = screen->GetItem( point, 0, SCH_JUNCTION_T );

        if( !junction )
            continue;

        if( junction->HasFlag( STRUCT_DELETED ) || !screen->IsExplicitJunction( point ) )
            m_frame->DeleteJunction( &commit, junction );
    }

    commit.Push( _( "Delete" ) );

    if( updateHierarchy )
        m_frame->UpdateHierarchyNavigator();

    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int SCH_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );
    m_pickerItem = nullptr;

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::REMOVE );
    picker->SetSnapping( false );

    picker->SetClickHandler(
            [this]( const VECTOR2D& aPosition ) -> bool
            {
                if( m_pickerItem )
                {
                    EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
                    selectionTool->UnbrightenItem( m_pickerItem );
                    selectionTool->AddItemToSel( m_pickerItem, true /*quiet mode*/ );
                    m_toolMgr->RunAction( ACTIONS::doDelete );
                    m_pickerItem = nullptr;
                }

                return true;
            } );

    picker->SetMotionHandler(
            [this]( const VECTOR2D& aPos )
            {
                EE_COLLECTOR collector;
                collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
                collector.Collect( m_frame->GetScreen(), deletableItems, aPos );

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
            [this]( const int& aFinalState )
            {
                if( m_pickerItem )
                    m_toolMgr->GetTool<EE_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

                // Wake the selection tool after exiting to ensure the cursor gets updated
                m_toolMgr->PostAction( EE_ACTIONS::selectionActivate );
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


void SCH_EDIT_TOOL::editFieldText( SCH_FIELD* aField )
{
    KICAD_T    parentType = aField->GetParent() ? aField->GetParent()->Type() : SCHEMATIC_T;
    SCH_COMMIT commit( m_toolMgr );

    // Save old symbol in undo list if not already in edit, or moving.
    if( aField->GetEditFlags() == 0 )    // i.e. not edited, or moved
        commit.Modify( aField, m_frame->GetScreen() );

    if( parentType == SCH_SYMBOL_T && aField->GetId() == REFERENCE_FIELD )
        static_cast<SCH_ITEM*>( aField->GetParent() )->SetConnectivityDirty();

    wxString caption;

    // Use title caps for mandatory fields.  "Edit Sheet name Field" looks dorky.
    if( parentType == SCH_SYMBOL_T && aField->GetId() >= 0 && aField->GetId() < MANDATORY_FIELDS )
    {
        wxString translated_fieldname;
        translated_fieldname = TEMPLATE_FIELDNAME::GetDefaultFieldName( aField->GetId(),
                                                                        DO_TRANSLATE );
        caption.Printf( _( "Edit %s Field" ), TitleCaps( translated_fieldname ) );
    }
    else if( parentType == SCH_SHEET_T && aField->GetId() < SHEET_MANDATORY_FIELDS )
        caption.Printf( _( "Edit %s Field" ), TitleCaps( aField->GetName() ) );
    else
        caption.Printf( _( "Edit '%s' Field" ), aField->GetName() );

    DIALOG_SCH_FIELD_PROPERTIES dlg( m_frame, caption, aField );

    // The footprint field dialog can invoke a KIWAY_PLAYER so we must use a quasi-modal
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    dlg.UpdateField( &commit, aField, &m_frame->GetCurrentSheet() );

    if( m_frame->eeconfig()->m_AutoplaceFields.enable || parentType == SCH_SHEET_T )
        static_cast<SCH_ITEM*>( aField->GetParent() )->AutoAutoplaceFields( m_frame->GetScreen() );

    if( !commit.Empty() )
        commit.Push( caption, SKIP_CONNECTIVITY );
}


int SCH_EDIT_TOOL::EditField( const TOOL_EVENT& aEvent )
{
    EE_SELECTION sel;

    if( aEvent.IsAction( &EE_ACTIONS::editReference ) )
        sel = m_selectionTool->RequestSelection( { SCH_FIELD_LOCATE_REFERENCE_T, SCH_SYMBOL_T } );
    else if( aEvent.IsAction( &EE_ACTIONS::editValue ) )
        sel = m_selectionTool->RequestSelection( { SCH_FIELD_LOCATE_VALUE_T, SCH_SYMBOL_T } );
    else if( aEvent.IsAction( &EE_ACTIONS::editFootprint ) )
        sel = m_selectionTool->RequestSelection( { SCH_FIELD_LOCATE_FOOTPRINT_T, SCH_SYMBOL_T } );

    if( sel.Size() != 1 )
        return 0;

    bool      clearSelection = sel.IsHover();
    EDA_ITEM* item = sel.Front();

    if( item->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( aEvent.IsAction( &EE_ACTIONS::editReference ) )
            editFieldText( symbol->GetField( REFERENCE_FIELD ) );
        else if( aEvent.IsAction( &EE_ACTIONS::editValue ) )
            editFieldText( symbol->GetField( VALUE_FIELD ) );
        else if( aEvent.IsAction( &EE_ACTIONS::editFootprint ) )
            editFieldText( symbol->GetField( FOOTPRINT_FIELD ) );
    }
    else if( item->Type() == SCH_FIELD_T )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

        editFieldText( field );

        if( !field->IsVisible() )
            clearSelection = true;
    }

    if( clearSelection )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    return 0;
}


int SCH_EDIT_TOOL::AutoplaceFields( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( RotatableItems );
    SCH_COMMIT    commit( m_toolMgr );
    SCH_ITEM*     head = static_cast<SCH_ITEM*>( selection.Front() );
    bool          moving = head && head->IsMoving();

    if( selection.Empty() )
        return 0;

    std::vector<SCH_ITEM*> autoplaceItems;

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

        if( item->IsType( EE_COLLECTOR::FieldOwners ) )
            autoplaceItems.push_back( item );
        else if( item->GetParent() && item->GetParent()->IsType( EE_COLLECTOR::FieldOwners ) )
            autoplaceItems.push_back( static_cast<SCH_ITEM*>( item->GetParent() ) );
    }

    for( SCH_ITEM* sch_item : autoplaceItems )
    {
        if( !moving && !sch_item->IsNew() )
            commit.Modify( sch_item, m_frame->GetScreen() );

        sch_item->AutoplaceFields( m_frame->GetScreen(), /* aManual */ true );

        updateItem( sch_item, true );
    }

    if( moving )
    {
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( !commit.Empty() )
            commit.Push( _( "Autoplace Fields" ) );

        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection );
    }

    return 0;
}


int SCH_EDIT_TOOL::ChangeSymbols( const TOOL_EVENT& aEvent )
{
    SCH_SYMBOL* selectedSymbol = nullptr;
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

    if( !selection.Empty() )
        selectedSymbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );

    DIALOG_CHANGE_SYMBOLS::MODE mode = DIALOG_CHANGE_SYMBOLS::MODE::UPDATE;

    if( aEvent.IsAction( &EE_ACTIONS::changeSymbol )
            || aEvent.IsAction( &EE_ACTIONS::changeSymbols ) )
    {
        mode = DIALOG_CHANGE_SYMBOLS::MODE::CHANGE;
    }

    DIALOG_CHANGE_SYMBOLS dlg( m_frame, selectedSymbol, mode );

    dlg.ShowQuasiModal();

    return 0;
}


int SCH_EDIT_TOOL::ConvertDeMorgan( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

    if( selection.Empty() )
        return 0;

    SCH_SYMBOL* symbol = (SCH_SYMBOL*) selection.Front();

    if( aEvent.IsAction( &EE_ACTIONS::showDeMorganStandard )
            && symbol->GetConvert() == LIB_ITEM::LIB_CONVERT::BASE )
    {
        return 0;
    }

    if( aEvent.IsAction( &EE_ACTIONS::showDeMorganAlternate )
            && symbol->GetConvert() != LIB_ITEM::LIB_CONVERT::DEMORGAN )
    {
        return 0;
    }

    if( !symbol->IsNew() )
        saveCopyInUndoList( symbol, UNDO_REDO::CHANGED );

    m_frame->ConvertPart( symbol );

    if( symbol->IsNew() )
        m_toolMgr->PostAction( ACTIONS::refreshPreview );

    if( selection.IsHover() )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    return 0;
}


int SCH_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::EditableItems );
    bool          clearSelection = selection.IsHover();

    if( selection.Empty() )
    {
        if( getView()->IsLayerVisible( LAYER_SCHEMATIC_DRAWINGSHEET ) )
        {
            DS_PROXY_VIEW_ITEM* ds = m_frame->GetCanvas()->GetView()->GetDrawingSheet();
            VECTOR2D            cursorPos = getViewControls()->GetCursorPosition( false );

            if( ds && ds->HitTestDrawingSheetItems( getView(), cursorPos ) )
                m_toolMgr->PostAction( ACTIONS::pageSettings );
        }

        return 0;
    }

    EDA_ITEM* curr_item = selection.Front();

    switch( curr_item->Type() )
    {
    case SCH_LINE_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_JUNCTION_T:
        break;

    default:
        if( selection.Size() > 1 )
            return 0;

        break;
    }

    switch( curr_item->Type() )
    {
    case SCH_SYMBOL_T:
    {
        int         retval;
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( curr_item );

        // This needs to be scoped so the dialog destructor removes blocking status
        // before we launch the next dialog.
        {
            DIALOG_SYMBOL_PROPERTIES symbolPropsDialog( m_frame, symbol );

            // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
            // frame. Therefore this dialog as a modal frame parent, MUST be run under
            // quasimodal mode for the quasimodal frame support to work.  So don't use
            // the QUASIMODAL macros here.
            retval = symbolPropsDialog.ShowQuasiModal();
        }

        if( retval == SYMBOL_PROPS_EDIT_OK )
        {
            if( m_frame->eeconfig()->m_AutoplaceFields.enable )
                symbol->AutoAutoplaceFields( m_frame->GetScreen() );

            m_frame->OnModify();
        }
        else if( retval == SYMBOL_PROPS_EDIT_SCHEMATIC_SYMBOL )
        {
            auto editor = (SYMBOL_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH_SYMBOL_EDITOR,
                                                                        true );

            if( wxWindow* blocking_win = editor->Kiway().GetBlockingDialog() )
                blocking_win->Close( true );

            // The broken library symbol link indicator cannot be edited.
            if( symbol->IsMissingLibSymbol() )
                return 0;

            editor->LoadSymbolFromSchematic( symbol );

            editor->Show( true );
            editor->Raise();
        }
        else if( retval == SYMBOL_PROPS_EDIT_LIBRARY_SYMBOL )
        {
            auto editor = (SYMBOL_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH_SYMBOL_EDITOR,
                                                                        true );

            if( wxWindow* blocking_win = editor->Kiway().GetBlockingDialog() )
                blocking_win->Close( true );

            editor->LoadSymbol( symbol->GetLibId(), symbol->GetUnit(), symbol->GetConvert() );

            editor->Show( true );
            editor->Raise();
        }
        else if( retval == SYMBOL_PROPS_WANT_UPDATE_SYMBOL )
        {
            DIALOG_CHANGE_SYMBOLS dlg( m_frame, symbol, DIALOG_CHANGE_SYMBOLS::MODE::UPDATE );
            dlg.ShowQuasiModal();
        }
        else if( retval == SYMBOL_PROPS_WANT_EXCHANGE_SYMBOL )
        {
            DIALOG_CHANGE_SYMBOLS dlg( m_frame, symbol, DIALOG_CHANGE_SYMBOLS::MODE::CHANGE );
            dlg.ShowQuasiModal();
        }
    }
        break;

    case SCH_SHEET_T:
    {
        SCH_SHEET*     sheet = static_cast<SCH_SHEET*>( curr_item );
        bool           doClearAnnotation;
        bool           doRefresh = false;

        // Keep track of existing sheet paths. EditSheet() can modify this list.
        // Note that we use the validity checking/repairing version here just to make sure
        // we've got a valid hierarchy to begin with.
        SCH_SHEET_LIST originalHierarchy( &m_frame->Schematic().Root(), true );

        doRefresh = m_frame->EditSheetProperties( sheet, &m_frame->GetCurrentSheet(),
                                                  &doClearAnnotation );

        // If the sheet file is changed and new sheet contents are loaded then we have to
        // clear the annotations on the new content (as it may have been set from some other
        // sheet path reference)
        if( doClearAnnotation )
        {
            SCH_SCREENS screensList( &m_frame->Schematic().Root() );

            // We clear annotation of new sheet paths here:
            screensList.ClearAnnotationOfNewSheetPaths( originalHierarchy );

            // Clear annotation of g_CurrentSheet itself, because its sheetpath is not a new
            // path, but symbols managed by its sheet path must have their annotation cleared
            // because they are new:
            sheet->GetScreen()->ClearAnnotation( &m_frame->GetCurrentSheet(), false );
        }

        if( doRefresh )
        {
            m_frame->GetCanvas()->Refresh();
            m_frame->UpdateHierarchyNavigator();
        }

        break;
    }

    case SCH_SHEET_PIN_T:
    {
        SCH_SHEET_PIN*              pin = static_cast<SCH_SHEET_PIN*>( curr_item );
        DIALOG_SHEET_PIN_PROPERTIES dlg( m_frame, pin );

        // QuasiModal required for help dialog
        if( dlg.ShowQuasiModal() == wxID_OK )
        {
            m_frame->OnModify();
        }
    }
        break;

    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
    {
        DIALOG_TEXT_PROPERTIES dlg( m_frame, static_cast<SCH_ITEM*>( curr_item ) );

        // QuasiModal required for syntax help and Scintilla auto-complete
        if( dlg.ShowQuasiModal() == wxID_OK )
        {
            m_frame->OnModify();
        }
    }
        break;

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    {
        DIALOG_LABEL_PROPERTIES dlg( m_frame, static_cast<SCH_LABEL_BASE*>( curr_item ) );

        // Must be quasi modal for syntax help
        if( dlg.ShowQuasiModal() == wxID_OK )
        {
            m_frame->OnModify();
        }
    }
        break;

    case SCH_FIELD_T:
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( curr_item );

        editFieldText( field );

        if( !field->IsVisible() )
            clearSelection = true;
    }
        break;

    case SCH_SHAPE_T:
    {
        DIALOG_SHAPE_PROPERTIES dlg( m_frame, static_cast<SCH_SHAPE*>( curr_item ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            m_frame->OnModify();
        }
    }
        break;

    case SCH_BITMAP_T:
    {
        SCH_BITMAP*             bitmap = static_cast<SCH_BITMAP*>( curr_item );
        DIALOG_IMAGE_PROPERTIES dlg( m_frame, bitmap );

        if( dlg.ShowModal() == wxID_OK )
        {
            // The bitmap is cached in Opengl: clear the cache in case it has become invalid
            getView()->RecacheAllItems();
            m_frame->OnModify();
        }
    }
        break;

    case SCH_LINE_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_JUNCTION_T:
        if( std::all_of( selection.Items().begin(), selection.Items().end(),
                [&]( const EDA_ITEM* item )
                {
                    return item->Type() == SCH_LINE_T
                            && static_cast<const SCH_LINE*>( item )->IsGraphicLine();
                } ) )
        {
            std::deque<SCH_LINE*> lines;

            for( EDA_ITEM* selItem : selection.Items() )
                lines.push_back( static_cast<SCH_LINE*>( selItem ) );

            DIALOG_LINE_PROPERTIES dlg( m_frame, lines );

            if( dlg.ShowModal() == wxID_OK )
            {
                m_frame->OnModify();
            }
        }
        else if( std::all_of( selection.Items().begin(), selection.Items().end(),
                [&]( const EDA_ITEM* item )
                {
                    return item->Type() == SCH_JUNCTION_T;
                } ) )
        {
            std::deque<SCH_JUNCTION*> junctions;

            for( EDA_ITEM* selItem : selection.Items() )
                junctions.push_back( static_cast<SCH_JUNCTION*>( selItem ) );

            DIALOG_JUNCTION_PROPS dlg( m_frame, junctions );

            if( dlg.ShowModal() == wxID_OK )
            {
                m_frame->OnModify();
            }
        }
        else if( std::all_of( selection.Items().begin(), selection.Items().end(),
                [&]( const EDA_ITEM* item )
                {
                    const SCH_ITEM* schItem = dynamic_cast<const SCH_ITEM*>( item );

                    wxCHECK( schItem, false );

                    return ( schItem->HasLineStroke() && schItem->IsConnectable() )
                            || item->Type() == SCH_JUNCTION_T;
                } ) )
        {
            std::deque<SCH_ITEM*> items;

            for( EDA_ITEM* selItem : selection.Items() )
                items.push_back( static_cast<SCH_ITEM*>( selItem ) );

            DIALOG_WIRE_BUS_PROPERTIES dlg( m_frame, items );

            if( dlg.ShowModal() == wxID_OK )
            {
                m_frame->OnModify();
            }
        }
        else
        {
            return 0;
        }

        break;

    case SCH_MARKER_T:        // These items have no properties to edit
    case SCH_NO_CONNECT_T:
        break;

    default:                // Unexpected item
        wxFAIL_MSG( wxString( "Cannot edit schematic item type " ) + curr_item->GetClass() );
    }

    updateItem( curr_item, true );

    if( clearSelection )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    return 0;
}


int SCH_EDIT_TOOL::ChangeTextType( const TOOL_EVENT& aEvent )
{
    KICAD_T       convertTo = aEvent.Parameter<KICAD_T>();
    EE_SELECTION  selection = m_selectionTool->RequestSelection( { SCH_LABEL_LOCATE_ANY_T,
                                                                   SCH_TEXT_T,
                                                                   SCH_TEXTBOX_T } );

    for( unsigned int i = 0; i < selection.GetSize(); ++i )
    {
        SCH_ITEM*    item = dynamic_cast<SCH_ITEM*>( selection.GetItem( i ) );

        if( item && item->Type() != convertTo )
        {
            bool             selected    = item->IsSelected();
            SCH_ITEM*        newtext     = nullptr;
            VECTOR2I         position    = item->GetPosition();
            wxString         txt;
            wxString         href;
            TEXT_SPIN_STYLE  orientation = TEXT_SPIN_STYLE::SPIN::RIGHT;
            LABEL_FLAG_SHAPE shape       = LABEL_FLAG_SHAPE::L_UNSPECIFIED;

            switch( item->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            {
                SCH_TEXT* label = static_cast<SCH_LABEL_BASE*>( item );

                txt = UnescapeString( label->GetText() );
                orientation = label->GetTextSpinStyle();
                shape = label->GetShape();
                href = label->GetHyperlink();
                break;
            }

            case SCH_DIRECTIVE_LABEL_T:
            {
                SCH_DIRECTIVE_LABEL* dirlabel = static_cast<SCH_DIRECTIVE_LABEL*>( item );

                // a SCH_DIRECTIVE_LABEL has no text
                txt = _( "<empty>" );

                orientation = dirlabel->GetTextSpinStyle();
                href = dirlabel->GetHyperlink();
                break;
            }

            case SCH_TEXT_T:
            {
                SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

                txt = text->GetText();
                orientation = text->GetTextSpinStyle();
                href = text->GetHyperlink();
                break;
            }

            case SCH_TEXTBOX_T:
            {
                SCH_TEXTBOX* textbox = static_cast<SCH_TEXTBOX*>( item );
                BOX2I        bbox = textbox->GetBoundingBox();

                bbox.Inflate( -textbox->GetTextMargin() );

                if( convertTo == SCH_LABEL_T
                  || convertTo == SCH_HIER_LABEL_T
                  || convertTo == SCH_GLOBAL_LABEL_T )
                {
                    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item );
                    wxCHECK( text, 0 );
                    int textSize = text->GetTextSize().y;
                    bbox.Inflate( item->Schematic()->Settings().m_LabelSizeRatio * textSize );
                }

                txt = textbox->GetText();

                if( textbox->GetTextAngle().IsVertical() )
                {
                    if( textbox->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                    {
                        orientation = TEXT_SPIN_STYLE::SPIN::BOTTOM;
                        position = VECTOR2I( bbox.Centre().x, bbox.GetOrigin().y );
                    }
                    else
                    {
                        orientation = TEXT_SPIN_STYLE::SPIN::UP;
                        position = VECTOR2I( bbox.Centre().x, bbox.GetEnd().y );
                    }
                }
                else
                {
                    if( textbox->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                    {
                        orientation = TEXT_SPIN_STYLE::SPIN::LEFT;
                        position = VECTOR2I( bbox.GetEnd().x, bbox.Centre().y );
                    }
                    else
                    {
                        orientation = TEXT_SPIN_STYLE::SPIN::RIGHT;
                        position = VECTOR2I( bbox.GetOrigin().x, bbox.Centre().y );
                    }
                }

                position = m_frame->GetNearestGridPosition( position );
                href = textbox->GetHyperlink();
                break;
            }

            default:
                UNIMPLEMENTED_FOR( item->GetClass() );
                break;
            }

            auto getValidNetname =
                    []( const wxString& aText )
                {
                    wxString local_txt = aText;
                    local_txt.Replace( "\n", "_" );
                    local_txt.Replace( "\r", "_" );
                    local_txt.Replace( "\t", "_" );

                    // Bus groups can have spaces; bus vectors and signal names cannot
                    if( !NET_SETTINGS::ParseBusGroup( aText, nullptr, nullptr ) )
                        local_txt.Replace( " ", "_" );

                    // label strings are "escaped" i.e. a '/' is replaced by "{slash}"
                    local_txt = EscapeString( local_txt, CTX_NETNAME );

                    if( local_txt.IsEmpty() )
                        return _( "<empty>" );
                    else
                        return local_txt;
                };

            switch( convertTo )
            {
            case SCH_LABEL_T:
            {
                SCH_LABEL_BASE* new_label = new SCH_LABEL( position, getValidNetname( txt ) );

                new_label->SetShape( shape );
                new_label->SetTextSpinStyle( orientation );
                new_label->SetHyperlink( href );
                newtext = new_label;
                break;
            }

            case SCH_GLOBAL_LABEL_T:
            {
                SCH_LABEL_BASE* new_label = new SCH_GLOBALLABEL( position, getValidNetname( txt ) );

                new_label->SetShape( shape );
                new_label->SetTextSpinStyle( orientation );
                new_label->SetHyperlink( href );
                newtext = new_label;
                break;
            }

            case SCH_HIER_LABEL_T:
            {
                SCH_LABEL_BASE* new_label = new SCH_HIERLABEL( position, getValidNetname( txt ) );

                new_label->SetShape( shape );
                new_label->SetTextSpinStyle( orientation );
                new_label->SetHyperlink( href );
                newtext = new_label;
                break;
            }

            case SCH_DIRECTIVE_LABEL_T:
            {
                SCH_LABEL_BASE* new_label = new SCH_DIRECTIVE_LABEL( position );

                // A SCH_DIRECTIVE_LABEL usually has at least one field containing the net class
                // name.  If we're copying from a text object assume the text is the netclass
                // name.  Otherwise, we'll just copy the fields which will either have a netclass
                // or not.
                if( !dynamic_cast<SCH_LABEL_BASE*>( item ) )
                {
                    SCH_FIELD netclass( position, 0, new_label, wxT( "Netclass" ) );
                    netclass.SetText( txt );
                    netclass.SetVisible( true );
                    new_label->GetFields().push_back( netclass );
                }

                new_label->SetShape( LABEL_FLAG_SHAPE::F_ROUND );
                new_label->SetTextSpinStyle( orientation );
                new_label->SetHyperlink( href );
                newtext = new_label;
                break;
            }

            case SCH_TEXT_T:
            {
                SCH_TEXT* new_text = new SCH_TEXT( position, txt );

                new_text->SetTextSpinStyle( orientation );
                new_text->SetHyperlink( href );
                newtext = new_text;
                break;
            }

            case SCH_TEXTBOX_T:
            {
                SCH_TEXTBOX* new_textbox = new SCH_TEXTBOX( 0, FILL_T::NO_FILL, txt );
                BOX2I        bbox = item->GetBoundingBox();

                if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( item ) )
                    bbox.Inflate( -label->GetLabelBoxExpansion() );

                EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( item );
                wxCHECK(textItem, 0 );

                // Careful: GetTextMargin() is dependent on font size...
                new_textbox->SetTextSize( textItem->GetTextSize() );

                int margin = new_textbox->GetTextMargin();
                bbox.Inflate( margin );

                // Add 1/20 of the margin at the end to reduce line-breaking changes.
                int slop = margin / 20;

                switch( orientation )
                {
                case TEXT_SPIN_STYLE::SPIN::RIGHT:
                    new_textbox->SetPosition( bbox.GetPosition() );
                    new_textbox->SetEnd( bbox.GetEnd() + VECTOR2I( slop, 0 ) );
                    break;

                case TEXT_SPIN_STYLE::SPIN::LEFT:
                    new_textbox->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                    new_textbox->SetPosition( bbox.GetPosition() - VECTOR2I( slop, 0 ) );
                    new_textbox->SetEnd( bbox.GetEnd() );
                    break;

                case TEXT_SPIN_STYLE::SPIN::UP:
                    new_textbox->SetTextAngle( ANGLE_VERTICAL );
                    new_textbox->SetPosition( bbox.GetPosition() - VECTOR2I( 0, slop ) );
                    new_textbox->SetEnd( bbox.GetEnd() );
                    break;

                case TEXT_SPIN_STYLE::SPIN::BOTTOM:
                    new_textbox->SetTextAngle( ANGLE_VERTICAL );
                    new_textbox->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                    new_textbox->SetPosition( bbox.GetPosition() );
                    new_textbox->SetEnd( bbox.GetEnd() + VECTOR2I( 0, slop ) );
                    break;
                }

                new_textbox->SetHyperlink( href );
                newtext = new_textbox;
                break;
            }

            default:
                UNIMPLEMENTED_FOR( wxString::Format( "%d.", convertTo ) );
                break;
            }

            wxCHECK2( newtext, continue );

            // Copy the old text item settings to the new one.  Justifications are not copied
            // because they are not used in labels.  Justifications will be set to default value
            // in the new text item type.
            //
            newtext->SetFlags( item->GetEditFlags() );

            EDA_TEXT* eda_text = dynamic_cast<EDA_TEXT*>( item );
            EDA_TEXT* new_eda_text = dynamic_cast<EDA_TEXT*>( newtext );

            wxCHECK2( eda_text && new_eda_text, continue );

            new_eda_text->SetFont( eda_text->GetFont() );
            new_eda_text->SetTextSize( eda_text->GetTextSize() );
            new_eda_text->SetTextThickness( eda_text->GetTextThickness() );
            new_eda_text->SetItalic( eda_text->IsItalic() );
            new_eda_text->SetBold( eda_text->IsBold() );

            newtext->AutoplaceFields( m_frame->GetScreen(), false );

            SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( item );
            SCH_LABEL_BASE* new_label = dynamic_cast<SCH_LABEL_BASE*>( newtext );

            if( label && new_label )
            {
                new_label->AddFields( label->GetFields() );

                // A SCH_GLOBALLABEL has a specific field, that has no meaning for
                // other labels, and expected to be the first field in list.
                // It is the first field in list for this kind of label
                // So remove field named "Intersheetrefs" if exists for other labels
                int min_idx = new_label->Type() == SCH_GLOBAL_LABEL_T ? 1 : 0;
                std::vector<SCH_FIELD>& fields = new_label->GetFields();

                for( int ii = fields.size()-1; ii >= min_idx; ii-- )
                {
                    if( fields[ii].GetCanonicalName() == wxT( "Intersheetrefs" ) )
                        fields.erase( fields.begin() + ii );
                }
            }

            if( selected )
                m_toolMgr->RunAction<EDA_ITEM*>( EE_ACTIONS::removeItemFromSel, item );

            SCH_COMMIT commit( m_toolMgr );

            if( !item->IsNew() )
            {
                m_frame->RemoveFromScreen( item, m_frame->GetScreen() );
                commit.Removed( item, m_frame->GetScreen() );

                m_frame->AddToScreen( newtext, m_frame->GetScreen() );
                commit.Added( newtext, m_frame->GetScreen() );
            }

            commit.Push( _( "Change Item Type" ) );

            if( selected )
                m_toolMgr->RunAction<EDA_ITEM*>( EE_ACTIONS::addItemToSel, newtext );

            // Otherwise, pointer is owned by the undo stack
            if( item->IsNew() )
                delete item;

            if( convertTo == SCH_TEXT_T || convertTo == SCH_TEXTBOX_T )
            {
                if( newtext->IsDangling() )
                    getView()->Update( newtext, KIGFX::REPAINT );
            }
            else
            {
                m_frame->TestDanglingEnds();
            }

            m_frame->OnModify();
        }
    }

    if( selection.IsHover() )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    return 0;
}


int SCH_EDIT_TOOL::BreakWire( const TOOL_EVENT& aEvent )
{
    bool          isSlice   = aEvent.Matches( EE_ACTIONS::slice.MakeEvent() );
    VECTOR2I      cursorPos = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_LINE_T } );
    SCH_SCREEN*   screen = m_frame->GetScreen();
    SCH_COMMIT    commit( m_toolMgr );
    std::vector<SCH_LINE*> lines;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( !line->IsEndPoint( cursorPos ) )
                lines.push_back( line );
        }
    }

    m_selectionTool->ClearSelection();

    for( SCH_LINE* line : lines )
    {
        SCH_LINE* newLine;

        // We let the user select the break point if they're on a single line
        if( lines.size() == 1 && line->HitTest( cursorPos ) )
            m_frame->BreakSegment( &commit, line, cursorPos, &newLine, screen );
        else
            m_frame->BreakSegment( &commit, line, line->GetMidPoint(), &newLine, screen );

        // Make sure both endpoints are deselected
        newLine->ClearFlags();

        m_selectionTool->AddItemToSel( line );
        line->SetFlags( ENDPOINT );

        // If we're a break, we want to drag both wires.
        // Side note: the drag/move tool only checks whether the first item is
        // new to determine if it should append undo or not, someday this should
        // be cleaned up and explictly controlled but for now the newLine
        // selection addition must be after the existing line.
        if( !isSlice )
        {
            m_selectionTool->AddItemToSel( newLine );
            newLine->SetFlags( STARTPOINT );
        }
    }

    if( !lines.empty() )
    {
        m_frame->TestDanglingEnds();

        if( m_toolMgr->RunSynchronousAction( EE_ACTIONS::drag, &commit, isSlice ) )
            commit.Push( isSlice ? _( "Slice Wire" ) : _( "Break Wire" ) );
        else
            commit.Revert();
    }

    return 0;
}


int SCH_EDIT_TOOL::CleanupSheetPins( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SHEET_T } );
    SCH_SHEET*    sheet = (SCH_SHEET*) selection.Front();

    if( !sheet || !sheet->HasUndefinedPins() )
        return 0;

    if( !IsOK( m_frame, _( "Do you wish to delete the unreferenced pins from this sheet?" ) ) )
        return 0;

    saveCopyInUndoList( sheet, UNDO_REDO::CHANGED );

    sheet->CleanupSheet();

    updateItem( sheet, true );
    m_frame->OnModify();

    if( selection.IsHover() )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    return 0;
}


int SCH_EDIT_TOOL::EditPageNumber( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SHEET_T } );

    if( selection.GetSize() > 1 )
        return 0;

    SCH_SHEET* sheet = (SCH_SHEET*) selection.Front();

    SCH_SHEET_PATH instance = m_frame->GetCurrentSheet();

    SCH_SCREEN* screen;

    if( sheet )
    {
        // When changing the page number of a selected sheet, the current screen owns the sheet.
        screen = m_frame->GetScreen();

        instance.push_back( sheet );
    }
    else
    {
        SCH_SHEET_PATH prevInstance = instance;

        // When change the page number in the screen, the previous screen owns the sheet.
        if( prevInstance.size() )
        {
            prevInstance.pop_back();
            screen = prevInstance.LastScreen();
        }
        else
        {
            // The root sheet and root screen are effectively the same thing.
            screen = m_frame->GetScreen();
        }

        sheet = m_frame->GetCurrentSheet().Last();
    }

    wxString msg;
    wxString sheetPath = instance.PathHumanReadable( false );
    wxString pageNumber = instance.GetPageNumber();

    msg.Printf( _( "Enter page number for sheet path%s" ),
                ( sheetPath.Length() > 20 ) ? "\n" + sheetPath : " " + sheetPath );

    wxTextEntryDialog dlg( m_frame, msg, _( "Edit Sheet Page Number" ), pageNumber );

    dlg.SetTextValidator( wxFILTER_ALPHANUMERIC );  // No white space.

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetValue() == instance.GetPageNumber() )
        return 0;

    m_frame->SaveCopyInUndoList( screen, sheet, UNDO_REDO::CHANGED, false );

    instance.SetPageNumber( dlg.GetValue() );

    if( instance == m_frame->GetCurrentSheet() )
    {
        m_frame->GetScreen()->SetPageNumber( dlg.GetValue() );
        m_frame->OnPageSettingsChange();
    }

    m_frame->OnModify();

    return 0;
}


int SCH_EDIT_TOOL::DdAppendFile( const TOOL_EVENT& aEvent )
{
    wxString aFileName = *aEvent.Parameter<wxString*>();
    return ( m_frame->AddSheetAndUpdateDisplay( aFileName ) ? 0 : 1 );
}


int SCH_EDIT_TOOL::SetAttribute( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );
    SCH_COMMIT    commit( m_toolMgr );

    if( selection.Empty() )
        return 0;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            commit.Modify( symbol, m_frame->GetScreen() );

            if( aEvent.IsAction( &EE_ACTIONS::setDNP ) )
                symbol->SetDNP( true );

            if( aEvent.IsAction( &EE_ACTIONS::setExcludeFromSimulation ) )
                symbol->SetExcludeFromSim( true );

            if( aEvent.IsAction( &EE_ACTIONS::setExcludeFromBOM ) )
                symbol->SetExcludedFromBOM( true );

            if( aEvent.IsAction( &EE_ACTIONS::setExcludeFromBoard ) )
                symbol->SetExcludedFromBoard( true );
        }
    }

    commit.Push( _( "Set Attribute" ) );
    return 0;
}


int SCH_EDIT_TOOL::UnsetAttribute( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );
    SCH_COMMIT    commit( m_toolMgr );

    if( selection.Empty() )
        return 0;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            commit.Modify( symbol, m_frame->GetScreen() );

            if( aEvent.IsAction( &EE_ACTIONS::unsetDNP ) )
                symbol->SetDNP( false );

            if( aEvent.IsAction( &EE_ACTIONS::unsetExcludeFromSimulation ) )
                symbol->SetExcludeFromSim( false );

            if( aEvent.IsAction( &EE_ACTIONS::unsetExcludeFromBOM ) )
                symbol->SetExcludedFromBOM( false );

            if( aEvent.IsAction( &EE_ACTIONS::unsetExcludeFromBoard ) )
                symbol->SetExcludedFromBoard( false );
        }
    }

    commit.Push( _( "Clear Attribute" ) );
    return 0;
}


int SCH_EDIT_TOOL::ToggleAttribute( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );
    SCH_COMMIT    commit( m_toolMgr );

    if( selection.Empty() )
        return 0;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            commit.Modify( symbol, m_frame->GetScreen() );

            if( aEvent.IsAction( &EE_ACTIONS::toggleDNP ) )
                symbol->SetDNP( !symbol->GetDNP() );

            if( aEvent.IsAction( &EE_ACTIONS::toggleExcludeFromSimulation ) )
                symbol->SetExcludeFromSim( !symbol->GetExcludeFromSim() );

            if( aEvent.IsAction( &EE_ACTIONS::toggleExcludeFromBOM ) )
                symbol->SetExcludedFromBOM( !symbol->GetExcludedFromBOM() );

            if( aEvent.IsAction( &EE_ACTIONS::toggleExcludeFromBoard ) )
                symbol->SetExcludedFromBoard( !symbol->GetExcludedFromBoard() );
        }
    }

    commit.Push( _( "Toggle Attribute" ) );
    return 0;

}


void SCH_EDIT_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TOOL::RepeatDrawItem,     EE_ACTIONS::repeatDrawItem.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorV.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorH.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Swap,               EE_ACTIONS::swap.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DoDelete,           ACTIONS::doDelete.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DeleteItemCursor,   ACTIONS::deleteTool.MakeEvent() );

    Go( &SCH_EDIT_TOOL::Properties,         EE_ACTIONS::properties.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          EE_ACTIONS::editReference.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          EE_ACTIONS::editValue.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          EE_ACTIONS::editFootprint.MakeEvent() );
    Go( &SCH_EDIT_TOOL::AutoplaceFields,    EE_ACTIONS::autoplaceFields.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      EE_ACTIONS::changeSymbols.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      EE_ACTIONS::updateSymbols.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      EE_ACTIONS::changeSymbol.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      EE_ACTIONS::updateSymbol.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ConvertDeMorgan,    EE_ACTIONS::toggleDeMorgan.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ConvertDeMorgan,    EE_ACTIONS::showDeMorganStandard.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ConvertDeMorgan,    EE_ACTIONS::showDeMorganAlternate.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toHLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toGLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toCLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toText.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     EE_ACTIONS::toTextBox.MakeEvent() );

    Go( &SCH_EDIT_TOOL::BreakWire,          EE_ACTIONS::breakWire.MakeEvent() );
    Go( &SCH_EDIT_TOOL::BreakWire,          EE_ACTIONS::slice.MakeEvent() );

    Go( &SCH_EDIT_TOOL::SetAttribute,       EE_ACTIONS::setDNP.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SetAttribute,       EE_ACTIONS::setExcludeFromBOM.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SetAttribute,       EE_ACTIONS::setExcludeFromBoard.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SetAttribute,       EE_ACTIONS::setExcludeFromSimulation.MakeEvent() );
    Go( &SCH_EDIT_TOOL::UnsetAttribute,     EE_ACTIONS::unsetDNP.MakeEvent() );
    Go( &SCH_EDIT_TOOL::UnsetAttribute,     EE_ACTIONS::unsetExcludeFromBOM.MakeEvent() );
    Go( &SCH_EDIT_TOOL::UnsetAttribute,     EE_ACTIONS::unsetExcludeFromBoard.MakeEvent() );
    Go( &SCH_EDIT_TOOL::UnsetAttribute,     EE_ACTIONS::unsetExcludeFromSimulation.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ToggleAttribute,    EE_ACTIONS::toggleDNP.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ToggleAttribute,    EE_ACTIONS::toggleExcludeFromBOM.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ToggleAttribute,    EE_ACTIONS::toggleExcludeFromBoard.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ToggleAttribute,    EE_ACTIONS::toggleExcludeFromSimulation.MakeEvent() );

    Go( &SCH_EDIT_TOOL::CleanupSheetPins,   EE_ACTIONS::cleanupSheetPins.MakeEvent() );
    Go( &SCH_EDIT_TOOL::GlobalEdit,         EE_ACTIONS::editTextAndGraphics.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditPageNumber,     EE_ACTIONS::editPageNumber.MakeEvent() );

    Go( &SCH_EDIT_TOOL::DdAppendFile,       EE_ACTIONS::ddAppendFile.MakeEvent() );
}
