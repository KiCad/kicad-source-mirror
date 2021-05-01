/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/hyperlink.h>
#include <base_screen.h>
#include <class_library.h>
#include <confirm.h>
#include <core/kicad_algo.h>
#include <eeschema_id.h>
#include <eeschema_settings.h>
#include <env_paths.h>
#include <kiface_i.h>
#include <kiplatform/app.h>
#include <kiway_express.h>
#include <symbol_edit_frame.h>
#include <symbol_library_manager.h>
#include <lib_text.h>
#include <symbol_editor_settings.h>
#include <paths.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <sch_view.h>
#include <settings/settings_manager.h>
#include <symbol_lib_table.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/editor_conditions.h>
#include <tool/picker_tool.h>
#include <tool/selection.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <tools/ee_point_editor.h>
#include <tools/ee_selection_tool.h>
#include <tools/symbol_editor_control.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <tools/symbol_editor_edit_tool.h>
#include <tools/symbol_editor_move_tool.h>
#include <tools/symbol_editor_pin_tool.h>
#include <widgets/app_progress_dialog.h>
#include <widgets/infobar.h>
#include <widgets/lib_tree.h>
#include <widgets/progress_reporter.h>
#include <widgets/symbol_tree_pane.h>
#include <wildcards_and_files_ext.h>
#include <panel_sym_lib_table.h>
#include <wx/choicdlg.h>


bool SYMBOL_EDIT_FRAME::m_showDeMorgan = false;


BEGIN_EVENT_TABLE( SYMBOL_EDIT_FRAME, EDA_DRAW_FRAME )
    EVT_SIZE( SYMBOL_EDIT_FRAME::OnSize )

    EVT_COMBOBOX( ID_LIBEDIT_SELECT_PART_NUMBER, SYMBOL_EDIT_FRAME::OnSelectUnit )

    // menubar commands
    EVT_MENU( wxID_EXIT, SYMBOL_EDIT_FRAME::OnExitKiCad )
    EVT_MENU( wxID_CLOSE, SYMBOL_EDIT_FRAME::CloseWindow )
    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )

    // Update user interface elements.
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_PART_NUMBER, SYMBOL_EDIT_FRAME::OnUpdatePartNumber )

END_EVENT_TABLE()


SYMBOL_EDIT_FRAME::SYMBOL_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH_SYMBOL_EDITOR, _( "Library Editor" ),
                        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE,
                        LIB_EDIT_FRAME_NAME ),
        m_unitSelectBox( nullptr ),
        m_isSymbolFromSchematic( false )
{
    SetShowDeMorgan( false );
    m_SyncPinEdit = false;

    m_my_part = nullptr;
    m_treePane = nullptr;
    m_libMgr = nullptr;
    m_unit = 1;
    m_convert = 1;
    m_aboutTitle = _( "KiCad Symbol Editor" );

    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    m_settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    LoadSettings( m_settings );

    // Ensure axis are always drawn
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = GetGalDisplayOptions();
    gal_opts.m_axesEnabled = true;

    m_dummyScreen = new SCH_SCREEN();
    SetScreen( m_dummyScreen );
    GetScreen()->m_Center = true;

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );

    GetRenderSettings()->LoadColors( GetColorSettings() );
    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );

    setupTools();
    setupUIConditions();

    m_libMgr = new SYMBOL_LIBRARY_MANAGER( *this );

    // Preload libraries before using SyncLibraries the first time, as the preload is threaded
    WX_PROGRESS_REPORTER reporter( this, _( "Loading Symbol Libraries" ),
                                   m_libMgr->GetLibraryCount(), true );
    m_libMgr->Preload( reporter );

    SyncLibraries( false );
    m_treePane = new SYMBOL_TREE_PANE( this, m_libMgr );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    updateTitle();
    DisplaySymbolDatasheet();
    RebuildSymbolUnitsList();

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" )
                      .Left().Layer( 3 ) );
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "ComponentTree" )
                      .Left().Layer( 2 )
                      .Caption( _( "Libraries" ) )
                      .MinSize( 250, -1 ).BestSize( 250, -1 ) );
    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" )
                      .Right().Layer( 2 ) );

    m_auimgr.AddPane( GetCanvas(), wxAuiPaneInfo().Name( "DrawFrame" )
                      .CentrePane() );

    FinishAUIInitialization();

    if( m_settings->m_LibWidth > 0 )
    {
        wxAuiPaneInfo& treePane = m_auimgr.GetPane( "ComponentTree" );

        // wxAUI hack: force width by setting MinSize() and then Fixed()
        // thanks to ZenJu http://trac.wxwidgets.org/ticket/13180
        treePane.MinSize( m_settings->m_LibWidth, -1 );
        treePane.Fixed();
        m_auimgr.Update();

        // now make it resizable again
        treePane.Resizable();
        m_auimgr.Update();

        // Note: DO NOT call m_auimgr.Update() anywhere after this; it will nuke the size
        // back to minimum.
        treePane.MinSize( 250, -1 );
    }

    Raise();
    Show( true );

    SyncView();
    GetCanvas()->GetView()->UseDrawPriority( true );
    GetCanvas()->GetGAL()->SetAxesEnabled( true );

    setupUnits( m_settings );

    // Set the working/draw area size to display a symbol to a reasonable value:
    // A 600mm x 600mm with a origin at the area center looks like a large working area
    double max_size_x = Millimeter2iu( 600 );
    double max_size_y = Millimeter2iu( 600 );
    BOX2D bbox;
    bbox.SetOrigin( -max_size_x /2, -max_size_y/2 );
    bbox.SetSize( max_size_x, max_size_y );
    GetCanvas()->GetView()->SetBoundary( bbox );

    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );

    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "Library changes are unsaved" ) );

    // Ensure the window is on top
    Raise();
}


