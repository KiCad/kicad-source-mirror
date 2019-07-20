/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <kiway_express.h>
#include <sch_draw_panel.h>
#include <base_screen.h>
#include <confirm.h>
#include <eda_dockart.h>
#include <general.h>
#include <eeschema_id.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <lib_manager.h>
#include <widgets/symbol_tree_pane.h>
#include <widgets/lib_tree.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>
#include <wx/progdlg.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/picker_tool.h>
#include <tool/common_tools.h>
#include <tool/zoom_tool.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_inspection_tool.h>
#include <tools/lib_pin_tool.h>
#include <tools/lib_edit_tool.h>
#include <tools/lib_move_tool.h>
#include <tools/lib_drawing_tools.h>
#include <tools/lib_control.h>
#include <tools/ee_point_editor.h>
#include <sch_view.h>
#include <sch_painter.h>


bool LIB_EDIT_FRAME::          m_showDeMorgan    = false;
int LIB_EDIT_FRAME::           g_LastTextSize    = -1;
double LIB_EDIT_FRAME::        g_LastTextAngle   = TEXT_ANGLE_HORIZ;
int LIB_EDIT_FRAME::           g_LastLineWidth   = 0;

// these values are overridden when reading the config
int LIB_EDIT_FRAME::           m_textPinNumDefaultSize = DEFAULTPINNUMSIZE;
int LIB_EDIT_FRAME::           m_textPinNameDefaultSize = DEFAULTPINNAMESIZE;
int LIB_EDIT_FRAME::           m_defaultPinLength = DEFAULTPINLENGTH;

FILL_T LIB_EDIT_FRAME::        g_LastFillStyle   = NO_FILL;


BEGIN_EVENT_TABLE( LIB_EDIT_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( LIB_EDIT_FRAME::OnCloseWindow )
    EVT_SIZE( LIB_EDIT_FRAME::OnSize )

    EVT_COMBOBOX( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnSelectUnit )

    // Right vertical toolbar.
    EVT_TOOL( ID_LIBEDIT_IMPORT_BODY_BUTT, LIB_EDIT_FRAME::OnImportBody )
    EVT_TOOL( ID_LIBEDIT_EXPORT_BODY_BUTT, LIB_EDIT_FRAME::OnExportBody )

    // menubar commands
    EVT_MENU( wxID_EXIT, LIB_EDIT_FRAME::OnExitKiCad )
    EVT_MENU( wxID_CLOSE, LIB_EDIT_FRAME::CloseWindow )
    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )

    // Update user interface elements.
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnUpdatePartNumber )

END_EVENT_TABLE()

LIB_EDIT_FRAME::LIB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH_LIB_EDITOR, _( "Library Editor" ),
        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, LIB_EDIT_FRAME_NAME )
{
    SetShowDeMorgan( false );
    m_DrawSpecificConvert = true;
    m_DrawSpecificUnit    = false;
    m_SyncPinEdit         = false;
    m_repeatPinStep = DEFAULT_REPEAT_OFFSET_PIN;
    SetShowElectricalType( true );
    m_FrameSize = ConvertDialogToPixels( wxSize( 500, 350 ) );    // default in case of no prefs

    m_my_part = nullptr;
    m_treePane = nullptr;
    m_libMgr = nullptr;
    m_unit = 1;
    m_convert = 1;
    m_AboutTitle = "LibEdit";

    // Delayed initialization
    if( g_LastTextSize == -1 )
        g_LastTextSize = GetDefaultTextSize();

    // Initialize grid id to the default value 50 mils:
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_libedit_xpm ) );
    SetIcon( icon );

    LoadSettings( config() );

    // Ensure axis are always drawn
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = GetGalDisplayOptions();
    gal_opts.m_axesEnabled = true;

    m_dummyScreen = new SCH_SCREEN( aKiway );
    SetScreen( m_dummyScreen );
    GetScreen()->m_Center = true;
    GetScreen()->SetMaxUndoItems( m_UndoRedoCountMax );

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    setupTools();

    m_libMgr = new LIB_MANAGER( *this );
    SyncLibraries( true );
    m_treePane = new SYMBOL_TREE_PANE( this, m_libMgr );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();
    InitExitKey();

    updateTitle();
    DisplayCmpDoc();
    RebuildSymbolUnitsList();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART() );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "ComponentTree" ).Left().Layer(1)
                      .Caption( _( "Libraries" ) ).MinSize( 250, -1 )
                      .BestSize( m_defaultLibWidth, -1 ).Resizable() );
    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" ).Right().Layer(1) );

    m_auimgr.AddPane( GetCanvas(), wxAuiPaneInfo().Name( "DrawFrame" ).CentrePane() );

    m_auimgr.Update();

    GetToolManager()->RunAction( "common.Control.gridPreset", true, m_LastGridSizeId );

    Raise();
    Show( true );

    SyncView();
    GetCanvas()->GetViewControls()->SetSnapping( true );
    GetCanvas()->GetView()->UseDrawPriority( true );
    GetCanvas()->GetGAL()->SetGridVisibility( IsGridVisible() );
    GetCanvas()->GetGAL()->SetAxesEnabled( true );

    // Set the working/draw area size to display a symbol to a reasonable value:
    // A 600mm x 600mm with a origin at the area center looks like a large working area
    double max_size_x = Millimeter2iu( 600 );
    double max_size_y = Millimeter2iu( 600 );
    BOX2D bbox;
    bbox.SetOrigin( -max_size_x /2, -max_size_y/2 );
    bbox.SetSize( max_size_x, max_size_y );
    GetCanvas()->GetView()->SetBoundary( bbox );

    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
}


