/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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


// WARNING - this Tom's crappy PNS hack tool code. Please don't complain about its quality
// (unless you want to improve it).
#include <string>

#include <confirm.h>
#include <wx/clipbrd.h>
#include <pgm_base.h>
#include <core/profile.h>
#include <reporter.h>
#include <trace_helpers.h>
#include <view/view_overlay.h>
#include <view/view_controls.h>
#include <wildcards_and_files_ext.h>


#include "label_manager.h"

#include "pns_log_file.h"
#include "pns_log_player.h"
#include "pns_log_viewer_frame.h"

#include "router/pns_diff_pair.h"
#include "router/pns_utils.h"
#include "router/router_preview_item.h"

#include <geometry/shape_compound.h>

class WX_SHAPE_TREE_ITEM_DATA : public wxClientData
{
public:
    WX_SHAPE_TREE_ITEM_DATA( PNS_DEBUG_SHAPE* item, int level = 0 ) : m_item( item ), m_level( level ) {};

    PNS_DEBUG_SHAPE* m_item;
    int m_level;
};


PNS_LOG_VIEWER_OVERLAY::PNS_LOG_VIEWER_OVERLAY( KIGFX::GAL* aGal )
{
    m_labelMgr.reset( new LABEL_MANAGER( aGal ) );
}


void PNS_LOG_VIEWER_OVERLAY::AnnotatedPolyset( const SHAPE_POLY_SET& aPolyset, std::string aName,
                                               bool aShowVertexNumbers )
{
    for( int i = 0; i < aPolyset.OutlineCount(); i++ )
    {
        if( i == 0 && !aName.empty() )
            AnnotatedPolyline( aPolyset.COutline( i ), aName );
        else
            AnnotatedPolyline( aPolyset.COutline( i ), "" );

        for( int j = 0; j < aPolyset.HoleCount( i ); j++ )
            AnnotatedPolyline( aPolyset.CHole( i, j ), "" );
    }
}


void PNS_LOG_VIEWER_OVERLAY::AnnotatedPolyline( const SHAPE_LINE_CHAIN& aL, std::string name,
                                                bool aShowVertexNumbers )
{
    Polyline( aL );

    if( name.length() > 0  && aL.PointCount() > 0 )
        m_labelMgr->Add( aL.CLastPoint(), name, GetStrokeColor() );

    if( aShowVertexNumbers )
    {
        for( int i = 0; i < aL.PointCount(); i++ )
            m_labelMgr->Add( aL.CPoint(i), wxString::Format("%d", i ), GetStrokeColor() );
    }
}


void PNS_LOG_VIEWER_OVERLAY::AnnotatedPoint( const VECTOR2I p, int size, std::string name, bool aShowVertexNumbers )
{
    Line( p + VECTOR2D( size, size ), p - VECTOR2D( size, size ) );
    Line( p + VECTOR2D( -size, size ), p - VECTOR2D( -size, size ) );

    if( name.length() > 0 )
        m_labelMgr->Add( p, name, GetStrokeColor() );
}


void PNS_LOG_VIEWER_OVERLAY::Arc( const SHAPE_ARC& arc )
{
    double    radius = arc.GetRadius();
    EDA_ANGLE start_angle = arc.GetStartAngle();
    EDA_ANGLE angle = arc.GetCentralAngle();

    KIGFX::VIEW_OVERLAY::SetLineWidth( arc.GetWidth() / 10 );
    KIGFX::VIEW_OVERLAY::Arc( arc.GetCenter(), radius, start_angle, start_angle + angle );

    COLOR4D prevStrokeCol = KIGFX::VIEW_OVERLAY::GetStrokeColor();
    COLOR4D lightStrokeCol = prevStrokeCol.WithAlpha(0.5);
    KIGFX::VIEW_OVERLAY::SetStrokeColor( lightStrokeCol );

    KIGFX::VIEW_OVERLAY::SetLineWidth( arc.GetWidth() );
    KIGFX::VIEW_OVERLAY::Arc( arc.GetCenter(), radius, start_angle, start_angle + angle );

    KIGFX::VIEW_OVERLAY::SetStrokeColor( prevStrokeCol );
}

void PNS_LOG_VIEWER_OVERLAY::DrawAnnotations()
{
    m_labelMgr->Redraw( this );
}


