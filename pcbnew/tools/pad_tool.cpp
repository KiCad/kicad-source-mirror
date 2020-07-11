/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include "pad_tool.h"
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <bitmaps.h>
#include <class_board_item.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <board_commit.h>
#include <dialogs/dialog_push_pad_properties.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>
#include <tools/pcb_selection_conditions.h>
#include <tools/edit_tool.h>
#include <dialogs/dialog_enum_pads.h>
#include <pad_naming.h>
#include <widgets/infobar.h>

PAD_TOOL::PAD_TOOL() :
        PCB_TOOL_BASE( "pcbnew.PadTool" ),
        m_padCopied( false ),
        m_editPad( niluuid )
{}


PAD_TOOL::~PAD_TOOL()
{}


void PAD_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_lastPadName = wxT( "1" );

    m_padCopied = false;
    m_editPad = niluuid;
}


bool PAD_TOOL::Init()
{
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( selTool )
    {
        // Add context menu entries that are displayed when selection tool is active
        CONDITIONAL_MENU& menu = selTool->GetToolMenu().GetMenu();

        SELECTION_CONDITION padSel = SELECTION_CONDITIONS::HasType( PCB_PAD_T );
        SELECTION_CONDITION singlePadSel = SELECTION_CONDITIONS::Count( 1 ) &&
                                           SELECTION_CONDITIONS::OnlyType( PCB_PAD_T );

        auto explodeCondition = [&]( const SELECTION& aSel )
                                {
                                    return m_editPad == niluuid
                                           && aSel.Size() == 1 && aSel[0]->Type() == PCB_PAD_T;
                                };

        auto recombineCondition = [&]( const SELECTION& aSel )
                                  {
                                      return m_editPad != niluuid;
                                  };

        menu.AddSeparator( 400 );

        if( m_editModules )
        {
            menu.AddItem( PCB_ACTIONS::enumeratePads,      SELECTION_CONDITIONS::ShowAlways, 400 );
            menu.AddItem( PCB_ACTIONS::recombinePad,       recombineCondition, 400 );
            menu.AddItem( PCB_ACTIONS::explodePad,         explodeCondition, 400 );
        }

        menu.AddItem( PCB_ACTIONS::copyPadSettings,        singlePadSel, 400 );
        menu.AddItem( PCB_ACTIONS::applyPadSettings,       padSel, 400 );
        menu.AddItem( PCB_ACTIONS::pushPadSettings,        singlePadSel, 400 );
    }

    return true;
}


int PAD_TOOL::pastePadProperties( const TOOL_EVENT& aEvent )
{
    auto& selTool = *m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool.GetSelection();
    const D_PAD& masterPad = frame()->GetDesignSettings().m_Pad_Master;

    BOARD_COMMIT commit( frame() );

    // for every selected pad, paste global settings
    for( auto item : selection )
    {
        if( item->Type() == PCB_PAD_T )
        {
            commit.Modify( item );
            static_cast<D_PAD&>( *item ).ImportSettingsFrom( masterPad );
        }
    }

    commit.Push( _( "Paste Pad Properties" ) );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    frame()->Refresh();

    return 0;
}


int PAD_TOOL::copyPadSettings( const TOOL_EVENT& aEvent )
{
    auto& selTool = *m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool.GetSelection();

    D_PAD& masterPad = frame()->GetDesignSettings().m_Pad_Master;

    // can only copy from a single pad
    if( selection.Size() == 1 )
    {
        auto item = selection[0];

        if( item->Type() == PCB_PAD_T )
        {
            const auto& selPad = static_cast<const D_PAD&>( *item );
            masterPad.ImportSettingsFrom( selPad );
            m_padCopied = true;
        }
    }

    return 0;
}


static void doPushPadProperties( BOARD& board, const D_PAD& aSrcPad, BOARD_COMMIT& commit,
                                 bool aSameFootprints,
                                 bool aPadShapeFilter,
                                 bool aPadOrientFilter,
                                 bool aPadLayerFilter,
                                 bool aPadTypeFilter )
{
    const MODULE* moduleRef = aSrcPad.GetParent();

    double pad_orient = aSrcPad.GetOrientation() - moduleRef->GetOrientation();

    for( auto module : board.Modules() )
    {
        if( !aSameFootprints && ( module != moduleRef ) )
            continue;

        if( module->GetFPID() != moduleRef->GetFPID() )
            continue;

        for( auto pad : module->Pads() )
        {
            if( aPadShapeFilter && ( pad->GetShape() != aSrcPad.GetShape() ) )
                continue;

            double currpad_orient = pad->GetOrientation() - module->GetOrientation();

            if( aPadOrientFilter && ( currpad_orient != pad_orient ) )
                continue;

            if( aPadLayerFilter && ( pad->GetLayerSet() != aSrcPad.GetLayerSet() ) )
                continue;

            if( aPadTypeFilter && ( pad->GetAttribute() != aSrcPad.GetAttribute() ) )
                    continue;

            // Special-case for aperture pads
            if( aPadTypeFilter && pad->GetAttribute() == PAD_ATTRIB_CONN )
            {
                if( pad->IsAperturePad() != aSrcPad.IsAperturePad() )
                    continue;
            }

            commit.Modify( pad );

            // Apply source pad settings to this pad
            pad->ImportSettingsFrom( aSrcPad );
        }
    }
}


