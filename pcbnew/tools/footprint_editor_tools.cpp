/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "footprint_editor_tools.h"
#include "kicad_clipboard.h"
#include "selection_tool.h"
#include "pcb_actions.h"
#include <core/optional.h>
#include <tool/tool_manager.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <pcb_painter.h>
#include <origin_viewitem.h>
#include <status_popup.h>
#include <footprint_edit_frame.h>
#include <kicad_plugin.h>
#include <pcbnew_id.h>
#include <collectors.h>
#include <confirm.h>
#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <board_commit.h>
#include <project.h>
#include <tools/tool_event_utils.h>
#include <fp_lib_table.h>
#include <functional>
using namespace std::placeholders;
#include <wx/defs.h>


MODULE_EDITOR_TOOLS::MODULE_EDITOR_TOOLS() :
    PCB_TOOL_BASE( "pcbnew.ModuleEditor" ),
    m_frame( nullptr )
{
}


MODULE_EDITOR_TOOLS::~MODULE_EDITOR_TOOLS()
{
}


void MODULE_EDITOR_TOOLS::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<FOOTPRINT_EDIT_FRAME>();
}


bool MODULE_EDITOR_TOOLS::Init()
{
    // Build a context menu for the footprint tree
    //
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    auto libSelectedCondition = [ this ] ( const SELECTION& aSel ) {
        LIB_ID sel = m_frame->GetTreeFPID();
        return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
    };
    auto fpSelectedCondition = [ this ] ( const SELECTION& aSel ) {
        LIB_ID sel = m_frame->GetTreeFPID();
        return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
    };

    ctxMenu.AddItem( ACTIONS::newLibrary,            SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::addLibrary,            SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::save,                  libSelectedCondition );
    ctxMenu.AddItem( ACTIONS::saveAs,                libSelectedCondition );
    ctxMenu.AddItem( ACTIONS::revert,                libSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::newFootprint,      SELECTION_CONDITIONS::ShowAlways );
#ifdef KICAD_SCRIPTING
    ctxMenu.AddItem( PCB_ACTIONS::createFootprint,   SELECTION_CONDITIONS::ShowAlways );
#endif
    ctxMenu.AddItem( PCB_ACTIONS::editFootprint,     fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( ACTIONS::save,                  fpSelectedCondition );
    ctxMenu.AddItem( ACTIONS::saveCopyAs,            fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::deleteFootprint,   fpSelectedCondition );
    ctxMenu.AddItem( ACTIONS::revert,                fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::cutFootprint,      fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::copyFootprint,     fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::pasteFootprint,    SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::importFootprint,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::exportFootprint,   fpSelectedCondition );

    return true;
}


int MODULE_EDITOR_TOOLS::NewFootprint( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_NEW_MODULE );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int MODULE_EDITOR_TOOLS::CreateFootprint( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_NEW_MODULE_FROM_WIZARD );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int MODULE_EDITOR_TOOLS::Save( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_SAVE );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int MODULE_EDITOR_TOOLS::SaveAs( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_SAVE_AS );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int MODULE_EDITOR_TOOLS::Revert( const TOOL_EVENT& aEvent )
{
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->RevertFootprint();
    return 0;
}


int MODULE_EDITOR_TOOLS::CutCopyFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID fpID = m_frame->GetTreeFPID();

    if( fpID == m_frame->GetLoadedFPID() )
        m_copiedModule.reset( new MODULE( *m_frame->GetBoard()->GetFirstModule() ) );
    else
        m_copiedModule.reset( m_frame->LoadFootprint( fpID ) );

    if( aEvent.IsAction( &PCB_ACTIONS::cutFootprint ) )
        DeleteFootprint(aEvent );

    return 0;
}


