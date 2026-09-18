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

#include <plugins/3dapi/model_import_queue.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

struct S3D::MODEL_IMPORT_QUEUE::STATE
{
    struct CANCELLATION
    {
        std::atomic_bool canceled{ false };
    };

    struct JOB
    {
        std::string          fileName;
        MODEL_IMPORT_OPTIONS options;
        std::uint64_t        generation = 0;
        COMPLETION           completion;
    };

    STATE()
    {
        worker = std::thread(
                [this]
                {
                    Run();
                } );
    }

    std::uint64_t Submit( std::string aFileName, const MODEL_IMPORT_OPTIONS& aOptions, COMPLETION aCompletion )
    {
        // The displaced job is destroyed after the lock is released, because its completion can
        // own arbitrary host state.
        std::optional<JOB> displaced;
        std::uint64_t      submitted = 0;

        {
            std::lock_guard<std::mutex> lock( mutex );

            if( !stopping )
            {
                if( activeCancellation )
                    activeCancellation->canceled = true;

                submitted = ++generation;
                displaced = std::move( pending );
                pending = JOB{ std::move( aFileName ), aOptions, submitted, std::move( aCompletion ) };
            }
        }

        if( submitted )
            condition.notify_one();

        return submitted;
    }

    void Cancel()
    {
        std::lock_guard<std::mutex> lock( mutex );
        pending.reset();

        if( activeCancellation )
            activeCancellation->canceled = true;
    }

    void Stop()
    {
        std::lock_guard<std::mutex> stopLock( stopMutex );

        {
            std::lock_guard<std::mutex> lock( mutex );

            if( !stopping )
            {
                stopping = true;
                pending.reset();

                if( activeCancellation )
                    activeCancellation->canceled = true;
            }
        }

        condition.notify_one();

        if( worker.joinable() )
            worker.join();
    }

    void Run() noexcept
    {
        try
        {
            for( ;; )
            {
                JOB                           job;
                std::shared_ptr<CANCELLATION> cancellation = std::make_shared<CANCELLATION>();

                {
                    std::unique_lock<std::mutex> lock( mutex );
                    condition.wait( lock,
                                    [this]
                                    {
                                        return stopping || pending.has_value();
                                    } );

                    if( stopping )
                        return;

                    job = std::move( *pending );
                    pending.reset();
                    activeCancellation = cancellation;
                }

                Process( std::move( job ), cancellation );
            }
        }
        catch( ... )
        {
            std::lock_guard<std::mutex> lock( mutex );
            activeCancellation.reset();
            stopping = true;
            pending.reset();
        }
    }

    void Process( JOB aJob, const std::shared_ptr<CANCELLATION>& aCancellation ) noexcept
    {
        aJob.options.cancellationContext = aCancellation.get();
        aJob.options.isCanceled = []( void* aContext ) noexcept
        {
            return static_cast<CANCELLATION*>( aContext )->canceled.load();
        };

        MODEL_IMPORT_RESULT result = ImportModel( aJob.fileName, aJob.options );

        {
            std::lock_guard<std::mutex> lock( mutex );
            activeCancellation.reset();

            if( stopping )
                return;
        }

        try
        {
            aJob.completion( aJob.generation, std::move( result ) );
        }
        catch( ... )
        {
        }
    }

    std::mutex                    stopMutex;
    std::mutex                    mutex;
    std::condition_variable       condition;
    std::optional<JOB>            pending;
    std::shared_ptr<CANCELLATION> activeCancellation;
    std::uint64_t                 generation = 0;
    bool                          stopping = false;
    std::thread                   worker;
};


S3D::MODEL_IMPORT_QUEUE::MODEL_IMPORT_QUEUE() :
        m_state( std::make_shared<STATE>() )
{
}


S3D::MODEL_IMPORT_QUEUE::~MODEL_IMPORT_QUEUE()
{
    Shutdown();
}


std::uint64_t S3D::MODEL_IMPORT_QUEUE::Submit( std::string aFileName, const MODEL_IMPORT_OPTIONS& aOptions,
                                               COMPLETION aCompletion )
{
    return m_state->Submit( std::move( aFileName ), aOptions, std::move( aCompletion ) );
}


void S3D::MODEL_IMPORT_QUEUE::Cancel()
{
    m_state->Cancel();
}


void S3D::MODEL_IMPORT_QUEUE::Shutdown()
{
    m_state->Stop();
}


std::uint64_t S3D::MODEL_IMPORT_QUEUE::CurrentGeneration() const
{
    std::lock_guard<std::mutex> lock( m_state->mutex );
    return m_state->generation;
}
