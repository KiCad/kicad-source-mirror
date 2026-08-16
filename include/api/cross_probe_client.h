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

#ifndef CROSS_PROBE_CLIENT_H
#define CROSS_PROBE_CLIENT_H

#include <map>
#include <mutex>
#include <string>

#include <kicommon.h>
#include <kiway.h>

#include <api/common/commands/cross_probe_commands.pb.h>


/**
 * Routes cross-probe messages between standalone KiCad instances over the
 * IPC API.  Can be removed once we remove standalone mode.
 */
class KICOMMON_API CROSS_PROBE_CLIENT
{
public:
    static bool SendToFrame( FRAME_T aTarget, const google::protobuf::Message& aRequest );

    static void AnnounceToPrimary( FRAME_T aFrameType );

    static void RegisterPeer( FRAME_T aFrameType, const std::string&  aSocketPath );

    static bool IsOnStandardSocketPath();

private:
    static std::mutex  s_mutex;
    static std::map<FRAME_T, std::string> s_peers;
};


#endif // CROSS_PROBE_CLIENT_H
