/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017  Cirilo Bernardo
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

/*
 * NOTE:
 *
 *  Rules to ensure friendly use within a DLL:
 *
 *  1. all functions which throw exceptions must not be publicly available;
 *  they must become FRIEND functions instead.
 *
 *  2. All objects with PRIVATE functions which throw exceptions when
 *  invoked by a PUBLIC function must indicate success or failure
 *  and make the exception information available via a GetError()
 *  routine.
 *
 *  General notes:
 *
 *  1. Due to the complexity of objects and the risk of accumulated
 *  position errors, CAD packages should only delete or add complete
 *  components. If a component being added already exists, it is
 *  replaced by the new component IF and only if the CAD type is
 *  permitted to make such changes.
 *
 *  2. Internally all units shall be in mm and by default we shall
 *  write files with mm units. The internal flags mm/thou shall only
 *  be used to translate data being read from or written to files.
 *  This avoids the painful management of a mixture of mm and thou.
 *  The API shall require all dimensions in mm; for people using any
 *  other unit, it is their responsibility to perform the conversion
 *  to mm. Conversion back to thou may incur small rounding errors.
 */


#ifndef IDF_PARSER_H
#define IDF_PARSER_H

#include <idf_outlines.h>

class IDF3_COMPONENT;

class IDF3_COMP_OUTLINE_DATA
{
public:
    /**
     * Create an object with default settings and no parent or associated outline.
     */
    IDF3_COMP_OUTLINE_DATA();

    /**
     * Create an object with default settings and the specified parent and associated outline.
     *
     * @param aParent is the owning IDF3_COMPONENT object.
     * @param aOutline is the outline for this placed component.
     */
    IDF3_COMP_OUTLINE_DATA( IDF3_COMPONENT* aParent, IDF3_COMP_OUTLINE* aOutline );

    /**
     * Create an object the specified parent and associated outline and the specified data.
     *
     * @param aParent is the owning IDF3_COMPONENT object.
     * @param aOutline is the outline for this placed component.
     * @param aXoff is the X offset of this outline in relation to its parent.
     * @param aYoff is the Y offset of this outline in relation to its parent.
     * @param aZoff is the board offset of this outline as per IDFv3 specification.
     * @param aAoff is the rotational offset of this outline in relation to its parent.
     */
    IDF3_COMP_OUTLINE_DATA( IDF3_COMPONENT* aParent, IDF3_COMP_OUTLINE* aOutline,
                            double aXoff, double aYoff, double aZoff, double aAngleOff );

    ~IDF3_COMP_OUTLINE_DATA();

    /**
     * Set the position and orientation of this outline item in relation to its parent.
     *
     * @param aXoff is the X offset of this outline in relation to its parent.
     * @param aYoff is the Y offset of this outline in relation to its parent.
     * @param aZoff is the board offset of this outline as per IDFv3 specification.
     * @param aAoff is the rotational offset of this outline in relation to its parent.
     *
     * @return true if the operation succeeded, false if an ownership violation occurred.
     */
    bool SetOffsets( double aXoff, double aYoff, double aZoff, double aAngleOff );

    /**
     * Retrieve the position and orientation of this outline item in relation to its parent.
     *
     * @param aXoff is the X offset of this outline in relation to its parent.
     * @param aYoff is the Y offset of this outline in relation to its parent.
     * @param aZoff is the board offset of this outline as per IDFv3 specification.
     * @param aAoff is the rotational offset of this outline in relation to its parent.
     */
    void GetOffsets( double& aXoff, double& aYoff, double& aZoff, double& aAngleOff );

    /**
     * Set the parent object.
     *
     * @param aParent is the owning IDF3_COMPONENT object.
     */
    void SetParent( IDF3_COMPONENT* aParent );

    /**
     * Set the outline whose position is managed by this object.
     *
     * @param aOutline is the outline for this component.
     * @return true if the operation succeeded, false if an ownership violation occurred.
     */
    bool SetOutline( IDF3_COMP_OUTLINE* aOutline );

    /**
     * Retrieve the outline whose position is managed by this object.
     *
     * @return the outline for this component.
     */
    IDF3_COMP_OUTLINE* GetOutline( void )
    {
        return outline;
    }

    const std::string& GetError( void )
    {
        return errormsg;
    }

private:

#ifndef DISABLE_IDF_OWNERSHIP
    bool checkOwnership( int aSourceLine, const char* aSourceFunc );
#endif

