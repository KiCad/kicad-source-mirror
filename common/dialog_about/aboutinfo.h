/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef ABOUTAPPINFO_H
#define ABOUTAPPINFO_H

#include <wx/aboutdlg.h>
#include <wx/bitmap.h>
#include <wx/dynarray.h>

#include <bitmaps/bitmap_types.h>

class CONTRIBUTOR;

WX_DECLARE_OBJARRAY( CONTRIBUTOR, CONTRIBUTORS );


/**
 * An object of this class is meant to be used to store application specific information
 * like who has contributed in which area of the application, the license, copyright
 * and other descriptive information.
 */
class ABOUT_APP_INFO
{
public:
    ABOUT_APP_INFO() {};
    virtual ~ABOUT_APP_INFO() {};

    void AddDeveloper( const CONTRIBUTOR* developer )
    {
        if( developer != NULL )
            mDevelopers.Add( developer );
    }

    void AddDocWriter( const CONTRIBUTOR* docwriter )
    {
        if( docwriter != NULL )
            mDocWriters.Add( docwriter );
    }

    void AddLibrarian( const CONTRIBUTOR* aLibrarian )
    {
        if( aLibrarian )
            mLibrarians.Add( aLibrarian );
    }

    void AddArtist( const CONTRIBUTOR* artist )
    {
        if( artist != NULL )
            mArtists.Add( artist );
    }

    void AddTranslator( const CONTRIBUTOR* translator )
    {
        if( translator != NULL )
            mTranslators.Add( translator );
    }

    void AddPackager( const CONTRIBUTOR* packager )
    {
        if( packager   != NULL )
            mPackagers.Add( packager );
    }

    CONTRIBUTORS GetDevelopers()  { return mDevelopers; }
    CONTRIBUTORS GetDocWriters()  { return mDocWriters; }
    CONTRIBUTORS GetLibrarians()  { return mLibrarians; }
    CONTRIBUTORS GetArtists()     { return mArtists; }
    CONTRIBUTORS GetTranslators() { return mTranslators; }
    CONTRIBUTORS GetPackagers()   { return mPackagers; }

    void SetDescription( const wxString& text ) { description = text; }
    wxString& GetDescription() { return description; }

    void SetLicense( const wxString& text ) { license = text; }
    wxString& GetLicense() { return license; }

    void SetAppName( const wxString& name ) { appName = name; }
    wxString& GetAppName() { return appName; }

    void SetBuildVersion( const wxString& version ) { buildVersion = version; }
    wxString& GetBuildVersion() { return buildVersion; }

    void SetBuildDate( const wxString& date ) { buildDate = date; }
    wxString& GetBuildDate() { return buildDate; }

    void SetLibVersion( const wxString& version ) { libVersion = version; }
    wxString& GetLibVersion() { return libVersion; }

    void SetAppIcon( const wxIcon& aIcon ) { m_appIcon = aIcon; }
    wxIcon& GetAppIcon() { return m_appIcon; }

    /// Wrapper to manage memory allocation for bitmaps.
    wxBitmap* CreateKiBitmap( BITMAPS aBitmap )
    {
        m_bitmaps.emplace_back( KiBitmapNew( aBitmap ) );
        return m_bitmaps.back().get();
    }

private:
    CONTRIBUTORS mDevelopers;
    CONTRIBUTORS mDocWriters;
    CONTRIBUTORS mLibrarians;
    CONTRIBUTORS mArtists;
    CONTRIBUTORS mTranslators;
    CONTRIBUTORS mPackagers;

    wxString     description;
    wxString     license;

    wxString     appName;
    wxString     buildVersion;
    wxString     buildDate;
    wxString     libVersion;

    wxIcon       m_appIcon;

    /// Bitmaps to be freed when the dialog is closed.
    std::vector<std::unique_ptr<wxBitmap>> m_bitmaps;
};


/**
 * A contributor, a person which was involved in the development of the application
 * or which has contributed in any kind somehow to the project.
 *
 * A contributor consists of the following mandatory information:
 * - Name
 *
 * Each contributor can have optional information assigned like:
 * - Extra identifying information
 * - A category
 * - A category specific icon
 */
class CONTRIBUTOR
{
public:
    CONTRIBUTOR( const wxString& aName, const wxString& aCategory,
                 const wxString& aUrl = wxEmptyString )
    {
        m_checked = false;
        m_name = aName;
        m_url = aUrl,
        m_category = aCategory;
    }

    virtual ~CONTRIBUTOR() {}

    wxString& GetName()     { return m_name; }
    wxString& GetUrl()      { return m_url; }
    wxString& GetCategory() { return m_category; }
    void SetChecked( bool status ) { m_checked = status; }
    bool IsChecked() { return m_checked; }

private:
    wxString  m_name;
    wxString  m_url;
    wxString  m_category;
    bool      m_checked;
};

#endif // ABOUTAPPINFO_H