int PAD_TOOL::pushPadSettings( const TOOL_EVENT& aEvent )
{
    auto&       selTool = *m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool.GetSelection();
    D_PAD*      srcPad;

    if( selection.Size() == 1 && selection[0]->Type() == PCB_PAD_T )
        srcPad = static_cast<D_PAD*>( selection[0] );
    else
        return 0;

    MODULE* module = srcPad->GetParent();

    if( !module )
        return 0;

    frame()->SetMsgPanel( module );

    DIALOG_PUSH_PAD_PROPERTIES dlg( frame() );
    int dialogRet = dlg.ShowModal();

    if( dialogRet == wxID_CANCEL )
        return 0;

    const bool edit_Same_Modules = (dialogRet == 1);

    BOARD_COMMIT commit( frame() );

    doPushPadProperties( *getModel<BOARD>(), *srcPad, commit, edit_Same_Modules,
                         DIALOG_PUSH_PAD_PROPERTIES::m_Pad_Shape_Filter,
                         DIALOG_PUSH_PAD_PROPERTIES::m_Pad_Orient_Filter,
                         DIALOG_PUSH_PAD_PROPERTIES::m_Pad_Layer_Filter,
                         DIALOG_PUSH_PAD_PROPERTIES::m_Pad_Type_Filter );

    commit.Push( _( "Push Pad Settings" ) );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    frame()->Refresh();

    return 0;
}


