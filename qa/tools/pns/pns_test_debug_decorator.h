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

#ifndef __PNS_TEST_DEBUG_DECORATOR_H
#define __PNS_TEST_DEBUG_DECORATOR_H

#include <geometry/shape.h>

#include <router/pns_debug_decorator.h>

class REPORTER;

class PNS_DEBUG_SHAPE
{
public:
    PNS_DEBUG_SHAPE( PNS_DEBUG_SHAPE* aParent = nullptr );
    ~PNS_DEBUG_SHAPE();

    PNS_DEBUG_SHAPE* NewChild();
    void             AddChild( PNS_DEBUG_SHAPE* ent );
    bool             IsVisible() const;
    void             IterateTree( std::function<bool( PNS_DEBUG_SHAPE* )> visitor, int depth = 0 );

    PNS_DEBUG_SHAPE*                        m_parent;
    std::vector<SHAPE*>                     m_shapes;
    std::vector<PNS_DEBUG_SHAPE*>           m_children;
    KIGFX::COLOR4D                          m_color;
    int                                     m_width;
    bool                                    m_hasLabels = true;
    int                                     m_iter;
    wxString                                m_name;
    wxString                                m_msg;
    PNS::DEBUG_DECORATOR::SRC_LOCATION_INFO m_srcLoc;
    bool                                    m_visible;
    bool                                    m_selected;
    bool                                    m_filterMatch;
    int                                     m_level;
};

struct PNS_DEBUG_STAGE
{
    PNS_DEBUG_STAGE();
    ~PNS_DEBUG_STAGE();

    wxString         m_name;
    int              m_iter;
    PNS_DEBUG_SHAPE* m_entries;
    bool             m_status;
};


class PNS_TEST_DEBUG_DECORATOR : public PNS::DEBUG_DECORATOR
{
public:
    PNS_TEST_DEBUG_DECORATOR( REPORTER* aReporter );
    virtual ~PNS_TEST_DEBUG_DECORATOR();

    virtual void SetIteration( int iter ) override { m_iter = iter; }

    virtual void Message( const wxString&          msg,
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void NewStage( const wxString& name, int iter,
                           const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void BeginGroup( const wxString& name, int aLevel = 0,
                             const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void EndGroup( const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;

    virtual void AddPoint( const VECTOR2I& aP, const KIGFX::COLOR4D& aColor, int aSize,
                           const wxString&          aName = wxT( "" ),
                           const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;

    virtual void AddItem( const PNS::ITEM* aItem, const KIGFX::COLOR4D& aColor,
                          int aOverrideWidth = 0, const wxString& aName = wxT( "" ),
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;

    virtual void AddShape( const SHAPE* aShape, const KIGFX::COLOR4D& aColor,
                           int aOverrideWidth = 0, const wxString& aName = wxT( "" ),
                           const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;

    virtual void Clear() override{};

    int GetStageCount() const { return m_stages.size(); }

    PNS_DEBUG_STAGE* GetStage( int index ) { return m_stages[index]; }

    BOX2I GetStageExtents( int stage ) const;

    void SetCurrentStageStatus( bool stat );

private:
    void addEntry( PNS_DEBUG_SHAPE* ent );
    PNS_DEBUG_STAGE*              currentStage();

    bool                          m_grouping;
    PNS_DEBUG_SHAPE*              m_activeEntry;
    int                           m_iter;
    std::vector<PNS_DEBUG_STAGE*> m_stages;

    REPORTER*                     m_reporter;
};

#endif
