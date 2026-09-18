/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#ifndef MODEL_IMPORT_QUEUE_H
#define MODEL_IMPORT_QUEUE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <plugins/3dapi/model_import.h>

namespace S3D
{
/**
 * Single-slot import queue shared by every native preview front end.
 *
 * A preview host asks for whichever file the user is pointing at now, so only the newest request
 * matters.  Submit() therefore cancels the running import and replaces anything still queued,
 * and each request carries a generation the host uses to drop results that arrived too late.
 *
 * The completion runs on the worker thread.  Hosts marshal it to their own loop, because nothing
 * here may depend on wxWidgets, the Win32 message loop or Grand Central Dispatch.
 */
class KICAD_3D_IMPORT_API MODEL_IMPORT_QUEUE
{
public:
    using COMPLETION = std::function<void( std::uint64_t aGeneration, MODEL_IMPORT_RESULT aResult )>;

    MODEL_IMPORT_QUEUE();
    ~MODEL_IMPORT_QUEUE();

    MODEL_IMPORT_QUEUE( const MODEL_IMPORT_QUEUE& ) = delete;
    MODEL_IMPORT_QUEUE& operator=( const MODEL_IMPORT_QUEUE& ) = delete;

    /**
     * Supersede any pending or running request with @a aFileName.
     *
     * @return the generation stamped on the new request, or 0 once the queue has been shut down.
     */
    std::uint64_t Submit( std::string aFileName, const MODEL_IMPORT_OPTIONS& aOptions, COMPLETION aCompletion );

    /**
     * Abandon the pending and running requests without stopping the worker.
     */
    void Cancel();

    /**
     * Abandon everything and join the worker.  Safe to call more than once, and called by the
     * destructor.  Must not be called from a completion.
     */
    void Shutdown();

    /**
     * The generation of the most recent Submit(), for hosts that track staleness themselves.
     */
    std::uint64_t CurrentGeneration() const;

private:
    struct STATE;

    std::shared_ptr<STATE> m_state;
};
} // namespace S3D

#endif // MODEL_IMPORT_QUEUE_H