PNS_LOG_VIEWER_FRAME::PNS_LOG_VIEWER_FRAME( wxFrame* frame ) :
        PNS_LOG_VIEWER_FRAME_BASE( frame ), m_rewindIter( 0 )
{
    LoadSettings();
    createView( m_mainSplitter, PCB_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );

    m_reporter.reset( new WX_TEXT_CTRL_REPORTER( m_consoleText ) );
    m_galPanel->SetParent( m_mainSplitter );
    m_mainSplitter->SplitHorizontally( m_galPanel.get(), m_panelProps );

    Layout();

    Show( true );
    Maximize();
    Raise();

    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_galPanel->GetView()->GetPainter()->GetSettings() );


    PCB_DISPLAY_OPTIONS opts;

    opts.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE;

    double opacity = 0.5;

    opts.m_TrackOpacity = opacity;     ///< Opacity override for all tracks
    opts.m_ViaOpacity = opacity;       ///< Opacity override for all types of via
    opts.m_PadOpacity = opacity;       ///< Opacity override for SMD pads and PTHs
    opts.m_ZoneOpacity = opacity;      ///< Opacity override for filled zone areas

    settings->LoadDisplayOptions( opts );


    m_listPopupMenu = new wxMenu( wxT( "" ) );
    m_listPopupMenu->Append( ID_LIST_COPY, wxT( "Copy selected geometry" ), wxT( "" ),
                             wxITEM_NORMAL );
    m_listPopupMenu->Append( ID_LIST_SHOW_ALL, wxT( "Show all" ), wxT( "" ), wxITEM_NORMAL );
    m_listPopupMenu->Append( ID_LIST_SHOW_NONE, wxT( "Show none" ), wxT( "" ), wxITEM_NORMAL );
    m_listPopupMenu->Append( ID_LIST_DISPLAY_LINE, wxT( "Go to line in IDE" ), wxT( "" ), wxITEM_NORMAL );

    m_itemList->Connect( m_itemList->GetId(), wxEVT_TREELIST_ITEM_CONTEXT_MENU,
                         wxMouseEventHandler( PNS_LOG_VIEWER_FRAME::onListRightClick ), nullptr,
                         this );
    //m_itemList->Connect(m_itemList->GetId(),wxEVT_LISTBOX,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListSelect),nullptr,this);
    m_itemList->Connect( m_itemList->GetId(), wxEVT_TREELIST_SELECTION_CHANGED,
                         wxCommandEventHandler( PNS_LOG_VIEWER_FRAME::onListSelect ),
                         nullptr, this );
    m_itemList->Connect( m_itemList->GetId(), wxEVT_TREELIST_ITEM_CHECKED,
                         wxCommandEventHandler( PNS_LOG_VIEWER_FRAME::onListChecked ),
                         nullptr, this );

    m_itemList->AppendColumn( "Type" );
    m_itemList->AppendColumn( "Value" );
    m_itemList->AppendColumn( "File" );
    m_itemList->AppendColumn( "Method" );
    m_itemList->AppendColumn( "Line" );
    m_itemList->AppendColumn( "VCount" );
    m_itemList->AppendColumn( "Non-45" );

    m_overlay.reset( new PNS_LOG_VIEWER_OVERLAY ( m_galPanel->GetGAL() ) );
    m_galPanel->GetView()->Add( m_overlay.get() );
    m_galPanel->GetViewControls()->EnableCursorWarping(false);

    for( PCB_LAYER_ID layer : LSET::AllNonCuMask().Seq() )
        m_galPanel->GetView()->SetLayerVisible( layer, false );
}


PNS_LOG_VIEWER_FRAME::~PNS_LOG_VIEWER_FRAME()
{
    m_board = nullptr;
    m_logPlayer = nullptr;
    m_logFile = nullptr;
    m_overlay = nullptr;
}


void PNS_LOG_VIEWER_FRAME::createUserTools()
{

}


PNS_DEBUG_STAGE* PNS_LOG_VIEWER_FRAME::getCurrentStage()
{
    PNS_TEST_DEBUG_DECORATOR* dbgd = m_logPlayer->GetDebugDecorator();
    int                       count = dbgd->GetStageCount();

    int iter = m_rewindIter;

    if( count <= 0 )
        return nullptr;

    if( iter < 0 )
        iter = 0;

    if( iter >= count )
        iter = count - 1;

    return dbgd->GetStage( iter );
}


