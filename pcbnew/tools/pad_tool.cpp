/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board_design_settings.h>
#include <board_item.h>
#include <footprint.h>
#include <fp_shape.h>
#include <pad.h>
#include <board_commit.h>
#include <dialogs/dialog_push_pad_properties.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_selection_conditions.h>
#include <tools/edit_tool.h>
#include <dialogs/dialog_enum_pads.h>
#include <widgets/infobar.h>

PAD_TOOL::PAD_TOOL() :
        PCB_TOOL_BASE( "pcbnew.PadTool" ),
        m_padCopied( false ),
        m_wasHighContrast( false ),
        m_editPad( niluuid )
{}


PAD_TOOL::~PAD_TOOL()
{}


void PAD_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_lastPadNumber = wxT( "1" );

    m_padCopied = false;
    m_editPad = niluuid;
}


bool PAD_TOOL::Init()
{
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    if( selTool )
    {
        // Add context menu entries that are displayed when selection tool is active
        CONDITIONAL_MENU& menu = selTool->GetToolMenu().GetMenu();

        SELECTION_CONDITION padSel = SELECTION_CONDITIONS::HasType( PCB_PAD_T );
        SELECTION_CONDITION singlePadSel = SELECTION_CONDITIONS::Count( 1 ) &&
                                           SELECTION_CONDITIONS::OnlyType( PCB_PAD_T );

        auto explodeCondition =
                [&]( const SELECTION& aSel )
                {
                    return m_editPad == niluuid && aSel.Size() == 1 && aSel[0]->Type() == PCB_PAD_T;
                };

        auto recombineCondition =
                [&]( const SELECTION& aSel )
                {
                    return m_editPad != niluuid;
                };

        menu.AddSeparator( 400 );

        if( m_isFootprintEditor )
        {
            menu.AddItem( PCB_ACTIONS::enumeratePads,      SELECTION_CONDITIONS::ShowAlways, 400 );
            menu.AddItem( PCB_ACTIONS::recombinePad,       recombineCondition, 400 );
            menu.AddItem( PCB_ACTIONS::explodePad,         explodeCondition, 400 );
        }

        menu.AddItem( PCB_ACTIONS::copyPadSettings,        singlePadSel, 400 );
        menu.AddItem( PCB_ACTIONS::applyPadSettings,       padSel, 400 );
        menu.AddItem( PCB_ACTIONS::pushPadSettings,        singlePadSel, 400 );
    }

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive,           SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( 1 );

    ctxMenu.AddItem( PCB_ACTIONS::rotateCcw,               SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::rotateCw,                SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::flip,                    SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::mirror,                  SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::properties,              SELECTION_CONDITIONS::ShowAlways );

    // Finally, add the standard zoom/grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( m_menu );

    return true;
}


int PAD_TOOL::pastePadProperties( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();
    const PAD*           masterPad = frame()->GetDesignSettings().m_Pad_Master.get();

    BOARD_COMMIT commit( frame() );

    // for every selected pad, paste global settings
    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == PCB_PAD_T )
        {
            commit.Modify( item );
            static_cast<PAD&>( *item ).ImportSettingsFrom( *masterPad );
        }
    }

    commit.Push( _( "Paste Pad Properties" ) );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    frame()->Refresh();

    return 0;
}


int PAD_TOOL::copyPadSettings( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    // can only copy from a single pad
    if( selection.Size() == 1 )
    {
        EDA_ITEM* item = selection[0];

        if( item->Type() == PCB_PAD_T )
        {
            const PAD& selPad = static_cast<const PAD&>( *item );
            frame()->GetDesignSettings().m_Pad_Master->ImportSettingsFrom( selPad );
            m_padCopied = true;
        }
    }

    return 0;
}


