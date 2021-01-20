/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers.
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

#include <pcb_painter.h>
#include <pcb_test_frame.h>
#include <qa_utils/utility_registry.h>
#include <pgm_base.h>

#include <profile.h>

#include <view/view_overlay.h>

#include "pns_log.h"
#include "router/pns_diff_pair.h"
#include "pns_log_viewer_frame_base.h"

#include "qa/drc_proto/drc_proto.h"

#define ID_LIST_COPY 10001
#define ID_LIST_SHOW_ALL 10002
#define ID_LIST_SHOW_NONE 10003

class PNS_LOG_VIEWER_FRAME : public PNS_LOG_VIEWER_FRAME_BASE, public PCB_TEST_FRAME_BASE
{
public:
    PNS_LOG_VIEWER_FRAME( wxFrame* frame ) : PNS_LOG_VIEWER_FRAME_BASE( frame )
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

        settings->SetZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE );

        m_listPopupMenu  = new wxMenu(wxT(""));
        m_listPopupMenu->Append(ID_LIST_COPY, wxT("Copy selected geometry"), wxT(""), wxITEM_NORMAL);
        m_listPopupMenu->Append(ID_LIST_SHOW_ALL, wxT("Show all"), wxT (""), wxITEM_NORMAL);
        m_listPopupMenu->Append(ID_LIST_SHOW_NONE, wxT("Show none"), wxT (""), wxITEM_NORMAL);

        m_itemList->Connect(m_itemList->GetId(),wxEVT_TREELIST_ITEM_CONTEXT_MENU,wxMouseEventHandler(PNS_LOG_VIEWER_FRAME::onListRightClick),NULL,this);
        //m_itemList->Connect(m_itemList->GetId(),wxEVT_LISTBOX,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListSelect),NULL,this);
        m_itemList->Connect(m_itemList->GetId(),wxEVT_TREELIST_SELECTION_CHANGED,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListSelect),NULL,this);
        m_itemList->Connect(m_itemList->GetId(),wxEVT_TREELIST_ITEM_CHECKED,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListChecked),NULL,this);
        
        //Connect(ID_LIST_COPY,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListCopy),NULL,this);
        //Connect(ID_LIST_SHOW_ALL,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListShowAll),NULL,this);
        //Connect(ID_LIST_SHOW_NONE,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListShowNone),NULL,this);
        m_itemList->AppendColumn ( "Type" );
        m_itemList->AppendColumn ( "Value" );
    }

    virtual ~PNS_LOG_VIEWER_FRAME()
    {
    }

    void SetLogFile( PNS_LOG_FILE* aLog );

private:
    void drawLoggedItems( int iter );
    void updateDumpPanel( int iter );
    virtual void createUserTools() override;
    void buildListTree( wxTreeListItem item, PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* ent );
    void syncModel();
    PNS_TEST_DEBUG_DECORATOR::STAGE* getCurrentStage();

    virtual void onReload( wxCommandEvent& event ) override;
    virtual void onExit( wxCommandEvent& event ) override;
    virtual void onRewindScroll( wxScrollEvent& event ) override;
    virtual void onRewindCountText( wxCommandEvent& event ) override;
    virtual void onListRightClick(wxMouseEvent& event);
    virtual void onListShowAll( wxCommandEvent& event );
    virtual void onListShowNone( wxCommandEvent& event );
    virtual void onListCopy( wxCommandEvent& event );
    virtual void onListSelect( wxCommandEvent& event );
    virtual void onBtnRewindLeft( wxCommandEvent& event ) override;
	virtual void onBtnRewindRight( wxCommandEvent& event ) override;
    virtual void onListChecked( wxCommandEvent& event );

    std::shared_ptr<KIGFX::VIEW_OVERLAY>  m_overlay;
    std::shared_ptr<PNS_LOG_FILE>         m_logFile;
    std::shared_ptr<PNS_TEST_ENVIRONMENT> m_env;
    int                                   m_rewindIter;
    wxMenu* m_listPopupMenu;
};


class WX_SHAPE_TREE_ITEM_DATA : public wxClientData
{
    public:
        WX_SHAPE_TREE_ITEM_DATA(PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT *item ) :
            m_item(item) {};

        PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT *m_item;
};

void PNS_LOG_VIEWER_FRAME::createUserTools()
{

}