SYMBOL_EDIT_FRAME::~SYMBOL_EDIT_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    if( IsSymbolFromSchematic() )
    {
        delete m_my_part;
        m_my_part = nullptr;

        SCH_SCREEN* screen = GetScreen();
        delete screen;
        m_isSymbolFromSchematic = false;
    }
    // current screen is destroyed in EDA_DRAW_FRAME
    SetScreen( m_dummyScreen );

    auto libedit = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    Pgm().GetSettingsManager().Save( libedit );

    delete m_libMgr;
}


void SYMBOL_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( m_settings, "Call to SYMBOL_EDIT_FRAME::LoadSettings with null m_settings" );

    SCH_BASE_FRAME::LoadSettings( GetSettings() );

    GetRenderSettings()->m_ShowPinsElectricalType = m_settings->m_ShowPinElectricalType;

    // Hidden elements must be editable
    GetRenderSettings()->m_ShowHiddenText = true;
    GetRenderSettings()->m_ShowHiddenPins = true;
    GetRenderSettings()->m_ShowUmbilicals = false;
}


void SYMBOL_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( m_settings, "Call to SYMBOL_EDIT_FRAME::LoadSettings with null m_settings" );

    GetGalDisplayOptions().m_axesEnabled = true;

    SCH_BASE_FRAME::SaveSettings( GetSettings() );

    m_settings->m_ShowPinElectricalType  = GetRenderSettings()->m_ShowPinsElectricalType;
    m_settings->m_LibWidth               = m_treePane->GetSize().x;
}


APP_SETTINGS_BASE* SYMBOL_EDIT_FRAME::config() const
{
    return static_cast<APP_SETTINGS_BASE*>( GetSettings() );
}


COLOR_SETTINGS* SYMBOL_EDIT_FRAME::GetColorSettings() const
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    if( GetSettings()->m_UseEeschemaColorSettings )
        return mgr.GetColorSettings( mgr.GetAppSettings<EESCHEMA_SETTINGS>()->m_ColorTheme );
    else
        return mgr.GetColorSettings( GetSettings()->m_ColorTheme );
}


