/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
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

/**
 * @file  pcbnew_footprint_wizards.h
 * @brief Class PCBNEW_FOOTPRINT_WIZARDS
 */

#ifndef PCBNEW_FOOTPRINT_WIZARDS_H
#define PCBNEW_FOOTPRINT_WIZARDS_H

#undef HAVE_CLOCK_GETTIME  // macro is defined in Python.h and causes redefine warning
#include <Python.h>
#undef HAVE_CLOCK_GETTIME

#include <vector>
#include <footprint_wizard.h>


class PYTHON_FOOTPRINT_WIZARD : public FOOTPRINT_WIZARD
{
public:
    PYTHON_FOOTPRINT_WIZARD( PyObject* wizard );
    ~PYTHON_FOOTPRINT_WIZARD();

    wxString        GetName() override;
    wxString        GetImage() override;
    wxString        GetDescription() override;
    int             GetNumParameterPages() override;
    wxString        GetParameterPageName( int aPage ) override;
    wxArrayString   GetParameterNames( int aPage ) override;
    wxArrayString   GetParameterTypes( int aPage ) override;
    wxArrayString   GetParameterValues( int aPage ) override;
    wxArrayString   GetParameterErrors( int aPage ) override;

    // must return an empty string or an error description:
    wxString        SetParameterValues( int aPage, wxArrayString& aValues ) override;
    FOOTPRINT*      GetFootprint( wxString* aMessages ) override;
    void*           GetObject() override;
    wxArrayString   GetParameterHints( int aPage ) override;
    wxArrayString   GetParameterDesignators( int aPage = 0 ) override;

    void            ResetParameters() override;

private:
    PyObject*       CallMethod( const char* aMethod, PyObject* aArglist = nullptr );
    wxString        CallRetStrMethod( const char* aMethod, PyObject* aArglist = nullptr );
    wxArrayString   CallRetArrayStrMethod( const char* aMethod, PyObject* aArglist = nullptr );

    PyObject* m_PyWizard;
};


class PYTHON_FOOTPRINT_WIZARD_LIST
{
public:
    static void register_wizard( PyObject* aPyWizard );
    static void deregister_wizard( PyObject* aPyWizard );
};

#endif /* PCBNEW_FOOTPRINT_WIZARDS_H */