static const COLOR4D assignColor( int aStyle )
{
    COLOR4D color;

    switch( aStyle )
    {
    case 0:
        color = COLOR4D( 0, 1, 0, 1 );
        break;

    case 1:
        color = COLOR4D( 1, 0, 0, 1 );
        break;

    case 2:
        color = COLOR4D( 1, 1, 0, 1 );
        break;

    case 3:
        color = COLOR4D( 0, 0, 1, 1 );
        break;

    case 4:
        color = COLOR4D( 1, 1, 1, 1 );
        break;

    case 5:
        color = COLOR4D( 1, 1, 0, 1 );
        break;

    case 6:
        color = COLOR4D( 0, 1, 1, 1 );
        break;

    case 32:
        color = COLOR4D( 0, 0, 1, 1 );
        break;

    default:
        color = COLOR4D( 0.4, 0.4, 0.4, 1 );
        break;
    }

    return color;
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

    m_overlay = m_galPanel->DebugOverlay();
    m_overlay->Clear();

    PNS_TEST_DEBUG_DECORATOR::STAGE* st = getCurrentStage();

    if( !st )
        return;

    auto drawShapes = [ & ] ( PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT *ent ) -> bool 
    {
        bool isEnabled = ent->IsVisible();
        bool isSelected = false;
        if (! isEnabled )
            return true;

        for( auto& sh : ent->m_shapes )
        {
            m_overlay->SetIsStroke( true );
            m_overlay->SetIsFill( false );
            m_overlay->SetStrokeColor( assignColor( ent->m_color ) );
            m_overlay->SetLineWidth( ent->m_width );

            switch( sh->Type() )
            {
            case SH_CIRCLE:
            {
                auto cir = static_cast<SHAPE_CIRCLE*>( sh );
                m_overlay->Circle( cir->GetCenter(), cir->GetRadius() );

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

                if( isSelected )
                {
                    m_overlay->SetLineWidth( ent->m_width * 2 );
                    m_overlay->SetStrokeColor( COLOR4D(1.0, 1.0, 1.0, 1.0) );
                }

                for( int i = 0; i < lc->SegmentCount(); i++ )
                {
                    auto s = lc->CSegment( i );
                    m_overlay->Line( s.A, s.B );
                }
                break;
            }
            default:
                break;
            }
        }


        return true;
    };

    st->m_entries->IterateTree( drawShapes );

/*        if ( !m_itemList->IsChecked(itemID) || !ent.m_shapes.size() )
        {
            if( itemID != m_selectedItem )
            {
                itemID++;
                continue;
            }
        }*/

    //m_galPanel->ForceRefresh();

    m_galPanel->GetView()->MarkDirty();
    m_galPanel->GetParent()->Refresh();
}


static BOARD* loadBoard( const std::string& filename )
{
    PLUGIN::RELEASER pi( new PCB_IO );
    BOARD* brd = nullptr;

    try
    {
        brd = pi->Load( wxString( filename.c_str() ), NULL, NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ),
                ioe.Problem() );

        printf( "Board Loading Error: '%s'\n", (const char*) msg.mb_str() );
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
    m_env->ReplayLog( m_logFile.get() );

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
    bbd.Inflate( std::min( bbd.GetWidth(), bbd.GetHeight() )/ 5);

    m_galPanel->GetView()->SetViewport( bbd );

    drawLoggedItems( m_rewindIter );
    updateDumpPanel( m_rewindIter );
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
    printf("On-list-checked\n");
    syncModel();
    drawLoggedItems(m_rewindIter);
}


void PNS_LOG_VIEWER_FRAME::onRewindScroll( wxScrollEvent& event )
{
    m_rewindIter = event.GetPosition();
    drawLoggedItems(m_rewindIter);
    updateDumpPanel(m_rewindIter);
    char str[128];
    sprintf(str,"%d",m_rewindIter);
    m_rewindPos->SetValue( str );
    event.Skip();
}

void PNS_LOG_VIEWER_FRAME::onBtnRewindLeft( wxCommandEvent& event )
{
    if(m_rewindIter > 0)
    {
        m_rewindIter--;
        drawLoggedItems(m_rewindIter);
        updateDumpPanel(m_rewindIter);
        char str[128];
        sprintf(str,"%d",m_rewindIter);
        m_rewindPos->SetValue( str );
    }
}

void PNS_LOG_VIEWER_FRAME::onBtnRewindRight( wxCommandEvent& event )
{
    auto dbgd = m_env->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    if(m_rewindIter < count)
    {
        m_rewindIter++;
        drawLoggedItems(m_rewindIter);
        updateDumpPanel(m_rewindIter);
        char str[128];
        sprintf(str,"%d",m_rewindIter);
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
    printf("SyncModel\n");

    for( wxTreeListItem item = m_itemList->GetFirstItem();
        item.IsOk();
        item = m_itemList->GetNextItem( item )
    )
    {
        WX_SHAPE_TREE_ITEM_DATA* idata = static_cast<WX_SHAPE_TREE_ITEM_DATA*> ( m_itemList->GetItemData( item ) );
        printf("IDATA %p\n", idata);
        if( idata )
        {
            idata->m_item->m_visible = m_itemList->GetCheckedState( item ) == wxCHK_CHECKED;
        }
    }

}

void PNS_LOG_VIEWER_FRAME::onListRightClick(wxMouseEvent& event)
{
    fprintf(stderr,"OnListMenu\n");
    //m_itemList->PopupMenu(m_listPopupMenu);
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
    }
}

void PNS_LOG_VIEWER_FRAME::onListShowAll( wxCommandEvent& event )
{
  
}

void PNS_LOG_VIEWER_FRAME::onListShowNone( wxCommandEvent& event )
{
    m_itemList->CheckItemRecursively( m_itemList->GetRootItem(), wxCHK_UNCHECKED );
  
    drawLoggedItems( m_rewindIter );
}