void SYMBOL_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), GetSettings(), this );
    m_actions = new EE_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EE_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PICKER_TOOL );
    m_toolManager->RegisterTool( new EE_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_PIN_TOOL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_DRAWING_TOOLS );
    m_toolManager->RegisterTool( new EE_POINT_EDITOR );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_MOVE_TOOL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_EDIT_TOOL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_CONTROL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "eeschema.InteractiveSelection" );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void SYMBOL_EDIT_FRAME::setupUIConditions()
{
    SCH_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    auto haveSymbolCond =
        [this] ( const SELECTION& )
        {
            return m_my_part;
        };

    auto isEditableCond =
        [this] ( const SELECTION& )
        {
            // Only root symbols from the new s-expression libraries or the schematic
            // are editable.
            return IsSymbolEditable() && !IsSymbolAlias();
        };

    auto libModifiedCondition =
        [this] ( const SELECTION& sel )
        {
            return m_libMgr->HasModifications();
        };

    auto libSelectedCondition =
        [this] ( const SELECTION& sel )
        {
            return !GetTargetLibId().GetLibNickname().empty();
        };

    auto canEditProperties =
        [this] ( const SELECTION& sel )
        {
            return m_my_part && ( !IsSymbolFromLegacyLibrary() || IsSymbolFromSchematic() );
        };

    auto saveSymbolAsCondition =
        [ this ]( const SELECTION& aSel )
        {
            LIB_ID sel = GetTargetLibId();
            return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
        };

    mgr->SetConditions( ACTIONS::saveAll,           ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::save,              ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::saveLibraryAs,  ENABLE( libSelectedCondition ) );
    mgr->SetConditions( EE_ACTIONS::saveSymbolAs,   ENABLE( saveSymbolAsCondition ) );
    mgr->SetConditions( EE_ACTIONS::newSymbol,      ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::importSymbol,   ENABLE( SELECTION_CONDITIONS::ShowAlways ) );

    mgr->SetConditions( ACTIONS::undo,              ENABLE( haveSymbolCond && cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,              ENABLE( haveSymbolCond && cond.RedoAvailable() ) );
    mgr->SetConditions( ACTIONS::revert,            ENABLE( haveSymbolCond && libModifiedCondition ) );

    mgr->SetConditions( ACTIONS::toggleGrid,        CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle, CHECK( cond.FullscreenCursor() ) );
    mgr->SetConditions( ACTIONS::millimetersUnits,  CHECK( cond.Units( EDA_UNITS::MILLIMETRES ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,       CHECK( cond.Units( EDA_UNITS::INCHES ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,         CHECK( cond.Units( EDA_UNITS::MILS ) ) );

    mgr->SetConditions( ACTIONS::cut,               ENABLE( isEditableCond ) );
    mgr->SetConditions( ACTIONS::copy,              ENABLE( haveSymbolCond ) );
    mgr->SetConditions( ACTIONS::paste,             ENABLE( isEditableCond && SELECTION_CONDITIONS::Idle ) );
    mgr->SetConditions( ACTIONS::doDelete,          ENABLE( isEditableCond ) );
    mgr->SetConditions( ACTIONS::duplicate,         ENABLE( isEditableCond ) );
    mgr->SetConditions( ACTIONS::selectAll,         ENABLE( haveSymbolCond ) );

    mgr->SetConditions( EE_ACTIONS::rotateCW,       ENABLE( isEditableCond ) );
    mgr->SetConditions( EE_ACTIONS::rotateCCW,      ENABLE( isEditableCond ) );
    mgr->SetConditions( EE_ACTIONS::mirrorH,        ENABLE( isEditableCond ) );
    mgr->SetConditions( EE_ACTIONS::mirrorV,        ENABLE( isEditableCond ) );

    mgr->SetConditions( ACTIONS::zoomTool,          CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,     CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );

     auto pinTypeCond =
        [this] ( const SELECTION& )
        {
            return GetRenderSettings()->m_ShowPinsElectricalType;
        };

    auto showCompTreeCond =
        [this] ( const SELECTION& )
        {
            return IsSymbolTreeShown();
        };

    mgr->SetConditions( EE_ACTIONS::showElectricalTypes, CHECK( pinTypeCond ) );
    mgr->SetConditions( EE_ACTIONS::showSymbolTree, CHECK( showCompTreeCond ) );

    auto demorganCond =
        [this] ( const SELECTION& )
        {
            return GetShowDeMorgan();
        };

    auto demorganStandardCond =
        [this] ( const SELECTION& )
        {
            return m_convert == LIB_ITEM::LIB_CONVERT::BASE;
        };

    auto demorganAlternateCond =
        [this] ( const SELECTION& )
        {
            return m_convert == LIB_ITEM::LIB_CONVERT::DEMORGAN;
        };

    auto multiUnitModeCond =
        [this] ( const SELECTION& )
        {
            return m_my_part && m_my_part->IsMulti() && !m_my_part->UnitsLocked();
        };

    auto syncedPinsModeCond =
        [this] ( const SELECTION& )
        {
            return m_SyncPinEdit;
        };

    auto haveDatasheetCond =
        [this] ( const SELECTION& )
        {
            return m_my_part && !m_my_part->GetDatasheetField().GetText().IsEmpty();
        };

    mgr->SetConditions( EE_ACTIONS::showDatasheet,    ENABLE( haveDatasheetCond ) );
    mgr->SetConditions( EE_ACTIONS::symbolProperties, ENABLE( canEditProperties && haveSymbolCond ) );
    mgr->SetConditions( EE_ACTIONS::runERC,           ENABLE( haveSymbolCond ) );
    mgr->SetConditions( EE_ACTIONS::pinTable,         ENABLE( isEditableCond && haveSymbolCond ) );

    mgr->SetConditions( EE_ACTIONS::showDeMorganStandard,
                        ACTION_CONDITIONS().Enable( demorganCond ).Check( demorganStandardCond ) );
    mgr->SetConditions( EE_ACTIONS::showDeMorganAlternate,
                        ACTION_CONDITIONS().Enable( demorganCond ).Check( demorganAlternateCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleSyncedPinsMode,
                        ACTION_CONDITIONS().Enable( multiUnitModeCond ).Check( syncedPinsModeCond ) );

// Only enable a tool if the part is edtable
#define EDIT_TOOL( tool ) ACTION_CONDITIONS().Enable( isEditableCond ).Check( cond.CurrentTool( tool ) )

    mgr->SetConditions( ACTIONS::deleteTool,             EDIT_TOOL( ACTIONS::deleteTool ) );
    mgr->SetConditions( EE_ACTIONS::placeSymbolPin,      EDIT_TOOL( EE_ACTIONS::placeSymbolPin ) );
    mgr->SetConditions( EE_ACTIONS::placeSymbolText,     EDIT_TOOL( EE_ACTIONS::placeSymbolText ) );
    mgr->SetConditions( EE_ACTIONS::drawSymbolRectangle, EDIT_TOOL( EE_ACTIONS::drawSymbolRectangle ) );
    mgr->SetConditions( EE_ACTIONS::drawSymbolCircle,    EDIT_TOOL( EE_ACTIONS::drawSymbolCircle ) );
    mgr->SetConditions( EE_ACTIONS::drawSymbolArc,       EDIT_TOOL( EE_ACTIONS::drawSymbolArc ) );
    mgr->SetConditions( EE_ACTIONS::drawSymbolLines,     EDIT_TOOL( EE_ACTIONS::drawSymbolLines ) );
    mgr->SetConditions( EE_ACTIONS::placeSymbolAnchor,   EDIT_TOOL( EE_ACTIONS::placeSymbolAnchor ) );

#undef CHECK
#undef ENABLE
#undef EDIT_TOOL
}


bool SYMBOL_EDIT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Shutdown blocks must be determined and vetoed as early as possible
    if( KIPLATFORM::APP::SupportsShutdownBlockReason() && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && IsContentModified() )
    {
        return false;
    }

    if( m_isSymbolFromSchematic && IsContentModified() )
    {
        SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );

        switch( UnsavedChangesDialog( this,
                                      _( "Save changes to schematic before closing?" ),
                                      nullptr ) )
        {
        case wxID_YES:
            if( schframe && GetCurPart() )  // Should be always the case
                schframe->SaveSymbolToSchematic( *GetCurPart());

            return true;

        case wxID_NO: return true;

        default:
        case wxID_CANCEL: return false;
        }
    }

    if( !saveAllLibraries( true ) )
    {
        return false;
    }

    return true;
}


void SYMBOL_EDIT_FRAME::doCloseWindow()
{
    Destroy();
}


void SYMBOL_EDIT_FRAME::RebuildSymbolUnitsList()
{
    if( !m_unitSelectBox )
        return;

    if( m_unitSelectBox->GetCount() != 0 )
        m_unitSelectBox->Clear();

    if( !m_my_part || m_my_part->GetUnitCount() <= 1 )
    {
        m_unit = 1;
        m_unitSelectBox->Append( wxEmptyString );
    }
    else
    {
        for( int i = 0; i < m_my_part->GetUnitCount(); i++ )
        {
            wxString sub  = LIB_PART::SubReference( i+1, false );
            wxString unit = wxString::Format( _( "Unit %s" ), sub );
            m_unitSelectBox->Append( unit );
        }
    }

    // Ensure the selected unit is compatible with the number of units of the current part:
    if( m_my_part && m_my_part->GetUnitCount() < m_unit )
        m_unit = 1;

    m_unitSelectBox->SetSelection(( m_unit > 0 ) ? m_unit - 1 : 0 );
}


void SYMBOL_EDIT_FRAME::OnToggleSymbolTree( wxCommandEvent& event )
{
    auto& treePane = m_auimgr.GetPane( m_treePane );
    treePane.Show( !IsSymbolTreeShown() );
    m_auimgr.Update();
}


bool SYMBOL_EDIT_FRAME::IsSymbolTreeShown()
{
    return m_auimgr.GetPane( m_treePane ).IsShown();
}


void SYMBOL_EDIT_FRAME::FreezeLibraryTree()
{
    m_treePane->Freeze();
    m_libMgr->GetAdapter()->Freeze();
}


void SYMBOL_EDIT_FRAME::ThawLibraryTree()
{
    m_libMgr->GetAdapter()->Thaw();
    m_treePane->Thaw();
}


void SYMBOL_EDIT_FRAME::OnExitKiCad( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


void SYMBOL_EDIT_FRAME::OnUpdatePartNumber( wxUpdateUIEvent& event )
{
    event.Enable( m_my_part && m_my_part->GetUnitCount() > 1 );
}


void SYMBOL_EDIT_FRAME::OnSelectUnit( wxCommandEvent& event )
{
    int i = event.GetSelection();

    if( ( i == wxNOT_FOUND ) || ( ( i + 1 ) == m_unit ) )
        return;

    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );
    m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );

    m_unit = i + 1;

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    RebuildView();
}


bool SYMBOL_EDIT_FRAME::IsSymbolFromLegacyLibrary() const
{
    if( m_my_part )
    {
        SYMBOL_LIB_TABLE_ROW* row = m_libMgr->GetLibrary( m_my_part->GetLibNickname() );

        if( row && row->GetType() == SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY ) )
            return true;
    }

    return false;
}


wxString SYMBOL_EDIT_FRAME::GetCurLib() const
{
    wxString libNickname = Prj().GetRString( PROJECT::SCH_LIBEDIT_CUR_LIB );

    if( !libNickname.empty() )
    {
        if( !Prj().SchSymbolLibTable()->HasLibrary( libNickname ) )
        {
            Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
            libNickname = wxEmptyString;
        }
    }

    return libNickname;
}


wxString SYMBOL_EDIT_FRAME::SetCurLib( const wxString& aLibNickname )
{
    wxString old = GetCurLib();

    if( aLibNickname.empty() || !Prj().SchSymbolLibTable()->HasLibrary( aLibNickname ) )
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
    else
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, aLibNickname );

    return old;
}


void SYMBOL_EDIT_FRAME::SetCurPart( LIB_PART* aPart, bool aUpdateZoom )
{
    m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );
    GetCanvas()->GetView()->Clear();
    delete m_my_part;

    m_my_part = aPart;

    // select the current component in the tree widget
    if( !IsSymbolFromSchematic() && m_my_part )
        m_treePane->GetLibTree()->SelectLibId( m_my_part->GetLibId() );
    else
        m_treePane->GetLibTree()->Unselect();

    wxString partName = m_my_part ? m_my_part->GetName() : wxString();

    // retain in case this wxFrame is re-opened later on the same PROJECT
    Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_PART, partName );

    // Ensure synchronized pin edit can be enabled only symbols with interchangeable units
    m_SyncPinEdit = aPart && aPart->IsRoot() && aPart->IsMulti() && !aPart->UnitsLocked();

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    GetRenderSettings()->m_ShowUnit = m_unit;
    GetRenderSettings()->m_ShowConvert = m_convert;
    GetRenderSettings()->m_ShowDisabled = IsSymbolFromLegacyLibrary() && !IsSymbolFromSchematic();
    GetRenderSettings()->m_ShowGraphicsDisabled = IsSymbolAlias() && !IsSymbolFromSchematic();
    GetCanvas()->DisplayComponent( m_my_part );
    GetCanvas()->GetView()->HideDrawingSheet();
    GetCanvas()->GetView()->ClearHiddenFlags();

    if( aUpdateZoom )
        m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );

    GetCanvas()->Refresh();

    WX_INFOBAR* infobar = GetInfoBar();

    if( IsSymbolFromSchematic() )
    {
        wxString msg;
        msg.Printf( _( "Editing symbol %s from schematic.  Saving will update the schematic "
                       "only." ), m_reference );

        infobar->RemoveAllButtons();
        infobar->ShowMessage( msg, wxICON_INFORMATION );
    }
    else if( IsSymbolFromLegacyLibrary() )
    {
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY,
                                                       _( "Manage symbol libraries" ),
                                                       wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [&]( wxHyperlinkEvent& aEvent )
                {
                    InvokeSchEditSymbolLibTable( &Kiway(), this );
                } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );
        infobar->ShowMessage( _( "Symbols in legacy libraries are not editable.  Use Manage "
                                 "Symbol Libraries to migrate to current format." ),
                              wxICON_INFORMATION );
    }
    else if( IsSymbolAlias() )
    {
        wxString parentPartName = m_my_part->GetParent().lock()->GetName();
        wxString msg;
        wxString link;

        msg.Printf( _( "Symbol %s is derived from %s.  Symbol graphics will not be editable." ),
                    partName,
                    parentPartName );

        link.Printf( _( "Open %s" ), parentPartName );

        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, link, wxEmptyString );
        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [&]( wxHyperlinkEvent& aEvent )
                {
                    LoadSymbolFromCurrentLib( m_my_part->GetParent().lock()->GetName(),
                                              GetUnit(), GetConvert() );
                } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );
        infobar->ShowMessage( msg, wxICON_INFORMATION );
    }
    else
    {
        infobar->Dismiss();
    }
}


