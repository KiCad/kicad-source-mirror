/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
 * Copyright (C) 2013 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <Python.h>
#include <vector>
#include <class_footprint_wizard.h>


class PYTHON_FOOTPRINT_WIZARD : public FOOTPRINT_WIZARD
{
    PyObject* m_PyWizard;
    PyObject*       CallMethod( const char* aMethod, PyObject* aArglist = NULL );
    wxString        CallRetStrMethod( const char* aMethod, PyObject* aArglist = NULL );
    wxArrayString   CallRetArrayStrMethod( const char*  aMethod,
                                           PyObject*    aArglist = NULL );

public:
    PYTHON_FOOTPRINT_WIZARD( PyObject* wizard );
    ~PYTHON_FOOTPRINT_WIZARD();
    wxString        GetName();
    wxString        GetImage();
    wxString        GetDescription();
    int             GetNumParameterPages();
    wxString        GetParameterPageName( int aPage );
    wxArrayString   GetParameterNames( int aPage );
    wxArrayString   GetParameterTypes( int aPage );
    wxArrayString   GetParameterValues( int aPage );
    wxArrayString   GetParameterErrors( int aPage );
    // must return an empty string or an error description
    wxString        SetParameterValues( int aPage, wxArrayString& aValues );
    MODULE*         GetFootprint( wxString * aMessages );
    void*           GetObject();
};


class PYTHON_FOOTPRINT_WIZARDS
{
public:
    static void register_wizard( PyObject* aPyWizard );
    static void deregister_wizard( PyObject* aPyWizard );
};

#endif /* PCBNEW_FOOTPRINT_WIZARDS_H */
