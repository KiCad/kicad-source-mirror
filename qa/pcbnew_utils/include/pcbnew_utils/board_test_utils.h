/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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


#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <optional>
#include <string>

#include <wx/string.h>

#include <reporter.h>
#include <core/typeinfo.h>
#include <tool/tool_manager.h>
#include <pcb_io/pcb_io.h>

class BOARD;
class BOARD_ITEM;
class FOOTPRINT;
class KIID;
class PCB_TEXT;
class PCB_SHAPE;
class ZONE;
class PAD;
class SHAPE_POLY_SET;
class SETTINGS_MANAGER;

namespace KI_TEST
{
class DUMMY_TOOL : public TOOL_BASE
{
public:
    DUMMY_TOOL() :
        TOOL_BASE( BATCH, TOOL_MANAGER::MakeToolId( "" ), "testframework.dummytool" )
    {};

    void Reset( RESET_REASON aReason ) override {}
};


/**
 * A helper that contains logic to assist in dumping boards to
 * disk depending on some environment variables.
 *
 * This is useful when setting up or verifying unit tests that work on BOARD
 * objects.
 *
 * To dump files set the KICAD_TEST_DUMP_BOARD_FILES environment variable.
 * Files will be written to the system temp directory (/tmp on Linux, or as set
 * by $TMP and friends).
 */
class BOARD_DUMPER
{
public:
    BOARD_DUMPER();

    void DumpBoardToFile( BOARD& aBoard, const std::string& aName ) const;

    const bool m_dump_boards;
};


class CONSOLE_LOG
{
public:
    enum COLOR
    {
        RED = 0,
        GREEN,
        DEFAULT
    };

    CONSOLE_LOG(){};

    void PrintProgress( const wxString& aMessage )
    {
        if( m_lastLineIsProgressBar )
            eraseLastLine();

        printf( "%s", (const char*) aMessage.c_str() );
        fflush( stdout );

        m_lastLineIsProgressBar = true;
    }


    void Print( const wxString& aMessage )
    {
        if( m_lastLineIsProgressBar )
            eraseLastLine();

        printf( "%s", (const char*) aMessage.c_str() );
        fflush( stdout );

        m_lastLineIsProgressBar = false;
    }


    void SetColor( COLOR color )
    {
        std::map<COLOR, wxString> colorMap = { { RED, "\033[0;31m" },
                                               { GREEN, "\033[0;32m" },
                                               { DEFAULT, "\033[0;37m" } };

        printf( "%s", (const char*) colorMap[color].c_str() );
        fflush( stdout );
    }


private:
    void eraseLastLine()
    {
        printf( "\r\033[K" );
        fflush( stdout );
    }

    bool       m_lastLineIsProgressBar = false;
    std::mutex m_lock;
};


class CONSOLE_MSG_REPORTER : public REPORTER
{
public:
    CONSOLE_MSG_REPORTER( CONSOLE_LOG* log ) : m_log( log ){};
    ~CONSOLE_MSG_REPORTER(){};


    virtual REPORTER& Report( const wxString& aText,
                              SEVERITY        aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        switch( aSeverity )
        {
        case RPT_SEVERITY_ERROR:
            m_log->SetColor( CONSOLE_LOG::RED );
            m_log->Print( "ERROR | " );
            break;

        default: m_log->SetColor( CONSOLE_LOG::DEFAULT ); m_log->Print( "      | " );
        }

        m_log->SetColor( CONSOLE_LOG::DEFAULT );
        m_log->Print( aText + "\n" );
        return *this;
    }

    virtual bool HasMessage() const override { return true; }

private:
    CONSOLE_LOG* m_log;
};


struct BOARD_LOAD_TEST_CASE
{
    wxString m_BoardFileRelativePath;

    // These tests may well test specific versions of the board file format,
    // so don't let it change accidentally in the files (e.g. by resaving in newer KiCad)!
    std::optional<int> m_ExpectedBoardVersion;

