/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <base_units.h>
#include <kiway.h>
#include <pgm_base.h>
#include <eeschema_settings.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <settings/settings_manager.h>
#include <confirm.h>
#include <preview_items/selection_area.h>
#include <symbol_library.h>
#include <sch_base_frame.h>
#include <symbol_lib_table.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>


LIB_SYMBOL* SchGetLibSymbol( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable,
                             SYMBOL_LIB* aCacheLib, wxWindow* aParent, bool aShowErrorMsg )
{
    wxCHECK_MSG( aLibTable, nullptr, "Invalid symbol library table." );

    LIB_SYMBOL* symbol = nullptr;

    try
    {
        symbol = aLibTable->LoadSymbol( aLibId );

        if( !symbol && aCacheLib )
        {
            wxCHECK_MSG( aCacheLib->IsCache(), nullptr, "Invalid cache library." );

            wxString cacheName = aLibId.GetLibNickname().wx_str();
            cacheName += "_" + aLibId.GetLibItemName();
            symbol = aCacheLib->FindSymbol( cacheName );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        if( aShowErrorMsg )
        {
            wxString msg = wxString::Format( _( "Error loading symbol %s from library '%s'." ),
                                             aLibId.GetLibItemName().wx_str(),
                                             aLibId.GetLibNickname().wx_str() );
            DisplayErrorMessage( aParent, msg, ioe.What() );
        }
    }

    return symbol;
}


SCH_BASE_FRAME::SCH_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aWindowType,
                                const wxString& aTitle, const wxPoint& aPosition,
                                const wxSize& aSize, long aStyle, const wxString& aFrameName ) :
    EDA_DRAW_FRAME( aKiway, aParent, aWindowType, aTitle, aPosition, aSize, aStyle, aFrameName ),
    m_base_frame_defaults( nullptr, "base_Frame_defaults" )
{
    createCanvas();

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              // Handle cursor adjustments.  While we can get motion and key events through
              // wxWidgets, we can't get modifier-key-up events.
              if( m_toolManager )
              {
                  EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();

                  if( selTool )
                      selTool->OnIdle( aEvent );
              }
          } );
}


SCH_BASE_FRAME::~SCH_BASE_FRAME()
{
}


SCH_SCREEN* SCH_BASE_FRAME::GetScreen() const
{
    return (SCH_SCREEN*) EDA_DRAW_FRAME::GetScreen();
}


EESCHEMA_SETTINGS* SCH_BASE_FRAME::eeconfig() const
{
    return dynamic_cast<EESCHEMA_SETTINGS*>( config() );
}


SYMBOL_EDITOR_SETTINGS* SCH_BASE_FRAME::libeditconfig() const
{
    return dynamic_cast<SYMBOL_EDITOR_SETTINGS*>( config() );
}


void SCH_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    GetScreen()->SetPageSettings( aPageSettings );
}


const PAGE_INFO& SCH_BASE_FRAME::GetPageSettings () const
{
    return GetScreen()->GetPageSettings();
}


const wxSize SCH_BASE_FRAME::GetPageSizeIU() const
{
    // GetSizeIU is compile time dependent:
    return GetScreen()->GetPageSettings().GetSizeIU();
}


const TITLE_BLOCK& SCH_BASE_FRAME::GetTitleBlock() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetTitleBlock();
}


void SCH_BASE_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetTitleBlock( aTitleBlock );
}


void SCH_BASE_FRAME::UpdateStatusBar()
{
    wxString        line;
    BASE_SCREEN*    screen = GetScreen();

    if( !screen )
        return;

    EDA_DRAW_FRAME::UpdateStatusBar();

    // Display absolute and relative coordinates
    VECTOR2D cursorPos = GetCanvas()->GetViewControls()->GetCursorPosition();
    VECTOR2D d         = cursorPos - screen->m_LocalOrigin;

    line.Printf( "X %s  Y %s",
                 MessageTextFromValue( GetUserUnits(), cursorPos.x, false ),
                 MessageTextFromValue( GetUserUnits(), cursorPos.y, false ) );
    SetStatusText( line, 2 );

    line.Printf( "dx %s  dy %s  dist %s",
                 MessageTextFromValue( GetUserUnits(), d.x, false ),
                 MessageTextFromValue( GetUserUnits(), d.y, false ),
                 MessageTextFromValue( GetUserUnits(), hypot( d.x, d.y ), false ) );
    SetStatusText( line, 3 );

    // refresh grid display
    DisplayGridMsg();

    // refresh units display
    DisplayUnitsMsg();
}


LIB_SYMBOL* SCH_BASE_FRAME::GetLibSymbol( const LIB_ID& aLibId, bool aUseCacheLib,
                                          bool aShowErrorMsg )
{
    SYMBOL_LIB* cache = ( aUseCacheLib ) ? Prj().SchLibs()->GetCacheLibrary() : nullptr;

    return SchGetLibSymbol( aLibId, Prj().SchSymbolLibTable(), cache, this, aShowErrorMsg );
}