static void doPushPadProperties( BOARD& board, const PAD& aSrcPad, BOARD_COMMIT& commit,
                                 bool aSameFootprints,
                                 bool aPadShapeFilter,
                                 bool aPadOrientFilter,
                                 bool aPadLayerFilter,
                                 bool aPadTypeFilter )
{
    const FOOTPRINT* refFootprint = aSrcPad.GetParent();

    double pad_orient = aSrcPad.GetOrientation() - refFootprint->GetOrientation();

    for( FOOTPRINT* footprint : board.Footprints() )
    {
        if( !aSameFootprints && ( footprint != refFootprint ) )
            continue;

        if( footprint->GetFPID() != refFootprint->GetFPID() )
            continue;

        for( auto pad : footprint->Pads() )
        {
            if( aPadShapeFilter && ( pad->GetShape() != aSrcPad.GetShape() ) )
                continue;

            double currpad_orient = pad->GetOrientation() - footprint->GetOrientation();

            if( aPadOrientFilter && ( currpad_orient != pad_orient ) )
                continue;

            if( aPadLayerFilter && ( pad->GetLayerSet() != aSrcPad.GetLayerSet() ) )
                continue;

            if( aPadTypeFilter && ( pad->GetAttribute() != aSrcPad.GetAttribute() ) )
                    continue;

            // Special-case for aperture pads
            if( aPadTypeFilter && pad->GetAttribute() == PAD_ATTRIB::CONN )
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
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();
    PAD*                 srcPad;

    if( selection.Size() == 1 && selection[0]->Type() == PCB_PAD_T )
        srcPad = static_cast<PAD*>( selection[0] );
    else
        return 0;

    FOOTPRINT* footprint = srcPad->GetParent();

    if( !footprint )
        return 0;

    frame()->SetMsgPanel( footprint );

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
    if( !board()->GetFirstFootprint() || board()->GetFirstFootprint()->Pads().empty() )
        return 0;

    GENERAL_COLLECTOR collector;
    const KICAD_T types[] = { PCB_PAD_T, EOT };

    GENERAL_COLLECTORS_GUIDE guide = frame()->GetCollectorsGuide();
    guide.SetIgnoreMTextsMarkedNoShow( true );
    guide.SetIgnoreMTextsOnBack( true );
    guide.SetIgnoreMTextsOnFront( true );
    guide.SetIgnoreModulesVals( true );
    guide.SetIgnoreModulesRefs( true );

    DIALOG_ENUM_PADS settingsDlg( frame() );

    if( settingsDlg.ShowModal() != wxID_OK )
        return 0;

    int             seqPadNum = settingsDlg.GetStartNumber();
    wxString        padPrefix = settingsDlg.GetPrefix();
    std::deque<int> storedPadNumbers;
    std::map<wxString, std::pair<int, wxString>> oldNumbers;

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    std::string tool = aEvent.GetCommandStr().get();
    frame()->PushTool( tool );

    VECTOR2I        oldCursorPos;  // store the previous mouse cursor position, during mouse drag
    std::list<PAD*> selectedPads;
    BOARD_COMMIT    commit( frame() );
    bool            isFirstPoint = true;   // make sure oldCursorPos is initialized at least once

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    // Set initial cursor
    setCursor();

    STATUS_TEXT_POPUP statusPopup( frame() );
    wxString msg = _( "Click on pad %s%d\nPress <esc> to cancel or double-click to commit" );
    statusPopup.SetText( wxString::Format( msg, padPrefix, seqPadNum ) );
    statusPopup.Popup();
    statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

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
                    selectedPads.push_back( static_cast<PAD*>( collector[i] ) );
            }

            selectedPads.unique();

            for( PAD* pad : selectedPads )
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

                    wxString newNumber = wxString::Format( wxT( "%s%d" ), padPrefix, newval );
                    oldNumbers[newNumber] = { newval, pad->GetNumber() };
                    pad->SetNumber( newNumber );
                    SetLastPadNumber( newNumber );
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
                    auto it = oldNumbers.find( pad->GetNumber() );
                    wxASSERT( it != oldNumbers.end() );

                    if( it != oldNumbers.end() )
                    {
                        storedPadNumbers.push_back( it->second.first );
                        pad->SetNumber( it->second.second );
                        SetLastPadNumber( it->second.second );
                        oldNumbers.erase( it );

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
        {
            evt->SetPassEvent();
        }

        // Prepare the next loop by updating the old cursor mouse position
        // to this last mouse cursor position
        oldCursorPos = getViewControls()->GetCursorPosition();
        statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
    }

    for( PAD* p : board()->GetFirstFootprint()->Pads() )
    {
        p->ClearSelected();
        getView()->Update( p );
    }

    statusPopup.Hide();
    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int PAD_TOOL::PlacePad( const TOOL_EVENT& aEvent )
{
    if( !board()->GetFirstFootprint() )
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
            PAD* pad = new PAD( m_board->GetFirstFootprint() );

            pad->ImportSettingsFrom( *(m_frame->GetDesignSettings().m_Pad_Master.get()) );

            // If the user has set the footprint type to SMD, we assume that they would like to place
            // SMD pads
            if( m_board->GetFirstFootprint()->GetAttributes() & FP_SMD )
            {
                pad->SetAttribute( PAD_ATTRIB::SMD );
                pad->SetShape( PAD_SHAPE::ROUNDRECT );
                pad->SetSizeX( 1.5 * pad->GetSizeY() );
                pad->SetLayerSet( PAD::SMDMask() );
            }

            if( pad->CanHaveNumber() )
            {
                wxString padNumber = m_padTool->GetLastPadNumber();
                padNumber = m_board->GetFirstFootprint()->GetNextPadNumber( padNumber );
                pad->SetNumber( padNumber );
                m_padTool->SetLastPadNumber( padNumber );
            }

            return std::unique_ptr<BOARD_ITEM>( pad );
        }

        bool PlaceItem( BOARD_ITEM *aItem, BOARD_COMMIT& aCommit ) override
        {
            PAD* pad = dynamic_cast<PAD*>( aItem );

            if( pad )
            {
                m_frame->GetDesignSettings().m_Pad_Master->ImportSettingsFrom( *pad );
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
    PCB_SELECTION&      selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();
    wxString            msg;

    if( m_editPad != niluuid )
    {
        PAD* pad = dynamic_cast<PAD*>( frame()->GetItem( m_editPad ) );

        if( pad )
            recombinePad( pad );

        m_editPad = niluuid;
    }
    else if( selection.Size() == 1 && selection[0]->Type() == PCB_PAD_T )
    {
        PAD*         pad = static_cast<PAD*>( selection[0] );
        PCB_LAYER_ID layer = explodePad( pad );

        m_wasHighContrast = ( opts.m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL );
        frame()->SetActiveLayer( layer );

        if( !m_wasHighContrast )
            m_toolMgr->RunAction( ACTIONS::highContrastMode, false );

        if( PCB_ACTIONS::explodePad.GetHotKey() == PCB_ACTIONS::recombinePad.GetHotKey() )
        {
            msg.Printf( _( "Pad Edit Mode.  Press %s again to exit." ),
                        KeyNameFromKeyCode( PCB_ACTIONS::recombinePad.GetHotKey() ) );}

        else
        {
            msg.Printf( _( "Pad Edit Mode.  Press %s to exit." ),
                        KeyNameFromKeyCode( PCB_ACTIONS::recombinePad.GetHotKey() ) );
        }

        infoBar->RemoveAllButtons();
        infoBar->ShowMessage( msg, wxICON_INFORMATION );

        m_editPad = pad->m_Uuid;
    }

    if( m_editPad == niluuid )
    {
        bool highContrast = ( opts.m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL );

        if( m_wasHighContrast != highContrast )
            m_toolMgr->RunAction( ACTIONS::highContrastMode, false );

        infoBar->Dismiss();
    }

    return 0;
}


PCB_LAYER_ID PAD_TOOL::explodePad( PAD* aPad )
{
    PCB_LAYER_ID layer;
    BOARD_COMMIT commit( frame() );

    if( aPad->IsOnLayer( F_Cu ) )
        layer = F_Cu;
    else if( aPad->IsOnLayer( B_Cu ) )
        layer = B_Cu;
    else
        layer = *aPad->GetLayerSet().UIOrder();

    if( aPad->GetShape() == PAD_SHAPE::CUSTOM )
    {
        commit.Modify( aPad );

        for( const std::shared_ptr<PCB_SHAPE>& primitive : aPad->GetPrimitives() )
        {
            FP_SHAPE* shape = new FP_SHAPE( board()->GetFirstFootprint() );

            shape->SetShape( primitive->GetShape() );
            shape->SetFilled( primitive->IsFilled() );
            shape->SetWidth( primitive->GetWidth() );
            shape->SetStart( primitive->GetStart() );
            shape->SetEnd( primitive->GetEnd() );
            shape->SetBezierC1( primitive->GetBezierC1());
            shape->SetBezierC2( primitive->GetBezierC2());
            shape->SetAngle( primitive->GetAngle() );
            shape->SetPolyShape( primitive->GetPolyShape() );
            shape->SetLocalCoord();
            shape->Move( aPad->GetPosition() );
            shape->Rotate( aPad->GetPosition(), aPad->GetOrientation() );
            shape->SetLayer( layer );

            commit.Add( shape );
        }

        aPad->SetShape( aPad->GetAnchorPadShape() );
        aPad->DeletePrimitivesList();
        m_editPad = aPad->m_Uuid;
    }

    commit.Push( _("Edit pad shapes") );
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    return layer;
}


void PAD_TOOL::recombinePad( PAD* aPad )
{
    int  maxError = board()->GetDesignSettings().m_MaxError;

    auto findNext =
            [&]( PCB_LAYER_ID aLayer ) -> FP_SHAPE*
            {
                SHAPE_POLY_SET padPoly;
                aPad->TransformShapeWithClearanceToPolygon( padPoly, aLayer, 0, maxError,
                                                            ERROR_INSIDE );

                for( BOARD_ITEM* item : board()->GetFirstFootprint()->GraphicalItems() )
                {
                    PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

                    if( !shape || ( shape->GetEditFlags() & STRUCT_DELETED ) )
                        continue;

                    if( shape->GetLayer() != aLayer )
                        continue;

                    SHAPE_POLY_SET drawPoly;
                    shape->TransformShapeWithClearanceToPolygon( drawPoly, aLayer, 0, maxError,
                                                                 ERROR_INSIDE );
                    drawPoly.BooleanIntersection( padPoly, SHAPE_POLY_SET::PM_FAST );

                    if( !drawPoly.IsEmpty() )
                        return (FP_SHAPE*) item;
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

    while( FP_SHAPE* fpShape = findNext( layer ) )
    {
        commit.Modify( aPad );

        // We've found an intersecting item.  First convert the pad to a custom-shape
        // pad (if it isn't already)
        //
        if( aPad->GetShape() == PAD_SHAPE::RECT || aPad->GetShape() == PAD_SHAPE::CIRCLE )
        {
            aPad->SetAnchorPadShape( aPad->GetShape() );
        }
        else if( aPad->GetShape() != PAD_SHAPE::CUSTOM )
        {
            // Create a new minimally-sized circular anchor and convert existing pad
            // to a polygon primitive
            SHAPE_POLY_SET existingOutline;
            aPad->TransformShapeWithClearanceToPolygon( existingOutline, layer, 0, maxError,
                                                        ERROR_INSIDE );

            aPad->SetAnchorPadShape( PAD_SHAPE::CIRCLE );
            if( aPad->GetSizeX() > aPad->GetSizeY() )
                aPad->SetSizeX( aPad->GetSizeY() );

            aPad->SetOffset( wxPoint( 0, 0 ) );

            PCB_SHAPE* shape = new PCB_SHAPE( nullptr, SHAPE_T::POLY );
            shape->SetFilled( true );
            shape->SetWidth( 0 );
            shape->SetPolyShape( existingOutline );
            shape->Move( - aPad->GetPosition() );
            shape->Rotate( wxPoint( 0, 0 ), - aPad->GetOrientation() );

            aPad->AddPrimitive( shape );
        }

        aPad->SetShape( PAD_SHAPE::CUSTOM );

        // Now add the new shape to the primitives list
        //
        PCB_SHAPE* pcbShape = new PCB_SHAPE;

        pcbShape->SetShape( fpShape->GetShape() );
        pcbShape->SetFilled( fpShape->IsFilled() );
        pcbShape->SetWidth( fpShape->GetWidth() );
        pcbShape->SetStart( fpShape->GetStart() );
        pcbShape->SetEnd( fpShape->GetEnd() );
        pcbShape->SetBezierC1( fpShape->GetBezierC1());
        pcbShape->SetBezierC2( fpShape->GetBezierC2());
        pcbShape->SetAngle( fpShape->GetAngle() );
        pcbShape->SetPolyShape( fpShape->GetPolyShape() );

        pcbShape->Move( - aPad->GetPosition() );
        pcbShape->Rotate( wxPoint( 0, 0 ), - aPad->GetOrientation() );
        aPad->AddPrimitive( pcbShape );

        fpShape->SetFlags( STRUCT_DELETED );
        commit.Remove( fpShape );
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
