/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_BOARD_PROJECT_SETTINGS_PARAMS_H
#define KICAD_BOARD_PROJECT_SETTINGS_PARAMS_H

#include <project/board_project_settings.h>
#include <settings/parameters.h>


class KICOMMON_API PARAM_LAYER_PRESET : public PARAM_LAMBDA<nlohmann::json>
{
public:
    PARAM_LAYER_PRESET( const std::string& aPath, std::vector<LAYER_PRESET>* aPresetList );

    static void MigrateToV9Layers( nlohmann::json& aJson );

    static void MigrateToNamedRenderLayers( nlohmann::json& aJson );

private:
    nlohmann::json presetsToJson();

    void jsonToPresets( const nlohmann::json& aJson );

    std::vector<LAYER_PRESET>* m_presets;
};


class KICOMMON_API PARAM_VIEWPORT : public PARAM_LAMBDA<nlohmann::json>
{
public:
    PARAM_VIEWPORT( const std::string& aPath, std::vector<VIEWPORT>* aViewportList );

private:
    nlohmann::json viewportsToJson();

    void jsonToViewports( const nlohmann::json& aJson );

    std::vector<VIEWPORT>* m_viewports;
};


class KICOMMON_API PARAM_VIEWPORT3D : public PARAM_LAMBDA<nlohmann::json>
{
public:
    PARAM_VIEWPORT3D( const std::string& aPath, std::vector<VIEWPORT3D>* aViewportList );

private:
    nlohmann::json viewportsToJson();

    void jsonToViewports( const nlohmann::json & aJson );

    std::vector<VIEWPORT3D>* m_viewports;
};


class KICOMMON_API PARAM_LAYER_PAIRS : public PARAM_LAMBDA<nlohmann::json>
{
public:
    PARAM_LAYER_PAIRS( const std::string& aPath, std::vector<LAYER_PAIR_INFO>& m_layerPairInfos );

private:
    nlohmann::json layerPairsToJson();

    void jsonToLayerPairs( const nlohmann::json& aJson );

    std::vector<LAYER_PAIR_INFO>& m_layerPairInfos;
};


#endif // KICAD_BOARD_PROJECT_SETTINGS_PARAMS_H