void PNS_LOG_VIEWER_FRAME::onListCopy( wxCommandEvent& event )
{
    wxString s;

    int sel = 0; //m_itemList->GetSelection();
    auto dbgd = m_env->GetDebugDecorator();
    int  count = dbgd->GetStageCount();

    if( count <= 0 )
        return;

    int iter = m_rewindIter;

    if( iter < 0 )
        iter = 0;

    if( iter >= count )
        iter = count - 1;

    const PNS_TEST_DEBUG_DECORATOR::STAGE* st = dbgd->GetStage( iter );

//    if ( st->m_entries.size() < sel )
 //       return;

    const PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT& ent = st->m_entries[sel];

    for( auto& sh : ent.m_shapes )
    {
        s += sh->Format() + " ";
    }

    if (wxTheClipboard->Open())
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject(s) );
        wxTheClipboard->Close();
    }
}

void PNS_LOG_VIEWER_FRAME::onListSelect( wxCommandEvent& event )
{
    printf("OnListSelect!\n");
    drawLoggedItems( m_rewindIter );
}


void PNS_LOG_VIEWER_FRAME::buildListTree( wxTreeListItem item, PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* ent )
{
        printf("LOG append %p\n", ent );
        
        wxTreeListItem ritem;

        if( ent->m_children.size() )
            ritem = m_itemList->AppendItem( item, "Child" );
        else
            ritem = item;

        //ent->m_item = ritem;

        if( ent->m_msg.length() )
        {
            m_itemList->SetItemText( ritem, 0, "Message" );
            m_itemList->SetItemText( ritem, 1, ent->m_msg );
        }
        else
        {
            m_itemList->SetItemText( ritem, 0, "Shapes" );
            m_itemList->SetItemText( ritem, 1, ent->m_name );
        }

        m_itemList->SetItemData( ritem, new WX_SHAPE_TREE_ITEM_DATA( ent ) );

        for( auto child : ent->m_children )
        {
            auto citem = m_itemList->AppendItem( ritem, "" );
            buildListTree( citem, child );
        }


        


}



static void expandAllChildren( wxTreeListCtrl* tree, const wxTreeListItem& item)
{
    tree->Freeze();
    // expand this item first, this might result in its children being added on
    // the fly
    if ( item != tree->GetRootItem()  )
        tree->Expand(item);
    //else: expanding hidden root item is unsupported and unnecessary

    // then (recursively) expand all the children
    for ( auto idCurr = tree->GetFirstChild(item);
          idCurr.IsOk();
          idCurr = tree->GetNextSibling(item) )
    {
       expandAllChildren(tree, idCurr);
    }
    tree->Thaw();
}

void PNS_LOG_VIEWER_FRAME::updateDumpPanel( int iter )
{
    if ( !m_env )
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

    printf("ItemList: %p\n", m_itemList );

    m_itemList->DeleteAllItems();
    buildListTree( rootItem, st->m_entries );
    m_itemList->CheckItemRecursively ( rootItem, wxCHK_CHECKED );
    //m_itemList->Expand( List->GetRootItem() );

    //expandAllChildren( m_itemList, m_itemList->GetRootItem() );
    m_itemList->Refresh();
}

static bool commonParallelProjection( SEG p, SEG n, SEG &pClip, SEG& nClip )
{
    SEG n_proj_p( p.LineProject( n.A ), p.LineProject( n.B ) );

    int64_t t_a = 0;
    int64_t t_b = p.TCoef( p.B );

    int64_t tproj_a = p.TCoef( n_proj_p.A );
    int64_t tproj_b = p.TCoef( n_proj_p.B );

    if( t_b < t_a )
        std::swap( t_b, t_a );

    if( tproj_b < tproj_a )
        std::swap( tproj_b, tproj_a );

    if( t_b <= tproj_a )
        return false;

    if( t_a >= tproj_b )
        return false;

    int64_t t[4] = { 0, p.TCoef( p.B ), p.TCoef( n_proj_p.A ), p.TCoef( n_proj_p.B ) };
    std::vector<int64_t> tv( t, t + 4 );
    std::sort( tv.begin(), tv.end() ); // fixme: awful and disgusting way of finding 2 midpoints

    int64_t pLenSq = p.SquaredLength();

    VECTOR2I dp = p.B - p.A;
    pClip.A.x = p.A.x + rescale( (int64_t)dp.x, tv[1], pLenSq );
    pClip.A.y = p.A.y + rescale( (int64_t)dp.y, tv[1], pLenSq );

    pClip.B.x = p.A.x + rescale( (int64_t)dp.x, tv[2], pLenSq );
    pClip.B.y = p.A.y + rescale( (int64_t)dp.y, tv[2], pLenSq );

    nClip.A = n.LineProject( pClip.A );
    nClip.B = n.LineProject( pClip.B );

    return true;
}

#if 0

using namespace PNS;



template <typename T1, typename T2>
typename std::map<T1, T2>::iterator findClosestKey( std::map<T1, T2> & data, T1 key)
{
    if (data.size() == 0) {
        return data.end();
    }

    auto lower = data.lower_bound(key);

    if (lower == data.end()) // If none found, return the last one.
        return std::prev(lower);

    if (lower == data.begin())
        return lower;

    // Check which one is closest.
    auto previous = std::prev(lower);
    if ((key - previous->first) < (lower->first - key))
        return previous;

    return lower;
}

