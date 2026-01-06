/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/action_manager.h>
#include <tool/picker_tool.h>
#include <tools/sch_edit_tool.h>
#include <tools/sch_inspection_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>
#include <tools/sch_drawing_tools.h>
#include <confirm.h>
#include <connection_graph.h>
#include <sch_actions.h>
#include <sch_tool_utils.h>
#include <increment.h>
#include <algorithm>
#include <set>
#include <string_utils.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_commit.h>
#include <sch_group.h>
#include <sch_label.h>
#include <sch_junction.h>
#include <sch_marker.h>
#include <sch_rule_area.h>
#include <sch_pin.h>
#include <sch_sheet_pin.h>
#include <sch_textbox.h>
#include <sch_table.h>
#include <sch_no_connect.h>
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
#include <dialogs/dialog_tablecell_properties.h>
#include <dialogs/dialog_table_properties.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>
#include <core/kicad_algo.h>
#include <view/view_controls.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <project/net_settings.h>
#include <tools/sch_tool_utils.h>


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
    ACTION_MENU* create() const override { return new SYMBOL_UNIT_MENU(); }

private:
    void update() override
    {
        SCH_SELECTION_TOOL* selTool = getToolManager()->GetTool<SCH_SELECTION_TOOL>();
        SCH_SELECTION&      selection = selTool->GetSelection();
        SCH_SYMBOL*         symbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );

        Clear();

        wxCHECK( symbol, /* void */ );

        const int           unit = symbol->GetUnit();
        const int           nUnits = symbol->GetLibSymbolRef()->GetUnitCount();
        const std::set<int> missingUnits = GetUnplacedUnitsForSymbol( *symbol );

        for( int ii = 0; ii < nUnits; ii++ )
        {
            wxString unit_text = symbol->GetUnitDisplayName( ii + 1, false );

            if( missingUnits.count( ii + 1 ) == 0 )
                unit_text += _( " (already placed)" );

            wxMenuItem* item = Append( ID_POPUP_SCH_SELECT_UNIT1 + ii, unit_text, wxEmptyString, wxITEM_CHECK );

            if( unit == ii + 1 )
                item->Check( true );

            // The ID max for these submenus is ID_POPUP_SCH_SELECT_UNIT_END
            // See eeschema_id to modify this value.
            if( ii >= ( ID_POPUP_SCH_SELECT_UNIT_END - ID_POPUP_SCH_SELECT_UNIT1 ) )
                break; // We have used all IDs for these submenus
        }

        if( !missingUnits.empty() )
        {
            AppendSeparator();

            for( int unitNumber : missingUnits )
            {
                wxString placeText =
                        wxString::Format( _( "Place unit %s" ), symbol->GetUnitDisplayName( unitNumber, false ) );
                Append( ID_POPUP_SCH_PLACE_UNIT1 + unitNumber - 1, placeText );
            }
        }
    }
};


class BODY_STYLE_MENU : public ACTION_MENU
{
public:
    BODY_STYLE_MENU() :
            ACTION_MENU( true )
    {
        SetIcon( BITMAPS::body_style );
        SetTitle( _( "Body Style" ) );
    }

protected:
    ACTION_MENU* create() const override { return new BODY_STYLE_MENU(); }

private:
    void update() override
    {
        SCH_SELECTION_TOOL* selTool = getToolManager()->GetTool<SCH_SELECTION_TOOL>();
        SCH_SELECTION&      selection = selTool->GetSelection();
        SCH_SYMBOL*         symbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );
        wxMenuItem*         item;

        Clear();

        wxCHECK( symbol, /* void */ );

        if( symbol->HasDeMorganBodyStyles() )
        {
            item = Append( ID_POPUP_SCH_SELECT_BODY_STYLE, _( "Standard" ), wxEmptyString, wxITEM_CHECK );
            item->Check( symbol->GetBodyStyle() == BODY_STYLE::BASE );

            item = Append( ID_POPUP_SCH_SELECT_BODY_STYLE1, _( "Alternate" ), wxEmptyString, wxITEM_CHECK );
            item->Check( symbol->GetBodyStyle() != BODY_STYLE::BASE );
        }
        else if( symbol->IsMultiBodyStyle() )
        {
            for( int i = 0; i < symbol->GetBodyStyleCount(); i++ )
            {
                item = Append( ID_POPUP_SCH_SELECT_BODY_STYLE + i, symbol->GetBodyStyleDescription( i + 1, true ),
                               wxEmptyString, wxITEM_CHECK );
                item->Check( symbol->GetBodyStyle() == i + 1 );
            }
        }
    }
};


class ALT_PIN_FUNCTION_MENU : public ACTION_MENU
{
public:
    ALT_PIN_FUNCTION_MENU() :
            ACTION_MENU( true )
    {
        SetIcon( BITMAPS::component_select_unit );
        SetTitle( _( "Pin Function" ) );
    }

protected:
    ACTION_MENU* create() const override { return new ALT_PIN_FUNCTION_MENU(); }

private:
    void update() override
    {
        SCH_SELECTION_TOOL* selTool = getToolManager()->GetTool<SCH_SELECTION_TOOL>();
        SCH_SELECTION&      selection = selTool->GetSelection();
        SCH_PIN*            pin = dynamic_cast<SCH_PIN*>( selection.Front() );
        SCH_PIN*            libPin = pin ? pin->GetLibPin() : nullptr;

        Clear();

        wxCHECK( libPin, /* void */ );

        wxMenuItem* item = Append( ID_POPUP_SCH_ALT_PIN_FUNCTION, libPin->GetName(), wxEmptyString, wxITEM_CHECK );

        if( pin->GetAlt().IsEmpty() || ( pin->GetAlt() == libPin->GetName() ) )
            item->Check( true );

        int ii = 1;

        for( const auto& [name, definition] : libPin->GetAlternates() )
        {
            // The default pin name is set above, avoid setting it again.
            if( name == libPin->GetName() )
                continue;

            item = Append( ID_POPUP_SCH_ALT_PIN_FUNCTION + ii, name, wxEmptyString, wxITEM_CHECK );

            if( name == pin->GetAlt() )
                item->Check( true );

            // The ID max for these submenus is ID_POPUP_SCH_ALT_PIN_FUNCTION_END
            // See eeschema_id to modify this value.
            if( ++ii >= ( ID_POPUP_SCH_ALT_PIN_FUNCTION_END - ID_POPUP_SCH_SELECT_UNIT ) )
                break; // We have used all IDs for these submenus
        }
    }
};


class PIN_TRICKS_MENU : public ACTION_MENU
{
public:
    PIN_TRICKS_MENU() :
            ACTION_MENU( true )
    {
        SetIcon( BITMAPS::pin );
        SetTitle( _( "Pin Helpers" ) );
    }

protected:
    ACTION_MENU* create() const override { return new PIN_TRICKS_MENU(); }

private:
    void update() override
    {
        SCH_SELECTION_TOOL* selTool = getToolManager()->GetTool<SCH_SELECTION_TOOL>();
        SCH_SELECTION&      selection = selTool->GetSelection();
        SCH_PIN*            pin = dynamic_cast<SCH_PIN*>( selection.Front() );
        SCH_SHEET_PIN*      sheetPin = dynamic_cast<SCH_SHEET_PIN*>( selection.Front() );

        Clear();

        if( !pin && !sheetPin )
            return;

        Add( _( "Wire" ), ID_POPUP_SCH_PIN_TRICKS_WIRE, BITMAPS::add_line );
        Add( _( "No Connect" ), ID_POPUP_SCH_PIN_TRICKS_NO_CONNECT, BITMAPS::noconn );
        Add( _( "Net Label" ), ID_POPUP_SCH_PIN_TRICKS_NET_LABEL, BITMAPS::add_label );
        Add( _( "Hierarchical Label" ), ID_POPUP_SCH_PIN_TRICKS_HIER_LABEL, BITMAPS::add_hierarchical_label );
        Add( _( "Global Label" ), ID_POPUP_SCH_PIN_TRICKS_GLOBAL_LABEL, BITMAPS::add_glabel );
    }
};


SCH_EDIT_TOOL::SCH_EDIT_TOOL() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveEdit" )
{
    m_pickerItem = nullptr;
}


using S_C = SCH_CONDITIONS;

