/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tools/board_inspection_tool.h"

#include <bitmaps.h>
#include <board_loader.h>
#include <collectors.h>
#include <dialogs/dialog_kicad_diff.h>
#include <diff_merge/pcb_diff_canvas_context.h>
#include <diff_merge/pcb_geometry_extractor.h>
#include <diff_merge/pcb_differ.h>
#include <footprint.h>
#include <pcb_io/pcb_io_mgr.h>
#include <settings/settings_manager.h>
#include <kiway.h>
#include <local_history.h>
#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <pcb_group.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/edit_tool.h>
#include <tools/drc_tool.h>
#include <pcb_painter.h>
#include <connectivity/connectivity_data.h>
#include <drc/drc_engine.h>
#include <dialogs/dialog_board_statistics.h>
#include <dialogs/dialog_book_reporter.h>
#include <dialogs/panel_setup_rules_base.h>
#include <dialogs/dialog_footprint_associations.h>
#include <dialogs/dialog_drc.h>
#include <kiplatform/ui.h>
#include <status_popup.h>
#include <string_utils.h>
#include <footprint_library_adapter.h>
#include <pcb_painter.h>
#include <pcb_shape.h>
#include <widgets/appearance_controls.h>
#include <widgets/wx_html_report_box.h>
#include <widgets/footprint_diff_widget.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <drc/drc_item.h>
#include <pad.h>
#include <pcb_track.h>
#include <project_pcb.h>
#include <view/view_controls.h>


BOARD_INSPECTION_TOOL::BOARD_INSPECTION_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InspectionTool" ),
        m_frame( nullptr )
{
    m_dynamicData     = nullptr;
}


class NET_CONTEXT_MENU : public ACTION_MENU
{
public:
    NET_CONTEXT_MENU() :
            ACTION_MENU( true )
    {
        SetIcon( BITMAPS::show_ratsnest );
        SetTitle( _( "Net Inspection Tools" ) );

        Add( PCB_ACTIONS::showNetInRatsnest );
        Add( PCB_ACTIONS::hideNetInRatsnest );
        AppendSeparator();
        Add( PCB_ACTIONS::highlightNetSelection );
        Add( PCB_ACTIONS::clearHighlight );
    }

private:
    ACTION_MENU* create() const override
    {
        return new NET_CONTEXT_MENU();
    }
};


bool BOARD_INSPECTION_TOOL::Init()
{
    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    std::shared_ptr<NET_CONTEXT_MENU> netSubMenu = std::make_shared<NET_CONTEXT_MENU>();
    netSubMenu->SetTool( this );

    // Only show the net menu if all items in the selection are connectable
    auto showNetMenuFunc =
            []( const SELECTION& aSelection )
            {
                if( aSelection.Empty() )
                    return false;

                for( const EDA_ITEM* item : aSelection )
                {
                    switch( item->Type() )
                    {
                    case PCB_TRACE_T:
                    case PCB_ARC_T:
                    case PCB_VIA_T:
                    case PCB_PAD_T:
                    case PCB_ZONE_T:
                        continue;

                    case PCB_SHAPE_T:
                    {
                        if( !static_cast<const PCB_SHAPE*>( item )->IsOnCopperLayer() )
                            return false;
                        else
                            continue;
                    }

                    default:
                        return false;
                    }
                }

                return true;
            };

    CONDITIONAL_MENU& menu = selectionTool->GetToolMenu().GetMenu();

    selectionTool->GetToolMenu().RegisterSubMenu( netSubMenu );

    menu.AddMenu( netSubMenu.get(), showNetMenuFunc, 100 );

    return true;
}


void BOARD_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


int BOARD_INSPECTION_TOOL::ShowBoardStatistics( const TOOL_EVENT& aEvent )
{
    DIALOG_BOARD_STATISTICS dialog( m_frame );
    dialog.ShowModal();
    return 0;
}


std::unique_ptr<DRC_ENGINE> BOARD_INSPECTION_TOOL::makeDRCEngine( bool* aCompileError,
                                                                  bool* aCourtyardError )
{
    auto engine = std::make_unique<DRC_ENGINE>( m_frame->GetBoard(),
                                                &m_frame->GetBoard()->GetDesignSettings() );

    try
    {
        engine->InitEngine( m_frame->GetBoard()->GetDesignRulesPath() );
    }
    catch( PARSE_ERROR& )
    {
        if( aCompileError )
            *aCompileError = true;
    }

    for( ZONE* zone : m_frame->GetBoard()->Zones() )
        zone->CacheBoundingBox();

    for( FOOTPRINT* footprint : m_frame->GetBoard()->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
            zone->CacheBoundingBox();

        footprint->BuildCourtyardCaches();

        if( aCourtyardError && ( footprint->GetFlags() & MALFORMED_COURTYARDS ) != 0 )
            *aCourtyardError = true;
    }

    return engine;
}


bool isNPTHPad( BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_PAD_T
            && static_cast<PAD*>( aItem )->GetAttribute() == PAD_ATTRIB::NPTH;
}


wxString BOARD_INSPECTION_TOOL::getItemDescription( BOARD_ITEM* aItem )
{
    // Null items have no description
    if( !aItem )
        return wxString();

    wxString msg = aItem->GetItemDescription( m_frame, true );

    if( aItem->IsConnected() && !isNPTHPad( aItem ) )
    {
        BOARD_CONNECTED_ITEM* cItem = static_cast<BOARD_CONNECTED_ITEM*>( aItem );

        msg += wxS( " " )
               + wxString::Format( _( "[netclass %s]" ),
                                   cItem->GetEffectiveNetClass()->GetHumanReadableName() );
    }

    return msg;
};


void BOARD_INSPECTION_TOOL::filterCollectorForInspection( GENERAL_COLLECTOR& aCollector,
                                                          const VECTOR2I& aPos )
{
    std::vector<BOARD_ITEM*> toAdd;

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        if( aCollector[i]->Type() == PCB_GROUP_T )
        {
            PCB_GROUP* group = static_cast<PCB_GROUP*>( aCollector[i] );

            group->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        if( child->Type() == PCB_GROUP_T )
                            return;

                        if( !child->HitTest( aPos ) )
                            return;

                        toAdd.push_back( child );

                        if( child->Type() == PCB_FOOTPRINT_T )
                        {
                            for( PAD* pad : static_cast<FOOTPRINT*>( child )->Pads() )
                            {
                                if( pad->HitTest( aPos ) )
                                    toAdd.push_back( pad );
                            }
                        }
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

    for( BOARD_ITEM* item : toAdd )
        aCollector.Append( item );

    bool hasPadOrTrack = false;

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        KICAD_T type = aCollector[i]->Type();

        if( type == PCB_PAD_T || type == PCB_VIA_T || type == PCB_TRACE_T
            || type == PCB_ARC_T || type == PCB_ZONE_T )
        {
            hasPadOrTrack = true;
            break;
        }
    }

    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
    {
        BOARD_ITEM* item = aCollector[i];

        if( hasPadOrTrack && item->Type() == PCB_FOOTPRINT_T )
        {
            aCollector.Remove( i );
            continue;
        }

        if( item->Type() == PCB_GROUP_T )
            aCollector.Remove( i );
    }
}


BOARD_ITEM* BOARD_INSPECTION_TOOL::pickItemForInspection( const TOOL_EVENT& aEvent,
                                                          const wxString& aPrompt,
                                                          const std::vector<KICAD_T>& aTypes,
                                                          BOARD_ITEM* aLockedHighlight )
{
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_PICKER_TOOL*    picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    STATUS_TEXT_POPUP   statusPopup( m_frame );
    BOARD_ITEM*         pickedItem = nullptr;
    BOARD_ITEM*         highlightedItem = nullptr;
    bool                done = false;

    statusPopup.SetText( aPrompt );

    picker->SetCursor( KICURSOR::BULLSEYE );
    picker->SetSnapping( false );
    picker->ClearHandlers();

    picker->SetClickHandler(
            [&]( const VECTOR2D& aPoint ) -> bool
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
                GENERAL_COLLECTOR        collector;

                collector.Collect( m_frame->GetBoard(), aTypes, aPoint, guide );

                for( int i = collector.GetCount() - 1; i >= 0; --i )
                {
                    if( !selTool->Selectable( collector[i] ) )
                        collector.Remove( i );
                }

                filterCollectorForInspection( collector, aPoint );

                if( collector.GetCount() > 1 )
                    selTool->GuessSelectionCandidates( collector, aPoint );

                if( collector.GetCount() == 0 )
                    return true;

                pickedItem = collector[0];
                statusPopup.Hide();

                return false;
            } );

    picker->SetMotionHandler(
            [&]( const VECTOR2D& aPos )
            {
                statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );

                GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
                GENERAL_COLLECTOR        collector;

                collector.Collect( m_frame->GetBoard(), aTypes, aPos, guide );

                for( int i = collector.GetCount() - 1; i >= 0; --i )
                {
                    if( !selTool->Selectable( collector[i] ) )
                        collector.Remove( i );
                }

                filterCollectorForInspection( collector, aPos );

                if( collector.GetCount() > 1 )
                    selTool->GuessSelectionCandidates( collector, aPos );

                BOARD_ITEM* item = collector.GetCount() >= 1 ? collector[0] : nullptr;

                if( highlightedItem != item )
                {
                    if( highlightedItem && highlightedItem != aLockedHighlight )
                        selTool->UnbrightenItem( highlightedItem );

                    highlightedItem = item;

                    if( highlightedItem && highlightedItem != aLockedHighlight )
                        selTool->BrightenItem( highlightedItem );
                }
            } );

    picker->SetCancelHandler(
            [&]()
            {
                if( highlightedItem && highlightedItem != aLockedHighlight )
                    selTool->UnbrightenItem( highlightedItem );

                highlightedItem = nullptr;
                statusPopup.Hide();
                done = true;
            } );

    picker->SetFinalizeHandler(
            [&]( const int& aFinalState )
            {
                if( highlightedItem && highlightedItem != aLockedHighlight )
                    selTool->UnbrightenItem( highlightedItem );

                highlightedItem = nullptr;

                if( !pickedItem )
                    done = true;
            } );

    statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
    statusPopup.Popup();
    m_frame->GetCanvas()->SetStatusPopup( statusPopup.GetPanel() );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    while( !done && !pickedItem )
    {
        if( TOOL_EVENT* evt = Wait() )
            evt->SetPassEvent();
        else
            break;
    }

    picker->ClearHandlers();
    m_frame->GetCanvas()->SetStatusPopup( nullptr );

    return pickedItem;
}


void BOARD_INSPECTION_TOOL::reportCompileError( REPORTER* r )
{
    r->Report( "" );
    r->Report( _( "Report incomplete: could not compile custom design rules." )
               + wxS( "&nbsp;&nbsp;" )
               + wxS( "<a href='$CUSTOM_RULES'>" ) + _( "Show design rules." ) + wxS( "</a>" ) );
}


void BOARD_INSPECTION_TOOL::reportHeader( const wxString& aTitle, BOARD_ITEM* a, REPORTER* r )
{
    r->Report( wxT( "<h7>" ) + EscapeHTML( aTitle ) + wxT( "</h7>" ) );
    r->Report( wxT( "<ul><li>" ) + EscapeHTML( getItemDescription( a ) ) + wxT( "</li></ul>" ) );
}


void BOARD_INSPECTION_TOOL::reportHeader( const wxString& aTitle, BOARD_ITEM* a, BOARD_ITEM* b,
                                          REPORTER* r )
{
    r->Report( wxT( "<h7>" ) + EscapeHTML( aTitle ) + wxT( "</h7>" ) );
    r->Report( wxT( "<ul><li>" ) + EscapeHTML( getItemDescription( a ) ) + wxT( "</li>" )
               + wxT( "<li>" ) + EscapeHTML( getItemDescription( b ) ) + wxT( "</li></ul>" ) );
}