void extractDiffPairItems ( PNS::NODE* node, int net_p, int net_n )
{
    std::set<PNS::ITEM*> pendingItems, complementItems;
    node->AllItemsInNet( net_p, pendingItems, PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T );
    node->AllItemsInNet( net_n, complementItems, PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T );

    while( pendingItems.size() )
    {
        PNS::LINE l = node->AssembleLine( static_cast<PNS::LINKED_ITEM*>( *pendingItems.begin() ) );

        printf("l net %d segs %d layer %d\n", net_p, l.SegmentCount(), l.Layer() );

        std::map<int, int> gapMap;
        std::map<PNS::LINKED_ITEM*, int> coupledCandidates;

        for( auto li : l.Links() )
        {
            pendingItems.erase( li );
            auto sp = dyn_cast<PNS::SEGMENT*> ( li );

            if(!sp)
                continue;

            for ( auto ci : complementItems )
            {
                auto sn = dyn_cast<PNS::SEGMENT*> ( ci );

                if( !sn->Layers().Overlaps( sp->Layers() ))
                    continue;

                
                auto ssp = sp->Seg();
                auto ssn = sn->Seg();
                
                if( ssp.ApproxParallel(ssn) )
                {
                    SEG ca, cb;
                    bool coupled = commonParallelProjection( ssp, ssn, ca, cb );
                    
                    if( coupled )
                    {
                       /* g_overlay->SetStrokeColor( KIGFX::COLOR4D( 1.0, 0.8, 0.8, 1.0 ) );
                        g_overlay->SetIsFill(false);
                        g_overlay->SetIsStroke(true );
                        g_overlay->SetLineWidth( 10000 );
                        g_overlay->Line( ca );
                        g_overlay->SetStrokeColor( KIGFX::COLOR4D( 0.8, 0.8, 1.0, 1.0 ) );
                        g_overlay->Line( cb );*/

                        int len = ca.Length();
                        int gap = (int)(ca.A - cb.A).EuclideanNorm() - (sp->Width()+sn->Width()) / 2;

                        auto closestGap = findClosestKey( gapMap, gap );

                        coupledCandidates[sn] = gap;

                        if( closestGap == gapMap.end() || std::abs(closestGap->first - gap) > 50 )
                        {
                            gapMap[gap] = len;
                        } else {
                            closestGap->second += len;
                        }

                        printf("Seg %p %p gap %d dist %d\n", sp, sn, gap, len );
                    }
                }
            }
        }

        int bestGap = -1;
        int maxLength = 0;
        for( auto i : gapMap )
        {
            if( i.second > maxLength )
            {
                maxLength = i.second;
                bestGap = i.first;
            }
        }

        printf("Best gap: %d\n", bestGap);

        bool pendingCandidates = true;

        while ( pendingCandidates )
        {
            pendingCandidates = false;

            for ( auto c : coupledCandidates )
            {
                if( std::abs(c.second - bestGap ) < 50 )
                {
                    PNS::LINE l_coupled = node->AssembleLine( c.first );

                    PNS::DIFF_PAIR pair(l, l_coupled, bestGap );
                    pair.SetNets( l.Net(), l_coupled.Net() );

/*                     g_overlay->SetStrokeColor( KIGFX::COLOR4D( 1.0, 0.8, 0.8, 1.0 ) );
                     g_overlay->SetIsFill(true);
                     g_overlay->SetIsStroke(true );
                     g_overlay->SetLineWidth( 10000 );
                     g_overlay->Polyline( l.CLine() );
                     g_overlay->SetStrokeColor( KIGFX::COLOR4D( 0.8, 0.8, 1.0, 1.0 ) );
                     g_overlay->Polyline( l_coupled.CLine() );
*/
                    for( auto li : l_coupled.Links() )
                        coupledCandidates.erase( li );

                    printf("Orig line : %d segs, coupled line: %d segs\n", l.SegmentCount(), l_coupled.SegmentCount() );

                    pendingCandidates = true;
                    break;
                }
            }
        }
    }
}



int pns1_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME(nullptr);
    std::shared_ptr<BOARD> board;

    board.reset( loadBoard ( argv[1] ) );

    frame->SetBoard( board );

    Pgm().App().SetTopWindow( frame );      // wxApp gets a face.
    frame->Show();

    PNS_KICAD_IFACE_BASE iface;
    PNS::NODE *world = new PNS::NODE;

    iface.SetBoard( board.get() );
    iface.SyncWorld( world );
    
    #if 0
    std::map<int, int> diffPairs;

    // FIXMe: GetNetInfo copies f***ing pointers...
    const auto& netinfo = board->GetNetInfo();

    int n_nets = netinfo.GetNetCount();

    for (int net = 0 ; net < n_nets; net++ )
    {
        int coupled = iface.GetRuleResolver()->DpCoupledNet( net );
        if( coupled > 0 && diffPairs.find( coupled ) == diffPairs.end() )
        {
            printf("net %d coupled %d\n", net, coupled );
            diffPairs[net] = coupled;
        }
    }

    for( auto p : diffPairs )
    {
        extractDiffPairItems ( world, p.first, p.second );
    }
    #endif

    return 0;
}