int PAD_TOOL::EnumeratePads( const TOOL_EVENT& aEvent )
{
    if( !board()->GetFirstModule() || board()->GetFirstModule()->Pads().empty() )
        return 0;

    DIALOG_ENUM_PADS settingsDlg( frame() );

    if( settingsDlg.ShowModal() != wxID_OK )
        return 0;

    std::string tool = aEvent.GetCommandStr().get();
    frame()->PushTool( tool );
    Activate();

    GENERAL_COLLECTOR collector;
    const KICAD_T types[] = { PCB_PAD_T, EOT };

    GENERAL_COLLECTORS_GUIDE guide = frame()->GetCollectorsGuide();
    guide.SetIgnoreMTextsMarkedNoShow( true );
    guide.SetIgnoreMTextsOnBack( true );
    guide.SetIgnoreMTextsOnFront( true );
    guide.SetIgnoreModulesVals( true );
    guide.SetIgnoreModulesRefs( true );

    int seqPadNum = settingsDlg.GetStartNumber();
    wxString padPrefix = settingsDlg.GetPrefix();
    std::deque<int> storedPadNumbers;

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    getViewControls()->ShowCursor( true );

    KIGFX::VIEW* view = m_toolMgr->GetView();
    VECTOR2I oldCursorPos;  // store the previous mouse cursor position, during mouse drag
    std::list<D_PAD*> selectedPads;
    BOARD_COMMIT commit( frame() );
    std::map<wxString, std::pair<int, wxString>> oldNames;
    bool isFirstPoint = true;   // used to be sure oldCursorPos will be initialized at least once.

    STATUS_TEXT_POPUP statusPopup( frame() );
    wxString msg = _( "Click on pad %s%d\nPress <esc> to cancel or double-click to commit" );
    statusPopup.SetText( wxString::Format( msg, padPrefix, seqPadNum ) );
    statusPopup.Popup();
    statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );

    while( TOOL_EVENT* evt = Wait() )
    {
        frame()->GetCanvas()->SetCurrentCursor( wxCURSOR_BULLSEYE );

        if( evt->IsCancelInteractive() )
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
            commit.Revert();

            frame()->PopTool( tool );
            break;
        }

        else if( evt->IsActivate() )
        {
            commit.Push( _( "Renumber pads" ) );

            frame()->PopTool( tool );
            break;
        }

        else if( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            selectedPads.clear();
            VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

            // Be sure the old cursor mouse position was initialized:
            if( isFirstPoint )
            {
                oldCursorPos = cursorPos;
                isFirstPoint = false;
            }

            // wxWidgets deliver mouse move events not frequently enough, resulting in skipping
            // pads if the user moves cursor too fast. To solve it, create a line that approximates
            // the mouse move and search pads that are on the line.
            int distance = ( cursorPos - oldCursorPos ).EuclideanNorm();
            // Search will be made every 0.1 mm:
            int segments = distance / int( 0.1*IU_PER_MM ) + 1;
            const wxPoint line_step( ( cursorPos - oldCursorPos ) / segments );

            collector.Empty();

            for( int j = 0; j < segments; ++j )
            {
                wxPoint testpoint( cursorPos.x - j * line_step.x, cursorPos.y - j * line_step.y );
                collector.Collect( board(), types, testpoint, guide );

                for( int i = 0; i < collector.GetCount(); ++i )
                    selectedPads.push_back( static_cast<D_PAD*>( collector[i] ) );
            }

            selectedPads.unique();

            for( D_PAD* pad : selectedPads )
            {
                // If pad was not selected, then enumerate it
                if( !pad->IsSelected() )
                {
                    commit.Modify( pad );

                    // Rename pad and store the old name
                    int newval;

                    if( storedPadNumbers.size() > 0 )
                    {
                        newval = storedPadNumbers.front();
                        storedPadNumbers.pop_front();
                    }
                    else
                        newval = seqPadNum++;

                    wxString newName = wxString::Format( wxT( "%s%d" ), padPrefix, newval );
                    oldNames[newName] = { newval, pad->GetName() };
                    pad->SetName( newName );
                    SetLastPadName( newName );
                    pad->SetSelected();
                    getView()->Update( pad );

                    // Ensure the popup text shows the correct next value
                    if( storedPadNumbers.size() > 0 )
                        newval = storedPadNumbers.front();
                    else
                        newval = seqPadNum;

                    statusPopup.SetText( wxString::Format( msg, padPrefix, newval ) );
                }

                // ... or restore the old name if it was enumerated and clicked again
                else if( pad->IsSelected() && evt->IsClick( BUT_LEFT ) )
                {
                    auto it = oldNames.find( pad->GetName() );
                    wxASSERT( it != oldNames.end() );

                    if( it != oldNames.end() )
                    {
                        storedPadNumbers.push_back( it->second.first );
                        pad->SetName( it->second.second );
                        SetLastPadName( it->second.second );
                        oldNames.erase( it );

                        int newval = storedPadNumbers.front();

                        statusPopup.SetText( wxString::Format( msg, padPrefix, newval ) );
                    }

                    pad->ClearSelected();
                    getView()->Update( pad );
                }
            }
        }

        else if( ( evt->IsKeyPressed() && evt->KeyCode() == WXK_RETURN ) ||
                 evt->IsDblClick( BUT_LEFT ) )
        {
            commit.Push( _( "Renumber pads" ) );
            frame()->PopTool( tool );
            break;
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }

        else
            evt->SetPassEvent();

        // Prepare the next loop by updating the old cursor mouse position
        // to this last mouse cursor position
        oldCursorPos = getViewControls()->GetCursorPosition();
        statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
    }

    for( auto p : board()->GetFirstModule()->Pads() )
    {
        p->ClearSelected();
        view->Update( p );
    }

    statusPopup.Hide();
    return 0;
}


int PAD_TOOL::PlacePad( const TOOL_EVENT& aEvent )
{
    if( !board()->GetFirstModule() )
        return 0;

    struct PAD_PLACER : public INTERACTIVE_PLACER_BASE
    {
        PAD_PLACER( PAD_TOOL* aPadTool )
        {
            m_padTool = aPadTool;
        }

        virtual ~PAD_PLACER()
        {
        }

        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            D_PAD* pad = new D_PAD( m_board->GetFirstModule() );

            pad->ImportSettingsFrom( m_frame->GetDesignSettings().m_Pad_Master );

            if( PAD_NAMING::PadCanHaveName( *pad ) )
            {
                wxString padName = m_padTool->GetLastPadName();
                padName = m_board->GetFirstModule()->GetNextPadName( padName );
                pad->SetName( padName );
                m_padTool->SetLastPadName( padName );
            }

            return std::unique_ptr<BOARD_ITEM>( pad );
        }

        bool PlaceItem( BOARD_ITEM *aItem, BOARD_COMMIT& aCommit ) override
        {
            D_PAD* pad = dynamic_cast<D_PAD*>( aItem );

            if( pad )
            {
                m_frame->GetDesignSettings().m_Pad_Master.ImportSettingsFrom( *pad );
                pad->SetLocalCoord();
                aCommit.Add( aItem );
                return true;
            }

            return false;
        }

        PAD_TOOL* m_padTool;
    };

    PAD_PLACER placer( this );

    doInteractiveItemPlacement( aEvent.GetCommandStr().get(), &placer,  _( "Place pad" ),
                                IPO_REPEAT | IPO_SINGLE_CLICK | IPO_ROTATE | IPO_FLIP );

    return 0;
}