    /**
     * Read placement data from an open IDFv3 file.
     *
     * @param aBoardFile is the open IDFv3 file
     * @param aBoardState is the internal status flag of the IDF parser
     * @param aIdfVersion is the version of the file currently being parsed
     * @param aBoard is the IDF3_BOARD object which will store the data
     *
     * @return true if placement data was successfully read. false if
     * no placement data was read; this may happen if the end of the placement
     * data was encountered or an error occurred. if an error occurred then
     * an exception is thrown.
     */
    bool readPlaceData( std::istream &aBoardFile, IDF3::FILE_STATE& aBoardState,
                        IDF3_BOARD *aBoard, IDF3::IDF_VERSION aIdfVersion,
                        bool aNoSubstituteOutlines );

    /**
     * Write RECORD 2 and RECORD 3 of a PLACEMENT section as per IDFv3 specification.
     *
     * @param aBoardFile is the open IDFv3 file.
     * @param aXpos is the X location of the parent component.
     * @param aYpos is the Y location of the parent component.
     * @param aAngle is the rotation of the parent component.
     * @param aRefDes is the reference designator of the parent component.
     * @param aPlacement is the IDF Placement Status of the parent component.
     * @param aSide is the IDF Layer Designator (TOP or BOTTOM).
     * @return true if data was successfully written, otherwise false.
     */
    void writePlaceData( std::ostream& aBoardFile, double aXpos, double aYpos, double aAngle,
                         const std::string& aRefDes, IDF3::IDF_PLACEMENT aPlacement,
                         IDF3::IDF_LAYER aSide );

    friend class IDF3_BOARD;
    friend class IDF3_COMPONENT;

    double xoff;    // X offset from KiCad or X placement from IDF file
    double yoff;    // Y offset from KiCad or Y placement from IDF file
    double zoff;    // height offset (specified in IDFv3 spec, corresponds to KiCad Z offset)
    double aoff;    // angular offset from KiCad or Rotation Angle from IDF file
    std::string errormsg;

    IDF3_COMP_OUTLINE* outline; // component outline to use
    IDF3_COMPONENT* parent;     // associated component
};


class IDF3_COMPONENT
{
public:
    /**
     * Set the parent object and initializes other internal parameters to default values.
     *
     * @param aParent is the owning IDF3_BOARD object.
     */
    IDF3_COMPONENT( IDF3_BOARD* aParent );

    ~IDF3_COMPONENT();

    /**
     * Set the parent object.
     *
     * @param aParent is the owning IDF3_BOARD object.
     */
    void SetParent( IDF3_BOARD* aParent );

    /**
     * @return the type of CAD (IDF3::CAD_ELEC, IDF3::CAD_MECH) which instantiated this object.
     */
    IDF3::CAD_TYPE GetCadType( void );

    /**
     * @return the IDF UNIT type of the parent object or IDF3::UNIT_INVALID if the parent was
     *         not set.
     */
    IDF3::IDF_UNIT GetUnit( void );

    /**
     * Set the Reference Designator (RefDes) of this component.
     *
     * The RefDes is shared by all outlines associated with this component.
     *
     * @return true if the RefDes was accepted, otherwise false. Prohibited values include empty
     *         strings and the word PANEL.
     */
    bool SetRefDes( const std::string& aRefDes );

    /**
     * Retrieve the Reference Designator (RefDes) of this component.
     *
     * @return the Reference Designator.
     */
    const std::string& GetRefDes( void );

    /**
     * Add a drill entry to the component and returns its pointer.
     *
     * @param aDia diameter of the drill (mm).
     * @param aXpos X position of the drill (mm).
     * @param aYpos Y position of the drill (mm).
     * @param aPlating plating type (PTH, NPTH).
     * @param aHoleType hole class (PIN, VIA, MTG, TOOL, etc).
     * @param aOwner owning CAD system (ECAD, MCAD, UNOWNED).
     * @return the newly created drill entry or NULL.
     */
    IDF_DRILL_DATA* AddDrill( double aDia, double aXpos, double aYpos,
                              IDF3::KEY_PLATING aPlating,
                              const std::string& aHoleType,
                              IDF3::KEY_OWNER aOwner );

    /**
     * Add the given drill entry to the component and returns the pointer to indicate success.
     *
     * A return value of NULL indicates that the item was not added and it is the user's
     * responsibility to delete the object if necessary.
     *
     * @param aDrilledHole pointer to a drill entry.
     * @return aDrilledHole if the function succeeds, otherwise NULL.
     */
    IDF_DRILL_DATA* AddDrill( IDF_DRILL_DATA* aDrilledHole );

