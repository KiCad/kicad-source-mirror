/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2025, Damjan Prerad <damjanovmail@gmail.com>
 * Copyright (C) The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _3D_SPHERES_GIZMO_H_
#define _3D_SPHERES_GIZMO_H_

#include "../render_3d_base.h"

/**
 * @class SPHERES_GIZMO
 * @brief Renders a set of colored spheres in 3D space that act as a directional orientation gizmo.
 */
class SPHERES_GIZMO
{
public:
    SPHERES_GIZMO( int aGizmoPosX, int aGizmoPosY );
    ~SPHERES_GIZMO();

    void                           setViewport( int ax, int ay, int aWidth, int aHeight );
    std::tuple<int, int, int, int> getViewport() const;
    void                           setGizmoPosition( int ax, int ay );

    void handleMouseInput( int aMouseX, int aMouseY );

    /**
     * @enum GizmoSphereSelection
     * @brief Enum to indicate which sphere (direction) is selected.
     */
    enum class GizmoSphereSelection
    {
        None = -1, ///< No sphere selected.
        Right = 0, ///< +X direction.
        Left,      ///< -X direction.
        Back,      ///< +Y direction.
        Front,     ///< -Y direction.
        Top,       ///< +Z direction.
        Bottom,    ///< -Z direction.
        Count      ///< Number of selectable spheres.
    };

    GizmoSphereSelection getSelectedGizmoSphere() const;
    void                 resetSelectedGizmoSphere();
    void                 render3dSpheresGizmo( glm::mat4 aCameraRotationMatrix );

private:
    void setGizmoMaterial();

    GLUquadric* m_quadric = nullptr;

    int m_gizmoPosX = 0;
    int m_gizmoPosY = 0;

    int m_viewportX = 0;
    int m_viewportY = 0;
    int m_viewportW = 0;
    int m_viewportH = 0;

    float m_ndcX = -1.0f;
    float m_ndcY = -1.0f;

    struct GizmoSphere
    {
        glm::vec3 m_position;
        float     m_radius;
        glm::vec3 m_labelPosition;
        glm::vec3 m_color;
        glm::vec3 m_originalColor;
    };

    // Define sphere positions
    const float m_arrowSize = RANGE_SCALE_3D * 0.20f;
    const float m_sphereRadius = 0.05f * RANGE_SCALE_3D;

    /**
     * @brief List of all directional gizmo spheres.
     * The order follows the GizmoSphereSelection enum.
     */
    std::array<GizmoSphere, 6> m_spheres = { { { { m_arrowSize, 0.0f, 0.0f },
                                                 m_sphereRadius,
                                                 { m_arrowSize + 0.02f, 0.0f, 0.0f },
                                                 { 0.9f, 0.0f, 0.0f },
                                                 { 0.9f, 0.0f, 0.0f } },
                                               { { -m_arrowSize, 0.0f, 0.0f },
                                                 m_sphereRadius,
                                                 { -m_arrowSize - 0.02f, 0.0f, 0.0f },
                                                 { 0.4f, 0.0f, 0.0f },
                                                 { 0.4f, 0.0f, 0.0f } },
                                               { { 0.0f, m_arrowSize, 0.0f },
                                                 m_sphereRadius,
                                                 { 0.0f, m_arrowSize + 0.02f, 0.0f },
                                                 { 0.0f, 0.9f, 0.0f },
                                                 { 0.0f, 0.9f, 0.0f } },
                                               { { 0.0f, -m_arrowSize, 0.0f },
                                                 m_sphereRadius,
                                                 { 0.0f, -m_arrowSize - 0.02f, 0.0f },
                                                 { 0.0f, 0.4f, 0.0f },
                                                 { 0.0f, 0.4f, 0.0f } },
                                               { { 0.0f, 0.0f, m_arrowSize },
                                                 m_sphereRadius,
                                                 { 0.0f, 0.0f, m_arrowSize + 0.02f },
                                                 { 0.0f, 0.0f, 0.9f },
                                                 { 0.0f, 0.0f, 0.9f } },
                                               { { 0.0f, 0.0f, -m_arrowSize },
                                                 m_sphereRadius,
                                                 { 0.0f, 0.0f, -m_arrowSize - 0.02f },
                                                 { 0.0f, 0.0f, 0.4f },
                                                 { 0.0f, 0.0f, 0.4f } } } };

    GizmoSphereSelection m_selectedGizmoSphere = GizmoSphereSelection::None;
};

#endif // _3D_SPHERES_GIZMO_H_