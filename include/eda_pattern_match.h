/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#include <vector>
#include <memory>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/regex.h>

static const int EDA_PATTERN_NOT_FOUND = wxNOT_FOUND;

/*
 * Interface for a pattern matcher, for which there are several implementations
 */
class EDA_PATTERN_MATCH
{
public:
    virtual ~EDA_PATTERN_MATCH() {}

    /**
     * Set the pattern against which candidates will be matched. If the pattern
     * can not be processed, returns false.
     */
    virtual bool SetPattern( const wxString& aPattern ) = 0;

    /**
     * Return the location of a match iff a given candidate string matches the set pattern.
     * Otherwise, return EDA_PATTERN_NOT_FOUND.
     */
    virtual int Find( const wxString& aCandidate ) const = 0;
};


/*
 * Match simple substring
 */
class EDA_PATTERN_MATCH_SUBSTR : public EDA_PATTERN_MATCH
{
public:
    virtual bool SetPattern( const wxString& aPattern ) override;
    virtual int Find( const wxString& aCandidate ) const override;

protected:
    wxString m_pattern;
};


/*
 * Match regular expression
 */
class EDA_PATTERN_MATCH_REGEX : public EDA_PATTERN_MATCH
{
public:
    virtual bool SetPattern( const wxString& aPattern ) override;
    virtual int Find( const wxString& aCandidate ) const override;

protected:
    wxString m_pattern;
    wxRegEx m_regex;
};


class EDA_PATTERN_MATCH_WILDCARD : public EDA_PATTERN_MATCH_REGEX
{
public:
    virtual bool SetPattern( const wxString& aPattern ) override;
    virtual int Find( const wxString& aCandidate ) const override;
};


class EDA_COMBINED_MATCHER
{
public:
    EDA_COMBINED_MATCHER( const wxString &aPattern );

    /*
     * Look in all existing matchers, return the earliest match of any of
     * the existing.
     *
     * @param aTerm                 term to look for
     * @param aMatchersTriggered    out: number of matcher that found the term
     * @param aPostion              out: where the term was found, or EDA_PATTERN_NOT_FOUND
     *
     * @return true if any matchers found the term
     */
    bool Find( const wxString &aTerm, int& aMatchersTriggered, int& aPosition );

    wxString const& GetPattern() const;

private:
    // Add matcher if it can compile the pattern.
    void AddMatcher( const wxString &aPattern, std::unique_ptr<EDA_PATTERN_MATCH> aMatcher );

    std::vector<std::unique_ptr<EDA_PATTERN_MATCH>> m_matchers;
    wxString m_pattern;
};

#endif  // EDA_PATTERN_MATCH_H
