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

#include "nl_footprint_properties_plugin_impl.h"
#include <3d-viewer/3d_canvas/eda_3d_canvas.h>

// KiCAD includes
#include <tool/action_manager.h>
#include <tool/tool_manager.h>
#include <tool/tools_holder.h>

#include <wx/mstream.h>

#define BOUNDING_BOX_SCALE_FACTOR 1.3f

/**
 * Flag to enable the NL_FOOTPRINT_PROPERTIES_PLUGIN debug tracing.
 *
 * Use "KI_TRACE_NL_FOOTPRINT_PROPERTIES_PLUGIN" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL::m_logTrace =
        wxT( "KI_TRACE_NL_FOOTPRINT_PROPERTIES_PLUGIN" );


NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL::NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL(EDA_3D_CANVAS* aCanvas) :
  NL_3D_VIEWER_PLUGIN_IMPL(aCanvas, "KiCAD Footprint Properties")
{}


long NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL::GetModelExtents( navlib::box_t& extents ) const
{
    SFVEC3F min = NL_3D_VIEWER_PLUGIN_IMPL::GetCanvas()->GetBoardAdapter().GetBBox().Min();
    SFVEC3F max = NL_3D_VIEWER_PLUGIN_IMPL::GetCanvas()->GetBoardAdapter().GetBBox().Max();

    SFVEC3F diff = ( BOUNDING_BOX_SCALE_FACTOR - 1.f ) / 2.f * ( max - min );

    min -= diff;
    max += diff;

    extents = { min.x, min.y, min.z, max.x, max.y, max.z };

    return 0;
}


void NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL::exportCommandsAndImages()
{
    std::list<TOOL_ACTION*> actions = ACTION_MANAGER::GetActionList();

    if( actions.size() == 0 )
    {
        return;
    }

    using TDx::SpaceMouse::CCommand;
    using TDx::SpaceMouse::CCommandSet;

    // The root action set node
    CCommandSet commandSet( "EDA_3D_CANVAS", "3D Viewer" );

    // Activate the command set
    NAV_3D::PutActiveCommands( commandSet.GetId() );

    // temporary store for the categories
    CATEGORY_STORE categoryStore;

    std::vector<TDx::CImage> vImages;

    // add the action set to the category_store
    categoryStore.insert( categoryStore.end(), CATEGORY_STORE::value_type( ".", &commandSet ) );

    std::list<TOOL_ACTION*>::const_iterator it;

    for( it = actions.begin(); it != actions.end(); ++it )
    {
        const TOOL_ACTION* action = *it;
        std::string        label = action->GetMenuLabel().ToStdString();

        if( label.empty() )
        {
            continue;
        }

        std::string name = action->GetName();

        // Do no export commands for the pcbnew app.
        if( name.rfind( "pcbnew.", 0 ) == 0 )
        {
            continue;
        }

        // Exclude commands which can't be used in the footprint properties.
        if( name.rfind( "3DViewer.Control.pivotCenter", 0 ) == 0
            || name.rfind( "3DViewer.Control.material", 0 ) == 0
            || name.rfind( "3DViewer.Control.attribute", 0 ) == 0
            || name.rfind( "3DViewer.Control.show", 0 ) == 0 )
        {
            continue;
        }

        std::string              strCategory = action->GetToolName();
        CATEGORY_STORE::iterator iter = categoryStore.find( strCategory );

        if( iter == categoryStore.end() )
        {
            iter = add_category( std::move( strCategory ), categoryStore );
        }

        std::string description = action->GetDescription().ToStdString();

        // Arbitrary 8-bit data stream
        wxMemoryOutputStream imageStream;

        if( action->GetIcon() != BITMAPS::INVALID_BITMAP )
        {
            wxImage image = KiBitmap( action->GetIcon() ).ConvertToImage();
            image.SaveFile( imageStream, wxBitmapType::wxBITMAP_TYPE_PNG );
            image.Destroy();

            if( imageStream.GetSize() )
            {
                wxStreamBuffer* streamBuffer = imageStream.GetOutputStreamBuffer();
                TDx::CImage     tdxImage = TDx::CImage::FromData( "", 0, name.c_str() );
                tdxImage.AssignImage( std::string( reinterpret_cast<const char*>(
                                                           streamBuffer->GetBufferStart() ),
                                                   streamBuffer->GetBufferSize() ),
                                      0 );

                wxLogTrace( m_logTrace, wxT( "Adding image for : %s" ), name );
                vImages.push_back( std::move( tdxImage ) );
            }
        }

        wxLogTrace( m_logTrace, wxT( "Inserting command: %s,  description: %s,  in category:  %s" ),
                    name, description, iter->first );

        iter->second->push_back(
                CCommand( std::move( name ), std::move( label ), std::move( description ) ) );
    }

    NAV_3D::AddCommandSet( commandSet );
    NAV_3D::AddImages( vImages );
}
