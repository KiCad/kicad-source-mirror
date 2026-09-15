/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <text_eval/text_eval_environment.h>
#include <optional>

namespace SCH_CONNECTIVITY
{
using TEXT_ENVIRONMENT = TEXT_EVAL::ENVIRONMENT;

// Excludes derived graph text and shares dynamic source values through nested resolvers.
class INPUT_TEXT_SCOPE
{
public:
    INPUT_TEXT_SCOPE();
    explicit INPUT_TEXT_SCOPE( TEXT_ENVIRONMENT& aEnvironment );
    ~INPUT_TEXT_SCOPE();
    INPUT_TEXT_SCOPE( const INPUT_TEXT_SCOPE& ) = delete;
    INPUT_TEXT_SCOPE& operator=( const INPUT_TEXT_SCOPE& ) = delete;

    static bool Active() { return s_active; }

private:
    bool                                        m_previous;
    std::optional<TEXT_ENVIRONMENT>             m_owned;
    std::optional<TEXT_EVAL::ENVIRONMENT_SCOPE> m_scope;
    inline static thread_local bool             s_active = false;
};

/**
 * Marks the canvas reads of this thread, which are item drawing and view bounds.
 *
 * Tools stage an item and redraw it before they push the commit. Staging changes the screen
 * revision, so a strict read would draw net names, bus styling and netclass colors as unconnected
 * until the next recalculation. Inside this scope, the facade still serves the last published row
 * of an item on a changed screen, if the screen is alive and still holds the item. Engine input
 * capture ignores the scope, and every read outside it stays strict.
 *
 * @see @ref sch_conn_revisions_render
 */
class RENDER_SCOPE
{
public:
    RENDER_SCOPE() { ++s_depth; }
    ~RENDER_SCOPE() { --s_depth; }
    RENDER_SCOPE( const RENDER_SCOPE& ) = delete;
    RENDER_SCOPE& operator=( const RENDER_SCOPE& ) = delete;

    /** True inside a render scope, unless an INPUT_TEXT_SCOPE is also active. */
    static bool Active() { return s_depth > 0 && !INPUT_TEXT_SCOPE::Active(); }

private:
    inline static thread_local unsigned s_depth = 0;
};
} // namespace SCH_CONNECTIVITY
