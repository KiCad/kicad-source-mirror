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


#include "pns_test_debug_decorator.h"

#include <reporter.h>
#include <router/pns_item.h>


#define VERBOSE     // Sends PSN_DBG message output to the console


PNS_DEBUG_SHAPE::PNS_DEBUG_SHAPE( PNS_DEBUG_SHAPE* aParent )
{
    m_iter = 0;
    m_color = KIGFX::COLOR4D::WHITE;
    m_width = 10000;
    m_name = "<unknown>";
    m_parent = aParent;
    m_visible = true;
    m_selected = false;
    m_level = 0;
}

PNS_DEBUG_SHAPE::~PNS_DEBUG_SHAPE()
{
    for( SHAPE* shape : m_shapes )
        delete shape;

    for( PNS_DEBUG_SHAPE* child : m_children )
        delete child;
}


PNS_DEBUG_SHAPE* PNS_DEBUG_SHAPE::NewChild()
{
    PNS_DEBUG_SHAPE* ent = new PNS_DEBUG_SHAPE( this );
    m_children.push_back( ent );

    return ent;
}


void PNS_DEBUG_SHAPE::AddChild( PNS_DEBUG_SHAPE* ent )
{
    ent->m_parent = this;
    m_children.push_back( ent );
}


bool PNS_DEBUG_SHAPE::IsVisible() const
{
    if( m_visible )
        return true;

    PNS_DEBUG_SHAPE* parent = m_parent;

    while( parent )
    {
        if( parent->m_visible )
            return true;

        parent = parent->m_parent;
    }

    return false;
}

void PNS_DEBUG_SHAPE::IterateTree( std::function<bool( PNS_DEBUG_SHAPE* )> visitor, int depth )
{
    if( !visitor( this ) )
        return;


    for( PNS_DEBUG_SHAPE* child : m_children )
        child->IterateTree( visitor, depth + 1 );
}


PNS_DEBUG_STAGE::PNS_DEBUG_STAGE()
{
    m_name = "<unknown>";
    m_iter = 0;
    m_entries = new PNS_DEBUG_SHAPE();
    m_status = false;
}

PNS_DEBUG_STAGE::~PNS_DEBUG_STAGE()
{
    delete m_entries;
}


PNS_TEST_DEBUG_DECORATOR::PNS_TEST_DEBUG_DECORATOR( REPORTER* aReporter ) :
    m_reporter( aReporter )
{
    m_iter = 0;
    m_grouping = false;
    m_activeEntry = nullptr;
    SetDebugEnabled( true );
}


PNS_TEST_DEBUG_DECORATOR::~PNS_TEST_DEBUG_DECORATOR()
{
    for( PNS_DEBUG_STAGE* stage : m_stages )
        delete stage;
    // fixme: I know it's a hacky tool but it should clean after itself at some point...
}


PNS_DEBUG_STAGE* PNS_TEST_DEBUG_DECORATOR::currentStage()
{
    if( m_stages.empty() )
        m_stages.push_back( new PNS_DEBUG_STAGE() );

    return m_stages.back();
}


void PNS_TEST_DEBUG_DECORATOR::BeginGroup( const wxString& name, int aLevel,
                                           const SRC_LOCATION_INFO& aSrcLoc )
{
    PNS_DEBUG_STAGE* stage = currentStage();
    PNS_DEBUG_SHAPE* ent = new PNS_DEBUG_SHAPE();

    ent->m_name = name;
    ent->m_iter = m_iter;
    ent->m_level = aLevel;

    if( m_activeEntry )
        m_activeEntry->AddChild( ent );

    m_activeEntry = ent;
    m_grouping = true;
}


void PNS_TEST_DEBUG_DECORATOR::EndGroup( const SRC_LOCATION_INFO& aSrcLoc )
{
    if( !m_activeEntry )
        return;

    m_activeEntry = m_activeEntry->m_parent;

    if( !m_activeEntry )
        m_grouping = false;
}


void PNS_TEST_DEBUG_DECORATOR::addEntry( PNS_DEBUG_SHAPE* ent )
{
    auto st = currentStage();
    m_activeEntry->AddChild( ent );
}


