#ifndef PROJECT_H_
#define PROJECT_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <vector>
#include <wx/string.h>
#include <wx/filename.h>

/// A variable name whose value holds the current project directory.
/// Currently an environment variable, eventually a project variable.
#define PROJECT_VAR_NAME            wxT( "KIPRJMOD" )


class wxConfigBase;
class PARAM_CFG_ARRAY;
class FP_LIB_TABLE;
class PART_LIBS;
class SEARCH_STACK;

#define VTBL_ENTRY      virtual

/**
 * Class PROJECT
 * holds project specific data.  Because it is in the neutral program top, which
 * is not linked to by subsidiarly DSOs, any functions in this interface must
 * be VTBL_ENTRYs.
 */
class PROJECT
{
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
    };

    PROJECT();
    ~PROJECT();

    //-----<Cross Module API>----------------------------------------------------

    // VTBL_ENTRY bool MaybeLoadProjectSettings( const std::vector<wxString>& aFileSet );

    /**
     * Function SetProjectFullName
     * sets the:
     * 1) full directory, 2) basename, and 3) extension of the project.  This is
     * the name of the *.pro file with full absolute path and it also defines
     * the name of the project.  The project name and the *.pro file names are
     * exactly the same, providing the *.pro filename is absolute.
     */
    VTBL_ENTRY void SetProjectFullName( const wxString& aFullPathAndName );

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
     * Function FootprintLibTblName
     * returns the path and filename of this project's fp-lib-table,
     * i.e. the project specific one, not the global one.
     */
    VTBL_ENTRY const wxString FootprintLibTblName() const;

    /**
     * Function ConfigSave
     * saves the current "project" parameters into the wxConfigBase* derivative.
     * Then the wxConfigBase derivative is written to the *.pro file for the project.
     *
     * @param aSList a SEARCH_STACK
     * @param aGroupName is the name of the group inside the config which contains parameters
     * @param aParams is a ptr vector of PARAM_CFG_BASE derivatives.
     *  Saved parameters are the subset in this array having the .m_Setup member
     *  set to false.
     * @param aFileName is where to save the *.pro file and if NULL means use this PROJECT's
     *   @a m_project_name.
     */
    VTBL_ENTRY void ConfigSave( const SEARCH_STACK& aSList, const wxString& aGroupName,
        const PARAM_CFG_ARRAY& aParams, const wxString& aFileName = wxEmptyString );

    /**
     * Function ConfigLoad
     * reads a subset of parameters from the "project" file.  Parameters are the
     * subset of variables given in @a aParams array which have the .m_Setup member
     * set to false.  The file which is read in and then extracted from is the
     * '*.pro' file for the project.
     * <p>
     * set:
     *  m_pro_date_and_time
     *
     * @param aSearchS a SEARCH_STACK where a kicad.pro template file may be found.
     * @param aGroupName
     * @param aParams is ptr vector of PARAM_CFG_BASE derivatives.
     * @param aForeignConfigFileName when NULL means load the *.pro filename given
     *  in this PROJECT's @a m_project_name field, otherwise load the provided filename.
     *
     * @return bool - true if loaded OK.
     */
    VTBL_ENTRY bool ConfigLoad( const SEARCH_STACK& aSearchS, const wxString& aGroupName,
            const PARAM_CFG_ARRAY& aParams, const wxString& aForeignConfigFileName = wxEmptyString );

    /// Retain a number of project specific wxStrings, enumerated here:
    enum RSTRING_T
    {
        DOC_PATH,
        SCH_LIB_PATH,
        SCH_LIB_SELECT,         // eeschema/selpart.cpp
        SCH_LIBEDIT_CUR_LIB,
        SCH_LIBEDIT_CUR_PART,        // eeschema/libeditframe.cpp

        VIEWER_3D_PATH,

        PCB_LIB_NICKNAME,
        PCB_FOOTPRINT,
        PCB_FOOTPRINT_VIEWER_FPNAME,
        PCB_FOOTPRINT_VIEWER_NICKNAME,

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
    // These are all prefaced with "Pcb"
    FP_LIB_TABLE* PcbFootprintLibs();
#endif


#if defined(EESCHEMA)
    // These are all prefaced with "Sch"
    PART_LIBS*  SchLibs();

    /// Accessor for Eeschema search stack.
    SEARCH_STACK*  SchSearchS();
#endif

    //-----</KIFACE Specific APIs>-----------------------------------------------

private:

    /**
     * Function configCreate
     * loads a *.pro file and returns a wxConfigBase.
     *
     * @param aSList is the KIFACE or PGM's SEARCH_STACK
     * @param aGroupName is the default config file subset to use.
     * @param aProjectFileName is the *.pro file to open.
     */
    wxConfigBase* configCreate( const SEARCH_STACK& aSList,
            const wxString& aGroupName, const wxString& aProjectFileName = wxEmptyString );

    wxFileName      m_project_name;         ///< \<fullpath\>/\<basename\>.pro
    wxString        m_pro_date_and_time;

    /// @see this::SetRString(), GetRString(), and enum RSTRING_T.
    wxString        m_rstrings[RSTRING_COUNT];

    /// @see this::Elem() and enum ELEM_T.
    _ELEM*          m_elems[ELEM_COUNT];
};


//-----<possible futures>---------------------------------------------------------

#if 0
    /**
     * Function Value
     * fetches a project variable @a aVariable and returns true if that variable was
     * found, else false. If not found, aFetchedValue is not touched.  Any environment
     * variable is also a project variable.
     *
     * @param aVariable is the property or option to look for.
     * @param aFetchedValue is where to put the value of the property if it exists
     *  and aFetchedValue is not NULL.
     * @return bool - true if variable was found, else false.
     */
    VTBL_ENTRY bool Value( const wxString& aVariable, wxString* aFetchedValue = NULL );

    /**
     * Function Substitute
     * replaces any project variable references found within @a aString with their
     * values.  Any referenced variable is first sought in the PROJECT space, and if
     * not found, then sought in the environment.
     */
    VTBL_ENTRY const wxString Substitute( const wxString& aString );

    /**
     * Function SubstituteAndEvaluate
     * replaces any project variable references found within @a aString with their
     * values, and evaluates aString as an expression.
     * Any referenced variable is first sought in the PROJECT space, and if
     * not found, then sought in the environment.
     */
    VTBL_ENTRY const wxString SubstituteAndEvaluate( const wxString& aString );
#endif

#endif  // PROJECT_H_