    /**
     * Delete a drill entry based on its size and location.
     *
     * This operation is subject to IDF ownership rules.
     *
     * @param aDia diameter (mm) of the drilled hole to be deleted.
     * @param aXpos X position (mm) of the hole to be deleted.
     * @param aYpos X position (mm) of the hole to be deleted.
     * @return true if a drill was found and deleted, otherwise false.
     * @throw if an ownership violation occurs.
     */
    bool DelDrill( double aDia, double aXpos, double aYpos );

    /**
     * Delete a drill entry based on pointer.
     *
     * This operation is subject to IDF ownership rules.
     *
     * @param aDrill the pointer associated with the drill entry to be deleted.
     * @return true if a drill was found and deleted, otherwise false.
     * @throw if an ownership violation occurs.
     */
    bool DelDrill( IDF_DRILL_DATA* aDrill );

    /**
     * Return the internal list of drills.
     *
     * To avoid IDF violations, the user should not alter these entries.
     */
    const std::list<IDF_DRILL_DATA*>* GetDrills( void );

    /**
     * Add the given component outline data to this component.
     *
     * @param aComponentOutline is a pointer to the outline data to be added.
     * @return true if the operation succeeds, otherwise false.
     */
    bool AddOutlineData( IDF3_COMP_OUTLINE_DATA* aComponentOutline );

    /**
     * Remove outline data based on the pointer provided.
     *
     * @param aComponentOutline is a pointer to be deleted from the internal list.
     * @return true if the data was found and deleted, otherwise false.
     */
    bool DeleteOutlineData( IDF3_COMP_OUTLINE_DATA* aComponentOutline );

    /**
     * Remove outline data based on the provided index.
     *
     * @param aIndex is an index to the internal outline list.
     * @return true if the data was deleted, false if the index was out of bounds.
     */
    bool DeleteOutlineData( size_t aIndex );

    /**
     * @return the number of outlines in the internal list.
     */
    size_t GetOutlinesSize( void );


    /**
     * @return the internal list of outline data.
     */
    const std::list< IDF3_COMP_OUTLINE_DATA* >* GetOutlinesData( void );

    /**
     * Retrieve the internal position parameters and returns true if the
     * position was previously set, otherwise false.
     */
    bool GetPosition( double& aXpos, double& aYpos, double& aAngle, IDF3::IDF_LAYER& aLayer );

    // NOTE: it may be possible to extend this so that internal drills and outlines
    // are moved when the component is moved. However there is always a danger of
    // position creep due to the relative position updates.
    /**
     * Set the internal position parameters and returns true if the position was set, false if
     * the position was previously set.
     *
     * This object does not allow modification of the position once it is set since this may
     * adversely affect the relationship with its internal objects.
     *
     * @param aXpos is the X position (mm) of the component.
     * @param aYpos is the Y position (mm) of the component.
     * @param aAngle is the rotation of the component (degrees).
     * @param aLayer is the layer on which the component is places (TOP, BOTTOM).
     * @return true if the position was set, otherwise false.
     */
    bool SetPosition( double aXpos, double aYpos, double aAngle, IDF3::IDF_LAYER aLayer );

    /**
     * @return the IDF placement value of this component (UNPLACED, PLACED, ECAD, MCAD).
     */
    IDF3::IDF_PLACEMENT GetPlacement( void );

    /**
     * Set the placement value of the component subject to ownership rules.
     *
     * An exception is thrown if aPlacementValue is invalid or an ownership
     * violation occurs.
     *
     * @return true if the operation succeeded, otherwise false and the error message is set.
     */
    bool SetPlacement( IDF3::IDF_PLACEMENT aPlacementValue );

    const std::string& GetError( void )
    {
        return errormsg;
    }

private:
    friend class IDF3_BOARD;

    /**
     * Write the internal drill data to an IDFv3 .DRILLED_HOLES section.
     *
     * @param aBoardFile is an IDFv3 file opened for writing.
     * @return true if the operation succeeded, otherwise false.
     */
    bool writeDrillData( std::ostream& aBoardFile );

    /**
     * Write the component placement data to an IDFv3 .PLACEMENT section.
     *
     * @param aBoardFile is an IDFv3 file opened for writing.
     * @return true if the operation succeeded, otherwise false.
     */
    bool writePlaceData( std::ostream& aBoardFile );

#ifndef DISABLE_IDF_OWNERSHIP
    bool checkOwnership( int aSourceLine, const char* aSourceFunc );
#endif