static bool registered = UTILITY_REGISTRY::Register( {
        "pns1",
        "PNS Dumb Test (Tom's hacks)",
        pns1_main_func,
} );

#endif

//extern "C" void drcCreateTestsProviderClearance();
//extern "C" void drcCreateTestsProviderEdgeClearance();

int replay_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME(nullptr);

  //  drcCreateTestsProviderClearance();
//    drcCreateTestsProviderEdgeClearance();
    

    if( argc >= 2 && std::string(argv[1]) == "-h")
    {
        printf("PNS Log (Re)player. Allows to step through the log written by the ROUTER_TOOL in debug Kicad builds. ");
        printf("Requires a board file with UUIDs and a matching log file. Both are written to /tmp when you press '0' during routing.");
        return 0;
    }

    if(argc < 3)
    {
        printf("Expected parameters: log_file.log board_file.dump\n");
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




bool commonParallelProjection( SEG p, SEG n, SEG &pClip, SEG& nClip );


template <typename T1, typename T2>
typename std::map<T1, T2>::iterator findClosestKey( std::map<T1, T2> & data, T1 key)
{
    if (data.size() == 0) {
        return data.end();
    }

    auto lower = data.lower_bound(key);

    if (lower == data.end()) // If none found, return the last one.
        return std::prev(lower);

    if (lower == data.begin())
        return lower;

    // Check which one is closest.
    auto previous = std::prev(lower);
    if ((key - previous->first) < (lower->first - key))
        return previous;

    return lower;
}



void extractDiffPair( BOARD* aBoard, int net_p, int net_n )
{
    std::set<TRACK*> pendingItems, complementItems;

    struct DP_COUPLED_SEGMENTS
    {
        TRACK* itemP, *itemN;
    };

    for( auto trk: aBoard->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T || trk->Type() == PCB_ARC_T )
        {
            if( trk->GetNetCode() == net_p )
                pendingItems.insert( trk );
            else if( trk->GetNetCode() == net_n )
                complementItems.insert( trk );
        }
    }

    while( pendingItems.size() )
    {
        auto li = *pendingItems.begin();
        pendingItems.erase( li );
        auto sp = dyn_cast<TRACK*>(li);

        if(!sp)
            continue;

        for ( auto ci : complementItems )
        {
            auto sn = dyn_cast<TRACK*> ( ci );

                if( ( sn->GetLayerSet() & sp->GetLayerSet() ).none() )
                    continue;

                SEG ssp ( sp->GetStart(), sp->GetEnd() );
                SEG ssn ( sn->GetStart(), sn->GetEnd() );

                if( ssp.ApproxParallel(ssn) )
                {
                    SEG ca, cb;
                    bool coupled = commonParallelProjection( ssp, ssn, ca, cb );

                                    }
            }
        }

}

static int matchDpSuffix( const wxString& aNetName, wxString& aComplementNet,
                                             wxString& aBaseDpName )
{
    int rv = 0;

    if( aNetName.EndsWith( "+" ) )
    {
        aComplementNet = "-";
        rv = 1;
    }
    else if( aNetName.EndsWith( "P" ) )
    {
        aComplementNet = "N";
        rv = 1;
    }
    else if( aNetName.EndsWith( "-" ) )
    {
        aComplementNet = "+";
        rv = -1;
    }
    else if( aNetName.EndsWith( "N" ) )
    {
        aComplementNet = "P";
        rv = -1;
    }
    // Match P followed by 2 digits
    else if( aNetName.Right( 2 ).IsNumber() && aNetName.Right( 3 ).Left( 1 ) == "P" )
    {
        aComplementNet = "N" + aNetName.Right( 2 );
        rv = 1;
    }
    // Match P followed by 1 digit
    else if( aNetName.Right( 1 ).IsNumber() && aNetName.Right( 2 ).Left( 1 ) == "P" )
    {
        aComplementNet = "N" + aNetName.Right( 1 );
        rv = 1;
    }
    // Match N followed by 2 digits
    else if( aNetName.Right( 2 ).IsNumber() && aNetName.Right( 3 ).Left( 1 ) == "N" )
    {
        aComplementNet = "P" + aNetName.Right( 2 );
        rv = -1;
    }
    // Match N followed by 1 digit
    else if( aNetName.Right( 1 ).IsNumber() && aNetName.Right( 2 ).Left( 1 ) == "N" )
    {
        aComplementNet = "P" + aNetName.Right( 1 );
        rv = -1;
    }
    if( rv != 0 )
    {
        aBaseDpName = aNetName.Left( aNetName.Length() - aComplementNet.Length() );
        aComplementNet = aBaseDpName + aComplementNet;
    }

    return rv;
}