void PNS_LOG_VIEWER_FRAME::drawSimpleShape( SHAPE* aShape, bool aIsSelected, const std::string& aName )
{
    switch( aShape->Type() )
    {
    case SH_CIRCLE:
    {
        auto cir = static_cast<SHAPE_CIRCLE*>( aShape );
        m_overlay->Circle( cir->GetCenter(), cir->GetRadius() );

        break;
    }
    case SH_SEGMENT:
    {
        auto seg = static_cast<SHAPE_SEGMENT*>( aShape );
        m_overlay->Line( seg->GetSeg().A, seg->GetSeg().B );

        break;
    }
    case SH_RECT:
    {
        auto rect = static_cast<SHAPE_RECT*>( aShape );
        m_overlay->Rectangle( rect->GetPosition(), rect->GetPosition() + rect->GetSize() );

        break;
    }
    case SH_LINE_CHAIN:
    {
        auto lc = static_cast<SHAPE_LINE_CHAIN*>( aShape );
        m_overlay->AnnotatedPolyline( *lc, aName, m_showVertices ||  aIsSelected );

        break;
    }
    default: break;
    }
}


void PNS_LOG_VIEWER_FRAME::drawLoggedItems( int iter )
{
    if( !m_logPlayer )
        return;

    PNS_DEBUG_STAGE* st = getCurrentStage();

    if( !st )
        return;

    m_overlay.reset( new PNS_LOG_VIEWER_OVERLAY ( m_galPanel->GetGAL() ) );
    m_galPanel->GetView()->Add( m_overlay.get() );
    //m_galPanel->GetGAL()->EnableDepthTest( false );

    auto drawShapes = [&]( PNS_DEBUG_SHAPE* ent ) -> bool
    {
        bool isEnabled = ent->IsVisible();
        bool isSelected = ent->m_selected;

        if( m_searchString.Length() > 0 )
            isEnabled = ent->m_filterMatch;

        if( !isEnabled )
            return true;

        for( auto& sh : ent->m_shapes )
        {
            COLOR4D color = ent->m_color;
            int lineWidth = ent->m_width;

            m_overlay->SetIsStroke( true );
            m_overlay->SetIsFill( false );

            if( isSelected )
            {
                color.Brighten( 0.5 );
            }

            color.a = 1.0;

            m_overlay->SetStrokeColor( color );
            m_overlay->SetLineWidth( m_showThinLines ? 10000 : ent->m_width );

            if( sh->Type() == SH_COMPOUND )
            {
                auto cmpnd = static_cast<SHAPE_COMPOUND*>( sh );

                for( auto subshape : cmpnd->Shapes() )
                {
                    drawSimpleShape( subshape, isSelected, ent->m_name.ToStdString() );
                }
            }
            else
            {
                drawSimpleShape( sh, isSelected, ent->m_name.ToStdString() );
            }
        }

        return true;
    };

    st->m_entries->IterateTree( drawShapes );

    m_overlay->DrawAnnotations();

    m_galPanel->GetView()->MarkDirty();
    m_galPanel->GetParent()->Refresh();
}


void PNS_LOG_VIEWER_FRAME::LoadLogFile( const wxString& aFile )
{
    std::unique_ptr<PNS_LOG_FILE> logFile( new PNS_LOG_FILE );

    wxFileName logFn( aFile );
    logFn.MakeAbsolute();

    if( logFile->Load( logFn, m_reporter.get() ) )
        SetLogFile( logFile.release() );
}


void PNS_LOG_VIEWER_FRAME::updateViewerIface()
{
    m_viewerIface = std::make_shared<PNS_VIEWER_IFACE>( m_board );
}


void PNS_LOG_VIEWER_FRAME::SetLogFile( PNS_LOG_FILE* aLog )
{
    m_logPlayer.reset( new PNS_LOG_PLAYER );
    m_board = nullptr;
    m_logFile.reset( aLog );

    SetBoard( m_logFile->GetBoard() );
    updateViewerIface();
    m_logPlayer->ReplayLog( m_logFile.get(), 0, 0, -1, true );

    auto dbgd = m_logPlayer->GetDebugDecorator();
    int  n_stages = dbgd->GetStageCount();
    m_rewindSlider->SetMax( n_stages - 1 );
    m_rewindSlider->SetValue( n_stages - 1 );
    m_rewindIter = n_stages - 1;

    auto extents = m_board->GetBoundingBox();


    BOX2D bbd;
    bbd.SetOrigin( extents.GetOrigin() );
    bbd.SetWidth( extents.GetWidth() );
    bbd.SetHeight( extents.GetHeight() );
    bbd.Inflate( std::min( bbd.GetWidth(), bbd.GetHeight() ) / 5 );

    m_galPanel->GetView()->SetViewport( bbd );

    drawLoggedItems( m_rewindIter );
    updateDumpPanel( m_rewindIter );
    updatePnsPreviewItems( m_rewindIter );

}