int MODULE_EDITOR_TOOLS::PasteFootprint( const TOOL_EVENT& aEvent )
{
    if( m_copiedModule && !m_frame->GetTreeFPID().GetLibNickname().empty() )
    {
        wxString newLib = m_frame->GetTreeFPID().GetLibNickname();
        MODULE*  newModule( m_copiedModule.get() );
        wxString newName = newModule->GetFPID().GetLibItemName();

        while( m_frame->Prj().PcbFootprintLibs()->FootprintExists( newLib, newName ) )
            newName += _( "_copy" );

        newModule->SetFPID( LIB_ID( newLib, newName ) );
        m_frame->SaveFootprintInLibrary( newModule, newLib );

        m_frame->SyncLibraryTree( true );
        m_frame->FocusOnLibID( newModule->GetFPID() );
    }

    return 0;
}


int MODULE_EDITOR_TOOLS::DeleteFootprint( const TOOL_EVENT& aEvent )
{
    FOOTPRINT_EDIT_FRAME* frame = getEditFrame<FOOTPRINT_EDIT_FRAME>();

    if( frame->DeleteModuleFromLibrary( frame->GetTargetFPID(), true ) )
    {
        if( frame->GetTargetFPID() == frame->GetLoadedFPID() )
            frame->Clear_Pcb( false );

        frame->SyncLibraryTree( true );
    }

    return 0;
}


int MODULE_EDITOR_TOOLS::ImportFootprint( const TOOL_EVENT& aEvent )
{
    if( !m_frame->Clear_Pcb( true ) )
        return -1;                  // this command is aborted

    getViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    m_frame->Import_Module();

    if( m_frame->GetBoard()->GetFirstModule() )
        m_frame->GetBoard()->GetFirstModule()->ClearFlags();

    // Clear undo and redo lists because we don't have handling to in
    // FP editor to undo across imports (the module _is_ the board with the stack)
    // todo: Abstract undo/redo stack to a higher element or keep consistent board item in fpeditor
    frame()->GetScreen()->ClearUndoRedoList();

    m_toolMgr->RunAction( ACTIONS::zoomFitScreen, true );
    m_frame->OnModify();
    return 0;
}


int MODULE_EDITOR_TOOLS::ExportFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID  fpID = m_frame->GetTreeFPID();
    MODULE* fp;

    if( !fpID.IsValid() )
        fp = m_frame->GetBoard()->GetFirstModule();
    else
        fp = m_frame->LoadFootprint( fpID );

    m_frame->Export_Module( fp );
    return 0;
}


int MODULE_EDITOR_TOOLS::EditFootprint( const TOOL_EVENT& aEvent )
{
    m_frame->LoadModuleFromLibrary( m_frame->GetTreeFPID() );
    return 0;
}


int MODULE_EDITOR_TOOLS::ToggleFootprintTree( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleSearchTree();
    return 0;
}


int MODULE_EDITOR_TOOLS::Properties( const TOOL_EVENT& aEvent )
{
    MODULE* footprint = m_frame->GetBoard()->GetFirstModule();

    if( footprint )
    {
        getEditFrame<FOOTPRINT_EDIT_FRAME>()->OnEditItemRequest( footprint );
        m_frame->GetCanvas()->Refresh();
    }
    return 0;
}


int MODULE_EDITOR_TOOLS::DefaultPadProperties( const TOOL_EVENT& aEvent )
{
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->InstallPadOptionsFrame( nullptr );
    return 0;
}


int MODULE_EDITOR_TOOLS::PlacePad( const TOOL_EVENT& aEvent )
{
    if( !m_frame->GetBoard()->GetFirstModule() )
        return 0;

    struct PAD_PLACER : public INTERACTIVE_PLACER_BASE
    {
        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            D_PAD* pad = new D_PAD( m_board->GetFirstModule() );
            pad->ImportSettingsFrom( m_frame->GetDesignSettings().m_Pad_Master );
            pad->IncrementPadName( true, true );
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
    };

    PAD_PLACER placer;

    doInteractiveItemPlacement( aEvent.GetCommandStr().get(), &placer,  _( "Place pad" ),
                                IPO_REPEAT | IPO_SINGLE_CLICK | IPO_ROTATE | IPO_FLIP );

    return 0;
}


