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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <reporter.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/statusbr.h>
#include <wx/log.h>
#include <vector>
#include <sstream>

// Mock TEST_REPORTER to test base REPORTER functionality
class TEST_REPORTER : public REPORTER
{
public:
    TEST_REPORTER() : m_hasMessage( false ), m_messageCount( 0 ) {}

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        REPORTER::Report( aText, aSeverity );  // Call base to update severity mask
        m_messages.push_back( std::make_pair( aText, aSeverity ) );
        m_hasMessage = true;
        m_messageCount++;
        return *this;
    }

    bool HasMessage() const override
    {
        return m_hasMessage;
    }

    void Clear() override
    {
        REPORTER::Clear();
        m_messages.clear();
        m_hasMessage = false;
        m_messageCount = 0;
    }

    const std::vector<std::pair<wxString, SEVERITY>>& GetMessages() const { return m_messages; }
    int GetMessageCount() const { return m_messageCount; }

private:
    std::vector<std::pair<wxString, SEVERITY>> m_messages;
    bool m_hasMessage;
    int m_messageCount;
};

// Mock wxTextCtrl for testing
class MockTextCtrl
{
public:
    MockTextCtrl() {}
    void AppendText( const wxString& text ) { m_content += text; }
    void SetValue( const wxString& text ) { m_content = text; }
    wxString GetValue() const { return m_content; }
    bool IsEmpty() const { return m_content.IsEmpty(); }
    void Clear() { m_content.Clear(); }

private:
    wxString m_content;
};

// Mock wxStatusBar for testing
class MockStatusBar
{
public:
    MockStatusBar() { m_fields.resize( 3 ); }  // Default 3 fields
    void SetStatusText( const wxString& text, int field = 0 )
    {
        if( field >= 0 && field < (int)m_fields.size() )
            m_fields[field] = text;
    }
    wxString GetStatusText( int field = 0 ) const
    {
        if( field >= 0 && field < (int)m_fields.size() )
            return m_fields[field];
        return wxEmptyString;
    }

private:
    std::vector<wxString> m_fields;
};

BOOST_AUTO_TEST_SUITE( ReporterTests )

BOOST_AUTO_TEST_CASE( BaseReporter_Constructor )
{
    TEST_REPORTER reporter;

    BOOST_CHECK( !reporter.HasMessage() );
    BOOST_CHECK( reporter.GetUnits() == EDA_UNITS::MM );
    BOOST_CHECK( !reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
}

BOOST_AUTO_TEST_CASE( BaseReporter_SingleReport )
{
    TEST_REPORTER reporter;

    reporter.Report( wxT("Test message"), RPT_SEVERITY_INFO );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 1 );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );

    const auto& messages = reporter.GetMessages();
    BOOST_CHECK_EQUAL( messages.size(), 1 );
    BOOST_CHECK_EQUAL( messages[0].first, wxString("Test message") );
    BOOST_CHECK_EQUAL( messages[0].second, RPT_SEVERITY_INFO );
}

BOOST_AUTO_TEST_CASE( BaseReporter_MultipleReports )
{
    TEST_REPORTER reporter;

    reporter.Report( wxT("Info message"), RPT_SEVERITY_INFO );
    reporter.Report( wxT("Warning message"), RPT_SEVERITY_WARNING );
    reporter.Report( wxT("Error message"), RPT_SEVERITY_ERROR );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 3 );

    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO | RPT_SEVERITY_WARNING ) );
}

BOOST_AUTO_TEST_CASE( BaseReporter_ReportTail )
{
    TEST_REPORTER reporter;

    reporter.ReportTail( wxT("Tail message"), RPT_SEVERITY_INFO );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 1 );

    const auto& messages = reporter.GetMessages();
    BOOST_CHECK_EQUAL( messages[0].first, wxString("Tail message") );
    BOOST_CHECK_EQUAL( messages[0].second, RPT_SEVERITY_INFO );
}

BOOST_AUTO_TEST_CASE( BaseReporter_ReportHead )
{
    TEST_REPORTER reporter;

    reporter.ReportHead( wxT("Head message"), RPT_SEVERITY_WARNING );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 1 );

    const auto& messages = reporter.GetMessages();
    BOOST_CHECK_EQUAL( messages[0].first, wxString("Head message") );
    BOOST_CHECK_EQUAL( messages[0].second, RPT_SEVERITY_WARNING );
}

