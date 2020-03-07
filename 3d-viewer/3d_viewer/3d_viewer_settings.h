/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EDA_3D_VIEWER_SETTINGS_H_
#define EDA_3D_VIEWER_SETTINGS_H_

#include <settings/app_settings.h>
#include <settings/parameters.h>


class EDA_3D_VIEWER_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct RENDER_SETTINGS
    {
        int  engine;
        int  grid_type;
        int  material_mode;
        bool opengl_copper_thickness;
        bool opengl_show_model_bbox;
        bool raytrace_anti_aliasing;
        bool raytrace_backfloor;
        bool raytrace_post_processing;
        bool raytrace_procedural_textures;
        bool raytrace_reflections;
        bool raytrace_refractions;
        bool raytrace_shadows;
        bool realistic;
        bool show_adhesive;
        bool show_axis;
        bool show_board_body;
        bool show_comments;
        bool show_eco;
        bool show_footprints_insert;
        bool show_footprints_normal;
        bool show_footprints_virtual;
        bool show_silkscreen;
        bool show_soldermask;
        bool show_solderpaste;
        bool show_zones;
        bool subtract_mask_from_silk;
    };

    EDA_3D_VIEWER_SETTINGS();

    virtual ~EDA_3D_VIEWER_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    RENDER_SETTINGS m_Render;

protected:

    virtual std::string getLegacyFrameName() const override { return "Viewer3DFrameName"; }
};


// Individual color param: not necessary once 3D viewer is migrated to color theme system
// TODO(JE) remove once irrelevant
class PARAM_COLOR : public PARAM_BASE
{
public:
    PARAM_COLOR( const std::string& aJsonPath, COLOR4D* aPtr, COLOR4D aDefault,
           bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ), m_ptr( aPtr ), m_default( aDefault )
    {}

    void Load( JSON_SETTINGS* aSettings ) const override
    {
        if( m_readOnly )
            return;

        COLOR4D val = m_default;

        if( OPT<COLOR4D> optval = aSettings->Get<COLOR4D>( m_path ) )
            val = *optval;

        *m_ptr = val;
    }

    void Store( JSON_SETTINGS* aSettings) const override
    {
        aSettings->Set<COLOR4D>( m_path, *m_ptr );
    }

    COLOR4D GetDefault() const
    {
        return m_default;
    }

    virtual void SetDefault() override
    {
        *m_ptr = m_default;
    }

private:
    COLOR4D* m_ptr;
    COLOR4D m_default;
};

#endif
