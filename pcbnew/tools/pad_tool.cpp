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


#include "pad_tool.h"
#include "pcb_painter.h"
#include <kiplatform/ui.h>
#include <macros.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <tool/tool_manager.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pad.h>
#include <pcbnew_settings.h>
#include <board_commit.h>
#include <dialogs/dialog_push_pad_properties.h>
#include <dialogs/dialog_fp_edit_pad_table.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_selection_conditions.h>
#include <tools/edit_tool.h>
#include <dialogs/dialog_enum_pads.h>
#include <widgets/wx_infobar.h>


using KIGFX::PCB_RENDER_SETTINGS;


PAD_TOOL::PAD_TOOL() :
        PCB_TOOL_BASE( "pcbnew.PadTool" ),
        m_previousHighContrastMode( HIGH_CONTRAST_MODE::NORMAL ),
        m_editPad( niluuid )
{}


void PAD_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_lastPadNumber = wxT( "1" );

    if( board() && board()->ResolveItem( m_editPad ) == DELETED_BOARD_ITEM::GetInstance() )
    {
        PCB_DISPLAY_OPTIONS opts = frame()->GetDisplayOptions();

        if( m_previousHighContrastMode != opts.m_ContrastModeDisplay )
        {
            opts.m_ContrastModeDisplay = m_previousHighContrastMode;
            frame()->SetDisplayOptions( opts );
        }

        frame()->GetInfoBar()->Dismiss();

        m_editPad = niluuid;
    }
}


bool PAD_TOOL::Init()
{
    static const std::vector<KICAD_T> padTypes = { PCB_PAD_T };

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    if( selTool )
    {
        // Add context menu entries that are displayed when selection tool is active
        CONDITIONAL_MENU& menu = selTool->GetToolMenu().GetMenu();

        SELECTION_CONDITION padSel = SELECTION_CONDITIONS::HasType( PCB_PAD_T );
        SELECTION_CONDITION singlePadSel = SELECTION_CONDITIONS::Count( 1 ) &&
                                           SELECTION_CONDITIONS::OnlyTypes( padTypes );

        auto explodeCondition =
                [this]( const SELECTION& aSel )
                {
                    return m_editPad == niluuid && aSel.Size() == 1 && aSel[0]->Type() == PCB_PAD_T;
                };

        auto recombineCondition =
                [this]( const SELECTION& aSel )
                {
                    return m_editPad != niluuid;
                };

        menu.AddSeparator( 400 );

        if( m_isFootprintEditor )
        {
            menu.AddItem( PCB_ACTIONS::padTable,           SELECTION_CONDITIONS::ShowAlways, 400 );
            menu.AddItem( PCB_ACTIONS::enumeratePads,      SELECTION_CONDITIONS::ShowAlways, 400 );
            menu.AddItem( PCB_ACTIONS::recombinePad,       recombineCondition, 400 );
            menu.AddItem( PCB_ACTIONS::explodePad,         explodeCondition, 400 );
        }

        menu.AddItem( PCB_ACTIONS::copyPadSettings,        singlePadSel, 400 );
        menu.AddItem( PCB_ACTIONS::applyPadSettings,       padSel, 400 );
        menu.AddItem( PCB_ACTIONS::pushPadSettings,        singlePadSel, 400 );
    }

    auto& ctxMenu = m_menu->GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive,           SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( 1 );

    ctxMenu.AddItem( PCB_ACTIONS::rotateCcw,               SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::rotateCw,                SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::flip,                    SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::mirrorH,                 SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::mirrorV,                 SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::properties,              SELECTION_CONDITIONS::ShowAlways );

    // Finally, add the standard zoom/grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( *m_menu.get() );

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
        }
    }

    return 0;
}