int MODULE_EDITOR_TOOLS::ExplodePadToShapes( const TOOL_EVENT& aEvent )
{
    PCBNEW_SELECTION& selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();
    BOARD_COMMIT      commit( m_frame );

    if( selection.Size() != 1 )
        return 0;

    if( selection[0]->Type() != PCB_PAD_T )
        return 0;

    auto pad = static_cast<D_PAD*>( selection[0] );

    if( pad->GetShape() != PAD_SHAPE_CUSTOM )
        return 0;

    commit.Modify( pad );

    wxPoint anchor = pad->GetPosition();

    for( auto prim : pad->GetPrimitives() )
    {
        auto ds = new EDGE_MODULE( board()->GetFirstModule() );

        prim.ExportTo( ds );    // ExportTo exports to a DRAWSEGMENT
        // Fix an arbitray draw layer for this EDGE_MODULE
        ds->SetLayer( Dwgs_User ); //pad->GetLayer() );
        ds->Move( anchor );
        ds->Rotate( anchor, pad->GetOrientation() );

        commit.Add( ds );
    }

    pad->SetShape( pad->GetAnchorPadShape() );
    // Cleanup the pad primitives data, because the initial pad was a custom
    // shaped pad, and it contains primitives, that does not exist in non custom pads,
    // and can create issues later:
    if( pad->GetShape() != PAD_SHAPE_CUSTOM )   // should be always the case
    {
        pad->DeletePrimitivesList();
    }

    commit.Push( _("Explode pad to shapes") );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    return 0;
}


int MODULE_EDITOR_TOOLS::CreatePadFromShapes( const TOOL_EVENT& aEvent )
{
    PCBNEW_SELECTION& selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();

    std::unique_ptr<D_PAD> pad( new D_PAD( board()->GetFirstModule() ) );
    D_PAD *refPad = nullptr;
    bool multipleRefPadsFound = false;
    bool illegalItemsFound = false;

    std::vector<PAD_CS_PRIMITIVE> shapes;

    BOARD_COMMIT commit( m_frame );

    for( auto item : selection )
    {
        switch( item->Type() )
        {
            case PCB_PAD_T:
            {
                if( refPad )
                    multipleRefPadsFound = true;

                refPad = static_cast<D_PAD*>( item );
                break;
            }

            case PCB_MODULE_EDGE_T:
            {
                auto em = static_cast<EDGE_MODULE*> ( item );

                PAD_CS_PRIMITIVE shape( em->GetShape() );
                shape.m_Start = em->GetStart();
                shape.m_End = em->GetEnd();
                shape.m_Radius = em->GetRadius();
                shape.m_Thickness = em->GetWidth();
                shape.m_ArcAngle = em->GetAngle();
                shape.m_Ctrl1 = em->GetBezControl1();
                shape.m_Ctrl2 = em->GetBezControl2();
                shape.m_Poly = em->BuildPolyPointsList();

                shapes.push_back(shape);

                break;
            }

            default:
            {
                illegalItemsFound = true;
                break;
            }
        }
    }

    if( refPad && selection.Size() == 1 )
    {
        // don't convert a pad into itself...
        return 0;
    }

    if( multipleRefPadsFound )
    {
        DisplayErrorMessage( m_frame, _(  "Cannot convert items to a custom-shaped pad:\n"
                                          "selection contains more than one reference pad." ) );
        return 0;
    }

    if( illegalItemsFound )
    {
        DisplayErrorMessage( m_frame, _( "Cannot convert items to a custom-shaped pad:\n"
                                         "selection contains unsupported items.\n"
                                         "Only graphical lines, circles, arcs and polygons "
                                         "are allowed." ) );
        return 0;
    }

    double refOrientation = 0.0;

    if( refPad )
    {
        pad.reset( static_cast<D_PAD*>( refPad->Clone() ) );

        if( refPad->GetShape() == PAD_SHAPE_RECT )
            pad->SetAnchorPadShape( PAD_SHAPE_RECT );

        // ignore the pad offset for the moment. Makes more trouble than it's worth.
        pad->SetOffset( wxPoint( 0, 0 ) );
        refOrientation = pad->GetOrientation();
        pad->SetOrientation( 0.0 );
    }
    else
    {
        // Create a default pad anchor:
        pad->SetAnchorPadShape( PAD_SHAPE_CIRCLE );
        pad->SetAttribute( PAD_ATTRIB_SMD );
        pad->SetLayerSet( D_PAD::SMDMask() );
        int radius = Millimeter2iu( 0.2 );
        pad->SetSize ( wxSize( radius, radius ) );
        pad->IncrementPadName( true, true );
        pad->SetOrientation( 0 );
    }

    pad->SetShape ( PAD_SHAPE_CUSTOM );

    OPT<VECTOR2I> anchor;
    VECTOR2I tmp;

    if( refPad )
    {
        anchor = VECTOR2I( pad->GetPosition() );
    }
    else if( pad->GetBestAnchorPosition( tmp ) )
    {
        anchor = tmp;
    }

    if( !anchor )
    {
        DisplayErrorMessage( m_frame, _( "Cannot convert items to a custom-shaped pad:\n"
                                         "unable to determine the anchor point position.\n"
                                         "Consider adding a small anchor pad to the selection "
                                         "and try again.") );
        return 0;
    }


    // relocate the shapes, they are relative to the anchor pad position
    for( auto& shape : shapes )
    {
        shape.Move( wxPoint( -anchor->x, -anchor->y ) );
        shape.Rotate( wxPoint( 0, 0 ), -refOrientation );
    }

    pad->SetPosition( wxPoint( anchor->x, anchor->y ) );
    pad->AddPrimitives( shapes );
    pad->ClearFlags();

    bool result = pad->MergePrimitivesAsPolygon();
    pad->Rotate( wxPoint( anchor->x, anchor->y ), refOrientation );

    if( !result )
    {
        DisplayErrorMessage( m_frame, _( "Cannot convert items to a custom-shaped pad:\n"
                                         "selected items do not form a single solid shape.") );
        return 0;
    }

    auto padPtr = pad.release();

    commit.Add( padPtr );
    for ( auto item : selection )
    {
        commit.Remove( item );
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    commit.Push(_("Create Pad from Selected Shapes") );
    m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, padPtr );

    return 0;
}

