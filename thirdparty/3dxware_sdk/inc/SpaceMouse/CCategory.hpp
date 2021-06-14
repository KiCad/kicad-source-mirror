#ifndef CCategory_HPP_INCLUDED
#define CCategory_HPP_INCLUDED
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
 * @file CCategory.hpp
 * @brief SiActionNodeEx_t wrapper.
 */
#include <SpaceMouse/CCommandTreeNode.hpp>

#if !_MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace TDx {
/// <summary>
/// Contains types used for programming the SpaceMouse.
/// </summary>
namespace SpaceMouse {
/// <summary>
/// The helper class implements the <see cref="SiActionNodeType_t::SI_CATEGORY_NODE"/> node type.
/// </summary>
class CCategory : public CCommandTreeNode {
  typedef CCommandTreeNode base_type;

public:
  CCategory() {
  }

  explicit CCategory(std::string id, std::string name)
      : base_type(std::move(id), std::move(name), SiActionNodeType_t::SI_CATEGORY_NODE) {
  }

#if defined(_MSC_VER) && _MSC_VER < 1900
  CCategory(CCategory &&other) NOEXCEPT : base_type(std::forward<base_type>(other)) {
  }
  CCategory &operator=(CCategory &&other) NOEXCEPT {
    base_type::operator=(std::forward<base_type>(other));
    return *this;
  }
#else
  CCategory(CCategory &&) = default;
  CCategory &operator=(CCategory &&) = default;
#endif
};
} // namespace SpaceMouse
} // namespace TDx

#if !_MSC_VER
#pragma GCC diagnostic pop
#endif

#endif // CCategory_HPP_INCLUDED