SYMBOL_LIBRARY_MANAGER& SYMBOL_EDIT_FRAME::GetLibManager()
{
    wxASSERT( m_libMgr );
    return *m_libMgr;
}


void SYMBOL_EDIT_FRAME::OnModify()
{
    GetScreen()->SetModify();
    storeCurrentPart();

    m_treePane->GetLibTree()->RefreshLibTree();

    if( !GetTitle().StartsWith( "*" ) )
        updateTitle();
}


bool SYMBOL_EDIT_FRAME::SynchronizePins()
{
    return m_SyncPinEdit && m_my_part && m_my_part->IsMulti() && !m_my_part->UnitsLocked();
}


bool SYMBOL_EDIT_FRAME::AddLibraryFile( bool aCreateNew )
{
    // Select the target library table (global/project)
    SYMBOL_LIB_TABLE* libTable = selectSymLibTable();

    if( !libTable )
        return false;

    wxFileName fn = m_libMgr->GetUniqueLibraryName();

    if( !LibraryFileBrowser( !aCreateNew, fn, KiCadSymbolLibFileWildcard(),
                             KiCadSymbolLibFileExtension, false,
                             ( libTable == &SYMBOL_LIB_TABLE::GetGlobalLibTable() ),
                             PATHS::GetDefaultUserSymbolsPath() ) )
    {
        return false;
    }

    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return false;

    if( m_libMgr->LibraryExists( libName ) )
    {
        DisplayError( this, wxString::Format( _( "Library \"%s\" already exists" ), libName ) );
        return false;
    }

    if( aCreateNew )
    {
        if( !m_libMgr->CreateLibrary( fn.GetFullPath(), libTable ) )
        {
            DisplayError( this, wxString::Format( _( "Could not create the library file '%s'.\n"
                                                     "Check write permission." ),
                                                  fn.GetFullPath() ) );
            return false;
        }
    }
    else
    {
        if( !m_libMgr->AddLibrary( fn.GetFullPath(), libTable ) )
        {
            DisplayError( this, _( "Could not open the library file." ) );
            return false;
        }
    }

    bool globalTable = ( libTable == &SYMBOL_LIB_TABLE::GetGlobalLibTable() );
    saveSymbolLibTables( globalTable, !globalTable );

    return true;
}


