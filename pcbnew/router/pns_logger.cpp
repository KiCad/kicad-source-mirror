/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pns_logger.h"
#include "pns_item.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_segment.h"
#include "pns_solid.h"

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>

PNS_LOGGER::PNS_LOGGER( )
{
    m_groupOpened = false;
}


PNS_LOGGER::~PNS_LOGGER()
{
}


void PNS_LOGGER::Clear()
{
    m_theLog.str( std::string() );
    m_groupOpened = false;
}


void PNS_LOGGER::NewGroup( const std::string& aName, int aIter )
{
    if( m_groupOpened )
        m_theLog << "endgroup" << std::endl;

    m_theLog << "group " << aName << " " << aIter << std::endl;
    m_groupOpened = true;
}


void PNS_LOGGER::EndGroup()
{
    if( !m_groupOpened )
        return;

    m_groupOpened = false;
    m_theLog << "endgroup" << std::endl;
}


void PNS_LOGGER::Log ( const PNS_ITEM* aItem, int aKind, const std::string aName )
{
    m_theLog << "aItem " << aKind << " " << aName << " ";
    m_theLog << aItem->Net() << " " << aItem->Layers().Start() << " " <<
                aItem->Layers().End() << " " << aItem->Marker() << " " << aItem->Rank();

    switch( aItem->Kind() )
    {
        case PNS_ITEM::LINE:
        {
            PNS_LINE* l = (PNS_LINE*) aItem;
            m_theLog << " line ";
            m_theLog << l->Width() << " " << ( l->EndsWithVia() ? 1 : 0 ) << " ";
            dumpShape ( l->Shape() );
            m_theLog << std::endl;
            break;
        }

        case PNS_ITEM::VIA:
        {
            m_theLog << " via 0 0 ";
            dumpShape ( aItem->Shape() );
            m_theLog << std::endl;
            break;
        }

        case PNS_ITEM::SEGMENT:
        {
            PNS_SEGMENT* s =(PNS_SEGMENT*) aItem;
            m_theLog << " line ";
            m_theLog << s->Width() << " 0 linechain 2 0 " << s->Seg().A.x << " " <<
                        s->Seg().A.y << " " << s->Seg().B.x << " " <<s->Seg().B.y << std::endl;
            break;
        }

        case PNS_ITEM::SOLID:
        {
            PNS_SOLID* s = (PNS_SOLID*) aItem;
            m_theLog << " solid 0 0 ";
            dumpShape( s->Shape() );
            m_theLog << std::endl;
            break;
        }

        default:
            break;
    }
}


void PNS_LOGGER::Log( const SHAPE_LINE_CHAIN *aL, int aKind, const std::string aName )
{
    m_theLog << "item " << aKind << " " << aName << " ";
    m_theLog << 0 << " " << 0 << " " << 0 << " " << 0 << " " << 0;
    m_theLog << " line ";
    m_theLog << 0 << " " << 0 << " ";
    dumpShape( aL );
    m_theLog << std::endl;
}


void PNS_LOGGER::Log( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                      int aKind, const std::string aName)
{
}


void PNS_LOGGER::dumpShape( const SHAPE* aSh )
{
    switch( aSh->Type() )
    {
        case SH_LINE_CHAIN:
        {
            const SHAPE_LINE_CHAIN* lc = (const SHAPE_LINE_CHAIN*) aSh;
            m_theLog << "linechain " << lc->PointCount() << " " << ( lc->IsClosed() ? 1 : 0 ) << " ";

            for( int i = 0; i < lc->PointCount(); i++ )
                m_theLog << lc->CPoint( i ).x << " " << lc->CPoint( i ).y << " ";

            break;
        }

        case SH_CIRCLE:
        {
            const SHAPE_CIRCLE *c = (const SHAPE_CIRCLE*) aSh;
            m_theLog << "circle " << c->GetCenter().x << " " << c->GetCenter().y << " " << c->GetRadius();
            break;
        }

        case SH_RECT:
        {
            const SHAPE_RECT* r = (const SHAPE_RECT*) aSh;
            m_theLog << "rect " << r->GetPosition().x << " " << r->GetPosition().y << " " <<
                        r->GetSize().x << " " <<r->GetSize().y;
            break;
        }

        case SH_SEGMENT:
        {
            const SHAPE_SEGMENT* s = (const SHAPE_SEGMENT*) aSh;
            m_theLog << "linechain 2 0 " << s->GetSeg().A.x << " " << s->GetSeg().A.y << " " <<
                        s->GetSeg().B.x << " " << s->GetSeg().B.y;
            break;
        }

        default:
            break;
    }
}


void PNS_LOGGER::Save( const std::string& aFilename )
{
    EndGroup();

    FILE* f = fopen( aFilename.c_str(), "wb" );
    printf( "Saving to '%s' [%p]\n", aFilename.c_str(), f );
    const std::string s = m_theLog.str();
    fwrite( s.c_str(), 1, s.length(), f );
    fclose( f );
}