bool SCH_EDIT_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    SCH_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<SCH_DRAWING_TOOLS>();
    SCH_MOVE_TOOL*     moveTool = m_toolMgr->GetTool<SCH_MOVE_TOOL>();

    wxASSERT_MSG( drawingTools, "eeshema.InteractiveDrawing tool is not available" );

    auto hasElements = [this]( const SELECTION& aSel )
    {
        return !m_frame->GetScreen()->Items().empty();
    };

    auto sheetHasUndefinedPins = []( const SELECTION& aSel )
    {
        if( aSel.Size() == 1 && aSel.Front()->Type() == SCH_SHEET_T )
            return static_cast<SCH_SHEET*>( aSel.Front() )->HasUndefinedPins();

        return false;
    };

    auto attribDNPCond = []( const SELECTION& aSel )
    {
        return std::all_of( aSel.Items().begin(), aSel.Items().end(),
                            []( const EDA_ITEM* item )
                            {
                                return !item->IsType( { SCH_SYMBOL_T } )
                                       || static_cast<const SCH_SYMBOL*>( item )->GetDNP();
                            } );
    };

    auto attribExcludeFromSimCond = []( const SELECTION& aSel )
    {
        return std::all_of( aSel.Items().begin(), aSel.Items().end(),
                            []( const EDA_ITEM* item )
                            {
                                return !item->IsType( { SCH_SYMBOL_T } )
                                       || static_cast<const SCH_SYMBOL*>( item )->GetExcludedFromSim();
                            } );
    };

    auto attribExcludeFromBOMCond = []( const SELECTION& aSel )
    {
        return std::all_of( aSel.Items().begin(), aSel.Items().end(),
                            []( const EDA_ITEM* item )
                            {
                                return !item->IsType( { SCH_SYMBOL_T } )
                                       || static_cast<const SCH_SYMBOL*>( item )->GetExcludedFromBOM();
                            } );
    };


    auto attribExcludeFromBoardCond = []( const SELECTION& aSel )
    {
        return std::all_of( aSel.Items().begin(), aSel.Items().end(),
                            []( const EDA_ITEM* item )
                            {
                                return !item->IsType( { SCH_SYMBOL_T } )
                                       || static_cast<const SCH_SYMBOL*>( item )->GetExcludedFromBoard();
                            } );
    };

    static const std::vector<KICAD_T> sheetTypes = { SCH_SHEET_T };

    auto sheetSelection = S_C::Count( 1 ) && S_C::OnlyTypes( sheetTypes );

    auto haveHighlight = [this]( const SELECTION& sel )
    {
        SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

        return editFrame && !editFrame->GetHighlightedConnection().IsEmpty();
    };

    auto anyTextTool = [this]( const SELECTION& aSel )
    {
        return ( m_frame->IsCurrentTool( SCH_ACTIONS::placeLabel )
                 || m_frame->IsCurrentTool( SCH_ACTIONS::placeClassLabel )
                 || m_frame->IsCurrentTool( SCH_ACTIONS::placeGlobalLabel )
                 || m_frame->IsCurrentTool( SCH_ACTIONS::placeHierLabel )
                 || m_frame->IsCurrentTool( SCH_ACTIONS::placeSchematicText ) );
    };

    auto duplicateCondition = []( const SELECTION& aSel )
    {
        if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        return true;
    };

    auto orientCondition = []( const SELECTION& aSel )
    {
        if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        return SELECTION_CONDITIONS::HasTypes( SCH_EDIT_TOOL::RotatableItems )( aSel );
    };

    const auto swapSelectionCondition = S_C::OnlyTypes( SwappableItems ) && SELECTION_CONDITIONS::MoreThan( 1 );

    auto propertiesCondition = [this]( const SELECTION& aSel )
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

        SCH_ITEM*            firstItem = dynamic_cast<SCH_ITEM*>( aSel.Front() );
        const SCH_SELECTION* eeSelection = dynamic_cast<const SCH_SELECTION*>( &aSel );

        if( !firstItem || !eeSelection )
            return false;

        switch( firstItem->Type() )
        {
        case SCH_SYMBOL_T:
        case SCH_SHEET_T:
        case SCH_SHEET_PIN_T:
        case SCH_TEXT_T:
        case SCH_TEXTBOX_T:
        case SCH_TABLE_T:
        case SCH_TABLECELL_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
        case SCH_RULE_AREA_T:
        case SCH_FIELD_T:
        case SCH_SHAPE_T:
        case SCH_BITMAP_T:
        case SCH_GROUP_T: return aSel.GetSize() == 1;

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

        default: return false;
        }
    };

    auto autoplaceCondition = []( const SELECTION& aSel )
    {
        for( const EDA_ITEM* item : aSel )
        {
            if( item->IsType( SCH_COLLECTOR::FieldOwners ) )
                return true;
        }

        return false;
    };

    // allTextTypes does not include SCH_SHEET_PIN_T because one cannot convert other
    // types to/from this type, living only in a SHEET
    static const std::vector<KICAD_T> allTextTypes = { SCH_LABEL_T,        SCH_DIRECTIVE_LABEL_T,
                                                       SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T,
                                                       SCH_TEXT_T,         SCH_TEXTBOX_T };

    auto toChangeCondition = ( S_C::OnlyTypes( allTextTypes ) );

    static const std::vector<KICAD_T> toLabelTypes = { SCH_DIRECTIVE_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T,
                                                       SCH_TEXT_T, SCH_TEXTBOX_T };

    auto toLabelCondition = ( S_C::Count( 1 ) && S_C::OnlyTypes( toLabelTypes ) )
                            || ( S_C::MoreThan( 1 ) && S_C::OnlyTypes( allTextTypes ) );

    static const std::vector<KICAD_T> toCLabelTypes = { SCH_LABEL_T, SCH_HIER_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_TEXT_T,
                                                        SCH_TEXTBOX_T };

    auto toCLabelCondition = ( S_C::Count( 1 ) && S_C::OnlyTypes( toCLabelTypes ) )
                             || ( S_C::MoreThan( 1 ) && S_C::OnlyTypes( allTextTypes ) );

    static const std::vector<KICAD_T> toHLabelTypes = { SCH_LABEL_T, SCH_DIRECTIVE_LABEL_T, SCH_GLOBAL_LABEL_T,
                                                        SCH_TEXT_T, SCH_TEXTBOX_T };

    auto toHLabelCondition = ( S_C::Count( 1 ) && S_C::OnlyTypes( toHLabelTypes ) )
                             || ( S_C::MoreThan( 1 ) && S_C::OnlyTypes( allTextTypes ) );

    static const std::vector<KICAD_T> toGLabelTypes = { SCH_LABEL_T, SCH_DIRECTIVE_LABEL_T, SCH_HIER_LABEL_T,
                                                        SCH_TEXT_T, SCH_TEXTBOX_T };

    auto toGLabelCondition = ( S_C::Count( 1 ) && S_C::OnlyTypes( toGLabelTypes ) )
                             || ( S_C::MoreThan( 1 ) && S_C::OnlyTypes( allTextTypes ) );

    static const std::vector<KICAD_T> toTextTypes = { SCH_LABEL_T, SCH_DIRECTIVE_LABEL_T, SCH_GLOBAL_LABEL_T,
                                                      SCH_HIER_LABEL_T, SCH_TEXTBOX_T };

    auto toTextCondition = ( S_C::Count( 1 ) && S_C::OnlyTypes( toTextTypes ) )
                           || ( S_C::MoreThan( 1 ) && S_C::OnlyTypes( allTextTypes ) );

    static const std::vector<KICAD_T> toTextBoxTypes = { SCH_LABEL_T, SCH_DIRECTIVE_LABEL_T, SCH_GLOBAL_LABEL_T,
                                                         SCH_HIER_LABEL_T, SCH_TEXT_T };

    auto toTextBoxCondition = ( S_C::Count( 1 ) && S_C::OnlyTypes( toTextBoxTypes ) )
                              || ( S_C::MoreThan( 1 ) && S_C::OnlyTypes( allTextTypes ) );

    static const std::vector<KICAD_T> busEntryTypes = { SCH_BUS_WIRE_ENTRY_T, SCH_BUS_BUS_ENTRY_T };

    auto entryCondition = S_C::MoreThan( 0 ) && S_C::OnlyTypes( busEntryTypes );

    auto singleSheetCondition = S_C::Count( 1 ) && S_C::OnlyTypes( sheetTypes );

    auto makeSymbolUnitMenu = [&]( TOOL_INTERACTIVE* tool )
    {
        std::shared_ptr<SYMBOL_UNIT_MENU> menu = std::make_shared<SYMBOL_UNIT_MENU>();
        menu->SetTool( tool );
        tool->GetToolMenu().RegisterSubMenu( menu );
        return menu.get();
    };

    auto makeBodyStyleMenu = [&]( TOOL_INTERACTIVE* tool )
    {
        std::shared_ptr<BODY_STYLE_MENU> menu = std::make_shared<BODY_STYLE_MENU>();
        menu->SetTool( tool );
        tool->GetToolMenu().RegisterSubMenu( menu );
        return menu.get();
    };

    auto makePinFunctionMenu = [&]( TOOL_INTERACTIVE* tool )
    {
        std::shared_ptr<ALT_PIN_FUNCTION_MENU> menu = std::make_shared<ALT_PIN_FUNCTION_MENU>();
        menu->SetTool( tool );
        tool->GetToolMenu().RegisterSubMenu( menu );
        return menu.get();
    };

    auto makePinTricksMenu = [&]( TOOL_INTERACTIVE* tool )
    {
        std::shared_ptr<PIN_TRICKS_MENU> menu = std::make_shared<PIN_TRICKS_MENU>();
        menu->SetTool( tool );
        tool->GetToolMenu().RegisterSubMenu( menu );
        return menu.get();
    };

    auto makeTransformMenu = [&]()
    {
        CONDITIONAL_MENU* menu = new CONDITIONAL_MENU( moveTool );
        menu->SetUntranslatedTitle( _HKI( "Transform Selection" ) );

        menu->AddItem( SCH_ACTIONS::rotateCCW, orientCondition );
        menu->AddItem( SCH_ACTIONS::rotateCW, orientCondition );
        menu->AddItem( SCH_ACTIONS::mirrorV, orientCondition );
        menu->AddItem( SCH_ACTIONS::mirrorH, orientCondition );

        return menu;
    };

    auto makeAttributesMenu = [&]()
    {
        CONDITIONAL_MENU* menu = new CONDITIONAL_MENU( moveTool );
        menu->SetUntranslatedTitle( _HKI( "Attributes" ) );

        menu->AddCheckItem( SCH_ACTIONS::setExcludeFromSimulation, S_C::ShowAlways );
        menu->AddCheckItem( SCH_ACTIONS::setExcludeFromBOM, S_C::ShowAlways );
        menu->AddCheckItem( SCH_ACTIONS::setExcludeFromBoard, S_C::ShowAlways );
        menu->AddCheckItem( SCH_ACTIONS::setDNP, S_C::ShowAlways );

        return menu;
    };

    auto makeEditFieldsMenu = [&]()
    {
        CONDITIONAL_MENU* menu = new CONDITIONAL_MENU( m_selectionTool );
        menu->SetUntranslatedTitle( _HKI( "Edit Main Fields" ) );

        menu->AddItem( SCH_ACTIONS::editReference, S_C::SingleSymbol, 200 );
        menu->AddItem( SCH_ACTIONS::editValue, S_C::SingleSymbol, 200 );
        menu->AddItem( SCH_ACTIONS::editFootprint, S_C::SingleSymbol, 200 );

        return menu;
    };

    auto makeConvertToMenu = [&]()
    {
        CONDITIONAL_MENU* menu = new CONDITIONAL_MENU( m_selectionTool );
        menu->SetUntranslatedTitle( _HKI( "Change To" ) );
        menu->SetIcon( BITMAPS::right );

        menu->AddItem( SCH_ACTIONS::toLabel, toLabelCondition );
        menu->AddItem( SCH_ACTIONS::toDLabel, toCLabelCondition );
        menu->AddItem( SCH_ACTIONS::toHLabel, toHLabelCondition );
        menu->AddItem( SCH_ACTIONS::toGLabel, toGLabelCondition );
        menu->AddItem( SCH_ACTIONS::toText, toTextCondition );
        menu->AddItem( SCH_ACTIONS::toTextBox, toTextBoxCondition );

        return menu;
    };

    const auto canCopyText = SCH_CONDITIONS::OnlyTypes( {
            SCH_TEXT_T,
            SCH_TEXTBOX_T,
            SCH_FIELD_T,
            SCH_LABEL_T,
            SCH_HIER_LABEL_T,
            SCH_GLOBAL_LABEL_T,
            SCH_DIRECTIVE_LABEL_T,
            SCH_SHEET_PIN_T,
            SCH_PIN_T,
            SCH_TABLE_T,
            SCH_TABLECELL_T,
    } );

    //
    // Add edit actions to the move tool menu
    //
    CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

    moveMenu.AddSeparator();
    moveMenu.AddMenu( makeSymbolUnitMenu( moveTool ), S_C::SingleMultiUnitSymbol, 1 );
    moveMenu.AddMenu( makeBodyStyleMenu( moveTool ), S_C::SingleMultiBodyStyleSymbol, 1 );

    moveMenu.AddMenu( makeTransformMenu(), orientCondition, 200 );
    moveMenu.AddMenu( makeAttributesMenu(), S_C::HasType( SCH_SYMBOL_T ), 200 );
    moveMenu.AddItem( SCH_ACTIONS::swap, swapSelectionCondition, 200 );
    moveMenu.AddItem( SCH_ACTIONS::properties, propertiesCondition, 200 );
    moveMenu.AddMenu( makeEditFieldsMenu(), S_C::SingleSymbol, 200 );

    moveMenu.AddSeparator();
    moveMenu.AddItem( ACTIONS::cut, S_C::IdleSelection );
    moveMenu.AddItem( ACTIONS::copy, S_C::IdleSelection );
    moveMenu.AddItem( ACTIONS::copyAsText, canCopyText && S_C::IdleSelection );
    moveMenu.AddItem( ACTIONS::doDelete, S_C::NotEmpty );
    moveMenu.AddItem( ACTIONS::duplicate, duplicateCondition );

    //
    // Add editing actions to the drawing tool menu
    //
    CONDITIONAL_MENU& drawMenu = drawingTools->GetToolMenu().GetMenu();

    drawMenu.AddItem( SCH_ACTIONS::clearHighlight, haveHighlight && SCH_CONDITIONS::Idle, 1 );
    drawMenu.AddSeparator( haveHighlight && SCH_CONDITIONS::Idle, 1 );

    drawMenu.AddItem( SCH_ACTIONS::enterSheet, sheetSelection && SCH_CONDITIONS::Idle, 1 );
    drawMenu.AddSeparator( sheetSelection && SCH_CONDITIONS::Idle, 1 );

    drawMenu.AddMenu( makeSymbolUnitMenu( drawingTools ), S_C::SingleMultiUnitSymbol, 1 );
    drawMenu.AddMenu( makeBodyStyleMenu( drawingTools ), S_C::SingleMultiBodyStyleSymbol, 1 );

    drawMenu.AddMenu( makeTransformMenu(), orientCondition, 200 );
    drawMenu.AddMenu( makeAttributesMenu(), S_C::HasType( SCH_SYMBOL_T ), 200 );
    drawMenu.AddItem( SCH_ACTIONS::properties, propertiesCondition, 200 );
    drawMenu.AddMenu( makeEditFieldsMenu(), S_C::SingleSymbol, 200 );
    drawMenu.AddItem( SCH_ACTIONS::autoplaceFields, autoplaceCondition, 200 );

    drawMenu.AddItem( SCH_ACTIONS::editWithLibEdit, S_C::SingleSymbolOrPower && S_C::Idle, 200 );

    drawMenu.AddItem( SCH_ACTIONS::toLabel, anyTextTool && S_C::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toHLabel, anyTextTool && S_C::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toGLabel, anyTextTool && S_C::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toText, anyTextTool && S_C::Idle, 200 );
    drawMenu.AddItem( SCH_ACTIONS::toTextBox, anyTextTool && S_C::Idle, 200 );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddMenu( makeSymbolUnitMenu( m_selectionTool ), S_C::SingleMultiUnitSymbol, 1 );
    selToolMenu.AddMenu( makeBodyStyleMenu( m_selectionTool ), S_C::SingleMultiBodyStyleSymbol, 1 );
    selToolMenu.AddMenu( makePinFunctionMenu( m_selectionTool ), S_C::SingleMultiFunctionPin, 1 );
    selToolMenu.AddMenu( makePinTricksMenu( m_selectionTool ), S_C::AllPinsOrSheetPins, 1 );

    selToolMenu.AddMenu( makeTransformMenu(), orientCondition, 200 );
    selToolMenu.AddMenu( makeAttributesMenu(), S_C::HasType( SCH_SYMBOL_T ), 200 );
    selToolMenu.AddItem( SCH_ACTIONS::swap, swapSelectionCondition, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::properties, propertiesCondition, 200 );
    selToolMenu.AddMenu( makeEditFieldsMenu(), S_C::SingleSymbol, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::autoplaceFields, autoplaceCondition, 200 );

    selToolMenu.AddItem( SCH_ACTIONS::editWithLibEdit, S_C::SingleSymbolOrPower && S_C::Idle, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::changeSymbol, S_C::SingleSymbolOrPower, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::updateSymbol, S_C::SingleSymbolOrPower, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::changeSymbols, S_C::MultipleSymbolsOrPower, 200 );
    selToolMenu.AddItem( SCH_ACTIONS::updateSymbols, S_C::MultipleSymbolsOrPower, 200 );
    selToolMenu.AddMenu( makeConvertToMenu(), toChangeCondition, 200 );

    selToolMenu.AddItem( SCH_ACTIONS::cleanupSheetPins, sheetHasUndefinedPins, 250 );

    selToolMenu.AddSeparator( 300 );
    selToolMenu.AddItem( ACTIONS::cut, S_C::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copy, S_C::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copyAsText, canCopyText && S_C::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::paste, S_C::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::pasteSpecial, S_C::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::doDelete, S_C::NotEmpty, 300 );
    selToolMenu.AddItem( ACTIONS::duplicate, duplicateCondition, 300 );

    selToolMenu.AddSeparator( 400 );
    selToolMenu.AddItem( ACTIONS::selectAll, hasElements, 400 );
    selToolMenu.AddItem( ACTIONS::unselectAll, hasElements, 400 );

    ACTION_MANAGER* mgr = m_toolMgr->GetActionManager();
    // clang-format off
    mgr->SetConditions( SCH_ACTIONS::setDNP,                   ACTION_CONDITIONS().Check( attribDNPCond ) );
    mgr->SetConditions( SCH_ACTIONS::setExcludeFromSimulation, ACTION_CONDITIONS().Check( attribExcludeFromSimCond ) );
    mgr->SetConditions( SCH_ACTIONS::setExcludeFromBOM,        ACTION_CONDITIONS().Check( attribExcludeFromBOMCond ) );
    mgr->SetConditions( SCH_ACTIONS::setExcludeFromBoard,      ACTION_CONDITIONS().Check( attribExcludeFromBoardCond ) );
    // clang-format on

    return true;
}


