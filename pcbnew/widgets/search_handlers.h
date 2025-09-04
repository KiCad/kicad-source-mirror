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

#ifndef SEARCH_HANDLERS_H
#define SEARCH_HANDLERS_H

#include <widgets/search_pane.h>
#include <board_statistics.h>
#include <unordered_map>

class PCB_EDIT_FRAME;


class PCB_SEARCH_HANDLER : public SEARCH_HANDLER
{
public:
    PCB_SEARCH_HANDLER( const wxString& aName, PCB_EDIT_FRAME* aFrame ) :
            SEARCH_HANDLER( aName ),
            m_frame( aFrame )
    {}

    wxString GetResultCell( int aRow, int aCol ) override
    {
        if( m_frame->IsClosing() )
            return wxEmptyString;

        if( aRow >= static_cast<int>(m_hitlist.size() ) )
            return wxEmptyString;

        BOARD_ITEM* item = m_hitlist[aRow];

        if( !item )
            return wxEmptyString;

        return getResultCell( item, aCol );
    }

    void Sort( int aCol, bool aAscending, std::vector<long>* aSelection ) override;

    void SelectItems( std::vector<long>& aItemRows ) override;
    void ActivateItem( long aItemRow ) override;

protected:
    virtual wxString getResultCell( BOARD_ITEM* aItem, int aCol ) = 0;

protected:
    PCB_EDIT_FRAME*          m_frame;
    std::vector<BOARD_ITEM*> m_hitlist;
};


class FOOTPRINT_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    FOOTPRINT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

private:
    wxString getResultCell( BOARD_ITEM* aItem, int aCol ) override;
};


class ZONE_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    ZONE_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

private:
    wxString getResultCell( BOARD_ITEM* aItem, int aCol ) override;
};


class TEXT_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    TEXT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

private:
    wxString getResultCell( BOARD_ITEM* aItem, int aCol ) override;
};


class GROUP_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    GROUP_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

private:
    wxString getResultCell( BOARD_ITEM* aItem, int aCol ) override;
};


class NETS_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    NETS_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;
    void SelectItems( std::vector<long>& aItemRows ) override;
    void ActivateItem( long aItemRow ) override;

private:
    wxString getResultCell( BOARD_ITEM* aItem, int aCol ) override;
};


class RATSNEST_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    RATSNEST_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int  Search( const wxString& aQuery ) override;
    void SelectItems( std::vector<long>& aItemRows ) override;
    void ActivateItem( long aItemRow ) override;

private:
    wxString getResultCell( BOARD_ITEM* aItem, int aCol ) override;
};


class DRILL_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    DRILL_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int  Search( const wxString& aQuery ) override;
    void Sort( int aCol, bool aAscending, std::vector<long>* aSelection ) override;
    void SelectItems( std::vector<long>& aItemRows ) override;

private:
    wxString getResultCell( BOARD_ITEM* aItem, int aCol ) override;

    wxString cellText( const DRILL_LINE_ITEM& e, int col ) const;
    bool     rowMatchesQuery( const DRILL_LINE_ITEM& e, const wxString& aQuery ) const;

private:
    struct DRILL_ROW
    {
        DRILL_LINE_ITEM entry;
        // While a DRILL_ROW will usually represent multiple identical drills/BOARD_ITEMs,
        // keeping a pointer to one BOARD_ITEM allows use to provide
        // compatibility with the rest of PCB_SEARCH_HANDLER, and also to allow
        // some convenience actions to work when there is just a single entry, e.g.
        // activating the item to show its properties.
        BOARD_ITEM*                              item;
    };

    std::vector<DRILL_ROW>               m_drills;

    // This maps the DRILL_ROW.item to the index in m_drills to allow fast lookup
    std::unordered_map<BOARD_ITEM*, int> m_ptrToDrill;

    PCB_EDIT_FRAME* m_frame;
};

#endif
