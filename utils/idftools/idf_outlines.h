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


#ifndef IDF_OUTLINES_H
#define IDF_OUTLINES_H

#include <string>
#include <list>
#include <map>

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
 * IDFv3 BOARD OUTLINE data and is the basis of other IDFv3 outline objects.
 */
class BOARD_OUTLINE
{
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
    virtual bool SetUnit( IDF3::IDF_UNIT aUnit );

    /**
     * Return the native unit type of the outline.
     *
     * @return IDF_UNIT is the native unit (UNIT_MM or UNIT_THOU).
     */
    virtual IDF3::IDF_UNIT GetUnit( void );

    /**
     * Set the thickness or height of the outline (mm).
     *
     * @param aThickness is the thickness or height of the outline in mm.
     */
    virtual bool SetThickness( double aThickness );

    /**
     * @return the thickness or height of an outline (mm).
     */
    virtual double GetThickness( void );

    /**
     * Free memory and reinitializes all internal data except for the parent pointer.
     *
     * @return true if OK, false on ownership violations.
     */
    virtual bool Clear( void );

    /**
     * @return the type of outline according to the IDFv3 classification.
     */
    IDF3::OUTLINE_TYPE GetOutlineType( void );

    /**
     * @return the parent IDF_BOARD object.
     */
    IDF3_BOARD* GetParent( void );


    /**
     * Add the specified outline to this object.
     *
     * @param aOutline is a valid IDF outline.
     * @return true if the outline was added; false if the outline already existed or an
     *         ownership violation occurs.
     */
    bool AddOutline( IDF_OUTLINE* aOutline );

    /**
     * Remove the given outline, subject to IDF ownership rules,
     * if it is owned by this object.
     *
     * The outline pointer remains valid and it is the user's responsibility to delete the
     * object.  The first outline in the list will never be deleted unless it is the sole
     * remaining outline; this is to ensure that a board outline is not removed while the
     * cutouts remain.
     *
     * @param aOutline is a pointer to the outline to remove from the list.
     * @return true if the outline was found and removed; false if the outline was not found
     *         or an ownership violation occurs.
     */
    bool DelOutline( IDF_OUTLINE* aOutline );

    /**
     * Delete the outline specified by the given index, subject to IDF ownership rules.
     *
     * The outline data is destroyed. The first outline in the list will never be deleted
     * unless it is the sole remaining outline; this is to ensure that a board outline is
     * not removed while the cutouts remain.
     *
     * @param aIndex is an index to the outline to delete
     * @return true if the outline was found and deleted; false if the outline was not found
     *         or an ownership violation or indexation error occurs.
     */
    bool DelOutline( size_t aIndex );

    /**
     *  Return outlines list.
     *
     * It is up to the user to respect the IDFv3 specification and avoid changes to this
     * list which are in violation of the specification.
     */
    const std::list<IDF_OUTLINE*>* GetOutlines( void );

    /**
     * @return the number of items in the internal outline list.
     */
    size_t OutlinesSize( void );

    /**
     * Return a pointer to the outline as specified by aIndex.
     *
     * If the index is out of bounds NULL is returned and the error message is set. It is the
     * responsibility of the user to observe IDF ownership rules.
     */
    IDF_OUTLINE* GetOutline( size_t aIndex );

    /**
     * @return the ownership status of the outline (ECAD, MCAD, UNOWNED).
     */
    IDF3::KEY_OWNER GetOwner( void );

    /**
     * Set the ownership status of the outline subject to IDF ownership rules.
     *
     * @return true if the ownership was changed and false if a specification violation occurred.
     */
    bool SetOwner( IDF3::KEY_OWNER aOwner );

    /**
     * @return true if this type of outline only supports a single outline. All outlines except
     *         for BOARD_OUTLINE are single.
     */
    bool IsSingle( void );

    /**
     * Clear internal data except for the parent pointer.
     */
    void ClearOutlines( void );

    /**
     * Add a comment to the outline data.
     *
     * This function is not subject to IDF ownership rules.
     */
    void AddComment( const std::string& aComment );

    /**
     * @return the number of comments in the internal list.
     */
    size_t CommentsSize( void );

    /**
     * @return the internal list of comments.
     */
    std::list< std::string >* GetComments( void );

    /**
     * @return indexed comment or NULL if the index is out of bounds.
     */
    const std::string* GetComment( size_t aIndex );

    /**
     * Deletes a comment based on the given index.
     *
     * @return true if a comment was deleted, false if the index is out of bounds.
     */
    bool DeleteComment( size_t aIndex );

    /**
     * Deletes all comments.
     */
    void ClearComments( void );

    const std::string& GetError( void )
    {
        return errormsg;
    }

protected:
    // Read outline data from a BOARD or LIBRARY file's outline section
    void readOutlines( std::istream& aBoardFile, IDF3::IDF_VERSION aIdfVersion );