void PNS_TEST_DEBUG_DECORATOR::AddPoint( const VECTOR2I& aP, const KIGFX::COLOR4D& aColor,
                                         int aSize, const wxString& aName,
                                         const SRC_LOCATION_INFO& aSrcLoc )
{
    SHAPE_LINE_CHAIN* sh = new SHAPE_LINE_CHAIN;

    sh->Append( aP.x - aSize, aP.y - aSize );
    sh->Append( aP.x + aSize, aP.y + aSize );
    sh->Append( aP.x, aP.y );
    sh->Append( aP.x - aSize, aP.y + aSize );
    sh->Append( aP.x + aSize, aP.y - aSize );

    PNS_DEBUG_SHAPE* ent = new PNS_DEBUG_SHAPE();

    ent->m_shapes.push_back( sh );
    ent->m_color = aColor;
    ent->m_width = 30000;
    ent->m_iter = m_iter;
    ent->m_name = aName;
    ent->m_hasLabels = false;
    ent->m_srcLoc = aSrcLoc;

    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::AddItem( const PNS::ITEM* aItem, const KIGFX::COLOR4D& aColor,
                                        int aOverrideWidth, const wxString& aName,
                                        const SRC_LOCATION_INFO& aSrcLoc )
{
    // TODO(JE) padstacks
    SHAPE*           sh = aItem->Shape( -1 )->Clone();
    PNS_DEBUG_SHAPE* ent = new PNS_DEBUG_SHAPE();

    ent->m_shapes.push_back( sh );
    ent->m_color = aColor;
    ent->m_width = aOverrideWidth;
    ent->m_name = aName;
    ent->m_iter = m_iter;
    ent->m_srcLoc = aSrcLoc;

    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::AddShape( const SHAPE* aShape, const KIGFX::COLOR4D& aColor,
                                         int aOverrideWidth, const wxString& aName,
                                         const SRC_LOCATION_INFO& aSrcLoc )
{
    SHAPE*           sh = aShape->Clone();
    PNS_DEBUG_SHAPE* ent = new PNS_DEBUG_SHAPE();

    ent->m_shapes.push_back( sh );
    ent->m_color = aColor;
    ent->m_width = aOverrideWidth;
    ent->m_name = aName;
    ent->m_iter = m_iter;
    ent->m_srcLoc = aSrcLoc;

    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::Message( const wxString& msg, const SRC_LOCATION_INFO& aSrcLoc )
{
    PNS_DEBUG_SHAPE* ent = new PNS_DEBUG_SHAPE();
    ent->m_msg = msg.c_str();
    ent->m_srcLoc = aSrcLoc;
    addEntry( ent );

    printf("PNS: %s\n", msg.c_str().AsChar() );

#ifdef VERBOSE
    static wxString lastMsg;

    if( msg != lastMsg )
    {
        m_reporter->Report( msg );
        lastMsg = msg;
    }
#endif
}


void PNS_TEST_DEBUG_DECORATOR::NewStage( const wxString& name, int iter,
                                         const SRC_LOCATION_INFO& aSrcLoc )
{
    PNS_DEBUG_STAGE* stage = new PNS_DEBUG_STAGE();
    stage->m_name = name;
    stage->m_iter = iter;

    m_stages.push_back( stage );
    m_activeEntry = m_stages.back()->m_entries;
}

void PNS_TEST_DEBUG_DECORATOR::SetCurrentStageStatus( bool stat )
{
    if( m_stages.empty() )
        return;

    m_stages.back()->m_status = stat;
}


BOX2I PNS_TEST_DEBUG_DECORATOR::GetStageExtents( int stage ) const
{
    PNS_DEBUG_STAGE* st = m_stages[stage];
    BOX2I  bb;
    bool   first = true;

    auto visitor =
            [&]( PNS_DEBUG_SHAPE* ent ) -> bool
            {
                for( SHAPE* sh : ent->m_shapes )
                {
                    if( first )
                        bb = sh->BBox();
                    else
                        bb.Merge( sh->BBox() );

                    first = false;
                }

                return true;
            };

    return bb;
}
