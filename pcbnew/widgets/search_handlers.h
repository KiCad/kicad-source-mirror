/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

class ZONE;
class FOOTPRINT;
class PCB_TEXT;
class PCB_EDIT_FRAME;


class PCB_SEARCH_HANDLER : public SEARCH_HANDLER
{
public:
    PCB_SEARCH_HANDLER( wxString aName, PCB_EDIT_FRAME* aFrame );
    void ActivateItem( long aItemRow ) override;

protected:
    PCB_EDIT_FRAME* m_frame;
};


class FOOTPRINT_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    FOOTPRINT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;
    wxString GetResultCell( int aRow, int aCol ) override;
    void     SelectItems( std::vector<long>& aItemRows ) override;

private:
    std::vector<FOOTPRINT*> m_hitlist;
};


class ZONE_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    ZONE_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int      Search( const wxString& aQuery ) override;
    wxString GetResultCell( int aRow, int aCol ) override;
    void     SelectItems( std::vector<long>& aItemRows ) override;

private:
    std::vector<ZONE*> m_hitlist;
};


class TEXT_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    TEXT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int      Search( const wxString& aQuery ) override;
    wxString GetResultCell( int aRow, int aCol ) override;
    void     SelectItems( std::vector<long>& aItemRows ) override;

private:
    std::vector<BOARD_ITEM*> m_hitlist;
};


class NETS_SEARCH_HANDLER : public PCB_SEARCH_HANDLER
{
public:
    NETS_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;
    wxString GetResultCell( int aRow, int aCol ) override;
    void     SelectItems( std::vector<long>& aItemRows ) override;
    void     ActivateItem( long aItemRow ) override;

private:
    std::vector<NETINFO_ITEM*> m_hitlist;
};

#endif