    // Write comments to a BOARD or LIBRARY file (must not be within a SECTION as per IDFv3 spec)
    bool writeComments( std::ostream& aBoardFile );

    // Write the outline owner to a BOARD file
    bool writeOwner( std::ostream& aBoardFile );

    // Write the data of a single outline object
    void writeOutline( std::ostream& aBoardFile, IDF_OUTLINE* aOutline, size_t aIndex );

    // Iterate through the outlines and write out all data
    void writeOutlines( std::ostream& aBoardFile );  // write outline data (no headers)

    // Clear internal list of outlines
    void clearOutlines( void );

    /**
     * Set the parent IDF_BOARD object.
     */
    void setParent( IDF3_BOARD* aParent );

    // Shadow routines used by friends to bypass ownership checks
    bool addOutline( IDF_OUTLINE* aOutline );
    virtual bool setThickness( double aThickness );
    virtual void clear( void );

    /**
     * Read data from a .BOARD_OUTLINE section.
     *
     * In case of an unrecoverable error an exception is thrown. On a successful
     * return the file pointer will be at the line following .END_BOARD_OUTLINE
     *
     * @param aBoardFile is an IDFv3 file opened for reading.
     * @param aHeader is the ".BOARD_OUTLINE" header line as read by FetchIDFLine.
     */
    virtual void readData( std::istream& aBoardFile, const std::string& aHeader,
                           IDF3::IDF_VERSION aIdfVersion );

    /**
     * Write the comments and .BOARD_OUTLINE section to an IDFv3 file.
     * Throws exceptions.
     *
     * @param aBoardFile is an IDFv3 file opened for writing.
     */
    virtual void writeData( std::ostream& aBoardFile );

    std::string                 errormsg;
    std::list< IDF_OUTLINE* >   outlines;

    // indicates the owner of this outline (MCAD, ECAD, UNOWNED).
    IDF3::KEY_OWNER             owner;
    IDF3::OUTLINE_TYPE          outlineType;  // type of IDF outline
    bool                        single;       // true if only a single outline is accepted
    std::list< std::string >    comments;     // associated comment list
    IDF3::IDF_UNIT              unit;         // outline's native unit (MM or THOU)
    IDF3_BOARD*                 parent;       // BOARD which contains this outline
    double                      thickness;    // Board/Extrude Thickness or Height (IDF spec)

private:
    friend class IDF3_BOARD;
};


/**
 * Miscellaneous extrusions on the board
 */
class OTHER_OUTLINE : public BOARD_OUTLINE
{
public:
    OTHER_OUTLINE( IDF3_BOARD* aParent );

    /**
     * Function SetOutlineIdentifier
     * sets the Outline Identifier string of this OTHER_OUTLINE object
     * as per IDFv3 spec.
     */
    virtual bool SetOutlineIdentifier( const std::string& aUniqueID );

    /**
     * Function GetOutlineIdentifier
     * returns the object's Outline Identifier
     */
    virtual const std::string& GetOutlineIdentifier( void );

    /**
     * Function SetSide
     * sets the side which this outline is applicable to (TOP, BOTTOM).
     *
     * @return bool: true if the side was set, false if the side is invalid
     * or there is a violation of IDF ownership rules.
     */
    virtual bool SetSide( IDF3::IDF_LAYER aSide );

    /**
     * Function GetSide
     * returns the side which this outline is applicable to
     */
    virtual IDF3::IDF_LAYER GetSide( void );

    /**
     * Function Clear
     * deletes internal data except for the parent object
     */
    virtual bool Clear( void ) override;

private:
    /**
     * Read an OTHER_OUTLINE data from an IDFv3 file.
     * If an unrecoverable error occurs an exception is thrown.
     *
     * @param aBoardFile is an IDFv3 file open for reading.
     * @param aHeader is the .OTHER_OUTLINE header as read via FetchIDFLine.
     */
    virtual void readData( std::istream& aBoardFile, const std::string& aHeader,
                           IDF3::IDF_VERSION aIdfVersion ) override;

    /**
     * Write the OTHER_OUTLINE data to an open IDFv3 file.
     *
     * @param aBoardFile is an IDFv3 file open for writing.
     * @return true if the data was successfully written, otherwise false.
     */
    virtual void writeData( std::ostream& aBoardFile ) override;

    friend class IDF3_BOARD;

    std::string uniqueID;   // Outline Identifier (IDF spec)
    IDF3::IDF_LAYER side;   // Board Side [TOP/BOTTOM ONLY] (IDF spec)
};


/**
 * Routing areas on the board.
 */
class ROUTE_OUTLINE : public BOARD_OUTLINE
{
public:
    ROUTE_OUTLINE( IDF3_BOARD* aParent );