static void doPushPadProperties( BOARD& board, const PAD& aSrcPad, BOARD_COMMIT& commit,
                                 bool aSameFootprints, bool aPadShapeFilter, bool aPadOrientFilter,
                                 bool aPadLayerFilter, bool aPadTypeFilter )
{
    const FOOTPRINT* refFootprint = aSrcPad.GetParentFootprint();

    EDA_ANGLE srcPadAngle = aSrcPad.GetOrientation() - refFootprint->GetOrientation();

    for( FOOTPRINT* footprint : board.Footprints() )
    {
        if( !aSameFootprints && ( footprint != refFootprint ) )
            continue;

        if( footprint->GetFPID() != refFootprint->GetFPID() )
            continue;

        for( PAD* pad : footprint->Pads() )
        {
            // TODO(JE) padstacks
            if( aPadShapeFilter && pad->GetShape( PADSTACK::ALL_LAYERS ) != aSrcPad.GetShape( PADSTACK::ALL_LAYERS ) )
                continue;

            EDA_ANGLE padAngle = pad->GetOrientation() - footprint->GetOrientation();

            if( aPadOrientFilter && ( padAngle != srcPadAngle ) )
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

    if( selection.Size() == 1 && selection[0]->Type() == PCB_PAD_T )
    {
        PAD* srcPad = static_cast<PAD*>( selection[0] );

        if( FOOTPRINT* footprint = srcPad->GetParentFootprint() )
        {
            frame()->SetMsgPanel( footprint );

            DIALOG_PUSH_PAD_PROPERTIES dlg( frame() );
            int dialogRet = dlg.ShowModal();

            if( dialogRet == wxID_CANCEL )
                return 0;

            const bool edit_Same_Modules = (dialogRet == 1);

            BOARD_COMMIT commit( frame() );

            doPushPadProperties( *getModel<BOARD>(), *srcPad, commit, edit_Same_Modules, dlg.GetPadShapeFilter(),
                                 dlg.GetPadOrientFilter(), dlg.GetPadLayerFilter(), dlg.GetPadTypeFilter() );

            commit.Push( _( "Push Pad Settings" ) );

            m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
            frame()->Refresh();
        }
    }

    return 0;
}


/**
 * @brief Prompts the user for parameters for sequential pad numbering
 *
 * @param aFrame The parent window for the dialog
 * @return The parameters, or nullopt if no parameters, e.g. user cancelled the dialog
*/
static std::optional<SEQUENTIAL_PAD_ENUMERATION_PARAMS> GetSequentialPadNumberingParams( wxWindow* aFrame )
{
    // Persistent settings for the pad enumeration dialog.
    static SEQUENTIAL_PAD_ENUMERATION_PARAMS s_lastUsedParams;

    DIALOG_ENUM_PADS settingsDlg( aFrame, s_lastUsedParams );

    if( settingsDlg.ShowModal() != wxID_OK )
        return std::nullopt;

    return s_lastUsedParams;
}


int PAD_TOOL::EnumeratePads( const TOOL_EVENT& aEvent )
{
    if( !m_isFootprintEditor || InPadEditMode() )
        return 0;

    if( !board()->GetFirstFootprint() || board()->GetFirstFootprint()->Pads().empty() )
        return 0;

    GENERAL_COLLECTOR        collector;
    GENERAL_COLLECTORS_GUIDE guide = frame()->GetCollectorsGuide();
    guide.SetIgnoreFPTextOnBack( true );
    guide.SetIgnoreFPTextOnFront( true );
    guide.SetIgnoreFPValues( true );
    guide.SetIgnoreFPReferences( true );

    const std::optional<SEQUENTIAL_PAD_ENUMERATION_PARAMS> params = GetSequentialPadNumberingParams( frame() );

    // Cancelled or otherwise failed to get any useful parameters
    if( !params )
        return 0;

    int seqPadNum = params->m_start_number;

    std::deque<int>                              storedPadNumbers;
    std::map<wxString, std::pair<int, wxString>> oldNumbers;

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    frame()->PushTool( aEvent );

    VECTOR2I          oldMousePos;  // store the previous mouse cursor position, during mouse drag
    std::list<PAD*>   selectedPads;
    BOARD_COMMIT      commit( frame() );
    bool              isFirstPoint = true;   // make sure oldMousePos is initialized at least once
    std::deque<PAD*>  pads = board()->GetFirstFootprint()->Pads();
    MAGNETIC_SETTINGS mag_settings;

    mag_settings.graphics = false;
    mag_settings.tracks = MAGNETIC_OPTIONS::NO_EFFECT;
    mag_settings.pads = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;

    PCB_GRID_HELPER grid( m_toolMgr, &mag_settings );

    grid.SetSnap( true );
    grid.SetUseGrid( false );

    auto setCursor =
            [&]()
            {
                canvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
            };

    KIGFX::VIEW*         view = m_toolMgr->GetView();
    RENDER_SETTINGS*     settings = view->GetPainter()->GetSettings();
    const std::set<int>& activeLayers = settings->GetHighContrastLayers();
    bool                 isHighContrast = settings->GetHighContrast();

    auto checkVisibility =
            [&]( BOARD_ITEM* item )
            {
                if( !view->IsVisible( item ) )
                    return false;

                for( PCB_LAYER_ID layer : item->GetLayerSet() )
                {
                    if( ( isHighContrast && activeLayers.count( layer ) ) || view->IsLayerVisible( layer ) )
                    {
                        if( item->ViewGetLOD( layer, view ) < view->GetScale() )
                            return true;
                    }
                }

                return false;
            };

    for( PAD* pad : board()->GetFirstFootprint()->Pads() )
    {
        if( checkVisibility( pad ) )
            pads.push_back( pad );
    }

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    getViewControls()->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    STATUS_TEXT_POPUP statusPopup( frame() );

    // Callable lambda to construct the pad number string for the given value
    const auto constructPadNumber =
            [&]( int aValue )
            {
                return wxString::Format( wxT( "%s%d" ), params->m_prefix.value_or( "" ), aValue );
            };

    // Callable lambda to set the popup text for the given pad value
    const auto setPopupTextForValue =
            [&]( int aValue )
            {
                const wxString msg = _( "Click on pad %s\n"
                                        "Press <esc> to cancel all; double-click to finish" );
                statusPopup.SetText( wxString::Format( msg, constructPadNumber( aValue ) ) );
            };

    setPopupTextForValue( seqPadNum );
    statusPopup.Popup();
    statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
    canvas()->SetStatusPopup( statusPopup.GetPanel() );

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        VECTOR2I mousePos = getViewControls()->GetMousePosition();
        VECTOR2I cursorPos = grid.SnapToPad( mousePos, pads );
        getViewControls()->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );
            commit.Revert();

            frame()->PopTool( aEvent );
            break;
        }
        else if( evt->IsActivate() )
        {
            commit.Push( _( "Renumber Pads" ) );

            frame()->PopTool( aEvent );
            break;
        }
        else if( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            selectedPads.clear();

            // Be sure the old cursor mouse position was initialized:
            if( isFirstPoint )
            {
                oldMousePos = mousePos;
                isFirstPoint = false;
            }

            // wxWidgets deliver mouse move events not frequently enough, resulting in skipping
            // pads if the user moves cursor too fast. To solve it, create a line that approximates
            // the mouse move and search pads that are on the line.
            int distance = ( mousePos - oldMousePos ).EuclideanNorm();
            // Search will be made every 0.1 mm:
            int            segments = distance / int( 0.1 * pcbIUScale.IU_PER_MM ) + 1;
            const VECTOR2I line_step( ( mousePos - oldMousePos ) / segments );

            collector.Empty();

            for( int j = 0; j < segments; ++j )
            {
                VECTOR2I testpoint( mousePos.x - j * line_step.x, mousePos.y - j * line_step.y );
                collector.Collect( board(), { PCB_PAD_T }, testpoint, guide );

                for( int i = 0; i < collector.GetCount(); ++i )
                {
                    PAD* pad = static_cast<PAD*>( collector[i] );

                    if( !pad->IsAperturePad() && checkVisibility( pad ) )
                        selectedPads.push_back( pad );
                }
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
                    {
                        newval = seqPadNum;
                        seqPadNum += params->m_step;
                    }

                    const wxString newNumber = constructPadNumber( newval );
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

                    setPopupTextForValue( newval );
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

                        const int newval = storedPadNumbers.front();
                        setPopupTextForValue( newval );
                    }

                    pad->ClearSelected();
                    getView()->Update( pad );
                }
            }
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            commit.Push( _( "Renumber Pads" ) );
            frame()->PopTool( aEvent );
            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else
        {
            evt->SetPassEvent();
        }

        // Prepare the next loop by updating the old cursor mouse position
        // to this last mouse cursor position
        oldMousePos = mousePos;
        statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
    }

    for( PAD* p : board()->GetFirstFootprint()->Pads() )
    {
        p->ClearSelected();
        getView()->Update( p );
    }

    canvas()->SetStatusPopup( nullptr );
    statusPopup.Hide();

    canvas()->SetCurrentCursor( KICURSOR::ARROW );
    getViewControls()->ForceCursorPosition( false );
    return 0;
}