void PNS_LOG_VIEWER_FRAME::SetBoard2( std::shared_ptr<BOARD> aBoard )
{
    SetBoard( aBoard );
    updateViewerIface();
    auto extents = m_board->GetBoundingBox();

    BOX2D bbd;
    bbd.SetOrigin( extents.GetOrigin() );
    bbd.SetWidth( extents.GetWidth() );
    bbd.SetHeight( extents.GetHeight() );
    bbd.Inflate( std::min( bbd.GetWidth(), bbd.GetHeight() ) / 5 );

    m_galPanel->GetView()->SetViewport( bbd );
}


void PNS_LOG_VIEWER_FRAME::onOpen( wxCommandEvent& event )
{
    wxFileDialog dlg( this, "Select Log File", m_mruPath, wxEmptyString,
                      "PNS log files" + AddFileExtListToFilter( { "log" } ),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        wxString logPath = dlg.GetPath();
        LoadLogFile( logPath );
        m_mruPath = wxFileName( logPath ).GetPath();
    }
}


void PNS_LOG_VIEWER_FRAME::onSaveAs( wxCommandEvent& event )
{
    if( !m_logFile )
    {
        DisplayErrorMessage( this, wxT( "No log file Loaded!" ) );
        return;
    }

    wxFileDialog dlg( this, "New log file", m_mruPath, wxEmptyString,
                      "PNS log files" + AddFileExtListToFilter( { "log" } ),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        // Enforce the extension, wxFileDialog is inept.
        wxFileName create_me = EnsureFileExtension( dlg.GetPath(), "log" );

        wxASSERT_MSG( create_me.IsAbsolute(), wxS( "wxFileDialog returned non-absolute path" ) );

        m_logFile->SaveLog( create_me, m_reporter.get() );
        m_mruPath = create_me.GetPath();
    }

}


void PNS_LOG_VIEWER_FRAME::onExit( wxCommandEvent& event )
{
    Close();
}


void PNS_LOG_VIEWER_FRAME::onListChecked( wxCommandEvent& event )
{
    syncModel();
    drawLoggedItems( m_rewindIter );
}

void PNS_LOG_VIEWER_FRAME::onShowThinLinesChecked( wxCommandEvent& event )
{
    m_showThinLines = event.GetInt();
    drawLoggedItems( m_rewindIter );
    updatePnsPreviewItems( m_rewindIter );
}

void PNS_LOG_VIEWER_FRAME::onShowRPIsChecked( wxCommandEvent& event )
{
    m_showRPIs = event.GetInt();
    drawLoggedItems( m_rewindIter );
    updatePnsPreviewItems( m_rewindIter );
}

void PNS_LOG_VIEWER_FRAME::onShowVerticesChecked( wxCommandEvent& event )
{
    m_showVertices = event.GetInt();
    drawLoggedItems( m_rewindIter );
    updatePnsPreviewItems( m_rewindIter );
}


void PNS_LOG_VIEWER_FRAME::onRewindScroll( wxScrollEvent& event )
{
    m_rewindIter = event.GetPosition();
    drawLoggedItems( m_rewindIter );
    updateDumpPanel( m_rewindIter );
    updatePnsPreviewItems( m_rewindIter );
    m_rewindPos->SetValue( std::to_string( m_rewindIter ) );
    event.Skip();
}


void PNS_LOG_VIEWER_FRAME::onBtnRewindLeft( wxCommandEvent& event )
{
    if( m_rewindIter > 0 )
    {
        m_rewindIter--;
        drawLoggedItems( m_rewindIter );
        updateDumpPanel( m_rewindIter );
        updatePnsPreviewItems( m_rewindIter );
        m_rewindPos->SetValue( std::to_string( m_rewindIter ) );
        m_rewindSlider->SetValue( m_rewindIter );
    }
}