    std::list< IDF3_COMP_OUTLINE_DATA* > components;
    std::list< IDF_DRILL_DATA* > drills;

    double xpos;
    double ypos;
    double angle;
    IDF3::IDF_PLACEMENT placement;
    IDF3::IDF_LAYER     layer;          // [TOP/BOTTOM ONLY as per IDF spec]
    bool                hasPosition;    ///< True after SetPosition is called once
    std::string         refdes;         ///< Reference Description (MUST BE UNIQUE)
    IDF3_BOARD*         parent;
    std::string         errormsg;
};


class IDF3_BOARD
{
public:
    IDF3_BOARD( IDF3::CAD_TYPE aCadType );
    virtual ~IDF3_BOARD();

    IDF3::CAD_TYPE GetCadType( void ) { return m_cadType; }

    // retrieve the nominal unit used in reading/writing
    // data. This is primarily for use by owned objects
    // and is only of informational use for the end user.
    // Internally all data is represented in mm and the
    // end user must use only mm in the API.
    IDF3::IDF_UNIT GetUnit( void ) const { return m_unit; }

    const std::string& GetNewRefDes( void );

    void SetBoardName( const std::string& aBoardName ) { m_boardName = aBoardName; }
    const std::string& GetBoardName( void ) const { return m_boardName; }

    bool SetBoardThickness( double aBoardThickness );
    double GetBoardThickness( void );

    bool ReadFile( const wxString& aFullFileName, bool aNoSubstituteOutlines = false );
    bool WriteFile( const wxString& aFullFileName, bool aUnitMM = true,
                    bool aForceUnitFlag = false );

    const std::string& GetIDFSource( void ) const { return m_idfSource; }
    void  SetIDFSource( const std::string& aIDFSource) { m_idfSource = aIDFSource; }

    const std::string& GetBoardSource( void ) const { return m_brdSource; }
    const std::string& GetLibrarySource( void ) const { return m_libSource; }
    const std::string& GetBoardDate( void ) const { return m_brdDate; }
    const std::string& GetLibraryDate( void ) const { return m_libDate; }

    int   GetBoardVersion( void ) const { return m_brdFileVersion; }
    bool  SetBoardVersion( int aVersion );
    int   GetLibraryVersion( void ) const { return m_libFileVersion; }
    bool  SetLibraryVersion( int aVersion );

    double GetUserScale( void ) const { return m_userScale; }
    bool SetUserScale( double aScaleFactor );

    int GetUserPrecision( void ) const { return m_userPrec; }
    bool SetUserPrecision( int aPrecision );

    void GetUserOffset( double& aXoff, double& aYoff );
    void SetUserOffset( double aXoff, double aYoff );

    bool AddBoardOutline( IDF_OUTLINE* aOutline );
    bool DelBoardOutline( IDF_OUTLINE* aOutline );
    bool DelBoardOutline( size_t aIndex );
    size_t GetBoardOutlinesSize( void );
    BOARD_OUTLINE* GetBoardOutline( void ) { return &m_boardOutline; }

    // Operations for OTHER OUTLINES
    const std::map<std::string, OTHER_OUTLINE*>* GetOtherOutlines( void ) const { return &m_otherOutlines; }