LIB_EDIT_FRAME::~LIB_EDIT_FRAME()
{
    // current screen is destroyed in EDA_DRAW_FRAME
    SetScreen( m_dummyScreen );

    delete m_libMgr;
    delete m_my_part;
}


void LIB_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), this );
    m_actions = new EE_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EE_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PICKER_TOOL );
    m_toolManager->RegisterTool( new EE_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new LIB_PIN_TOOL );
    m_toolManager->RegisterTool( new LIB_DRAWING_TOOLS );
    m_toolManager->RegisterTool( new EE_POINT_EDITOR );
    m_toolManager->RegisterTool( new LIB_MOVE_TOOL );
    m_toolManager->RegisterTool( new LIB_EDIT_TOOL );
    m_toolManager->RegisterTool( new LIB_CONTROL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "eeschema.InteractiveSelection" );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void LIB_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( saveAllLibraries( true ) )
        Destroy();
    else
        Event.Veto();
}


double LIB_EDIT_FRAME::BestZoom()
{
    LIB_PART*  part = GetCurPart();
    double     defaultLibraryZoom = 7.33;

    if( !part )
    {
        GetCanvas()->GetView()->SetCenter( VECTOR2D( 0, 0 ) );
        return defaultLibraryZoom;
    }

    EDA_RECT boundingBox = part->GetUnitBoundingBox( m_unit, m_convert );

    double  sizeX  = (double) boundingBox.GetWidth();
    double  sizeY  = (double) boundingBox.GetHeight();
    wxPoint centre = boundingBox.Centre();

    // Reserve a 20% margin around component bounding box.
    double margin_scale_factor = 1.2;

    return bestZoom( sizeX, sizeY, margin_scale_factor, centre);
}


void LIB_EDIT_FRAME::RebuildSymbolUnitsList()
{
    if( !m_unitSelectBox )
        return;

    if( m_unitSelectBox->GetCount() != 0 )
        m_unitSelectBox->Clear();

    LIB_PART* part = GetCurPart();

    if( !part || part->GetUnitCount() <= 1 )
    {
        m_unit = 1;
        m_unitSelectBox->Append( wxEmptyString );
    }
    else
    {
        for( int i = 0; i < part->GetUnitCount(); i++ )
        {
            wxString sub  = LIB_PART::SubReference( i+1, false );
            wxString unit = wxString::Format( _( "Unit %s" ), GetChars( sub ) );
            m_unitSelectBox->Append( unit );
        }
    }

    // Ensure the selected unit is compatible with the number of units of the current part:
    if( part && part->GetUnitCount() < m_unit )
        m_unit = 1;

    m_unitSelectBox->SetSelection(( m_unit > 0 ) ? m_unit - 1 : 0 );
}


void LIB_EDIT_FRAME::OnToggleSearchTree( wxCommandEvent& event )
{
    auto& treePane = m_auimgr.GetPane( m_treePane );
    treePane.Show( !IsSearchTreeShown() );
    m_auimgr.Update();
}


bool LIB_EDIT_FRAME::IsSearchTreeShown()
{
    return m_auimgr.GetPane( m_treePane ).IsShown();
}


void LIB_EDIT_FRAME::FreezeSearchTree()
{
    m_libMgr->GetAdapter()->Freeze();
}


void LIB_EDIT_FRAME::ThawSearchTree()
{
    m_libMgr->GetAdapter()->Thaw();
}