void PNS_LOG_VIEWER_FRAME::onBtnRewindRight( wxCommandEvent& event )
{
    auto dbgd = m_logPlayer->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    if( m_rewindIter < count )
    {
        m_rewindIter++;
        drawLoggedItems( m_rewindIter );
        updateDumpPanel( m_rewindIter );
        updatePnsPreviewItems( m_rewindIter );
        m_rewindPos->SetValue( std::to_string( m_rewindIter ) );
        m_rewindSlider->SetValue( m_rewindIter );
    }
}

void PNS_LOG_VIEWER_FRAME::onFilterText( wxCommandEvent& event )
{
    m_searchString = m_filterString->GetValue();
    updateDumpPanel( m_rewindIter );
}


void PNS_LOG_VIEWER_FRAME::onRewindCountText( wxCommandEvent& event )
{
    if( !m_logPlayer )
        return;

    int val = wxAtoi( m_rewindPos->GetValue() );

    auto dbgd = m_logPlayer->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    if( val < 0 )
        val = 0;

    if( val >= count )
        val = count - 1;

    m_rewindIter = val;
    m_rewindSlider->SetValue( m_rewindIter );
    drawLoggedItems( m_rewindIter );
    updateDumpPanel( m_rewindIter );
    updatePnsPreviewItems( m_rewindIter );

    event.Skip();
}


void PNS_LOG_VIEWER_FRAME::syncModel()
{
    for( wxTreeListItem item = m_itemList->GetFirstItem(); item.IsOk();
         item = m_itemList->GetNextItem( item ) )
    {
        WX_SHAPE_TREE_ITEM_DATA* idata =
                static_cast<WX_SHAPE_TREE_ITEM_DATA*>( m_itemList->GetItemData( item ) );

        if( idata )
        {
            bool checked = m_itemList->GetCheckedState( item ) == wxCHK_CHECKED;
            bool selected = m_itemList->IsSelected( item );
            idata->m_item->m_visible = checked || selected;
            idata->m_item->m_selected = selected;
        }
    }
}


void runCommand( const wxString& aCommand )
{
#ifdef __WXMSW__
    wxShell( aCommand ); // on windows we need to launch a shell in order to run the command
#else
    wxExecute( aCommand );
#endif /* __WXMSW__ */
}


void PNS_LOG_VIEWER_FRAME::onListRightClick( wxMouseEvent& event )
{
    auto sel = m_itemList->GetPopupMenuSelectionFromUser( *m_listPopupMenu );

    switch( sel )
    {
    case ID_LIST_SHOW_NONE:
        m_itemList->CheckItemRecursively( m_itemList->GetRootItem(), wxCHK_UNCHECKED );
        syncModel();
        drawLoggedItems( m_rewindIter );
        break;
    case ID_LIST_SHOW_ALL:
        m_itemList->CheckItemRecursively( m_itemList->GetRootItem(), wxCHK_CHECKED );
        syncModel();
        drawLoggedItems( m_rewindIter );
        break;
    case ID_LIST_COPY:
    {
        wxString s;

        PNS_DEBUG_STAGE* st = getCurrentStage();

        if( !st )
            return;

        auto formatShapes = [&]( PNS_DEBUG_SHAPE* ent ) -> bool
        {
            if( ent->m_selected )
            {
                for( auto sh : ent->m_shapes )
                {
                    s += "// " + ent->m_name + "\n " + sh->Format() + "; \n";
                }
            }

            return true;
        };

        st->m_entries->IterateTree( formatShapes );

        if( wxTheClipboard->Open() )
        {
            // This data objects are held by the clipboard,
            // so do not delete them in the app.
            wxTheClipboard->SetData( new wxTextDataObject( s ) );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }

        return;
    }
    case ID_LIST_DISPLAY_LINE:
    {
        wxVector<wxTreeListItem> selectedItems;

        if( m_itemList->GetSelections( selectedItems ) == 1 )
        {
            wxString filename = m_itemList->GetItemText(selectedItems.back(), 2);
            wxString line = m_itemList->GetItemText(selectedItems.back(), 4);


            if( !filename.empty() && !line.empty() )
            {
                wxString filepath = m_filenameToPathMap[filename];

                switch( m_ideChoice->GetCurrentSelection() )
                {
                    case 0: runCommand( wxString::Format( "code --goto %s:%s", filepath, line ) ); return;
                    case 1: runCommand( wxString::Format( "start devenv /edit %s /command \"Gotoln %s\"", filepath, line ) ); return; // fixme
                    case 2: runCommand( wxString::Format( "clion --line %s %s", line, filepath ) ); return;
                    case 3: runCommand( wxString::Format( "emacsclient +%s %s", line, filepath ) ); return;
                    default: return;
                }
            }
        }
        break;
    }
    }
    return;
}


