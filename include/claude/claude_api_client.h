/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef CLAUDE_API_CLIENT_H_
#define CLAUDE_API_CLIENT_H_

#include <kicommon.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class KICAD_CURL_EASY;

/**
 * @brief Message structure for Claude API conversations
 */
struct CLAUDE_MESSAGE
{
    enum class ROLE
    {
        USER,
        ASSISTANT
    };

    ROLE        role;
    std::string content;

    CLAUDE_MESSAGE( ROLE aRole, const std::string& aContent ) :
            role( aRole ), content( aContent )
    {
    }
};

/**
 * @brief Callback function type for streaming responses
 * Called with chunks of text as they arrive from the API
 */
typedef std::function<void( const std::string& )> STREAM_CALLBACK;

/**
 * @brief Error callback function type
 * Called when an error occurs during API communication
 */
typedef std::function<void( const std::string& )> ERROR_CALLBACK;

/**
 * @brief Client for interacting with the Anthropic Claude API
 *
 * This class provides a C++ interface to the Claude AI API, supporting:
 * - Synchronous and streaming message requests
 * - Conversation history management
 * - Automatic retry on transient failures
 * - Error handling and reporting
 *
 * Usage example:
 * @code
 *   CLAUDE_API_CLIENT client( "your-api-key" );
 *   client.AddMessage( CLAUDE_MESSAGE::ROLE::USER, "Hello, Claude!" );
 *
 *   // Streaming response
 *   client.SendMessage( [](const std::string& chunk) {
 *       std::cout << chunk;
 *   });
 * @endcode
 */
class KICOMMON_API CLAUDE_API_CLIENT
{
public:
    /**
     * @brief Construct a new Claude API client
     * @param aApiKey The Anthropic API key for authentication
     * @param aModel The Claude model to use (default: claude-sonnet-4-5-20250929)
     */
    CLAUDE_API_CLIENT( const std::string& aApiKey,
                       const std::string& aModel = "claude-sonnet-4-5-20250929" );

    ~CLAUDE_API_CLIENT();

    /**
     * @brief Add a message to the conversation history
     * @param aRole The role (USER or ASSISTANT)
     * @param aContent The message content
     */
    void AddMessage( CLAUDE_MESSAGE::ROLE aRole, const std::string& aContent );

    /**
     * @brief Clear all messages from the conversation history
     */
    void ClearMessages();

    /**
     * @brief Get the current conversation history
     * @return Vector of messages in the conversation
     */
    const std::vector<CLAUDE_MESSAGE>& GetMessages() const { return m_messages; }

    /**
     * @brief Send a message and receive streaming response
     * @param aStreamCallback Called with each chunk of the response as it arrives
     * @param aErrorCallback Called if an error occurs (optional)
     * @param aMaxTokens Maximum tokens in the response (default: 4096)
     * @return True if successful, false on error
     */
    bool SendMessage( STREAM_CALLBACK aStreamCallback,
                      ERROR_CALLBACK aErrorCallback = nullptr,
                      int aMaxTokens = 4096 );

    /**
     * @brief Send a message and receive complete response
     * @param aResponse Output parameter for the complete response
     * @param aMaxTokens Maximum tokens in the response (default: 4096)
     * @return True if successful, false on error
     */
    bool SendMessageSync( std::string& aResponse, int aMaxTokens = 4096 );

    /**
     * @brief Set the system prompt for the conversation
     * @param aSystemPrompt The system-level instructions for Claude
     */
    void SetSystemPrompt( const std::string& aSystemPrompt );

    /**
     * @brief Get the last error message
     * @return The last error message, or empty string if no error
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief Set the API endpoint URL
     * @param aEndpoint The API endpoint (default: https://api.anthropic.com/v1/messages)
     */
    void SetEndpoint( const std::string& aEndpoint );

    /**
     * @brief Set the timeout for API requests
     * @param aTimeoutMs Timeout in milliseconds (default: 30000)
     */
    void SetTimeout( long aTimeoutMs );

private:
    /**
     * @brief Build the JSON request body for the API
     * @param aStream Whether to enable streaming
     * @param aMaxTokens Maximum tokens in the response
     * @return JSON request body as string
     */
    std::string buildRequestBody( bool aStream, int aMaxTokens );

    /**
     * @brief Parse a streaming response chunk
     * @param aChunk The raw chunk data
     * @param aStreamCallback Callback to invoke with parsed content
     * @return True if chunk was valid, false otherwise
     */
    bool parseStreamChunk( const std::string& aChunk, STREAM_CALLBACK aStreamCallback );

    /**
     * @brief Process the API response buffer
     * @param aBuffer The response buffer to process
     * @param aStreamCallback Callback for streaming chunks (optional)
     * @return True if successful, false on error
     */
    bool processResponse( const std::string& aBuffer, STREAM_CALLBACK aStreamCallback = nullptr );

    std::string                    m_apiKey;
    std::string                    m_model;
    std::string                    m_systemPrompt;
    std::string                    m_endpoint;
    std::string                    m_lastError;
    std::vector<CLAUDE_MESSAGE>    m_messages;
    long                           m_timeoutMs;
    std::string                    m_streamBuffer;  // Buffer for incomplete stream chunks
};

#endif // CLAUDE_API_CLIENT_H_
