/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#pragma once

#include <board.h>
#include <board_commit.h>
#include <drc/drc_engine.h>
#include <pcb_marker.h>

#include <functional>
#include <set>

class DRC_ENGINE;
class DRC_TEST_PROVIDER;
class DRC_RULE;
class DRC_CONSTRAINT;

class DRC_TEST_PROVIDER_REGISTRY
{
public:
    DRC_TEST_PROVIDER_REGISTRY() {}

    ~DRC_TEST_PROVIDER_REGISTRY();

    static DRC_TEST_PROVIDER_REGISTRY& Instance()
    {
        static DRC_TEST_PROVIDER_REGISTRY self;
        return self;
    }

    void RegisterTestProvider( DRC_TEST_PROVIDER* provider ) { m_providers.push_back( provider ); }
    std::vector<DRC_TEST_PROVIDER*> GetTestProviders() const { return m_providers; }

private:
    std::vector<DRC_TEST_PROVIDER*> m_providers;
};

template<class T> class DRC_REGISTER_TEST_PROVIDER
{
public:
    DRC_REGISTER_TEST_PROVIDER()
    {
        T* provider = new T;
        DRC_TEST_PROVIDER_REGISTRY::Instance().RegisterTestProvider( provider );
    }
};


class DRC_SHOWMATCHES_PROVIDER_REGISTRY
{
public:
    DRC_SHOWMATCHES_PROVIDER_REGISTRY() {}

    ~DRC_SHOWMATCHES_PROVIDER_REGISTRY();

    static DRC_SHOWMATCHES_PROVIDER_REGISTRY& Instance()
    {
        static DRC_SHOWMATCHES_PROVIDER_REGISTRY self;
        return self;
    }

    void RegisterShowMatchesProvider( DRC_TEST_PROVIDER* provider ) { m_providers.push_back( provider ); }
    std::vector<DRC_TEST_PROVIDER*> GetShowMatchesProviders() const { return m_providers; }

private:
    std::vector<DRC_TEST_PROVIDER*> m_providers;
};

template<class T> class DRC_REGISTER_SHOWMATCHES_PROVIDER
{
public:
    DRC_REGISTER_SHOWMATCHES_PROVIDER()
    {
        T* provider = new T;
        DRC_SHOWMATCHES_PROVIDER_REGISTRY::Instance().RegisterShowMatchesProvider( provider );
    }
};


/**
 * Represent a DRC "provider" which runs some DRC functions over a #BOARD and spits out
 * #DRC_ITEM and positions as needed.
 */
class DRC_TEST_PROVIDER : public UNITS_PROVIDER
{
public:
    DRC_TEST_PROVIDER();
    virtual ~DRC_TEST_PROVIDER() = default;

    static void Init();

    void SetDRCEngine( DRC_ENGINE *engine )
    {
        m_drcEngine = engine;
    }

    bool RunTests( EDA_UNITS aUnits )
    {
        SetUserUnits( aUnits );
        return Run();
    }

    /**
     * Run this provider against the given PCB with configured options (if any).
     */
    virtual bool Run() = 0;

    virtual const wxString GetName() const;

protected:
    int forEachGeometryItem( const std::vector<KICAD_T>& aTypes, const LSET& aLayers,
                             const std::function<bool(BOARD_ITEM*)>& aFunc );

    REPORTER* getLogReporter() const { return m_drcEngine->GetLogReporter(); }

#define REPORT_AUX( s ) if( getLogReporter() ) getLogReporter()->Report( s, RPT_SEVERITY_INFO )

    void reportViolation( std::shared_ptr<DRC_ITEM>& item, const VECTOR2I& aMarkerPos,
                          int aMarkerLayer,
                          const std::function<void( PCB_MARKER* )>& aPathGenerator = []( PCB_MARKER* ){} );

    void reportTwoPointGeometry( std::shared_ptr<DRC_ITEM>& aDrcItem, const VECTOR2I& aMarkerPos,
                                 const VECTOR2I& ptA, const VECTOR2I& ptB, PCB_LAYER_ID aLayer );

    void reportTwoShapeGeometry( std::shared_ptr<DRC_ITEM>& aDrcItem, const VECTOR2I& aMarkerPos,
                                 const SHAPE* aShape1, const SHAPE* aShape2, PCB_LAYER_ID aLayer,
                                 int aDistance );

    void reportTwoItemGeometry( std::shared_ptr<DRC_ITEM>& aDrcItem, const VECTOR2I& aMarkerPos,
                                const BOARD_ITEM* aItem1, const BOARD_ITEM* aItem2, PCB_LAYER_ID aLayer,
                                int aDistance );

    virtual bool reportProgress( size_t aCount, size_t aSize, size_t aDelta = 1 );
    virtual bool reportPhase( const wxString& aStageName );

    bool isInvisibleText( const BOARD_ITEM* aItem ) const;

    wxString formatMsg( const wxString& aFormatString, const wxString& aSource, double aConstraint,
                        double aActual, EDA_DATA_TYPE aDataType = EDA_DATA_TYPE::DISTANCE );
    wxString formatMsg( const wxString& aFormatString, const wxString& aSource,
                        const EDA_ANGLE& aConstraint, const EDA_ANGLE& aActual );

    // List of basic (ie: non-compound) geometry items
    static std::vector<KICAD_T> s_allBasicItems;
    static std::vector<KICAD_T> s_allBasicItemsButZones;

protected:
    DRC_ENGINE* m_drcEngine;
    BOARD*      m_board;
    bool        m_isRuleDriven = true;
};