const std::vector<KICAD_T> SCH_EDIT_TOOL::RotatableItems = {
    SCH_SHAPE_T,         SCH_RULE_AREA_T,      SCH_TEXT_T,      SCH_TEXTBOX_T,    SCH_TABLE_T,
    SCH_TABLECELL_T, // will be promoted to parent table(s)
    SCH_LABEL_T,         SCH_GLOBAL_LABEL_T,   SCH_GROUP_T,     SCH_HIER_LABEL_T, SCH_DIRECTIVE_LABEL_T,
    SCH_FIELD_T,         SCH_SYMBOL_T,         SCH_SHEET_PIN_T, SCH_SHEET_T,      SCH_BITMAP_T,
    SCH_BUS_BUS_ENTRY_T, SCH_BUS_WIRE_ENTRY_T, SCH_LINE_T,      SCH_JUNCTION_T,   SCH_NO_CONNECT_T
};


const std::vector<KICAD_T> SCH_EDIT_TOOL::SwappableItems = {
    SCH_SHAPE_T,     SCH_RULE_AREA_T,    SCH_TEXT_T,       SCH_TEXTBOX_T,         SCH_LABEL_T,
    SCH_SHEET_PIN_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, SCH_DIRECTIVE_LABEL_T, SCH_FIELD_T,
    SCH_SYMBOL_T,    SCH_SHEET_T,        SCH_BITMAP_T,     SCH_JUNCTION_T,        SCH_NO_CONNECT_T
};


int SCH_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    bool           clockwise = ( aEvent.Matches( SCH_ACTIONS::rotateCW.MakeEvent() ) );
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( RotatableItems, true, false );

    wxLogTrace( "KICAD_SCH_MOVE", "SCH_EDIT_TOOL::Rotate: start, clockwise=%d, selection size=%u", clockwise,
                selection.GetSize() );

    if( selection.GetSize() == 0 )
        return 0;

    SCH_ITEM*   head = nullptr;
    int         principalItemCount = 0; // User-selected items (as opposed to connected wires)
    VECTOR2I    rotPoint;
    bool        moving = false;
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );
    SCH_SCREEN* screen = m_frame->GetScreen();

    std::map<SCH_SHEET_PIN*, SCH_NO_CONNECT*> noConnects;

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
            commit->Modify( head, screen, RECURSE_MODE::RECURSE );

        switch( head->Type() )
        {
        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( head );

            symbol->Rotate( rotPoint, !clockwise );

            if( m_frame->eeconfig()->m_AutoplaceFields.enable )
            {
                AUTOPLACE_ALGO fieldsAutoplaced = symbol->GetFieldsAutoplaced();

                if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
                    symbol->AutoplaceFields( screen, fieldsAutoplaced );
            }

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
            SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( head );
            SCH_SHEET*     sheet = pin->GetParent();

            for( SCH_ITEM* ncItem : screen->Items().Overlapping( SCH_NO_CONNECT_T, pin->GetTextPos() ) )
                noConnects[pin] = static_cast<SCH_NO_CONNECT*>( ncItem );

            pin->Rotate( sheet->GetBoundingBox().GetCenter(), !clockwise );

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
        case SCH_NO_CONNECT_T:
        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T: head->Rotate( rotPoint, !clockwise ); break;

        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( head );

            if( field->GetTextAngle().IsHorizontal() )
                field->SetTextAngle( ANGLE_VERTICAL );
            else
                field->SetTextAngle( ANGLE_HORIZONTAL );

            // Now that we're moving a field, they're no longer autoplaced.
            static_cast<SCH_ITEM*>( head->GetParent() )->SetFieldsAutoplaced( AUTOPLACE_NONE );

            break;
        }

        case SCH_RULE_AREA_T:
        case SCH_SHAPE_T:
        case SCH_TEXTBOX_T: head->Rotate( rotPoint, !clockwise ); break;

        case SCH_GROUP_T:
        {
            // Rotate the group on itself. Groups do not have an anchor point.
            SCH_GROUP* group = static_cast<SCH_GROUP*>( head );
            rotPoint = m_frame->GetNearestHalfGridPosition( group->GetPosition() );

            group->Rotate( rotPoint, !clockwise );

            group->Move( rotPoint - m_frame->GetNearestHalfGridPosition( group->GetPosition() ) );

            break;
        }

        case SCH_TABLE_T:
        {
            // Rotate the table on itself. Tables do not have an anchor point.
            SCH_TABLE* table = static_cast<SCH_TABLE*>( head );
            rotPoint = m_frame->GetNearestHalfGridPosition( table->GetCenter() );

            table->Rotate( rotPoint, !clockwise );

            table->Move( rotPoint - m_frame->GetNearestHalfGridPosition( table->GetCenter() ) );

            break;
        }

        case SCH_BITMAP_T:
            head->Rotate( rotPoint, clockwise );

            // The bitmap is cached in Opengl: clear the cache to redraw
            getView()->RecacheAllItems();
            break;

        case SCH_SHEET_T:
        {
            // Rotate the sheet on itself. Sheets do not have an anchor point.
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( head );

            noConnects = sheet->GetNoConnects();

            rotPoint = m_frame->GetNearestHalfGridPosition( sheet->GetRotationCenter() );
            sheet->Rotate( rotPoint, !clockwise );

            break;
        }

        default: UNIMPLEMENTED_FOR( head->GetClass() );
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
            commit->Modify( item, screen, RECURSE_MODE::RECURSE );

        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = (SCH_LINE*) item;

            line->Rotate( rotPoint, !clockwise );
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

                for( SCH_ITEM* ncItem : screen->Items().Overlapping( SCH_NO_CONNECT_T, pin->GetTextPos() ) )
                    noConnects[pin] = static_cast<SCH_NO_CONNECT*>( ncItem );

                pin->Rotate( sheet->GetBodyBoundingBox().GetCenter(), !clockwise );
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

                field->Rotate( rotPoint, !clockwise );

                // Now that we're moving a field, they're no longer autoplaced.
                static_cast<SCH_ITEM*>( field->GetParent() )->SetFieldsAutoplaced( AUTOPLACE_NONE );
            }
        }
        else if( item->Type() == SCH_TABLE_T )
        {
            SCH_TABLE* table = static_cast<SCH_TABLE*>( item );
            VECTOR2I   beforeCenter = table->GetCenter();

            table->Rotate( rotPoint, !clockwise );
            RotatePoint( beforeCenter, rotPoint, clockwise ? -ANGLE_90 : ANGLE_90 );

            table->Move( beforeCenter - table->GetCenter() );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            noConnects = sheet->GetNoConnects();

            sheet->Rotate( rotPoint, !clockwise );
        }
        else
        {
            VECTOR2I posBefore = item->GetPosition();
            item->Rotate( rotPoint, !clockwise );
            VECTOR2I posAfter = item->GetPosition();
            wxLogTrace( "KICAD_SCH_MOVE", "  SCH_EDIT_TOOL::Rotate: item type=%d rotated, pos (%d,%d) -> (%d,%d)",
                        item->Type(), posBefore.x, posBefore.y, posAfter.x, posAfter.y );
        }

        m_frame->UpdateItem( item, false, true );
        updateItem( item, true );
    }

    wxLogTrace( "KICAD_SCH_MOVE", "SCH_EDIT_TOOL::Rotate: complete, moving=%d", moving );

    if( moving )
    {
        wxLogTrace( "KICAD_SCH_MOVE", "SCH_EDIT_TOOL::Rotate: posting refreshPreview" );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else
    {
        for( auto& [sheetPin, noConnect] : noConnects )
        {
            if( noConnect->GetPosition() != sheetPin->GetTextPos() )
            {
                commit->Modify( noConnect, screen );
                noConnect->SetPosition( sheetPin->GetTextPos() );
                updateItem( noConnect, true );
            }
        }

        SCH_SELECTION selectionCopy = selection;

        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
        lwbTool->TrimOverLappingWires( commit, &selectionCopy );
        lwbTool->AddJunctionsIfNeeded( commit, &selectionCopy );

        m_frame->Schematic().CleanUp( commit );

        if( !localCommit.Empty() )
            localCommit.Push( _( "Rotate" ) );
    }

    return 0;
}


int SCH_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( RotatableItems, false, false );

    if( selection.GetSize() == 0 )
        return 0;

    bool        vertical = ( aEvent.Matches( SCH_ACTIONS::mirrorV.MakeEvent() ) );
    SCH_ITEM*   item = static_cast<SCH_ITEM*>( selection.Front() );
    bool        connections = false;
    bool        moving = item->IsMoving();
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );
    SCH_SCREEN* screen = m_frame->GetScreen();

    std::map<SCH_SHEET_PIN*, SCH_NO_CONNECT*> noConnects;

    if( !commit )
        commit = &localCommit;

    if( selection.GetSize() == 1 )
    {
        if( !moving )
            commit->Modify( item, screen, RECURSE_MODE::RECURSE );

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( vertical )
                symbol->SetOrientation( SYM_MIRROR_X );
            else
                symbol->SetOrientation( SYM_MIRROR_Y );

            symbol->SetFieldsAutoplaced( AUTOPLACE_NONE );
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

            for( SCH_ITEM* ncItem : screen->Items().Overlapping( SCH_NO_CONNECT_T, pin->GetTextPos() ) )
                noConnects[pin] = static_cast<SCH_NO_CONNECT*>( ncItem );

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
                field->SetVertJustify( GetFlippedAlignment( field->GetVertJustify() ) );
            else
                field->SetHorizJustify( GetFlippedAlignment( field->GetHorizJustify() ) );

            // Now that we're re-justifying a field, they're no longer autoplaced.
            static_cast<SCH_ITEM*>( field->GetParent() )->SetFieldsAutoplaced( AUTOPLACE_NONE );

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
            noConnects = static_cast<SCH_SHEET*>( item )->GetNoConnects();

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
                commit->Modify( item, screen, RECURSE_MODE::RECURSE );

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
                    field->SetVertJustify( GetFlippedAlignment( field->GetVertJustify() ) );
                else
                    field->SetHorizJustify( GetFlippedAlignment( field->GetHorizJustify() ) );

                // Now that we're re-justifying a field, they're no longer autoplaced.
                static_cast<SCH_ITEM*>( field->GetParent() )->SetFieldsAutoplaced( AUTOPLACE_NONE );
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
        for( auto& [sheetPin, noConnect] : noConnects )
        {
            if( noConnect->GetPosition() != sheetPin->GetTextPos() )
            {
                commit->Modify( noConnect, screen );
                noConnect->SetPosition( sheetPin->GetTextPos() );
                updateItem( noConnect, true );
            }
        }

        SCH_SELECTION selectionCopy = selection;

        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        if( connections )
        {
            SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
            lwbTool->TrimOverLappingWires( commit, &selectionCopy );
            lwbTool->AddJunctionsIfNeeded( commit, &selectionCopy );

            m_frame->Schematic().CleanUp( commit );
        }

        if( !localCommit.Empty() )
            localCommit.Push( _( "Mirror" ) );
    }

    return 0;
}
/**
 * Swap the positions of the fields in the two lists, aAFields and aBFields,
 * relative to their parent positions.
 *
 * If a field is in both lists, it will be swapped to the position of the
 * matching field on the counterpart.
 *
 * If a field is in only one list, it will simply be rotated by aFallbackRotation
 * (CW or CCW depending on which list it is in)
 */