LIB_ID SYMBOL_EDIT_FRAME::GetTreeLIBID( int* aUnit ) const
{
    return m_treePane->GetLibTree()->GetSelectedLibId( aUnit );
}


LIB_PART* SYMBOL_EDIT_FRAME::getTargetPart() const
{
    LIB_ID libId = GetTreeLIBID();

    if( libId.IsValid() )
    {
        LIB_PART* alias = m_libMgr->GetAlias( libId.GetLibItemName(), libId.GetLibNickname() );
        return alias;
    }

    return m_my_part;
}


LIB_ID SYMBOL_EDIT_FRAME::GetTargetLibId() const
{
    LIB_ID id = GetTreeLIBID();

    if( id.GetLibNickname().empty() && m_my_part )
        id = m_my_part->GetLibId();

    return id;
}


LIB_TREE_NODE* SYMBOL_EDIT_FRAME::GetCurrentTreeNode() const
{
    return m_treePane->GetLibTree()->GetCurrentTreeNode();
}


wxString SYMBOL_EDIT_FRAME::getTargetLib() const
{
    return GetTargetLibId().GetLibNickname();
}


void SYMBOL_EDIT_FRAME::SyncLibraries( bool aShowProgress, const wxString& aForceRefresh )
{
    LIB_ID selected;

    if( m_treePane )
        selected = m_treePane->GetLibTree()->GetSelectedLibId();

    if( aShowProgress )
    {
        APP_PROGRESS_DIALOG progressDlg( _( "Loading Symbol Libraries" ), wxEmptyString,
                                         m_libMgr->GetAdapter()->GetLibrariesCount(), this );

        m_libMgr->Sync( aForceRefresh,
                [&]( int progress, int max, const wxString& libName )
                {
                    progressDlg.Update( progress,
                                        wxString::Format( _( "Loading library '%s'" ), libName ) );
                } );
    }
    else
    {
        m_libMgr->Sync( aForceRefresh,
                [&]( int progress, int max, const wxString& libName )
                {
                } );
    }

    if( m_treePane )
    {
        wxDataViewItem found;

        if( selected.IsValid() )
        {
            // Check if the previously selected item is still valid,
            // if not - it has to be unselected to prevent crash
            found = m_libMgr->GetAdapter()->FindItem( selected );

            if( !found )
                m_treePane->GetLibTree()->Unselect();
        }

        m_treePane->GetLibTree()->Regenerate( true );

        // Try to select the parent library, in case the part is not found
        if( !found && selected.IsValid() )
        {
            selected.SetLibItemName( "" );
            found = m_libMgr->GetAdapter()->FindItem( selected );

            if( found )
                m_treePane->GetLibTree()->SelectLibId( selected );
        }

        // If no selection, see if there's a current part to centre
        if( !selected.IsValid() && m_my_part )
        {
            LIB_ID current( GetCurLib(), m_my_part->GetName() );
            m_treePane->GetLibTree()->CenterLibId( current );
        }
    }
}


