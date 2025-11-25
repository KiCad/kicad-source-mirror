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

#ifndef AI_ASSISTANT_PANEL_H
#define AI_ASSISTANT_PANEL_H

#include <wx/panel.h>
#include <memory>

class wxTextCtrl;
class wxButton;
class wxBoxSizer;
class EDA_BASE_FRAME;
class CLAUDE_API_CLIENT;

/**
 * @brief Panel providing an AI assistant chat interface
 *
 * This panel provides a native Claude AI chat interface within KiCAD,
 * allowing users to interact with Claude AI without leaving the application.
 *
 * Features:
 * - Real-time streaming chat responses
 * - Conversation history display
 * - Markdown-style message formatting
 * - API key configuration
 */
class AI_ASSISTANT_PANEL : public wxPanel
{
public:
    /**
     * @brief Construct an AI Assistant Panel
     * @param aParent Parent window
     * @param aFrame The main editor frame
     */
    AI_ASSISTANT_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame );

    virtual ~AI_ASSISTANT_PANEL();

    /**
     * @brief Set the API key for Claude API
     * @param aApiKey The Anthropic API key
     */
    void SetApiKey( const std::string& aApiKey );

    /**
     * @brief Get the current API key
     * @return The current API key or empty string if not set
     */
    std::string GetApiKey() const;

    /**
     * @brief Check if the API key is configured
     * @return True if API key is set, false otherwise
     */
    bool HasApiKey() const;

    /**
     * @brief Clear the conversation history
     */
    void ClearConversation();

    /**
     * @brief Add context information to the next message
     * @param aContext Context string to append to the system prompt
     */
    void AddContext( const std::string& aContext );

private:
    /**
     * @brief Initialize the UI components
     */
    void initUI();

    /**
     * @brief Handle send button click
     */
    void onSendMessage( wxCommandEvent& aEvent );

    /**
     * @brief Handle Enter key in input field
     */
    void onInputEnter( wxCommandEvent& aEvent );

    /**
     * @brief Handle clear conversation button
     */
    void onClearConversation( wxCommandEvent& aEvent );

    /**
     * @brief Handle API key settings button
     */
    void onApiKeySettings( wxCommandEvent& aEvent );

    /**
     * @brief Send the current message to Claude API
     */
    void sendMessage();

    /**
     * @brief Append text to the conversation display
     * @param aText Text to append
     * @param aIsUser True if message is from user, false if from assistant
     */
    void appendMessage( const std::string& aText, bool aIsUser );

    /**
     * @brief Show error message in the conversation display
     * @param aError Error message to display
     */
    void showError( const std::string& aError );

    /**
     * @brief Enable or disable the input controls
     * @param aEnable True to enable, false to disable
     */
    void enableInput( bool aEnable );

    /**
     * @brief Load API key from settings
     * @return True if API key was loaded, false otherwise
     */
    bool loadApiKey();

    /**
     * @brief Save API key to settings
     */
    void saveApiKey();

    EDA_BASE_FRAME*                   m_frame;
    wxTextCtrl*                       m_conversationDisplay;
    wxTextCtrl*                       m_inputField;
    wxButton*                         m_sendButton;
    wxButton*                         m_clearButton;
    wxButton*                         m_settingsButton;
    std::unique_ptr<CLAUDE_API_CLIENT> m_apiClient;
    std::string                       m_apiKey;
    std::string                       m_contextBuffer;
};

#endif // AI_ASSISTANT_PANEL_H