    /**
     * Function SetLayers
     * sets the layer or group of layers this outline is applicable to.
     * This function is subject to IDF ownership rules; true is returned
     * on success, otherwise false is returned and the error message is set.
     */
    virtual bool SetLayers( IDF3::IDF_LAYER aLayer );

    /**
     * Function GetLayers
     * returns the layer or group of layers which this outline is applicable to
     */
    virtual IDF3::IDF_LAYER GetLayers( void );

    /**
     * Function Clear
     * deletes internal data except for the parent object
     */
    virtual bool Clear( void ) override;

private:
    friend class IDF3_BOARD;

    /**
     * Read ROUTE_OUTLINE data from an IDFv3 file.
     * If an unrecoverable error occurs an exception is thrown.
     *
     * @param aBoardFile is an open IDFv3 board file.
     * @param aHeader is the .ROUTE_OUTLINE header as returned by FetchIDFLine.
     */
    virtual void readData( std::istream& aBoardFile, const std::string& aHeader,
                           IDF3::IDF_VERSION aIdfVersion ) override;

    /**
     * Write the ROUTE_OUTLINE data to an open IDFv3 file.
     */
    virtual void writeData( std::ostream& aBoardFile ) override;

protected:
    IDF3::IDF_LAYER layers; // Routing layers (IDF spec)
};


/**
 * Area on the board for placing components.
 */
class PLACE_OUTLINE : public BOARD_OUTLINE
{
public:
    PLACE_OUTLINE( IDF3_BOARD* aParent );

    /**
     * Function SetSide
     * sets the side (TOP, BOTTOM, BOTH) which this outline applies to.
     * This function is subject to IDF ownership rules; true is returned
     * on success, otherwise false is returned and the error message is set.
     */
    virtual bool SetSide( IDF3::IDF_LAYER aSide );

    /**
     * Function GetSide
     * returns the side which this outline is applicable to
     */
    virtual IDF3::IDF_LAYER GetSide( void );

    /**
     * Function SetMaxHeight
     * sets the maximum height of a component within this outline.
     * This function is subject to IDF ownership rules; true is returned
     * on success, otherwise false is returned and the error message is set.
     */
    virtual bool SetMaxHeight( double aHeight );

    /**
     * Function GetMaxHeight
     * returns the maximum allowable height for a component in this region
     */
    virtual double GetMaxHeight( void );

    /**
     * Function Clear
     * deletes all internal data
     */
    virtual bool Clear( void ) override;

private:
    friend class IDF3_BOARD;

    /**
     * Read PLACE_OUTLINE data from an open IDFv3 file.
     *
     * If an unrecoverable error occurs an exception is thrown.
     *
     * @param aBoardFile is an IDFv3 file opened for reading.
     * @param aHeader is the .PLACE_OUTLINE header as returned by FetchIDFLine.
     */
    virtual void readData( std::istream& aBoardFile, const std::string& aHeader,
                           IDF3::IDF_VERSION aIdfVersion ) override;

    /**
     * Write the PLACE_OUTLINE data to an open IDFv3 file.
     *
     * @param aBoardFile is an IDFv3 file opened for writing.
     * @return true if the data was successfully written, otherwise false.
     */
    virtual void writeData( std::ostream& aBoardFile ) override;

protected:
    IDF3::IDF_LAYER side;   // Board Side [TOP/BOTTOM/BOTH ONLY] (IDF spec)
};


/**
 * Regions and layers where no electrical routing is permitted.
 */
class ROUTE_KO_OUTLINE : public ROUTE_OUTLINE
{
public:
    ROUTE_KO_OUTLINE( IDF3_BOARD* aParent );
};


/**
 * Region in which vias are prohibited.
 *
 * @note IDFv3 only considers thru-hole vias and makes no statement regarding behavior with
 *       blind or buried vias.
 */
class VIA_KO_OUTLINE : public OTHER_OUTLINE
{
public:
    VIA_KO_OUTLINE( IDF3_BOARD* aParent );
};


/**
 * Regions and layers in which no component may be placed or on which a maximum component height
 * is in effect.
 */
class PLACE_KO_OUTLINE : public PLACE_OUTLINE
{
public:
    PLACE_KO_OUTLINE( IDF3_BOARD* aParent );
};


/**
 * Regions and layers in which user-specified features or components may be placed.
 */
class GROUP_OUTLINE : public BOARD_OUTLINE
{
public:
    GROUP_OUTLINE( IDF3_BOARD* aParent );

    /**
     * Set the side which this outline applies to (TOP, BOTTOM, BOTH).
     *
     * This function is subject to IDF ownership rules; true is returned
     * on success, otherwise false is returned and the error message is set.
     */
    virtual bool SetSide( IDF3::IDF_LAYER aSide );

    /**
     * @return the side which this outline applies to.
     */
    virtual IDF3::IDF_LAYER GetSide( void );