void BOARD_INSPECTION_TOOL::reportHeader( const wxString& aTitle, BOARD_ITEM* a, BOARD_ITEM* b,
                                          PCB_LAYER_ID aLayer, REPORTER* r )
{
    wxString layerStr = _( "Layer" ) + wxS( " " ) + m_frame->GetBoard()->GetLayerName( aLayer );

    r->Report( wxT( "<h7>" ) + EscapeHTML( aTitle ) + wxT( "</h7>" ) );
    r->Report( wxT( "<ul><li>" ) + EscapeHTML( layerStr ) + wxT( "</li>" )
               + wxT( "<li>" ) + EscapeHTML( getItemDescription( a ) ) + wxT( "</li>" )
               + wxT( "<li>" ) + EscapeHTML( getItemDescription( b ) ) + wxT( "</li></ul>" ) );
}


namespace
{
class VECTOR_REPORTER : public REPORTER
{
public:
    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        m_messages.push_back( aText );
        return *this;
    }

    bool      HasMessage() const override { return !m_messages.empty(); }
    EDA_UNITS GetUnits() const override { return EDA_UNITS::UNSCALED; }

    std::vector<wxString> m_messages;
};
} // namespace


wxString reportMin( PCB_BASE_FRAME* aFrame, DRC_CONSTRAINT& aConstraint )
{
    if( aConstraint.m_Value.HasMin() )
        return aFrame->StringFromValue( aConstraint.m_Value.Min(), true );
    else
        return wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
}


wxString reportOpt( PCB_BASE_FRAME* aFrame, DRC_CONSTRAINT& aConstraint )
{
    if( aConstraint.m_Value.HasOpt() )
        return aFrame->StringFromValue( aConstraint.m_Value.Opt(), true );
    else
        return wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
}


wxString reportMax( PCB_BASE_FRAME* aFrame, DRC_CONSTRAINT& aConstraint )
{
    if( aConstraint.m_Value.HasMax() )
        return aFrame->StringFromValue( aConstraint.m_Value.Max(), true );
    else
        return wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
}


wxString BOARD_INSPECTION_TOOL::InspectDRCErrorMenuText( const std::shared_ptr<RC_ITEM>& aDRCItem )
{
    if( aDRCItem->GetErrorCode() == DRCE_CLEARANCE
            || aDRCItem->GetErrorCode() == DRCE_EDGE_CLEARANCE
            || aDRCItem->GetErrorCode() == DRCE_HOLE_CLEARANCE
            || aDRCItem->GetErrorCode() == DRCE_DRILLED_HOLES_TOO_CLOSE
            || aDRCItem->GetErrorCode() == DRCE_STARVED_THERMAL )
    {
        return m_frame->GetRunMenuCommandDescription( PCB_ACTIONS::inspectClearance );
    }
    else if( aDRCItem->GetErrorCode() == DRCE_TEXT_HEIGHT
            || aDRCItem->GetErrorCode() == DRCE_TEXT_THICKNESS
            || aDRCItem->GetErrorCode() == DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG
            || aDRCItem->GetErrorCode() == DRCE_TRACK_WIDTH
            || aDRCItem->GetErrorCode() == DRCE_TRACK_ANGLE
            || aDRCItem->GetErrorCode() == DRCE_TRACK_SEGMENT_LENGTH
            || aDRCItem->GetErrorCode() == DRCE_VIA_DIAMETER
            || aDRCItem->GetErrorCode() == DRCE_ANNULAR_WIDTH
            || aDRCItem->GetErrorCode() == DRCE_DRILL_OUT_OF_RANGE
            || aDRCItem->GetErrorCode() == DRCE_MICROVIA_DRILL_OUT_OF_RANGE
            || aDRCItem->GetErrorCode() == DRCE_CONNECTION_WIDTH
            || aDRCItem->GetErrorCode() == DRCE_ASSERTION_FAILURE )
    {
        return m_frame->GetRunMenuCommandDescription( PCB_ACTIONS::inspectConstraints );
    }
    else if( aDRCItem->GetErrorCode() == DRCE_LIB_FOOTPRINT_MISMATCH )
    {
        return m_frame->GetRunMenuCommandDescription( PCB_ACTIONS::diffFootprint );
    }

    return wxEmptyString;
}


