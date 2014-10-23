/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
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

 /**
  * @file UnitSelector.h
  * a wxChoiceBox to select units in Pcb_Calculator
  */

#ifndef _UnitSelector_h_
#define _UnitSelector_h_


#include <wx/string.h>
#include <wx/choice.h>


class UNIT_SELECTOR: public wxChoice
{
public:
    UNIT_SELECTOR( wxWindow *parent, wxWindowID id,
                   const wxPoint& pos, const wxSize& size,
                   const wxArrayString& choices, long style = 0 ):
            wxChoice( parent, id, pos, size, choices, style )
    {
    }

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (meter, herz, ohm, radian )
     */
    virtual double GetUnitScale() = 0;

    wxString GetUnitName()
    {
        return GetStringSelection();
    }
};

class UNIT_SELECTOR_LEN: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_LEN( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (meter)
     */
    virtual double GetUnitScale();
};

class UNIT_SELECTOR_FREQUENCY: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_FREQUENCY( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (Hz)
     */
    virtual double GetUnitScale();
};

class UNIT_SELECTOR_ANGLE: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_ANGLE( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (Hz)
     */
    virtual double GetUnitScale();
};

class UNIT_SELECTOR_RESISTOR: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_RESISTOR( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (Hz)
     */
    virtual double GetUnitScale();
};

#endif  // _UnitSelector_h_

