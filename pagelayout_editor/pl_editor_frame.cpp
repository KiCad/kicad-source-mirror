/**
 * @file pl_editor_frame.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <kiface_i.h>
#include <class_drawpanel.h>
#include <build_version.h>
#include <macros.h>
#include <base_units.h>
#include <msgpanel.h>

#include <pl_editor_frame.h>
#include <pl_editor_id.h>
#include <hotkeys.h>
#include <class_pl_editor_screen.h>
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>
#include <design_tree_frame.h>
#include <properties_frame.h>

#include <wildcards_and_files_ext.h>
#include <confirm.h>

/*************************/
/* class PL_EDITOR_FRAME */
/*************************/

PL_EDITOR_FRAME::PL_EDITOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    EDA_DRAW_FRAME( aKiway, aParent, FRAME_PL_EDITOR, wxT( "PlEditorFrame" ),
            wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, PL_EDITOR_FRAME_NAME )
{
    m_zoomLevelCoeff = 290.0;   // Adjusted to roughly displays zoom level = 1
                                // when the screen shows a 1:1 image
                                // obviously depends on the monitor,
                                // but this is an acceptable value

    m_showAxis = false;                 // true to show X and Y axis on screen
    m_showGridAxis = true;
    m_showBorderAndTitleBlock   = true; // true for reference drawings.
    m_HotkeysZoomAndGridList    = PlEditorHokeysDescr;
    m_originSelectChoice = 0;
    SetDrawBgColor( WHITE );            // default value, user option (WHITE/BLACK)
    SetShowPageLimits( true );

    m_designTreeWidth = 150;
    m_propertiesFrameWidth = 200;

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( false );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_pagelayout_editor_xpm ) );
    SetIcon( icon );
    wxSize pageSizeIU = GetPageLayout().GetPageSettings().GetSizeIU();
    SetScreen( new PL_EDITOR_SCREEN( pageSizeIU ) );

    LoadSettings( config() );
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( m_LastGridSizeId < ID_POPUP_GRID_LEVEL_1MM-ID_POPUP_GRID_LEVEL_1000 )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_1MM-ID_POPUP_GRID_LEVEL_1000;
    if( m_LastGridSizeId > ID_POPUP_GRID_LEVEL_0_1MM-ID_POPUP_GRID_LEVEL_1000 )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_0_1MM-ID_POPUP_GRID_LEVEL_1000;
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateOptToolbar();

    wxWindow* stsbar = GetStatusBar();
    int dims[] = {

        // balance of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        GetTextSize( wxT( "Z 762000" ), stsbar ).x + 10,

        // cursor coords
        GetTextSize( wxT( "X 0234.567  Y 0234.567" ), stsbar ).x + 10,

        // delta distances
        GetTextSize( wxT( "dx 0234.567  dx 0234.567" ), stsbar ).x + 10,

        // Coord origin (use the bigger message)
        GetTextSize( _( "coord origin: Right Bottom page corner" ), stsbar ).x + 10,

        // units display, Inches is bigger than mm
        GetTextSize( _( "Inches" ), stsbar ).x + 10
    };

    SetStatusWidths( DIM( dims ), dims );


    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO    horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO    vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO    mesg;
    mesg.MessageToolbarPane();

    m_propertiesPagelayout = new PROPERTIES_FRAME( this );
    EDA_PANEINFO    props;
    props.LayersToolbarPane();
    props.MinSize( m_propertiesPagelayout->GetMinSize() );
    props.BestSize( m_propertiesFrameWidth, -1 );
    props.Caption( _( "Properties" ) );

    m_treePagelayout = new DESIGN_TREE_FRAME( this );
    EDA_PANEINFO    tree;
    tree.LayersToolbarPane();
    tree.MinSize( m_treePagelayout->GetMinSize() );
    tree.BestSize( m_designTreeWidth, -1 );
    tree.Caption( _( "Design" ) );

    if( m_mainToolBar )
        m_auimgr.AddPane( m_mainToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top().Row( 0 ) );

    if( m_drawToolBar )
        m_auimgr.AddPane( m_drawToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_drawToolBar" ) ).Right().Row( 1 ) );

    m_auimgr.AddPane( m_propertiesPagelayout,
                      props.Name( wxT( "m_propertiesPagelayout" ) ).Right().Layer( 1 ) );

    m_auimgr.AddPane( m_treePagelayout,
                      tree.Name( wxT( "m_treePagelayout" ) ).Left().Layer( 0 ) );

    if( m_optionsToolBar )
        m_auimgr.AddPane( m_optionsToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_optionsToolBar" ) ).Left() );

    if( m_canvas )
        m_auimgr.AddPane( m_canvas,
                          wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane().Layer( 5 ) );

    if( m_messagePanel )
        m_auimgr.AddPane( m_messagePanel,
                          wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer( 10 ) );

    m_auimgr.Update();

    // Initialize the current page layout
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
#if 0       //start with empty layout
    pglayout.AllowVoidList( true );
    pglayout.ClearList();
#else       // start with the default Kicad layout
    pglayout.SetPageLayout();
#endif
    OnNewPageLayout();
}


PL_EDITOR_FRAME::~PL_EDITOR_FRAME()
{
}


bool PL_EDITOR_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    wxString fn = aFileSet[0];

    if( !LoadPageLayoutDescrFile( fn ) )
    {
        wxString msg = wxString::Format(
            _( "Error when loading file '%s'" ),
            GetChars( fn )
            );

        wxMessageBox( msg );
        return false;
    }
    else
    {
        OnNewPageLayout();
        return true;
    }
}