int PAD_TOOL::PlacePad( const TOOL_EVENT& aEvent )
{
    // When creating a new pad (in FP editor) we can use a new pad number or the last entered pad number
    // neednewPadNumber = true to create a new pad number, false to use the last entered pad number
    static bool neednewPadNumber;

    if( !m_isFootprintEditor )
        return 0;

    if( !board()->GetFirstFootprint() )
        return 0;

    struct PAD_PLACER : public INTERACTIVE_PLACER_BASE
    {
        PAD_PLACER( PAD_TOOL* aPadTool, PCB_BASE_EDIT_FRAME* aFrame ) :
                m_padTool( aPadTool ),
                m_frame( aFrame ),
                m_gridHelper( aPadTool->GetManager(), aFrame->GetMagneticItemsSettings() )
        {
            neednewPadNumber = true;    // Use a new pad number when creatin a pad by default
        }

        virtual ~PAD_PLACER() = default;

        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            // TODO(JE) padstacks
            PAD* pad = new PAD( m_board->GetFirstFootprint() );
            PAD* master = m_frame->GetDesignSettings().m_Pad_Master.get();

            pad->ImportSettingsFrom( *master );

            if( pad->CanHaveNumber() )
            {
                wxString padNumber = m_padTool->GetLastPadNumber();

                // Use the last entered pad number when recreating a pad without using the
                // previously created pad, and a new number when creating a really new pad
                if( neednewPadNumber )
                    padNumber = m_board->GetFirstFootprint()->GetNextPadNumber( padNumber );

                pad->SetNumber( padNumber );
                m_padTool->SetLastPadNumber( padNumber );

                // If a pad is recreated and the previously created was not placed, use
                // the last entered pad number
                neednewPadNumber = false;
            }

            return std::unique_ptr<BOARD_ITEM>( pad );
        }

        bool PlaceItem( BOARD_ITEM *aItem, BOARD_COMMIT& aCommit ) override
        {
            PAD* pad = dynamic_cast<PAD*>( aItem );
            // We are using this pad number.
            // therefore use a new pad number for a newly created pad
            neednewPadNumber = true;

            if( pad )
            {
                m_frame->GetDesignSettings().m_Pad_Master->ImportSettingsFrom( *pad );
                aCommit.Add( aItem );
                return true;
            }

            return false;
        }

        void SnapItem( BOARD_ITEM *aItem ) override
        {
            m_gridHelper.SetSnap( !( m_modifiers & MD_SHIFT ) );
            m_gridHelper.SetUseGrid( !( m_modifiers & MD_CTRL ) );

            if( !m_gridHelper.GetSnap() )
                return;

            MAGNETIC_SETTINGS*       settings = m_frame->GetMagneticItemsSettings();
            PAD*                     pad = static_cast<PAD*>( aItem );
            VECTOR2I                 position = m_padTool->getViewControls()->GetMousePosition();
            KIGFX::VIEW_CONTROLS*    viewControls = m_padTool->getViewControls();
            std::vector<BOARD_ITEM*> ignored_items( 1, pad );

            if( settings->pads == MAGNETIC_OPTIONS::NO_EFFECT )
            {
                PADS& pads = m_board->GetFirstFootprint()->Pads();
                ignored_items.insert( ignored_items.end(), pads.begin(), pads.end() );
            }

            if( !settings->graphics )
            {
                DRAWINGS& graphics = m_board->GetFirstFootprint()->GraphicalItems();
                ignored_items.insert( ignored_items.end(), graphics.begin(), graphics.end() );
            }

            VECTOR2I cursorPos = m_gridHelper.BestSnapAnchor( position, LSET::AllLayersMask(), GRID_CURRENT,
                                                              ignored_items );
            viewControls->ForceCursorPosition( true, cursorPos );
            aItem->SetPosition( cursorPos );
        }

        PAD_TOOL*            m_padTool;
        PCB_BASE_EDIT_FRAME* m_frame;
        PCB_GRID_HELPER      m_gridHelper;
    };

    PAD_PLACER placer( this, frame() );

    doInteractiveItemPlacement( aEvent, &placer, _( "Place pad" ),
                                IPO_REPEAT | IPO_SINGLE_CLICK | IPO_ROTATE | IPO_FLIP );

    return 0;
}


