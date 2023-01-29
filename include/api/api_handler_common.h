/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_API_HANDLER_COMMON_H
#define KICAD_API_HANDLER_COMMON_H

#include <api/api_handler.h>
#include <api/common/commands/base_commands.pb.h>
#include <api/common/commands/editor_commands.pb.h>

#include <google/protobuf/empty.pb.h>

using namespace kiapi;
using namespace kiapi::common;

class API_HANDLER_COMMON : public API_HANDLER
{
public:
    API_HANDLER_COMMON();

private:
    HANDLER_RESULT<commands::GetVersionResponse> handleGetVersion( commands::GetVersion& aMsg );
};

#endif //KICAD_API_HANDLER_COMMON_H
