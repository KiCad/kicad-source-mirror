/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TOOLS_MULTICHANNEL_TOOL_H
#define TOOLS_MULTICHANNEL_TOOL_H

#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <tools/pcb_tool_base.h>

#include <pad.h>
#include <footprint.h>
#include <reporter.h>
#include <core/profile.h>

typedef std::vector<std::pair<FOOTPRINT*, FOOTPRINT*> > FP_PAIRS;

enum class REPEAT_LAYOUT_EDGE_MODE
{
    INSIDE = 0,
    TOUCHING,
    CLIP
};

struct REPEAT_LAYOUT_OPTIONS
{
    bool m_copyRouting = true;
    bool m_copyPlacement = true;
    bool m_groupItems = false;
    bool m_moveOffRAComponents = true;
    bool m_includeLockedItems = true;
    bool m_keepOldRouting = false;
    REPEAT_LAYOUT_EDGE_MODE m_edgeMode;
};

struct PAD_PREFIX_ENTRY
{
    PAD_PREFIX_ENTRY( PAD* pad_, wxString pfx_ ):
        pad(pad_), componentPrefix( pfx_ ), processed( false ) {};
    PAD* pad;
    wxString componentPrefix;
    bool processed;

    wxString format() const {
        return wxString::Format( wxT("%s-%s"), componentPrefix, pad->GetNumber() );
    }

    bool matchesPadNumberAndPrefix( const PAD_PREFIX_ENTRY& aOther ) const
    {
        if( componentPrefix != aOther.componentPrefix )
            return false;

        if( pad->GetNumber() != aOther.pad->GetNumber() )
            return false;

        return true;
    }
};

struct FP_WITH_CONNECTIONS
{
    FOOTPRINT* fp;
    std::unordered_map<PAD*, std::vector<PAD_PREFIX_ENTRY> > connsWithinRA;
    bool processed;

    void sortByPadNumbers()
    {
        // fixme
        std::unordered_map<PAD*, std::vector<PAD_PREFIX_ENTRY> > sorted;
        for( auto& ent : connsWithinRA )
        {
            auto v = ent.second;
            auto compare = [] ( const PAD_PREFIX_ENTRY& a, const PAD_PREFIX_ENTRY& b ) -> int
            {
                if( a.pad->GetNumber() > b.pad->GetNumber() )
                    return 0;
                else if( a.pad->GetNumber() == b.pad->GetNumber() )
                    return a.componentPrefix < b.componentPrefix;
                else
                    return a.pad->GetNumber() < b.pad->GetNumber();
            };

            std::sort( v.begin(), v.end(), compare );
            sorted[ ent.first ] = v;
        }

        connsWithinRA = sorted;
    }
};

struct RULE_AREA;

struct RULE_AREA_COMPAT_DATA
{
    RULE_AREA* m_refArea;
    bool m_isOk;
    bool m_doCopy;
    wxString m_errorMsg;
    std::vector< std::pair<FOOTPRINT*, FOOTPRINT*> > m_matchingFootprints;
};

struct RULE_AREA
    {
        ZONE* m_area;
        std::vector<FP_WITH_CONNECTIONS> m_raFootprints;
        std::set<FOOTPRINT*> m_sheetComponents;
        std::map<PAD*, FOOTPRINT*> m_fpPads;
        bool m_existsAlready;
        bool m_generateEnabled;
        wxString m_sheetPath;
        wxString m_sheetName;
        wxString m_ruleName;
        VECTOR2I m_center;
    };


struct RA_SHEET
{
    bool m_generateEnabled;
    wxString m_sheetPath;
    wxString m_sheetName;
};


struct RULE_AREAS_DATA
{
    bool m_replaceExisting;
    bool m_groupItems;
    RULE_AREA* m_refRA;
    REPEAT_LAYOUT_OPTIONS m_options;

    std::vector<RULE_AREA> m_areas;
    std::unordered_map<RULE_AREA*, RULE_AREA_COMPAT_DATA> m_compatMap;

};



class MULTICHANNEL_TOOL : public PCB_TOOL_BASE
{
    public:
        MULTICHANNEL_TOOL();
        ~MULTICHANNEL_TOOL();

        RULE_AREAS_DATA* GetData() { return &m_areas; }

    private:
        void setTransitions() override;
        int autogenerateRuleAreas( const TOOL_EVENT& aEvent );
        int repeatLayout( const TOOL_EVENT& aEvent );
        wxString stripComponentIndex( wxString aRef ) const;
        bool identifyComponentsInRuleArea( ZONE* aRuleArea, std::set<FOOTPRINT*>& aComponents );
        const SHAPE_LINE_CHAIN buildRAOutline( std::set<FOOTPRINT*>& aFootprints, int aMargin );
        std::set<FOOTPRINT*> queryComponentsInSheet( wxString aSheetName );
        void findExistingRuleAreas();
        void querySheets();

        RULE_AREA* findRAByName( const wxString& aName );
        bool resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea, RULE_AREA_COMPAT_DATA& aMatches );
        bool copyRuleAreaContents( FP_PAIRS& aMatches, BOARD_COMMIT* aCommit, RULE_AREA* aRefArea, RULE_AREA* aTargetArea, REPEAT_LAYOUT_OPTIONS aOpts );

        std::unique_ptr<REPORTER> m_reporter;
        RULE_AREAS_DATA m_areas;
};



#endif // TOOLS_MULTICHANNEL_TOOL
