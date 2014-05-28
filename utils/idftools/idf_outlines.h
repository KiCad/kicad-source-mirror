/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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


#ifndef IDF_OUTLINES_H
#define IDF_OUTLINES_H

#include <string>
#include <list>
#include <map>
#include <wx/string.h>
#include <wx/filename.h>

#include <idf_common.h>

/*
 *  NOTES ON OUTLINE TYPES:
 *
 *  BOARD_OUTLINE (PANEL_OUTLINE)
 *      .BOARD_OUTLINE  [OWNER]
 *      [thickness]
 *      [outlines]
 *
 *  OTHER_OUTLINE
 *      .OTHER_OUTLINE  [OWNER]
 *      [outline identifier] [thickness] [board side: Top/Bot]
 *      [outline]
 *
 *  ROUTE_OUTLINE
 *      .ROUTE_OUTLINE [OWNER]
 *      [layers]
 *      [outline]
 *
 *  PLACE_OUTLINE
 *      .PLACE_OUTLINE [OWNER]
 *      [board side: Top/Bot/Both] [height]
 *      [outline]
 *
 *  ROUTE_KEEPOUT
 *      .ROUTE_KEEPOUT [OWNER]
 *      [layers]
 *      [outline]
 *
 *  VIA_KEEPOUT
 *      .VIA_KEEPOUT [OWNER]
 *      [outline]
 *
 *  PLACE_KEEPOUT
 *      .PLACE_KEEPOUT [OWNER]
 *      [board side: Top/Bot/Both] [height]
 *      [outline]
 *
 *  Placement Group
 *      .PLACE_REGION [OWNER]
 *      [side: Top/Bot/Both ] [component group name]
 *      [outline]
 *
 *  Component Outline:
 *      .ELECTRICAL/.MECHANICAL
 *      [GEOM] [PART] [UNIT] [HEIGHT]
 *      [outline]
 *      [PROP] [prop name] [prop value]
 */

class IDF3_BOARD;


/**
 * Class BOARD_OUTLINE
 * supports the IDFv3 BOARD OUTLINE data and is the basis of other IDFv3 outline classes
 */
class BOARD_OUTLINE
{
protected:
    std::list< IDF_OUTLINE* >   outlines;
    IDF3::KEY_OWNER             owner;      // indicates the owner of this outline (MCAD, ECAD, UNOWNED)
    IDF3::OUTLINE_TYPE          outlineType;// type of IDF outline
    bool                        single;     // true if only a single outline is accepted
    std::list< std::string >    comments;   // associated comment list
    IDF3::IDF_UNIT              unit;       // outline's native unit (MM or THOU)
    IDF3_BOARD*                 parent;     // BOARD which contains this outline
    double                      thickness;  // Board/Extrude Thickness or Height (IDF spec)

    // Read outline data from a BOARD or LIBRARY file's outline section
    bool readOutlines( std::ifstream& aBoardFile );
    // Write comments to a BOARD or LIBRARY file (must not be within a SECTION as per IDFv3 spec)
    bool writeComments( std::ofstream& aBoardFile );
    // Write the outline owner to a BOARD file
    bool writeOwner( std::ofstream& aBoardFile );
    // Write the data of a single outline object
    bool writeOutline( std::ofstream& aBoardFile, IDF_OUTLINE* aOutline, size_t aIndex );
    // Iterate through the outlines and write out all data
    bool writeOutlines( std::ofstream& aBoardFile );  // write outline data (no headers)

public:
    BOARD_OUTLINE();
    virtual ~BOARD_OUTLINE();

    /**
     * Function SetUnit
     * sets the native unit of the outline; except for component outlines this must
     * be the same as the native unit of the parent IDF_BOARD object
     *
     * @param aUnit is the native unit (UNIT_MM or UNIT_THOU)
     */
    virtual void SetUnit( IDF3::IDF_UNIT aUnit );

    /**
     * Function GetUnit
     * returns the native unit type of the outline
     *
     * @return IDF_UNIT is the native unit (UNIT_MM or UNIT_THOU)
     */
    virtual IDF3::IDF_UNIT GetUnit( void );

    /**
     * Function SetThickness
     * sets the thickness or height of the outline (mm)
     *
     * @param aThickness is the thickness or height of the outline in mm
     */
    virtual bool SetThickness( double aThickness );

    /**
     * Function GetThickness
     * returns the thickness or height of an outline (mm)
     */
    virtual double GetThickness( void );

