#ifndef IAccessors_HPP_INCLUDED
#define IAccessors_HPP_INCLUDED
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2018-2021 3Dconnexion.
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
 * @file IAccessors.hpp
 * @brief The accessor interface to the client 3D properties.
 */
#include <SpaceMouse/IEvents.hpp>
#include <SpaceMouse/IHit.hpp>
#include <SpaceMouse/IModel.hpp>
#include <SpaceMouse/IPivot.hpp>
#include <SpaceMouse/ISpace3D.hpp>
#include <SpaceMouse/IState.hpp>
#include <SpaceMouse/IView.hpp>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The accessor interface to the client 3D properties.
/// </summary>
class IAccessors : public ISpace3D,
                    public IView,
                    public IModel,
                    public IPivot,
                    public IHit,
                    public IEvents,
                    public IState {};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IAccessors_HPP_INCLUDED