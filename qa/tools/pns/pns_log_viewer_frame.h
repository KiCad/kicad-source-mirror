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

#ifndef __PNS_LOG_VIEWER_FRAME_H
#define __PNS_LOG_VIEWER_FRAME_H

#include <length_delay_calculation/length_delay_calculation.h>
#include <pcb_painter.h>
#include <pcb_test_frame.h>
#include <pcbnew_utils/board_test_utils.h>
#include <reporter.h>
#include <router/pns_solid.h>

#include "pns_log_file.h"
#include "pns_log_player.h"
#include "pns_test_debug_decorator.h"
#include "pns_log_viewer_frame_base.h"

#include "label_manager.h"

#define ID_LIST_COPY 10001
#define ID_LIST_SHOW_ALL 10002
#define ID_LIST_SHOW_NONE 10003
#define ID_LIST_DISPLAY_LINE 10004

class PNS_LOG_VIEWER_OVERLAY;

class PNS_VIEWER_IFACE : public PNS::ROUTER_IFACE
{
public:
    PNS_VIEWER_IFACE( std::shared_ptr<BOARD> aBoard ){ m_board = aBoard; };
    ~PNS_VIEWER_IFACE() override{};

    void EraseView() override {};
    void SyncWorld( PNS::NODE* aWorld ) override {};
    bool IsAnyLayerVisible( const PNS_LAYER_RANGE& aLayer ) const override { return true; };
    bool IsFlashedOnLayer( const PNS::ITEM* aItem, int aLayer ) const override { return false; };
    bool IsFlashedOnLayer( const PNS::ITEM* aItem, const PNS_LAYER_RANGE& aLayer ) const override { return false; };
    bool IsItemVisible( const PNS::ITEM* aItem ) const override { return true; };
    bool IsPNSCopperLayer( int aLayer ) const override { return false; };
    void HideItem( PNS::ITEM* aItem ) override {}
    void DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit = false,
                      int aFlags = 0 ) override {}
    void DisplayPathLine( const SHAPE_LINE_CHAIN& aLine, int aImportance ) override {}
    void DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, PNS::NET_HANDLE aNet ) override {}
    void AddItem( PNS::ITEM* aItem ) override {}
    void UpdateItem( PNS::ITEM* aItem ) override {}
    void RemoveItem( PNS::ITEM* aItem ) override {}
    void Commit() override {}
    bool ImportSizes( PNS::SIZES_SETTINGS& aSizes, PNS::ITEM* aStartItem,
                      PNS::NET_HANDLE aNet, VECTOR2D aStartPosition ) override { return false; }
    int StackupHeight( int aFirstLayer, int aSecondLayer ) const override { return 0; }

    int GetNetCode( PNS::NET_HANDLE aNet ) const override { return -1; }
    wxString GetNetName( PNS::NET_HANDLE aNet ) const override { return wxEmptyString; }
    void UpdateNet( PNS::NET_HANDLE aNet ) override {}
    PNS::NET_HANDLE GetOrphanedNetHandle() override { return nullptr; }

    virtual PNS::NODE* GetWorld() const override { return nullptr; };
    PNS::RULE_RESOLVER* GetRuleResolver() override { return nullptr; }
    PNS::DEBUG_DECORATOR* GetDebugDecorator() override { return nullptr; }

    PCB_LAYER_ID GetBoardLayerFromPNSLayer( int aLayer ) const override
    {
        if( aLayer == 0 )
            return F_Cu;

        if( aLayer == m_board->GetCopperLayerCount() - 1 )
            return B_Cu;

        return ToLAYER_ID( ( aLayer + 1 ) * 2 );
    }


    int GetPNSLayerFromBoardLayer( PCB_LAYER_ID aLayer ) const override
    {
        if( aLayer == F_Cu )
            return 0;

        if( aLayer == B_Cu )
            return m_board->GetCopperLayerCount() - 1;

        return ( aLayer / 2 ) - 1;
    }

    long long int CalculateRoutedPathLength( const PNS::ITEM_SET& aLine, const PNS::SOLID* aStartPad,
                                             const PNS::SOLID* aEndPad, const NETCLASS* aNetClass ) override
    {
        std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems = getLengthDelayCalculationItems( aLine, aNetClass );

        const PAD* startPad = nullptr;
        const PAD* endPad = nullptr;

        if( aStartPad )
            startPad = static_cast<PAD*>( aStartPad->Parent() );

        if( aEndPad )
            endPad = static_cast<PAD*>( aEndPad->Parent() );

        constexpr PATH_OPTIMISATIONS opts = {
            .OptimiseViaLayers = false, .MergeTracks = false, .OptimiseTracesInPads = false, .InferViaInPad = true
        };

        return m_board->GetLengthCalculation()->CalculateLength( lengthItems, opts, startPad, endPad );
    }

    int64_t CalculateRoutedPathDelay( const PNS::ITEM_SET& aLine, const PNS::SOLID* aStartPad,
                                      const PNS::SOLID* aEndPad, const NETCLASS* aNetClass ) override
    {
        std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems = getLengthDelayCalculationItems( aLine, aNetClass );

        const PAD* startPad = nullptr;
        const PAD* endPad = nullptr;

        if( aStartPad )
            startPad = static_cast<PAD*>( aStartPad->Parent() );

        if( aEndPad )
            endPad = static_cast<PAD*>( aEndPad->Parent() );

        constexpr PATH_OPTIMISATIONS opts = {
            .OptimiseViaLayers = false, .MergeTracks = false, .OptimiseTracesInPads = false, .InferViaInPad = true
        };

        return m_board->GetLengthCalculation()->CalculateDelay( lengthItems, opts, startPad, endPad );
    }

    std::vector<LENGTH_DELAY_CALCULATION_ITEM> getLengthDelayCalculationItems( const PNS::ITEM_SET& aLine,
                                                                               const NETCLASS*      aNetClass ) const
    {
        std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems;

        for( int idx = 0; idx < aLine.Size(); idx++ )
        {
            const PNS::ITEM* lineItem = aLine[idx];

            if( const PNS::LINE* l = dyn_cast<const PNS::LINE*>( lineItem ) )
            {
                LENGTH_DELAY_CALCULATION_ITEM item;
                item.SetLine( l->CLine() );

                const PCB_LAYER_ID layer = GetBoardLayerFromPNSLayer( lineItem->Layer() );
                item.SetLayers( layer );
                item.SetEffectiveNetClass( aNetClass );

                lengthItems.emplace_back( std::move( item ) );
            }
            else if( lineItem->OfKind( PNS::ITEM::VIA_T ) && idx > 0 && idx < aLine.Size() - 1 )
            {
                const int          layerPrev = aLine[idx - 1]->Layer();
                const int          layerNext = aLine[idx + 1]->Layer();
                const PCB_LAYER_ID pcbLayerPrev = GetBoardLayerFromPNSLayer( layerPrev );
                const PCB_LAYER_ID pcbLayerNext = GetBoardLayerFromPNSLayer( layerNext );

                if( layerPrev != layerNext )
                {
                    LENGTH_DELAY_CALCULATION_ITEM item;
                    item.SetVia( static_cast<PCB_VIA*>( lineItem->GetSourceItem() ) );
                    item.SetLayers( pcbLayerPrev, pcbLayerNext );
                    item.SetEffectiveNetClass( aNetClass );
                    lengthItems.emplace_back( std::move( item ) );
                }
            }
        }

        return lengthItems;
    }

    int64_t CalculateLengthForDelay( int64_t aDesiredDelay, const int aWidth, const bool aIsDiffPairCoupled,
                                     const int aDiffPairCouplingGap, const int aPNSLayer,
                                     const NETCLASS* aNetClass ) override
    {
        TUNING_PROFILE_GEOMETRY_CONTEXT ctx;
        ctx.NetClass = aNetClass;
        ctx.Width = aWidth;
        ctx.IsDiffPairCoupled = aIsDiffPairCoupled;
        ctx.DiffPairCouplingGap = aDiffPairCouplingGap;
        ctx.Layer = GetBoardLayerFromPNSLayer( aPNSLayer );

        return m_board->GetLengthCalculation()->CalculateLengthForDelay( aDesiredDelay, ctx );
    }

    int64_t CalculateDelayForShapeLineChain( const SHAPE_LINE_CHAIN& aShape, int aWidth, bool aIsDiffPairCoupled,
                                             int aDiffPairCouplingGap, int aPNSLayer,
                                             const NETCLASS* aNetClass ) override
    {
        TUNING_PROFILE_GEOMETRY_CONTEXT ctx;
        ctx.NetClass = aNetClass;
        ctx.Width = aWidth;
        ctx.IsDiffPairCoupled = aIsDiffPairCoupled;
        ctx.DiffPairCouplingGap = aDiffPairCouplingGap;
        ctx.Layer = GetBoardLayerFromPNSLayer( aPNSLayer );

        return m_board->GetLengthCalculation()->CalculatePropagationDelayForShapeLineChain( aShape, ctx );
    }

    private:
        std::shared_ptr<BOARD> m_board;
};