    /**
     * Function ReadData
     * reads data from a .BOARD_OUTLINE section
     *
     * @param aBoardFile is an IDFv3 file opened for reading
     * @param aHeader is the ".BOARD_OUTLINE" header line as read by FetchIDFLine
     *
     * @return bool: true if the BOARD_OUTLINE section was successfully read, otherwise
     * false. In case of an unrecoverable error an exception is thrown. On a successful
     * return the file pointer will be at the line following .END_BOARD_OUTLINE
     */
    virtual bool ReadData( std::ifstream& aBoardFile, const std::string& aHeader );

    /**
     * Function WriteData
     * writes the comments and .BOARD_OUTLINE section to an IDFv3 file
     *
     * @param aBoardFile is an IDFv3 file opened for writing
     *
     * @return bool: true if the data had been successfully written, otherwise false.
     */
    virtual bool WriteData( std::ofstream& aBoardFile );

    /**
     * Function Clear
     * frees memory and reinitializes all internal data except for the parent pointer
     */
    virtual void Clear( void );

    /**
     * Function GetOutlineType
     * returns the type of outline according to the IDFv3 classification
     */
    IDF3::OUTLINE_TYPE GetOutlineType( void );

    /**
     * Function SetParent
     * sets the parent IDF_BOARD object
     */
    void SetParent( IDF3_BOARD* aParent );

    /**
     * Function GetParent
     * returns the parent IDF_BOARD object
     */
    IDF3_BOARD* GetParent( void );


    /**
     * Function AddOutline
     * adds the specified outline to this object.
     *
     * @param aOutline is a valid IDF outline
     *
     * @return bool: true if the outline was added; false if the outline
     * already existed. If the outline cannot be added due to a violation
     * of the IDF specification (multiple outlines for anything other than
     * a BOARD_OUTLINE, or the ownership rules are violated) an exception is
     * thrown.
     */
    bool AddOutline( IDF_OUTLINE* aOutline );

    /**
     * Function DelOutline( IDF_OUTLINE* aOutline )
     * removes the given outline, subject to IDF ownership rules,
     * if it is owned by this object. The outline pointer remains
     * valid and it is the user's responsibility to delete the object.
     * The first outline in the list will never be deleted unless it
     * is the sole remaining outline; this is to ensure that a board
     * outline is not removed while the cutouts remain.
     *
     * @param aOutline is a pointer to the outline to remove from the list
     *
     * @return bool: true if the outline was found and removed; false if
     * the outline was not found. If an ownership violation occurs an
     * exception is thrown.
     */
    bool DelOutline( IDF_OUTLINE* aOutline );

    /**
     * Function DelOutline( IDF_OUTLINE* aOutline )
     * deletes the outline specified by the given index, subject to
     * IDF ownership rules. The outline data is destroyed.
     * The first outline in the list will never be deleted unless it
     * is the sole remaining outline; this is to ensure that a board
     * outline is not removed while the cutouts remain.
     *
     * @param aIndex is an index to the outline to delete
     *
     * @return bool: true if the outline was found and deleted; false if
     * the outline was not found. If an ownership violation or indexation
     * error occurs an exception is thrown.
     */
    bool DelOutline( size_t aIndex );

    /**
     *  Function GetOutlines
     *  returns a pointer to the internal outlines list. It is up to the
     *  user to respect the IDFv3 specification and avoid changes to this
     *  list which are in violation of the specification.
     */
    const std::list< IDF_OUTLINE* >*const GetOutlines( void );

    /**
     * Function OutlinesSize
     * returns the number of items in the internal outline list
     */
    size_t OutlinesSize( void );

    /**
     * Function GetOutline
     * returns a pointer to the outline as specified by aIndex.
     * If the index is out of bounds an error is thrown. It is the
     * responsibility of the user to observe IDF ownership rules.
     */
    IDF_OUTLINE* GetOutline( size_t aIndex );

    /**
     * Function GetOwner
     * returns the ownership status of the outline ( ECAD, MCAD, UNOWNED)
     */
    IDF3::KEY_OWNER GetOwner( void );

    /**
     * Function SetOwner
     * sets the ownership status of the outline subject to IDF
     * ownership rules. The return value is true if the ownership
     * was changed and false if a specification violation occurred.
     */
    bool SetOwner( IDF3::KEY_OWNER aOwner );

    /**
     * Function IsSingle
     * return true if this type of outline only supports a single
     * outline. All outlines except for BOARD_OUTLINE are single.
     */
    bool IsSingle( void );