void MODULE_EDITOR_TOOLS::setTransitions()
{
    Go( &MODULE_EDITOR_TOOLS::NewFootprint,         PCB_ACTIONS::newFootprint.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::CreateFootprint,      PCB_ACTIONS::createFootprint.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::Save,                 ACTIONS::save.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::Save,                 PCB_ACTIONS::saveToBoard.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::Save,                 PCB_ACTIONS::saveToLibrary.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::SaveAs,               ACTIONS::saveAs.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::SaveAs,               ACTIONS::saveCopyAs.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::Revert,               ACTIONS::revert.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::DeleteFootprint,      PCB_ACTIONS::deleteFootprint.MakeEvent() );

    Go( &MODULE_EDITOR_TOOLS::EditFootprint,        PCB_ACTIONS::editFootprint.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::CutCopyFootprint,     PCB_ACTIONS::cutFootprint.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::CutCopyFootprint,     PCB_ACTIONS::copyFootprint.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::PasteFootprint,       PCB_ACTIONS::pasteFootprint.MakeEvent() );

    Go( &MODULE_EDITOR_TOOLS::ImportFootprint,      PCB_ACTIONS::importFootprint.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::ExportFootprint,      PCB_ACTIONS::exportFootprint.MakeEvent() );

    Go( &MODULE_EDITOR_TOOLS::ToggleFootprintTree,  PCB_ACTIONS::toggleFootprintTree.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::Properties,           PCB_ACTIONS::footprintProperties.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::DefaultPadProperties, PCB_ACTIONS::defaultPadProperties.MakeEvent() );

    Go( &MODULE_EDITOR_TOOLS::PlacePad,             PCB_ACTIONS::placePad.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::CreatePadFromShapes,  PCB_ACTIONS::createPadFromShapes.MakeEvent() );
    Go( &MODULE_EDITOR_TOOLS::ExplodePadToShapes,   PCB_ACTIONS::explodePadToShapes.MakeEvent() );
}