void BOARD_INSPECTION_TOOL::InspectDRCError( const std::shared_ptr<RC_ITEM>& aDRCItem )
{
    DRC_TOOL* drcTool = m_toolMgr->GetTool<DRC_TOOL>();

    wxCHECK( drcTool && m_frame, /* void */ );

    BOARD_ITEM*           a = m_frame->GetBoard()->ResolveItem( aDRCItem->GetMainItemID() );
    BOARD_ITEM*           b = m_frame->GetBoard()->ResolveItem( aDRCItem->GetAuxItemID() );
    BOARD_CONNECTED_ITEM* ac = dynamic_cast<BOARD_CONNECTED_ITEM*>( a );
    BOARD_CONNECTED_ITEM* bc = dynamic_cast<BOARD_CONNECTED_ITEM*>( b );
    PCB_LAYER_ID          layer = m_frame->GetActiveLayer();

    if( aDRCItem->GetErrorCode() == DRCE_LIB_FOOTPRINT_MISMATCH )
    {
        if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( a ) )
            DiffFootprint( footprint, drcTool->GetDRCDialog() );

        return;
    }

    DIALOG_BOOK_REPORTER* dialog = m_frame->GetInspectDrcErrorDialog();
    wxCHECK( dialog, /* void */ );

    dialog->DeleteAllPages();

    bool compileError = false;
    bool courtyardError = false;
    std::unique_ptr<DRC_ENGINE> drcEngine = makeDRCEngine( &compileError, &courtyardError );

    WX_HTML_REPORT_BOX* r = nullptr;
    DRC_CONSTRAINT      constraint;
    int                 clearance = 0;
    wxString            clearanceStr;

    switch( aDRCItem->GetErrorCode() )
    {
    case DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG:
    {
        for( KIID id : aDRCItem->GetIDs() )
        {
            bc = dynamic_cast<BOARD_CONNECTED_ITEM*>( m_frame->GetBoard()->ResolveItem( id, true ) );

            if( ac && bc && ac->GetNetCode() != bc->GetNetCode() )
                break;
        }

        r = dialog->AddHTMLPage( _( "Uncoupled Length" ) );
        reportHeader( _( "Diff pair uncoupled length resolution for:" ), ac, bc, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( MAX_UNCOUPLED_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved max uncoupled length: %s." ),
                                     reportMax( m_frame, constraint ) ) );
        break;
    }

    case DRCE_TEXT_HEIGHT:
        r = dialog->AddHTMLPage( _( "Text Height" ) );
        reportHeader( _( "Text height resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( TEXT_HEIGHT_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved height constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_TEXT_THICKNESS:
        r = dialog->AddHTMLPage( _( "Text Thickness" ) );
        reportHeader( _( "Text thickness resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( TEXT_THICKNESS_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved thickness constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_TRACK_WIDTH:
        r = dialog->AddHTMLPage( _( "Track Width" ) );
        reportHeader( _( "Track width resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( TRACK_WIDTH_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved width constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_TRACK_ANGLE:
        r = dialog->AddHTMLPage( _( "Track Angle" ) );
        reportHeader( _( "Track Angle resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( TRACK_ANGLE_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved angle constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_TRACK_SEGMENT_LENGTH:
        r = dialog->AddHTMLPage( _( "Track Segment Length" ) );
        reportHeader( _( "Track segment length resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( TRACK_SEGMENT_LENGTH_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved segment length constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_CONNECTION_WIDTH:
        r = dialog->AddHTMLPage( _( "Connection Width" ) );
        reportHeader( _( "Connection width resolution for:" ), a, b, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( CONNECTION_WIDTH_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved min connection width: %s." ),
                                     reportMin( m_frame, constraint ) ) );
        break;

    case DRCE_VIA_DIAMETER:
        r = dialog->AddHTMLPage( _( "Via Diameter" ) );
        reportHeader( _( "Via diameter resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( VIA_DIAMETER_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved diameter constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_ANNULAR_WIDTH:
        r = dialog->AddHTMLPage( _( "Via Annulus" ) );
        reportHeader( _( "Via annular width resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved annular width constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_DRILL_OUT_OF_RANGE:
    case DRCE_MICROVIA_DRILL_OUT_OF_RANGE:
        r = dialog->AddHTMLPage( _( "Hole Size" ) );
        reportHeader( _( "Hole size resolution for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( HOLE_SIZE_CONSTRAINT, a, b, layer, r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved hole size constraints: min %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );
        break;

    case DRCE_HOLE_CLEARANCE:
        r = dialog->AddHTMLPage( _( "Hole Clearance" ) );
        reportHeader( _( "Hole clearance resolution for:" ), a, b, r );

        if( compileError )
            reportCompileError( r );

        if( ac && bc && ac->GetNetCode() == bc->GetNetCode() )
        {
            r->Report( "" );
            r->Report( _( "Items belong to the same net. Clearance is 0." ) );
        }
        else
        {
            constraint = drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();
            clearanceStr = m_frame->StringFromValue( clearance, true );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ), clearanceStr ) );
        }

        r->Report( "" );
        r->Report( "" );
        r->Report( "" );
        reportHeader( _( "Physical hole clearance resolution for:" ), a, b, layer, r );

        constraint = drcEngine->EvalRules( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, a, b, layer, r );
        clearance = constraint.m_Value.Min();
        clearanceStr = m_frame->StringFromValue( clearance, true );

        if( !drcEngine->HasRulesForConstraintType( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT ) )
        {
            r->Report( "" );
            r->Report( _( "No 'physical_hole_clearance' constraints defined." ) );
        }
        else
        {
            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ), clearanceStr ) );
        }

        break;

    case DRCE_DRILLED_HOLES_TOO_CLOSE:
        r = dialog->AddHTMLPage( _( "Hole to Hole" ) );
        reportHeader( _( "Hole-to-hole clearance resolution for:" ), a, b, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( HOLE_TO_HOLE_CONSTRAINT, a, b, UNDEFINED_LAYER, r );
        clearance = constraint.m_Value.Min();
        clearanceStr = m_frame->StringFromValue( clearance, true );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved min clearance: %s." ), clearanceStr ) );
        break;

    case DRCE_EDGE_CLEARANCE:
        r = dialog->AddHTMLPage( _( "Edge Clearance" ) );
        reportHeader( _( "Edge clearance resolution for:" ), a, b, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine->EvalRules( EDGE_CLEARANCE_CONSTRAINT, a, b, layer, r );
        clearance = constraint.m_Value.Min();
        clearanceStr = m_frame->StringFromValue( clearance, true );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved min clearance: %s." ), clearanceStr ) );
        break;

    case DRCE_CLEARANCE:
        if( a->Type() == PCB_TRACE_T || a->Type() == PCB_ARC_T )
        {
            layer = a->GetLayer();
        }
        else if( b->Type() == PCB_TRACE_T || b->Type() == PCB_ARC_T )
        {
            layer = b->GetLayer();
        }
        else if( a->Type() == PCB_PAD_T
               && static_cast<PAD*>( a )->GetAttribute() == PAD_ATTRIB::SMD )
        {
            PAD* pad = static_cast<PAD*>( a );

            if( pad->IsOnLayer( F_Cu ) )
                layer = F_Cu;
            else
                layer = B_Cu;
        }
        else if( b->Type() == PCB_PAD_T
               && static_cast<PAD*>( a )->GetAttribute() == PAD_ATTRIB::SMD )
        {
            PAD* pad = static_cast<PAD*>( b );

            if( pad->IsOnLayer( F_Cu ) )
                layer = F_Cu;
            else
                layer = B_Cu;
        }

        r = dialog->AddHTMLPage( _( "Clearance" ) );
        reportHeader( _( "Clearance resolution for:" ), a, b, layer, r );

        if( compileError )
            reportCompileError( r );

        if( ac && bc && ac->GetNetCode() == bc->GetNetCode() )
        {
            r->Report( "" );
            r->Report( _( "Items belong to the same net. Clearance is 0." ) );
        }
        else
        {
            constraint = drcEngine->EvalRules( CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();
            clearanceStr = m_frame->StringFromValue( clearance, true );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ), clearanceStr ) );
        }

        r->Report( "" );
        r->Report( "" );
        r->Report( "" );
        reportHeader( _( "Physical clearance resolution for:" ), a, b, layer, r );

        constraint = drcEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, a, b, layer, r );
        clearance = constraint.m_Value.Min();
        clearanceStr = m_frame->StringFromValue( clearance, true );

        if( !drcEngine->HasRulesForConstraintType( PHYSICAL_CLEARANCE_CONSTRAINT ) )
        {
            r->Report( "" );
            r->Report( _( "No 'physical_clearance' constraints defined." ) );
        }
        else
        {
            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ), clearanceStr ) );
        }

        break;

    case DRCE_ASSERTION_FAILURE:
        r = dialog->AddHTMLPage( _( "Assertions" ) );
        reportHeader( _( "Assertions for:" ), a, r );

        if( compileError )
            reportCompileError( r );

        drcEngine->ProcessAssertions( a, []( const DRC_CONSTRAINT* c ){}, r );
        break;

    default:
        return;
    }

    r->Flush();

    KIPLATFORM::UI::ReparentWindow( dialog, drcTool->GetDRCDialog() );
    dialog->Show( true );
}


int BOARD_INSPECTION_TOOL::InspectClearance( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxCHECK( selTool, 0 );

    const PCB_SELECTION& selection = selTool->GetSelection();
    BOARD_ITEM*          firstItem = nullptr;
    BOARD_ITEM*          secondItem = nullptr;

    if( selection.Size() == 2 )
    {
        if( !selection.GetItem( 0 )->IsBOARD_ITEM() || !selection.GetItem( 1 )->IsBOARD_ITEM() )
            return 0;

        firstItem = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) );
        secondItem = static_cast<BOARD_ITEM*>( selection.GetItem( 1 ) );

        reportClearance( firstItem, secondItem );
        return 0;
    }

    // Selection size is not 2, so we need to use picker mode.
    // If there is one item selected, use it as the first item.
    if( selection.Size() == 1 && selection.GetItem( 0 )->IsBOARD_ITEM() )
        firstItem = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) );

    static const std::vector<KICAD_T> clearanceTypes = {
        PCB_PAD_T,
        PCB_VIA_T,
        PCB_TRACE_T,
        PCB_ARC_T,
        PCB_ZONE_T,
        PCB_SHAPE_T,
        PCB_FOOTPRINT_T,
        PCB_GROUP_T
    };

    Activate();

    if( !firstItem )
    {
        firstItem = pickItemForInspection( aEvent,
                                           _( "Select first item for clearance resolution..." ),
                                           clearanceTypes, nullptr );

        if( !firstItem )
            return 0;
    }

    // Keep the first item highlighted while selecting the second
    selTool->BrightenItem( firstItem );

    secondItem = pickItemForInspection( aEvent,
                                        _( "Select second item for clearance resolution..." ),
                                        clearanceTypes, firstItem );

    selTool->UnbrightenItem( firstItem );

    if( !secondItem )
        return 0;

    if( firstItem == secondItem )
    {
        m_frame->ShowInfoBarError( _( "Select two different items for clearance resolution." ) );
        return 0;
    }

    reportClearance( firstItem, secondItem );

    return 0;
}


void BOARD_INSPECTION_TOOL::reportClearance( BOARD_ITEM* aItemA, BOARD_ITEM* aItemB )
{
    wxCHECK( m_frame && aItemA && aItemB, /* void */ );

    BOARD_ITEM* a = aItemA;
    BOARD_ITEM* b = aItemB;

    if( a->Type() == PCB_GROUP_T )
    {
        PCB_GROUP* ag = static_cast<PCB_GROUP*>( a );

        if( ag->GetItems().empty() )
        {
            m_frame->ShowInfoBarError( _( "Cannot generate clearance report on empty group." ) );
            return;
        }

        a = static_cast<BOARD_ITEM*>( *ag->GetItems().begin() );
    }

    if( b->Type() == PCB_GROUP_T )
    {
        PCB_GROUP* bg = static_cast<PCB_GROUP*>( b );

        if( bg->GetItems().empty() )
        {
            m_frame->ShowInfoBarError( _( "Cannot generate clearance report on empty group." ) );
            return;
        }

        b = static_cast<BOARD_ITEM*>( *bg->GetItems().begin() );
    }

    if( !a || !b )
        return;

    auto checkFootprint =
            [&]( FOOTPRINT* footprint ) -> BOARD_ITEM*
            {
                PAD* foundPad = nullptr;

                for( PAD* pad : footprint->Pads() )
                {
                    if( !foundPad || pad->SameLogicalPadAs( foundPad ) )
                        foundPad = pad;
                    else
                        return footprint;
                }

                if( !foundPad )
                    return footprint;

                return foundPad;
            };

    if( a->Type() == PCB_FOOTPRINT_T )
        a = checkFootprint( static_cast<FOOTPRINT*>( a ) );

    if( b->Type() == PCB_FOOTPRINT_T )
        b = checkFootprint( static_cast<FOOTPRINT*>( b ) );

    if( !a || !b )
        return;

    DIALOG_BOOK_REPORTER* dialog = m_frame->GetInspectClearanceDialog();

    wxCHECK( dialog, /* void */ );

    dialog->DeleteAllPages();

    if( a->Type() != PCB_ZONE_T && b->Type() == PCB_ZONE_T )
        std::swap( a, b );
    else if( !a->IsConnected() && b->IsConnected() )
        std::swap( a, b );

    WX_HTML_REPORT_BOX*   r = nullptr;
    PCB_LAYER_ID          active = m_frame->GetActiveLayer();
    LSET                  layerIntersection = a->GetLayerSet() & b->GetLayerSet();
    LSET                  copperIntersection = layerIntersection & LSET::AllCuMask();
    BOARD_CONNECTED_ITEM* ac = dynamic_cast<BOARD_CONNECTED_ITEM*>( a );
    BOARD_CONNECTED_ITEM* bc = dynamic_cast<BOARD_CONNECTED_ITEM*>( b );
    ZONE*                 zone = dynamic_cast<ZONE*>( a );
    PAD*                  pad = dynamic_cast<PAD*>( b );
    FOOTPRINT*            aFP = dynamic_cast<FOOTPRINT*>( a );
    FOOTPRINT*            bFP = dynamic_cast<FOOTPRINT*>( b );
    DRC_CONSTRAINT        constraint;
    int                   clearance = 0;

    bool compileError = false;
    bool courtyardError = false;
    std::unique_ptr<DRC_ENGINE> drcEngine = makeDRCEngine( &compileError, &courtyardError );

    if( copperIntersection.any() && zone && pad && zone->GetNetCode() == pad->GetNetCode() )
    {
        PCB_LAYER_ID layer = UNDEFINED_LAYER;

        if( zone->IsOnLayer( active ) )
            layer = active;
        else if( zone->GetLayerSet().count() > 0 )
            layer = zone->GetLayerSet().Seq().front();

        r = dialog->AddHTMLPage( _( "Zone" ) );
        reportHeader( _( "Zone connection resolution for:" ), a, b, layer, r );

        constraint = drcEngine->EvalZoneConnection( pad, zone, layer, r );

        if( constraint.m_ZoneConnection == ZONE_CONNECTION::THERMAL )
        {
            r->Report( "" );
            r->Report( "" );
            reportHeader( _( "Thermal-relief gap resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( THERMAL_RELIEF_GAP_CONSTRAINT, pad, zone, layer, r );
            int gap = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved thermal relief gap: %s." ),
                                         m_frame->StringFromValue( gap, true ) ) );

            r->Report( "" );
            r->Report( "" );
            reportHeader( _( "Thermal-relief spoke width resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( THERMAL_SPOKE_WIDTH_CONSTRAINT, pad, zone, layer, r );
            int width = constraint.m_Value.Opt();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved spoke width: %s." ),
                                         m_frame->StringFromValue( width, true ) ) );

            r->Report( "" );
            r->Report( "" );
            reportHeader( _( "Thermal-relief min spoke count resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( MIN_RESOLVED_SPOKES_CONSTRAINT, pad, zone, layer, r );
            int minSpokes = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min spoke count: %d." ),
                                         minSpokes ) );

            std::shared_ptr<CONNECTIVITY_DATA> connectivity = pad->GetBoard()->GetConnectivity();
        }
        else if( constraint.m_ZoneConnection == ZONE_CONNECTION::NONE )
        {
            r->Report( "" );
            r->Report( "" );
            reportHeader( _( "Zone clearance resolution for:" ), a, b, layer, r );

            clearance = zone->GetLocalClearance().value();
            r->Report( "" );
            r->Report( wxString::Format( _( "Zone clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );

            constraint = drcEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, pad, zone, layer, r );

            if( constraint.m_Value.Min() > clearance )
            {
                clearance = constraint.m_Value.Min();

                r->Report( "" );
                r->Report( wxString::Format( _( "Overridden by larger physical clearance from %s;"
                                                "clearance: %s." ),
                                             EscapeHTML( constraint.GetName() ),
                                             m_frame->StringFromValue( clearance, true ) ) );
            }

            if( !pad->FlashLayer( layer ) )
            {
                constraint = drcEngine->EvalRules( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, pad, zone,
                                                  layer, r );

                if( constraint.m_Value.Min() > clearance )
                {
                    clearance = constraint.m_Value.Min();

                    r->Report( "" );
                    r->Report( wxString::Format( _( "Overridden by larger physical hole clearance "
                                                    "from %s; clearance: %s." ),
                                                 EscapeHTML( constraint.GetName() ),
                                                 m_frame->StringFromValue( clearance, true ) ) );
                }
            }

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );
        }
        else
        {
            r->Report( "" );
            r->Report( "" );
            reportHeader( _( "Zone clearance resolution for:" ), a, b, layer, r );

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( 0, true ) ) );
        }

        r->Flush();
    }
    else if( copperIntersection.any() && !aFP && !bFP )
    {
        bool sameNet = ac && bc && ac->GetNetCode() > 0 && ac->GetNetCode() == bc->GetNetCode();

        std::vector<PCB_LAYER_ID> layers;

        if( copperIntersection.test( active ) )
            layers.push_back( active );

        for( PCB_LAYER_ID layer : copperIntersection.Seq() )
        {
            if( layer != active )
                layers.push_back( layer );
        }

        auto fillReport =
                [&]( PCB_LAYER_ID layer, REPORTER* rep )
                {
                    reportHeader( _( "Clearance resolution for:" ), a, b, layer, rep );

                    if( sameNet )
                    {
                        rep->Report( _( "Items belong to the same net. Min clearance is 0." ) );
                        return;
                    }

                    constraint = drcEngine->EvalRules( CLEARANCE_CONSTRAINT, a, b, layer, rep );
                    clearance = constraint.m_Value.Min();

                    if( compileError )
                        reportCompileError( rep );

                    rep->Report( "" );

                    if( constraint.IsNull() )
                    {
                        rep->Report( _( "Min clearance is 0." ) );
                    }
                    else if( clearance < 0 )
                    {
                        rep->Report( wxString::Format( _( "Resolved clearance: %s; clearance will "
                                                          "not be tested." ),
                                                       m_frame->StringFromValue( clearance, true ) ) );
                    }
                    else
                    {
                        rep->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                                       m_frame->StringFromValue( clearance, true ) ) );
                    }
                };

                if( layers.size() == 1 )
                {
                    PCB_LAYER_ID layer = layers.front();

                    r = dialog->AddHTMLPage( m_frame->GetBoard()->GetLayerName( layer ) );
                    fillReport( layer, r );
                    r->Flush();
                }
                else
                {
                    auto perLayerMessages = std::make_shared<std::vector<std::vector<wxString>>>();
                    perLayerMessages->reserve( layers.size() );

                    for( PCB_LAYER_ID layer : layers )
                    {
                        VECTOR_REPORTER tmp;
                        fillReport( layer, &tmp );
                        perLayerMessages->push_back( std::move( tmp.m_messages ) );
                    }

                    wxPanel*    panel = dialog->AddBlankPage( _( "Clearance" ) );
                    wxBoxSizer* vbox = new wxBoxSizer( wxVERTICAL );

                    wxChoice* choice = new wxChoice( panel, wxID_ANY );

                    for( PCB_LAYER_ID layer : layers )
                        choice->Append( m_frame->GetBoard()->GetLayerName( layer ) );

                    choice->SetSelection( 0 );

                    WX_HTML_REPORT_BOX* reportBox = new WX_HTML_REPORT_BOX( panel, wxID_ANY, wxDefaultPosition,
                                                                            wxDefaultSize,
                                                                            wxHW_SCROLLBAR_AUTO | wxBORDER_SIMPLE );
                    reportBox->SetUnits( m_frame->GetUserUnits() );

                    wxStaticText* layerLabel = new wxStaticText( panel, wxID_ANY, _( "Layer:" ) );

                    vbox->Add( layerLabel, 0, wxLEFT | wxRIGHT | wxTOP, 5 );
                    vbox->Add( choice, 0, wxEXPAND | wxALL, 5 );
                    vbox->Add( reportBox, 1, wxEXPAND | wxALL, 5 );
                    panel->SetSizer( vbox );
                    panel->Layout();

                    auto refresh =
                            [reportBox, perLayerMessages]( int sel )
                            {
                                reportBox->Clear();

                                if( sel >= 0 && sel < (int) perLayerMessages->size() )
                                {
                                    for( const wxString& line : ( *perLayerMessages )[sel] )
                                        reportBox->Report( line );
                                }

                                reportBox->Flush();
                            };

                            choice->Bind( wxEVT_CHOICE,
                                          [refresh]( wxCommandEvent& evt )
                                          {
                                              refresh( evt.GetSelection() );
                                          } );

                            refresh( 0 );
                        }
            }

    if( ac && bc )
    {
        NETINFO_ITEM* refNet = ac->GetNet();
        wxString      coupledNet;
        wxString      dummy;

        if( DRC_ENGINE::MatchDpSuffix( refNet->GetNetname(), coupledNet, dummy )
                && bc->GetNetname() == coupledNet )
        {
            LSET         dpIntersection = ac->GetLayerSet() & bc->GetLayerSet() & LSET::AllCuMask();
            PCB_LAYER_ID dpLayer = active;

            if( !dpIntersection.test( dpLayer ) && dpIntersection.any() )
                dpLayer = dpIntersection.Seq().front();

            r = dialog->AddHTMLPage( _( "Diff Pair" ) );
            reportHeader( _( "Diff-pair gap resolution for:" ), ac, bc, dpLayer, r );

            constraint = drcEngine->EvalRules( DIFF_PAIR_GAP_CONSTRAINT, ac, bc, dpLayer, r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved gap constraints: min %s; opt %s; max %s." ),
                                         reportMin( m_frame, constraint ),
                                         reportOpt( m_frame, constraint ),
                                         reportMax( m_frame, constraint ) ) );

            r->Report( "" );
            r->Report( "" );
            r->Report( "" );
            reportHeader( _( "Diff-pair max uncoupled length resolution for:" ), ac, bc, dpLayer, r );

            if( !drcEngine->HasRulesForConstraintType( MAX_UNCOUPLED_CONSTRAINT ) )
            {
                r->Report( "" );
                r->Report( _( "No 'diff_pair_uncoupled' constraints defined." ) );
            }
            else
            {
                constraint = drcEngine->EvalRules( MAX_UNCOUPLED_CONSTRAINT, ac, bc, dpLayer, r );

                r->Report( "" );
                r->Report( wxString::Format( _( "Resolved max uncoupled length: %s." ),
                                             reportMax( m_frame, constraint ) ) );
            }

            r->Flush();
        }
    }

    auto isOnCorrespondingLayer=
            [&]( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, wxString* aWarning )
            {
                if( aItem->IsOnLayer( aLayer ) )
                    return true;

                PCB_LAYER_ID correspondingMask =   IsFrontLayer( aLayer ) ? F_Mask : B_Mask;
                PCB_LAYER_ID correspondingCopper = IsFrontLayer( aLayer ) ? F_Cu   : B_Cu;

                if( aItem->IsOnLayer( aLayer ) )
                    return true;

                if( aItem->IsOnLayer( correspondingMask ) )
                    return true;

                if( aItem->IsTented( correspondingMask ) && aItem->IsOnLayer( correspondingCopper ) )
                {
                    *aWarning = wxString::Format( _( "Note: %s is tented; clearance will only be "
                                                     "applied to holes." ),
                                                  getItemDescription( aItem ) );
                    return true;
                }

                return false;
            };

    for( PCB_LAYER_ID layer : { F_SilkS, B_SilkS } )
    {
        wxString warning;

        if( ( a->IsOnLayer( layer ) && isOnCorrespondingLayer( b, layer, &warning ) )
            || ( b->IsOnLayer( layer ) && isOnCorrespondingLayer( a, layer, &warning ) ) )
        {
            r = dialog->AddHTMLPage( m_frame->GetBoard()->GetLayerName( layer ) );
            reportHeader( _( "Silkscreen clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( SILK_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );

            if( !warning.IsEmpty() )
                r->Report( warning );

            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );

            r->Flush();
        }
    }

    for( PCB_LAYER_ID layer : { F_CrtYd, B_CrtYd } )
    {
        bool aCourtyard = aFP && !aFP->GetCourtyard( layer ).IsEmpty();
        bool bCourtyard = bFP && !bFP->GetCourtyard( layer ).IsEmpty();

        if( aCourtyard && bCourtyard )
        {
            r = dialog->AddHTMLPage( m_frame->GetBoard()->GetLayerName( layer ) );
            reportHeader( _( "Courtyard clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );

            r->Flush();
        }
    }

    if( a->HasHole() || b->HasHole() )
    {
        PCB_LAYER_ID layer = UNDEFINED_LAYER;
        bool         pageAdded = false;

        if( a->HasHole() && b->IsOnLayer( active ) && IsCopperLayer( active ) )
            layer = active;
        else if( b->HasHole() && a->IsOnLayer( active ) && IsCopperLayer( active ) )
            layer = active;
        else if( a->HasHole() && b->IsOnCopperLayer() )
            layer = b->GetLayer();
        else if( b->HasHole() && a->IsOnCopperLayer() )
            layer = a->GetLayer();

        if( layer >= 0 )
        {
            r = dialog->AddHTMLPage( _( "Hole" ) );
            pageAdded = true;

            reportHeader( _( "Hole clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );

            r->Flush();
        }

        if( a->HasDrilledHole() || b->HasDrilledHole() )
        {
            if( !pageAdded )
            {
                r = dialog->AddHTMLPage( _( "Hole" ) );
                pageAdded = true;
            }
            else
            {
                r->Report( "" );
                r->Report( "" );
                r->Report( "" );
            }

            reportHeader( _( "Hole-to-hole clearance resolution for:" ), a, b, r );

            constraint = drcEngine->EvalRules( HOLE_TO_HOLE_CONSTRAINT, a, b, UNDEFINED_LAYER, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );

            r->Flush();
        }
    }

    for( PCB_LAYER_ID edgeLayer : { Edge_Cuts, Margin } )
    {
        PCB_LAYER_ID layer = UNDEFINED_LAYER;

        if( a->IsOnLayer( edgeLayer ) && b->Type() != PCB_FOOTPRINT_T )
        {
            if( b->IsOnLayer( active ) && IsCopperLayer( active ) )
                layer = active;
            else if( IsCopperLayer( b->GetLayer() ) )
                layer = b->GetLayer();
        }
        else if( b->IsOnLayer( edgeLayer ) && a->Type() != PCB_FOOTPRINT_T )
        {
            if( a->IsOnLayer( active ) && IsCopperLayer( active ) )
                layer = active;
            else if( IsCopperLayer( a->GetLayer() ) )
                layer = a->GetLayer();
        }

        if( layer >= 0 )
        {
            wxString layerName = m_frame->GetBoard()->GetLayerName( edgeLayer );
            r = dialog->AddHTMLPage( layerName + wxS( " " ) + _( "Clearance" ) );
            reportHeader( _( "Edge clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( EDGE_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );

            r->Flush();
        }
    }

    r = dialog->AddHTMLPage( _( "Physical Clearances" ) );

    if( compileError )
    {
        reportCompileError( r );
    }
    else if( !drcEngine->HasRulesForConstraintType( PHYSICAL_CLEARANCE_CONSTRAINT ) )
    {
        r->Report( "" );
        r->Report( _( "No 'physical_clearance' constraints defined." ) );
    }
    else
    {
        LSET reportLayers = layerIntersection;
        bool reported = false;

        if( a->IsOnLayer( Edge_Cuts ) )
        {
            LSET edgeInteractingLayers = bFP ? LSET( { F_CrtYd, B_CrtYd } )
                                             : LSET( b->GetLayerSet() & LSET::PhysicalLayersMask() );
            reportLayers |= edgeInteractingLayers;
        }

        if( b->IsOnLayer( Edge_Cuts ) )
        {
            LSET edgeInteractingLayers = aFP ? LSET( { F_CrtYd, B_CrtYd } )
                                             : LSET( a->GetLayerSet() & LSET::PhysicalLayersMask() );
            reportLayers |= edgeInteractingLayers;
        }

        for( PCB_LAYER_ID layer : reportLayers )
        {
            reported = true;
            reportHeader( _( "Physical clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( constraint.IsNull() )
            {
                r->Report( "" );
                r->Report( wxString::Format( _( "No 'physical_clearance' constraints in effect on %s." ),
                                             m_frame->GetBoard()->GetLayerName( layer ) ) );
            }
            else
            {
                r->Report( "" );
                r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                             m_frame->StringFromValue( clearance, true ) ) );
            }

            r->Report( "" );
            r->Report( "" );
            r->Report( "" );
        }

        if( !reported )
        {
            reportHeader( _( "Physical clearance resolution for:" ), a, b, r );
            r->Report( "" );
            r->Report( _( "Items share no relevant layers.  No 'physical_clearance' constraints will "
                          "be applied." ) );
        }
    }

    if( a->HasHole() || b->HasHole() )
    {
        PCB_LAYER_ID layer;

        if( a->HasHole() && b->IsOnLayer( active ) )
            layer = active;
        else if( b->HasHole() && a->IsOnLayer( active ) )
            layer = active;
        else if( a->HasHole() )
            layer = b->GetLayer();
        else
            layer = a->GetLayer();

        reportHeader( _( "Physical hole clearance resolution for:" ), a, b, layer, r );

        constraint = drcEngine->EvalRules( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, a, b, layer, r );
        clearance = constraint.m_Value.Min();

        if( compileError )
        {
            reportCompileError( r );
        }
        else if( !drcEngine->HasRulesForConstraintType( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT ) )
        {
            r->Report( "" );
            r->Report( _( "No 'physical_hole_clearance' constraints defined." ) );
        }
        else
        {
            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( clearance, true ) ) );
        }
    }

    r->Flush();

    dialog->Raise();
    dialog->Show( true );
}


int BOARD_INSPECTION_TOOL::InspectConstraints( const TOOL_EVENT& aEvent )
{
#define EVAL_RULES( constraint, a, b, layer, r ) drcEngine->EvalRules( constraint, a, b, layer, r )

    wxCHECK( m_frame, 0 );

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxCHECK( selTool, 0 );

    const PCB_SELECTION& selection = selTool->GetSelection();
    BOARD_ITEM*          item = nullptr;

    if( selection.Size() == 1 && selection.GetItem( 0 )->IsBOARD_ITEM() )
    {
        item = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) );
    }
    else if( selection.Size() == 0 )
    {
        static const std::vector<KICAD_T> constraintTypes = {
            PCB_PAD_T,
            PCB_VIA_T,
            PCB_TRACE_T,
            PCB_ARC_T,
            PCB_ZONE_T,
            PCB_SHAPE_T,
            PCB_FOOTPRINT_T,
            PCB_FIELD_T,
            PCB_TEXT_T,
            PCB_TEXTBOX_T,
            PCB_GROUP_T
        };

        Activate();

        item = pickItemForInspection( aEvent, _( "Select item for constraints resolution..." ),
                                      constraintTypes, nullptr );

        if( !item )
            return 0;
    }
    else
    {
        m_frame->ShowInfoBarError( _( "Select a single item for a constraints resolution report." ) );
        return 0;
    }

    DIALOG_BOOK_REPORTER* dialog = m_frame->GetInspectConstraintsDialog();

    wxCHECK( dialog, 0 );

    dialog->DeleteAllPages();
    DRC_CONSTRAINT constraint;

    bool compileError = false;
    bool courtyardError = false;
    std::unique_ptr<DRC_ENGINE> drcEngine = makeDRCEngine( &compileError, &courtyardError );

    WX_HTML_REPORT_BOX* r = nullptr;

    if( item->Type() == PCB_TRACE_T )
    {
        r = dialog->AddHTMLPage( _( "Track Width" ) );
        reportHeader( _( "Track width resolution for:" ), item, r );

        constraint = EVAL_RULES( TRACK_WIDTH_CONSTRAINT, item, nullptr, item->GetLayer(), r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved width constraints: min %s; opt %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportOpt( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );

        r->Flush();
    }

    if( item->Type() == PCB_VIA_T )
    {
        r = dialog->AddHTMLPage( _( "Via Diameter" ) );
        reportHeader( _( "Via diameter resolution for:" ), item, r );

        // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
        constraint = EVAL_RULES( VIA_DIAMETER_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved diameter constraints: min %s; opt %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportOpt( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );

        r->Flush();

        r = dialog->AddHTMLPage( _( "Via Annular Width" ) );
        reportHeader( _( "Via annular width resolution for:" ), item, r );

        // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
        constraint = EVAL_RULES( ANNULAR_WIDTH_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved annular width constraints: min %s; opt %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportOpt( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );

        r->Flush();
    }

    if( ( item->Type() == PCB_PAD_T && static_cast<PAD*>( item )->GetDrillSize().x > 0 )
            || item->Type() == PCB_VIA_T )
    {
        r = dialog->AddHTMLPage( _( "Hole Size" ) );
        reportHeader( _( "Hole size resolution for:" ), item, r );

        constraint = EVAL_RULES( HOLE_SIZE_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved hole size constraints: min %s; opt %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportOpt( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );

        r->Flush();
    }

    if( item->Type() == PCB_PAD_T || item->Type() == PCB_SHAPE_T || dynamic_cast<PCB_TRACK*>( item ) )
    {
        r = dialog->AddHTMLPage( _( "Solder Mask" ) );
        reportHeader( _( "Solder mask expansion resolution for:" ), item, r );

        constraint = EVAL_RULES( SOLDER_MASK_EXPANSION_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved solder mask expansion: %s." ),
                                     reportOpt( m_frame, constraint ) ) );

        r->Flush();
    }

    if( item->Type() == PCB_PAD_T )
    {
        r = dialog->AddHTMLPage( _( "Solder Paste" ) );
        reportHeader( _( "Solder paste absolute clearance resolution for:" ), item, r );

        constraint = EVAL_RULES( SOLDER_PASTE_ABS_MARGIN_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved solder paste absolute clearance: %s." ),
                                     reportOpt( m_frame, constraint ) ) );

        reportHeader( _( "Solder paste relative clearance resolution for:" ), item, r );

        constraint = EVAL_RULES( SOLDER_PASTE_REL_MARGIN_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( "" );
        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved solder paste relative clearance: %s." ),
                                     reportOpt( m_frame, constraint ) ) );

        r->Flush();
    }

    if( item->Type() == PCB_FIELD_T || item->Type() == PCB_TEXT_T || item->Type() == PCB_TEXTBOX_T )
    {
        r = dialog->AddHTMLPage( _( "Text Size" ) );
        reportHeader( _( "Text height resolution for:" ), item, r );

        constraint = EVAL_RULES( TEXT_HEIGHT_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved height constraints: min %s; opt %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportOpt( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );

        r->Report( "" );
        r->Report( "" );
        r->Report( "" );
        reportHeader( _( "Text thickness resolution for:" ), item, r );

        constraint = EVAL_RULES( TEXT_THICKNESS_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved thickness constraints: min %s; opt %s; max %s." ),
                                     reportMin( m_frame, constraint ),
                                     reportOpt( m_frame, constraint ),
                                     reportMax( m_frame, constraint ) ) );

        r->Flush();
    }

    r = dialog->AddHTMLPage( _( "Keepouts" ) );
    reportHeader( _( "Keepout resolution for:" ), item, r );

    constraint = EVAL_RULES( DISALLOW_CONSTRAINT, item, nullptr, item->GetLayer(), r );

    if( compileError )
        reportCompileError( r );

    if( courtyardError )
    {
        r->Report( "" );
        r->Report( _( "Report may be incomplete: some footprint courtyards are malformed." )
                   + wxS( "&nbsp;&nbsp;" )
                   + wxS( "<a href='$DRC'>" ) + _( "Run DRC for a full analysis." )
                   + wxS( "</a>" ) );
    }

    r->Report( "" );

    if( constraint.m_DisallowFlags )
        r->Report( _( "Item <b>disallowed</b> at current location." ) );
    else
        r->Report( _( "Item allowed at current location." ) );

    r->Flush();

    r = dialog->AddHTMLPage( _( "Assertions" ) );
    reportHeader( _( "Assertions for:" ), item, r );

    if( compileError )
        reportCompileError( r );

    if( courtyardError )
    {
        r->Report( "" );
        r->Report( _( "Report may be incomplete: some footprint courtyards are malformed." )
                   + wxS( "&nbsp;&nbsp;" )
                   + wxS( "<a href='$DRC'>" ) + _( "Run DRC for a full analysis." )
                   + wxS( "</a>" ) );
    }

    drcEngine->ProcessAssertions( item, []( const DRC_CONSTRAINT* c ){}, r );
    r->Flush();

    dialog->Raise();
    dialog->Show( true );
    return 0;
}


int BOARD_INSPECTION_TOOL::DiffFootprint( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxCHECK( selTool, 0 );

    const PCB_SELECTION& selection = selTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[ i ];

                    if( !dynamic_cast<FOOTPRINT*>( item ) )
                        aCollector.Remove( item );
                }
            } );

    if( selection.Size() == 1 )
        DiffFootprint( static_cast<FOOTPRINT*>( selection.GetItem( 0 ) ) );
    else
        m_frame->ShowInfoBarError( _( "Select a footprint to diff with its library equivalent." ) );

    return 0;
}


