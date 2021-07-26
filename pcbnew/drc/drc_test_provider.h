/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef DRC_TEST_PROVIDER__H
#define DRC_TEST_PROVIDER__H

#include <board.h>
#include <pcb_marker.h>

#include <functional>
#include <set>

class DRC_ENGINE;
class DRC_TEST_PROVIDER;

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


/**
 * Represent a DRC "provider" which runs some DRC functions over a #BOARD and spits out
 * #DRC_ITEMs and positions as needed.
 */
class DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER ();
    virtual ~DRC_TEST_PROVIDER() = default;

    void SetDRCEngine( DRC_ENGINE *engine )
    {
        m_drcEngine = engine;
        m_stats.clear();
    }

    /**
     * Run this provider against the given PCB with configured options (if any).
     */
    virtual bool Run() = 0;

    virtual const wxString GetName() const;
    virtual const wxString GetDescription() const;

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const = 0;

    virtual int GetNumPhases() const = 0;

    virtual bool IsRuleDriven() const
    {
        return m_isRuleDriven;
    }

    bool IsEnabled() const
    {
        return m_enabled;
    }

    void Enable( bool aEnable )
    {
        m_enabled = aEnable;
    }

protected:
    int forEachGeometryItem( const std::vector<KICAD_T>& aTypes, LSET aLayers,
                             const std::function<bool(BOARD_ITEM*)>& aFunc );

    virtual void reportAux( wxString fmt, ... );
    virtual void reportViolation( std::shared_ptr<DRC_ITEM>& item, const wxPoint& aMarkerPos );
    virtual bool reportProgress( int aCount, int aSize, int aDelta );
    virtual bool reportPhase( const wxString& aStageName );

    virtual void reportRuleStatistics();
    virtual void accountCheck( const DRC_RULE* ruleToTest );
    virtual void accountCheck( const DRC_CONSTRAINT& constraintToTest );

    bool isInvisibleText( const BOARD_ITEM* aItem ) const;

    // List of basic (ie: non-compound) geometry items
    static std::vector<KICAD_T> s_allBasicItems;
    static std::vector<KICAD_T> s_allBasicItemsButZones;

    EDA_UNITS   userUnits() const;
    DRC_ENGINE* m_drcEngine;
    std::unordered_map<const DRC_RULE*, int> m_stats;
    bool        m_isRuleDriven = true;
    bool        m_enabled = true;

    wxString    m_msg;  // Allocating strings gets expensive enough to want to avoid it
};

#endif // DRC_TEST_PROVIDER__H