int PAD_TOOL::EditPad( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = frame()->GetDisplayOptions();
    WX_INFOBAR*         infoBar = frame()->GetInfoBar();
    PCBNEW_SELECTION&   selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();
    wxString            msg;

    if( m_editPad != niluuid )
    {
        D_PAD* pad = dynamic_cast<D_PAD*>( frame()->GetItem( m_editPad ) );

        if( pad )
            recombinePad( pad );

        m_editPad = niluuid;
    }
    else if( selection.Size() == 1 && selection[0]->Type() == PCB_PAD_T )
    {
        D_PAD*       pad = static_cast<D_PAD*>( selection[0] );
        PCB_LAYER_ID layer = explodePad( pad );

        m_wasHighContrast = ( opts.m_ContrastModeDisplay !=
                              HIGH_CONTRAST_MODE::NORMAL );
        frame()->SetActiveLayer( layer );

        if( !m_wasHighContrast )
            m_toolMgr->RunAction( ACTIONS::highContrastMode, false );

        if( PCB_ACTIONS::explodePad.GetHotKey() == PCB_ACTIONS::recombinePad.GetHotKey() )
            msg.Printf( _( "Pad Edit Mode.  Press %s again to exit." ),
                        KeyNameFromKeyCode( PCB_ACTIONS::recombinePad.GetHotKey() ) );
        else
            msg.Printf( _( "Pad Edit Mode.  Press %s to exit." ),
                        KeyNameFromKeyCode( PCB_ACTIONS::recombinePad.GetHotKey() ) );

        infoBar->RemoveAllButtons();
        infoBar->ShowMessage( msg, wxICON_INFORMATION );

        m_editPad = pad->m_Uuid;
    }

    if( m_editPad == niluuid )
    {
        bool highContrast = ( opts.m_ContrastModeDisplay !=
                              HIGH_CONTRAST_MODE::NORMAL );

        if( m_wasHighContrast != highContrast )
            m_toolMgr->RunAction( ACTIONS::highContrastMode, false );

        infoBar->Dismiss();
    }

    return 0;
}


PCB_LAYER_ID PAD_TOOL::explodePad( D_PAD* aPad )
{
    PCB_LAYER_ID layer;
    BOARD_COMMIT commit( frame() );

    if( aPad->IsOnLayer( F_Cu ) )
        layer = F_Cu;
    else if( aPad->IsOnLayer( B_Cu ) )
        layer = B_Cu;
    else
        layer = *aPad->GetLayerSet().UIOrder();

    if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
    {
        commit.Modify( aPad );

        for( const std::shared_ptr<DRAWSEGMENT>& primitive : aPad->GetPrimitives() )
        {
            EDGE_MODULE* ds = new EDGE_MODULE( board()->GetFirstModule() );

            ds->SetShape( primitive->GetShape() );
            ds->SetWidth( primitive->GetWidth() );
            ds->SetStart( primitive->GetStart() );
            ds->SetEnd( primitive->GetEnd() );
            ds->SetBezControl1( primitive->GetBezControl1() );
            ds->SetBezControl2( primitive->GetBezControl2() );
            ds->SetAngle( primitive->GetAngle() );
            ds->SetPolyShape( primitive->GetPolyShape() );
            ds->SetLocalCoord();
            ds->Move( aPad->GetPosition() );
            ds->Rotate( aPad->GetPosition(), aPad->GetOrientation() );
            ds->SetLayer( layer );

            commit.Add( ds );
        }

        aPad->SetShape( aPad->GetAnchorPadShape() );
        aPad->DeletePrimitivesList();
        m_editPad = aPad->m_Uuid;
    }

    commit.Push( _("Edit pad shapes") );
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    return layer;
}


