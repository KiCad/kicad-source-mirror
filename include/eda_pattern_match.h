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

/**
 * @file eda_pattern_match.h
 * @brief Abstract pattern-matching tool and implementations.
 */

#ifndef EDA_PATTERN_MATCH_H
#define EDA_PATTERN_MATCH_H

#include <kicommon.h>
#include <vector>
#include <map>
#include <memory>
#include <wx/string.h>
#include <wx/regex.h>

static const int EDA_PATTERN_NOT_FOUND = wxNOT_FOUND;

/**
 * A structure for storing weighted search terms.
 *
 * @note An exact match is scored at 8 * Score while a match at the start of the text is scored
 * at 2 * Score.
 */
struct KICOMMON_API SEARCH_TERM
{
    SEARCH_TERM( const wxString& aText, int aScore ) :
            Text( aText ),
            Score( aScore ),
            Normalized( false )
    {}

    wxString Text;
    int      Score;
    bool     Normalized;
};


/**
 * Interface for a pattern matcher for which there are several implementations.
 */
class KICOMMON_API EDA_PATTERN_MATCH
{
public:
    struct FIND_RESULT
    {
        int start  = EDA_PATTERN_NOT_FOUND;
        int length = 0;

        bool valid() const
        {
            return start != EDA_PATTERN_NOT_FOUND;
        }

        explicit operator bool() const
        {
            return valid();
        }
    };

    virtual ~EDA_PATTERN_MATCH() {}

    /**
     * Set the pattern against which candidates will be matched.
     *
     * @return false if the pattern not be processed.
     */
    virtual bool SetPattern( const wxString& aPattern ) = 0;

    /**
     * Return the pattern passed to SetPattern().
     */
    virtual wxString const& GetPattern() const = 0;

    /**
     * Return the location and possibly length of a match if a given candidate
     * string matches the set pattern.
     *
     * Otherwise, return an invalid #FIND_RESULT.
     */
    virtual FIND_RESULT Find( const wxString& aCandidate ) const = 0;
};


/**
 * Match simple substring.
 */
class KICOMMON_API EDA_PATTERN_MATCH_SUBSTR : public EDA_PATTERN_MATCH
{
public:

    virtual bool SetPattern( const wxString& aPattern ) override;
    virtual wxString const& GetPattern() const override;
    virtual FIND_RESULT     Find( const wxString& aCandidate ) const override;

protected:
    wxString m_pattern;
};


/**
 * Match regular expression.
 */
class KICOMMON_API EDA_PATTERN_MATCH_REGEX : public EDA_PATTERN_MATCH
{
public:

    virtual bool SetPattern( const wxString& aPattern ) override;
    virtual wxString const& GetPattern() const override;
    virtual FIND_RESULT     Find( const wxString& aCandidate ) const override;

protected:
    wxString m_pattern;
    wxRegEx m_regex;
};


class KICOMMON_API EDA_PATTERN_MATCH_REGEX_ANCHORED : public EDA_PATTERN_MATCH_REGEX
{
public:
    virtual bool SetPattern( const wxString& aPattern ) override;
};


class KICOMMON_API EDA_PATTERN_MATCH_WILDCARD : public EDA_PATTERN_MATCH_REGEX
{
public:

    virtual bool SetPattern( const wxString& aPattern ) override;
    virtual wxString const& GetPattern() const override;
    virtual FIND_RESULT     Find( const wxString& aCandidate ) const override;

protected:
    wxString m_wildcard_pattern;
};


class KICOMMON_API EDA_PATTERN_MATCH_WILDCARD_ANCHORED : public EDA_PATTERN_MATCH_WILDCARD
{
public:
    virtual bool SetPattern( const wxString& aPattern ) override;
};


/**
 * Relational match.
 *
 * Matches tokens of the format:
 *
 *      key:value       or      key=value
 *
 * with search patterns of the format:
 *
 *      key<value, key<=value, key=value, key>=value, key>value
 *
 * by parsing the value numerically and comparing.
 */
class KICOMMON_API EDA_PATTERN_MATCH_RELATIONAL : public EDA_PATTERN_MATCH
{
public:
    virtual bool SetPattern( const wxString& aPattern ) override;
    virtual wxString const& GetPattern() const override;
    virtual FIND_RESULT     Find( const wxString& aCandidate ) const override;
    int FindOne( const wxString& aCandidate ) const;

protected:

    enum RELATION { LT, LE, EQ, GE, GT, ANY };

    wxString m_pattern;
    wxString m_key;
    RELATION m_relation;
    double   m_value;

    static const std::map<wxString, double> m_units;
};


enum COMBINED_MATCHER_CONTEXT
{
    CTX_LIBITEM,
    CTX_NET,
    CTX_NETCLASS,
    CTX_SIGNAL,
    CTX_SEARCH
};


class KICOMMON_API EDA_COMBINED_MATCHER
{
public:
    EDA_COMBINED_MATCHER( const wxString& aPattern, COMBINED_MATCHER_CONTEXT aContext );

    /**
     * Deleted copy or else we have to implement copy constructors for all EDA_PATTERN_MATCH classes
     * due to this class' m_matchers member being copied.
     */
    EDA_COMBINED_MATCHER( EDA_COMBINED_MATCHER const& ) = delete;

    /**
     * Deleted copy or else we have to implement copy constructors for all EDA_PATTERN_MATCH classes
     * due to this class' m_matchers member being copied
     */
    EDA_COMBINED_MATCHER& operator=( EDA_COMBINED_MATCHER const& ) = delete;

    /**
     * Look in all existing matchers, return the earliest match of any of the existing.
     *
     * @param aTerm                 term to look for.
     * @param aMatchersTriggered    out: number of matcher that found the term.
     * @param aPostion              out: where the term was found, or #EDA_PATTERN_NOT_FOUND.
     *
     * @return true if any matchers found the term
     */
    bool Find( const wxString& aTerm, int& aMatchersTriggered, int& aPosition );

    bool Find( const wxString& aTerm );

    bool StartsWith( const wxString& aTerm );

    const wxString& GetPattern() const;

    int ScoreTerms( std::vector<SEARCH_TERM>& aWeightedTerms );

private:
    /// Add matcher if it can compile the pattern.
    void AddMatcher( const wxString& aPattern, std::unique_ptr<EDA_PATTERN_MATCH> aMatcher );

    std::vector<std::unique_ptr<EDA_PATTERN_MATCH>> m_matchers;
    wxString m_pattern;
};

#endif  // EDA_PATTERN_MATCH_H
