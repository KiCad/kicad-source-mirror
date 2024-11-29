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

#include <google/protobuf/empty.pb.h>

#include <api/api_handler.h>
#include <api/common/commands/base_commands.pb.h>
#include <api/common/commands/project_commands.pb.h>

using namespace kiapi::common;
using google::protobuf::Empty;

class API_HANDLER_COMMON : public API_HANDLER
{
public:
    API_HANDLER_COMMON();

    ~API_HANDLER_COMMON() override {}

private:
    HANDLER_RESULT<commands::GetVersionResponse> handleGetVersion( commands::GetVersion& aMsg,
                                                                   const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::NetClassesResponse> handleGetNetClasses( commands::GetNetClasses& aMsg,
                                                                      const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<Empty> handlePing( commands::Ping& aMsg, const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<types::Box2> handleGetTextExtents( commands::GetTextExtents& aMsg,
            const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::GetTextAsShapesResponse>
    handleGetTextAsShapes( commands::GetTextAsShapes& aMsg, const HANDLER_CONTEXT& aCtx );

    HANDLER_RESULT<commands::ExpandTextVariablesResponse>
    handleExpandTextVariables( commands::ExpandTextVariables& aMsg, const HANDLER_CONTEXT& aCtx );
};

#endif //KICAD_API_HANDLER_COMMON_H
