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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GENERATOR_H_
#define GENERATOR_H_


#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>
#include <string_any_map.h>

#include <lset.h>
#include <pcb_group.h>
#include <geometry/shape_poly_set.h>

class EDIT_POINTS;
class BOARD;
class BOARD_ITEM;
class ZONE;
class PCB_BASE_EDIT_FRAME;
class GENERATOR_TOOL;
class STATUS_MIN_MAX_POPUP;
class ACTION_MENU;
class TOOL_INTERACTIVE;


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

    /**
     * @return true if individual children of this generator can individually selected
     */
    virtual bool ChildrenAreIndividuallySelectable() const { return false; }

    /**
     * Get a context menu when interacting with a generator child
     *
     * @return a non-owning ACTION_MENU* to display, or nullptr to fall through to the
     *         standard selection menu.
     */
    virtual ACTION_MENU* GetChildContextMenu( TOOL_INTERACTIVE* aTool ) const { return nullptr; }

    /**
     * @return true if children of this generator should be treated as
     *         read-only outside of the generator, i.e. disable property editors
     */
    virtual bool ChildrenAreReadOnly() const { return true; }

    /**
     * Named template items used by the generator. The via stitcher for example
     * holds a single template via. These are real board items that are not attached
     * to the board.
     *
     * @return non-owning pointers to the template items; empty by default.
     */
    virtual std::vector<std::pair<wxString, const BOARD_ITEM*>> GetTemplateItems() const { return {}; }

    /**
     * Insert a template item (from board file loading)
     *
     * @param aName Template name
     * @param aItem The parsed item
     */
    virtual void SetTemplateItem( const wxString& aName, std::unique_ptr<BOARD_ITEM> aItem ) {}

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

    /**
     * Dirty flag system to for determining if generator output is not up to date
     * It is up to each generator whether or not to actually use this flag
     */
    bool IsDirty() const { return m_isDirty; }
    void MarkDirty() { m_isDirty = true; }
    void ClearDirty() { m_isDirty = false; }

    /**
     * Callback to be informed zones have changed
     */
    virtual void OnZoneFillChanged( const std::vector<ZONE*>& aZones ) {}

    /**
     * Zones that need to be refilled after the generator runs Update()
     */
    virtual std::vector<ZONE*> GetZonesNeedingRefillAfterUpdate() const { return {}; }

protected:
    wxString m_generatorType;

    VECTOR2I m_origin;

    bool m_isDirty;

#ifdef GENERATOR_ORDER
    int m_updateOrder = 0;
#endif

    friend class GENERATORS_MGR;

    void baseMirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection );
};

class PCB_GENERATOR_POLY : public PCB_GENERATOR
{
public:
    PCB_GENERATOR_POLY( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer ) :
            PCB_GENERATOR( aParent, aLayer )
    {
    }

    const SHAPE_POLY_SET& Outline() const { return m_outline; }
    SHAPE_POLY_SET& Outline() { return m_outline; }

    void SetOutline( const SHAPE_POLY_SET& aOutline ) { m_outline = aOutline; MarkDirty(); }

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_outline.Move( aMoveVector );
        PCB_GENERATOR::Move( aMoveVector );
        MarkDirty();
    }

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override
    {
        m_outline.Rotate( aAngle, aRotCentre );
        PCB_GENERATOR::Rotate( aRotCentre, aAngle );
        MarkDirty();
    }

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override
    {
        m_outline.Mirror( aCentre, aFlipDirection );
        PCB_GENERATOR::Flip( aCentre, aFlipDirection );
        MarkDirty();
    }

    void Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aMirrorDirection ) override
    {
        m_outline.Mirror( aCentre, aMirrorDirection );
        PCB_GENERATOR::Mirror( aCentre, aMirrorDirection );
        MarkDirty();
    }

protected:
    SHAPE_POLY_SET m_outline;
};

#endif /* GENERATOR_H_ */
