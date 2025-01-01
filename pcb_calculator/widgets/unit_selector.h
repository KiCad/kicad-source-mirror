/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
  * @file UnitSelector.h
  * a wxChoiceBox to select units in Pcb_Calculator
  */

#ifndef UNIT_SELECTOR_H
#define UNIT_SELECTOR_H


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
    double GetUnitScale() override;
};

class UNIT_SELECTOR_THICKNESS: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_THICKNESS( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (meter) including oz/ft^2
     */
    double GetUnitScale() override;
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
    double GetUnitScale() override;
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
    double GetUnitScale() override;
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
    double GetUnitScale() override;
};

class UNIT_SELECTOR_LINEAR_RESISTANCE : public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_LINEAR_RESISTANCE( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units ( ohm/m )
     */
    double GetUnitScale() override;
};

class UNIT_SELECTOR_LEN_CABLE : public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_LEN_CABLE( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units ( m )
     */
    double GetUnitScale() override;
};

class UNIT_SELECTOR_VOLTAGE : public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_VOLTAGE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                           const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units ( V )
     */
    double GetUnitScale() override;
};

class UNIT_SELECTOR_POWER : public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_POWER( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                         const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units ( W )
     */
    double GetUnitScale() override;
};

class UNIT_SELECTOR_SPEED : public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_SPEED( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                         const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units ( ohm/m )
     */
    double GetUnitScale() override;
};

class UNIT_SELECTOR_TIME : public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_TIME( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                        const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units ( ohm/m )
     */
    double GetUnitScale() override;
};

#endif  // UNIT_SELECTOR_H