class PNS_LOG_VIEWER_FRAME : public PNS_LOG_VIEWER_FRAME_BASE, public PCB_TEST_FRAME_BASE
{
public:
    PNS_LOG_VIEWER_FRAME( wxFrame* frame );
    virtual ~PNS_LOG_VIEWER_FRAME();

    void LoadLogFile( const wxString& aFile );
    void SetLogFile( PNS_LOG_FILE* aLog );
    void SetBoard2( std::shared_ptr<BOARD> aBoard );
    REPORTER* GetConsoleReporter();

    std::shared_ptr<PNS_LOG_VIEWER_OVERLAY> GetOverlay() const { return m_overlay; }

private:
    void             drawLoggedItems( int iter );
    void             updateDumpPanel( int iter );
    virtual void     createUserTools() override;
    void             buildListTree( wxTreeListItem item, PNS_DEBUG_SHAPE* ent, int depth = 0 );
    void             syncModel();
    PNS_DEBUG_STAGE* getCurrentStage();
    void             updatePnsPreviewItems( int iter );
    bool             filterStringMatches( PNS_DEBUG_SHAPE* ent );
    void             updateViewerIface();

    virtual void onOpen( wxCommandEvent& event ) override;
    virtual void onSaveAs( wxCommandEvent& event ) override;
    virtual void onExit( wxCommandEvent& event ) override;
    virtual void onRewindScroll( wxScrollEvent& event ) override;
    virtual void onRewindCountText( wxCommandEvent& event ) override;
    virtual void onListRightClick( wxMouseEvent& event );
    virtual void onListSelect( wxCommandEvent& event );
    virtual void onBtnRewindLeft( wxCommandEvent& event ) override;
    virtual void onBtnRewindRight( wxCommandEvent& event ) override;
    virtual void onListChecked( wxCommandEvent& event );
    virtual void onShowThinLinesChecked( wxCommandEvent& event ) override;
    virtual void onShowRPIsChecked( wxCommandEvent& event ) override;
    virtual void onShowVerticesChecked( wxCommandEvent& event ) override;
    virtual void onFilterText( wxCommandEvent& event ) override;
    void drawSimpleShape( SHAPE* aShape, bool aIsSelected, const std::string& aName );

