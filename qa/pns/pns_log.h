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
#ifndef __PNS_LOG_H
#define __PNS_LOG_H

#include <cstdio>

#include <wx/tokenzr.h>

#include <geometry/shape.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_file_io.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>

#include <router/pns_debug_decorator.h>
#include <router/pns_item.h>
#include <router/pns_line.h>
#include <router/pns_line_placer.h>
#include <router/pns_dragger.h>
#include <router/pns_logger.h>
#include <router/pns_node.h>
#include <router/pns_router.h>
#include <router/pns_solid.h>
#include <router/pns_routing_settings.h>

#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcbnew/zone.h>

#include <router/pns_kicad_iface.h>

#include <pcbnew/board.h>

#include <pcbnew/plugins/kicad/kicad_plugin.h>
#include <pcbnew/plugins/kicad/pcb_parser.h>

#include <wx/treelist.h>

class PNS_LOG_FILE
{
public:
    PNS_LOG_FILE();
    ~PNS_LOG_FILE() {}

    struct EVENT_ENTRY
    {
        VECTOR2I                p;
        PNS::LOGGER::EVENT_TYPE type;
        const PNS::ITEM*        item;
        KIID                    uuid;
    };

    // loads a board file and associated P&s event log
    bool Load( const std::string& logName, const std::string boardName );

    BOARD_CONNECTED_ITEM* ItemById( const EVENT_ENTRY& evt )
    {
        BOARD_CONNECTED_ITEM* parent = nullptr;

        for( auto item : m_board->AllConnectedItems() )
        {
            if( item->m_Uuid == evt.uuid )
            {
                parent = item;
                break;
            };
        }

        return parent;
    }

    std::vector<EVENT_ENTRY>& Events() { return m_events; }

    std::shared_ptr<BOARD> GetBoard() const { return m_board; }

    void SetBoard( std::shared_ptr<BOARD> brd ) { m_board = brd; }

    PNS::ROUTING_SETTINGS* GetRoutingSettings() const { return m_routerSettings.get(); }

private:
    std::unique_ptr<PNS::ROUTING_SETTINGS> m_routerSettings;
    std::vector<EVENT_ENTRY>               m_events;
    std::shared_ptr<BOARD>                 m_board;
};


class PNS_TEST_DEBUG_DECORATOR : public PNS::DEBUG_DECORATOR
{
public:

    struct DEBUG_ENT
    {
        DEBUG_ENT( DEBUG_ENT* aParent = nullptr )
        {
            m_iter = 0;
            m_color = KIGFX::COLOR4D::WHITE;
            m_width = 10000;
            m_name = "<unknown>";
            m_parent = aParent;
            m_visible = true;
            m_selected = false;
        }

        ~DEBUG_ENT()
        {
            for( auto s : m_shapes )
            {
                delete s;
            }

            for( auto ch : m_children )
            {
                delete ch;
            }
        }

        DEBUG_ENT* NewChild()
        {
            DEBUG_ENT* ent = new DEBUG_ENT( this );
            m_children.push_back( ent );

            return ent;
        }

        void AddChild( DEBUG_ENT* ent )
        {
            ent->m_parent = this;
            m_children.push_back( ent );
        }

        bool IsVisible() const
        {
            if ( m_visible )
                return true;

            auto parent = m_parent;

            while(parent)
            {
                if(parent->m_visible)
                    return true;

                parent = parent->m_parent;
            }

            return false;
        }

        void IterateTree( std::function<bool(DEBUG_ENT*)> visitor, int depth=0 );

        DEBUG_ENT*              m_parent;
        std::vector<SHAPE*>     m_shapes;
        std::vector<DEBUG_ENT*> m_children;
        KIGFX::COLOR4D          m_color;
        int                     m_width;
        bool m_hasLabels = true;
        int                     m_iter;
        std::string             m_name;
        std::string             m_msg;
        PNS::DEBUG_DECORATOR::SRC_LOCATION_INFO m_srcLoc;
        bool m_visible;
        bool m_selected;
    };

    struct STAGE
    {
        STAGE()
        {
            m_name = "<unknown>";
            m_iter = 0;
            m_entries = new DEBUG_ENT();
        }

        ~STAGE()
        {
        }

        std::string             m_name;
        int                     m_iter;
        DEBUG_ENT*              m_entries;
    };

    PNS_TEST_DEBUG_DECORATOR()
    {
        m_iter = 0;
        m_grouping = false;
        m_activeEntry = nullptr;
        SetDebugEnabled( true );
    }

    virtual ~PNS_TEST_DEBUG_DECORATOR()
    {
        // fixme: I know it's a hacky tool but it should clean after itself at some point...

    }

    virtual void SetIteration( int iter ) override { m_iter = iter; }

    virtual void Message( const wxString& msg,
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void NewStage( const std::string& name, int iter,
                           const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void BeginGroup( const std::string& name,
                             const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void EndGroup( const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void AddPoint( VECTOR2I aP, const KIGFX::COLOR4D& aColor, int aSize,
                           const std::string& aName,
                           const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void AddLine( const SHAPE_LINE_CHAIN& aLine, const KIGFX::COLOR4D& aColor,
                          int aWidth, const std::string& aName,
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void AddSegment( SEG aS, const KIGFX::COLOR4D& aColor,
                             const std::string& aName,
                             const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void AddBox( BOX2I aB, const KIGFX::COLOR4D& aColor,
                         const std::string& aName,
                         const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void Clear(){};

    int GetStageCount() const { return m_stages.size(); }

    STAGE* GetStage( int index ) { return m_stages[index]; }

    BOX2I GetStageExtents( int stage ) const;

private:
    void addEntry( DEBUG_ENT* ent );

    bool               m_grouping;
    DEBUG_ENT*         m_activeEntry;
    STAGE*             currentStage();
    int                m_iter;
    std::vector<STAGE*> m_stages;
};


class PNS_TEST_ENVIRONMENT
{
public:
    PNS_TEST_ENVIRONMENT();
    ~PNS_TEST_ENVIRONMENT();

    void SetMode( PNS::ROUTER_MODE mode );
    void ReplayLog( PNS_LOG_FILE* aLog, int aStartEventIndex = 0, int aFrom = 0, int aTo = -1 );

    PNS_TEST_DEBUG_DECORATOR* GetDebugDecorator() { return &m_debugDecorator; };

private:
    void createRouter();

    PNS::ROUTER_MODE                      m_mode;
    PNS_TEST_DEBUG_DECORATOR              m_debugDecorator;
    std::shared_ptr<BOARD>                m_board;
    std::unique_ptr<PNS_KICAD_IFACE_BASE> m_iface;
    std::unique_ptr<PNS::ROUTER>          m_router;
};


#endif