    /**
     * Function ClearOutlines
     * clears internal data except for the parent pointer
     */
    void ClearOutlines( void );

    /**
     * Function AddComment
     * adds a comment to the outline data; this function is not
     * subject to IDF ownership rules.
     */
    void AddComment( const std::string& aComment );

    /**
     * Function CommentSize
     * returns the number of comments in the internal list
     */
    size_t CommentsSize( void );

    /**
     * Function GetComments
     * returns a pointer to the internal list of comments
     */
    std::list< std::string >* GetComments( void );

    /**
     * Function GetComment
     * returns the string representing the indexed comment or
     * NULL if the index is out of bounds
     */
    const std::string* GetComment( size_t aIndex );

    /**
     * Function DeleteComment
     * deletes a comment based on the given index.
     *
     * @return bool: true if a comment was deleted, false if
     * the index is out of bounds.
     */
    bool  DeleteComment( size_t aIndex );

    /**
     * Function ClearComments
     * deletes all comments
     */
    void  ClearComments( void );
};


/**
 * Class OTHER_OUTLINE
 * describes miscellaneous extrusions on the board
 */
class OTHER_OUTLINE : public BOARD_OUTLINE
{
private:
    std::string uniqueID;   // Outline Identifier (IDF spec)
    IDF3::IDF_LAYER side;   // Board Side [TOP/BOTTOM ONLY] (IDF spec)

public:
    OTHER_OUTLINE();

    /**
     * Function SetOutlineIdentifier
     * sets the Outline Identifier string of this OTHER_OUTLINE object
     * as per IDFv3 spec.
     */
    virtual void SetOutlineIdentifier( const std::string aUniqueID );

    /**
     * Function GetOutlineIdentifier
     * returns the object's Outline Identifier
     */
    virtual const std::string& GetOutlineIdentifier( void );

    /**
     * Function SetSide
     * sets the side which this outline is applicable to (TOP, BOTTOM).
     *
     * @return bool: true if the side was set, false if the side is invalid.
     * An exception is thrown if there is a violation of IDF ownership rules.
     */
    virtual bool SetSide( IDF3::IDF_LAYER aSide );

    /**
     * Function GetSide
     * returns the side which this outline is applicable to
     */
    virtual IDF3::IDF_LAYER GetSide( void );

    /**
     * Function ReadData
     * reads an OTHER_OUTLINE data from an IDFv3 file.
     *
     * @param aBoardFile is an IDFv3 file open for reading
     * @param aHeader is the .OTHER_OUTLINE header as read via FetchIDFLine
     *
     * @return bool: true if data was read, otherwise false. If an unrecoverable
     * error occurs an exception is thrown.
     */
    virtual bool ReadData( std::ifstream& aBoardFile, const std::string& aHeader );

    /**
     * Function WriteData
     * writes the OTHER_OUTLINE data to an open IDFv3 file
     *
     * @param aBoardFile is an IDFv3 file open for writing
     *
     * @return bool: true if the data was successfully written, otherwise false.
     */
    virtual bool WriteData( std::ofstream& aBoardFile );

    /**
     * Function Clear
     * deletes internal data except for the parent object
     */
    virtual void Clear( void );
};


/**
 * Class ROUTE_OUTLINE
 * describes routing areas on the board
 */
class ROUTE_OUTLINE : public BOARD_OUTLINE
{
protected:
    IDF3::IDF_LAYER layers; // Routing layers (IDF spec)

public:
    ROUTE_OUTLINE();

    /**
     * Function SetLayers
     * sets the layer or group of layers this outline is applicable to.
     * This function is subject to IDF ownership rules. An exception is
     * thrown if an invalid layer is provided or an IDF ownership violation
     * occurs.
     */
    virtual void SetLayers( IDF3::IDF_LAYER aLayer );

    /**
     * Function GetLayers
     * returns the layer or group of layers which this outline is applicable to
     */
    virtual IDF3::IDF_LAYER GetLayers( void );

    /**
     * Function ReadData
     * reads ROUTE_OUTLINE data from an IDFv3 file
     *
     * @param aBoardFile is an open IDFv3 board file
     * @param aHeader is the .ROUTE_OUTLINE header as returned by FetchIDFLine
     *
     * @return bool: true if data was read, otherwise false. If unrecoverable
     * errors occur an exception is thrown.
     */
    virtual bool ReadData( std::ifstream& aBoardFile, const std::string& aHeader );