static void swapFieldPositionsWithMatching( std::vector<SCH_FIELD>& aAFields, std::vector<SCH_FIELD>& aBFields,
                                            unsigned aFallbackRotationsCCW )
{
    std::set<wxString> handledKeys;

    const auto swapFieldTextProps = []( SCH_FIELD& aField, SCH_FIELD& bField )
    {
        const VECTOR2I          aRelPos = aField.GetPosition() - aField.GetParentPosition();
        const GR_TEXT_H_ALIGN_T aTextJustifyH = aField.GetHorizJustify();
        const GR_TEXT_V_ALIGN_T aTextJustifyV = aField.GetVertJustify();
        const EDA_ANGLE         aTextAngle = aField.GetTextAngle();

        const VECTOR2I          bRelPos = bField.GetPosition() - bField.GetParentPosition();
        const GR_TEXT_H_ALIGN_T bTextJustifyH = bField.GetHorizJustify();
        const GR_TEXT_V_ALIGN_T bTextJustifyV = bField.GetVertJustify();
        const EDA_ANGLE         bTextAngle = bField.GetTextAngle();

        aField.SetPosition( aField.GetParentPosition() + bRelPos );
        aField.SetHorizJustify( bTextJustifyH );
        aField.SetVertJustify( bTextJustifyV );
        aField.SetTextAngle( bTextAngle );

        bField.SetPosition( bField.GetParentPosition() + aRelPos );
        bField.SetHorizJustify( aTextJustifyH );
        bField.SetVertJustify( aTextJustifyV );
        bField.SetTextAngle( aTextAngle );
    };

    for( SCH_FIELD& aField : aAFields )
    {
        const wxString name = aField.GetCanonicalName();

        auto it = std::find_if( aBFields.begin(), aBFields.end(),
                                [name]( const SCH_FIELD& bField )
                                {
                                    return bField.GetCanonicalName() == name;
                                } );

        if( it != aBFields.end() )
        {
            // We have a field with the same key in both labels
            SCH_FIELD& bField = *it;
            swapFieldTextProps( aField, bField );
        }
        else
        {
            // We only have this field in A, so just rotate it
            for( unsigned ii = 0; ii < aFallbackRotationsCCW; ii++ )
            {
                aField.Rotate( aField.GetParentPosition(), true );
            }
        }

        // And keep track that we did this one
        handledKeys.insert( name );
    }

    // Any fields in B that weren't in A weren't handled and need to be rotated
    // in reverse
    for( SCH_FIELD& bField : aBFields )
    {
        const wxString bName = bField.GetCanonicalName();
        if( handledKeys.find( bName ) == handledKeys.end() )
        {
            for( unsigned ii = 0; ii < aFallbackRotationsCCW; ii++ )
            {
                bField.Rotate( bField.GetParentPosition(), false );
            }
        }
    }
}


int SCH_EDIT_TOOL::Swap( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION&         selection = m_selectionTool->RequestSelection( SwappableItems );
    std::vector<EDA_ITEM*> sorted = selection.GetItemsSortedBySelectionOrder();

    if( selection.Size() < 2 )
        return 0;

    // Sheet pins are special, we need to make sure if we have any sheet pins,
    // that we only have sheet pins, and that they have the same parent
    if( selection.CountType( SCH_SHEET_PIN_T ) > 0 )
    {
        if( !selection.OnlyContains( { SCH_SHEET_PIN_T } ) )
            return 0;

        EDA_ITEM* parent = selection.Front()->GetParent();

        for( EDA_ITEM* item : selection )
        {
            if( item->GetParent() != parent )
                return 0;
        }
    }

    bool moving = selection.Front()->IsMoving();
    bool connections = false;

    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    for( size_t i = 0; i < sorted.size() - 1; i++ )
    {
        SCH_ITEM* a = static_cast<SCH_ITEM*>( sorted[i] );
        SCH_ITEM* b = static_cast<SCH_ITEM*>( sorted[( i + 1 ) % sorted.size()] );

        if( !moving )
        {
            commit->Modify( a, m_frame->GetScreen(), RECURSE_MODE::RECURSE );
            commit->Modify( b, m_frame->GetScreen(), RECURSE_MODE::RECURSE );
        }

        VECTOR2I aPos = a->GetPosition(), bPos = b->GetPosition();
        std::swap( aPos, bPos );

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
            {
                SCH_LABEL_BASE& aLabelBase = static_cast<SCH_LABEL_BASE&>( *a );
                SCH_LABEL_BASE& bLabelBase = static_cast<SCH_LABEL_BASE&>( *b );

                const SPIN_STYLE aSpinStyle = aLabelBase.GetSpinStyle();
                const SPIN_STYLE bSpinStyle = bLabelBase.GetSpinStyle();

                // First, swap the label orientations
                aLabelBase.SetSpinStyle( bSpinStyle );
                bLabelBase.SetSpinStyle( aSpinStyle );

                // And swap the fields as best we can
                std::vector<SCH_FIELD>& aFields = aLabelBase.GetFields();
                std::vector<SCH_FIELD>& bFields = bLabelBase.GetFields();

                const unsigned rotationsAtoB = aSpinStyle.CCWRotationsTo( bSpinStyle );

                swapFieldPositionsWithMatching( aFields, bFields, rotationsAtoB );
                break;
            }
            case SCH_SYMBOL_T:
            {
                SCH_SYMBOL* aSymbol = static_cast<SCH_SYMBOL*>( a );
                SCH_SYMBOL* bSymbol = static_cast<SCH_SYMBOL*>( b );
                int         aOrient = aSymbol->GetOrientation(), bOrient = bSymbol->GetOrientation();
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

    if( moving )
    {
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        if( connections )
            m_frame->TestDanglingEnds();
        m_frame->OnModify();

        if( !localCommit.Empty() )
            localCommit.Push( _( "Swap" ) );
    }

    return 0;
}


/*
 * This command always works on the instance owned by the schematic, never directly on the
 * external library file. Pins that still reference their library definition are swapped by
 * touching that shared lib pin first; afterwards we call UpdatePins() so the schematic now owns a
 * cached copy with the new geometry. Pins that already have an instance-local copy simply swap in
 * place. In both cases the undo stack captures the modified pins (and the parent symbol) so the
 * user can revert the change. Saving the schematic writes the updated pin order into the sheet,
 * while the global symbol library remains untouched unless the user explicitly pushes it later.
 */
int SCH_EDIT_TOOL::SwapPins( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    if( !m_frame->eeconfig()->m_Input.allow_unconstrained_pin_swaps )
        return 0;

    SCH_SELECTION&         selection = m_selectionTool->RequestSelection( { SCH_PIN_T } );
    std::vector<EDA_ITEM*> sorted = selection.GetItemsSortedBySelectionOrder();

    if( selection.Size() < 2 )
        return 0;

    EDA_ITEM* parent = selection.Front()->GetParent();

    if( !parent || parent->Type() != SCH_SYMBOL_T )
        return 0;

    SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( parent );

    // All pins need to be on the same symbol
    for( EDA_ITEM* item : selection )
    {
        if( item->GetParent() != parent )
            return 0;
    }

    std::set<wxString> sharedSheetPaths;
    std::set<wxString> sharedProjectNames;

    if( SymbolHasSheetInstances( *parentSymbol, m_frame->Prj().GetProjectName(), &sharedSheetPaths,
                                 &sharedProjectNames ) )
    {
        // This will give us nice names for our project, but not when the sheet is shared across
        // multiple projects. But, in that case we bail early and just list the project names so it isn't an issue.
        std::set<wxString> friendlySheets;

        if( !sharedSheetPaths.empty() )
            friendlySheets = GetSheetNamesFromPaths( sharedSheetPaths, m_frame->Schematic() );

        if( !sharedProjectNames.empty() )
        {
            wxString projects = AccumulateDescriptions( sharedProjectNames );

            if( projects.IsEmpty() )
            {
                m_frame->ShowInfoBarError( _( "Pin swaps are disabled for symbols shared across other projects. "
                                              "Duplicate the sheet to edit pins independently." ) );
            }
            else
            {
                m_frame->ShowInfoBarError(
                        wxString::Format( _( "Pin swaps are disabled for symbols shared across other projects (%s). "
                                             "Duplicate the sheet to edit pins independently." ),
                                          projects ) );
            }
        }
        else if( !friendlySheets.empty() )
        {
            wxString sheets = AccumulateDescriptions( friendlySheets );

            m_frame->ShowInfoBarError(
                    wxString::Format( _( "Pin swaps are disabled for symbols used by multiple sheet instances (%s). "
                                         "Duplicate the sheet to edit pins independently." ),
                                      sheets ) );
        }
        else
        {
            m_frame->ShowInfoBarError(
                    _( "Pin swaps are disabled for shared symbols. Duplicate the sheet to edit pins independently." ) );
        }

        return 0;
    }

    bool connections = false;

    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    // Stage the parent symbol so undo/redo captures the cache copy that UpdatePins() may rebuild
    // after we touch any shared library pins.
    commit->Modify( parentSymbol, m_frame->GetScreen(), RECURSE_MODE::RECURSE ); // RECURSE is harmless here

    bool swappedLibPins = false;

    for( size_t i = 0; i < sorted.size() - 1; i++ )
    {
        SCH_PIN* aPin = static_cast<SCH_PIN*>( sorted[i] );
        SCH_PIN* bPin = static_cast<SCH_PIN*>( sorted[( i + 1 ) % sorted.size()] );

        // Record both pins in the commit and swap their geometry.  SwapPinGeometry returns true if
        // it had to operate on the shared library pins (meaning the schematic instance still
        // referenced them), in which case UpdatePins() below promotes the symbol to an instance
        // copy that reflects the new pin order.
        commit->Modify( aPin, m_frame->GetScreen(), RECURSE_MODE::RECURSE );
        commit->Modify( bPin, m_frame->GetScreen(), RECURSE_MODE::RECURSE );

        swappedLibPins |= SwapPinGeometry( aPin, bPin );

        connections |= aPin->IsConnectable();
        connections |= bPin->IsConnectable();
        m_frame->UpdateItem( aPin, false, true );
        m_frame->UpdateItem( bPin, false, true );
    }

    if( swappedLibPins )
        parentSymbol->UpdatePins(); // clone the library data into the schematic cache with new geometry

    // Refresh changed symbol in screen R-Tree / lib caches
    m_frame->UpdateItem( parentSymbol, false, true );

    SCH_SELECTION selectionCopy = selection;

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    // Reconcile any wiring that was connected to the swapped pins so the schematic stays tidy and
    // the undo stack captures the resulting edits to wires and junctions.
    SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
    lwbTool->TrimOverLappingWires( commit, &selectionCopy );
    lwbTool->AddJunctionsIfNeeded( commit, &selectionCopy );

    m_frame->Schematic().CleanUp( commit );

    if( connections )
        m_frame->TestDanglingEnds();

    m_frame->OnModify();

    if( !localCommit.Empty() )
        localCommit.Push( _( "Swap Pins" ) );

    return 0;
}


// Used by SwapPinLabels() and SwapUnitLabels() to find the single net label connected to a pin
static SCH_LABEL_BASE* findSingleNetLabelForPin( SCH_PIN* aPin, CONNECTION_GRAPH* aGraph,
                                                 const SCH_SHEET_PATH& aSheetPath )
{
    if( !aGraph || !aPin )
        return nullptr;

    CONNECTION_SUBGRAPH* sg = aGraph->GetSubgraphForItem( aPin );

    if( !sg )
        return nullptr;

    const std::set<SCH_ITEM*>& items = sg->GetItems();

    size_t          pinCount = 0;
    SCH_LABEL_BASE* label = nullptr;

    for( SCH_ITEM* item : items )
    {
        if( item->Type() == SCH_PIN_T )
            pinCount++;

        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_CONNECTION* conn = item->Connection( &aSheetPath );

            if( conn && conn->IsNet() )
            {
                if( label )
                    return nullptr; // more than one label

                label = static_cast<SCH_LABEL_BASE*>( item );
            }

            break;
        }
        default: break;
        }
    }

    if( pinCount != 1 )
        return nullptr;

    return label;
}


