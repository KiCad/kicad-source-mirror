/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2022 KiCad Developers.
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

#include <wx/clipbrd.h>

#include <qa_utils/utility_registry.h>
#include <pgm_base.h>

#include <profile.h>
#include <trace_helpers.h>

#include <view/view_overlay.h>

#include "pns_log.h"
#include "router/pns_diff_pair.h"
#include "router/pns_utils.h"

#include "pns_log_viewer_frame.h"

#include "qa/drc_proto/drc_proto.h"

#include "label_manager.h"
#define TOM_EXTRA_DEBUG



class WX_SHAPE_TREE_ITEM_DATA : public wxClientData
{
public:
    WX_SHAPE_TREE_ITEM_DATA( PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* item ) : m_item( item ){};

    PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* m_item;
};


PNS_LOG_VIEWER_OVERLAY::PNS_LOG_VIEWER_OVERLAY( KIGFX::GAL* aGal )
{
    m_labelMgr.reset( new LABEL_MANAGER( aGal ) );
}


void PNS_LOG_VIEWER_OVERLAY::AnnotatedPolyline( const SHAPE_LINE_CHAIN& aL, std::string name,
                                                bool aShowVertexNumbers )
{
    Polyline( aL );

    if( aShowVertexNumbers)
        m_labelMgr->Add( aL, GetStrokeColor() );
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


PNS_LOG_VIEWER_FRAME::PNS_LOG_VIEWER_FRAME( wxFrame* frame ) : PNS_LOG_VIEWER_FRAME_BASE( frame )
{
    LoadSettings();
    createView( this, PCB_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );

    m_viewSizer->Add( m_galPanel.get(), 1, wxEXPAND, 5 );

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

    m_overlay.reset( new PNS_LOG_VIEWER_OVERLAY ( m_galPanel->GetGAL() ) );
    m_galPanel->GetView()->Add( m_overlay.get() );
}


PNS_LOG_VIEWER_FRAME::~PNS_LOG_VIEWER_FRAME()
{
    m_overlay = nullptr;
}


void PNS_LOG_VIEWER_FRAME::createUserTools()
{

}


PNS_TEST_DEBUG_DECORATOR::STAGE* PNS_LOG_VIEWER_FRAME::getCurrentStage()
{
    PNS_TEST_DEBUG_DECORATOR* dbgd = m_env->GetDebugDecorator();
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


void PNS_LOG_VIEWER_FRAME::drawLoggedItems( int iter )
{
    if( !m_env )
        return;

    PNS_TEST_DEBUG_DECORATOR::STAGE* st = getCurrentStage();

    if( !st )
        return;

    m_overlay.reset( new PNS_LOG_VIEWER_OVERLAY ( m_galPanel->GetGAL() ) );
    m_galPanel->GetView()->Add( m_overlay.get() );

    auto drawShapes = [&]( PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* ent ) -> bool
    {
        bool isEnabled = ent->IsVisible();
        bool isSelected = false;

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

            m_overlay->SetStrokeColor( color );
            m_overlay->SetLineWidth( ent->m_width );

            switch( sh->Type() )
            {
            case SH_CIRCLE:
            {
                auto cir = static_cast<SHAPE_CIRCLE*>( sh );
                m_overlay->Circle( cir->GetCenter(), cir->GetRadius() );

                break;
            }
            case SH_SEGMENT:
            {
                auto seg = static_cast<SHAPE_SEGMENT*>( sh );
                m_overlay->Line( seg->GetSeg().A, seg->GetSeg().B );
                
                break;
            }
            case SH_RECT:
            {
                auto rect = static_cast<SHAPE_RECT*>( sh );
                m_overlay->Rectangle( rect->GetPosition(), rect->GetPosition() + rect->GetSize() );

                break;
            }
            case SH_LINE_CHAIN:
            {
                auto lc = static_cast<SHAPE_LINE_CHAIN*>( sh );
                m_overlay->AnnotatedPolyline( *lc, ent->m_name.ToStdString(), ent->m_hasLabels && isSelected );

                break;

            }
            default:
                break;
            }
        }

        return true;
    };

    st->m_entries->IterateTree( drawShapes );

    m_overlay->DrawAnnotations();

    m_galPanel->GetView()->MarkDirty();
    m_galPanel->GetParent()->Refresh();
}


static BOARD* loadBoard( const std::string& filename )
{
    PLUGIN::RELEASER pi( new PCB_PLUGIN );
    BOARD*           brd = nullptr;

    try
    {
        brd = pi->Load( wxString( filename.c_str() ), nullptr, nullptr );
    }
    catch( const IO_ERROR& )
    {
        return nullptr;
    }

    return brd;
}


void PNS_LOG_VIEWER_FRAME::SetLogFile( PNS_LOG_FILE* aLog )
{
    m_logFile.reset( aLog );

    SetBoard( m_logFile->GetBoard() );

    m_env.reset( new PNS_TEST_ENVIRONMENT );

    m_env->SetMode( PNS::PNS_MODE_ROUTE_SINGLE );
    m_env->ReplayLog( m_logFile.get(), 0, 0, -1);

    auto dbgd = m_env->GetDebugDecorator();
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
}



void PNS_LOG_VIEWER_FRAME::SetBoard2( std::shared_ptr<BOARD> aBoard )
{
    SetBoard( aBoard );

    auto extents = m_board->GetBoundingBox();

    BOX2D bbd;
    bbd.SetOrigin( extents.GetOrigin() );
    bbd.SetWidth( extents.GetWidth() );
    bbd.SetHeight( extents.GetHeight() );
    bbd.Inflate( std::min( bbd.GetWidth(), bbd.GetHeight() ) / 5 );

    m_galPanel->GetView()->SetViewport( bbd );
}

void PNS_LOG_VIEWER_FRAME::onReload( wxCommandEvent& event )
{
    event.Skip();
}

void PNS_LOG_VIEWER_FRAME::onExit( wxCommandEvent& event )
{
    event.Skip();
}


void PNS_LOG_VIEWER_FRAME::onListChecked( wxCommandEvent& event )
{
    syncModel();
    drawLoggedItems( m_rewindIter );
}


void PNS_LOG_VIEWER_FRAME::onRewindScroll( wxScrollEvent& event )
{
    m_rewindIter = event.GetPosition();
    drawLoggedItems( m_rewindIter );
    updateDumpPanel( m_rewindIter );
    char str[128];
    sprintf( str, "%d", m_rewindIter );
    m_rewindPos->SetValue( str );
    event.Skip();
}


void PNS_LOG_VIEWER_FRAME::onBtnRewindLeft( wxCommandEvent& event )
{
    if( m_rewindIter > 0 )
    {
        m_rewindIter--;
        drawLoggedItems( m_rewindIter );
        updateDumpPanel( m_rewindIter );
        char str[128];
        sprintf( str, "%d", m_rewindIter );
        m_rewindPos->SetValue( str );
    }
}


void PNS_LOG_VIEWER_FRAME::onBtnRewindRight( wxCommandEvent& event )
{
    auto dbgd = m_env->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    if( m_rewindIter < count )
    {
        m_rewindIter++;
        drawLoggedItems( m_rewindIter );
        updateDumpPanel( m_rewindIter );
        char str[128];
        sprintf( str, "%d", m_rewindIter );
        m_rewindPos->SetValue( str );
    }
}


void PNS_LOG_VIEWER_FRAME::onRewindCountText( wxCommandEvent& event )
{
    if( !m_env )
        return;

    int val = wxAtoi( m_rewindPos->GetValue() );

    auto dbgd = m_env->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    if( val < 0 )
        val = 0;

    if( val >= count )
        val = count - 1;

    m_rewindIter = val;
    m_rewindSlider->SetValue( m_rewindIter );
    drawLoggedItems( m_rewindIter );
    updateDumpPanel( m_rewindIter );

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

        PNS_TEST_DEBUG_DECORATOR::STAGE* st = getCurrentStage();

        if( !st )
            return;

        auto formatShapes = [&]( PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* ent ) -> bool
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
    }
}