void PL_EDITOR_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( GetScreen()->IsModify() )
    {
        wxString msg;
        wxString filename = GetCurrFileName();
        if( filename.IsEmpty() )
            msg = _("Save changes in a new file before closing?");
        else
            msg.Printf( _("Save the changes in\n<%s>\nbefore closing?"),
                        GetChars( filename ) );

        int ii = DisplayExitDialog( this, msg );

        switch( ii )
        {
        case wxID_CANCEL:
            Event.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_OK:
        case wxID_YES:
        {
            if( filename.IsEmpty() )
            {
                wxFileDialog openFileDialog(this, _("Create file"), wxEmptyString,
                        wxEmptyString, PageLayoutDescrFileWildcard, wxFD_SAVE);

                if (openFileDialog.ShowModal() == wxID_CANCEL)
                    return;

                filename = openFileDialog.GetPath();
            }

            if( !SavePageLayoutDescrFile( filename ) )
            {
                wxString msg;
                msg.Printf( _("Unable to create <%s>"), GetChars( filename ) );
                wxMessageBox( msg );
            }
        }
            break;
        }
    }

    // do not show the window because we do not want any paint event
    Show( false );

    // was: Pgm().SaveCurrentSetupValues( m_configSettings );
    wxConfigSaveSetups( Kiface().KifaceSettings(), m_configSettings );

    // On Linux, m_propertiesPagelayout must be destroyed
    // before deleting the main frame to avoid a crash when closing
    m_propertiesPagelayout->Destroy();
    Destroy();
}


double PL_EDITOR_FRAME::BestZoom()
{
    int    dx, dy;
    wxSize size;

    dx = GetPageLayout().GetPageSettings().GetWidthIU();
    dy = GetPageLayout().GetPageSettings().GetHeightIU();

    size = m_canvas->GetClientSize();

    // Reserve no margin because best zoom shows the full page
    // and margins are already included in function that draws the sheet refernces
    double margin_scale_factor = 1.0;
    double zx =(double) dx / ( margin_scale_factor * (double)size.x );
    double zy = (double) dy / ( margin_scale_factor * (double)size.y );

    double bestzoom = std::max( zx, zy );

    SetScrollCenterPosition( wxPoint( dx / 2, dy / 2 ) );

    return bestzoom;
}