int PAD_TOOL::EditPad( const TOOL_EVENT& aEvent )
{
    if( !m_isFootprintEditor )
        return 0;

    Activate();

    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( view()->GetPainter() );
    PCB_RENDER_SETTINGS* settings = painter->GetSettings();
    PCB_SELECTION&       selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_editPad != niluuid )
    {
        if( PAD* pad = dynamic_cast<PAD*>( frame()->ResolveItem( m_editPad ) ) )
        {
            BOARD_COMMIT commit( frame() );
            commit.Modify( pad );

            std::vector<PCB_SHAPE*> mergedShapes = RecombinePad( pad, false );

            for( PCB_SHAPE* shape : mergedShapes )
                commit.Remove( shape );

            commit.Push( _( "Edit Pad" ) );
        }

        m_editPad = niluuid;
    }
    else if( selection.Size() == 1 && selection[0]->Type() == PCB_PAD_T )
    {
        PCB_LAYER_ID layer;
        PAD*         pad = static_cast<PAD*>( selection[0] );
        BOARD_COMMIT commit( frame() );

        commit.Modify( pad );
        explodePad( pad, &layer, commit );
        commit.Push( _( "Edit Pad" ) );

        m_toolMgr->RunAction( ACTIONS::selectionClear );
        frame()->SetActiveLayer( layer );

        settings->m_PadEditModePad = pad;
        enterPadEditMode();
    }

    if( m_editPad == niluuid )
        ExitPadEditMode();

    return 0;
}