int SCH_EDIT_TOOL::SwapPinLabels( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION&         selection = m_selectionTool->RequestSelection( { SCH_PIN_T } );
    std::vector<EDA_ITEM*> orderedPins = selection.GetItemsSortedBySelectionOrder();

    if( orderedPins.size() < 2 )
        return 0;

    CONNECTION_GRAPH* connectionGraph = m_frame->Schematic().ConnectionGraph();

    const SCH_SHEET_PATH& sheetPath = m_frame->GetCurrentSheet();

    std::vector<SCH_LABEL_BASE*> labels;

    for( EDA_ITEM* item : orderedPins )
    {
        SCH_PIN*        pin = static_cast<SCH_PIN*>( item );
        SCH_LABEL_BASE* label = findSingleNetLabelForPin( pin, connectionGraph, sheetPath );

        if( !label )
        {
            m_frame->ShowInfoBarError(
                    _( "Each selected pin must have exactly one attached net label and no other pin connections." ) );
            return 0;
        }

        labels.push_back( label );
    }

    if( labels.size() >= 2 )
    {
        SCH_COMMIT commit( m_frame );

        for( SCH_LABEL_BASE* lb : labels )
            commit.Modify( lb, m_frame->GetScreen() );

        for( size_t i = 0; i < labels.size() - 1; ++i )
        {
            SCH_LABEL_BASE* a = labels[i];
            SCH_LABEL_BASE* b = labels[( i + 1 ) % labels.size()];
            wxString        aText = a->GetText();
            wxString        bText = b->GetText();
            a->SetText( bText );
            b->SetText( aText );
        }

        commit.Push( _( "Swap Pin Labels" ) );
    }

    return 0;
}


int SCH_EDIT_TOOL::SwapUnitLabels( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION&           selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );
    std::vector<SCH_SYMBOL*> selectedUnits = GetSameSymbolMultiUnitSelection( selection );

    if( selectedUnits.size() < 2 )
        return 0;

    CONNECTION_GRAPH* connectionGraph = m_frame->Schematic().ConnectionGraph();

    const SCH_SHEET_PATH& sheetPath = m_frame->GetCurrentSheet();

    // Build ordered label vectors (sorted by pin X/Y) for each selected unit
    std::vector<std::vector<SCH_LABEL_BASE*>> symbolLabelVectors;

    for( SCH_SYMBOL* symbol : selectedUnits )
    {
        std::vector<std::pair<VECTOR2I, SCH_LABEL_BASE*>> byPos;

        for( SCH_PIN* pin : symbol->GetPins( &sheetPath ) )
        {
            SCH_LABEL_BASE* label = findSingleNetLabelForPin( pin, connectionGraph, sheetPath );

            if( !label )
            {
                m_frame->ShowInfoBarError( _( "Each pin of selected units must have exactly one attached net label and "
                                              "no other pin connections." ) );
                return 0;
            }

            byPos.emplace_back( pin->GetPosition(), label );
        }

        // Sort labels by pin position (X, then Y)
        std::sort( byPos.begin(), byPos.end(),
                   []( const auto& a, const auto& b )
                   {
                       if( a.first.x != b.first.x )
                           return a.first.x < b.first.x;

                       return a.first.y < b.first.y;
                   } );

        // Discard position, just keep the order
        std::vector<SCH_LABEL_BASE*> labels;

        for( const auto& pr : byPos )
            labels.push_back( pr.second );

        symbolLabelVectors.push_back( labels );
    }

    // All selected units are guaranteed to have identical pin counts by GetSameSymbolMultiUnitSelection()
    const size_t pinCount = symbolLabelVectors.front().size();

    // Perform cyclic swap of labels across all selected symbols, per pin index
    SCH_COMMIT commit( m_frame );

    for( size_t pin = 0; pin < pinCount; pin++ )
    {
        for( auto& vec : symbolLabelVectors )
            commit.Modify( vec[pin], m_frame->GetScreen() );

        wxString carry = symbolLabelVectors.back()[pin]->GetText();

        for( size_t i = 0; i < symbolLabelVectors.size(); i++ )
        {
            SCH_LABEL_BASE* lbl = symbolLabelVectors[i][pin];
            wxString        next = lbl->GetText();
            lbl->SetText( carry );
            carry = next;
        }
    }

    if( !commit.Empty() )
        commit.Push( _( "Swap Unit Labels" ) );

    return 0;
}


int SCH_EDIT_TOOL::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    const std::vector<std::unique_ptr<SCH_ITEM>>& sourceItems = m_frame->GetRepeatItems();

    if( sourceItems.empty() )
        return 0;

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_COMMIT          commit( m_toolMgr );
    SCH_SELECTION       newItems;

    for( const std::unique_ptr<SCH_ITEM>& item : sourceItems )
    {
        SCH_ITEM* newItem = item->Duplicate( IGNORE_PARENT_GROUP );
        bool      restore_state = false;

        // Ensure newItem has a suitable parent: the current screen, because an item from
        // a list of items to repeat must be attached to this current screen
        newItem->SetParent( m_frame->GetScreen() );

        if( SCH_GROUP* enteredGroup = selectionTool->GetEnteredGroup() )
        {
            if( newItem->IsGroupableType() )
            {
                commit.Modify( enteredGroup, m_frame->GetScreen(), RECURSE_MODE::NO_RECURSE );
                enteredGroup->AddItem( newItem );
            }
        }

        if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( newItem ) )
        {
            // If incrementing tries to go below zero, tell user why the value is repeated
            if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
            {
                if( !label->IncrementLabel( cfg->m_Drawing.repeat_label_increment ) )
                    m_frame->ShowInfoBarWarning( _( "Label value cannot go below zero" ), true );
            }
        }

        // If cloning a symbol then put into 'move' mode.
        if( newItem->Type() == SCH_SYMBOL_T )
        {
            VECTOR2I cursorPos = getViewControls()->GetCursorPosition( true );
            newItem->Move( cursorPos - newItem->GetPosition() );
        }
        else if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        {
            newItem->Move( VECTOR2I( schIUScale.MilsToIU( cfg->m_Drawing.default_repeat_offset_x ),
                                     schIUScale.MilsToIU( cfg->m_Drawing.default_repeat_offset_y ) ) );
        }

        // If cloning a sheet, check that we aren't going to create recursion
        if( newItem->Type() == SCH_SHEET_T )
        {
            SCH_SHEET_PATH* currentSheet = &m_frame->GetCurrentSheet();
            SCH_SHEET*      sheet = static_cast<SCH_SHEET*>( newItem );

            if( m_frame->CheckSheetForRecursion( sheet, currentSheet ) )
            {
                // Clear out the filename so that the user can pick a new one
                const wxString originalFileName = sheet->GetFileName();
                const wxString originalScreenFileName = sheet->GetScreen()->GetFileName();

                sheet->SetFileName( wxEmptyString );
                sheet->GetScreen()->SetFileName( wxEmptyString );
                restore_state = !m_frame->EditSheetProperties( sheet, currentSheet );

                if( restore_state )
                {
                    sheet->SetFileName( originalFileName );
                    sheet->GetScreen()->SetFileName( originalScreenFileName );
                }
            }
        }

        m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, newItem );
        newItem->SetFlags( IS_NEW );
        m_frame->AddToScreen( newItem, m_frame->GetScreen() );
        commit.Added( newItem, m_frame->GetScreen() );

        if( newItem->Type() == SCH_SYMBOL_T )
        {
            SCHEMATIC_SETTINGS& projSettings = m_frame->Schematic().Settings();
            int                 annotateStartNum = projSettings.m_AnnotateStartNum;
            ANNOTATE_ORDER_T    annotateOrder = static_cast<ANNOTATE_ORDER_T>( projSettings.m_AnnotateSortOrder );
            ANNOTATE_ALGO_T     annotateAlgo = static_cast<ANNOTATE_ALGO_T>( projSettings.m_AnnotateMethod );

            if( m_frame->eeconfig()->m_AnnotatePanel.automatic )
            {
                static_cast<SCH_SYMBOL*>( newItem )->ClearAnnotation( nullptr, false );
                NULL_REPORTER reporter;
                m_frame->AnnotateSymbols( &commit, ANNOTATE_SELECTION, annotateOrder, annotateAlgo,
                                          true /* recursive */, annotateStartNum, false, false, false,
                                          reporter );
            }

            // Annotation clears the selection so re-add the item
            m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, newItem );

            restore_state = !m_toolMgr->RunSynchronousAction( SCH_ACTIONS::move, &commit );
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

            m_frame->Schematic().CleanUp( &commit );
            commit.Push( _( "Repeat Item" ) );
        }
    }

    if( !newItems.Empty() )
        m_frame->SaveCopyForRepeatItem( static_cast<SCH_ITEM*>( newItems[0] ) );

    for( size_t ii = 1; ii < newItems.GetSize(); ++ii )
        m_frame->AddCopyForRepeatItem( static_cast<SCH_ITEM*>( newItems[ii] ) );

    return 0;
}


int SCH_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*           screen = m_frame->GetScreen();
    std::deque<EDA_ITEM*> items = m_selectionTool->RequestSelection( SCH_COLLECTOR::DeletableItems ).GetItems();
    SCH_COMMIT            commit( m_toolMgr );
    std::vector<VECTOR2I> pts;
    bool                  updateHierarchy = false;

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( ACTIONS::selectionClear );

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
            // Hide field
            commit.Modify( item, m_frame->GetScreen() );
            static_cast<SCH_FIELD*>( sch_item )->SetVisible( false );
        }
        else if( sch_item->Type() == SCH_TABLECELL_T )
        {
            // Clear contents of table cell
            commit.Modify( item, m_frame->GetScreen() );
            static_cast<SCH_TABLECELL*>( sch_item )->SetText( wxEmptyString );
        }
        else if( sch_item->Type() == SCH_RULE_AREA_T )
        {
            sch_item->SetFlags( STRUCT_DELETED );
            commit.Remove( item, m_frame->GetScreen() );
        }
        else if( sch_item->Type() == SCH_GROUP_T )
        {
            // Groups need to delete their children
            sch_item->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        aChild->SetFlags( STRUCT_DELETED );
                        commit.Remove( aChild, m_frame->GetScreen() );
                    },
                    RECURSE_MODE::RECURSE );

            sch_item->SetFlags( STRUCT_DELETED );
            commit.Remove( sch_item, m_frame->GetScreen() );
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


void SCH_EDIT_TOOL::editFieldText( SCH_FIELD* aField )
{
    KICAD_T    parentType = aField->GetParent() ? aField->GetParent()->Type() : SCHEMATIC_T;
    SCH_COMMIT commit( m_toolMgr );

    // Save old symbol in undo list if not already in edit, or moving.
    if( aField->GetEditFlags() == 0 ) // i.e. not edited, or moved
        commit.Modify( aField, m_frame->GetScreen() );

    if( parentType == SCH_SYMBOL_T && aField->GetId() == FIELD_T::REFERENCE )
        static_cast<SCH_ITEM*>( aField->GetParent() )->SetConnectivityDirty();

    wxString caption;

    // Use title caps for mandatory fields.  "Edit Sheet name Field" looks dorky.
    if( aField->IsMandatory() )
    {
        wxString fieldName = GetDefaultFieldName( aField->GetId(), DO_TRANSLATE );
        caption.Printf( _( "Edit %s Field" ), TitleCaps( fieldName ) );
    }
    else
    {
        caption.Printf( _( "Edit '%s' Field" ), aField->GetName() );
    }

    DIALOG_FIELD_PROPERTIES dlg( m_frame, caption, aField );

    // The footprint field dialog can invoke a KIWAY_PLAYER so we must use a quasi-modal
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    dlg.UpdateField( &commit, aField, &m_frame->GetCurrentSheet() );

    if( m_frame->eeconfig()->m_AutoplaceFields.enable || parentType == SCH_SHEET_T )
    {
        SCH_ITEM*      parent = static_cast<SCH_ITEM*>( aField->GetParent() );
        AUTOPLACE_ALGO fieldsAutoplaced = parent->GetFieldsAutoplaced();

        if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
            parent->AutoplaceFields( m_frame->GetScreen(), fieldsAutoplaced );
    }

    if( !commit.Empty() )
        commit.Push( caption );
}