static const wxChar designTreeWidthKey[] = wxT("DesignTreeWidth");
static const wxChar propertiesFrameWidthKey[] = wxT("PropertiesFrameWidth");
static const wxChar cornerOriginChoiceKey[] = wxT("CornerOriginChoice");
static const wxChar blackBgColorKey[] = wxT( "BlackBgColor" );

void PL_EDITOR_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    aCfg->Read( designTreeWidthKey, &m_designTreeWidth, 100);
    aCfg->Read( propertiesFrameWidthKey, &m_propertiesFrameWidth, 150);
    aCfg->Read( cornerOriginChoiceKey, &m_originSelectChoice );
    bool tmp;
    aCfg->Read( blackBgColorKey, &tmp, false );
    SetDrawBgColor( tmp ? BLACK : WHITE );
}


void PL_EDITOR_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    m_designTreeWidth = m_treePagelayout->GetSize().x;
    m_propertiesFrameWidth = m_propertiesPagelayout->GetSize().x;

    aCfg->Write( designTreeWidthKey, m_designTreeWidth);
    aCfg->Write( propertiesFrameWidthKey, m_propertiesFrameWidth);
    aCfg->Write( cornerOriginChoiceKey, m_originSelectChoice );
    aCfg->Write( blackBgColorKey, GetDrawBgColor() == BLACK );

    // was: wxGetApp().SaveCurrentSetupValues( GetConfigurationSettings() );
    wxConfigSaveSetups( aCfg, GetConfigurationSettings() );
}


/*
 * Function UpdateTitleAndInfo
 * displays the filename (if exists) of the current page layout descr file.
 */
void PL_EDITOR_FRAME::UpdateTitleAndInfo()
{
    wxString title;
    title.Printf( wxT( "Pl_Editor %s [%s]" ), GetChars( GetBuildVersion() ),
        GetChars( GetCurrFileName() ) );
    SetTitle( title );
}

/* return the filename of the current layout descr file
 */
const wxString& PL_EDITOR_FRAME::GetCurrFileName() const
{
    return BASE_SCREEN::m_PageLayoutDescrFileName;
}

/* Stores the current layout descr file filename
 */
void PL_EDITOR_FRAME::SetCurrFileName( const wxString& aName )
{
    BASE_SCREEN::m_PageLayoutDescrFileName = aName;
}

void PL_EDITOR_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_pageLayout.SetPageSettings( aPageSettings );

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU() );
}


const PAGE_INFO& PL_EDITOR_FRAME::GetPageSettings() const
{
    return m_pageLayout.GetPageSettings();
}


const wxSize PL_EDITOR_FRAME::GetPageSizeIU() const
{
    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return m_pageLayout.GetPageSettings().GetSizeIU();
}


const TITLE_BLOCK& PL_EDITOR_FRAME::GetTitleBlock() const
{
    return GetPageLayout().GetTitleBlock();
}


void PL_EDITOR_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    m_pageLayout.SetTitleBlock( aTitleBlock );
}


/*
 * Update the status bar information.
 */