    /// XXX - TO BE IMPLEMENTED
    //
    // SetBoardOutlineOwner()
    //
    // AddDrillComment
    // AddPlacementComment
    // GetDrillComments()
    // GetPlacementComments()
    // ClearDrillComments()
    // ClearPlacementComments()
    // AddNoteComment
    // GetNoteComments
    // AddNote
    //
    // [IMPLEMENTED] const std::map<std::string, OTHER_OUTLINE*>*const GetOtherOutlines( void )
    // size_t GetOtherOutlinesSize()
    // OTHER_OUTLINE* AddOtherOutline( OTHER_OUTLINE* aOtherOutline )
    // bool DelOtherOutline( int aIndex )
    // bool DelOtherOutline( OTHER_OUTLINE* aOtherOutline )
    //
    // const std::list<ROUTE_OUTLINE*>*const GetRouteOutlines()
    // size_t GetRouteOutlinesSize()
    // ROUTE_OUTLINE* AddRouteOutline( ROUTE_OUTLINE* aRouteOutline )
    // bool DelRouteOutline( int aIndex )
    // bool DelRouteOutline( ROUTE_OUTLINE* aRouteOutline )
    //
    // const std::list<PLACE_OUTLINE*>*const GetPlacementOutlines()
    // size_t GetPlacementOutlinesSize()
    // PLACE_OUTLINE* AddPlacementOutline( PLACE_OUTLINE* aPlaceOutline )
    // bool DelPlacementOutline( int aIndex )
    // bool DelPlacementOutline( PLACE_OUTLINE* aPlaceOutline )
    //
    // const std::list<ROUTE_KO_OUTLINE*>*const GetRouteKeepOutOutlines()
    // size_t GetRouteKeepOutOutlinesSize()
    // ROUTE_KO_OUTLINE* AddRouteKeepoutOutline( ROUTE_KO_OUTLINE* aRouteKeepOut )
    // bool DelRouteKeepOutOutline( int aIndex )
    // bool DelRouteKeepOutOutline( ROUTE_KO_OUTLINE* aRouteKeepOut )
    //
    // const std::list<VIA_KO_OUTLINE*>*const GetViaKeepOutOutlines()
    // size_t GetViaKeepOutOutlinesSize()
    // VIA_KO_OUTLINE* AddViaKeepoutOutline( VIA_KO_OUTLINE* aViaKeepOut )
    // bool DelViaKeepOutOutline( int aIndex )
    // bool DelViaKeepOutOutline( VIA_KO_OUTLINE* aViaKeepOut )
    //
    // const std::list<PLACE_KO_OUTLINE*>*const GetPlacementKeepOutOutlines()
    // size_t GetPlacementKeepOutOutlinesSize()
    // PLACE_KO_OUTLINE* AddPlacementKeepoutOutline( PLACE_KO_OUTLINE* aPlaceKeepOut )
    // bool DelPlacementKeepOutOutline( int aIndex )
    // bool DelPlacementKeepOutOutline( PLACE_KO_OUTLINE* aPlaceKeepOut )
    //
    // const std::multimap<std::string, GROUP_OUTLINE*>*const GetGroupOutlines()
    // size_t GetGroupOutlinesSize()
    // GROUP_OUTLINE* AddGroupOutline( GROUP_OUTLINE* aGroupOutline )
    // bool DelGroupOutline( int aIndex )
    // bool DelGroupOutline( GROUP_OUTLINE* aGroupOutline )

    std::list<IDF_DRILL_DATA*>& GetBoardDrills( void )
    {
        return m_drills;
    }

    IDF_DRILL_DATA* AddBoardDrill( double aDia, double aXpos, double aYpos,
                                   IDF3::KEY_PLATING aPlating,
                                   const std::string& aHoleType,
                                   IDF3::KEY_OWNER aOwner );

    IDF_DRILL_DATA* AddDrill( IDF_DRILL_DATA* aDrilledHole );

    bool DelBoardDrill( double aDia, double aXpos, double aYpos );

    // a slot is a deficient representation of a kicad slotted hole;
    // it is usually associated with a component but IDFv3 does not
    // provide for such an association.
    bool AddSlot( double aWidth, double aLength, double aOrientation, double aX, double aY );

    bool AddComponent( IDF3_COMPONENT* aComponent );
    bool DelComponent( IDF3_COMPONENT* aComponent );
    bool DelComponent( size_t aIndex );
    size_t GetComponentsSize( void );
    std::map< std::string, IDF3_COMPONENT* >* GetComponents( void );
    IDF3_COMPONENT* FindComponent( const std::string& aRefDes );

    // Return a pointer to a component outline object or NULL if the object doesn't exist.
    IDF3_COMP_OUTLINE* GetComponentOutline( const wxString& aFullFileName );

    // returns a pointer to the component outline object with the
    // unique ID aComponentID
    IDF3_COMP_OUTLINE* GetComponentOutline( const std::string& aComponentID );

    // returns a pointer to the outline "NOGEOM NOPART" which is substituted
    // whenever a true outline cannot be found or is defective
    IDF3_COMP_OUTLINE* GetInvalidOutline( const std::string& aGeomName,
                                          const std::string& aPartName );

    // clears all data
    void Clear( void );

    // return error string
    const std::string& GetError( void )
    {
        return m_errormsg;
    }

private:
    // Set the unit; this can only be done internally upon reading a file or saving.
    bool setUnit( IDF3::IDF_UNIT aUnit, bool convert = false );

    IDF_DRILL_DATA* addCompDrill( double aDia, double aXpos, double aYpos,
                                  IDF3::KEY_PLATING aPlating,
                                  const std::string& aHoleType,
                                  IDF3::KEY_OWNER aOwner,
                                  const std::string& aRefDes );