BOOST_AUTO_TEST_CASE( BaseReporter_OperatorLeftShift )
{
    TEST_REPORTER reporter;

    reporter << wxT("Stream message");

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 1 );

    const auto& messages = reporter.GetMessages();
    BOOST_CHECK_EQUAL( messages[0].first, wxString("Stream message") );
    BOOST_CHECK_EQUAL( messages[0].second, RPT_SEVERITY_UNDEFINED );
}

BOOST_AUTO_TEST_CASE( BaseReporter_CharPointerReport )
{
    TEST_REPORTER reporter;

    reporter.Report( "C-style string", RPT_SEVERITY_ERROR );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 1 );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) );
}

BOOST_AUTO_TEST_CASE( BaseReporter_Clear )
{
    TEST_REPORTER reporter;

    reporter.Report( wxT("Message 1"), RPT_SEVERITY_INFO );
    reporter.Report( wxT("Message 2"), RPT_SEVERITY_WARNING );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) );

    reporter.Clear();

    BOOST_CHECK( !reporter.HasMessage() );
    BOOST_CHECK( !reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
    BOOST_CHECK( !reporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 0 );
}

BOOST_AUTO_TEST_CASE( BaseReporter_SeverityMask )
{
    TEST_REPORTER reporter;

    // Test individual severities
    reporter.Report( wxT("Info"), RPT_SEVERITY_INFO );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
    BOOST_CHECK( !reporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) );

    reporter.Report( wxT("Warning"), RPT_SEVERITY_WARNING );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO | RPT_SEVERITY_WARNING ) );

    // Test combined mask
    BOOST_CHECK( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) );
    BOOST_CHECK( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ACTION ) );
}

BOOST_AUTO_TEST_CASE( BaseReporter_EmptyMessage )
{
    TEST_REPORTER reporter;

    reporter.Report( wxT(""), RPT_SEVERITY_INFO );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 1 );

    const auto& messages = reporter.GetMessages();
    BOOST_CHECK_EQUAL( messages[0].first, wxString("") );
}

BOOST_AUTO_TEST_CASE( BaseReporter_LongMessage )
{
    TEST_REPORTER reporter;
    wxString longMessage( 10000, 'A' );

    reporter.Report( longMessage, RPT_SEVERITY_INFO );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK_EQUAL( reporter.GetMessageCount(), 1 );

    const auto& messages = reporter.GetMessages();
    BOOST_CHECK_EQUAL( messages[0].first, longMessage );
}

BOOST_AUTO_TEST_CASE( WxStringReporter_BasicFunctionality )
{
    WX_STRING_REPORTER reporter;

    BOOST_CHECK( !reporter.HasMessage() );

    reporter.Report( wxT("Test message"), RPT_SEVERITY_INFO );

    BOOST_CHECK( reporter.HasMessage() );
    BOOST_CHECK( !reporter.GetMessages().IsEmpty() );
    BOOST_CHECK( reporter.GetMessages().Contains( wxT("Test message") ) );
}

BOOST_AUTO_TEST_CASE( WxStringReporter_MultipleMessages )
{
    WX_STRING_REPORTER reporter;

    reporter.Report( wxT("Message 1"), RPT_SEVERITY_INFO );
    reporter.Report( wxT("Message 2"), RPT_SEVERITY_WARNING );

    BOOST_CHECK( reporter.HasMessage() );

    const wxString& messages = reporter.GetMessages();
    BOOST_CHECK( messages.Contains( wxT("Message 1") ) );
    BOOST_CHECK( messages.Contains( wxT("Message 2") ) );
}

BOOST_AUTO_TEST_CASE( WxStringReporter_Clear )
{
    WX_STRING_REPORTER reporter;

    reporter.Report( wxT("Test message"), RPT_SEVERITY_INFO );
    BOOST_CHECK( reporter.HasMessage() );

    reporter.Clear();

    BOOST_CHECK( !reporter.HasMessage() );
    BOOST_CHECK( reporter.GetMessages().IsEmpty() );
}

BOOST_AUTO_TEST_CASE( WxStringReporter_SeverityMask )
{
    WX_STRING_REPORTER reporter;

    reporter.Report( wxT("Info"), RPT_SEVERITY_INFO );
    reporter.Report( wxT("Warning"), RPT_SEVERITY_WARNING );

    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO | RPT_SEVERITY_WARNING ) );
    BOOST_CHECK( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) );
}