void PNS_LOG_VIEWER_FRAME::onListSelect( wxCommandEvent& event )
{
    syncModel();
    drawLoggedItems( m_rewindIter );
}


void PNS_LOG_VIEWER_FRAME::buildListTree( wxTreeListItem item,
                                          PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* ent, int depth )
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

    printf("depth %d\n", depth );

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

    m_itemList->SetItemText( ritem, 2, wxFileNameFromPath( ent->m_srcLoc.fileName ) );
    m_itemList->SetItemText( ritem, 3, ent->m_srcLoc.funcName );
    m_itemList->SetItemText( ritem, 4, wxString::Format("%d", ent->m_srcLoc.line ) );

    m_itemList->SetItemData( ritem, new WX_SHAPE_TREE_ITEM_DATA( ent ) );

    if( !ent->m_children.size() )
        return;

    for( auto child : ent->m_children )
    {
        buildListTree( ritem, child, depth + 1 );
    }
}


static void expandAllChildren( wxTreeListCtrl* tree, int maxLevel = 0 )
{
    wxTreeListItem child = tree->GetFirstItem ();

    while( child.IsOk() )
    {
        WX_SHAPE_TREE_ITEM_DATA* idata =
                static_cast<WX_SHAPE_TREE_ITEM_DATA*>( tree->GetItemData( child ) );

        if( idata->m_item->m_level > maxLevel )
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
    if( !m_env )
        return;

    auto dbgd = m_env->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    wxArrayString dumpStrings;

    if( count <= 0 )
        return;

    if( iter < 0 )
        iter = 0;

    if( iter >= count )
        iter = count - 1;

    auto st = dbgd->GetStage( iter );
    auto rootItem = m_itemList->GetRootItem();

    m_itemList->DeleteAllItems();
    buildListTree( rootItem, st->m_entries );
    m_itemList->CheckItemRecursively( rootItem, wxCHK_UNCHECKED );

    collapseAllChildren( m_itemList );

    m_itemList->Refresh();
}


int replay_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );

    //  drcCreateTestsProviderClearance();
    //  drcCreateTestsProviderEdgeClearance();

    if( argc >= 2 && std::string( argv[1] ) == "-h" )
    {
        printf( "PNS Log (Re)player. Allows to step through the log written by the ROUTER_TOOL "
                "in debug KiCad builds. " );
        printf( "Requires a board file with UUIDs and a matching log file. Both are written to "
                "/tmp when you press '0' during routing." );
        return 0;
    }

    if( argc < 3 )
    {
        printf( "Expected parameters: log_file.log board_file.dump\n" );
        return 0;
    }

    PNS_LOG_FILE* logFile = new PNS_LOG_FILE;
    logFile->Load( argv[1], argv[2] );

    frame->SetLogFile( logFile );
    //SetTopFrame( frame );      // wxApp gets a face.

    return 0;
}


static bool registered2 = UTILITY_REGISTRY::Register( {
        "replay",
        "PNS Log Player",
        replay_main_func,
} );



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
