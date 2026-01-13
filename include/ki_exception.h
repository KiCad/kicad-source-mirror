/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef KI_EXCEPTION_H_
#define KI_EXCEPTION_H_

#include <kicommon.h>
#include <wx/string.h>


/**
 * @ingroup exception_types
 * @{
 */


/// macro which captures the "call site" values of __FILE_, __FUNCTION__ & __LINE__
#define THROW_IO_ERROR( msg )   throw IO_ERROR( msg, __FILE__, __FUNCTION__, __LINE__ )

/**
 * Hold a translatable error message and may be used when throwing exceptions containing a
 * translated error message.
 */
class KICOMMON_API KI_PARAM_ERROR // similar to std::invalid_argument for instance
{
public:
    /**
     * Constructor
     */
    KI_PARAM_ERROR( const wxString& aMessage )
    {
        m_message = aMessage;
    }

    KI_PARAM_ERROR() {}

    const wxString What() const
    {
        return m_message;
    }

    virtual ~KI_PARAM_ERROR() throw () {}

private:
    wxString    m_message;
};


/**
 * Hold an error message and may be used when throwing exceptions containing meaningful
 * error messages.
 *
 * @author Dick Hollenbeck
 */
class KICOMMON_API IO_ERROR : public std::exception
{
public:
    /**
     * Use macro #THROW_IO_ERROR() to wrap a call to this constructor at the call site.
     *
     * @param aProblem is Problem() text.
     * @param aThrowersFile is the __FILE__ preprocessor macro but generated at the source
     *                      file of thrower.
     * @param aThrowersFunction is the function name at the throw site.
     * @param aThrowersLineNumber is the source code line number of the throw.
     */
    IO_ERROR( const wxString& aProblem, const char* aThrowersFile,
              const char* aThrowersFunction, int aThrowersLineNumber )
    {
        init( aProblem, aThrowersFile, aThrowersFunction, aThrowersLineNumber );
    }

    IO_ERROR() = default;
    ~IO_ERROR() throw() {}

    void init( const wxString& aProblem, const char* aThrowersFile,
               const char* aThrowersFunction, int aThrowersLineNumber );

    virtual const wxString Problem() const;         ///< what was the problem?
    virtual const wxString Where() const;           ///< where did the Problem() occur?

    virtual const char*    what() const throw() override; ///< std::exception interface, returned as UTF-8
    virtual const wxString What() const;            ///< A composite of Problem() and Where()

protected:
    wxString    problem;
    wxString    where;
};


/**
 * A filename or source description, a problem input line, a line number, a byte
 * offset, and an error message which contains the caller's report and his call
 * site information: CPP source file, function, and line number.
 *
 * @author Dick Hollenbeck
 */
struct KICOMMON_API PARSE_ERROR : public IO_ERROR
{
    int         lineNumber;     ///< at which line number, 1 based index.
    int         byteIndex;      ///< at which byte offset within the line, 1 based index

    /// problem line of input [say, from a LINE_READER].
    /// this is brought up in original byte format rather than wxString form, in case
    /// there was a problem with the encoding, in which case converting to wxString is
    /// not reliable in this context.
    std::string inputLine;

    /**
     * Normally called via the macro #THROW_PARSE_ERROR so that __FILE__, __FUNCTION__,
     * and __LINE__ can be captured from the call site.
     */
    PARSE_ERROR( const wxString& aProblem, const char* aThrowersFile,
                 const char* aThrowersFunction, int aThrowersLineNumber,
                 const wxString& aSource, const char* aInputLine,
                 int aLineNumber, int aByteIndex ) :
        IO_ERROR()
    {
        init( aProblem, aThrowersFile, aThrowersFunction, aThrowersLineNumber,
              aSource, aInputLine, aLineNumber, aByteIndex );
    }

    void init( const wxString& aProblem, const char* aThrowersFile,
               const char* aThrowersFunction, int aThrowersLineNumber,
               const wxString& aSource, const char* aInputLine,
               int aLineNumber, int aByteIndex );

    ~PARSE_ERROR() throw () {}

    const wxString ParseProblem() { return parseProblem; }

protected:
    PARSE_ERROR() :
        IO_ERROR(),
        lineNumber( 0 ),
        byteIndex( 0 )
    {}

protected:
    wxString parseProblem;
};


#define THROW_PARSE_ERROR( aProblem, aSource, aInputLine, aLineNumber, aByteIndex )      \
    throw PARSE_ERROR( aProblem, __FILE__, __FUNCTION__, __LINE__, aSource, aInputLine,  \
                       aLineNumber, aByteIndex )


/**
 * Variant of #PARSE_ERROR indicating that a syntax or related error was likely caused
 * by a file generated by a newer version of KiCad than this. This can be used to generate
 * more informative error messages.
 */
struct KICOMMON_API FUTURE_FORMAT_ERROR : public PARSE_ERROR
{
    wxString requiredVersion;   ///< Date of KiCad file format required to open file
    wxString requiredGenerator; ///< Version of KiCad required to open file

    FUTURE_FORMAT_ERROR( const wxString& aRequiredVersion,
                         const wxString& aRequiredGenerator = wxEmptyString );
    FUTURE_FORMAT_ERROR( const PARSE_ERROR& aParseError, const wxString& aRequiredVersion,
                         const wxString& aRequiredGenerator = wxEmptyString );
    ~FUTURE_FORMAT_ERROR() throw () {}

    void init( const wxString& aRequiredVersion,
               const wxString& aRequiredGenerator = wxEmptyString );
};

/** @} exception_types */

#endif // KI_EXCEPTION_H_