void LIB_EDIT_FRAME::OnExitKiCad( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


void LIB_EDIT_FRAME::OnUpdatePartNumber( wxUpdateUIEvent& event )
{
    if( !m_unitSelectBox )
        return;

    LIB_PART* part = GetCurPart();

    // Using the typical event.Enable() call doesn't seem to work with wxGTK
    // so use the pointer to alias combobox to directly enable or disable.
    m_unitSelectBox->Enable( part && part->GetUnitCount() > 1 );
}


void LIB_EDIT_FRAME::OnSelectUnit( wxCommandEvent& event )
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


wxString LIB_EDIT_FRAME::GetCurLib() const
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


wxString LIB_EDIT_FRAME::SetCurLib( const wxString& aLibNickname )
{
    wxString old = GetCurLib();

    if( aLibNickname.empty() || !Prj().SchSymbolLibTable()->HasLibrary( aLibNickname ) )
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
    else
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, aLibNickname );

    m_libMgr->SetCurrentLib( aLibNickname );

    return old;
}


void LIB_EDIT_FRAME::SetCurPart( LIB_PART* aPart )
{
    if( !aPart && !m_my_part )
        return;

    m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );

    if( m_my_part != aPart )
    {
        delete m_my_part;
        m_my_part = aPart;
    }

    // select the current component in the tree widget
    if( aPart )
        m_treePane->GetLibTree()->SelectLibId( aPart->GetLibId() );

    wxString partName = aPart ? aPart->GetName() : wxString();
    m_libMgr->SetCurrentPart( partName );

    // retain in case this wxFrame is re-opened later on the same PROJECT
    Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_PART, partName );

    // Ensure synchronized pin edit can be enabled only symbols with interchangeable units
    m_SyncPinEdit = aPart && aPart->IsMulti() && !aPart->UnitsLocked();

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    RebuildView();
}


void LIB_EDIT_FRAME::OnImportBody( wxCommandEvent& aEvent )
{
    m_toolManager->DeactivateTool();
    LoadOneSymbol();
    m_drawToolBar->ToggleTool( ID_LIBEDIT_IMPORT_BODY_BUTT, false );
}


void LIB_EDIT_FRAME::OnExportBody( wxCommandEvent& aEvent )
{
    m_toolManager->DeactivateTool();
    SaveOneSymbol();
    m_drawToolBar->ToggleTool( ID_LIBEDIT_EXPORT_BODY_BUTT, false );
}


void LIB_EDIT_FRAME::OnModify()
{
    GetScreen()->SetModify();
    storeCurrentPart();

    m_treePane->GetLibTree()->Refresh();
}


bool LIB_EDIT_FRAME::SynchronizePins()
{
    LIB_PART* part = GetCurPart();

    return m_SyncPinEdit && part && part->IsMulti() && !part->UnitsLocked();
}


void LIB_EDIT_FRAME::refreshSchematic()
{
    // There may be no parent window so use KIWAY message to refresh the schematic editor
    // in case any symbols have changed.
    std::string dummyPayload;
    Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_REFRESH, dummyPayload, this );
}