    /**
     * Function WriteData
     * writes the ROUTE_OUTLINE data to an open IDFv3 file
     */
    virtual bool WriteData( std::ofstream& aBoardFile );

    /**
     * Function Clear
     * deletes internal data except for the parent object
     */
    virtual void Clear( void );
};

/**
 * Class PLACE_OUTLINE
 * describes areas on the board for placing components
 */
class PLACE_OUTLINE : public BOARD_OUTLINE
{
protected:
    IDF3::IDF_LAYER side;   // Board Side [TOP/BOTTOM/BOTH ONLY] (IDF spec)
    double height;          // Max Height (IDF spec)

public:
    PLACE_OUTLINE();

    /**
     * Function SetSide
     * sets the side (TOP, BOTTOM, BOTH) which this outline applies to,
     * subject to IDF ownership rules. An exception is thrown if there is
     * an ownership violation or an invalid layer is passed.
     */
    virtual void SetSide( IDF3::IDF_LAYER aSide );

    /**
     * Function GetSide
     * returns the side which this outline is applicable to
     */
    virtual IDF3::IDF_LAYER GetSide( void );

    /**
     * Function SetMaxHeight
     * sets the maximum height of a component within this outline,
     * subject to IDF ownership rules. An exception is thrown if
     * there is an ownership violation or aHeight is negative.
     */
    virtual void SetMaxHeight( double aHeight );

    /**
     * Function GetMaxHeight
     * returns the maximum allowable height for a component in this region
     */
    virtual double GetMaxHeight( void );

    /**
     * Function ReadData
     * reads PLACE_OUTLINE data from an open IDFv3 file.
     *
     * @param aBoardFile is an IDFv3 file opened for reading
     * @param aHeader is the .PLACE_OUTLINE header as returned by FetchIDFLine
     *
     * @return bool: true if data was read, otherwise false. If there are
     * unrecoverable errors an exception is thrown.
     */
    virtual bool ReadData( std::ifstream& aBoardFile, const std::string& aHeader );

    /**
     * Function WriteData
     * writes the PLACE_OUTLINE data to an open IDFv3 file
     *
     * @param aBoardFile is an IDFv3 file opened for writing
     *
     * @return bool: true if the data was successfully written, otherwise false
     */
    virtual bool WriteData( std::ofstream& aBoardFile );

    /**
     * Function Clear
     * deletes all internal data
     */
    virtual void Clear( void );
};


/**
 * Class ROUTE_KO_OUTLINE
 * describes regions and layers where no electrical routing is permitted
 */
class ROUTE_KO_OUTLINE : public ROUTE_OUTLINE
{
public:
    ROUTE_KO_OUTLINE();
};

/**
 * Class VIA_KO_OUTLINE
 * describes regions in which vias are prohibited. Note: IDFv3 only considers
 * thru-hole vias and makes no statement regarding behavior with blind or buried
 * vias.
 */
class VIA_KO_OUTLINE : public OTHER_OUTLINE
{
public:
    VIA_KO_OUTLINE();
};


/**
 * Class PLACE_KO_OUTLINE
 * represents regions and layers in which no component may
 * be placed or on which a maximum component height is in effect.
 */
class PLACE_KO_OUTLINE : public PLACE_OUTLINE
{
public:
    PLACE_KO_OUTLINE();
};

/**
 * Class GROUP_OUTLINE
 * represents regions and layers in which user-specified features or components
 * may be placed.
 */
class GROUP_OUTLINE : public BOARD_OUTLINE
{
private:
    IDF3::IDF_LAYER side;   // Board Side [TOP/BOTTOM/BOTH ONLY] (IDF spec)
    std::string groupName;  // non-unique string

public:
    GROUP_OUTLINE();

    /**
     * Function SetSide
     * sets the side which this outline applies to (TOP, BOTTOM, BOTH),
     * subject to IDF ownership rules. If an ownership violation occurs
     * or an invalid side is specified, an exception is thrown.
     */
    virtual void SetSide( IDF3::IDF_LAYER aSide );

    /**
     * Function GetSide
     * returns the side which this outline applies to
     */
    virtual IDF3::IDF_LAYER GetSide( void );

    /**
     * Function SetGroupName
     * sets the name of the group, subject to IDF ownership rules.
     * An empty name or an ownership violation results in a thrown
     * exception.
     */
    virtual void SetGroupName( std::string aGroupName );