int BOARD_INSPECTION_TOOL::CompareBoardWithFile( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    wxFileDialog dlg( m_frame, _( "Choose Board to Compare With" ), wxEmptyString,
                      wxEmptyString, FILEEXT::PcbFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    wxFileName otherFn( dlg.GetPath() );
    otherFn.MakeAbsolute();

    const wxString otherPath = otherFn.GetFullPath();

    wxFileName projectFn = otherFn;
    projectFn.SetExt( FILEEXT::ProjectFileExtension );
    const wxString projectPath = projectFn.GetFullPath();

    wxFileName activeProjectFn( m_frame->Prj().GetProjectFullName() );
    activeProjectFn.MakeAbsolute();

    // Refuse the self-compare before touching SETTINGS_MANAGER — attaching the
    // active PROJECT to a second BOARD would clobber its m_BoardSettings.  Use
    // wxFileName::SameAs (path normalization + case folding) rather than a raw
    // string compare, matching SCH_INSPECTION_TOOL::CompareSchematic.
    if( projectFn.SameAs( activeProjectFn ) )
    {
        m_frame->ShowInfoBarError(
                _( "Select a board file from another project to compare." ) );
        return 0;
    }

    return showBoardComparison( otherPath, projectPath, otherPath );
}


namespace
{
// Everything needed to show one board-vs-board comparison: the diff, the clones
// of removed live-board items (kept alive because the canvas references them),
// and the canvas-context switcher. Moveable so it can be swapped on reload.
struct PCB_DIFF_VIEW
{
    KICAD_DIFF::DOCUMENT_DIFF                result;
    std::vector<std::unique_ptr<BOARD_ITEM>> clones;
    DIALOG_KICAD_DIFF::SHEET_SWITCHER        switcher;
};


PCB_DIFF_VIEW buildPcbDiffView( BOARD* aLive, BOARD* aOther, const wxString& aOtherPath )
{
    PCB_DIFF_VIEW view;

    KICAD_DIFF::PCB_DIFFER differ( aLive, aOther, aOtherPath );
    view.result = differ.Diff();

    const KICAD_DIFF::DIFF_COLOR_THEME theme;

    std::map<KIID, KIGFX::COLOR4D>       refOverrides;
    std::map<KIID, KIGFX::COLOR4D>       compOverrides;
    std::map<KIID, KICAD_DIFF::CATEGORY> kiidCategories;

    std::function<void( const KICAD_DIFF::ITEM_CHANGE& )> collect =
            [&]( const KICAD_DIFF::ITEM_CHANGE& aChange )
            {
                if( !aChange.id.empty() )
                {
                    const KIID& kiid = aChange.id.back();
                    kiidCategories[kiid] = KICAD_DIFF::CategoryFor( aChange.kind );

                    switch( aChange.kind )
                    {
                    case KICAD_DIFF::CHANGE_KIND::ADDED: compOverrides[kiid] = theme.added; break;
                    case KICAD_DIFF::CHANGE_KIND::REMOVED:
                        refOverrides[kiid] = theme.removed;
                        compOverrides[kiid] = theme.removed;
                        break;
                    case KICAD_DIFF::CHANGE_KIND::MODIFIED:
                        refOverrides[kiid] = theme.modified;
                        compOverrides[kiid] = theme.modified;
                        break;
                    default:
                        refOverrides[kiid] = theme.conflict;
                        compOverrides[kiid] = theme.conflict;
                        break;
                    }
                }

                for( const KICAD_DIFF::ITEM_CHANGE& child : aChange.children )
                    collect( child );
            };

    for( const KICAD_DIFF::ITEM_CHANGE& change : view.result.changes )
        collect( change );

    // Clone removed items from the live board because their originals are
    // pinned to the editor's VIEW.
    for( const auto& [kiid, color] : refOverrides )
    {
        if( color != theme.removed )
            continue;

        if( BOARD_ITEM* found = aLive->ResolveItem( kiid, /*aAllowNullptrReturn=*/true ) )
        {
            if( BOARD_ITEM* clone = dynamic_cast<BOARD_ITEM*>( found->Clone() ) )
                view.clones.emplace_back( clone );
        }
    }

    std::vector<KIGFX::VIEW_ITEM*> extraItems;

    for( const std::unique_ptr<BOARD_ITEM>& clone : view.clones )
    {
        // A footprint draws nothing itself. Its pads, graphics and fields are
        // independent view items, so add them too or the clone is invisible.
        if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( clone.get() ) )
        {
            std::vector<KIGFX::VIEW_ITEM*> fpItems = KICAD_DIFF::CollectFootprintDiffContextItems( *fp );
            extraItems.insert( extraItems.end(), fpItems.begin(), fpItems.end() );
        }
        else
        {
            extraItems.push_back( clone.get() );
        }
    }

    view.switcher =
            [other = aOther, color = theme.modified, overrides = compOverrides, extras = std::move( extraItems ),
             categories = kiidCategories] ( WIDGET_DIFF_CANVAS& aCanvas, const KIID_PATH& )
            {
                KICAD_DIFF::ConfigurePcbDiffCanvasContext( aCanvas, nullptr, other, color, overrides, extras,
                                                           categories );
            };

    return view;
}
} // namespace