void PNS_LOG_VIEWER_FRAME::onListSelect( wxCommandEvent& event )
{
    syncModel();
    drawLoggedItems( m_rewindIter );
}



static bool isLine45Degree( const SHAPE_LINE_CHAIN* lc )
{
    for( int i = 0; i < lc->SegmentCount(); i++ )
    {
        const SEG& s = lc->CSegment( i );

        if( lc->IsArcSegment( i ) )
            continue;

        if( s.Length() < 10 )
            continue;

        double angle = 180.0 / M_PI *
                       atan2( (double) s.B.y - (double) s.A.y,
                              (double) s.B.x - (double) s.A.x );

        if( angle < 0 )
            angle += 360.0;

        double angle_a = fabs( fmod( angle, 45.0 ) );

        if( angle_a > 1.0 && angle_a < 44.0 )
            return false;
    }

    return true;
}


bool PNS_LOG_VIEWER_FRAME::filterStringMatches( PNS_DEBUG_SHAPE* ent )
{

    std::set<PNS_DEBUG_SHAPE*> processed;
    std::deque<PNS_DEBUG_SHAPE*> q;

    q.push_back(ent);
    int total = 0;
    while ( q.size() > 0 )
    {
        PNS_DEBUG_SHAPE* top = q.front();

        q.pop_front();

        for ( auto chld : top->m_children )
        {
            bool match = m_searchString.Length() == 0 ? true : false;
            //printf("CHK %s\n", (const char *) chld->m_name.c_str() );
            chld->m_filterMatch = false;
            if ( chld->m_name.Contains( m_searchString ) )
                match = true;
            if ( chld->m_msg.Contains( m_searchString ) )
                match = true;

            if( match )
            {
                for ( PNS_DEBUG_SHAPE *cur = chld; cur; cur = cur->m_parent )
                    cur->m_filterMatch = match;
            }

            if( processed.find(chld) == processed.end() )
            {
                q.push_back( chld );
                processed.insert( chld );
            }
        }
        total++;
    }

    printf("total: %d\n", total );

    return false;
}


void PNS_LOG_VIEWER_FRAME::buildListTree( wxTreeListItem item,
                                          PNS_DEBUG_SHAPE* ent, int depth )
{
#ifdef EXTRA_VERBOSE
    for( int i = 0; i < depth * 2; i++ )
        printf( " " );

    if( ent->m_msg.length() )
        printf( "MSG: %s\n", ent->m_msg.c_str() );
    else
        printf( "SHAPES: %s [%d]\n", ent->m_name.c_str(), ent->m_children.size() );
#endif

    wxTreeListItem ritem;

    if( !ent->m_filterMatch )
        return;

    if( ent->m_msg.length() )
    {
        ritem = m_itemList->AppendItem( item, "Child" );
        m_itemList->SetItemText( ritem, 0, "Message" );
        m_itemList->SetItemText( ritem, 1, ent->m_msg );
    }
    else
    {
        ritem = m_itemList->AppendItem( item, "Child" );
        int n_verts = 0;
        for(auto sh : ent->m_shapes )
        {
            if ( sh->Type() == SH_LINE_CHAIN )
            {
                n_verts += static_cast<const SHAPE_LINE_CHAIN*>( sh )->PointCount();
            }
        }
        m_itemList->SetItemText( ritem, 0, wxString::Format( "Shapes [%d verts]", n_verts ) );
        m_itemList->SetItemText( ritem, 1, ent->m_name );
    }

    wxString fullfilepath = ent->m_srcLoc.fileName;
    wxString filename = wxFileNameFromPath( fullfilepath );

    if( !filename.empty() )
        m_filenameToPathMap.insert( { filename, fullfilepath } );

    m_itemList->SetItemText( ritem, 2, filename );
    m_itemList->SetItemText( ritem, 3, ent->m_srcLoc.funcName );
    m_itemList->SetItemText( ritem, 4, wxString::Format("%d", ent->m_srcLoc.line ) );

    int  totalVC = 0, totalVCSimplified = 0;
    bool is45Degree = true;

    for( SHAPE* sh : ent->m_shapes )
    {
        if( sh->Type() == SH_LINE_CHAIN )
        {
            auto lc = static_cast<SHAPE_LINE_CHAIN*>( sh );

            totalVC += lc->PointCount();

            SHAPE_LINE_CHAIN simp(*lc);

            simp.Simplify();

            totalVCSimplified += simp.PointCount();

            if( !isLine45Degree( lc ) )
                is45Degree = false;
        }
    }

    if( totalVC > 0 )
        m_itemList->SetItemText( ritem, 5, wxString::Format( "%d [%d]", totalVC, totalVCSimplified ) );

    if( !is45Degree )
        m_itemList->SetItemText( ritem, 6, wxT("") );

    m_itemList->SetItemData( ritem, new WX_SHAPE_TREE_ITEM_DATA( ent, depth ) );

    if( !ent->m_children.size() )
        return;

    for( auto child : ent->m_children )
    {
        buildListTree( ritem, child, depth + 1 );
    }
}