    /**
     * Set the name of the group, subject to IDF ownership rules.
     *
     * This function is subject to IDF ownership rules; true is returned
     * on success, otherwise false is returned and the error message is set.
     */
    virtual bool SetGroupName( std::string aGroupName );

    /**
     * Return a reference to the (non-unique) group name.
     */
    virtual const std::string& GetGroupName( void );

    /**
     * Delete internal data, subject to IDF ownership rules.
     */
    virtual bool Clear( void ) override;

private:
    friend class IDF3_BOARD;

    /**
     * Read GROUP_OUTLINE data from an open IDFv3 file.
     *
     * If an unrecoverable error occurs an exception is thrown.
     *
     * @param aBoardFile is an open IDFv3 file.
     * @param aHeader is the .PLACE_REGION header as returned by FetchIDFLine.
     */
    virtual void readData( std::istream& aBoardFile, const std::string& aHeader,
                           IDF3::IDF_VERSION aIdfVersion ) override;

    /**
     * Write the data to a .PLACE_REGION section of an IDFv3 file.
     *
     * @param aBoardFile is an IDFv3 file open for writing.
     * @return true if the data is successfully written, otherwise false.
     */
    virtual void writeData( std::ostream& aBoardFile ) override;

    IDF3::IDF_LAYER side;   // Board Side [TOP/BOTTOM/BOTH ONLY] (IDF spec)
    std::string groupName;  // non-unique string
};


/**
 * A component's outline as stored in an IDF library file.
 */
class IDF3_COMP_OUTLINE : public BOARD_OUTLINE
{
public:
    IDF3_COMP_OUTLINE( IDF3_BOARD* aParent );

    /**
     * Delete internal outline data.
     */
    virtual bool Clear( void ) override;

    /**
     * Set the type of component outline (.ELECTRICAL or .MECHANICAL).
     *
     * @return true on success, otherwise false and the error message is set.
     */
    bool SetComponentClass( IDF3::COMP_TYPE aCompClass );

    /**
     * @return2 the class of component represented by this outline.
     */
    IDF3::COMP_TYPE GetComponentClass( void );

    /**
     * Set the Geometry Name (Package Name, IDFv3 spec) of the component outline.
     */
    void SetGeomName( const std::string& aGeomName );

    /**
     * @return the Geometry Name (Package Name) of the component outline.
     */
    const std::string& GetGeomName( void );

    /**
     * Set the Part Name (Part Number, IDFv3 spec) of the component outline.
     */
    void SetPartName( const std::string& aPartName );

    /**
     * Return the Part Name (Part Number) of the component outline.
     */
    const std::string& GetPartName( void );

    /**
     * @return the unique identifier for this component outline, this is equal to
     *         GEOM_NAME + "_" + PART_NAME.
     */
    const std::string& GetUID( void );

    /**
     * Create a default outline with the given Geometry and Part names.
     *
     * This outline is a star with outer radius 5mm and inner radius 2.5mm.
     */
    bool CreateDefaultOutline( const std::string &aGeom, const std::string &aPart );

    // XXX: property manipulators

private:
    friend class IDF3_BOARD;
    friend class IDF3_COMP_OUTLINE_DATA;

    void readProperties( std::istream& aLibFile );
    bool writeProperties( std::ostream& aLibFile );

    /**
     * Read a component outline from an open IDFv3 file.
     *
     * If an unrecoverable error occurs, an exception is thrown.
     *
     * @param aLibFile is an open IDFv3 Library file.
     * @param aHeader is the .ELECTRICAL or .MECHANICAL header as returned by FetchIDFLine.
     */
    virtual void readData( std::istream& aLibFile, const std::string& aHeader,
                           IDF3::IDF_VERSION aIdfVersion ) override;

    /**
     * Write comments and component outline data to an IDFv3 Library file.
     *
     * @param aLibFile is an IDFv3 library file open for writing.
     * @return true if the data was successfully written, otherwise false.
     */
    virtual void writeData( std::ostream& aLibFile ) override;

    /**
     * Increment the internal reference counter to keep track of the number of
     * components referring to this outline.
     *
     * @return the number of current references to this component outline.
     */
    int incrementRef( void );

    /**
     * Decrement the internal reference counter to keep track of the number of
     * components referring to this outline.
     *
     * @return the number of remaining references or -1 if there were no references when the
     *         function was invoked, in which case the error message is also set.
     */
    int decrementRef( void );

    std::string     uid;        // unique ID
    std::string     geometry;   // geometry name (IDF)
    std::string     part;       // part name (IDF)
    IDF3::COMP_TYPE compType;   // component type
    int             refNum;     // number of components referring to this outline

    std::map< std::string, std::string >    props;      // properties list
};

#endif // IDF_OUTLINES_H