bool LIB_EDIT_FRAME::AddLibraryFile( bool aCreateNew )
{
    wxFileName fn = m_libMgr->GetUniqueLibraryName();

    if( !LibraryFileBrowser( !aCreateNew, fn, SchematicLibraryFileWildcard(),
                             SchematicLibraryFileExtension, false ) )
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

    // Select the target library table (global/project)
    SYMBOL_LIB_TABLE* libTable = selectSymLibTable();

    if( !libTable )
        return false;

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


LIB_ID LIB_EDIT_FRAME::GetTreeLIBID( int* aUnit ) const
{
    return m_treePane->GetLibTree()->GetSelectedLibId( aUnit );
}


LIB_PART* LIB_EDIT_FRAME::getTargetPart() const
{
    LIB_ID libId = GetTreeLIBID();

    if( libId.IsValid() )
    {
        LIB_ALIAS* alias = m_libMgr->GetAlias( libId.GetLibItemName(), libId.GetLibNickname() );
        return alias ? alias->GetPart() : nullptr;
    }

    return GetCurPart();
}


LIB_ID LIB_EDIT_FRAME::getTargetLibId() const
{
    LIB_ID id = GetTreeLIBID();

    if( id.GetLibNickname().empty() && GetCurPart() )
        id = GetCurPart()->GetLibId();

    return id;
}


wxString LIB_EDIT_FRAME::getTargetLib() const
{
    return getTargetLibId().GetLibNickname();
}


void LIB_EDIT_FRAME::SyncLibraries( bool aShowProgress )
{
    LIB_ID selected;

    if( m_treePane )
        selected = m_treePane->GetLibTree()->GetSelectedLibId();

    if( aShowProgress )
    {
        wxProgressDialog progressDlg( _( "Loading Symbol Libraries" ), wxEmptyString,
                                      m_libMgr->GetAdapter()->GetLibrariesCount(), this );

        m_libMgr->Sync( true, [&]( int progress, int max, const wxString& libName )
        {
            progressDlg.Update( progress, wxString::Format( _( "Loading library \"%s\"" ), libName ) );
        } );
    }
    else
    {
        m_libMgr->Sync( true );
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

        m_treePane->Regenerate();

        // Try to select the parent library, in case the part is not found
        if( !found && selected.IsValid() )
        {
            selected.SetLibItemName( "" );
            found = m_libMgr->GetAdapter()->FindItem( selected );

            if( found )
                m_treePane->GetLibTree()->SelectLibId( selected );
        }

        // If no selection, see if there's a current part to centre
        if( !selected.IsValid() && GetCurPart() )
        {
            LIB_ID current( GetCurLib(), GetCurPart()->GetName() );
            m_treePane->GetLibTree()->CenterLibId( current );
        }
    }
}


SYMBOL_LIB_TABLE* LIB_EDIT_FRAME::selectSymLibTable( bool aOptional )
{
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


bool LIB_EDIT_FRAME::backupFile( const wxFileName& aOriginalFile, const wxString& aBackupExt )
{
    if( aOriginalFile.FileExists() )
    {
        wxFileName backupFileName( aOriginalFile );
        backupFileName.SetExt( "bck" );

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


void LIB_EDIT_FRAME::storeCurrentPart()
{
    if( m_my_part && !GetCurLib().IsEmpty() && GetScreen()->IsModify() )
        m_libMgr->UpdatePart( m_my_part, GetCurLib() ); // UpdatePart() makes a copy
}


bool LIB_EDIT_FRAME::isCurrentPart( const LIB_ID& aLibId ) const
{
    // This will return the root part of any alias
    LIB_PART* part = m_libMgr->GetBufferedPart( aLibId.GetLibItemName(), aLibId.GetLibNickname() );
    // Now we can compare the libId of the current part and the root part
    return ( part && GetCurPart() && part->GetLibId() == GetCurPart()->GetLibId() );
}


void LIB_EDIT_FRAME::emptyScreen()
{
    m_treePane->GetLibTree()->Unselect();
    SetCurLib( wxEmptyString );
    SetCurPart( nullptr );
    SetScreen( m_dummyScreen );
    m_dummyScreen->ClearUndoRedoList();
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    Refresh();
}


void LIB_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    SCH_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    RecreateToolbars();

    if( aEnvVarsChanged )
        SyncLibraries( true );

    Layout();
    SendSizeEvent();
}


void LIB_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    SCH_BASE_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateToolbars();

    // status bar
    UpdateMsgPanel();
}


void LIB_EDIT_FRAME::SetScreen( BASE_SCREEN* aScreen )
{
    SCH_BASE_FRAME::SetScreen( aScreen );
}


void LIB_EDIT_FRAME::RebuildView()
{
    GetRenderSettings()->m_ShowUnit = m_unit;
    GetRenderSettings()->m_ShowConvert = m_convert;
    GetCanvas()->DisplayComponent( m_my_part );

    GetCanvas()->GetView()->HideWorksheet();
    GetCanvas()->GetView()->ClearHiddenFlags();

    GetCanvas()->Refresh();
}


void LIB_EDIT_FRAME::HardRedraw()
{
    SyncLibraries( true );
    RebuildView();
}


const BOX2I LIB_EDIT_FRAME::GetDocumentExtents() const
{
    LIB_PART*  part = GetCurPart();

    if( !part )
    {
        return BOX2I( VECTOR2I(-100, -100), VECTOR2I( 200, 200 ) );
    }
    else
    {
        EDA_RECT boundingBox = part->GetUnitBoundingBox( m_unit, m_convert );
        return BOX2I( boundingBox.GetOrigin(), VECTOR2I( boundingBox.GetWidth(), boundingBox.GetHeight() ) );
    }
}


void LIB_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
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
                msg.Printf( _( "The current configuration does not include the symbol library\n"
                               "\"%s\".\nUse Manage Symbol Libraries to edit the configuration." ),
                            libFileName );
                DisplayErrorMessage( this, _( "Library not found in symbol library table." ), msg );
                break;
            }

            libNickname = libTableRow->GetNickName();

            if( !libTable->HasLibrary( libNickname, true ) )
            {
                msg.Printf( _( "The library with the nickname \"%s\" is not enabled\n"
                               "in the current configuration.  Use Manage Symbol Libraries to\n"
                               "edit the configuration." ), libNickname );
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


void LIB_EDIT_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // switches currently used canvas ( Cairo / OpenGL):
    SCH_BASE_FRAME::SwitchCanvas( aCanvasType );

    // Set options specific to symbol editor (axies are always enabled):
    GetCanvas()->GetGAL()->SetAxesEnabled( true );
}


bool LIB_EDIT_FRAME::HasLibModifications() const
{
    wxCHECK( m_libMgr, false );

    return m_libMgr->HasModifications();
}