int SCH_EDIT_TOOL::EditField( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION sel = m_selectionTool->RequestSelection( { SCH_FIELD_T, SCH_SYMBOL_T, SCH_PIN_T } );

    if( sel.Size() != 1 )
        return 0;

    bool      clearSelection = sel.IsHover();
    EDA_ITEM* item = sel.Front();

    if( item->Type() == SCH_FIELD_T )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

        if( ( aEvent.IsAction( &SCH_ACTIONS::editReference ) && field->GetId() != FIELD_T::REFERENCE )
            || ( aEvent.IsAction( &SCH_ACTIONS::editValue ) && field->GetId() != FIELD_T::VALUE )
            || ( aEvent.IsAction( &SCH_ACTIONS::editFootprint ) && field->GetId() != FIELD_T::FOOTPRINT ) )
        {
            item = field->GetParentSymbol();

            m_selectionTool->ClearSelection( true );

            // If the field to edit is not a symbol field, we cannot edit the ref, value or footprint
            if( item == nullptr )
                return 0;

            m_selectionTool->AddItemToSel( item );
        }
    }

    if( item->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( aEvent.IsAction( &SCH_ACTIONS::editReference ) )
        {
            editFieldText( symbol->GetField( FIELD_T::REFERENCE ) );
        }
        else if( aEvent.IsAction( &SCH_ACTIONS::editValue ) )
        {
            editFieldText( symbol->GetField( FIELD_T::VALUE ) );
        }
        else if( aEvent.IsAction( &SCH_ACTIONS::editFootprint ) )
        {
            if( !symbol->IsPower() )
                editFieldText( symbol->GetField( FIELD_T::FOOTPRINT ) );
        }
    }
    else if( item->Type() == SCH_FIELD_T )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

        editFieldText( field );

        if( !field->IsVisible() )
            clearSelection = true;
    }
    else if( item->Type() == SCH_PIN_T )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParent() );

        if( symbol )
        {
            if( aEvent.IsAction( &SCH_ACTIONS::editReference ) )
            {
                editFieldText( symbol->GetField( FIELD_T::REFERENCE ) );
            }
            else if( aEvent.IsAction( &SCH_ACTIONS::editValue ) )
            {
                editFieldText( symbol->GetField( FIELD_T::VALUE ) );
            }
            else if( aEvent.IsAction( &SCH_ACTIONS::editFootprint ) )
            {
                if( !symbol->IsPower() )
                    editFieldText( symbol->GetField( FIELD_T::FOOTPRINT ) );
            }
        }
    }

    if( clearSelection )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


int SCH_EDIT_TOOL::AutoplaceFields( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( RotatableItems );
    SCH_COMMIT     commit( m_toolMgr );
    SCH_ITEM*      head = static_cast<SCH_ITEM*>( selection.Front() );
    bool           moving = head && head->IsMoving();

    if( selection.Empty() )
        return 0;

    std::vector<SCH_ITEM*> autoplaceItems;

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

        if( item->IsType( SCH_COLLECTOR::FieldOwners ) )
            autoplaceItems.push_back( item );
        else if( item->GetParent() && item->GetParent()->IsType( SCH_COLLECTOR::FieldOwners ) )
            autoplaceItems.push_back( static_cast<SCH_ITEM*>( item->GetParent() ) );
    }

    for( SCH_ITEM* sch_item : autoplaceItems )
    {
        if( !moving && !sch_item->IsNew() )
            commit.Modify( sch_item, m_frame->GetScreen() );

        sch_item->AutoplaceFields( m_frame->GetScreen(), AUTOPLACE_MANUAL );

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
            m_toolMgr->RunAction( ACTIONS::selectionClear );
    }

    return 0;
}


int SCH_EDIT_TOOL::ChangeSymbols( const TOOL_EVENT& aEvent )
{
    SCH_SYMBOL*    selectedSymbol = nullptr;
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

    if( !selection.Empty() )
        selectedSymbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );

    DIALOG_CHANGE_SYMBOLS::MODE mode = DIALOG_CHANGE_SYMBOLS::MODE::UPDATE;

    if( aEvent.IsAction( &SCH_ACTIONS::changeSymbol ) || aEvent.IsAction( &SCH_ACTIONS::changeSymbols ) )
        mode = DIALOG_CHANGE_SYMBOLS::MODE::CHANGE;

    DIALOG_CHANGE_SYMBOLS dlg( m_frame, selectedSymbol, mode );

    // QuasiModal required to invoke symbol browser
    dlg.ShowQuasiModal();

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


int SCH_EDIT_TOOL::CycleBodyStyle( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

    if( selection.Empty() )
        return 0;

    SCH_SYMBOL* symbol = (SCH_SYMBOL*) selection.Front();
    SCH_COMMIT  commit( m_toolMgr );

    if( !symbol->IsNew() )
        commit.Modify( symbol, m_frame->GetScreen() );

    int nextBodyStyle = symbol->GetBodyStyle() + 1;

    if( nextBodyStyle > symbol->GetBodyStyleCount() )
        nextBodyStyle = 1;

    m_frame->SelectBodyStyle( symbol, nextBodyStyle );

    if( symbol->IsNew() )
        m_toolMgr->PostAction( ACTIONS::refreshPreview );

    if( !commit.Empty() )
        commit.Push( _( "Change Body Style" ) );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


int SCH_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection();
    bool           clearSelection = selection.IsHover();

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

    // If a single pin is selected, promote to its parent symbol
    if( ( selection.GetSize() == 1 ) && ( curr_item->Type() == SCH_PIN_T ) )
    {
        EDA_ITEM* parent = curr_item->GetParent();

        if( parent->Type() == SCH_SYMBOL_T )
            curr_item = parent;
    }

    switch( curr_item->Type() )
    {
    case SCH_LINE_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_JUNCTION_T:
        if( SELECTION_CONDITIONS::OnlyTypes( { SCH_ITEM_LOCATE_GRAPHIC_LINE_T } )( selection ) )
        {
            std::deque<SCH_LINE*> lines;

            for( EDA_ITEM* selItem : selection.Items() )
                lines.push_back( static_cast<SCH_LINE*>( selItem ) );

            DIALOG_LINE_PROPERTIES dlg( m_frame, lines );

            dlg.ShowModal();
        }
        else if( SELECTION_CONDITIONS::OnlyTypes( { SCH_JUNCTION_T } )( selection ) )
        {
            std::deque<SCH_JUNCTION*> junctions;

            for( EDA_ITEM* selItem : selection.Items() )
                junctions.push_back( static_cast<SCH_JUNCTION*>( selItem ) );

            DIALOG_JUNCTION_PROPS dlg( m_frame, junctions );

            dlg.ShowModal();
        }
        else if( SELECTION_CONDITIONS::OnlyTypes( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T, SCH_BUS_WIRE_ENTRY_T,
                                                    SCH_JUNCTION_T } )( selection ) )
        {
            std::deque<SCH_ITEM*> items;

            for( EDA_ITEM* selItem : selection.Items() )
                items.push_back( static_cast<SCH_ITEM*>( selItem ) );

            DIALOG_WIRE_BUS_PROPERTIES dlg( m_frame, items );

            dlg.ShowModal();
        }
        else
        {
            return 0;
        }

        break;

    case SCH_MARKER_T:
        if( SELECTION_CONDITIONS::OnlyTypes( { SCH_MARKER_T } )( selection ) )
        {
            SCH_INSPECTION_TOOL* inspectionTool = m_toolMgr->GetTool<SCH_INSPECTION_TOOL>();

            if( inspectionTool )
                inspectionTool->CrossProbe( static_cast<SCH_MARKER*>( selection.Front() ) );
        }
        break;

    case SCH_TABLECELL_T:
        if( SELECTION_CONDITIONS::OnlyTypes( { SCH_TABLECELL_T } )( selection ) )
        {
            std::vector<SCH_TABLECELL*> cells;

            for( EDA_ITEM* item : selection.Items() )
                cells.push_back( static_cast<SCH_TABLECELL*>( item ) );

            DIALOG_TABLECELL_PROPERTIES dlg( m_frame, cells );

            // QuasiModal required for syntax help and Scintilla auto-complete
            dlg.ShowQuasiModal();

            if( dlg.GetReturnValue() == DIALOG_TABLECELL_PROPERTIES::TABLECELL_PROPS_EDIT_TABLE )
            {
                SCH_TABLE*              table = static_cast<SCH_TABLE*>( cells[0]->GetParent() );
                DIALOG_TABLE_PROPERTIES tableDlg( m_frame, table );

                tableDlg.ShowModal();
            }
        }

        break;

    default:
        if( selection.Size() > 1 )
            return 0;

        EditProperties( curr_item );
    }

    if( clearSelection )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


void SCH_EDIT_TOOL::EditProperties( EDA_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case SCH_SYMBOL_T:
    {
        int         retval;
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( aItem );

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
            {
                AUTOPLACE_ALGO fieldsAutoplaced = symbol->GetFieldsAutoplaced();

                if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
                    symbol->AutoplaceFields( m_frame->GetScreen(), fieldsAutoplaced );
            }

            m_frame->OnModify();
        }
        else if( retval == SYMBOL_PROPS_EDIT_SCHEMATIC_SYMBOL )
        {
            if( KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_SCH_SYMBOL_EDITOR, true ) )
            {
                SYMBOL_EDIT_FRAME* editor = static_cast<SYMBOL_EDIT_FRAME*>( frame );

                if( wxWindow* blocking_win = editor->Kiway().GetBlockingDialog() )
                    blocking_win->Close( true );

                // The broken library symbol link indicator cannot be edited.
                if( symbol->IsMissingLibSymbol() )
                    return;

                editor->LoadSymbolFromSchematic( symbol );
                editor->Show( true );
                editor->Raise();
            }
        }
        else if( retval == SYMBOL_PROPS_EDIT_LIBRARY_SYMBOL )
        {
            if( KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_SCH_SYMBOL_EDITOR, true ) )
            {
                SYMBOL_EDIT_FRAME* editor = static_cast<SYMBOL_EDIT_FRAME*>( frame );

                if( wxWindow* blocking_win = editor->Kiway().GetBlockingDialog() )
                    blocking_win->Close( true );

                editor->LoadSymbol( symbol->GetLibId(), symbol->GetUnit(), symbol->GetBodyStyle() );
                editor->Show( true );
                editor->Raise();
            }
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

        break;
    }

    case SCH_SHEET_T:
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
        bool       isUndoable = false;
        bool       doClearAnnotation = false;
        bool       okPressed = false;
        bool       updateHierarchyNavigator = false;

        // Keep track of existing sheet paths. EditSheet() can modify this list.
        // Note that we use the validity checking/repairing version here just to make sure
        // we've got a valid hierarchy to begin with.
        SCH_SHEET_LIST originalHierarchy;
        originalHierarchy.BuildSheetList( &m_frame->Schematic().Root(), true );

        SCH_COMMIT commit( m_toolMgr );
        commit.Modify( sheet, m_frame->GetScreen() );
        okPressed = m_frame->EditSheetProperties( sheet, &m_frame->GetCurrentSheet(), &isUndoable, &doClearAnnotation,
                                                  &updateHierarchyNavigator );

        if( okPressed )
        {
            if( isUndoable )
            {
                commit.Push( _( "Edit Sheet Properties" ) );
            }
            else
            {
                std::vector<SCH_ITEM*> items;

                items.emplace_back( sheet );
                m_frame->Schematic().OnItemsRemoved( items );
                m_frame->Schematic().OnItemsAdded( items );
                m_frame->OnModify();
                m_frame->Schematic().RefreshHierarchy();
                m_frame->UpdateHierarchyNavigator();
            }
        }
        else
        {
            // If we are renaming files, the undo/redo list becomes invalid and must be cleared.
            m_frame->ClearUndoRedoList();
            m_frame->OnModify();
        }

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

        if( okPressed )
            m_frame->GetCanvas()->Refresh();

        if( updateHierarchyNavigator )
            m_frame->UpdateHierarchyNavigator();

        break;
    }

    case SCH_SHEET_PIN_T:
    {
        SCH_SHEET_PIN*              pin = static_cast<SCH_SHEET_PIN*>( aItem );
        DIALOG_SHEET_PIN_PROPERTIES dlg( m_frame, pin );

        // QuasiModal required for help dialog
        dlg.ShowQuasiModal();
        break;
    }

    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
    {
        DIALOG_TEXT_PROPERTIES dlg( m_frame, static_cast<SCH_ITEM*>( aItem ) );

        // QuasiModal required for syntax help and Scintilla auto-complete
        dlg.ShowQuasiModal();
        break;
    }

    case SCH_TABLE_T:
    {
        DIALOG_TABLE_PROPERTIES dlg( m_frame, static_cast<SCH_TABLE*>( aItem ) );

        // QuasiModal required for Scintilla auto-complete
        dlg.ShowQuasiModal();
        break;
    }

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    {
        DIALOG_LABEL_PROPERTIES dlg( m_frame, static_cast<SCH_LABEL_BASE*>( aItem ), false );

        // QuasiModal for syntax help and Scintilla auto-complete
        dlg.ShowQuasiModal();
        break;
    }

    case SCH_FIELD_T:
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( aItem );

        editFieldText( field );

        if( !field->IsVisible() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        break;
    }

    case SCH_SHAPE_T:
    {
        DIALOG_SHAPE_PROPERTIES dlg( m_frame, static_cast<SCH_SHAPE*>( aItem ) );

        dlg.ShowModal();
        break;
    }

    case SCH_BITMAP_T:
    {
        SCH_BITMAP&             bitmap = static_cast<SCH_BITMAP&>( *aItem );
        DIALOG_IMAGE_PROPERTIES dlg( m_frame, bitmap );

        if( dlg.ShowModal() == wxID_OK )
        {
            // The bitmap is cached in Opengl: clear the cache in case it has become invalid
            getView()->RecacheAllItems();
        }

        break;
    }

    case SCH_RULE_AREA_T:
    {
        DIALOG_SHAPE_PROPERTIES dlg( m_frame, static_cast<SCH_SHAPE*>( aItem ) );
        dlg.SetTitle( _( "Rule Area Properties" ) );

        dlg.ShowModal();
        break;
    }

    case SCH_NO_CONNECT_T:
    case SCH_PIN_T: break;

    case SCH_GROUP_T:
        m_toolMgr->RunAction( ACTIONS::groupProperties, static_cast<EDA_GROUP*>( static_cast<SCH_GROUP*>( aItem ) ) );

        break;

    default: // Unexpected item
        wxFAIL_MSG( wxString( "Cannot edit schematic item type " ) + aItem->GetClass() );
    }

    updateItem( aItem, true );
}