int BOARD_INSPECTION_TOOL::showBoardComparison( const wxString& aOtherPath, const wxString& aProjectPath,
                                                const wxString& aComparisonLabel )
{
    // Load the comparison board into its own non-active PROJECT so the live
    // project settings are never overwritten. A missing .kicad_pro is fine
    // (LoadProject returns false but still inserts a defaults-only slot); a
    // present-but-malformed .kicad_pro is treated as failure.
    SETTINGS_MANAGER* mgr           = m_frame->GetSettingsManager();
    bool              projectLoadOk = mgr && mgr->LoadProject( aProjectPath, false );
    PROJECT*          otherPrj = mgr ? mgr->GetProject( aProjectPath ) : nullptr;

    if( !otherPrj )
    {
        m_frame->ShowInfoBarError( wxString::Format( _( "Failed to load project for %s" ), aOtherPath ) );
        return 0;
    }

    if( !projectLoadOk && wxFileName( aProjectPath ).FileExists() )
    {
        mgr->UnloadProject( otherPrj, false );
        m_frame->ShowInfoBarError( wxString::Format( _( "Failed to load project for %s" ), aOtherPath ) );
        return 0;
    }

    PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::FindPluginTypeFromBoardPath( aOtherPath, KICTL_KICAD_ONLY );

    // Skip editor-frame initialization (drawing sheet singleton, BASE_SCREEN
    // globals, DRC engine, connectivity) so the read-only compare cannot leak
    // state into the live frame.
    BOARD_LOADER::OPTIONS loadOptions;
    loadOptions.initialize_after_load = false;

    std::unique_ptr<BOARD> otherBoard;

    try
    {
        otherBoard = BOARD_LOADER::Load( aOtherPath, pluginType, otherPrj, loadOptions );
    }
    catch( const IO_ERROR& ioe )
    {
        m_frame->ShowInfoBarError( wxString::Format( _( "Failed to load %s: %s" ), aOtherPath, ioe.What() ) );
        mgr->UnloadProject( otherPrj, false );
        return 0;
    }
    catch( ... )
    {
        m_frame->ShowInfoBarError( wxString::Format( _( "Failed to load %s" ), aOtherPath ) );
        mgr->UnloadProject( otherPrj, false );
        return 0;
    }

    if( !otherBoard )
    {
        m_frame->ShowInfoBarError( wxString::Format( _( "Failed to load %s" ), aOtherPath ) );
        mgr->UnloadProject( otherPrj, false );
        return 0;
    }

    BOARD*        board = m_frame->GetBoard();
    PCB_DIFF_VIEW view = buildPcbDiffView( board, otherBoard.get(), aOtherPath );

    auto dlgDiff = std::make_unique<DIALOG_KICAD_DIFF>( m_frame, board->GetFileName(), aComparisonLabel, view.result,
                                                        KICAD_DIFF::DOCUMENT_GEOMETRY{},
                                                        KICAD_DIFF::DOCUMENT_GEOMETRY{}, view.switcher );
    dlgDiff->ShowModal();

    // Destroy the dialog (its canvas drops its references to the board's items)
    // before the board is freed, or teardown dereferences freed items.
    dlgDiff.reset();

    otherBoard->ClearProject();
    otherBoard.reset();

    mgr->UnloadProject( otherPrj, false );

    return 0;
}


