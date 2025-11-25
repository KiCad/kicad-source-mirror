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

#include <curl/curl.h>
#include <kicad_curl/kicad_curl_easy.h>
#include <claude/claude_api_client.h>

#include <sstream>


CLAUDE_API_CLIENT::CLAUDE_API_CLIENT( const std::string& aApiKey, const std::string& aModel ) :
        m_apiKey( aApiKey ),
        m_model( aModel ),
        m_endpoint( "https://api.anthropic.com/v1/messages" ),
        m_timeoutMs( 30000 )
{
}


CLAUDE_API_CLIENT::~CLAUDE_API_CLIENT()
{
}


void CLAUDE_API_CLIENT::AddMessage( CLAUDE_MESSAGE::ROLE aRole, const std::string& aContent )
{
    m_messages.emplace_back( aRole, aContent );
}


void CLAUDE_API_CLIENT::ClearMessages()
{
    m_messages.clear();
    m_streamBuffer.clear();
}


void CLAUDE_API_CLIENT::SetSystemPrompt( const std::string& aSystemPrompt )
{
    m_systemPrompt = aSystemPrompt;
}


void CLAUDE_API_CLIENT::SetEndpoint( const std::string& aEndpoint )
{
    m_endpoint = aEndpoint;
}


void CLAUDE_API_CLIENT::SetTimeout( long aTimeoutMs )
{
    m_timeoutMs = aTimeoutMs;
}


std::string CLAUDE_API_CLIENT::buildRequestBody( bool aStream, int aMaxTokens )
{
    nlohmann::json request;

    request["model"] = m_model;
    request["max_tokens"] = aMaxTokens;
    request["stream"] = aStream;

    // Add system prompt if provided
    if( !m_systemPrompt.empty() )
    {
        request["system"] = m_systemPrompt;
    }

    // Build messages array
    nlohmann::json messages = nlohmann::json::array();
    for( const auto& msg : m_messages )
    {
        nlohmann::json msgJson;
        msgJson["role"] = ( msg.role == CLAUDE_MESSAGE::ROLE::USER ) ? "user" : "assistant";
        msgJson["content"] = msg.content;
        messages.push_back( msgJson );
    }

    request["messages"] = messages;

    return request.dump();
}


bool CLAUDE_API_CLIENT::parseStreamChunk( const std::string& aChunk,
                                          STREAM_CALLBACK aStreamCallback )
{
    // Claude API sends Server-Sent Events (SSE) in the format:
    // event: <event_type>
    // data: <json_data>
    //
    // We're interested in:
    // - event: content_block_delta
    // - event: message_stop

    std::string chunk = aChunk;

    // Skip empty lines
    if( chunk.empty() || chunk == "\n" || chunk == "\r\n" )
        return true;

    // Parse SSE event
    if( chunk.find( "event:" ) == 0 )
    {
        // This is an event type line, we'll process it with the next data line
        return true;
    }

    if( chunk.find( "data:" ) == 0 )
    {
        // Extract JSON data after "data: "
        std::string jsonStr = chunk.substr( 5 ); // Skip "data:"
        jsonStr.erase( 0, jsonStr.find_first_not_of( " \t\n\r" ) ); // Trim whitespace

        // Skip ping/heartbeat messages
        if( jsonStr == "[DONE]" || jsonStr.empty() )
            return true;

        try
        {
            nlohmann::json data = nlohmann::json::parse( jsonStr );

            // Check the event type
            std::string type = data.value( "type", "" );

            if( type == "content_block_delta" )
            {
                // Extract the text delta
                if( data.contains( "delta" ) && data["delta"].contains( "text" ) )
                {
                    std::string text = data["delta"]["text"];
                    if( aStreamCallback )
                    {
                        aStreamCallback( text );
                    }
                }
            }
            else if( type == "message_stop" || type == "content_block_stop" )
            {
                // End of stream
                return true;
            }
            else if( type == "error" )
            {
                // Error in stream
                m_lastError = data.value( "error", nlohmann::json::object() )
                                      .value( "message", "Unknown streaming error" );
                return false;
            }
        }
        catch( const nlohmann::json::exception& e )
        {
            // JSON parse error - might be incomplete chunk
            m_lastError = std::string( "JSON parse error: " ) + e.what();
            return false;
        }
    }

    return true;
}


bool CLAUDE_API_CLIENT::processResponse( const std::string& aBuffer,
                                         STREAM_CALLBACK aStreamCallback )
{
    if( aStreamCallback )
    {
        // Streaming response - process as SSE
        m_streamBuffer += aBuffer;

        // Process complete lines
        size_t pos = 0;
        while( ( pos = m_streamBuffer.find( "\n\n" ) ) != std::string::npos )
        {
            std::string event = m_streamBuffer.substr( 0, pos );
            m_streamBuffer.erase( 0, pos + 2 );

            // Split event by newlines to process each line
            std::istringstream eventStream( event );
            std::string        line;
            while( std::getline( eventStream, line ) )
            {
                if( !parseStreamChunk( line, aStreamCallback ) )
                {
                    return false;
                }
            }
        }

        return true;
    }
    else
    {
        // Non-streaming response - parse complete JSON
        try
        {
            nlohmann::json response = nlohmann::json::parse( aBuffer );

            // Check for API errors
            if( response.contains( "error" ) )
            {
                m_lastError = response["error"].value( "message", "Unknown API error" );
                return false;
            }

            // Extract content from the response
            if( response.contains( "content" ) && response["content"].is_array()
                && !response["content"].empty() )
            {
                // Claude returns content as an array of blocks
                const auto& content = response["content"][0];
                if( content.contains( "text" ) )
                {
                    std::string text = content["text"];
                    // Add assistant message to conversation history
                    AddMessage( CLAUDE_MESSAGE::ROLE::ASSISTANT, text );
                    return true;
                }
            }

            m_lastError = "Invalid response format";
            return false;
        }
        catch( const nlohmann::json::exception& e )
        {
            m_lastError = std::string( "JSON parse error: " ) + e.what();
            return false;
        }
    }
}


