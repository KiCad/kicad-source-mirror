/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef PROJECT_H_
#define PROJECT_H_

/**
 * @file project.h
 */

#include <vector>
#include <wx/string.h>
#include <wx/filename.h>
#include <core/typeinfo.h>
#include <common.h>

/// A variable name whose value holds the current project directory.
/// Currently an environment variable, eventually a project variable.
#define PROJECT_VAR_NAME            wxT( "KIPRJMOD" )


class wxConfigBase;
class PARAM_CFG;
class FP_LIB_TABLE;
class PART_LIBS;
class SEARCH_STACK;
class S3D_CACHE;
class KIWAY;
class SYMBOL_LIB_TABLE;
class FILENAME_RESOLVER;
class PROJECT_FILE;
class PROJECT_LOCAL_SETTINGS;

#define VTBL_ENTRY      virtual

/**
 * PROJECT
 * holds project specific data.  Because it is in the neutral program top, which
 * is not linked to by subsidiarly DSOs, any functions in this interface must
 * be VTBL_ENTRYs.
 */
class PROJECT
{
    friend class SETTINGS_MANAGER; // so that SM can set project path
    friend class TEST_NETLISTS_FIXTURE; // TODO(JE) make this not required

public:

    /// A PROJECT can hold stuff it knows nothing about, in the form of
    /// _ELEM derivatives. Derive PROJECT elements from this, it has a virtual
    /// destructor, and Elem*() functions can work with it.  Implementation is
    /// opaque in class PROJECT.  If find you have to include derived class headers
    /// in this file, you are doing incompatible with the goal of this class.
    /// Keep knowledge of derived classes opaque to class PROJECT please.
    class _ELEM
    {
    public:
        virtual ~_ELEM() {}

        virtual KICAD_T Type() = 0;     // Sanity-checking for returned values.
    };

    PROJECT();
    VTBL_ENTRY ~PROJECT();

    //-----<Cross Module API>----------------------------------------------------

    VTBL_ENTRY bool TextVarResolver( wxString* aToken ) const;

    VTBL_ENTRY std::map<wxString, wxString>& GetTextVars() const;

    /**
     * Function GetProjectFullName
     * returns the full path and name of the project.  This is the same as the
     * name of the *.pro file and will always be an absolute path.
     */
    VTBL_ENTRY const wxString GetProjectFullName() const;

    /**
     * Function GetProjectPath
     * returns the full path of the project.  This is the path
     * of the *.pro file and will always be an absolute path, ending by a dir separator.
     */
    VTBL_ENTRY const wxString GetProjectPath() const;

    /**
     * Function GetProjectName
     * returns the short name of the project. This is the file name without
     * extension or path.
     */
    VTBL_ENTRY const wxString GetProjectName() const;

    /**
     * Return the name of the sheet identified by the given UUID.
     */
    VTBL_ENTRY const wxString GetSheetName( const KIID& aSheetID );

    /**
     * Function FootprintLibTblName
     * returns the path and filename of this project's fp-lib-table,
     * i.e. the project specific one, not the global one.
     */
    VTBL_ENTRY const wxString FootprintLibTblName() const;

    /**
     * Return the path and file name of this projects symbol library table.
     */
    VTBL_ENTRY const wxString SymbolLibTableName() const;

    VTBL_ENTRY PROJECT_FILE& GetProjectFile() const
    {
        wxASSERT( m_projectFile );
        return *m_projectFile;
    }

    VTBL_ENTRY PROJECT_LOCAL_SETTINGS& GetLocalSettings() const
    {
        wxASSERT( m_localSettings );
        return *m_localSettings;
    }

    /// Retain a number of project specific wxStrings, enumerated here:
    enum RSTRING_T
    {
        DOC_PATH,
        SCH_LIB_PATH,
        SCH_LIB_SELECT,         // eeschema/selpart.cpp
        SCH_LIBEDIT_CUR_LIB,
        SCH_LIBEDIT_CUR_PART,        // eeschema/libeditframe.cpp

        VIEWER_3D_PATH,
        VIEWER_3D_FILTER_INDEX,

        PCB_LIB_NICKNAME,
        PCB_FOOTPRINT,
        PCB_FOOTPRINT_EDITOR_FP_NAME,
        PCB_FOOTPRINT_EDITOR_LIB_NICKNAME,
        PCB_FOOTPRINT_VIEWER_FP_NAME,
        PCB_FOOTPRINT_VIEWER_LIB_NICKNAME,

        RSTRING_COUNT
    };

    /**
     * Function GetRString
     * returns a "retained string", which is any session and project specific string
     * identified in enum RSTRING_T.  Retained strings are not written to disk, and
     * are therefore good only for the current session.
     */
    VTBL_ENTRY  const wxString& GetRString( RSTRING_T aStringId );

    /**
     * Function SetRString
     * stores a "retained string", which is any session and project specific string
     * identified in enum RSTRING_T.  Retained strings are not written to disk, and
     * are therefore good only for the current session.
     */
    VTBL_ENTRY  void SetRString( RSTRING_T aStringId, const wxString& aString );

    /**
     * Enum ELEM_T
     * is the set of _ELEMs that a PROJECT can hold.
     */
    enum ELEM_T
    {
        ELEM_FPTBL,

        ELEM_SCH_PART_LIBS,
        ELEM_SCH_SEARCH_STACK,
        ELEM_3DCACHE,
        ELEM_SYMBOL_LIB_TABLE,