void SYMBOL_EDIT_FRAME::RegenerateLibraryTree()
{
    LIB_ID target = GetTargetLibId();

    m_treePane->GetLibTree()->Regenerate( true );

    if( target.IsValid() )
        m_treePane->GetLibTree()->CenterLibId( target );
}


void SYMBOL_EDIT_FRAME::RefreshLibraryTree()
{
    m_treePane->GetLibTree()->RefreshLibTree();
}


SYMBOL_LIB_TABLE* SYMBOL_EDIT_FRAME::selectSymLibTable( bool aOptional )
{
    // If no project is loaded, always work with the global table
    if( Prj().IsNullProject() )
    {
        SYMBOL_LIB_TABLE* ret = &SYMBOL_LIB_TABLE::GetGlobalLibTable();

        if( aOptional )
        {
            wxMessageDialog dlg( this, _( "Add the library to the global library table?" ),
                                 _( "Add To Global Library Table" ), wxYES_NO );

            if( dlg.ShowModal() != wxID_OK )
                ret = nullptr;
        }

        return ret;
    }

    wxArrayString libTableNames;
    libTableNames.Add( _( "Global" ) );
    libTableNames.Add( _( "Project" ) );

    wxSingleChoiceDialog dlg( this, _( "Choose the Library Table to add the library to:" ),
                              _( "Add To Library Table" ), libTableNames );

    if( aOptional )
    {
        dlg.FindWindow( wxID_CANCEL )->SetLabel( _( "Skip" ) );
        dlg.FindWindow( wxID_OK )->SetLabel( _( "Add" ) );
    }

    if( dlg.ShowModal() != wxID_OK )
        return nullptr;

    switch( dlg.GetSelection() )
    {
    case 0:  return &SYMBOL_LIB_TABLE::GetGlobalLibTable();
    case 1:  return Prj().SchSymbolLibTable();
    default: return nullptr;
    }
}


bool SYMBOL_EDIT_FRAME::backupFile( const wxFileName& aOriginalFile, const wxString& aBackupExt )
{
    if( aOriginalFile.FileExists() )
    {
        wxFileName backupFileName( aOriginalFile );
        backupFileName.SetExt( aBackupExt );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxCopyFile( aOriginalFile.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            DisplayError( this, wxString::Format( _( "Failed to save backup to \"%s\"" ),
                                                  backupFileName.GetFullPath() ) );
            return false;
        }
    }

    return true;
}


void SYMBOL_EDIT_FRAME::storeCurrentPart()
{
    if( m_my_part && !GetCurLib().IsEmpty() && GetScreen()->IsModify() )
        m_libMgr->UpdatePart( m_my_part, GetCurLib() ); // UpdatePart() makes a copy
}


bool SYMBOL_EDIT_FRAME::isCurrentPart( const LIB_ID& aLibId ) const
{
    // This will return the root part of any alias
    LIB_PART* part = m_libMgr->GetBufferedPart( aLibId.GetLibItemName(), aLibId.GetLibNickname() );
    // Now we can compare the libId of the current part and the root part
    return ( part && m_my_part && part->GetLibId() == m_my_part->GetLibId() );
}


void SYMBOL_EDIT_FRAME::emptyScreen()
{
    m_treePane->GetLibTree()->Unselect();
    SetCurLib( wxEmptyString );
    SetCurPart( nullptr, false );
    SetScreen( m_dummyScreen );
    ClearUndoRedoList();
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    Refresh();
}