// Callback for streaming data
static size_t stream_write_callback( void* contents, size_t size, size_t nmemb, void* userp )
{
    size_t realsize = size * nmemb;
    auto*  ctx = static_cast<std::pair<CLAUDE_API_CLIENT*, STREAM_CALLBACK>*>( userp );

    std::string chunk( static_cast<const char*>( contents ), realsize );

    // Process the chunk immediately
    ctx->first->processResponse( chunk, ctx->second );

    return realsize;
}


bool CLAUDE_API_CLIENT::SendMessage( STREAM_CALLBACK aStreamCallback,
                                     ERROR_CALLBACK aErrorCallback, int aMaxTokens )
{
    m_lastError.clear();
    m_streamBuffer.clear();

    if( m_apiKey.empty() )
    {
        m_lastError = "API key not set";
        if( aErrorCallback )
            aErrorCallback( m_lastError );
        return false;
    }

    if( m_messages.empty() )
    {
        m_lastError = "No messages to send";
        if( aErrorCallback )
            aErrorCallback( m_lastError );
        return false;
    }

    try
    {
        KICAD_CURL_EASY curl;

        // Set URL
        curl.SetURL( m_endpoint );

        // Set headers
        curl.SetHeader( "Content-Type", "application/json" );
        curl.SetHeader( "x-api-key", m_apiKey );
        curl.SetHeader( "anthropic-version", "2023-06-01" );

        // Build request body
        std::string requestBody = buildRequestBody( true, aMaxTokens );

        // Set POST data
        curl.SetPostFields( requestBody );

        // Set up streaming callback
        std::pair<CLAUDE_API_CLIENT*, STREAM_CALLBACK> context( this, aStreamCallback );
        CURL*                                          curlHandle = curl.GetCurl();
        curl_easy_setopt( curlHandle, CURLOPT_WRITEFUNCTION, stream_write_callback );
        curl_easy_setopt( curlHandle, CURLOPT_WRITEDATA, &context );

        // Set timeout
        curl_easy_setopt( curlHandle, CURLOPT_TIMEOUT_MS, m_timeoutMs );

        // Perform request
        int result = curl.Perform();

        if( result != CURLE_OK )
        {
            m_lastError = curl.GetErrorText( result );
            if( aErrorCallback )
                aErrorCallback( m_lastError );
            return false;
        }

        // Check HTTP status code
        int statusCode = curl.GetResponseStatusCode();
        if( statusCode != 200 )
        {
            m_lastError = "HTTP error: " + std::to_string( statusCode );
            if( aErrorCallback )
                aErrorCallback( m_lastError );
            return false;
        }

        return true;
    }
    catch( const std::exception& e )
    {
        m_lastError = std::string( "Exception: " ) + e.what();
        if( aErrorCallback )
            aErrorCallback( m_lastError );
        return false;
    }
}


bool CLAUDE_API_CLIENT::SendMessageSync( std::string& aResponse, int aMaxTokens )
{
    m_lastError.clear();
    aResponse.clear();

    if( m_apiKey.empty() )
    {
        m_lastError = "API key not set";
        return false;
    }

    if( m_messages.empty() )
    {
        m_lastError = "No messages to send";
        return false;
    }

    try
    {
        KICAD_CURL_EASY curl;

        // Set URL
        curl.SetURL( m_endpoint );

        // Set headers
        curl.SetHeader( "Content-Type", "application/json" );
        curl.SetHeader( "x-api-key", m_apiKey );
        curl.SetHeader( "anthropic-version", "2023-06-01" );

        // Build request body (non-streaming)
        std::string requestBody = buildRequestBody( false, aMaxTokens );

        // Set POST data
        curl.SetPostFields( requestBody );

        // Set timeout
        CURL* curlHandle = curl.GetCurl();
        curl_easy_setopt( curlHandle, CURLOPT_TIMEOUT_MS, m_timeoutMs );

        // Perform request
        int result = curl.Perform();

        if( result != CURLE_OK )
        {
            m_lastError = curl.GetErrorText( result );
            return false;
        }

        // Check HTTP status code
        int statusCode = curl.GetResponseStatusCode();
        if( statusCode != 200 )
        {
            m_lastError = "HTTP error: " + std::to_string( statusCode );
            // Try to get error details from response body
            const std::string& buffer = curl.GetBuffer();
            if( !buffer.empty() )
            {
                try
                {
                    nlohmann::json errorJson = nlohmann::json::parse( buffer );
                    if( errorJson.contains( "error" ) && errorJson["error"].contains( "message" ) )
                    {
                        m_lastError += ": " + errorJson["error"]["message"].get<std::string>();
                    }
                }
                catch( ... )
                {
                    // Ignore JSON parse errors for error messages
                }
            }
            return false;
        }

        // Process the response
        const std::string& buffer = curl.GetBuffer();
        if( !processResponse( buffer, nullptr ) )
        {
            return false;
        }

        // Get the assistant's response from the last message
        if( !m_messages.empty() && m_messages.back().role == CLAUDE_MESSAGE::ROLE::ASSISTANT )
        {
            aResponse = m_messages.back().content;
        }

        return true;
    }
    catch( const std::exception& e )
    {
        m_lastError = std::string( "Exception: " ) + e.what();
        return false;
    }
}