static void expandAllChildren( wxTreeListCtrl* tree, int maxLevel = -1 )
{
    wxTreeListItem child = tree->GetFirstItem ();

    while( child.IsOk() )
    {
        WX_SHAPE_TREE_ITEM_DATA* idata =
                static_cast<WX_SHAPE_TREE_ITEM_DATA*>( tree->GetItemData( child ) );

        if( maxLevel < 0 || idata->m_level <= maxLevel )
            tree->Expand ( child );
        else
            tree->Collapse ( child );
        child = tree->GetNextItem( child );
    }
}

static void collapseAllChildren( wxTreeListCtrl* tree )
{
    wxTreeListItem child = tree->GetFirstItem ();

    while( child.IsOk() )
    {
        tree->Collapse ( child );
        child = tree->GetNextItem( child );
    }
}


void PNS_LOG_VIEWER_FRAME::updateDumpPanel( int iter )
{
    printf("UpdateDUmp %d\n", iter );
    if( !m_logPlayer )
        return;

    auto dbgd = m_logPlayer->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    wxArrayString dumpStrings;

    if( count <= 0 )
        return;

    if( iter < 0 )
        iter = 0;

    if( iter >= count )
        iter = count - 1;

    auto st = dbgd->GetStage( iter );

    if( st->m_status )
    {
        m_algoStatus->SetLabel("OK");
        m_algoStatus->SetForegroundColour( wxColor(*wxGREEN));
    }
    else
    {
        m_algoStatus->SetLabel("FAIL");
        m_algoStatus->SetForegroundColour( wxColor(*wxRED));
    }

    auto rootItem = m_itemList->GetRootItem();

    m_itemList->DeleteAllItems();
    filterStringMatches( st->m_entries );
    buildListTree( rootItem, st->m_entries );
    m_itemList->CheckItemRecursively( rootItem, wxCHK_UNCHECKED );

    expandAllChildren( m_itemList, 0 );

    m_itemList->Refresh();
}

void PNS_LOG_VIEWER_FRAME::updatePnsPreviewItems( int iter )
{
    auto viewTracker = m_logPlayer->GetViewTracker();
    PNS_LOG_VIEW_TRACKER::VIEW_ENTRIES& entries = viewTracker->GetEntriesForStage( iter );
    auto view = m_galPanel->GetView();
    printf("DBG updatePnsPreviewItems: %zu items\n", entries.size() );

    m_previewItems.reset( new KIGFX::VIEW_GROUP( m_galPanel->GetView() ) );
    m_galPanel->GetView()->Add( m_previewItems.get() );
    m_previewItems->SetLayer( LAYER_SELECT_OVERLAY ) ;
    m_galPanel->GetView()->SetLayerVisible( LAYER_SELECT_OVERLAY );

    if( !m_showRPIs )
        return;

    for( auto& ent : entries )
    {
        if ( ent.m_isHideOp )
        {

            auto parent = ent.m_item->Parent();
            if( parent )
            {

                view->Hide( parent );
            }
        }
        else
        {
            ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( ent.m_item, m_viewerIface.get(), view );
            pitem->Update( ent.m_item );
            m_previewItems->Add(pitem);
    //        printf("DBG vadd %p total %d\n", pitem, m_previewItems->GetSize() );
        }
    }

    view->SetVisible( m_previewItems.get(), true );

    view->Update( m_previewItems.get() );
    printf("DBG vgrp %p total %d\n", m_previewItems.get(), m_previewItems->GetSize() );


    //view->UpdateAllItems( KIGFX::ALL );
}