void SYMBOL_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    SCH_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );
    GetCanvas()->GetGAL()->DrawGrid();

    RecreateToolbars();

    if( aEnvVarsChanged )
        SyncLibraries( true );

    Layout();
    SendSizeEvent();
}


void SYMBOL_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    SCH_BASE_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateToolbars();

    // status bar
    UpdateMsgPanel();
}


void SYMBOL_EDIT_FRAME::SetScreen( BASE_SCREEN* aScreen )
{
    SCH_BASE_FRAME::SetScreen( aScreen );
}


void SYMBOL_EDIT_FRAME::RebuildView()
{
    GetRenderSettings()->m_ShowUnit = m_unit;
    GetRenderSettings()->m_ShowConvert = m_convert;
    GetRenderSettings()->m_ShowDisabled = IsSymbolFromLegacyLibrary() && !IsSymbolFromSchematic();
    GetRenderSettings()->m_ShowGraphicsDisabled = IsSymbolAlias() && !IsSymbolFromSchematic();
    GetCanvas()->DisplayComponent( m_my_part );
    GetCanvas()->GetView()->HideDrawingSheet();
    GetCanvas()->GetView()->ClearHiddenFlags();

    GetCanvas()->Refresh();
}


void SYMBOL_EDIT_FRAME::HardRedraw()
{
    SyncLibraries( true );

    if( m_my_part )
    {
        EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
        EE_SELECTION&      selection = selectionTool->GetSelection();

        for( LIB_ITEM& item : m_my_part->GetDrawItems() )
        {
            if( !alg::contains( selection, &item ) )
                item.ClearSelected();
            else
                item.SetSelected();
        }
    }

    RebuildView();
}


const BOX2I SYMBOL_EDIT_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    if( !m_my_part )
    {
        return BOX2I( VECTOR2I( Mils2iu( -100 ), Mils2iu( -100 ) ),
                      VECTOR2I( Mils2iu( 200 ), Mils2iu( 200 ) ) );
    }
    else
    {
        EDA_RECT boundingBox = m_my_part->Flatten()->GetUnitBoundingBox( m_unit, m_convert );
        return BOX2I( boundingBox.GetOrigin(), VECTOR2I( boundingBox.GetWidth(),
                                                         boundingBox.GetHeight() ) );
    }
}


void SYMBOL_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_LIB_EDIT:
        if( !payload.empty() )
        {
            wxString libFileName( payload );
            wxString libNickname;
            wxString msg;

            SYMBOL_LIB_TABLE*    libTable = Prj().SchSymbolLibTable();
            const LIB_TABLE_ROW* libTableRow = libTable->FindRowByURI( libFileName );

            if( !libTableRow )
            {
                msg.Printf( _( "The current configuration does not include the library '%s'.\n"
                               "Use Manage Symbol Libraries to edit the configuration." ),
                            libFileName );
                DisplayErrorMessage( this, _( "Library not found in symbol library table." ), msg );
                break;
            }

            libNickname = libTableRow->GetNickName();

            if( !libTable->HasLibrary( libNickname, true ) )
            {
                msg.Printf( _( "The library '%s' is not enabled in the current configuration.\n"
                               "Use Manage Symbol Libraries to edit the configuration." ),
                            libNickname );
                DisplayErrorMessage( this, _( "Symbol library not enabled." ), msg );
                break;
            }

            SetCurLib( libNickname );

            if( m_treePane )
            {
                LIB_ID id( libNickname, wxEmptyString );
                m_treePane->GetLibTree()->ExpandLibId( id );
                m_treePane->GetLibTree()->CenterLibId( id );
            }
        }

        break;

    default:
        ;
    }
}


void SYMBOL_EDIT_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // switches currently used canvas ( Cairo / OpenGL):
    SCH_BASE_FRAME::SwitchCanvas( aCanvasType );

    // Set options specific to symbol editor (axies are always enabled):
    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );
}


bool SYMBOL_EDIT_FRAME::HasLibModifications() const
{
    wxCHECK( m_libMgr, false );

    return m_libMgr->HasModifications();
}


bool SYMBOL_EDIT_FRAME::IsContentModified()
{
    wxCHECK( m_libMgr, false );

    // Test if the currently edited part is modified
    if( GetScreen() && GetScreen()->IsModify() && GetCurPart() )
        return true;

    // Test if any library has been modified
    for( const wxString& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libName ) && !m_libMgr->IsLibraryReadOnly( libName ) )
            return true;
    }

    return false;
}


void SYMBOL_EDIT_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = whichList == UNDO_LIST ? m_undoList : m_redoList;

    for( PICKED_ITEMS_LIST* command : list.m_CommandsList )
    {
        command->ClearListAndDeleteItems();
        delete command;
    }

    list.m_CommandsList.clear();
}


SELECTION& SYMBOL_EDIT_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();
}


