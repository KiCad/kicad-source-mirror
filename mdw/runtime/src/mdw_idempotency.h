/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
/**
 * @file mdw_idempotency.h
 * @brief Durable replay of mutating requests by their idempotency_id.
 *
 * A client that loses a reply retries with the same idempotency_id, and the backend must not apply it twice. The
 * server routes every request type carrying that field through this store, and records live on disk so a retry
 * still replays after a restart.
 *
 * An intent record holding the request and a backend recovery token reaches stable storage before the handler
 * runs. A result record follows on success, and a returned failure clears the intent. An intent left alone means
 * a crash or an exception interrupted the attempt, so a retry asks the backend to compare its state against the
 * token, and only a verified miss runs it again.
 */

#ifndef MDW_IDEMPOTENCY_H
#define MDW_IDEMPOTENCY_H

#include <mdw_runtime/mdw_backend_server.h>

#include <api/mdw/mdw_envelope.pb.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>

class MDW_IDEMPOTENCY_STORE
{
public:
    struct RECOVERY
    {
        /// Capture the backend state the request is about to change. On false the response holds the error
        std::function<bool( std::string& aToken, kiapi::mdw::MdwResponse& aResponse )> token;

        /// Decide from the token whether an interrupted attempt took effect
        std::function<MDW_RECOVERY( const std::string& aToken, kiapi::mdw::MdwResponse& aResponse )> recover;
    };

    /// Open, creating if needed, the store in aDir
    explicit MDW_IDEMPOTENCY_STORE( const std::filesystem::path& aDir );

    bool IsOpen() const { return m_open; }

    /**
     * Run aHandler, which fills aResponse, at most once per aKey. A later call with the same key, even after a
     * restart or from another process sharing the store, gets the stored result instead. Calls with one key run
     * one at a time across processes. A key reused with different aRequestBytes answers WE_INVALID_PARAMETER.
     *
     * aHandler must return WE_OK whenever it left any effect, since a returned error clears the intent and lets
     * the key run again. Only a successful response is stored. A handler that applied an effect but cannot build
     * its reply throws, which leaves the attempt interrupted. A retry of an interrupted attempt runs aHandler only
     * when aRecovery verifies no effect, and without a recover hook answers WE_VERSION_CONFLICT. aHandler gets the
     * token its intent records.
     *
     * An operation keeps its first attempt's creation time through every retry, so recovery never extends
     * RETENTION.
     */
    void Run( const std::string& aKey, const std::string& aRequestBytes, kiapi::mdw::MdwResponse& aResponse,
              const std::function<void( const std::string& aToken )>& aHandler, const RECOVERY& aRecovery );

private:
    /// Operations older than this are never replayed and are pruned periodically
    static constexpr std::chrono::hours RETENTION{ 24 * 7 };

    /// The longest key accepted, which bounds the id a backend may record with its change
    static constexpr std::size_t MAX_KEY_LENGTH = 120;

    struct RECORD
    {
        std::int64_t createdAt = 0; ///< Wall-clock seconds of the first attempt
        std::string  request;
        std::string  body; ///< The recovery token of an intent, or the response of a result
    };

    enum class LOAD
    {
        ABSENT,
        FOUND,
        DAMAGED
    };

    std::filesystem::path recordPath( const std::string& aKey, const char* aExtension ) const;

    /// The lock stripe of the record files named aName
    std::filesystem::path stripePath( const std::string& aName ) const;

    std::int64_t nowSeconds() const;

    bool expired( const RECORD& aRecord ) const;

    static LOAD load( const std::filesystem::path& aPath, RECORD& aRecord );

    /// False when the record could not be made durable
    bool save( const std::filesystem::path& aPath, const RECORD& aRecord ) const;

    void runLocked( const std::string& aKey, const std::string& aRequestBytes, kiapi::mdw::MdwResponse& aResponse,
                    const std::function<void( const std::string& aToken )>& aHandler, const RECOVERY& aRecovery );

    void prune();

    std::filesystem::path                 m_dir;
    bool                                  m_open = false;
    std::mutex                            m_mutex; ///< Guards m_lastPrune
    std::chrono::steady_clock::time_point m_lastPrune;
};

#endif // MDW_IDEMPOTENCY_H
