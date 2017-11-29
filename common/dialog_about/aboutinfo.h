/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2014-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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
            developers.Add( developer );
    }

    void AddDocWriter( const CONTRIBUTOR* docwriter )
    {
        if( docwriter != NULL )
            docwriters.Add( docwriter );
    }

    void AddArtist( const CONTRIBUTOR* artist )
    {
        if( artist != NULL )
            artists.Add( artist );
    }

    void AddTranslator( const CONTRIBUTOR* translator )
    {
        if( translator != NULL )
            translators.Add( translator );
    }

    void AddPackager( const CONTRIBUTOR* packager )
    {
        if( packager   != NULL )
            packagers.Add( packager );
    }

    CONTRIBUTORS GetDevelopers()  { return developers; }
    CONTRIBUTORS GetDocWriters()  { return docwriters; }
    CONTRIBUTORS GetArtists()     { return artists; }
    CONTRIBUTORS GetTranslators() { return translators; }
    CONTRIBUTORS GetPackagers()   { return packagers; }

    void SetDescription( const wxString& text ) { description = text; }
    wxString& GetDescription() { return description; }

    void SetLicense( const wxString& text ) { license = text; }
    wxString& GetLicense() { return license; }

    void SetCopyright( const wxString& text ) { copyright = text; }
    wxString GetCopyright() { return copyright; }

    void SetAppName( const wxString& name ) { appName = name; }
    wxString& GetAppName() { return appName; }

    void SetBuildVersion( const wxString& version ) { buildVersion = version; }
    wxString& GetBuildVersion() { return buildVersion; }

    void SetLibVersion( const wxString& version ) { libVersion = version; }
    wxString& GetLibVersion() { return libVersion; }

    void SetAppIcon( const wxIcon& aIcon ) { m_appIcon = aIcon; }
    wxIcon& GetAppIcon() { return m_appIcon; }

private:
    CONTRIBUTORS developers;
    CONTRIBUTORS docwriters;
    CONTRIBUTORS artists;
    CONTRIBUTORS translators;
    CONTRIBUTORS packagers;

    wxString     description;
    wxString     license;

    wxString     copyright;
    wxString     appName;
    wxString     buildVersion;
    wxString     libVersion;

    wxIcon       m_appIcon;
};


/**
 * A contributor, a person which was involved in the development of the application
 * or which has contributed in any kind somehow to the project.
 *
 * A contributor consists of the following mandatory information:
 * - Name
 *
 * Each contributor can have optional information assigned like:
 * - EMail address
 * - A category
 * - A category specific icon
 */
class CONTRIBUTOR
{
public:
    CONTRIBUTOR( const wxString& aName,
                 const wxString& aEmail = wxEmptyString,
                 const wxString& aCategory = wxEmptyString,
                 wxBitmap*       aIcon = NULL )
    {
        m_checked = false;
        m_name = aName;
        m_email = aEmail;
        m_category = aCategory;
        m_icon = aIcon;
    }

    virtual ~CONTRIBUTOR() {}

    wxString& GetName()     { return m_name; }
    wxString& GetEMail()    { return m_email; }
    wxString& GetCategory() { return m_category; }
    wxBitmap* GetIcon()     { return m_icon; }
    void SetChecked( bool status ) { m_checked = status; }
    bool IsChecked() { return m_checked; }

private:
    wxString  m_name;
    wxString  m_email;
    wxString  m_category;
    wxBitmap* m_icon;
    bool      m_checked;
};

#endif // ABOUTAPPINFO_H