    // Be default, printing the board file is sufficent to identify the test case
    friend std::ostream& operator<<( std::ostream& os, const BOARD_LOAD_TEST_CASE& aTestCase )
    {
        os << aTestCase.m_BoardFileRelativePath;
        return os;
    }
};


void LoadBoard( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                std::unique_ptr<BOARD>& aBoard );

/**
 * @brief Get an item from the given board with a certain type and UUID.
 *
 * If this doesn't exist, it's a BOOST_REQUIRE failure.
 *
 * @param aBoard the board to look in
 * @param aItemType the required item type
 * @param aID the required
 * @return BOARD_ITEM& the required board item
 */
BOARD_ITEM& RequireBoardItemWithTypeAndId( const BOARD& aBoard, KICAD_T aItemType,
                                           const KIID& aID );

/**
 * @brief Perform "some test" on a board file loaded from the path,
 * then optionally save and reload and run the test again.
 *
 * The roundtrip is useful to test stability of serialisation/reload.
 *
 * @param aRelativePath relative path of file to load
 * @param aRoundtrip true to save, reload and re-test
 * @param aBoardTestFunction the function that runs tests on the board
 * @param aExpectedBoardVersion the expected board version, or nullopt to not check
 */
void LoadAndTestBoardFile( const wxString aRelativePath, bool aRoundtrip,
                           std::function<void( BOARD& )> aBoardTestFunction,
                           std::optional<int> aExpectedBoardVersion = std::nullopt );

/**
 * Same as LoadAndTestBoardFile, but for footprints
 */
void LoadAndTestFootprintFile( const wxString& aLibRelativePath, const wxString& aFpName,
                               bool                              aRoundtrip,
                               std::function<void( FOOTPRINT& )> aFootprintTestFunction,
                               std::optional<int>                aExpectedFootprintVersion );


void FillZones( BOARD* m_board );


/**
 * Helper method to check if two footprints are semantically the same.
 */
void CheckFootprint( const FOOTPRINT* expected, const FOOTPRINT* fp );

void CheckFpPad( const PAD* expected, const PAD* pad );

void CheckFpText( const PCB_TEXT* expected, const PCB_TEXT* text );

void CheckFpShape( const PCB_SHAPE* expected, const PCB_SHAPE* shape );

void CheckFpZone( const ZONE* expected, const ZONE* zone );

void CheckShapePolySet( const SHAPE_POLY_SET* expected, const SHAPE_POLY_SET* polyset );

/**
 * Print detailed board statistics for debugging using test-framework logging.
 */
void PrintBoardStats( const BOARD* aBoard, const std::string& aBoardName );


/**
 * Custom REPORTER that captures all messages for later analysis in the unit test framework.
 */
class CAPTURING_REPORTER : public REPORTER
{
public:
    struct MESSAGE
    {
        wxString text;
        SEVERITY severity = RPT_SEVERITY_UNDEFINED;
    };

    CAPTURING_REPORTER() :
            m_errorCount( 0 ),
            m_warningCount( 0 ),
            m_infoCount( 0 )
    {
    }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    bool HasMessage() const override { return !m_messages.empty(); }

    EDA_UNITS GetUnits() const override { return EDA_UNITS::MM; }

    void Clear() override
    {
        m_messages.clear();
        m_errorCount = 0;
        m_warningCount = 0;
        m_infoCount = 0;
    }

    void PrintAllMessages( const std::string& aContext ) const;

    int                         GetErrorCount() const { return m_errorCount; }
    int                         GetWarningCount() const { return m_warningCount; }
    const std::vector<MESSAGE>& GetMessages() const { return m_messages; }

private:
    std::vector<MESSAGE> m_messages;
    int                  m_errorCount;
    int                  m_warningCount;
    int                  m_infoCount;
};


/**
 * Attempt to load an board with a given IO plugin, capturing all reporter messages.
 * Returns the board (or nullptr on failure) and populates the reporter.
 */
extern std::unique_ptr<BOARD> LoadBoardWithCapture( PCB_IO& aIoPlugin, const std::string& aFilePath,
                                                    REPORTER& aReporter );


/**
 * Manager for caching loaded boards in memory, to avoid repeatedly loading and parsing the same board.
 *
 * Generally, you might want to use a singleton instance of this class for each PCB_IO plugin.
 *
 * This class can learn additional features such as load profiling, or cache eviction policies.
 */
class CACHED_BOARD_LOADER
{
public:
    /**
     * Get a cached board for the given file path, or load it if not already cached.
     *
     * Probably will be implemented in terms of a PCB_IO inheritor.
     *
     * @param aFilePath the file path to load
     * @return a pointer to the cached board, or nullptr if loading failed
     */
    virtual BOARD* GetCachedBoard( const std::string& aFilePath ) = 0;

protected:
    BOARD* getCachedBoard( PCB_IO& aIoPlugin, const std::string& aFilePath );

private:
    std::map<std::string, std::unique_ptr<BOARD>> m_boardCache;
};

} // namespace KI_TEST
