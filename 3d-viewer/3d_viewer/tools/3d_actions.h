/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef EDA_3D_ACTIONS_H
#define EDA_3D_ACTIONS_H

#include <tool/tool_action.h>
#include <tool/actions.h>
#include <core/optional.h>

class TOOL_EVENT;
class TOOL_MANAGER;

/**
 * EDA_3D_ACTIONS
 *
 * Note: these aren't "real" actions; we just use them to see the hotkeys display.
 */
class EDA_3D_ACTIONS : public ACTIONS
{
public:
    static TOOL_ACTION controlActivate;

    static TOOL_ACTION pivotCenter;
    static TOOL_ACTION rotateXCW;
    static TOOL_ACTION rotateXCCW;
    static TOOL_ACTION rotateYCW;
    static TOOL_ACTION rotateYCCW;
    static TOOL_ACTION rotateZCW;
    static TOOL_ACTION rotateZCCW;
    static TOOL_ACTION moveLeft;
    static TOOL_ACTION moveRight;
    static TOOL_ACTION moveUp;
    static TOOL_ACTION moveDown;
    static TOOL_ACTION homeView;
    static TOOL_ACTION resetView;
    static TOOL_ACTION flipView;
    static TOOL_ACTION toggleOrtho;

    static TOOL_ACTION viewFront;
    static TOOL_ACTION viewBack;
    static TOOL_ACTION viewLeft;
    static TOOL_ACTION viewRight;
    static TOOL_ACTION viewTop;
    static TOOL_ACTION viewBottom;

    static TOOL_ACTION noGrid;
    static TOOL_ACTION show10mmGrid;
    static TOOL_ACTION show5mmGrid;
    static TOOL_ACTION show2_5mmGrid;
    static TOOL_ACTION show1mmGrid;

    static TOOL_ACTION materialNormal;
    static TOOL_ACTION materialDiffuse;
    static TOOL_ACTION materialCAD;

    static TOOL_ACTION rotate45axisZ;
    static TOOL_ACTION attributesTHT;
    static TOOL_ACTION attributesSMD;
    static TOOL_ACTION attributesVirtual;
    static TOOL_ACTION showBoundingBoxes;
    static TOOL_ACTION renderShadows;
    static TOOL_ACTION proceduralTextures;
    static TOOL_ACTION addFloor;
    static TOOL_ACTION showRefractions;
    static TOOL_ACTION showReflections;
    static TOOL_ACTION antiAliasing;
    static TOOL_ACTION postProcessing;
    static TOOL_ACTION toggleRealisticMode;
    static TOOL_ACTION toggleBoardBody;
    static TOOL_ACTION showAxis;
    static TOOL_ACTION toggleZones;
    static TOOL_ACTION toggleAdhesive;
    static TOOL_ACTION toggleSilk;
    static TOOL_ACTION toggleSolderMask;
    static TOOL_ACTION toggleSolderPaste;
    static TOOL_ACTION toggleComments;
    static TOOL_ACTION toggleECO;

    ///< @copydoc COMMON_ACTIONS::TranslateLegacyId()
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) override { return OPT<TOOL_EVENT>(); }
};

#endif