    IDF_DRILL_DATA* addCompDrill( IDF_DRILL_DATA* aDrilledHole );

    bool delCompDrill( double aDia, double aXpos, double aYpos, const std::string& aRefDes );

    // read the DRILLED HOLES section
    void readBrdDrills( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState );

    // read the NOTES section
    void readBrdNotes( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState );

    // read the component placement section
    void readBrdPlacement( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState,
                           bool aNoSubstituteOutlines );

    // read the board HEADER
    void readBrdHeader( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState );

    // read individual board sections; pay attention to IDFv3 section specifications
    // exception thrown on unrecoverable errors. state flag set to FILE_PLACEMENT
    // upon reading the PLACEMENT file; according to IDFv3 this is the final section
    void readBrdSection( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState,
                         bool aNoSubstituteOutlines );
    // read the board file data
    void readBoardFile( const std::string& aFileName, bool aNoSubstituteOutlines );

    // write the board file data
    void writeBoardFile( const std::string& aFileName );

    // read the library sections (outlines)
    void readLibSection( std::istream& aLibFile, IDF3::FILE_STATE& aLibState, IDF3_BOARD* aBoard );

    // read the library HEADER
    void readLibHeader( std::istream& aLibFile, IDF3::FILE_STATE& aLibState );

    // read the library file data
    void readLibFile( const std::string& aFileName );

    // write the library file data
    bool writeLibFile( const std::string& aFileName );

#ifndef DISABLE_IDF_OWNERSHIP
    bool checkComponentOwnership( int aSourceLine, const char* aSourceFunc, IDF3_COMPONENT* aComponent );
#endif

    std::map<std::string, std::string>         m_uidFileList;        // map of files opened and UIDs
    std::list<std::string>                     m_uidLibList;         // list of UIDs read from a library file

    // string for passing error messages to user
    std::string                                m_errormsg;
    std::list< IDF_NOTE*>                      m_notes;              // IDF notes
    std::list<std::string>                     m_noteComments;       // comment list for NOTES section
    std::list<std::string>                     m_drillComments;      // comment list for DRILL section
    std::list<std::string>                     m_placeComments;      // comment list for PLACEMENT section
    std::list<IDF_DRILL_DATA*>                 m_drills;
    std::map<std::string, IDF3_COMPONENT*>     m_components;         // drill and placement data for components
    std::map<std::string, IDF3_COMP_OUTLINE*>  m_componentOutlines;  // component outlines (data for library file).

    std::string          m_boardName;
    IDF3::CAD_TYPE       m_cadType;
    IDF3::IDF_UNIT       m_unit;
    IDF3::IDF_VERSION    m_idfVer;             // IDF version of Board or Library

    int                  m_refDesCounter;      // counter for automatically numbered NOREFDES items
    std::string          m_refDesString;

    std::string          m_idfSource;          // SOURCE string to use when writing BOARD and LIBRARY headers
    std::string          m_brdSource;          // SOURCE string as retrieved from a BOARD file
    std::string          m_libSource;          // SOURCE string as retrieved from a LIBRARY file
    std::string          m_brdDate;            // DATE string from BOARD file
    std::string          m_libDate;            // DATE string from LIBRARY file
    int                  m_brdFileVersion;     // File Version from BOARD file
    int                  m_libFileVersion;     // File Version from LIBRARY file

    int                  m_userPrec;           // user may store any integer here
    double               m_userScale;          // user may store a scale for translating to arbitrary units
    double               m_userXoff;           // user may specify an arbitrary X/Y offset
    double               m_userYoff;

    BOARD_OUTLINE                              m_boardOutline;          // main board outline and cutouts
    std::map<std::string, OTHER_OUTLINE*>      m_otherOutlines;         // OTHER outlines
    std::list<ROUTE_OUTLINE*>                  m_routeOutlines;         // ROUTE outlines
    std::list<PLACE_OUTLINE*>                  m_placeOutlines;         // PLACEMENT outlines
    std::multimap<std::string, GROUP_OUTLINE*> m_groupOutlines;         // PLACEMENT GROUP outlines

    std::list<ROUTE_KO_OUTLINE*>               m_routeKeepoutOutlines;  // ROUTE KEEPOUT outlines
    std::list<VIA_KO_OUTLINE*>                 m_viaKeepoutOutlines;    // VIA KEEPOUT outlines
    std::list<PLACE_KO_OUTLINE*>               m_placeKeepoutOutlines;  // PLACE KEEPOUT outlines
};

#endif // IDF_PARSER_H