void PAD_TOOL::recombinePad( D_PAD* aPad )
{
    auto findNext = [&]( PCB_LAYER_ID aLayer ) -> EDGE_MODULE*
                    {
                        SHAPE_POLY_SET padPoly;
                        aPad->TransformShapeWithClearanceToPolygon( padPoly, 0 );

                        for( BOARD_ITEM* item : board()->GetFirstModule()->GraphicalItems() )
                        {
                            DRAWSEGMENT* draw = dynamic_cast<DRAWSEGMENT*>( item );

                            if( !draw || ( draw->GetEditFlags() & STRUCT_DELETED ) )
                                continue;

                            if( draw->GetLayer() != aLayer )
                                continue;

                            SHAPE_POLY_SET drawPoly;
                            draw->TransformShapeWithClearanceToPolygon( drawPoly, 0 );
                            drawPoly.BooleanIntersection( padPoly, SHAPE_POLY_SET::PM_FAST );

                            if( !drawPoly.IsEmpty() )
                                return (EDGE_MODULE*) item;
                        }

                        return nullptr;
                    };

    BOARD_COMMIT commit( frame() );
    PCB_LAYER_ID layer;

    if( aPad->IsOnLayer( F_Cu ) )
        layer = F_Cu;
    else if( aPad->IsOnLayer( B_Cu ) )
        layer = B_Cu;
    else
        layer = *aPad->GetLayerSet().UIOrder();

    while( EDGE_MODULE* edge = findNext( layer ) )
    {
        commit.Modify( aPad );

        // We've found an intersecting item.  First convert the pad to a custom-shape
        // pad (if it isn't already)
        //
        if( aPad->GetShape() == PAD_SHAPE_RECT || aPad->GetShape() == PAD_SHAPE_CIRCLE )
        {
            aPad->SetAnchorPadShape( aPad->GetShape() );
        }
        else if( aPad->GetShape() != PAD_SHAPE_CUSTOM )
        {
            // Create a new minimally-sized circular anchor and convert existing pad
            // to a polygon primitive
            SHAPE_POLY_SET existingOutline;
            int maxError = board()->GetDesignSettings().m_MaxError;
            aPad->TransformShapeWithClearanceToPolygon( existingOutline, 0, maxError );

            aPad->SetAnchorPadShape( PAD_SHAPE_CIRCLE );
            wxSize minAnnulus( Millimeter2iu( 0.2 ), Millimeter2iu( 0.2 ) );
            aPad->SetSize( aPad->GetDrillSize() + minAnnulus );
            aPad->SetOffset( wxPoint( 0, 0 ) );

            DRAWSEGMENT* shape = new DRAWSEGMENT;
            shape->SetShape( S_POLYGON );
            shape->SetPolyShape( existingOutline );
            shape->Move( - aPad->GetPosition() );
            shape->Rotate( wxPoint( 0, 0 ), - aPad->GetOrientation() );

            aPad->AddPrimitive( shape );
        }

        aPad->SetShape( PAD_SHAPE_CUSTOM );

        // Now add the new shape to the primitives list
        //
        DRAWSEGMENT* ds = new DRAWSEGMENT;

        ds->SetShape( edge->GetShape() );
        ds->SetWidth( edge->GetWidth() );
        ds->SetStart( edge->GetStart() );
        ds->SetEnd( edge->GetEnd() );
        ds->SetBezControl1( edge->GetBezControl1() );
        ds->SetBezControl2( edge->GetBezControl2() );
        ds->SetAngle( edge->GetAngle() );
        ds->SetPolyShape( edge->GetPolyShape() );

        ds->Move( - aPad->GetPosition() );
        ds->Rotate( wxPoint( 0, 0 ), - aPad->GetOrientation() );
        aPad->AddPrimitive( ds );

        edge->SetFlags( STRUCT_DELETED );
        commit.Remove( edge );
    }

    commit.Push(_("Recombine pads") );
}


void PAD_TOOL::setTransitions()
{
    Go( &PAD_TOOL::pastePadProperties,      PCB_ACTIONS::applyPadSettings.MakeEvent() );
    Go( &PAD_TOOL::copyPadSettings,         PCB_ACTIONS::copyPadSettings.MakeEvent() );
    Go( &PAD_TOOL::pushPadSettings,         PCB_ACTIONS::pushPadSettings.MakeEvent() );

    Go( &PAD_TOOL::PlacePad,                PCB_ACTIONS::placePad.MakeEvent() );
    Go( &PAD_TOOL::EnumeratePads,           PCB_ACTIONS::enumeratePads.MakeEvent() );

    Go( &PAD_TOOL::EditPad,                 PCB_ACTIONS::explodePad.MakeEvent() );
    Go( &PAD_TOOL::EditPad,                 PCB_ACTIONS::recombinePad.MakeEvent() );
}
