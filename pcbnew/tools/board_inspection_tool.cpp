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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <bitmaps.h>
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
#include <string_utils.h>
#include <tools/board_inspection_tool.h>
#include <footprint_library_adapter.h>
#include <pcb_shape.h>
#include <widgets/appearance_controls.h>
#include <widgets/wx_html_report_box.h>
#include <widgets/footprint_diff_widget.h>
#include <drc/drc_item.h>
#include <pad.h>
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
        engine->InitEngine( m_frame->GetDesignRulesPath() );
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

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxCHECK( selTool, 0 );

    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() != 2 )
    {
        m_frame->ShowInfoBarError( _( "Select two items for a clearance resolution report." ) );
        return 0;
    }

    if( !selection.GetItem( 0 )->IsBOARD_ITEM() || !selection.GetItem( 1 )->IsBOARD_ITEM() )
        return 0;

    BOARD_ITEM* a = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) );
    BOARD_ITEM* b = static_cast<BOARD_ITEM*>( selection.GetItem( 1 ) );

    if( a->Type() == PCB_GROUP_T )
    {
        PCB_GROUP* ag = static_cast<PCB_GROUP*>( a );

        if( ag->GetItems().empty() )
        {
            m_frame->ShowInfoBarError( _( "Cannot generate clearance report on empty group." ) );
            return 0;
        }

        a = static_cast<BOARD_ITEM*>( *ag->GetItems().begin() );
    }

    if( b->Type() == PCB_GROUP_T )
    {
        PCB_GROUP* bg = static_cast<PCB_GROUP*>( b );

        if( bg->GetItems().empty() )
        {
            m_frame->ShowInfoBarError( _( "Cannot generate clearance report on empty group." ) );
            return 0;
        }

        b = static_cast<BOARD_ITEM*>( *bg->GetItems().begin() );
    }

    // a or b could be null after group tests above.
    if( !a || !b )
        return 0;

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

    // a or b could be null after footprint tests above.
    if( !a || !b )
        return 0;

    DIALOG_BOOK_REPORTER* dialog = m_frame->GetInspectClearanceDialog();

    wxCHECK( dialog, 0 );

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

            // Report a 0 clearance for solid connections
            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                         m_frame->StringFromValue( 0, true ) ) );
        }

        r->Flush();
    }
    else if( copperIntersection.any() && !aFP && !bFP )
    {
        PCB_LAYER_ID layer = active;

        if( !copperIntersection.test( layer ) )
            layer = copperIntersection.Seq().front();

        r = dialog->AddHTMLPage( m_frame->GetBoard()->GetLayerName( layer ) );
        reportHeader( _( "Clearance resolution for:" ), a, b, layer, r );

        if( ac && bc && ac->GetNetCode() > 0 && ac->GetNetCode() == bc->GetNetCode() )
        {
            // Same nets....
            r->Report( _( "Items belong to the same net. Min clearance is 0." ) );
        }
        else
        {
            // Different nets (or one or both unconnected)....
            constraint = drcEngine->EvalRules( CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );

            if( constraint.IsNull() )
            {
                r->Report( _( "Min clearance is 0." ) );
            }
            else if( clearance < 0 )
            {
                r->Report( wxString::Format( _( "Resolved clearance: %s; clearance will not be "
                                                "tested." ),
                                             m_frame->StringFromValue( clearance, true ) ) );
            }
            else
            {
                r->Report( wxString::Format( _( "Resolved min clearance: %s." ),
                                             m_frame->StringFromValue( clearance, true ) ) );
            }
        }

        r->Flush();
    }

    if( ac && bc )
    {
        NETINFO_ITEM* refNet = ac->GetNet();
        wxString      coupledNet;
        wxString      dummy;

        if( DRC_ENGINE::MatchDpSuffix( refNet->GetNetname(), coupledNet, dummy )
                && bc->GetNetname() == coupledNet )
        {
            r = dialog->AddHTMLPage( _( "Diff Pair" ) );
            reportHeader( _( "Diff-pair gap resolution for:" ), ac, bc, active, r );

            constraint = drcEngine->EvalRules( DIFF_PAIR_GAP_CONSTRAINT, ac, bc, active, r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved gap constraints: min %s; opt %s; max %s." ),
                                         reportMin( m_frame, constraint ),
                                         reportOpt( m_frame, constraint ),
                                         reportMax( m_frame, constraint ) ) );

            r->Report( "" );
            r->Report( "" );
            r->Report( "" );
            reportHeader( _( "Diff-pair max uncoupled length resolution for:" ), ac, bc,
                          active, r );

            if( !drcEngine->HasRulesForConstraintType( MAX_UNCOUPLED_CONSTRAINT ) )
            {
                r->Report( "" );
                r->Report( _( "No 'diff_pair_uncoupled' constraints defined." ) );
            }
            else
            {
                constraint = drcEngine->EvalRules( MAX_UNCOUPLED_CONSTRAINT, ac, bc, active, r );

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
        else if( b->HasHole() && b->IsOnCopperLayer() )
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
    return 0;
}


int BOARD_INSPECTION_TOOL::InspectConstraints( const TOOL_EVENT& aEvent )
{
#define EVAL_RULES( constraint, a, b, layer, r ) drcEngine->EvalRules( constraint, a, b, layer, r )

    wxCHECK( m_frame, 0 );

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    wxCHECK( selTool, 0 );

    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() != 1 )
    {
        m_frame->ShowInfoBarError( _( "Select a single item for a constraints resolution report." ) );
        return 0;
    }

    DIALOG_BOOK_REPORTER* dialog = m_frame->GetInspectConstraintsDialog();

    wxCHECK( dialog, 0 );

    dialog->DeleteAllPages();

    if( !selection.GetItem( 0 )->IsBOARD_ITEM() )
        return 0;

    BOARD_ITEM*    item = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) );
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

    m_frame->m_probingSchToPcb = true; // recursion guard
    {
        m_toolMgr->RunAction( ACTIONS::selectionClear );

        if( item )
            m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, item );
    }
    m_frame->m_probingSchToPcb = false;

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

    // Toggle highlight when the same net was picked
    if( !aUseSelection && netcodes.size() == 1 && netcodes.contains( net ) )
        enableHighlight = !settings->IsHighlightEnabled();

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


int BOARD_INSPECTION_TOOL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    BOARD*                  board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* settings = m_toolMgr->GetView()->GetPainter()->GetSettings();

    m_currentlyHighlighted.clear();
    m_lastHighlighted.clear();

    board->ResetNetHighLight();
    settings->SetHighlight( false );
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

    auto selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    auto& selection = selectionTool->GetSelection();
    auto connectivity = getModel<BOARD>()->GetConnectivity();

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
    Go( &BOARD_INSPECTION_TOOL::ShowFootprintLinks,  PCB_ACTIONS::showFootprintAssociations.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::highlightNet.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::highlightNetSelection.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::toggleLastNetHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ClearHighlight,      PCB_ACTIONS::clearHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,        PCB_ACTIONS::toggleNetHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightItem,       PCB_ACTIONS::highlightItem.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::HideNetInRatsnest,   PCB_ACTIONS::hideNetInRatsnest.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ShowNetInRatsnest,   PCB_ACTIONS::showNetInRatsnest.MakeEvent() );
}
