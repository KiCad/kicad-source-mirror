#ifndef CCommand_HPP_INCLUDED
#define CCommand_HPP_INCLUDED
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
 * @file CCommand.hpp
 * @brief SiActionNodeEx_t wrapper.
 */
#include <SpaceMouse/CCommandTreeNode.hpp>

#if !_MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace TDx {
namespace SpaceMouse {
/// <summary>
/// The <see cref="CCommand"/> class implements the application command node.
/// </summary>
class CCommand : public CCommandTreeNode {
  typedef CCommandTreeNode base_type;

public:
  CCommand() {
  }

  explicit CCommand(std::string id, std::string name, std::string description)
      : base_type(std::move(id), std::move(name), std::move(description),
                  SiActionNodeType_t::SI_ACTION_NODE) {
  }
  explicit CCommand(std::string id, std::string name)
      : base_type(std::move(id), std::move(name), SiActionNodeType_t::SI_ACTION_NODE) {
  }
#if defined(_MSC_VER) && _MSC_VER < 1900
  CCommand(CCommand &&other) NOEXCEPT : base_type(std::forward<base_type>(other)) {
  }
  CCommand &operator=(CCommand &&other) NOEXCEPT {
    base_type::operator=(std::forward<base_type>(other));
    return *this;
  }
#else
  CCommand(CCommand &&) = default;
  CCommand &operator=(CCommand &&) = default;
#endif
};
} // namespace SpaceMouse
} // namespace TDx

#if !_MSC_VER
#pragma GCC diagnostic pop
#endif

#endif // CCommand_HPP_INCLUDED