int BOARD_INSPECTION_TOOL::CompareBoardWithHistory( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    BOARD* liveBoard = m_frame->GetBoard();

    if( !liveBoard || liveBoard->GetFileName().IsEmpty() )
    {
        m_frame->ShowInfoBarError( _( "Save the board before comparing against local history." ) );
        return 0;
    }

    const wxString projectPath = m_frame->Prj().GetProjectPath();
    LOCAL_HISTORY& history = m_frame->Kiway().LocalHistory();

    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> snapshots = history.GetSnapshots( projectPath );

    if( snapshots.empty() )
    {
        m_frame->ShowInfoBarError( _( "No local history snapshots for this project." ) );
        return 0;
    }

    // The history repo stores files by project-relative path with forward slashes.
    wxFileName boardFn( liveBoard->GetFileName() );
    boardFn.MakeRelativeTo( projectPath );
    const wxString relPath = boardFn.GetFullPath( wxPATH_UNIX );

    wxFileName projFn( m_frame->Prj().GetProjectFullName() );
    projFn.MakeRelativeTo( projectPath );
    const wxString projRel = projFn.GetFullPath( wxPATH_UNIX );

    // One entry per distinct board version: newest-first, skipping commits with
    // no board or whose board content matches the previously kept one. This
    // drops schematic-only saves and collapses runs that left the board untouched.
    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> boardSnapshots;
    wxString                                 prevFingerprint;

    for( const LOCAL_HISTORY_SNAPSHOT_INFO& s : snapshots )
    {
        wxString fingerprint = history.TreeFingerprint( projectPath, s.hash, wxS( ".kicad_pcb" ) );

        if( fingerprint.IsEmpty() || fingerprint == prevFingerprint )
            continue;

        prevFingerprint = fingerprint;
        boardSnapshots.push_back( s );
    }

    if( boardSnapshots.empty() )
    {
        m_frame->ShowInfoBarError( _( "No local history snapshots change this board." ) );
        return 0;
    }

    snapshots = std::move( boardSnapshots );

    SETTINGS_MANAGER* mgr = m_frame->GetSettingsManager();

    std::vector<wxString> labels;

    for( const LOCAL_HISTORY_SNAPSHOT_INFO& s : snapshots )
    {
        wxString summary = s.summary.IsEmpty() ? s.message.BeforeFirst( '\n' ) : s.summary;
        labels.push_back( wxString::Format( wxS( "%s (%s)" ), summary, s.hash.Left( 8 ) ) );
    }

    // State for the revision currently shown. Swapped on each dropdown change.
    std::unique_ptr<BOARD> curBoard;
    PROJECT*               curPrj = nullptr;
    wxString               curTempDir;

    auto cleanupCurrent =
            [&]()
            {
                if( curBoard )
                {
                    curBoard->ClearProject();
                    curBoard.reset();
                }

                // Skip if the project was already evicted from the manager.
                if( curPrj && mgr->IsProjectLoaded( curPrj ) )
                    mgr->UnloadProject( curPrj, false );

                curPrj = nullptr;

                if( !curTempDir.IsEmpty() )
                {
                    wxFileName::Rmdir( curTempDir, wxPATH_RMDIR_RECURSIVE );
                    curTempDir.Clear();
                }
            };

    // Extract + load snapshot aIndex into the out-params. Cleans up its own temp
    // on any failure.
    auto loadRevision =
            [&]( int aIndex, std::unique_ptr<BOARD>& aBoard, PROJECT*& aPrj, wxString& aTempDir ) -> bool
            {
                const wxString hash = snapshots[aIndex].hash;
                wxFileName     dirFn;
                dirFn.AssignDir( wxFileName::GetTempDir() );
                dirFn.AppendDir( wxS( "kicad-history-" ) + hash.Left( 8 ) );
                const wxString tempDir = dirFn.GetPath();

                // Extract just the board and project file (its real net classes and
                // design settings), skipping the schematic, 3D models, gerbers, etc.
                if( !history.ExtractAllFilesAtCommit( projectPath, hash, tempDir,
                                                      { wxS( ".kicad_pcb" ), wxS( ".kicad_pro" ) } ) )
                {
                    wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
                    return false;
                }

                const wxString boardPath = tempDir + wxS( "/" ) + relPath;
                const wxString proPath = tempDir + wxS( "/" ) + projRel;

                mgr->LoadProject( proPath, false );
                PROJECT* prj = mgr->GetProject( proPath );

                if( !prj )
                {
                    wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
                    return false;
                }

                BOARD_LOADER::OPTIONS loadOptions;
                loadOptions.initialize_after_load = false;

                std::unique_ptr<BOARD> loaded;

                try
                {
                    loaded = BOARD_LOADER::Load( boardPath,
                                                 PCB_IO_MGR::FindPluginTypeFromBoardPath( boardPath, KICTL_KICAD_ONLY ),
                                                 prj, loadOptions );
                }
                catch( ... )
                {
                    // A historical board may be malformed or in a format this build cannot parse. Skip it
                    // rather than letting the throw escape.
                    mgr->UnloadProject( prj, false );
                    wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
                    return false;
                }

                if( !loaded )
                {
                    mgr->UnloadProject( prj, false );
                    wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
                    return false;
                }

                aBoard = std::move( loaded );
                aPrj = prj;
                aTempDir = tempDir;
                return true;
            };

    // Load a revision and build its diff view, guarding both the parse and the
    // diff so a bad snapshot is skipped instead of crashing.
    auto loadView =
            [&]( int aIndex, std::unique_ptr<BOARD>& aBoard, PROJECT*& aPrj, wxString& aTempDir,
                 PCB_DIFF_VIEW& aView ) -> bool
            {
                if( !loadRevision( aIndex, aBoard, aPrj, aTempDir ) )
                    return false;

                try
                {
                    aView = buildPcbDiffView( liveBoard, aBoard.get(), aTempDir + wxS( "/" ) + relPath );
                }
                catch( ... )
                {
                    aBoard->ClearProject();
                    aBoard.reset();
                    mgr->UnloadProject( aPrj, false );
                    aPrj = nullptr;
                    wxFileName::Rmdir( aTempDir, wxPATH_RMDIR_RECURSIVE );
                    aTempDir.Clear();
                    return false;
                }

                return true;
            };

    PCB_DIFF_VIEW view;
    int           startIndex = 0;

    if( !loadView( 0, curBoard, curPrj, curTempDir, view ) )
    {
        m_frame->ShowInfoBarError( _( "Could not compare against the selected snapshot." ) );
        return 0;
    }

    // Default to the first commit back from HEAD that actually differs from the
    // current board, so opening lands on real changes rather than an in-sync HEAD.
    while( view.result.Empty() && startIndex + 1 < static_cast<int>( snapshots.size() ) )
    {
        std::unique_ptr<BOARD> nextBoard;
        PROJECT*               nextPrj = nullptr;
        wxString               nextTempDir;
        PCB_DIFF_VIEW          nextView;

        if( !loadView( startIndex + 1, nextBoard, nextPrj, nextTempDir, nextView ) )
            break;

        cleanupCurrent();
        view = std::move( nextView );
        curBoard = std::move( nextBoard );
        curPrj = nextPrj;
        curTempDir = nextTempDir;
        startIndex++;
    }

    auto dlgDiff = std::make_unique<DIALOG_KICAD_DIFF>( m_frame, liveBoard->GetFileName(), labels[startIndex],
                                                        view.result, KICAD_DIFF::DOCUMENT_GEOMETRY{},
                                                        KICAD_DIFF::DOCUMENT_GEOMETRY{}, view.switcher );

    dlgDiff->SetRevisionChooser(
            labels, startIndex,
            [&]( int aIndex )
            {
                std::unique_ptr<BOARD> newBoard;
                PROJECT*               newPrj = nullptr;
                wxString               newTempDir;
                PCB_DIFF_VIEW          newView;

                if( !loadView( aIndex, newBoard, newPrj, newTempDir, newView ) )
                {
                    m_frame->ShowInfoBarError( _( "Could not compare against the selected snapshot." ) );
                    return;
                }

                dlgDiff->Reload( liveBoard->GetFileName(), labels[aIndex], newView.result,
                                 /*aReferenceGeometry=*/{}, /*aComparisonGeometry=*/{}, newView.switcher, {} );

                // The canvas now references newBoard, so the previous one can go.
                cleanupCurrent();
                view = std::move( newView );
                curBoard = std::move( newBoard );
                curPrj = newPrj;
                curTempDir = newTempDir;
            } );

    dlgDiff->ShowModal();

    // Destroy the dialog before the board it references is freed.
    dlgDiff.reset();

    cleanupCurrent();
    return 0;
}


int BOARD_INSPECTION_TOOL::ShowFootprintLinks( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxCHECK( selTool, 0 );

    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() != 1 || selection.Front()->Type() != PCB_FOOTPRINT_T )
    {
        m_frame->ShowInfoBarError( _( "Select a footprint for a footprint associations report." ) );
        return 0;
    }

    DIALOG_FOOTPRINT_ASSOCIATIONS dlg( m_frame, static_cast<FOOTPRINT*>( selection.Front() ) );

    dlg.ShowModal();

    return 0;
}