int PAD_TOOL::OnUndoRedo( const TOOL_EVENT& aEvent )
{
    PAD* flaggedPad = nullptr;
    KIID flaggedPadId = niluuid;

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->IsEntered() )
            {
                flaggedPad = pad;
                flaggedPadId = pad->m_Uuid;
                break;
            }
        }
    }

    if( flaggedPadId != m_editPad )
    {
        KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( view()->GetPainter() );
        PCB_RENDER_SETTINGS* settings = painter->GetSettings();

        m_editPad = flaggedPadId;
        settings->m_PadEditModePad = flaggedPad;

        if( flaggedPad )
            enterPadEditMode();
        else
            ExitPadEditMode();
    }

    return 0;
}


void PAD_TOOL::enterPadEditMode()
{
    PCB_DISPLAY_OPTIONS opts = frame()->GetDisplayOptions();
    WX_INFOBAR*         infoBar = frame()->GetInfoBar();
    wxString            msg;

    canvas()->GetView()->UpdateAllItemsConditionally( KIGFX::REPAINT,
            [&]( KIGFX::VIEW_ITEM* aItem ) -> bool
            {
                return dynamic_cast<PAD*>( aItem ) != nullptr;
            } );

    m_previousHighContrastMode = opts.m_ContrastModeDisplay;

    if( opts.m_ContrastModeDisplay == HIGH_CONTRAST_MODE::NORMAL )
    {
        opts.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::DIMMED;
        frame()->SetDisplayOptions( opts );
    }

    if( PCB_ACTIONS::explodePad.GetHotKey() == PCB_ACTIONS::recombinePad.GetHotKey() )
    {
        msg.Printf( _( "Pad Edit Mode.  Press %s again to exit." ),
                    KeyNameFromKeyCode( PCB_ACTIONS::recombinePad.GetHotKey() ) );
    }
    else
    {
        msg.Printf( _( "Pad Edit Mode.  Press %s to exit." ),
                    KeyNameFromKeyCode( PCB_ACTIONS::recombinePad.GetHotKey() ) );
    }

    infoBar->RemoveAllButtons();
    infoBar->ShowMessage( msg, wxICON_INFORMATION );
}