static int dpCoupledNet( BOARD* aBoard, int aNet )
{
    wxString refName = aBoard->FindNet( aNet )->GetNetname();
    wxString dummy, coupledNetName;

    if( matchDpSuffix( refName, coupledNetName, dummy ) )
    {
        NETINFO_ITEM* net = aBoard->FindNet( coupledNetName );

        if( !net )
            return -1;

        return net->GetNetCode();
    }

    return -1;
}


void extractAllDiffPairs( BOARD* aBoard )
{
    std::map<int, int> diffPairs;

    // FIXMe: GetNetInfo copies f***ing pointers...
    const auto& netinfo = aBoard->GetNetInfo();

    int n_nets = netinfo.GetNetCount();

    for (int net = 0 ; net < n_nets; net++ )
    {
        int coupled = dpCoupledNet(aBoard, net );

        if( coupled > 0 && diffPairs.find( coupled ) == diffPairs.end() )
        {
            diffPairs[net] = coupled;
        }
    }

    for( auto p : diffPairs )
    {
        printf("Extract DP: %s/%s\n", (const char*) aBoard->FindNet( p.first )->GetNetname(),
        (const char*) aBoard->FindNet( p.second )->GetNetname() );
        
        extractDiffPair ( aBoard, p.first, p.second );
    }
}

#if 0
int test1_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME(nullptr);
    
    PROJECT_CONTEXT project = loadKicadProject( argv[1], OPT<wxString>() );

    frame->SetBoard( project.board);
    
    Pgm().App().SetTopWindow( frame );      // wxApp gets a face.
    frame->Show();

    auto view = frame->GetPanel()->GetView();
    //auto settings =  static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
        //settings->SetZoneDisplayMode( ZONE_DISPLAY_MODE:: );

/*    view->SetLayerVisible( LAYER_TRACKS, false );
    view->SetLayerVisible( LAYER_PADS, false );
    view->SetLayerVisible( LAYER_MOD_TEXT_BK, false );
    view->SetLayerVisible( LAYER_MOD_TEXT_FR, false );
    view->SetLayerVisible( LAYER_MOD_REFERENCES, false );
    view->SetLayerVisible( LAYER_MOD_VALUES, false );
    view->SetLayerVisible( LAYER_MOD_BK, false );
    view->SetLayerVisible( LAYER_MOD_FR, false );*/

    runDRCProto( project, frame->GetPanel()->DebugOverlay() );

    return 0;
}
#endif

//std::shared_ptr<KIGFX::VIEW_OVERLAY> g_overlay;
static std::shared_ptr<KIGFX::VIEW_OVERLAY> overlay;

bool segmentCrossesHullBoundary( const SHAPE_LINE_CHAIN& hull, const SEG& seg)
{
        for( int j = 0; j < hull.SegmentCount(); j++ )
        {
            auto sr = hull.CSegment(j);
        }
    

}

