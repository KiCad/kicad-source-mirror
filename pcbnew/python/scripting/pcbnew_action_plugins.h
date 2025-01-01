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

/**
 * @file  pcbnew_action_plugins.h
 * @brief Class PCBNEW_ACTION_PLUGINS
 */

#ifndef PCBNEW_ACTION_PLUGINS_H
#define PCBNEW_ACTION_PLUGINS_H

#undef HAVE_CLOCK_GETTIME  // macro is defined in Python.h and causes redefine warning
#include <Python.h>
#undef HAVE_CLOCK_GETTIME

#include <vector>
#include <action_plugin.h>


class PYTHON_ACTION_PLUGIN : public ACTION_PLUGIN
{
public:
    PYTHON_ACTION_PLUGIN( PyObject* action );
    ~PYTHON_ACTION_PLUGIN();
    wxString    GetCategoryName() override;
    wxString    GetClassName() override;
    wxString    GetName() override;
    wxString    GetDescription() override;
    bool        GetShowToolbarButton() override;
    wxString    GetIconFileName( bool aDark ) override;
    wxString    GetPluginPath() override;
    void        Run() override;
    void*       GetObject() override;

private:
    wxString  m_cachedName;

    PyObject* m_PyAction;
    PyObject* CallMethod( const char* aMethod, PyObject* aArglist = nullptr );
    wxString  CallRetStrMethod( const char* aMethod, PyObject* aArglist = nullptr );
};


class PYTHON_ACTION_PLUGINS
{
public:
    static void register_action( PyObject* aPyAction );
    static void deregister_action( PyObject* aPyAction );
};

#endif /* PCBNEW_ACTION_PLUGINS_H */
