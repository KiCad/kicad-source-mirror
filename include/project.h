/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2022 CERN
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
#include <array>
#include <map>
#include <vector>
#include <kiid.h>
#include <wx_filename.h>
#include <wx/string.h>
#include <core/typeinfo.h>

/// A variable name whose value holds the current project directory.
/// Currently an environment variable, eventually a project variable.
#define PROJECT_VAR_NAME            wxT( "KIPRJMOD" )

/// default name for nameless projects
#define NAMELESS_PROJECT _( "untitled" )

class DESIGN_BLOCK_LIBRARY_ADAPTER;
class FP_LIB_TABLE;
class LEGACY_SYMBOL_LIBS;
class SEARCH_STACK;
class S3D_CACHE;
class KIWAY;
class SYMBOL_LIB_TABLE;
class FILENAME_RESOLVER;
class PROJECT_FILE;
class PROJECT_LOCAL_SETTINGS;
class LOCKFILE;


/**
 * Container for project specific data.
 *
 * Because it is in the neutral program top, which is not linked to by subsidiary DSOs,
 * any functions in this interface must be virtual.
 */
class KICOMMON_API PROJECT
{
public:
    /**
     * The set of #_ELEMs that a #PROJECT can hold.
     */
    enum class ELEM
    {
        FPTBL,

        SCH_SYMBOL_LIBS,
        SCH_SEARCH_STACK,
        S3DCACHE,
        SYMBOL_LIB_TABLE,
        SEARCH_STACK,

        DESIGN_BLOCK_LIB_TABLE,

        SCHEMATIC,
        BOARD,

        COUNT
    };

    /**
     * A #PROJECT can hold stuff it knows nothing about, in the form of _ELEM derivatives.
     *
     * Derive #PROJECT elements from this, it has a virtual destructor, and Elem*() functions
     * can work with it.  Implementation is opaque in class #PROJECT.  If find you have to
     * include derived class headers in this file, you are doing incompatible with the goal
     * of this class.  Keep knowledge of derived classes opaque to class PROJECT please.
    */
    class KICOMMON_API _ELEM
    {
    public:
        virtual ~_ELEM() {}

        virtual PROJECT::ELEM ProjectElementType() = 0; // Sanity-checking for returned values.
    };

    PROJECT();
    virtual ~PROJECT();

    //-----<Cross Module API>----------------------------------------------------

    virtual bool TextVarResolver( wxString* aToken ) const;

    virtual std::map<wxString, wxString>& GetTextVars() const;

    /**
     * Applies the given var map, it will create or update existing vars
     */
    virtual void ApplyTextVars( const std::map<wxString, wxString>& aVarsMap );

    int  GetTextVarsTicker() const { return m_textVarsTicker; }
    void IncrementTextVarsTicker() { m_textVarsTicker++; }

    int  GetNetclassesTicker() const { return m_netclassesTicker; }
    void IncrementNetclassesTicker() { m_netclassesTicker++; }

    /**
     * Return the full path and name of the project.
     *
     * This is the same as the name of the project file (.pro prior to version 6 and .kicad_prj
     * from version 6 onwards) and will always be an absolute path.
     */
    virtual const wxString GetProjectFullName() const;

    /**
     * Return the full path of the project.
     *
     * This is the path of the project file and will always be an absolute path, ending with
     * a path separator.
     */
    virtual const wxString GetProjectPath() const;

    /**
     * Return the full path of the project DIRECTORY
     *
     * This is the path of the project file and will always be an absolute path, ending with
     * a path separator.
     */
    virtual const wxString GetProjectDirectory() const;

    /**
     * Return the short name of the project.
     *
     * This is the file name without extension or path.
     */
    virtual const wxString GetProjectName() const;

    /**
     * Check if this project is a null project (i.e. the default project object created when
     * no real project is open).
     *
     * The null project still presents all the same project interface, but is not backed by
     * any files, so saving it makes no sense.
     *
     * @return true if this is an empty project.
     */
    virtual bool IsNullProject() const;

    virtual bool IsReadOnly() const { return m_readOnly || IsNullProject(); }

    virtual void SetReadOnly( bool aReadOnly = true ) { m_readOnly = aReadOnly; }

    /**
     * Return the name of the sheet identified by the given UUID.
     */
    virtual const wxString GetSheetName( const KIID& aSheetID );

    /**
     * Returns the path and filename of this project's footprint library table.
     *
     * This project specific footprint library table not the global one.
     */
    virtual const wxString FootprintLibTblName() const;

    /**
     * Return the path and file name of this projects symbol library table.
     */
    virtual const wxString SymbolLibTableName() const;

    /**
     * Return the path and file name of this projects design block library table.
     */
    virtual const wxString DesignBlockLibTblName() const;

    enum LIB_TYPE_T
    {
        SYMBOL_LIB,
        FOOTPRINT_LIB,
        DESIGN_BLOCK_LIB,

        LIB_TYPE_COUNT
    };

    void PinLibrary( const wxString& aLibrary, enum LIB_TYPE_T aLibType );
    void UnpinLibrary( const wxString& aLibrary, enum LIB_TYPE_T aLibType );

    virtual PROJECT_FILE& GetProjectFile() const
    {
        wxASSERT( m_projectFile );
        return *m_projectFile;
    }

    virtual PROJECT_LOCAL_SETTINGS& GetLocalSettings() const
    {
        wxASSERT( m_localSettings );
        return *m_localSettings;
    }