void PAD_TOOL::ExitPadEditMode()
{
    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( view()->GetPainter() );
    PCB_RENDER_SETTINGS* settings = painter->GetSettings();
    PCB_DISPLAY_OPTIONS  opts = frame()->GetDisplayOptions();

    settings->m_PadEditModePad = nullptr;

    if( m_previousHighContrastMode != opts.m_ContrastModeDisplay )
    {
        opts.m_ContrastModeDisplay = m_previousHighContrastMode;
        frame()->SetDisplayOptions( opts );
    }

    // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible as
    // they won't be found in the view layer's itemset for re-painting.
    canvas()->GetView()->UpdateAllItemsConditionally( KIGFX::ALL,
            [&]( KIGFX::VIEW_ITEM* aItem ) -> bool
            {
                return dynamic_cast<PAD*>( aItem ) != nullptr;
            } );

    // Refresh now (otherwise there's an uncomfortably long pause while the infoBar
    // closes before refresh).
    canvas()->ForceRefresh();

    frame()->GetInfoBar()->Dismiss();
}


void PAD_TOOL::explodePad( PAD* aPad, PCB_LAYER_ID* aLayer, BOARD_COMMIT& aCommit )
{
    if( aPad->IsOnLayer( F_Cu ) )
        *aLayer = F_Cu;
    else if( aPad->IsOnLayer( B_Cu ) )
        *aLayer = B_Cu;
    else
        *aLayer = aPad->GetLayerSet().UIOrder().front();

    // TODO(JE) padstacks
    if( aPad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CUSTOM )
    {
        for( const std::shared_ptr<PCB_SHAPE>& primitive : aPad->GetPrimitives( PADSTACK::ALL_LAYERS ) )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( primitive->Duplicate( true, &aCommit ) );

            shape->SetParent( board()->GetFirstFootprint() );
            shape->Rotate( VECTOR2I( 0, 0 ), aPad->GetOrientation() );
            shape->Move( aPad->ShapePos( PADSTACK::ALL_LAYERS ) );
            shape->SetLayer( *aLayer );

            if( shape->IsProxyItem() && shape->GetShape() == SHAPE_T::SEGMENT )
            {
                if( aPad->GetLocalThermalSpokeWidthOverride().has_value() )
                    shape->SetWidth( aPad->GetLocalThermalSpokeWidthOverride().value() );
                else
                    shape->SetWidth( pcbIUScale.mmToIU( ZONE_THERMAL_RELIEF_COPPER_WIDTH_MM ) );
            }

            aCommit.Add( shape );
        }

        // TODO(JE) padstacks
        aPad->SetShape( PADSTACK::ALL_LAYERS, aPad->GetAnchorPadShape( PADSTACK::ALL_LAYERS ) );
        aPad->DeletePrimitivesList();
    }

    aPad->SetFlags( ENTERED );
    m_editPad = aPad->m_Uuid;
}


std::vector<PCB_SHAPE*> PAD_TOOL::RecombinePad( PAD* aPad, bool aIsDryRun )
{
    int maxError = board()->GetDesignSettings().m_MaxError;

    // Don't leave an object in the point editor that might no longer exist after recombining.
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    return aPad->Recombine( aIsDryRun, maxError );
}

int PAD_TOOL::PadTable( const TOOL_EVENT& aEvent )
{
    if( !m_isFootprintEditor || InPadEditMode() )
        return 0;

    FOOTPRINT* footprint = board()->GetFirstFootprint();

    if( !footprint )
        return 0;

    DIALOG_FP_EDIT_PAD_TABLE dlg( frame(), footprint );
    dlg.ShowQuasiModal();

    return 0;
}


void PAD_TOOL::setTransitions()
{
    Go( &PAD_TOOL::pastePadProperties,      PCB_ACTIONS::applyPadSettings.MakeEvent() );
    Go( &PAD_TOOL::copyPadSettings,         PCB_ACTIONS::copyPadSettings.MakeEvent() );
    Go( &PAD_TOOL::pushPadSettings,         PCB_ACTIONS::pushPadSettings.MakeEvent() );

    Go( &PAD_TOOL::PlacePad,                PCB_ACTIONS::placePad.MakeEvent() );
    Go( &PAD_TOOL::EnumeratePads,           PCB_ACTIONS::enumeratePads.MakeEvent() );
    Go( &PAD_TOOL::PadTable,                PCB_ACTIONS::padTable.MakeEvent() );

    Go( &PAD_TOOL::EditPad,                 PCB_ACTIONS::explodePad.MakeEvent() );
    Go( &PAD_TOOL::EditPad,                 PCB_ACTIONS::recombinePad.MakeEvent() );

    Go( &PAD_TOOL::OnUndoRedo,              EVENTS::UndoRedoPostEvent );
}