int SCH_EDIT_TOOL::ChangeTextType( const TOOL_EVENT& aEvent )
{
    KICAD_T       convertTo = aEvent.Parameter<KICAD_T>();
    SCH_SELECTION selection =
            m_selectionTool->RequestSelection( { SCH_LABEL_LOCATE_ANY_T, SCH_TEXT_T, SCH_TEXTBOX_T } );
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    for( unsigned int i = 0; i < selection.GetSize(); ++i )
    {
        SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( selection.GetItem( i ) );

        if( item && item->Type() != convertTo )
        {
            EDA_TEXT*        sourceText = dynamic_cast<EDA_TEXT*>( item );
            bool             selected = item->IsSelected();
            SCH_ITEM*        newtext = nullptr;
            VECTOR2I         position = item->GetPosition();
            wxString         txt;
            wxString         href;
            SPIN_STYLE       spinStyle = SPIN_STYLE::SPIN::RIGHT;
            LABEL_FLAG_SHAPE shape = LABEL_FLAG_SHAPE::L_UNSPECIFIED;

            wxCHECK2( sourceText, continue );

            switch( item->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            {
                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

                txt = UnescapeString( label->GetText() );
                spinStyle = label->GetSpinStyle();
                shape = label->GetShape();
                href = label->GetHyperlink();
                break;
            }

            case SCH_DIRECTIVE_LABEL_T:
            {
                SCH_DIRECTIVE_LABEL* dirlabel = static_cast<SCH_DIRECTIVE_LABEL*>( item );

                // a SCH_DIRECTIVE_LABEL has no text
                txt = _( "<empty>" );

                spinStyle = dirlabel->GetSpinStyle();
                href = dirlabel->GetHyperlink();
                break;
            }

            case SCH_TEXT_T:
            {
                SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

                txt = text->GetText();
                href = text->GetHyperlink();
                break;
            }

            case SCH_TEXTBOX_T:
            {
                SCH_TEXTBOX* textbox = static_cast<SCH_TEXTBOX*>( item );
                BOX2I        bbox = textbox->GetBoundingBox();

                bbox.SetOrigin( bbox.GetLeft() + textbox->GetMarginLeft(), bbox.GetTop() + textbox->GetMarginTop() );
                bbox.SetEnd( bbox.GetRight() - textbox->GetMarginRight(),
                             bbox.GetBottom() - textbox->GetMarginBottom() );

                if( convertTo == SCH_LABEL_T || convertTo == SCH_HIER_LABEL_T || convertTo == SCH_GLOBAL_LABEL_T )
                {
                    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item );
                    wxCHECK( text, 0 );
                    int textSize = text->GetTextSize().y;
                    bbox.Inflate( KiROUND( item->Schematic()->Settings().m_LabelSizeRatio * textSize ) );
                }

                txt = textbox->GetText();

                if( textbox->GetTextAngle().IsVertical() )
                {
                    if( textbox->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                    {
                        spinStyle = SPIN_STYLE::SPIN::BOTTOM;
                        position = VECTOR2I( bbox.Centre().x, bbox.GetOrigin().y );
                    }
                    else
                    {
                        spinStyle = SPIN_STYLE::SPIN::UP;
                        position = VECTOR2I( bbox.Centre().x, bbox.GetEnd().y );
                    }
                }
                else
                {
                    if( textbox->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                    {
                        spinStyle = SPIN_STYLE::SPIN::LEFT;
                        position = VECTOR2I( bbox.GetEnd().x, bbox.Centre().y );
                    }
                    else
                    {
                        spinStyle = SPIN_STYLE::SPIN::RIGHT;
                        position = VECTOR2I( bbox.GetOrigin().x, bbox.Centre().y );
                    }
                }

                position = m_frame->GetNearestGridPosition( position );
                href = textbox->GetHyperlink();
                break;
            }

            default: UNIMPLEMENTED_FOR( item->GetClass() ); break;
            }

            auto getValidNetname = []( const wxString& aText )
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
                new_label->SetAttributes( *sourceText, false );
                new_label->SetSpinStyle( spinStyle );
                new_label->SetHyperlink( href );

                if( item->Type() == SCH_GLOBAL_LABEL_T || item->Type() == SCH_HIER_LABEL_T )
                {
                    if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::UP )
                        new_label->MirrorVertically( position.y );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::BOTTOM )
                        new_label->MirrorVertically( position.y );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::LEFT )
                        new_label->MirrorHorizontally( position.x );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::RIGHT )
                        new_label->MirrorHorizontally( position.x );
                }

                newtext = new_label;
                break;
            }

            case SCH_GLOBAL_LABEL_T:
            {
                SCH_LABEL_BASE* new_label = new SCH_GLOBALLABEL( position, getValidNetname( txt ) );

                new_label->SetShape( shape );
                new_label->SetAttributes( *sourceText, false );
                new_label->SetSpinStyle( spinStyle );
                new_label->SetHyperlink( href );

                if( item->Type() == SCH_LABEL_T )
                {
                    if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::UP )
                        new_label->MirrorVertically( position.y );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::BOTTOM )
                        new_label->MirrorVertically( position.y );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::LEFT )
                        new_label->MirrorHorizontally( position.x );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::RIGHT )
                        new_label->MirrorHorizontally( position.x );
                }

                newtext = new_label;
                break;
            }

            case SCH_HIER_LABEL_T:
            {
                SCH_LABEL_BASE* new_label = new SCH_HIERLABEL( position, getValidNetname( txt ) );

                new_label->SetShape( shape );
                new_label->SetAttributes( *sourceText, false );
                new_label->SetSpinStyle( spinStyle );
                new_label->SetHyperlink( href );

                if( item->Type() == SCH_LABEL_T )
                {
                    if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::UP )
                        new_label->MirrorVertically( position.y );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::BOTTOM )
                        new_label->MirrorVertically( position.y );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::LEFT )
                        new_label->MirrorHorizontally( position.x );
                    else if( static_cast<SCH_LABEL_BASE*>( item )->GetSpinStyle() == SPIN_STYLE::SPIN::RIGHT )
                        new_label->MirrorHorizontally( position.x );
                }

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
                    SCH_FIELD netclass( new_label, FIELD_T::USER, wxT( "Netclass" ) );
                    netclass.SetText( txt );
                    netclass.SetTextPos( position );
                    new_label->GetFields().push_back( netclass );
                }

                new_label->SetShape( LABEL_FLAG_SHAPE::F_ROUND );
                new_label->SetAttributes( *sourceText, false );
                new_label->SetSpinStyle( spinStyle );
                new_label->SetHyperlink( href );
                newtext = new_label;
                break;
            }

            case SCH_TEXT_T:
            {
                SCH_TEXT* new_text = new SCH_TEXT( position, txt );

                new_text->SetAttributes( *sourceText, false );
                new_text->SetHyperlink( href );
                newtext = new_text;
                break;
            }

            case SCH_TEXTBOX_T:
            {
                SCH_TEXTBOX* new_textbox = new SCH_TEXTBOX( LAYER_NOTES, 0, FILL_T::NO_FILL, txt );
                BOX2I        bbox = item->GetBoundingBox();

                if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( item ) )
                    bbox.Inflate( -label->GetLabelBoxExpansion() );

                new_textbox->SetAttributes( *sourceText, false );

                bbox.SetOrigin( bbox.GetLeft() - new_textbox->GetMarginLeft(),
                                bbox.GetTop() - new_textbox->GetMarginTop() );
                bbox.SetEnd( bbox.GetRight() + new_textbox->GetMarginRight(),
                             bbox.GetBottom() + new_textbox->GetMarginBottom() );

                VECTOR2I topLeft = bbox.GetPosition();
                VECTOR2I botRight = bbox.GetEnd();

                // Add 1/20 of the margin at the end to reduce line-breaking changes.
                int slop = new_textbox->GetLegacyTextMargin() / 20;

                if( sourceText->GetTextAngle() == ANGLE_VERTICAL )
                {
                    if( sourceText->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                        botRight.y += slop;
                    else
                        topLeft.y -= slop;
                }
                else
                {
                    if( sourceText->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                        topLeft.x -= slop;
                    else
                        botRight.x += slop;
                }

                new_textbox->SetPosition( topLeft );
                new_textbox->SetEnd( botRight );

                new_textbox->SetHyperlink( href );
                newtext = new_textbox;
                break;
            }

            default: UNIMPLEMENTED_FOR( wxString::Format( "%d.", convertTo ) ); break;
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

            // Must be after SetTextSize()
            new_eda_text->SetBold( eda_text->IsBold() );
            new_eda_text->SetItalic( eda_text->IsItalic() );

            newtext->AutoplaceFields( m_frame->GetScreen(), AUTOPLACE_AUTO );

            SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( item );
            SCH_LABEL_BASE* new_label = dynamic_cast<SCH_LABEL_BASE*>( newtext );

            if( label && new_label )
            {
                new_label->AddFields( label->GetFields() );

                // A SCH_GLOBALLABEL has a specific field for intersheet references that has
                // no meaning for other labels
                std::erase_if( new_label->GetFields(),
                               [&]( SCH_FIELD& field )
                               {
                                   return field.GetId() == FIELD_T::INTERSHEET_REFS
                                          && new_label->Type() != SCH_GLOBAL_LABEL_T;
                               } );
            }

            if( selected )
                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::unselectItem, item );

            m_frame->RemoveFromScreen( item, m_frame->GetScreen() );

            if( commit->GetStatus( item, m_frame->GetScreen() ) == CHT_ADD )
                commit->Unstage( item, m_frame->GetScreen() );
            else
                commit->Removed( item, m_frame->GetScreen() );

            m_frame->AddToScreen( newtext, m_frame->GetScreen() );
            commit->Added( newtext, m_frame->GetScreen() );

            if( selected )
                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, newtext );
        }
    }

    if( !localCommit.Empty() )
        localCommit.Push( _( "Change To" ) );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