void PL_EDITOR_FRAME::UpdateStatusBar()
{
    PL_EDITOR_SCREEN* screen = (PL_EDITOR_SCREEN*) GetScreen();

    if( !screen )
        return;

    // Display Zoom level:
    EDA_DRAW_FRAME::UpdateStatusBar();

    // coodinate origin can be the paper Top Left corner,
    // or each of 4 page corners
    // We know the origin, and the orientation of axis
    wxPoint originCoord;
    int Xsign = 1;
    int Ysign = 1;

    WORKSHEET_DATAITEM dummy( WORKSHEET_DATAITEM::WS_SEGMENT );

    switch( m_originSelectChoice )
    {
    default:
    case 0: // Origin = paper Left Top corner
        break;

    case 1: // Origin = page Right Bottom corner
        Xsign = -1;
        Ysign = -1;
        dummy.SetStart( 0, 0, RB_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;

    case 2: // Origin = page Left Bottom corner
        Ysign = -1;
        dummy.SetStart( 0, 0, LB_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;

    case 3: // Origin = page Right Top corner
        Xsign = -1;
        dummy.SetStart( 0, 0, RT_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;

    case 4: // Origin = page Left Top corner
        dummy.SetStart( 0, 0, LT_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;
    }

    SetGridOrigin( originCoord );

    // Display absolute coordinates:
    wxPoint coord = GetCrossHairPosition() - originCoord;
    double dXpos = To_User_Unit( g_UserUnit, coord.x*Xsign );
    double dYpos = To_User_Unit( g_UserUnit, coord.y*Ysign );

    wxString pagesizeformatter = _( "Page size: width %.4g height %.4g" );
    wxString absformatter = wxT( "X %.4g  Y %.4g" );
    wxString locformatter = wxT( "dx %.4g  dy %.4g" );

    switch( g_UserUnit )
    {
    case INCHES:        // Should not be used in page layout editor
        SetStatusText( _("inches"), 5 );
        break;

    case MILLIMETRES:
        SetStatusText( _("mm"), 5 );
        break;

    case UNSCALED_UNITS:
        SetStatusText( wxEmptyString, 5 );
        break;

    case DEGREES:
        wxASSERT( false );
        break;
    }

    wxString line;

    // Display page size
    #define milsTomm (25.4/1000)
    DSIZE size = GetPageSettings().GetSizeMils();
    size = size * milsTomm;
    line.Printf( pagesizeformatter, size.x, size.y );
    SetStatusText( line, 0 );

    // Display abs coordinates
    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    // Display relative coordinates:
    int dx = GetCrossHairPosition().x - screen->m_O_Curseur.x;
    int dy = GetCrossHairPosition().y - screen->m_O_Curseur.y;
    dXpos = To_User_Unit( g_UserUnit, dx * Xsign );
    dYpos = To_User_Unit( g_UserUnit, dy * Ysign );
    line.Printf( locformatter, dXpos, dYpos );
    SetStatusText( line, 3 );

    // Display corner reference for coord origin
    line.Printf( _("coord origin: %s"),
                m_originSelectBox->GetString( m_originSelectChoice ). GetData() );
    SetStatusText( line, 4 );

    // Display units
}

void PL_EDITOR_FRAME::PrintPage( wxDC* aDC, LSET aPrintMasklayer,
                           bool aPrintMirrorMode, void * aData )
{
    GetScreen()-> m_ScreenNumber = GetPageNumberOption() ? 1 : 2;
    DrawWorkSheet( aDC, GetScreen(), 0, IU_PER_MILS, wxEmptyString );
}

void PL_EDITOR_FRAME::RedrawActiveWindow( wxDC* aDC, bool aEraseBg )
{

    GetScreen()-> m_ScreenNumber = GetPageNumberOption() ? 1 : 2;

    if( aEraseBg )
        m_canvas->EraseScreen( aDC );

    m_canvas->DrawBackGround( aDC );

    const WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    WORKSHEET_DATAITEM* selecteditem = GetSelectedItem();

    // the color to draw selected items
    if( GetDrawBgColor() == WHITE )
        WORKSHEET_DATAITEM::m_SelectedColor = DARKCYAN;
    else
        WORKSHEET_DATAITEM::m_SelectedColor = YELLOW;

    for( unsigned ii = 0; ; ii++ )
    {
        WORKSHEET_DATAITEM* item = pglayout.GetItem( ii );

        if( item == NULL )
            break;

        item->SetSelected( item == selecteditem );
    }

    DrawWorkSheet( aDC, GetScreen(), 0, IU_PER_MILS, GetCurrFileName() );

#ifdef USE_WX_OVERLAY
    if( IsShown() )
    {
        m_overlay.Reset();
        wxDCOverlay overlaydc( m_overlay, (wxWindowDC*)aDC );
        overlaydc.Clear();
    }
#endif

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( aDC, wxDefaultPosition, false );

    m_canvas->DrawCrossHair( aDC );

    // Display the filename
    UpdateTitleAndInfo();
}

void PL_EDITOR_FRAME::RebuildDesignTree()
{
    const WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    int rectId = 0;
    int lineId = 0;
    int textId = 0;
    int polyId = 0;
    int bitmapId = 0;

    for( unsigned ii = 0; ii < pglayout.GetCount(); ii++ )
    {
        WORKSHEET_DATAITEM* item = pglayout.GetItem( ii );
        switch( item->GetType() )
        {
            case WORKSHEET_DATAITEM::WS_TEXT:
                item->m_Name = wxString::Format( wxT("text%d:%s"), ++textId,
                                                 GetChars(item->GetClassName()) );
                break;

            case WORKSHEET_DATAITEM:: WS_SEGMENT:
                item->m_Name = wxString::Format( wxT("segm%d:%s"), ++lineId,
                                                 GetChars(item->GetClassName()) );
                break;

            case WORKSHEET_DATAITEM::WS_RECT:
                item->m_Name = wxString::Format( wxT("rect%d:%s"), ++rectId,
                                                 GetChars(item->GetClassName()) );
                break;

            case WORKSHEET_DATAITEM::WS_POLYPOLYGON:
                item->m_Name = wxString::Format( wxT("poly%d:%s"), ++polyId,
                                                 GetChars(item->GetClassName()) );
                break;

            case WORKSHEET_DATAITEM::WS_BITMAP:
                item->m_Name = wxString::Format( wxT("bm%d:%s"), ++bitmapId,
                                                 GetChars(item->GetClassName()) );
                break;
        }
    }

    m_treePagelayout->ReCreateDesignTree();
}

/* Add a new item to the page layout item list.
 * aType = WS_TEXT, WS_SEGMENT, WS_RECT, WS_POLYPOLYGON
 */
WORKSHEET_DATAITEM * PL_EDITOR_FRAME::AddPageLayoutItem( int aType, int aIdx )
{
    WORKSHEET_DATAITEM * item = NULL;

    switch( aType )
    {
        case WORKSHEET_DATAITEM::WS_TEXT:
            item = new WORKSHEET_DATAITEM_TEXT( wxT("Text") );
            break;

        case WORKSHEET_DATAITEM::WS_SEGMENT:
            item = new WORKSHEET_DATAITEM( WORKSHEET_DATAITEM::WS_SEGMENT );
            break;

        case WORKSHEET_DATAITEM::WS_RECT:
            item = new WORKSHEET_DATAITEM( WORKSHEET_DATAITEM::WS_RECT );
            break;

        case WORKSHEET_DATAITEM::WS_POLYPOLYGON:
            item = new WORKSHEET_DATAITEM_POLYPOLYGON();
            break;

        case WORKSHEET_DATAITEM::WS_BITMAP:
        {
            wxFileDialog fileDlg( this, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                                  _( "Image Files " ) + wxImage::GetImageExtWildcard(),
                                  wxFD_OPEN );

            if( fileDlg.ShowModal() != wxID_OK )
                return NULL;

            wxString fullFilename = fileDlg.GetPath();

            if( !wxFileExists( fullFilename ) )
            {
                wxMessageBox( _( "Couldn't load image from <%s>" ), GetChars( fullFilename ) );
                break;
            }
            BITMAP_BASE* image = new BITMAP_BASE();

            if( !image->ReadImageFile( fullFilename ) )
            {
                wxMessageBox( _( "Couldn't load image from <%s>" ),
                                 GetChars( fullFilename ) );
                delete image;
                break;
            }
            item = new WORKSHEET_DATAITEM_BITMAP( image );
        }
            break;
    }

    if( item == NULL )
        return NULL;

    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    pglayout.Insert( item, aIdx );
    RebuildDesignTree();

    return item;
}

/* returns the current selected item, or NULL
 */
WORKSHEET_DATAITEM * PL_EDITOR_FRAME::GetSelectedItem()
{
    WORKSHEET_DATAITEM* item =  m_treePagelayout->GetPageLayoutSelectedItem();
    return item;
}

/* return the page layout item found at position aPosition
 * aPosition = the position (in user units) of the reference point
 */
WORKSHEET_DATAITEM* PL_EDITOR_FRAME::Locate( const wxPoint& aPosition )
{
    const PAGE_INFO&    pageInfo = GetPageSettings();
    TITLE_BLOCK         t_block = GetTitleBlock();
    EDA_COLOR_T         color = RED;    // Needed, not used
    PL_EDITOR_SCREEN*   screen = (PL_EDITOR_SCREEN*) GetScreen();

    screen-> m_ScreenNumber = GetPageNumberOption() ? 1 : 2;

    WS_DRAW_ITEM_LIST drawList;
    drawList.SetPenSize( 0 );
    drawList.SetMilsToIUfactor( IU_PER_MILS );
    drawList.SetSheetNumber( screen->m_ScreenNumber );
    drawList.SetSheetCount( screen->m_NumberOfScreens );
    drawList.SetFileName( GetCurrFileName() );
    // GetScreenDesc() returns a temporary string. Store it to avoid issues.
    wxString descr = GetScreenDesc();
    drawList.SetSheetName( descr );

    drawList.BuildWorkSheetGraphicList( pageInfo, t_block, color, color );

    // locate items.
    // We do not use here the COLLECTOR classes in use in pcbnew and eeschema
    // because the locate requirements are very basic.
    std::vector <WS_DRAW_ITEM_BASE*> list;
    drawList.Locate( list, aPosition );

    if( list.size() == 0 )
        return NULL;

    WS_DRAW_ITEM_BASE* drawitem = list[0];

    // Choose item in list if more than 1 item
    if( list.size() > 1 )
    {
        wxArrayString choices;
        wxString text;
        wxPoint cursPos = GetCrossHairPosition();

        for( unsigned ii = 0; ii < list.size(); ++ii )
        {
            wxString    text;
            drawitem = list[ii];
            text = drawitem->GetParent()->m_Name;

            if( (drawitem->m_Flags & (LOCATE_STARTPOINT|LOCATE_ENDPOINT))
                == (LOCATE_STARTPOINT|LOCATE_ENDPOINT) )
                text << wxT(" ") << _("(start or end point)");
            else
            {
                if( (drawitem->m_Flags & LOCATE_STARTPOINT) )
                    text << wxT(" ") << _("(start point)");

                if( (drawitem->m_Flags & LOCATE_ENDPOINT) )
                    text << wxT(" ") << _("(end point)");
            }

            if( ! drawitem->GetParent()->m_Info.IsEmpty() )
                text << wxT(" \"") << drawitem->GetParent()->m_Info << wxT("\"");

            choices.Add( text );
        }
        int selection = wxGetSingleChoiceIndex ( wxEmptyString,
                                                _( "Selection Clarification" ),
                                                choices, this );
        if( selection < 0 )
            return NULL;

        SetCrossHairPosition( cursPos );
        m_canvas->MoveCursorToCrossHair();
        drawitem = list[selection];
    }

    WORKSHEET_DATAITEM* item = drawitem->GetParent();
    item->ClearFlags(LOCATE_STARTPOINT|LOCATE_ENDPOINT);

    if( (drawitem->m_Flags & LOCATE_STARTPOINT) )
        item->SetFlags( LOCATE_STARTPOINT );

    if( (drawitem->m_Flags & LOCATE_ENDPOINT) )
        item->SetFlags( LOCATE_ENDPOINT );

    return item;
}

/* Must be called to initialize parameters when a new page layout
 * description is loaded
 */
void PL_EDITOR_FRAME::OnNewPageLayout()
{
    GetScreen()->ClearUndoRedoList();
    GetScreen()->ClrModify();
    m_propertiesPagelayout->CopyPrmsFromGeneralToPanel();
    RebuildDesignTree();
    Zoom_Automatique( false );
    m_canvas->Refresh();
}


const wxString PL_EDITOR_FRAME::GetZoomLevelIndicator() const
{
    return EDA_DRAW_FRAME::GetZoomLevelIndicator();
}