    /**
     * Function GetGroupName
     * returns a reference to the (non-unique) group name
     */
    virtual const std::string& GetGroupName( void );

    /**
     * Function ReadData
     * reads GROUP_OUTLINE data from an open IDFv3 file
     *
     * @param aBoardFile is an open IDFv3 file
     * @param aHeader is the .PLACE_REGION header as returned by FetchIDFLine
     *
     * @return bool: true if data was read, otherwise false. If an unrecoverable
     * error occurs an exception is thrown.
     */
    virtual bool ReadData( std::ifstream& aBoardFile, const std::string& aHeader );

    /**
     * Function WriteData
     * writes the data to a .PLACE_REGION section of an IDFv3 file
     *
     * @param aBoardFile is an IDFv3 file open for writing
     *
     * @return bool: true if the data is successfully written, otherwise false
     */
    virtual bool WriteData( std::ofstream& aBoardFile );

    /**
     * Function Clear
     * deletes internal data, subject to IDF ownership rules
     */
    virtual void Clear( void );
};


/**
 * class IDF3_COMP_OUTLINE
 * represents a component's outline as stored in an IDF library file
 */
class IDF3_COMP_OUTLINE : public BOARD_OUTLINE
{
private:
    std::string     uid;        // unique ID
    std::string     geometry;   // geometry name (IDF)
    std::string     part;       // part name (IDF)
    IDF3::COMP_TYPE compType;   // component type
    int             refNum;     // number of components referring to this outline

    std::map< std::string, std::string >    props;      // properties list

    bool readProperties( std::ifstream& aLibFile );
    bool writeProperties( std::ofstream& aLibFile );

public:
    IDF3_COMP_OUTLINE();

    /**
     * Function ReadData
     * reads a component outline from an open IDFv3 file
     *
     * @param aLibFile is an open IDFv3 Library file
     * @param aHeader is the .ELECTRICAL or .MECHANICAL header as returned by FetchIDFLine
     *
     * @return bool: true if data was read, otherwise false. If unrecoverable errors
     * occur, an exception is thrown.
     */
    virtual bool ReadData( std::ifstream& aLibFile, const std::string& aHeader );

    /**
     * Function WriteData
     * writes comments and component outline data to an IDFv3 Library file
     *
     * @param aLibFile is an IDFv3 library file open for writing
     *
     * @return bool: true if the data was successfully written, otherwise false
     */
    virtual bool WriteData( std::ofstream& aLibFile );

    /**
     * Function Clear
     * deletes internal outline data
     */
    virtual void Clear( void );

    /**
     * Function SetComponentClass
     * sets the type of component outline (.ELECTRICAL or .MECHANICAL)
     * If the specified class is invalid an exception is thrown.
     */
    void SetComponentClass( IDF3::COMP_TYPE aCompClass );

    /**
     * Function GetComponentClass
     * returns the class of component represented by this outline
     */
    IDF3::COMP_TYPE GetComponentClass( void );

    /**
     * Function SetGeomName
     * sets the Geometry Name (Package Name, IDFv3 spec) of the component outline
     */
    void SetGeomName( const std::string& aGeomName );

    /**
     * Function GetGeomName
     * returns the Geometry Name (Package Name) of the component outline
     */
    const std::string& GetGeomName( void );

    /**
     * Function SetPartName
     * sets the Part Name (Part Number, IDFv3 spec) of the component outline
     */
    void SetPartName( const std::string& aPartName );

    /**
     * Function GetPartName
     * returns the Part Name (Part Number) of the component outline
     */
    const std::string& GetPartName( void );

    /**
     * Function GetUID
     * returns the unique identifier for this component outline;
     * this is equal to GEOM_NAME + "_" + PART_NAME
     */
    const std::string& GetUID( void );

    /**
     * Function IncrementRef
     * increments the internal reference counter to keep track of the number of
     * components referring to this outline.
     */
    int IncrementRef( void );

    /**
     * Function DecrementRef
     * decrements the internal reference counter to keep track of the number of
     * components referring to this outline.
     */
    int DecrementRef( void );

    /**
     * Function CreateDefaultOutline
     * creates a default outline with the given Geometry and Part names.
     * This outline is a star with outer radius 5mm and inner radius 2.5mm.
     */
    bool CreateDefaultOutline( const std::string &aGeom, const std::string &aPart );

    // XXX: property manipulators
};

#endif // IDF_OUTLINES_H