        ELEM_COUNT
    };

    /**
     * Typically wrapped somewhere else in a more meaningful function wrapper.
     * This is a cross module API, therefore the _ELEM destructor is virtual and
     * can point to a destructor function in another link image.  Be careful that
     * that program module is resident at time of destruction.
     * <p>
     * Summary: 1) cross module API, 2) PROJECT knows nothing about _ELEM objects,
     * except how to delete them and set and get pointers to them.
     */
    VTBL_ENTRY  _ELEM*  GetElem( ELEM_T aIndex );
    VTBL_ENTRY  void    SetElem( ELEM_T aIndex, _ELEM* aElem );

    /**
     * Function ElemsClear
     * deletes all the _ELEMs and set their pointers to NULL.
     */
    VTBL_ENTRY void ElemsClear();

    /**
     * Function Clear
     * clears the _ELEMs and RSTRINGs.
     */
    void Clear()        // inline not virtual
    {
        ElemsClear();

        for( unsigned i = 0; i<RSTRING_COUNT;  ++i )
            SetRString( RSTRING_T( i ), wxEmptyString );
    }

    /**
     * Function AbsolutePath
     * fixes up @a aFileName if it is relative to the project's directory to
     * be an absolute path and filename.  This intends to overcome the now missing
     * chdir() into the project directory.
     */
    VTBL_ENTRY const wxString AbsolutePath( const wxString& aFileName ) const;

    /**
     * Return the table of footprint libraries. Requires an active Kiway as
     * this is fetched from pcbnew.
     */
    VTBL_ENTRY FP_LIB_TABLE* PcbFootprintLibs( KIWAY& aKiway );

    //-----</Cross Module API>---------------------------------------------------

    //-----<KIFACE Specific APIs>------------------------------------------------

    // These are the non-virtual DATA LOAD ON DEMAND members. They load project related
    // data on demand, and do so typicallly into m_elems[] at a particular index using
    // SetElem() & GetElem().  That is, they wrap SetElem() and GetElem().
    // To get the data to reload on demand, first SetProjectFullName(),
    // then call SetElem( ELEM_T, NULL ) from client code.

    // non-virtuals resident in PCBNEW link image(s).  By being non-virtual, these
    // functions can get linked into the KIFACE that needs them, and only there.
    // In fact, the other KIFACEs don't even know they exist.
#if defined(PCBNEW) || defined(CVPCB)
    /**
     * Return the table of footprint libraries without Kiway, only from within
     * pcbnew.
     */
    FP_LIB_TABLE* PcbFootprintLibs();

    /**
     * Function Get3DCacheManager
     * returns a pointer to an instance of the 3D cache manager;
     * an instance is created and initialized if appropriate.
     *
     * @return a pointer to an instance of the 3D cache manager or NULL on failure
     */
    S3D_CACHE* Get3DCacheManager( bool updateProjDir = false );

    /// Accessor for 3D path resolver
    FILENAME_RESOLVER* Get3DFilenameResolver();
#endif


#if defined(EESCHEMA)
    // These are all prefaced with "Sch"
    PART_LIBS*  SchLibs();

    /// Accessor for Eeschema search stack.
    SEARCH_STACK*  SchSearchS();

    /// Accessor for project symbol library table.
    SYMBOL_LIB_TABLE* SchSymbolLibTable();

    /// Accessor for 3D path resolver
    FILENAME_RESOLVER* Get3DFilenameResolver() { return nullptr; }
#endif

    //-----</KIFACE Specific APIs>-----------------------------------------------

private:

    /**
     * Sets the:
     * 1) full directory, 2) basename, and 3) extension of the project.  This is
     * the name of the *.pro file with full absolute path and it also defines
     * the name of the project.  The project name and the *.pro file names are
     * exactly the same, providing the *.pro filename is absolute.
     */
    VTBL_ENTRY void setProjectFullName( const wxString& aFullPathAndName );

    /**
     * Sets the backing store file for this project
     * Should only be called by SETTINGS_MANGER on load.
     * @param aFile is a loaded PROJECT_FILE
     */
    VTBL_ENTRY void setProjectFile( PROJECT_FILE* aFile )
    {
        m_projectFile = aFile;
    }

    /**
     * Sets the local settings backing store.  Should only be called by SETTINGS_MANAGER on load.
     * @param aSettings is the local settings object (may or may not exist on disk at this point)
     */
    VTBL_ENTRY void setLocalSettings( PROJECT_LOCAL_SETTINGS* aSettings )
    {
        m_localSettings = aSettings;
    }

    /**
     * Return the full path and file name of the project specific library table \a aLibTableName..
     */
    const wxString libTableName( const wxString& aLibTableName ) const;

    wxFileName      m_project_name;         ///< \<fullpath\>/\<basename\>.pro
    wxString        m_pro_date_and_time;

    /// Backing store for project data -- owned by SETTINGS_MANAGER
    PROJECT_FILE*   m_projectFile;

    /// Backing store for project local settings -- owned by SETTINGS_MANAGER
    PROJECT_LOCAL_SETTINGS* m_localSettings;

    std::map<KIID, wxString>     m_sheetNames;

    /// @see this::SetRString(), GetRString(), and enum RSTRING_T.
    wxString        m_rstrings[RSTRING_COUNT];

    /// @see this::Elem() and enum ELEM_T.
    _ELEM*          m_elems[ELEM_COUNT];
};


#endif  // PROJECT_H_