bool SCH_BASE_FRAME::saveSymbolLibTables( bool aGlobal, bool aProject )
{
    wxString msg;
    bool success = true;

    if( aGlobal )
    {
        try
        {
            SYMBOL_LIB_TABLE::GetGlobalLibTable().Save( SYMBOL_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            msg.Printf( _( "Error saving global symbol library table:\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( aProject && !Prj().GetProjectName().IsEmpty() )
    {
        wxFileName fn( Prj().GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        try
        {
            Prj().SchSymbolLibTable()->Save( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            msg.Printf( _( "Error saving project-specific symbol library table:\n%s" ),
                        ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    return success;
}


void SCH_BASE_FRAME::RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    GetCanvas()->GetView()->SetCenter( aCenterPoint );

    if( aWarpPointer )
        GetCanvas()->GetViewControls()->CenterOnCursor();

    GetCanvas()->Refresh();
}


void SCH_BASE_FRAME::CenterScreen( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    GetCanvas()->GetView()->SetCenter( aCenterPoint );

    if( aWarpPointer )
        GetCanvas()->GetViewControls()->WarpCursor( aCenterPoint, true );

    GetCanvas()->Refresh();
}


void SCH_BASE_FRAME::HardRedraw()
{
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetCanvas()->ForceRefresh();
}


SCH_DRAW_PANEL* SCH_BASE_FRAME::GetCanvas() const
{
    return static_cast<SCH_DRAW_PANEL*>( EDA_DRAW_FRAME::GetCanvas() );
}


KIGFX::SCH_RENDER_SETTINGS* SCH_BASE_FRAME::GetRenderSettings()
{
    KIGFX::PAINTER* painter = GetCanvas()->GetView()->GetPainter();
    return static_cast<KIGFX::SCH_RENDER_SETTINGS*>( painter->GetSettings() );
}


void SCH_BASE_FRAME::createCanvas()
{
    m_canvasType = loadCanvasTypeSetting();

    SetCanvas( new SCH_DRAW_PANEL( this, wxID_ANY, wxPoint( 0, 0 ), m_frameSize,
                                   GetGalDisplayOptions(), m_canvasType ) );
    ActivateGalCanvas();
}


void SCH_BASE_FRAME::UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete, bool aUpdateRtree )
{
    EDA_ITEM* parent = aItem->GetParent();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // Sheet pins aren't in the view.  Refresh their parent.
        if( parent )
            GetCanvas()->GetView()->Update( parent );
    }
    else
    {
        if( !isAddOrDelete )
            GetCanvas()->GetView()->Update( aItem );

        // Some children are drawn from their parents.  Mark them for re-paint.
        static KICAD_T parentTypes[] = { SCH_SYMBOL_T, SCH_SHEET_T, SCH_GLOBAL_LABEL_T, EOT };

        if( parent && parent->IsType( parentTypes ) )
            GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
    }

    /**
     * Be careful when calling this.  Update will invalidate RTree iterators, so you cannot call this
     * while doing things like `for( SCH_ITEM* item : screen->Items() )`
     */
    if( aUpdateRtree)
        GetScreen()->Update( static_cast<SCH_ITEM*>( aItem ) );

    // Calling Refresh() here introduces a bi-stable state: when doing operations on a
    // large number of items if at some point the refresh timer times out and does a
    // refresh it will take long enough that the next item will also time out, and the
    // next, and the next, etc.
    // GetCanvas()->Refresh();
}


void SCH_BASE_FRAME::RefreshSelection()
{
    if( m_toolManager )
    {
        EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
        SELECTION&         selection = selectionTool->GetSelection();
        KIGFX::SCH_VIEW*   view = GetCanvas()->GetView();

        for( EDA_ITEM* item : selection )
        {
            EDA_ITEM* parent = item->GetParent();

            if( item->Type() == SCH_SHEET_PIN_T )
            {
                // Sheet pins aren't in the view.  Refresh their parent.
                if( parent )
                    GetCanvas()->GetView()->Update( parent );
            }
            else
            {
                view->Update( item, KIGFX::REPAINT );

                // Symbol children are drawn from their parents.  Mark them for re-paint.
                if( parent && parent->Type() == SCH_SYMBOL_T )
                    GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
            }
        }
    }
}


void SCH_BASE_FRAME::AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen )
{
    auto screen = aScreen;

    if( aScreen == nullptr )
        screen = GetScreen();

    screen->Append( (SCH_ITEM*) aItem );

    if( screen == GetScreen() )
    {
        GetCanvas()->GetView()->Add( aItem );
        UpdateItem( aItem, true );           // handle any additional parent semantics
    }
}


void SCH_BASE_FRAME::RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen )
{
    auto screen = aScreen;

    if( aScreen == nullptr )
        screen = GetScreen();

    if( screen == GetScreen() )
        GetCanvas()->GetView()->Remove( aItem );

    screen->Remove( (SCH_ITEM*) aItem );

    if( screen == GetScreen() )
        UpdateItem( aItem, true );           // handle any additional parent semantics
}


void SCH_BASE_FRAME::SyncView()
{
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
}


COLOR4D SCH_BASE_FRAME::GetLayerColor( SCH_LAYER_ID aLayer )
{
    return GetColorSettings()->GetColor( aLayer );
}


void SCH_BASE_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    m_colorSettings = Pgm().GetSettingsManager().GetColorSettings( cfg->m_ColorTheme );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetCanvas()->Refresh();
}


COLOR_SETTINGS* SCH_BASE_FRAME::GetColorSettings() const
{
    if( !m_colorSettings )
    {
        SETTINGS_MANAGER&  settingsManager = Pgm().GetSettingsManager();
        EESCHEMA_SETTINGS* cfg = settingsManager.GetAppSettings<EESCHEMA_SETTINGS>();
        COLOR_SETTINGS*    colorSettings = settingsManager.GetColorSettings( cfg->m_ColorTheme );

        const_cast<SCH_BASE_FRAME*>( this )->m_colorSettings = colorSettings;
    }

    return m_colorSettings;
}


COLOR4D SCH_BASE_FRAME::GetDrawBgColor() const
{
    return GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
}

