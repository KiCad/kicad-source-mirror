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

#include <widgets/ai_assistant_panel.h>
#include <claude/claude_api_client.h>
#include <eda_base_frame.h>
#include <pgm_base.h>
#include <settings/common_settings.h>

#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>


AI_ASSISTANT_PANEL::AI_ASSISTANT_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        wxPanel( aParent, wxID_ANY ),
        m_frame( aFrame )
{
    initUI();
    loadApiKey();

    // Initialize API client if key is available
    if( HasApiKey() )
    {
        m_apiClient = std::make_unique<CLAUDE_API_CLIENT>( m_apiKey );
        m_apiClient->SetSystemPrompt(
                "You are an AI assistant integrated into KiCAD, a PCB design application. "
                "Help users with PCB design questions, component placement, routing advice, "
                "design rule questions, and general electronics design. Be concise and practical." );
    }
}


AI_ASSISTANT_PANEL::~AI_ASSISTANT_PANEL()
{
}


void AI_ASSISTANT_PANEL::initUI()
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // Title and settings bar
    wxBoxSizer* titleSizer = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* title = new wxStaticText( this, wxID_ANY, _( "Claude AI Assistant" ) );
    wxFont        titleFont = title->GetFont();
    titleFont.SetWeight( wxFONTWEIGHT_BOLD );
    titleFont.SetPointSize( titleFont.GetPointSize() + 1 );
    title->SetFont( titleFont );
    titleSizer->Add( title, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    m_settingsButton = new wxButton( this, wxID_ANY, _( "Settings" ), wxDefaultPosition,
                                     wxDefaultSize, wxBU_EXACTFIT );
    titleSizer->Add( m_settingsButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    m_clearButton = new wxButton( this, wxID_ANY, _( "Clear" ), wxDefaultPosition, wxDefaultSize,
                                  wxBU_EXACTFIT );
    titleSizer->Add( m_clearButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    mainSizer->Add( titleSizer, 0, wxEXPAND | wxALL, 5 );

    // Conversation display (read-only, multi-line)
    m_conversationDisplay = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                            wxDefaultSize,
                                            wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2
                                                    | wxTE_WORDWRAP );
    m_conversationDisplay->SetMinSize( wxSize( -1, 200 ) );
    mainSizer->Add( m_conversationDisplay, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    // Input area
    wxBoxSizer* inputSizer = new wxBoxSizer( wxHORIZONTAL );

    m_inputField = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                   wxDefaultSize, wxTE_PROCESS_ENTER );
    m_inputField->SetHint( _( "Type your message here..." ) );
    inputSizer->Add( m_inputField, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    m_sendButton = new wxButton( this, wxID_ANY, _( "Send" ) );
    inputSizer->Add( m_sendButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    mainSizer->Add( inputSizer, 0, wxEXPAND | wxALL, 5 );

    SetSizer( mainSizer );
    Layout();

    // Bind events
    m_sendButton->Bind( wxEVT_BUTTON, &AI_ASSISTANT_PANEL::onSendMessage, this );
    m_inputField->Bind( wxEVT_TEXT_ENTER, &AI_ASSISTANT_PANEL::onInputEnter, this );
    m_clearButton->Bind( wxEVT_BUTTON, &AI_ASSISTANT_PANEL::onClearConversation, this );
    m_settingsButton->Bind( wxEVT_BUTTON, &AI_ASSISTANT_PANEL::onApiKeySettings, this );
}


void AI_ASSISTANT_PANEL::SetApiKey( const std::string& aApiKey )
{
    m_apiKey = aApiKey;
    m_apiClient = std::make_unique<CLAUDE_API_CLIENT>( m_apiKey );
    m_apiClient->SetSystemPrompt(
            "You are an AI assistant integrated into KiCAD, a PCB design application. "
            "Help users with PCB design questions, component placement, routing advice, "
            "design rule questions, and general electronics design. Be concise and practical." );
    saveApiKey();
}


std::string AI_ASSISTANT_PANEL::GetApiKey() const
{
    return m_apiKey;
}


bool AI_ASSISTANT_PANEL::HasApiKey() const
{
    return !m_apiKey.empty();
}


void AI_ASSISTANT_PANEL::ClearConversation()
{
    m_conversationDisplay->Clear();
    if( m_apiClient )
    {
        m_apiClient->ClearMessages();
    }
    m_contextBuffer.clear();
}


void AI_ASSISTANT_PANEL::AddContext( const std::string& aContext )
{
    m_contextBuffer += aContext + "\n";
}


void AI_ASSISTANT_PANEL::onSendMessage( wxCommandEvent& aEvent )
{
    sendMessage();
}


void AI_ASSISTANT_PANEL::onInputEnter( wxCommandEvent& aEvent )
{
    sendMessage();
}


void AI_ASSISTANT_PANEL::onClearConversation( wxCommandEvent& aEvent )
{
    if( wxMessageBox( _( "Clear the entire conversation history?" ), _( "Confirm Clear" ),
                      wxYES_NO | wxICON_QUESTION, this )
        == wxYES )
    {
        ClearConversation();
    }
}


void AI_ASSISTANT_PANEL::onApiKeySettings( wxCommandEvent& aEvent )
{
    wxTextEntryDialog dlg( this, _( "Enter your Anthropic API key:" ), _( "API Key Settings" ),
                           wxString::FromUTF8( m_apiKey ) );
    dlg.SetTextValidator( wxFILTER_NONE );

    if( dlg.ShowModal() == wxID_OK )
    {
        std::string newKey = dlg.GetValue().ToStdString();
        if( !newKey.empty() )
        {
            SetApiKey( newKey );
            wxMessageBox( _( "API key updated successfully." ), _( "Success" ),
                          wxOK | wxICON_INFORMATION, this );
        }
    }
}


void AI_ASSISTANT_PANEL::sendMessage()
{
    if( !HasApiKey() )
    {
        wxMessageBox(
                _( "Please configure your Anthropic API key in Settings before using the AI assistant." ),
                _( "API Key Required" ), wxOK | wxICON_WARNING, this );
        onApiKeySettings( wxCommandEvent() );
        return;
    }

    wxString userMessage = m_inputField->GetValue();
    if( userMessage.IsEmpty() )
    {
        return;
    }

    // Clear input field
    m_inputField->Clear();

    // Disable input while processing
    enableInput( false );

    // Display user message
    appendMessage( userMessage.ToStdString(), true );

    // Add context if any
    std::string fullMessage = userMessage.ToStdString();
    if( !m_contextBuffer.empty() )
    {
        fullMessage = m_contextBuffer + "\n" + fullMessage;
        m_contextBuffer.clear();
    }

    // Add message to API client
    m_apiClient->AddMessage( CLAUDE_MESSAGE::ROLE::USER, fullMessage );

    // Prepare for assistant response
    appendMessage( "", false ); // Start assistant message block

    // Send message with streaming
    bool success = m_apiClient->SendMessage(
            [this]( const std::string& chunk )
            {
                // This callback runs in the curl thread, so we need to post to main thread
                wxString wxChunk = wxString::FromUTF8( chunk );
                CallAfter(
                        [this, wxChunk]()
                        {
                            // Append to conversation display
                            m_conversationDisplay->AppendText( wxChunk );
                        } );
            },
            [this]( const std::string& error )
            {
                // Error callback
                CallAfter(
                        [this, error]()
                        {
                            showError( error );
                            enableInput( true );
                        } );
            } );

    if( success )
    {
        // Re-enable input after successful completion
        CallAfter( [this]() { enableInput( true ); } );
    }
    else
    {
        // Show error if immediate failure
        showError( m_apiClient->GetLastError() );
        enableInput( true );
    }
}


void AI_ASSISTANT_PANEL::appendMessage( const std::string& aText, bool aIsUser )
{
    wxTextAttr style;

    // Add separator if not first message
    long currentLength = m_conversationDisplay->GetLastPosition();
    if( currentLength > 0 )
    {
        m_conversationDisplay->AppendText( wxT( "\n\n" ) );
    }

    // Set style for message header
    if( aIsUser )
    {
        style.SetTextColour( wxColour( 0, 100, 200 ) );
        style.SetFontWeight( wxFONTWEIGHT_BOLD );
        m_conversationDisplay->SetDefaultStyle( style );
        m_conversationDisplay->AppendText( _( "You: " ) );
    }
    else
    {
        style.SetTextColour( wxColour( 0, 150, 0 ) );
        style.SetFontWeight( wxFONTWEIGHT_BOLD );
        m_conversationDisplay->SetDefaultStyle( style );
        m_conversationDisplay->AppendText( _( "Claude: " ) );
    }

    // Reset style for message content
    style.SetTextColour( *wxBLACK );
    style.SetFontWeight( wxFONTWEIGHT_NORMAL );
    m_conversationDisplay->SetDefaultStyle( style );

    if( !aText.empty() )
    {
        m_conversationDisplay->AppendText( wxString::FromUTF8( aText ) );
    }

    // Scroll to bottom
    m_conversationDisplay->ShowPosition( m_conversationDisplay->GetLastPosition() );
}


void AI_ASSISTANT_PANEL::showError( const std::string& aError )
{
    wxTextAttr style;
    style.SetTextColour( *wxRED );
    style.SetFontWeight( wxFONTWEIGHT_BOLD );
    m_conversationDisplay->SetDefaultStyle( style );
    m_conversationDisplay->AppendText( wxT( "\n\nError: " ) + wxString::FromUTF8( aError ) );

    // Reset style
    style.SetTextColour( *wxBLACK );
    style.SetFontWeight( wxFONTWEIGHT_NORMAL );
    m_conversationDisplay->SetDefaultStyle( style );

    // Scroll to bottom
    m_conversationDisplay->ShowPosition( m_conversationDisplay->GetLastPosition() );
}


void AI_ASSISTANT_PANEL::enableInput( bool aEnable )
{
    m_inputField->Enable( aEnable );
    m_sendButton->Enable( aEnable );

    if( aEnable )
    {
        m_inputField->SetFocus();
    }
}


bool AI_ASSISTANT_PANEL::loadApiKey()
{
    // Try to load from common settings
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();
    if( settings )
    {
        // For now, we'll use an environment variable or prompt user
        // In a production version, this should be encrypted in settings
        wxString key = wxGetEnv( wxT( "ANTHROPIC_API_KEY" ) ) ? wxGetenv( wxT( "ANTHROPIC_API_KEY" ) )
                                                                : wxString();
        if( !key.IsEmpty() )
        {
            m_apiKey = key.ToStdString();
            return true;
        }
    }
    return false;
}


void AI_ASSISTANT_PANEL::saveApiKey()
{
    // In a production version, this should save encrypted to settings
    // For now, we'll just keep it in memory
}