void SYMBOL_EDIT_FRAME::LoadSymbolFromSchematic( SCH_COMPONENT* aSymbol )
{
    std::unique_ptr<LIB_PART> part = aSymbol->GetPartRef()->Flatten();
    wxCHECK( part, /* void */ );

    std::vector<LIB_FIELD> fullSetOfFields;

    for( int i = 0; i < (int) aSymbol->GetFields().size(); ++i )
    {
        const SCH_FIELD& field = aSymbol->GetFields()[i];
        wxPoint          pos = field.GetPosition() - aSymbol->GetPosition();
        LIB_FIELD        libField( part.get(), field.GetId() );

        if( i >= MANDATORY_FIELDS && !field.GetName( false ).IsEmpty() )
            libField.SetName( field.GetName( false ) );

        libField.SetText( field.GetText() );
        libField.SetEffects( field );
        libField.SetPosition( wxPoint( pos.x, -pos.y ) );

        fullSetOfFields.emplace_back( std::move( libField ) );
    }

    part->SetFields( fullSetOfFields );

    if( m_my_part )
        SetCurPart( nullptr, false );

    m_isSymbolFromSchematic = true;
    m_reference = part->GetFieldById( REFERENCE_FIELD )->GetText();
    m_unit = std::max( 1, aSymbol->GetUnit() );
    m_convert = std::max( 1, aSymbol->GetConvert() );

    // The buffered screen for the part
    SCH_SCREEN* tmpScreen = new SCH_SCREEN();

    SetScreen( tmpScreen );
    SetCurPart( part.release(), true );

    ReCreateMenuBar();
    ReCreateHToolbar();

    if( IsSymbolTreeShown() )
    {
        wxCommandEvent evt;
        OnToggleSymbolTree( evt );
    }

    updateTitle();
    RebuildSymbolUnitsList();
    SetShowDeMorgan( GetCurPart()->HasConversion() );
    DisplaySymbolDatasheet();
    Refresh();
}


bool SYMBOL_EDIT_FRAME::addLibTableEntry( const wxString& aLibFile, TABLE_SCOPE aScope )
{
    wxFileName fn = aLibFile;
    wxFileName libTableFileName( Prj().GetProjectPath(),
                                 SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
    wxString libNickname = fn.GetName();
    SYMBOL_LIB_TABLE* libTable = Prj().SchSymbolLibTable();
    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();

    if( libTable->HasLibrary( libNickname ) )
    {
        wxString tmp;
        int suffix = 1;

        while( libTable->HasLibrary( libNickname ) )
        {
            tmp.Printf( "%s%d", fn.GetName(), suffix );
            libNickname = tmp;
            suffix += 1;
        }
    }

    SYMBOL_LIB_TABLE_ROW* row = new SYMBOL_LIB_TABLE_ROW();
    row->SetNickName( libNickname );

    wxString normalizedPath = NormalizePath( aLibFile, &envVars, Prj().GetProjectPath() );

    if( aScope == GLOBAL_LIB_TABLE )
    {
        libTable = &SYMBOL_LIB_TABLE::GetGlobalLibTable();
        libTableFileName = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

        // We cannot normalize against the current project path when saving to global table.
        normalizedPath = NormalizePath( aLibFile, &envVars, wxEmptyString );
    }

    if( normalizedPath.IsEmpty() )
        normalizedPath = aLibFile;

    row->SetFullURI( normalizedPath );

    wxCHECK( libTable->InsertRow( row ), false );

    try
    {
        libTable->Save( libTableFileName.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = aScope == GLOBAL_LIB_TABLE ? _( "Error saving global library table." )
                                                  : _( "Error saving project library table." );

        wxMessageDialog dlg( this, msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        dlg.SetExtendedMessage( ioe.What() );
        dlg.ShowModal();

        return false;
    }

    return true;
}


bool SYMBOL_EDIT_FRAME::replaceLibTableEntry( const wxString& aLibNickname,
                                              const wxString& aLibFile )
{
    // Check the global library table first because checking the project library table
    // checks the global library table as well due to library chaining.
    bool isGlobalTable = true;
    wxFileName libTableFileName = SYMBOL_LIB_TABLE::GetGlobalTableFileName();;
    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();
    SYMBOL_LIB_TABLE* libTable = &SYMBOL_LIB_TABLE::GetGlobalLibTable();
    SYMBOL_LIB_TABLE_ROW* row = libTable->FindRow( aLibNickname );

    if( !row )
    {
        libTableFileName.SetPath( Prj().GetProjectPath() );
        libTableFileName.SetName( SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
        libTable = Prj().SchSymbolLibTable();
        isGlobalTable = false;
        row = libTable->FindRow( aLibNickname );
    }

    wxCHECK( row, false );

    wxString projectPath;

    if( !isGlobalTable )
        projectPath = Prj().GetProjectPath();

    wxString normalizedPath = NormalizePath( aLibFile, &envVars, projectPath );

    if( normalizedPath.IsEmpty() )
        normalizedPath = aLibFile;

    row->SetFullURI( normalizedPath );
    row->SetType( "KiCad" );

    try
    {
        libTable->Save( libTableFileName.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = isGlobalTable ? _( "Error saving global library table." )
                                     : _( "Error saving project library table." );

        wxMessageDialog dlg( this, msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        dlg.SetExtendedMessage( ioe.What() );
        dlg.ShowModal();

        return false;
    }

    return true;
}


bool SYMBOL_EDIT_FRAME::IsSymbolAlias() const
{
    return m_my_part && !m_my_part->IsRoot();
}


bool SYMBOL_EDIT_FRAME::IsSymbolEditable() const
{
    return m_my_part && ( !IsSymbolFromLegacyLibrary() || IsSymbolFromSchematic() );
}