    std::shared_ptr<PNS_LOG_VIEWER_OVERLAY> m_overlay;
    std::shared_ptr<PNS_LOG_FILE>           m_logFile;
    std::shared_ptr<PNS_LOG_PLAYER>         m_logPlayer;
    int                                     m_rewindIter;
    wxMenu*                                 m_listPopupMenu;
    std::shared_ptr<KIGFX::VIEW_GROUP>      m_previewItems;
    std::shared_ptr<PNS_VIEWER_IFACE>       m_viewerIface;
    std::map<wxString,wxString>             m_filenameToPathMap;

    bool m_showThinLines = true;
    bool m_showRPIs = true;
    bool m_showVertices = false;
    wxString m_searchString;
    //KI_TEST::CONSOLE_LOG          m_consoleLog;
    std::shared_ptr<WX_TEXT_CTRL_REPORTER> m_reporter;
};

class LABEL_MANAGER;

class PNS_LOG_VIEWER_OVERLAY : public KIGFX::VIEW_OVERLAY
{
public:
    PNS_LOG_VIEWER_OVERLAY( KIGFX::GAL* aGal );
    void AnnotatedPolyset( const SHAPE_POLY_SET& aL, std::string name = "",
                           bool aShowVertexNumbers = false );
    void AnnotatedPolyline( const SHAPE_LINE_CHAIN& aL, std::string name,
                            bool aShowVertexNumbers = false );
    void AnnotatedPoint( const VECTOR2I p, int size, std::string name = "",
                         bool aShowVertexNumbers = false );
    void Arc( const SHAPE_ARC& arc );
    void DrawAnnotations();

private:


    std::unique_ptr<LABEL_MANAGER> m_labelMgr;
};

#endif
