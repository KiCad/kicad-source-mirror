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

#include <wx/wx.h>
#include <richio.h>

#include <chrono>
#include <ios>
#include <functional>
#include <iostream>

#include <fstream>

#include <wx/wfstream.h>
#include <wx/filename.h>

#include <qa_utils/stdstream_line_reader.h>
#include <qa_utils/utility_registry.h>


using CLOCK = std::chrono::steady_clock;
using TIME_PT = std::chrono::time_point<CLOCK>;


struct BENCH_REPORT
{
    unsigned linesRead;

    /**
     * Char accumulator, used to prevent compilers optimising away
     * otherwise unused line buffers, and also as a primitive sanity
     * check that the same lines were read
     */
    unsigned charAcc;

    std::chrono::milliseconds benchDurMs;
};


using BENCH_FUNC = std::function<void(const wxFileName&, int, BENCH_REPORT&)>;


struct BENCHMARK
{
    char triggerChar;
    BENCH_FUNC func;
    wxString name;
};


/**
 * Benchmark using a raw std::ifstream, with no LINE_READER
 * wrapper. The stream is recreated for each cycle.
 */
static void bench_fstream( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    std::string line;

    for( int i = 0; i < aReps; ++i)
    {
        std::ifstream fstr( aFile.GetFullPath().fn_str() );

        while( getline( fstr, line ) )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) line[0];
        }

        fstr.close();
    }
}


/**
 * Benchmark using a raw std::ifstream, with no LINE_READER
 * wrapper. The stream is not recreated for each cycle, just reset.
 */
static void bench_fstream_reuse( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    std::string line;
    std::ifstream fstr( aFile.GetFullPath().fn_str() );

    for( int i = 0; i < aReps; ++i)
    {
        while( getline( fstr, line ) )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) line[0];
        }
        fstr.clear() ;
        fstr.seekg(0, std::ios::beg) ;
    }

    fstr.close();
}


/**
 * Benchmark using a given LINE_READER implementation.
 * The LINE_READER is recreated for each cycle.
 */
template<typename LR>
static void bench_line_reader( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    for( int i = 0; i < aReps; ++i)
    {
        LR fstr( aFile.GetFullPath() );
        while( fstr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) fstr.Line()[0];
        }
    }
}


/**
 * Benchmark using a given LINE_READER implementation.
 * The LINE_READER is rewound for each cycle, not recreated.
 */
template<typename LR>
static void bench_line_reader_reuse( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    LR fstr( aFile.GetFullPath() );
    for( int i = 0; i < aReps; ++i)
    {

        while( fstr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) fstr.Line()[0];
        }

        fstr.Rewind();
    }
}


/**
 * Benchmark using STRING_LINE_READER on string data read into memory from a file
 * using std::ifstream, but read the data fresh from the file each time
 */
static void bench_string_lr( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    for( int i = 0; i < aReps; ++i)
    {
        std::ifstream ifs( aFile.GetFullPath().ToStdString() );
        std::string content((std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        STRING_LINE_READER fstr( content, aFile.GetFullPath() );
        while( fstr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) fstr.Line()[0];
        }
    }
}


/**
 * Benchmark using STRING_LINE_READER on string data read into memory from a file
 * using std::ifstream
 *
 * The STRING_LINE_READER is not reused (it cannot be rewound),
 * but the file is read only once
 */
static void bench_string_lr_reuse( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    std::ifstream ifs( aFile.GetFullPath().ToStdString() );
    std::string content((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());

    for( int i = 0; i < aReps; ++i)
    {
        STRING_LINE_READER fstr( content, aFile.GetFullPath() );
        while( fstr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) fstr.Line()[0];
        }
    }
}


/**
 * Benchmark using an INPUTSTREAM_LINE_READER with a given
 * wxInputStream implementation.
 * The wxInputStream is recreated for each cycle.
 */
template<typename S>
static void bench_wxis( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    S fileStream( aFile.GetFullPath() );

    for( int i = 0; i < aReps; ++i)
    {
        INPUTSTREAM_LINE_READER istr( &fileStream, aFile.GetFullPath() );

        while( istr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) istr.Line()[0];
        }

        fileStream.SeekI( 0 );
    }
}


/**
 * Benchmark using an INPUTSTREAM_LINE_READER with a given
 * wxInputStream implementation.
 * The wxInputStream is reset for each cycle.
 */
template<typename S>
static void bench_wxis_reuse( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    S fileStream( aFile.GetFullPath() );
    INPUTSTREAM_LINE_READER istr( &fileStream, aFile.GetFullPath() );

    for( int i = 0; i < aReps; ++i)
    {
        while( istr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) istr.Line()[0];
        }

        fileStream.SeekI( 0 );
    }
}


/**
 * Benchmark using a INPUTSTREAM_LINE_READER with a given
 * wxInputStream implementation, buffered with wxBufferedInputStream.
 * The wxInputStream is recreated for each cycle.
 */