    /// Retain a number of project specific wxStrings, enumerated here:
    enum RSTRING_T
    {
        DOC_PATH,
        SCH_LIB_PATH,
        SCH_LIB_SELECT, // eeschema/selpart.cpp
        SCH_LIBEDIT_CUR_LIB,
        SCH_LIBEDIT_CUR_SYMBOL, // eeschema/libeditframe.cpp

        VIEWER_3D_PATH,
        VIEWER_3D_FILTER_INDEX,

        PCB_LIB_PATH,
        PCB_LIB_NICKNAME,
        PCB_FOOTPRINT,
        PCB_FOOTPRINT_EDITOR_FP_NAME,
        PCB_FOOTPRINT_EDITOR_LIB_NICKNAME,
        PCB_FOOTPRINT_VIEWER_FP_NAME,
        PCB_FOOTPRINT_VIEWER_LIB_NICKNAME,

        RSTRING_COUNT
    };

    /**
     * Return a "retained string", which is any session and project specific string
     * identified in enum #RSTRING_T.
     *
     * Retained strings are not written to disk, and are therefore good only for the current
     * session.
     */
    virtual const wxString& GetRString( RSTRING_T aStringId );

    /**
     * Store a "retained string", which is any session and project specific string
     * identified in enum #RSTRING_T.
     *
     * Retained strings are not written to disk, and are therefore good only for the current
     * session.
     */
    virtual void SetRString( RSTRING_T aStringId, const wxString& aString );

    /**
     * Get and set the elements for this project.
     *
     * This is a cross module API, therefore the #_ELEM destructor is virtual and
     * can point to a destructor function in another link image.  Be careful that
     * that program module is resident at time of destruction.
     *
     * Summary:
     *  -#) cross module API.
     *  -#) #PROJECT knows nothing about #_ELEM objects except how to delete them and
     *      set and get pointers to them.
     */
    virtual _ELEM* GetElem( PROJECT::ELEM aIndex );
    virtual void   SetElem( PROJECT::ELEM aIndex, _ELEM* aElem );

    /**
     * Clear the _ELEMs and RSTRINGs.
     */
    void Clear() // inline not virtual
    {
        elemsClear();

        for( unsigned i = 0; i < RSTRING_COUNT; ++i )
            SetRString( RSTRING_T( i ), wxEmptyString );
    }

    /**
     * Fix up @a aFileName if it is relative to the project's directory to be an absolute
     * path and filename.
     *
     * This intends to overcome the now missing chdir() into the project directory.
     */
    virtual const wxString AbsolutePath( const wxString& aFileName ) const;

    /**
     * Return the table of footprint libraries. Requires an active Kiway as this is fetched
     * from Pcbnew.
     */
    virtual FP_LIB_TABLE* PcbFootprintLibs( KIWAY& aKiway );

    /**
     * Return the table of design block libraries.
     */
    virtual DESIGN_BLOCK_LIBRARY_ADAPTER* DesignBlockLibs();

    void SetProjectLock( LOCKFILE* aLockFile );

    LOCKFILE* GetProjectLock() const;

private:
    friend class SETTINGS_MANAGER; // so that SM can set project path
    friend class TEST_NETLISTS_FIXTURE; // TODO(JE) make this not required


    /**
     * Delete all the _ELEMs and set their pointers to NULL.
     */
    virtual void elemsClear();

    /**
     * Set the full directory, basename, and extension of the project.
     *
     * This is the name of the project file with full absolute path and it also defines
     * the name of the project.  The project name and the project file names are exactly
     * the same, providing the project filename is absolute.
     */
    virtual void setProjectFullName( const wxString& aFullPathAndName );

    /**
     * Set the backing store file for this project.
     *
     * This should only be called by #SETTINGS_MANGER on load.
     *
     * @param aFile is a loaded PROJECT_FILE.
     */
    virtual void setProjectFile( PROJECT_FILE* aFile )
    {
        m_projectFile = aFile;
    }

    /**
     * Set the local settings backing store.
     *
     * This should only be called by #SETTINGS_MANAGER on load.
     *
     * @param aSettings is the local settings object (may or may not exist on disk at this point)
     */
    virtual void setLocalSettings( PROJECT_LOCAL_SETTINGS* aSettings )
    {
        m_localSettings = aSettings;
    }

    /**
     * Return the full path and file name of the project specific library table \a aLibTableName..
     */
    const wxString libTableName( const wxString& aLibTableName ) const;

private:
    wxFileName      m_project_name;         ///< \<fullpath\>/\<basename\>.pro

    bool            m_readOnly;             ///< No project files will be written to disk
    int             m_textVarsTicker;       ///< Update counter on text vars
    int             m_netclassesTicker;     ///< Update counter on netclasses

    /// Backing store for project data -- owned by SETTINGS_MANAGER
    PROJECT_FILE*            m_projectFile;

    /// Backing store for project local settings -- owned by SETTINGS_MANAGER
    PROJECT_LOCAL_SETTINGS*  m_localSettings;

    std::map<KIID, wxString> m_sheetNames;

    /// @see this::SetRString(), GetRString(), and enum RSTRING_T.
    std::array<wxString,RSTRING_COUNT> m_rstrings;

    /// @see this::Elem() and enum ELEM_T.
    std::array<_ELEM*,static_cast<unsigned int>( PROJECT::ELEM::COUNT )> m_elems;

    /// Lock
    std::unique_ptr<LOCKFILE> m_project_lock;
};


#endif  // PROJECT_H_