bool walkaround2( SHAPE_LINE_CHAIN& aLine, SHAPE_LINE_CHAIN aObstacle, SHAPE_LINE_CHAIN& aPre,
                           SHAPE_LINE_CHAIN& aWalk, SHAPE_LINE_CHAIN& aPost, bool aCw )
{
    const SHAPE_LINE_CHAIN& line( aLine );

    if( line.SegmentCount() < 1 )
        return false;

    const auto pFirst = line.CPoint(0);
    const auto pLast = line.CPoint(-1);

    bool inFirst = aObstacle.PointInside( pFirst ) && !aObstacle.PointOnEdge( pFirst );
    bool inLast = aObstacle.PointInside( pLast ) && !aObstacle.PointOnEdge( pLast );

    if( inFirst || inLast )
    {
	    return false;
    }

    enum VERTEX_TYPE { INSIDE = 0, OUTSIDE, ON_EDGE };

    struct VERTEX
    {
        VERTEX_TYPE type;
        bool isHull;
        VECTOR2I pos;
        std::vector<VERTEX*> neighbours;
        int indexp, indexh;
        bool visited = false;
    };

    SHAPE_LINE_CHAIN::INTERSECTIONS ips;

    line.Intersect( aObstacle, ips );

    SHAPE_LINE_CHAIN pnew(aLine), hnew(aObstacle);

    std::vector<VERTEX> vts;

    auto findVertex = [&]( VECTOR2I pos) -> VERTEX*
    {
        for(auto& v : vts)
            if(v.pos == pos )
                return &v;

        return nullptr;
    };

    for( auto ip : ips )
    {
        bool isNewP, isNewH;

        if( pnew.Find( ip.p ) < 0 )
        {
            pnew.Split(ip.p);
            //overlay->SetStrokeColor( YELLOW );
            //overlay->Circle( ip.p, 200000 );
        }

        if( hnew.Find( ip.p ) < 0 )
        {
            hnew.Split(ip.p);
            //overlay->SetStrokeColor( CYAN );
            //overlay->Circle( ip.p, 250000 );
        }
    }

    if ( !aCw )
        hnew = hnew.Reverse();

    for( int i = 0; i < pnew.PointCount(); i++ )
    {
        auto p = pnew.CPoint(i);
        VERTEX v;
        v.indexp = i;
        v.isHull = false;
        v.pos = p;
        v.type = hnew.PointInside( p ) && !hnew.PointOnEdge( p ) ? INSIDE : hnew.PointOnEdge( p ) ? ON_EDGE : OUTSIDE;
        vts.push_back(v);
        overlay->SetStrokeColor( WHITE );
        overlay->SetFillColor( WHITE );
        overlay->SetGlyphSize( VECTOR2D( 200000, 200000 ));
        overlay->BitmapText( wxString::Format("%d", i), v.pos, 0 );
    }

    for( int i = 0; i < pnew.PointCount() - 1; i++ )
    {
        vts[i].neighbours.push_back(&vts[i+1]);
    }

    for( int i = 0; i < hnew.PointCount(); i++ )
    {
        auto hp = hnew.CPoint(i);
        bool found = false;
        auto vn = findVertex( hp );

        if( vn )
        {
            vn->isHull = true;
            vn->indexh = i;
        } else {
            VERTEX v;
            v.pos = hp;
            v.type = ON_EDGE;
            v.indexh = i;
            v.isHull = true;
            vts.push_back( v );
        }

        overlay->SetStrokeColor( WHITE );
        overlay->SetFillColor( WHITE );
        overlay->SetGlyphSize( VECTOR2D( 200000, 200000 ));
      //  overlay->BitmapText( wxString::Format("%d", i), hp, 0 );
    }


    for( int i = 0; i < hnew.PointCount(); i++ )
    {
        auto vc = findVertex( hnew.CPoint(i ) );
        auto vnext = findVertex( hnew.CPoint(i+1) );

       

        if(vc && vnext)
            vc->neighbours.push_back(vnext);
    }

    for( auto& v : vts )
    {
        printf(" ip %d ih %d t %d ishull %d\n", v.indexp, v.indexh, v.type, v.isHull?1:0);
    }

    
    VERTEX* v = &vts[0];

    SHAPE_LINE_CHAIN out;

    int n = 0;
    while (v->indexp != pnew.PointCount() )
    {
        printf("SCAN ip %d ih %d t %d ishull %d\n", v->indexp, v->indexh, v->type, v->isHull?1:0);
        
        out.Append( v->pos );
        if(v->visited)
            break;

        overlay->SetStrokeColor( v->isHull ? RED : WHITE );
        overlay->SetFillColor( v->isHull ? RED : WHITE );
        overlay->SetGlyphSize( VECTOR2D( 200000, 200000 ));
        //overlay->BitmapText( wxString::Format("%d", v->isHull ? v->indexh : v->indexp), v->pos, 0 );


        v->visited = true;
        
        if ( (n++) == 2000 ) // sanity check
            break;

        for( auto vn : v->neighbours )
        {
            printf(" ---> next ip %d ih %d t %d ishull %d\n",vn->indexp, vn->indexh, vn->type, vn->isHull?1:0);
        }


        if (v->type == OUTSIDE)
        {
            out.Append(v->pos);
            for( auto vn : v->neighbours )
                if( (vn->indexp > v->indexp) && vn->type != INSIDE )
                {
                    v = vn;
                    break;
                }
        }
        else if (v->type == ON_EDGE)
        {
            VERTEX* v_next = nullptr;
            int best_dist = INT_MAX;
 
            for( auto vn: v->neighbours)
            {
                if( vn->type  == ON_EDGE && (vn->indexp == (v->indexp + 1) ) )
                {
                    v_next = vn;
                    break;
                }
            }

            if( !v_next )
            {
                for( auto vn: v->neighbours)
                {
                    if( vn->type  == OUTSIDE )
                    {
                        v_next = vn;
                        break;
                    }
                }
            }
            
            if( !v_next )
            {
                for( auto vn: v->neighbours)
                {
                    if ( v->type == ON_EDGE )
                    {
                        if( vn->indexh == ( (v->indexh + 1) % hnew.PointCount() ) )
                        {
                            v_next = vn;
                            //printf("E2\n");
                            

                            break;
                        }
                    }
                }
            }

            v = v_next;


        }
    }

    out.Append( v->pos );

    aWalk = out;

    return true;
}