template<typename WXIS>
static void bench_wxbis( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    WXIS fileStream( aFile.GetFullPath() );
    wxBufferedInputStream bufferedStream( fileStream );

    for( int i = 0; i < aReps; ++i)
    {
        INPUTSTREAM_LINE_READER istr( &bufferedStream, aFile.GetFullPath() );

        while( istr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) istr.Line()[0];
        }

        fileStream.SeekI( 0 );
    }
}


/**
 * Benchmark using a INPUTSTREAM_LINE_READER with a given
 * wxInputStream implementation, buffered with wxBufferedInputStream.
 * The wxInputStream is reset for each cycle.
 */
template<typename WXIS>
static void bench_wxbis_reuse( const wxFileName& aFile, int aReps, BENCH_REPORT& report )
{
    WXIS fileStream( aFile.GetFullPath() );
    wxBufferedInputStream bufferedStream( fileStream );

    INPUTSTREAM_LINE_READER istr( &bufferedStream, aFile.GetFullPath() );

    for( int i = 0; i < aReps; ++i)
    {
        while( istr.ReadLine() )
        {
            report.linesRead++;
            report.charAcc += (unsigned char) istr.Line()[0];
        }

        fileStream.SeekI( 0 );
    }
}

/**
 * List of available benchmarks
 */
static std::vector<BENCHMARK> benchmarkList =
{
    { 'f', bench_fstream, "std::fstream" },
    { 'F', bench_fstream_reuse, "std::fstream, reused" },
    { 'r', bench_line_reader<FILE_LINE_READER>, "RichIO FILE_L_R" },
    { 'R', bench_line_reader_reuse<FILE_LINE_READER>, "RichIO FILE_L_R, reused" },
    { 'n', bench_line_reader<IFSTREAM_LINE_READER>, "std::ifstream L_R" },
    { 'N', bench_line_reader_reuse<IFSTREAM_LINE_READER>, "std::ifstream L_R, reused" },
    { 's', bench_string_lr, "RichIO STRING_L_R"},
    { 'S', bench_string_lr_reuse, "RichIO STRING_L_R, reused"},
    { 'w', bench_wxis<wxFileInputStream>, "wxFileIStream" },
    { 'W', bench_wxis<wxFileInputStream>, "wxFileIStream, reused" },
    { 'g', bench_wxis<wxFFileInputStream>, "wxFFileIStream" },
    { 'G', bench_wxis_reuse<wxFFileInputStream>, "wxFFileIStream, reused" },
    { 'b', bench_wxbis<wxFileInputStream>, "wxFileIStream. buf'd" },
    { 'B', bench_wxbis_reuse<wxFileInputStream>, "wxFileIStream, buf'd, reused" },
    { 'c', bench_wxbis<wxFFileInputStream>, "wxFFileIStream. buf'd" },
    { 'C', bench_wxbis_reuse<wxFFileInputStream>, "wxFFileIStream, buf'd, reused" },
};


/**
 * Construct string of all flags used for specifying benchmarks
 * on the command line
 */
static wxString getBenchFlags()
{
    wxString flags;

    for( auto& bmark : benchmarkList )
    {
        flags << bmark.triggerChar;
    }

    return flags;
}


/**
 * Usage description of a benchmakr spec
 */
static wxString getBenchDescriptions()
{
    wxString desc;

    for( auto& bmark : benchmarkList )
    {
        desc << "    " << bmark.triggerChar << ": " << bmark.name << "\n";
    }

    return desc;
}


BENCH_REPORT executeBenchMark( const BENCHMARK& aBenchmark, int aReps,
        const wxFileName& aFilename )
{
    BENCH_REPORT report = {};

    TIME_PT start = CLOCK::now();
    aBenchmark.func( aFilename, aReps, report );
    TIME_PT end = CLOCK::now();

    using std::chrono::milliseconds;
    using std::chrono::duration_cast;

    report.benchDurMs = duration_cast<milliseconds>( end - start );

    return report;
}


int io_benchmark_func( int argc, char* argv[] )
{
    auto& os = std::cout;

    if (argc < 3)
    {
        os << "Usage: " << argv[0] << " <FILE> <REPS> [" << getBenchFlags() << "]\n\n";
        os << "Benchmarks:\n";
        os << getBenchDescriptions();
        return KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    wxFileName inFile( argv[1] );

    long reps = 0;
    wxString( argv[2] ).ToLong( &reps );

    // get the benchmark to do, or all of them if nothing given
    wxString bench;
    if ( argc == 4 )
        bench = argv[3];

    os << "IO Bench Mark Util" << std::endl;

    os << "  Benchmark file: " << inFile.GetFullPath() << std::endl;
    os << "  Repetitions:    " << (int) reps << std::endl;
    os << std::endl;

    for( auto& bmark : benchmarkList )
    {
        if( bench.size() && !bench.Contains( bmark.triggerChar ) )
            continue;

        BENCH_REPORT report = executeBenchMark( bmark, reps, inFile );

        os << wxString::Format( "%-30s %u lines, acc: %u in %u ms",
                bmark.name, report.linesRead, report.charAcc, (int) report.benchDurMs.count() )
            << std::endl;;
    }

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "io_benchmark",
        "Benchmark various kinds of IO methods",
        io_benchmark_func,
} );