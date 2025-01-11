/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 3Dconnexion
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file  nl_footprint_properties_plugin_impl.h
 * @brief declaration of the nl_footprint_properties_plugin_impl class
 */

#ifndef NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL_H_
#define NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL_H_

#include "nl_3d_viewer_plugin_impl.h"
// TDxWare SDK.
#include <SpaceMouse/CNavigation3D.hpp>

/**
 * The class that adjusts NL_3D_VIEWER_PLUGIN_IMPL implementation for 3D Model preview in
 * footprint properties dialog.
 */
class NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL : public NL_3D_VIEWER_PLUGIN_IMPL
{
public:
    /**
     * Initialize a new instance of the NL_FOOTPRINT_PROPERTIES_PLUGIN.
     *
     *  @param aCanvas is the viewport to be navigated.
     */
    NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL( EDA_3D_CANVAS* aCanvas );

private:
    /**
     * Get Footprint 3D Model extents.
     *
     *  @param extents is the box around the 3D model.
     */
    long GetModelExtents( navlib::box_t& extents ) const override;

    /**
     * Export the invocable actions and images to the 3Dconnexion UI.
     */
    void exportCommandsAndImages() override;

private:
    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_NL_FOOTPRINT_PROPERTIES_PLUGIN".  See the wxWidgets documentation on
     *  wxLogTrace for more information.
     */
    static const wxChar* m_logTrace;
};
#endif // NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL_H_
