/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef GENERATOR_H_
#define GENERATOR_H_


#include <unordered_set>
#include <string_any_map.h>

#include <lset.h>
#include <pcb_group.h>

class EDIT_POINTS;
class BOARD;
class BOARD_ITEM;
class PCB_BASE_EDIT_FRAME;
class GENERATOR_TOOL;
class STATUS_MIN_MAX_POPUP;


class PCB_GENERATOR : public PCB_GROUP
{
public:

    PCB_GENERATOR( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer );

    virtual ~PCB_GENERATOR();

    /*
     * Clone() this and all descendants
     */
    PCB_GENERATOR* DeepClone() const;

    virtual void EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) = 0;
    virtual bool Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) = 0;
    virtual void EditFinish( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) = 0;
    virtual void EditCancel( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) = 0;

    virtual void Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) = 0;

#define STATUS_ITEMS_ONLY true

    virtual std::vector<EDA_ITEM*> GetPreviewItems( GENERATOR_TOOL* aTool,
                                                    PCB_BASE_EDIT_FRAME* aFrame,
                                                    bool aStatusItemsOnly = false );

    virtual bool MakeEditPoints( EDIT_POINTS& aEditPoints ) const;

    virtual bool UpdateFromEditPoints( EDIT_POINTS& aEditPoints );

    virtual bool UpdateEditPoints( EDIT_POINTS& aEditPoints );

    const BOX2I GetBoundingBox() const override;

    VECTOR2I GetPosition() const override { return m_origin; }
    void SetPosition( const VECTOR2I& aPos ) override { m_origin = aPos; }

    void Move( const VECTOR2I& aMoveVector ) override;

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    void Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aMirrorDirection ) override;

    LSET GetLayerSet() const override;

    virtual void SetLayer( PCB_LAYER_ID aLayer ) override;

    virtual wxString GetGeneratorType() const;

    virtual const STRING_ANY_MAP GetProperties() const;

    virtual void SetProperties( const STRING_ANY_MAP& aProps );

    virtual std::vector<std::pair<wxString, wxVariant>> GetRowData();

    virtual void ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame ) {};

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    virtual wxString GetPluralName() const = 0;
    virtual wxString GetCommitMessage() const = 0;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

    wxString GetClass() const override;

    static inline bool ClassOf( const EDA_ITEM* aItem );

#ifdef GENERATOR_ORDER
    int  GetUpdateOrder() const { return m_updateOrder; }
    void SetUpdateOrder( int aValue ) { m_updateOrder = aValue; }
#endif

protected:
    wxString m_generatorType;

    VECTOR2I m_origin;

#ifdef GENERATOR_ORDER
    int m_updateOrder = 0;
#endif

    friend class GENERATORS_MGR;

    void baseMirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection );
};

#endif /* GENERATOR_H_ */