REPORTER* PNS_LOG_VIEWER_FRAME::GetConsoleReporter()
{
    return m_reporter.get();
}


#if 0

static BOARD* loadBoard( const std::string& filename )
{
    IO_RELEASER<PCB_IO> pi( new PCB_IO_KICAD_SEXPR );
    BOARD*              brd = nullptr;

    try
    {
        brd = pi->LoadBoard( wxString( filename.c_str() ), nullptr, nullptr );
    }
    catch( const IO_ERROR& )
    {
        return nullptr;
    }

    return brd;
}





int render_perftest_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );

    //  drcCreateTestsProviderClearance();
    //  drcCreateTestsProviderEdgeClearance();

    if( argc >= 2 && std::string( argv[1] ) == "-h" )
    {
        printf( "PCB render performance test. Just renders a board without UI update overhead.\n" );
        return 0;
    }

    if( argc < 2 )
    {
        printf( "Expected parameters: board_file\n" );
        return 0;
    }

    PROF_TIMER             cnt("load-board");
    std::shared_ptr<BOARD> brd ( loadBoard( argv[1] ) );
    cnt.Stop();

    KI_TRACE( traceGalProfile, "%s\n", cnt.to_string() );

    frame->SetBoard2( brd );

    return 0;
}


static bool registered3 = UTILITY_REGISTRY::Register( {
        "render_perftest",
        "Renderer performance test",
        render_perftest_main_func,
} );


VECTOR2I NearestPointFixpt( SEG seg, const VECTOR2I& aP )
{
    VECTOR2I d = seg.B - seg.A;
    SEG::ecoord   l_squared = d.Dot( d );

    if( l_squared == 0 )
        return seg.A;

    SEG::ecoord t = d.Dot( aP - seg.A );

    if( t < 0 )
        return seg.A;
    else if( t > l_squared )
        return seg.B;

    int xp = rescale( t, (SEG::ecoord) d.x, l_squared );
    int yp = rescale( t, (SEG::ecoord) d.y, l_squared );

    return seg.A + VECTOR2I( xp, yp );
}


VECTOR2D NearestPointDbl( SEG seg, const VECTOR2I& aP )
{
    VECTOR2D d = seg.B - seg.A;
    double l_squared = d.Dot(d);

    if( l_squared == 0 )
        return seg.A;

    double t = d.Dot(VECTOR2D( aP - seg.A ) );

    if( t < 0 )
        return seg.A;
    else if( t > l_squared )
        return seg.B;

    double xp = t * d.x / l_squared;
    double yp = t * d.y / l_squared;

    return VECTOR2D(seg.A) + VECTOR2D( xp, yp );
}

int ttt_main_func( int argc, char* argv[] )
{
    int n = 1000000;
    std::vector<VECTOR2I> pts;
    std::vector<SEG> segs;
    std::vector<VECTOR2D> rv;
    std::vector<VECTOR2I> rvi;


    rv.resize(n);
    rvi.resize(n);

    for (int i = 0; i < n ;i++)
    {
        pts.push_back(VECTOR2I(random()%100000000, random()%100000000));
        segs.push_back(SEG( VECTOR2I(random()%100000000, random()%100000000), VECTOR2I(random()%100000000, random()%100000000) ) );
    }

    PROF_TIMER tmrFix("nearest-fixpt");
    for(int i = 0; i < n ; i++)
    {
        rvi[i] = NearestPointFixpt( segs[i], pts[i]);
    }
    tmrFix.Show();

    PROF_TIMER tmrDbl("nearest-double");
    for(int i = 0; i < n ; i++)
    {
        rv[i] = NearestPointDbl( segs[i], pts[i]);
    }
    tmrDbl.Show();
    return 0;
}



static bool registered4 = UTILITY_REGISTRY::Register( {
        "ttt",
        "Renderer performance test",
        ttt_main_func,
} );

#endif