int test2_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME(nullptr);
    Pgm().App().SetTopWindow( frame );      // wxApp gets a face.
    frame->Show();

    overlay = frame->GetPanel()->DebugOverlay();

    
    overlay->SetIsFill(false);
    overlay->SetLineWidth(10000);

   // auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 155977520, 128439216), VECTOR2I( 155639216, 128777520), VECTOR2I( 155160784, 128777520), VECTOR2I( 154822480, 128439216), VECTOR2I( 154822480, 97960784), VECTOR2I( 155160784, 97622480), VECTOR2I( 155639216, 97622480), VECTOR2I( 155977520, 97960784)}, true ); 
    //auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 148981200, 102320000), VECTOR2I( 154822480, 102320000), VECTOR2I( 154822480, 98777520), VECTOR2I( 60977520, 98777520), VECTOR2I( 60977520, 127622480), VECTOR2I( 155639216, 127622480), VECTOR2I( 155977520, 127960784), VECTOR2I( 155977520, 128439216), VECTOR2I( 155639216, 128777520), VECTOR2I( 60160784, 128777520), VECTOR2I( 59822480, 128439216), VECTOR2I( 59822480, 97960784), VECTOR2I( 60160784, 97622480), VECTOR2I( 155639216, 97622480), VECTOR2I( 155977520, 97960784), VECTOR2I( 155977520, 102320000), VECTOR2I( 160208000, 102320000), VECTOR2I( 160462000, 102574000)}, false ); 

    //auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 112456000, 102726400), VECTOR2I( 112456000, 98560800), VECTOR2I( 112456000, 98827020)}, false );
    //auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 155659720, 97572980), VECTOR2I( 156027020, 97940280), VECTOR2I( 156027020, 98459720), VECTOR2I( 155659720, 98827020), VECTOR2I( 60140280, 98827020), VECTOR2I( 59772980, 98459720), VECTOR2I( 59772980, 97940280), VECTOR2I( 60140280, 97572980)}, true );
    
    //auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 119364800, 100288000), VECTOR2I( 119364800, 97697200), VECTOR2I( 119364800, 97572980)}, false );
    //auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 155659720, 97572980), VECTOR2I( 156027020, 97940280), VECTOR2I( 156027020, 98459720), VECTOR2I( 155659720, 98827020), VECTOR2I( 60140280, 98827020), VECTOR2I( 59772980, 98459720), VECTOR2I( 59772980, 97940280), VECTOR2I( 60140280, 97572980)}, true );

    //auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 119263200, 101253200), VECTOR2I( 119263200, 98827020), VECTOR2I( 61027020, 98827020), VECTOR2I( 61027020, 127572980), VECTOR2I( 154772980, 127572980), VECTOR2I( 154772980, 97940280), VECTOR2I( 155140280, 97572980), VECTOR2I( 155659720, 97572980), VECTOR2I( 156027020, 97940280), VECTOR2I( 156027020, 128459720), VECTOR2I( 155659720, 128827020), VECTOR2I( 60140280, 128827020), VECTOR2I( 59772980, 128459720), VECTOR2I( 59772980, 97940280), VECTOR2I( 60140280, 97572980), VECTOR2I( 119641420, 97572980), VECTOR2I( 121650800, 95563600)}, false );
    //auto hull =  SHAPE_LINE_CHAIN( { VECTOR2I( 155659720, 97572980), VECTOR2I( 156027020, 97940280), VECTOR2I( 156027020, 98459720), VECTOR2I( 155659720, 98827020), VECTOR2I( 60140280, 98827020), VECTOR2I( 59772980, 98459720), VECTOR2I( 59772980, 97940280), VECTOR2I( 60140280, 97572980)}, true );

//    auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 96722489, 117694794), VECTOR2I( 97594794, 116822489), VECTOR2I( 99205206, 116822489), VECTOR2I( 100077511, 117694794), VECTOR2I( 100077511, 119305206), VECTOR2I( 99205206, 120177511), VECTOR2I( 97594794, 120177511), VECTOR2I( 96722489, 119305206)}, true ); 
  //  auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 103400000, 118500000), VECTOR2I( 93400000, 118500000)}, false ); 

    auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 66280505, 107710033), VECTOR2I( 65914967, 107344495), VECTOR2I( 65914967, 106827549), VECTOR2I( 66280505, 106462011), VECTOR2I( 74810033, 106462009), VECTOR2I( 75175571, 106827547), VECTOR2I( 75175571, 107344493), VECTOR2I( 74810033, 107710031)}, true ); 
    auto path = SHAPE_LINE_CHAIN( { /*VECTOR2I( 143928480, 109445996), VECTOR2I( 111066480, 109445996), VECTOR2I( 106254391, 104633907), VECTOR2I( 105909001, 104633907), VECTOR2I( 105775094, 104500000),*/ VECTOR2I( 76250000, 104500000), VECTOR2I( 74287991, 106462009), VECTOR2I( 66280505, 106462011), VECTOR2I( 66012989, 106462011)}, false );


    BOX2D bb ( path.BBox().GetPosition(), path.BBox().GetSize() );

    frame->GetPanel()->GetView()->SetViewport(bb);

    PNS::LINE l;
    SHAPE_LINE_CHAIN path_pre, path_walk, path_post;
    l.SetShape( path );

    auto status = l.Walkaround( hull, path_walk, false );
    printf("Stat: %d\n", status );
    
        //printf("status: %d\n", walkaround2( path, hull, path_pre, path_walk,
    //                  path_post, false ) );

    overlay->SetLineWidth(200000.0);
    overlay->SetStrokeColor( BLUE );
    //overlay->Polyline( path_pre );
    overlay->Polyline( path_walk );
    //overlay->Polyline( path_post );

    overlay->SetStrokeColor( WHITE );
    overlay->SetLineWidth( 100000.0 );
    overlay->Polyline( path );
    
    overlay->SetStrokeColor( RED );
    overlay->Polyline( hull );
    

    return 0;
}

#if 0
static bool registered3 = UTILITY_REGISTRY::Register( {
        "test1",
        "Test1",
        test1_main_func,
} );

#endif

static bool registered4 = UTILITY_REGISTRY::Register( {
        "test2",
        "Test2",
        test2_main_func,
} );