void BOARD_INSPECTION_TOOL::DiffFootprint( FOOTPRINT* aFootprint, wxTopLevelWindow* aReparentTo )
{
    DIALOG_BOOK_REPORTER* dialog = m_frame->GetFootprintDiffDialog();

    wxCHECK( dialog, /* void */ );

    dialog->DeleteAllPages();
    dialog->SetUserItemID( aFootprint->m_Uuid );

    LIB_ID              fpID = aFootprint->GetFPID();
    wxString            libName = fpID.GetLibNickname();
    wxString            fpName = fpID.GetLibItemName();
    WX_HTML_REPORT_BOX* r = nullptr;

    r = dialog->AddHTMLPage( _( "Summary" ) );

    r->Report( wxS( "<h7>" ) + _( "Board vs library diff for:" ) + wxS( "</h7>" ) );
    r->Report( wxS( "<ul><li>" ) + EscapeHTML( getItemDescription( aFootprint ) ) + wxS( "</li>" )
             + wxS( "<li>" ) + _( "Library: " ) + EscapeHTML( libName ) + wxS( "</li>" )
             + wxS( "<li>" ) + _( "Library item: " ) + EscapeHTML( fpName ) + wxS( "</li></ul>" ) );

    r->Report( "" );

    PROJECT*             project = aFootprint->GetBoard()->GetProject();
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( project );

    if( !adapter->HasLibrary( libName, false ) )
    {
        r->Report( _( "The library is not included in the current configuration." )
                   + wxS( "&nbsp;&nbsp;&nbsp" )
                   + wxS( "<a href='$CONFIG'>" ) + _( "Manage Footprint Libraries" )
                   + wxS( "</a>" ) );

    }
    else if( !adapter->HasLibrary( libName, true ) )
    {
        r->Report( _( "The library is not enabled in the current configuration." )
                   + wxS( "&nbsp;&nbsp;&nbsp" )
                   + wxS( "<a href='$CONFIG'>" ) + _( "Manage Footprint Libraries" )
                   + wxS( "</a>" ) );

    }
    else
    {
        std::shared_ptr<FOOTPRINT> libFootprint;

        try
        {
            libFootprint.reset( adapter->LoadFootprint( libName, fpName, true ) );
        }
        catch( const IO_ERROR& )
        {
        }

        if( !libFootprint )
        {
            r->Report( wxString::Format( _( "The library no longer contains the item %s." ),
                                         fpName) );
        }
        else
        {
            if( !aFootprint->FootprintNeedsUpdate( libFootprint.get(), 0, r ) )
                r->Report( _( "No relevant differences detected." ) );

            wxPanel*               panel = dialog->AddBlankPage( _( "Visual" ) );
            FOOTPRINT_DIFF_WIDGET* diff = constructDiffPanel( panel );

            diff->DisplayDiff( aFootprint, libFootprint );
        }
    }

    r->Flush();

    if( aReparentTo )
        KIPLATFORM::UI::ReparentWindow( dialog, aReparentTo );
    else
        dialog->Raise();

    dialog->Show( true );
}


FOOTPRINT_DIFF_WIDGET* BOARD_INSPECTION_TOOL::constructDiffPanel( wxPanel* aParentPanel )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    FOOTPRINT_DIFF_WIDGET* diffWidget = new FOOTPRINT_DIFF_WIDGET( aParentPanel, m_frame->Kiway() );

   	sizer->Add( diffWidget, 1, wxEXPAND | wxALL, 5 );
    aParentPanel->SetSizer( sizer );
    aParentPanel->Layout();

    return diffWidget;
}


int BOARD_INSPECTION_TOOL::HighlightItem( const TOOL_EVENT& aEvent )
{
    BOARD_ITEM* item = aEvent.Parameter<BOARD_ITEM*>();

    m_frame->m_ProbingSchToPcb = true; // recursion guard
    {
        m_toolMgr->RunAction( ACTIONS::selectionClear );

        if( item )
            m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, item );
    }
    m_frame->m_ProbingSchToPcb = false;

    bool request3DviewRedraw = frame()->GetPcbNewSettings()->m_Display.m_Live3DRefresh;

    if( item && item->Type() != PCB_FOOTPRINT_T )
        request3DviewRedraw = false;

    // Update 3D viewer highlighting
    if( request3DviewRedraw )
        m_frame->Update3DView( false, true );

    return 0;
}


 bool BOARD_INSPECTION_TOOL::highlightNet( const VECTOR2D& aPosition, bool aUseSelection )
{
    BOARD*                  board         = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* settings      = getView()->GetPainter()->GetSettings();
    PCB_SELECTION_TOOL*     selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    int net = -1;
    bool enableHighlight = false;

    if( aUseSelection )
    {
        const PCB_SELECTION& selection = selectionTool->GetSelection();
        std::set<int> netcodes;

        for( EDA_ITEM* item : selection )
        {
            if( BOARD_CONNECTED_ITEM* ci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                netcodes.insert( ci->GetNetCode() );
        }

        enableHighlight = !netcodes.empty();

        if( enableHighlight && netcodes.size() > 1 )
        {
            // If we are doing a multi-highlight, cross-probing back and other stuff is not
            // yet supported
            settings->SetHighlight( netcodes );
            board->ResetNetHighLight();

            for( int multiNet : netcodes )
                board->SetHighLightNet( multiNet, true );

            board->HighLightON();
            m_toolMgr->GetView()->UpdateAllLayersColor();
            m_currentlyHighlighted = netcodes;
            return true;
        }
        else if( enableHighlight )
        {
            net = *netcodes.begin();
        }
    }

    // If we didn't get a net to highlight from the selection, use the cursor
    if( net < 0 )
    {
        GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
        guide.SetIgnoreZoneFills( false );
        guide.SetIgnoreNoNets( true );

        PCB_LAYER_ID activeLayer = static_cast<PCB_LAYER_ID>( view()->GetTopLayer() );
        guide.SetPreferredLayer( activeLayer );

        GENERAL_COLLECTOR collector;
        collector.Collect( board, { PCB_PAD_T, PCB_VIA_T, PCB_TRACE_T, PCB_ARC_T, PCB_SHAPE_T }, aPosition,
                           guide );

        if( collector.GetCount() == 0 )
            collector.Collect( board, { PCB_ZONE_T }, aPosition, guide );

        // Apply the active selection filter, except we want to allow picking locked items for
        // highlighting even if the user has disabled them for selection
        PCB_SELECTION_FILTER_OPTIONS& filter = selectionTool->GetFilter();

        bool saved         = filter.lockedItems;
        filter.lockedItems = true;

        selectionTool->FilterCollectedItems( collector, true, nullptr );

        filter.lockedItems = saved;

        // Clear the previous highlight
        //m_frame->SendMessageToEESCHEMA( nullptr );

        bool         highContrast  = settings->GetHighContrast();
        PCB_LAYER_ID contrastLayer = settings->GetPrimaryHighContrastLayer();

        for( int i = collector.GetCount() - 1; i >= 0; i-- )
        {
            LSET itemLayers = collector[i]->GetLayerSet();

            if( ( itemLayers & LSET::AllCuMask() ).none() ||
                ( highContrast && !itemLayers.Contains( contrastLayer ) ) )
            {
                collector.Remove( i );
                continue;
            }
        }

        enableHighlight = ( collector.GetCount() > 0 );

        // Obtain net code for the clicked item
        if( enableHighlight )
        {
            BOARD_CONNECTED_ITEM* targetItem = static_cast<BOARD_CONNECTED_ITEM*>( collector[0] );

            if( targetItem->Type() == PCB_PAD_T )
                m_frame->SendCrossProbeItem( targetItem );

            net = targetItem->GetNetCode();
        }
    }

    const std::set<int>& netcodes = settings->GetHighlightNetCodes();

    if( !aUseSelection && net >= 0 && netcodes.size() == 1 && netcodes.contains( net ) && settings->IsHighlightEnabled() )
    {
        if( BOARD* board2 = m_frame->GetBoard() )
        {
            if( NETINFO_ITEM* netinfo = board2->FindNet( net ) )
            {
                wxString sig = netinfo->GetNetChain();
                if( !sig.IsEmpty() )
                {
                    int count = 0;
                    for( NETINFO_ITEM* n : board2->GetNetInfo() )
                        if( n->GetNetChain() == sig )
                            count++;

                    if( count > 1 )
                    {
                        std::set<int> sigCodes;


                        for( NETINFO_ITEM* n : board2->GetNetInfo() )
                            if( n->GetNetChain() == sig )
                                sigCodes.insert( n->GetNetCode() );

                        settings->SetHighlight( sigCodes, true );
                        m_toolMgr->GetView()->UpdateAllLayersColor();
                        m_currentlyHighlighted = sigCodes;
                        m_highlightedNetChain    = sig;

                        board2->ResetNetHighLight();
                        for( int c : sigCodes )
                            board2->SetHighLightNet( c, true );
                        board2->HighLightON();

                        if( auto pcbSettings = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( settings ) )
                            pcbSettings->SetHighlightedNetChain( sig );

                        return true;
                    }
                }
            }
        }

        enableHighlight = !settings->IsHighlightEnabled();
    }
    else if( !aUseSelection && netcodes.size() == 1 && netcodes.contains( net ) )
    {
        enableHighlight = !settings->IsHighlightEnabled();
    }

    if( enableHighlight != settings->IsHighlightEnabled() || !netcodes.count( net ) )
    {
        if( !netcodes.empty() )
            m_lastHighlighted = netcodes;

        settings->SetHighlight( enableHighlight, net );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }

    // Store the highlighted netcode in the current board (for dialogs for instance)
    if( enableHighlight && net >= 0 )
    {
        m_currentlyHighlighted = netcodes;
        board->SetHighLightNet( net );
        board->HighLightON();

        NETINFO_ITEM* netinfo = board->FindNet( net );

        if( netinfo )
        {
            std::vector<MSG_PANEL_ITEM> items;
            netinfo->GetMsgPanelInfo( m_frame, items );
            m_frame->SetMsgPanel( items );
            m_frame->SendCrossProbeNetName( netinfo->GetNetname() );
        }
    }
    else
    {
        m_currentlyHighlighted.clear();
        board->ResetNetHighLight();
        m_frame->SetMsgPanel( board );
        m_frame->SendCrossProbeNetName( "" );
    }

    return true;
}


int BOARD_INSPECTION_TOOL::HighlightNet( const TOOL_EVENT& aEvent )
{
    int                     netcode     = aEvent.Parameter<int>();
    KIGFX::RENDER_SETTINGS* settings    = m_toolMgr->GetView()->GetPainter()->GetSettings();
    const std::set<int>&    highlighted = settings->GetHighlightNetCodes();

    if( netcode > 0 )
    {
        m_lastHighlighted = highlighted;
        settings->SetHighlight( true, netcode );
        m_toolMgr->GetView()->UpdateAllLayersColor();
        m_currentlyHighlighted.clear();
        m_currentlyHighlighted.insert( netcode );

        // If this net belongs to a multi-net chain and was already highlighted, escalate to chain highlight
        if( BOARD* board = m_frame->GetBoard() )
        {
            if( NETINFO_ITEM* net = board->FindNet( netcode ) )
            {
                wxString sig = net->GetNetChain();
                if( !sig.IsEmpty() )
                {
                    int count = 0;
                    for( NETINFO_ITEM* n : board->GetNetInfo() )
                        if( n->GetNetChain() == sig )
                            count++;
                    bool alreadyHighlighted = highlighted.count( netcode );
                    if( count > 1 && alreadyHighlighted )
                    {
                        if( m_highlightedNetChain != sig )
                        {
                            TOOL_EVENT sigEvt = PCB_ACTIONS::highlightNetChain.MakeEvent();
                            HighlightNetChain( sigEvt );
                        }
                    }
                }
            }
        }
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::highlightNetSelection ) )
    {
        // Highlight selection (cursor position will be ignored)
        highlightNet( getViewControls()->GetMousePosition(), true );
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::toggleLastNetHighlight ) )
    {
        std::set<int> temp = highlighted;
        settings->SetHighlight( m_lastHighlighted );
        m_toolMgr->GetView()->UpdateAllLayersColor();
        m_currentlyHighlighted = m_lastHighlighted;
        m_lastHighlighted      = std::move( temp );
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::toggleNetHighlight ) )
    {
        bool turnOn = highlighted.empty() && !m_currentlyHighlighted.empty();
        settings->SetHighlight( m_currentlyHighlighted, turnOn );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }
    else    // Highlight the net belonging to the item under the cursor
    {
        highlightNet( getViewControls()->GetMousePosition(), false );
    }

    return 0;
}


