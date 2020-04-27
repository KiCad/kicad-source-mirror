/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2020 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <settings/settings_manager.h>
#include <gal/graphics_abstraction_layer.h>
#include <confirm.h>
#include <preview_items/selection_area.h>
#include <class_library.h>
#include <sch_base_frame.h>
#include <symbol_lib_table.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <default_values.h>    // For some default values


LIB_PART* SchGetLibPart( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable, PART_LIB* aCacheLib,
                         wxWindow* aParent, bool aShowErrorMsg )
{
    wxCHECK_MSG( aLibTable, nullptr, "Invalid symbol library table." );

    LIB_PART* symbol = nullptr;

    try
    {
        symbol = aLibTable->LoadSymbol( aLibId );

        if( !symbol && aCacheLib )
        {
            wxCHECK_MSG( aCacheLib->IsCache(), nullptr, "Invalid cache library." );

            wxString cacheName = aLibId.GetLibNickname().wx_str();
            cacheName += "_" + aLibId.GetLibItemName();
            symbol = aCacheLib->FindPart( cacheName );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        if( aShowErrorMsg )
        {
            wxString msg = wxString::Format( _( "Error loading symbol '%s' from library '%s'." ),
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
    m_defaultLineWidth( DEFAULT_LINE_THICKNESS * IU_PER_MILS ),
    m_defaultWireThickness( DEFAULT_WIRE_THICKNESS * IU_PER_MILS ),
    m_defaultBusThickness( DEFAULT_BUS_THICKNESS * IU_PER_MILS ),
    m_defaultTextSize( DEFAULT_TEXT_SIZE * IU_PER_MILS ),
    m_textOffsetRatio( 0.08 ),
    m_showPinElectricalTypeName( false )
{
    createCanvas();

    // Adjusted to display zoom level ~ 1 when the screen shows a 1:1 image
    // Obviously depends on the monitor, but this is an acceptable value
    m_zoomLevelCoeff = 11.0 * IU_PER_MILS;
}


SCH_BASE_FRAME::~SCH_BASE_FRAME()
{
}


SCH_SCREEN* SCH_BASE_FRAME::GetScreen() const
{
    return (SCH_SCREEN*) EDA_DRAW_FRAME::GetScreen();
}


void SCH_BASE_FRAME::SetScreen(  BASE_SCREEN* aScreen )
{
    EDA_DRAW_FRAME::SetScreen( aScreen );

    if( m_toolManager )
    {
        m_toolManager->SetEnvironment( aScreen, GetCanvas()->GetView(),
                                       GetCanvas()->GetViewControls(), this );
    }
}


const wxString SCH_BASE_FRAME::GetZoomLevelIndicator() const
{
    return EDA_DRAW_FRAME::GetZoomLevelIndicator();
}


void SCH_BASE_FRAME::SetDefaultLineWidth( int aWidth )
{
    m_defaultLineWidth = aWidth;
    GetRenderSettings()->SetDefaultPenWidth( aWidth );
}


void SCH_BASE_FRAME::SetDefaultWireThickness( int aThickness )
{
    m_defaultWireThickness = aThickness;
    GetRenderSettings()->m_DefaultWireThickness = aThickness;
}


void SCH_BASE_FRAME::SetDefaultBusThickness( int aThickness )
{
    m_defaultBusThickness = aThickness;
    GetRenderSettings()->m_DefaultBusThickness = aThickness;
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


const wxPoint& SCH_BASE_FRAME::GetAuxOrigin() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetAuxOrigin();
}


void SCH_BASE_FRAME::SetAuxOrigin( const wxPoint& aPosition )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetAuxOrigin( aPosition );
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

    // Display absolute coordinates:
    VECTOR2D cursorPos = GetCanvas()->GetViewControls()->GetCursorPosition();
    double   dXpos = To_User_Unit( GetUserUnits(), cursorPos.x );
    double   dYpos = To_User_Unit( GetUserUnits(), cursorPos.y );

    wxString absformatter;
    wxString locformatter;

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCHES:
        absformatter = "X %.3f  Y %.3f";
        locformatter = "dx %.3f  dy %.3f  dist %.3f";
        break;

    case EDA_UNITS::MILLIMETRES:
        absformatter = "X %.4f  Y %.4f";
        locformatter = "dx %.4f  dy %.4f  dist %.4f";
        break;

    case EDA_UNITS::UNSCALED:
        absformatter = "X %f  Y %f";
        locformatter = "dx %f  dy %f  dist %f";
        break;

    default:
        wxASSERT( false );
        break;
    }

    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    // Display relative coordinates:
    double dx = cursorPos.x - screen->m_LocalOrigin.x;
    double dy = cursorPos.y - screen->m_LocalOrigin.y;

    dXpos = To_User_Unit( GetUserUnits(), dx );
    dYpos = To_User_Unit( GetUserUnits(), dy );

    // We already decided the formatter above
    line.Printf( locformatter, dXpos, dYpos, hypot( dXpos, dYpos ) );
    SetStatusText( line, 3 );

    // refresh grid display
    DisplayGridMsg();

    // refresh units display
    DisplayUnitsMsg();
}


LIB_PART* SCH_BASE_FRAME::GetLibPart( const LIB_ID& aLibId, bool aUseCacheLib, bool aShowErrorMsg )
{
    PART_LIB* cache = ( aUseCacheLib ) ? Prj().SchLibs()->GetCacheLibrary() : NULL;

    return SchGetLibPart( aLibId, Prj().SchSymbolLibTable(), cache, this, aShowErrorMsg );
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
            msg.Printf( _( "Error saving project-specific symbol library table:\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    return success;
}


void SCH_BASE_FRAME::RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    KIGFX::GAL* gal = GetCanvas()->GetGAL();

    double selectedZoom = GetScreen()->GetZoom();
    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
    double scale = 1.0 / ( zoomFactor * selectedZoom );

    if( aCenterPoint != wxPoint( 0, 0 ) )
        GetCanvas()->GetView()->SetScale( scale, aCenterPoint );
    else
        GetCanvas()->GetView()->SetScale( scale );

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


void SCH_BASE_FRAME::FocusOnItem( SCH_ITEM* aItem )
{
    static KIID lastBrightenedItemID( niluuid );

    SCH_SHEET_LIST sheetList( g_RootSheet );
    SCH_SHEET_PATH dummy;
    SCH_ITEM*      lastItem = sheetList.GetItem( lastBrightenedItemID, &dummy );

    if( lastItem && lastItem != aItem )
    {
        lastItem->ClearBrightened();

        RefreshItem( lastItem );
        lastBrightenedItemID = niluuid;
    }

    if( aItem )
    {
        aItem->SetBrightened();

        RefreshItem( aItem );
        lastBrightenedItemID = aItem->m_Uuid;

        // JEY TODO: test this with pins and fields (and with rotated symbols) ....
        FocusOnLocation( aItem->GetFocusPosition() );
    }
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
    m_canvasType = LoadCanvasTypeSetting();

    // Allows only a CAIRO or OPENGL canvas:
    if( m_canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL
            && m_canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO )
    {
        m_canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    }

    SetCanvas( new SCH_DRAW_PANEL( this, wxID_ANY, wxPoint( 0, 0 ), m_FrameSize,
                                   GetGalDisplayOptions(), m_canvasType ));
    ActivateGalCanvas();
}


void SCH_BASE_FRAME::RefreshItem( EDA_ITEM* aItem, bool isAddOrDelete )
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

        // Component children are drawn from their parents.  Mark them for re-paint.
        if( parent && parent->Type() == SCH_COMPONENT_T )
            GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
        else if( parent && parent->Type() == SCH_SHEET_T )
            GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
    }

    GetCanvas()->Refresh();
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

                // Component children are drawn from their parents.  Mark them for re-paint.
                if( parent && parent->Type() == SCH_COMPONENT_T )
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
        RefreshItem( aItem, true );           // handle any additional parent semantics
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
        RefreshItem( aItem, true );           // handle any additional parent semantics
}


void SCH_BASE_FRAME::SyncView()
{
    auto gs = GetScreen()->GetGridSize();
    GetCanvas()->GetGAL()->SetGridSize( VECTOR2D( gs.x, gs.y ));
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
}


COLOR4D SCH_BASE_FRAME::GetLayerColor( SCH_LAYER_ID aLayer )
{
    return GetColorSettings()->GetColor( aLayer );
}


void SCH_BASE_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    m_colorSettings = Pgm().GetSettingsManager().GetColorSettings( cfg->m_ColorTheme );
}


COLOR_SETTINGS* SCH_BASE_FRAME::GetColorSettings()
{
    if( !m_colorSettings )
    {
        EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
        m_colorSettings = Pgm().GetSettingsManager().GetColorSettings( cfg->m_ColorTheme );
    }

    return m_colorSettings;
}