BOOST_AUTO_TEST_CASE( WxStringReporter_OperatorChaining )
{
    WX_STRING_REPORTER reporter;

    reporter.Report( wxT("First"), RPT_SEVERITY_INFO )
           .Report( wxT("Second"), RPT_SEVERITY_WARNING )
           << wxT("Third");

    BOOST_CHECK( reporter.HasMessage() );
    const wxString& messages = reporter.GetMessages();
    BOOST_CHECK( messages.Contains( wxT("First") ) );
    BOOST_CHECK( messages.Contains( wxT("Second") ) );
    BOOST_CHECK( messages.Contains( wxT("Third") ) );
}

BOOST_AUTO_TEST_CASE( NullReporter_Singleton )
{
    REPORTER& reporter1 = NULL_REPORTER::GetInstance();
    REPORTER& reporter2 = NULL_REPORTER::GetInstance();

    BOOST_CHECK_EQUAL( &reporter1, &reporter2 );
}

BOOST_AUTO_TEST_CASE( NullReporter_SeverityMask )
{
    REPORTER& reporter = NULL_REPORTER::GetInstance();

    reporter.Report( wxT("Info"), RPT_SEVERITY_INFO );
    reporter.Report( wxT("Warning"), RPT_SEVERITY_WARNING );

    // NULL_REPORTER should track severity mask even though it doesn't store messages
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_INFO ) );
    BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) );
}

BOOST_AUTO_TEST_CASE( CliReporter_Singleton )
{
    REPORTER& reporter1 = CLI_REPORTER::GetInstance();
    REPORTER& reporter2 = CLI_REPORTER::GetInstance();

    BOOST_CHECK_EQUAL( &reporter1, &reporter2 );
}

BOOST_AUTO_TEST_CASE( StdoutReporter_Singleton )
{
    REPORTER& reporter1 = STDOUT_REPORTER::GetInstance();
    REPORTER& reporter2 = STDOUT_REPORTER::GetInstance();

    BOOST_CHECK_EQUAL( &reporter1, &reporter2 );
}

BOOST_AUTO_TEST_CASE( WxLogReporter_Singleton )
{
    REPORTER& reporter1 = WXLOG_REPORTER::GetInstance();
    REPORTER& reporter2 = WXLOG_REPORTER::GetInstance();

    BOOST_CHECK_EQUAL( &reporter1, &reporter2 );
}

// Note: WX_TEXT_CTRL_REPORTER and STATUSBAR_REPORTER tests would require
// actual wxWidgets objects or more sophisticated mocking

BOOST_AUTO_TEST_CASE( ReporterInterface_Polymorphism )
{
    std::vector<std::unique_ptr<REPORTER>> reporters;
    reporters.push_back( std::make_unique<WX_STRING_REPORTER>() );
    reporters.push_back( std::make_unique<TEST_REPORTER>() );

    for( auto& reporter : reporters )
    {
        reporter->Report( wxT( "Polymorphic test" ), RPT_SEVERITY_INFO );

        BOOST_CHECK( reporter->HasMessage() );
    }
}

BOOST_AUTO_TEST_CASE( ReporterInterface_ChainedOperations )
{
    WX_STRING_REPORTER reporter;

    // Test method chaining
    REPORTER& result = reporter.Report( wxT("First"), RPT_SEVERITY_INFO )
                              .ReportHead( wxT("Head"), RPT_SEVERITY_WARNING )
                              .ReportTail( wxT("Tail"), RPT_SEVERITY_ERROR );

    BOOST_CHECK_EQUAL( &result, &reporter );  // Should return reference to self
    BOOST_CHECK( reporter.HasMessage() );

    const wxString& messages = reporter.GetMessages();
    BOOST_CHECK( messages.Contains( wxT("First") ) );
    BOOST_CHECK( messages.Contains( wxT("Head") ) );
    BOOST_CHECK( messages.Contains( wxT("Tail") ) );
}

BOOST_AUTO_TEST_CASE( ReporterInterface_DefaultSeverity )
{
    TEST_REPORTER reporter;

    // Test default severity parameter
    reporter.Report( wxT("Default severity") );

    const auto& messages = reporter.GetMessages();
    BOOST_CHECK_EQUAL( messages[0].second, RPT_SEVERITY_UNDEFINED );
}

BOOST_AUTO_TEST_CASE( ReporterInterface_UnitsDefault )
{
    TEST_REPORTER reporter;

    BOOST_CHECK( reporter.GetUnits() == EDA_UNITS::MM );
}

BOOST_AUTO_TEST_SUITE_END()