int BOARD_INSPECTION_TOOL::HighlightNetChain( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    VECTOR2D cursorPos = controls->GetCursorPosition( !aEvent.DisableGridSnapping() );
    BOARD_ITEM* item = nullptr;

    {
        // Collect nearest connectable item at cursor position
        BOARD* board = m_frame->GetBoard();
        GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
        GENERAL_COLLECTOR collector;
        collector.Collect( board, { PCB_PAD_T, PCB_VIA_T, PCB_TRACE_T, PCB_ARC_T, PCB_SHAPE_T }, cursorPos, guide );

        if( collector.GetCount() > 0 )
            item = collector[0];
    }

    wxString sig;

    if( item )
    {
        NETINFO_ITEM* net = nullptr;

        if( PAD* pad = dynamic_cast<PAD*>( item ) )
            net = pad->GetNet();
        else if( BOARD_CONNECTED_ITEM* ci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            net = ci->GetNet();

        if( net )
            sig = net->GetNetChain();
    }

    KIGFX::RENDER_SETTINGS* settings = m_toolMgr->GetView()->GetPainter()->GetSettings();

    // If no chain under cursor and a chain is currently highlighted, toggle off
    if( sig.IsEmpty() && !m_highlightedNetChain.IsEmpty() )
    {
        m_highlightedNetChain.clear();
        settings->SetHighlight( false );
        m_currentlyHighlighted.clear();
        m_toolMgr->GetView()->UpdateAllLayersColor();

        if( KIGFX::PCB_RENDER_SETTINGS* pcbSettings = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( settings ) )
            pcbSettings->SetHighlightedNetChain( wxString() );

        return 0;
    }


    // If same chain already highlighted, clear highlight
    if( !sig.IsEmpty() && sig == m_highlightedNetChain )
    {
        m_highlightedNetChain.clear();
        settings->SetHighlight( false );
        m_currentlyHighlighted.clear();
        m_toolMgr->GetView()->UpdateAllLayersColor();

        if( KIGFX::PCB_RENDER_SETTINGS* pcbSettings = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( settings ) )
            pcbSettings->SetHighlightedNetChain( wxString() );

        return 0;
    }

    // If we have a net highlight active but no chain highlight, convert to chain highlight
    if( !sig.IsEmpty() && m_highlightedNetChain.IsEmpty() && !m_currentlyHighlighted.empty() )
    {
        // Determine chain from first highlighted net if cursor item had no chain
        if( sig.IsEmpty() )
        {
            int firstCode = *m_currentlyHighlighted.begin();

            if( NETINFO_ITEM* net = m_frame->GetBoard()->FindNet( firstCode ) )
                sig = net->GetNetChain();
        }
    }

    m_highlightedNetChain = sig;

    if( KIGFX::PCB_RENDER_SETTINGS* pcbSettings = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( settings ) )
        pcbSettings->SetHighlightedNetChain( sig );

    std::set<int> codes;

    if( !sig.IsEmpty() )
    {
        for( NETINFO_ITEM* net : m_frame->GetBoard()->GetNetInfo() )
        {
            if( net->GetNetChain() == sig )
                codes.insert( net->GetNetCode() );
        }
    }

    settings->SetHighlight( codes, true );
    m_toolMgr->GetView()->UpdateAllLayersColor();
    m_currentlyHighlighted = codes;

    return 0;
}


int BOARD_INSPECTION_TOOL::ReplaceTerminalPad( const TOOL_EVENT& aEvent )
{
    if( m_highlightedNetChain.IsEmpty() )
        return 0;

    // Parameters are passed as a single pair<old,new>
    std::pair<wxString, wxString> ids = aEvent.Parameter<std::pair<wxString, wxString>>();
    KIID oldId( ids.first );
    KIID newId( ids.second );
    m_frame->GetBoard()->ReplaceNetChainTerminalPad( m_highlightedNetChain, oldId, newId );
    return 0;
}


int BOARD_INSPECTION_TOOL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    BOARD*                  board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* settings = m_toolMgr->GetView()->GetPainter()->GetSettings();

    m_currentlyHighlighted.clear();
    m_lastHighlighted.clear();

    board->ResetNetHighLight();
    settings->SetHighlight( false );

    // Also clear any chain-specific state
    if( KIGFX::PCB_RENDER_SETTINGS* pcbSettings = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( settings ) )
        pcbSettings->SetHighlightedNetChain( wxString() );

    m_toolMgr->GetView()->UpdateAllLayersColor();
    m_frame->SetMsgPanel( board );
    m_frame->SendCrossProbeNetName( "" );
    return 0;
}


int BOARD_INSPECTION_TOOL::LocalRatsnestTool( const TOOL_EVENT& aEvent )
{
    PCB_PICKER_TOOL*  picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::BULLSEYE );
    picker->SetSnapping( false );
    picker->ClearHandlers();

    picker->SetClickHandler(
            [this]( const VECTOR2D& pt ) -> bool
            {
                PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_toolMgr->RunAction<CLIENT_SELECTION_FILTER>( ACTIONS::selectionCursor,
                                                               EDIT_TOOL::PadFilter );

                PCB_SELECTION& selection = selectionTool->GetSelection();

                if( selection.Empty() )
                {
                    m_toolMgr->RunAction<CLIENT_SELECTION_FILTER>( ACTIONS::selectionCursor,
                                                                   EDIT_TOOL::FootprintFilter );
                    selection = selectionTool->GetSelection();
                }

                if( selection.Empty() )
                {
                    // Clear the previous local ratsnest if we click off all items
                    for( FOOTPRINT* fp : getModel<BOARD>()->Footprints() )
                    {
                        for( PAD* pad : fp->Pads() )
                            pad->SetLocalRatsnestVisible( displayOptions().m_ShowGlobalRatsnest );
                    }
                }
                else
                {
                    for( EDA_ITEM* item : selection )
                    {
                        if( PAD* pad = dyn_cast<PAD*>( item) )
                        {
                            pad->SetLocalRatsnestVisible( !pad->GetLocalRatsnestVisible() );
                        }
                        else if( FOOTPRINT* fp = dyn_cast<FOOTPRINT*>( item) )
                        {
                            if( !fp->Pads().empty() )
                            {
                                bool enable = !fp->Pads()[0]->GetLocalRatsnestVisible();

                                for( PAD* childPad : fp->Pads() )
                                    childPad->SetLocalRatsnestVisible( enable );
                            }
                        }
                    }
                }

                m_toolMgr->GetView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );

                return true;
            } );

    picker->SetFinalizeHandler(
            [this]( int aCondition )
            {
                if( aCondition != PCB_PICKER_TOOL::END_ACTIVATE )
                {
                    for( FOOTPRINT* fp : getModel<BOARD>()->Footprints() )
                    {
                        for( PAD* pad : fp->Pads() )
                            pad->SetLocalRatsnestVisible( displayOptions().m_ShowGlobalRatsnest );
                    }
                }
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


int BOARD_INSPECTION_TOOL::UpdateLocalRatsnest( const TOOL_EVENT& aEvent )
{
    VECTOR2I  delta = aEvent.Parameter<VECTOR2I>();

    if( delta == VECTOR2I() )
    {
        // We can delete the existing map to force a recalculation
        delete m_dynamicData;
        m_dynamicData = nullptr;
    }

    PCB_SELECTION_TOOL*                selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION&                     selection = selectionTool->GetSelection();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = getModel<BOARD>()->GetConnectivity();

    if( selection.Empty() )
    {
        connectivity->ClearLocalRatsnest();
        delete m_dynamicData;
        m_dynamicData = nullptr;
    }
    else
    {
        calculateSelectionRatsnest( delta );
    }

    return 0;
}


int BOARD_INSPECTION_TOOL::HideLocalRatsnest( const TOOL_EVENT& aEvent )
{
    getModel<BOARD>()->GetConnectivity()->ClearLocalRatsnest();
    delete m_dynamicData;
    m_dynamicData = nullptr;

    return 0;
}


void BOARD_INSPECTION_TOOL::calculateSelectionRatsnest( const VECTOR2I& aDelta )
{
    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    SELECTION&          selection = selectionTool->GetSelection();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();
    std::vector<BOARD_ITEM*> items;
    std::deque<EDA_ITEM*>    queued_items( selection.begin(), selection.end() );

    for( std::size_t i = 0; i < queued_items.size(); ++i )
    {
        if( !queued_items[i]->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( queued_items[i] );

        wxCHECK2( item, continue );

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
            {
                if( pad->GetLocalRatsnestVisible() || displayOptions().m_ShowModuleRatsnest )
                    items.push_back( pad );
            }
        }
        else if( item->Type() == PCB_GROUP_T || item->Type() == PCB_GENERATOR_T )
        {
            item->RunOnChildren( [ &queued_items ]( BOARD_ITEM *aItem )
                                    {
                                        queued_items.push_back( aItem );
                                    },
                                    RECURSE_MODE::RECURSE );
        }
        else if( BOARD_CONNECTED_ITEM* boardItem = dyn_cast<BOARD_CONNECTED_ITEM*>( item ) )
        {
            if( boardItem->GetLocalRatsnestVisible() || displayOptions().m_ShowModuleRatsnest )
                items.push_back( boardItem );
        }
    }

    if( items.empty() || std::none_of( items.begin(), items.end(),
                                       []( const BOARD_ITEM* aItem )
                                       {
                                           return( aItem->Type() == PCB_TRACE_T
                                                    || aItem->Type() == PCB_PAD_T
                                                    || aItem->Type() == PCB_ARC_T
                                                    || aItem->Type() == PCB_ZONE_T
                                                    || aItem->Type() == PCB_FOOTPRINT_T
                                                    || aItem->Type() == PCB_VIA_T
                                                    || aItem->Type() == PCB_SHAPE_T );
                                       } ) )
    {
        return;
    }

    if( !m_dynamicData )
    {
        m_dynamicData = new CONNECTIVITY_DATA( board()->GetConnectivity(), items, true );
        connectivity->BlockRatsnestItems( items );
    }
    else
    {
        m_dynamicData->Move( aDelta );
    }

    connectivity->ComputeLocalRatsnest( items, m_dynamicData );
}


int BOARD_INSPECTION_TOOL::HideNetInRatsnest( const TOOL_EVENT& aEvent )
{
    doHideRatsnestNet( aEvent.Parameter<int>(), true );
    return 0;
}


int BOARD_INSPECTION_TOOL::ShowNetInRatsnest( const TOOL_EVENT& aEvent )
{
    doHideRatsnestNet( aEvent.Parameter<int>(), false );
    return 0;
}


void BOARD_INSPECTION_TOOL::doHideRatsnestNet( int aNetCode, bool aHide )
{
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_toolMgr->GetView()->GetPainter()->GetSettings() );

    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    SELECTION&          selection     = selectionTool->GetSelection();

    if( aNetCode <= 0 && !selection.Empty() )
    {
        for( EDA_ITEM* item : selection )
        {
            if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            {
                if( bci->GetNetCode() > 0 )
                    doHideRatsnestNet( bci->GetNetCode(), aHide );
            }
        }

        return;
    }

    if( aHide )
        rs->GetHiddenNets().insert( aNetCode );
    else
        rs->GetHiddenNets().erase( aNetCode );

    if( !m_frame->GetAppearancePanel()->IsTogglingNetclassRatsnestVisibility() )
    {
        m_frame->GetCanvas()->RedrawRatsnest();
        m_frame->GetCanvas()->Refresh();

        m_frame->GetAppearancePanel()->OnNetVisibilityChanged( aNetCode, !aHide );
    }
}


void BOARD_INSPECTION_TOOL::setTransitions()
{
    Go( &BOARD_INSPECTION_TOOL::LocalRatsnestTool,   PCB_ACTIONS::localRatsnestTool.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HideLocalRatsnest,   PCB_ACTIONS::hideLocalRatsnest.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::UpdateLocalRatsnest, PCB_ACTIONS::updateLocalRatsnest.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::ShowBoardStatistics, PCB_ACTIONS::boardStatistics.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::InspectClearance,    PCB_ACTIONS::inspectClearance.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::InspectConstraints,  PCB_ACTIONS::inspectConstraints.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::DiffFootprint,       PCB_ACTIONS::diffFootprint.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::CompareBoardWithFile,
        PCB_ACTIONS::compareBoardWithFile.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::CompareBoardWithHistory, PCB_ACTIONS::compareBoardWithHistory.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ShowFootprintLinks,  PCB_ACTIONS::showFootprintAssociations.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::highlightNet.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::highlightNetSelection.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::toggleLastNetHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNetChain,     PCB_ACTIONS::highlightNetChain.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ClearHighlight,      PCB_ACTIONS::clearHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::toggleNetHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightItem,       PCB_ACTIONS::highlightItem.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ReplaceTerminalPad,  PCB_ACTIONS::setTerminalPad.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::HideNetInRatsnest,   PCB_ACTIONS::hideNetInRatsnest.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ShowNetInRatsnest,   PCB_ACTIONS::showNetInRatsnest.MakeEvent() );
}