int SCH_EDIT_TOOL::JustifyText( const TOOL_EVENT& aEvent )
{
    static std::vector<KICAD_T> justifiableItems = { SCH_FIELD_T, SCH_TEXT_T, SCH_TEXTBOX_T, SCH_LABEL_T };

    SCH_SELECTION& selection = m_selectionTool->RequestSelection( justifiableItems );

    if( selection.GetSize() == 0 )
        return 0;

    SCH_ITEM*   item = static_cast<SCH_ITEM*>( selection.Front() );
    bool        moving = item->IsMoving();
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    auto setJustify = [&]( EDA_TEXT* aTextItem )
    {
        if( aEvent.Matches( ACTIONS::leftJustify.MakeEvent() ) )
            aTextItem->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( aEvent.Matches( ACTIONS::centerJustify.MakeEvent() ) )
            aTextItem->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        else
            aTextItem->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
    };

    for( EDA_ITEM* edaItem : selection )
    {
        item = static_cast<SCH_ITEM*>( edaItem );

        if( !moving )
            commit->Modify( item, m_frame->GetScreen() );

        if( item->Type() == SCH_FIELD_T )
        {
            setJustify( static_cast<SCH_FIELD*>( item ) );

            // Now that we're re-justifying a field, they're no longer autoplaced.
            static_cast<SCH_ITEM*>( item->GetParent() )->SetFieldsAutoplaced( AUTOPLACE_NONE );
        }
        else if( item->Type() == SCH_TEXT_T )
        {
            setJustify( static_cast<SCH_TEXT*>( item ) );
        }
        else if( item->Type() == SCH_TEXTBOX_T )
        {
            setJustify( static_cast<SCH_TEXTBOX*>( item ) );
        }
        else if( item->Type() == SCH_LABEL_T )
        {
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            if( label->GetTextAngle() == ANGLE_HORIZONTAL )
                setJustify( label );
        }

        m_frame->UpdateItem( item, false, true );
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
        SCH_SELECTION selectionCopy = selection;

        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        if( !localCommit.Empty() )
        {
            if( aEvent.Matches( ACTIONS::leftJustify.MakeEvent() ) )
                localCommit.Push( _( "Left Justify" ) );
            else if( aEvent.Matches( ACTIONS::centerJustify.MakeEvent() ) )
                localCommit.Push( _( "Center Justify" ) );
            else
                localCommit.Push( _( "Right Justify" ) );
        }
    }

    return 0;
}


int SCH_EDIT_TOOL::CleanupSheetPins( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SHEET_T } );
    SCH_SHEET*     sheet = (SCH_SHEET*) selection.Front();
    SCH_COMMIT     commit( m_toolMgr );

    if( !sheet || !sheet->HasUndefinedPins() )
        return 0;

    if( !IsOK( m_frame, _( "Do you wish to delete the unreferenced pins from this sheet?" ) ) )
        return 0;

    commit.Modify( sheet, m_frame->GetScreen() );

    sheet->CleanupSheet();

    updateItem( sheet, true );

    commit.Push( _( "Cleanup Sheet Pins" ) );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


int SCH_EDIT_TOOL::EditPageNumber( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SHEET_T } );

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

    dlg.SetTextValidator( wxFILTER_ALPHANUMERIC ); // No white space.

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetValue() == instance.GetPageNumber() )
        return 0;

    SCH_COMMIT commit( m_frame );

    commit.Modify( sheet, screen );

    instance.SetPageNumber( dlg.GetValue() );

    if( instance == m_frame->GetCurrentSheet() )
    {
        m_frame->GetScreen()->SetPageNumber( dlg.GetValue() );
        m_frame->OnPageSettingsChange();
    }

    commit.Push( wxS( "Change Sheet Page Number" ) );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


int SCH_EDIT_TOOL::DdAppendFile( const TOOL_EVENT& aEvent )
{
    return m_toolMgr->RunAction( SCH_ACTIONS::importSheet, aEvent.Parameter<wxString*>() );
}


int SCH_EDIT_TOOL::DdAddImage( const TOOL_EVENT& aEvent )
{
    wxString* filename = aEvent.Parameter<wxString*>();

    if( !filename )
        return 0;

    SCH_BITMAP* image = new SCH_BITMAP( VECTOR2I( 0, 0 ) );

    if( !image->GetReferenceImage().ReadImageFile( *filename ) )
    {
        wxMessageBox( wxString::Format( _( "Could not load image from '%s'." ), *filename ) );
        delete image;
        return 0;
    }

    return m_toolMgr->RunAction( SCH_ACTIONS::placeImage, image );
}


void SCH_EDIT_TOOL::collectUnits( const SCH_SELECTION&                           aSelection,
                                  std::set<std::pair<SCH_SYMBOL*, SCH_SCREEN*>>& aCollectedUnits )
{
    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            aCollectedUnits.insert( { symbol, m_frame->GetScreen() } );

            // The attributes should be kept in sync in multi-unit parts.
            // Of course the symbol must be annotated to collect other units.
            if( symbol->IsAnnotated( &m_frame->GetCurrentSheet() ) )
            {
                wxString ref = symbol->GetRef( &m_frame->GetCurrentSheet() );
                int      unit = symbol->GetUnit();
                LIB_ID   libId = symbol->GetLibId();

                for( SCH_SHEET_PATH& sheet : m_frame->Schematic().Hierarchy() )
                {
                    SCH_SCREEN*              screen = sheet.LastScreen();
                    std::vector<SCH_SYMBOL*> otherUnits;

                    CollectOtherUnits( ref, unit, libId, sheet, &otherUnits );

                    for( SCH_SYMBOL* otherUnit : otherUnits )
                        aCollectedUnits.insert( { otherUnit, screen } );
                }
            }
        }
    }
}


int SCH_EDIT_TOOL::SetAttribute( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );
    SCH_COMMIT     commit( m_toolMgr );

    std::set<std::pair<SCH_SYMBOL*, SCH_SCREEN*>> collectedUnits;

    collectUnits( selection, collectedUnits );
    bool new_state = false;

    for( const auto& [symbol, _] : collectedUnits )
    {
        if( ( aEvent.IsAction( &SCH_ACTIONS::setDNP ) && !symbol->GetDNP() )
            || ( aEvent.IsAction( &SCH_ACTIONS::setExcludeFromSimulation ) && !symbol->GetExcludedFromSim() )
            || ( aEvent.IsAction( &SCH_ACTIONS::setExcludeFromBOM ) && !symbol->GetExcludedFromBOM() )
            || ( aEvent.IsAction( &SCH_ACTIONS::setExcludeFromBoard ) && !symbol->GetExcludedFromBoard() ) )
        {
            new_state = true;
            break;
        }
    }

    for( const auto& [symbol, screen] : collectedUnits )
    {
        commit.Modify( symbol, screen );

        if( aEvent.IsAction( &SCH_ACTIONS::setDNP ) )
            symbol->SetDNP( new_state );

        if( aEvent.IsAction( &SCH_ACTIONS::setExcludeFromSimulation ) )
            symbol->SetExcludedFromSim( new_state );

        if( aEvent.IsAction( &SCH_ACTIONS::setExcludeFromBOM ) )
            symbol->SetExcludedFromBOM( new_state );

        if( aEvent.IsAction( &SCH_ACTIONS::setExcludeFromBoard ) )
            symbol->SetExcludedFromBoard( new_state );
    }

    if( !commit.Empty() )
        commit.Push( _( "Toggle Attribute" ) );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


wxString SCH_EDIT_TOOL::FixERCErrorMenuText( const std::shared_ptr<RC_ITEM>& aERCItem )
{
    if( aERCItem->GetErrorCode() == ERCE_SIMULATION_MODEL || aERCItem->GetErrorCode() == ERCE_FOOTPRINT_FILTERS
        || aERCItem->GetErrorCode() == ERCE_FOOTPRINT_LINK_ISSUES )
    {
        return _( "Edit Symbol Properties..." );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_ISSUES )
    {
        return m_frame->GetRunMenuCommandDescription( SCH_ACTIONS::showSymbolLibTable );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_MISMATCH )
    {
        return m_frame->GetRunMenuCommandDescription( SCH_ACTIONS::updateSymbol );
    }
    else if( aERCItem->GetErrorCode() == ERCE_UNANNOTATED || aERCItem->GetErrorCode() == ERCE_DUPLICATE_REFERENCE )
    {
        return m_frame->GetRunMenuCommandDescription( SCH_ACTIONS::annotate );
    }
    else if( aERCItem->GetErrorCode() == ERCE_UNDEFINED_NETCLASS )
    {
        return _( "Edit Netclasses..." );
    }

    return wxEmptyString;
}


void SCH_EDIT_TOOL::FixERCError( const std::shared_ptr<RC_ITEM>& aERCItem )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, /* void */ );

    if( aERCItem->GetErrorCode() == ERCE_SIMULATION_MODEL || aERCItem->GetErrorCode() == ERCE_FOOTPRINT_FILTERS
        || aERCItem->GetErrorCode() == ERCE_FOOTPRINT_LINK_ISSUES )
    {
        if( EDA_ITEM* item = frame->ResolveItem( aERCItem->GetMainItemID() ) )
            EditProperties( item );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_ISSUES )
    {
        m_toolMgr->RunAction( SCH_ACTIONS::showSymbolLibTable );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_MISMATCH )
    {
        EDA_ITEM* item = frame->ResolveItem( aERCItem->GetMainItemID() );

        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
        {
            DIALOG_CHANGE_SYMBOLS dlg( frame, symbol, DIALOG_CHANGE_SYMBOLS::MODE::CHANGE );
            dlg.ShowQuasiModal();
        }
    }
    else if( aERCItem->GetErrorCode() == ERCE_UNANNOTATED || aERCItem->GetErrorCode() == ERCE_DUPLICATE_REFERENCE )
    {
        m_toolMgr->RunAction( SCH_ACTIONS::annotate );
    }
    else if( aERCItem->GetErrorCode() == ERCE_UNDEFINED_NETCLASS )
    {
        frame->ShowSchematicSetupDialog( _( "Net Classes" ) );
    }
}


void SCH_EDIT_TOOL::setTransitions()
{
    // clang-format off
    Go( &SCH_EDIT_TOOL::RepeatDrawItem,     SCH_ACTIONS::repeatDrawItem.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorV.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorH.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Swap,               SCH_ACTIONS::swap.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SwapPinLabels,      SCH_ACTIONS::swapPinLabels.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SwapUnitLabels,     SCH_ACTIONS::swapUnitLabels.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SwapPins,           SCH_ACTIONS::swapPins.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DoDelete,           ACTIONS::doDelete.MakeEvent() );
    Go( &SCH_EDIT_TOOL::InteractiveDelete,  ACTIONS::deleteTool.MakeEvent() );

    Go( &SCH_EDIT_TOOL::Increment,          ACTIONS::increment.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Increment,          ACTIONS::incrementPrimary.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Increment,          ACTIONS::decrementPrimary.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Increment,          ACTIONS::incrementSecondary.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Increment,          ACTIONS::decrementSecondary.MakeEvent() );

    Go( &SCH_EDIT_TOOL::Properties,         SCH_ACTIONS::properties.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editReference.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editValue.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editFootprint.MakeEvent() );
    Go( &SCH_EDIT_TOOL::AutoplaceFields,    SCH_ACTIONS::autoplaceFields.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      SCH_ACTIONS::changeSymbols.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      SCH_ACTIONS::updateSymbols.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      SCH_ACTIONS::changeSymbol.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeSymbols,      SCH_ACTIONS::updateSymbol.MakeEvent() );
    Go( &SCH_EDIT_TOOL::CycleBodyStyle,     SCH_ACTIONS::cycleBodyStyle.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toHLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toGLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toDLabel.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toText.MakeEvent() );
    Go( &SCH_EDIT_TOOL::ChangeTextType,     SCH_ACTIONS::toTextBox.MakeEvent() );
    Go( &SCH_EDIT_TOOL::JustifyText,        ACTIONS::leftJustify.MakeEvent() );
    Go( &SCH_EDIT_TOOL::JustifyText,        ACTIONS::centerJustify.MakeEvent() );
    Go( &SCH_EDIT_TOOL::JustifyText,        ACTIONS::rightJustify.MakeEvent() );


    Go( &SCH_EDIT_TOOL::SetAttribute,       SCH_ACTIONS::setDNP.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SetAttribute,       SCH_ACTIONS::setExcludeFromBOM.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SetAttribute,       SCH_ACTIONS::setExcludeFromBoard.MakeEvent() );
    Go( &SCH_EDIT_TOOL::SetAttribute,       SCH_ACTIONS::setExcludeFromSimulation.MakeEvent() );

    Go( &SCH_EDIT_TOOL::CleanupSheetPins,   SCH_ACTIONS::cleanupSheetPins.MakeEvent() );
    Go( &SCH_EDIT_TOOL::GlobalEdit,         SCH_ACTIONS::editTextAndGraphics.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditPageNumber,     SCH_ACTIONS::editPageNumber.MakeEvent() );

    Go( &SCH_EDIT_TOOL::DdAppendFile,       SCH_ACTIONS::ddAppendFile.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DdAddImage,        SCH_ACTIONS::ddAddImage.MakeEvent() );
    // clang-format on
}
