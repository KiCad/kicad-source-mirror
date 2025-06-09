/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#ifndef SHEET_SYNCHRONIZATION_ITEM_H
#define SHEET_SYNCHRONIZATION_ITEM_H

#include <wx/bitmap.h>
#include <wx/string.h>
#include <functional>

class EDA_ITEM;
class SCH_HIERLABEL;
class SCH_SHEET_PIN;
class SCH_SHEET;
class SCH_COMMIT;
class SCH_ITEM;

enum class SHEET_SYNCHRONIZATION_ITEM_KIND
{
    HIERLABEL,
    SHEET_PIN,
    HIERLABEL_AND_SHEET_PIN
};

using SCREEN_UPDATER = std::function<void( EDA_ITEM* )>;

class SHEET_SYNCHRONIZATION_ITEM
{
public:
    virtual ~SHEET_SYNCHRONIZATION_ITEM() = default;

    virtual wxString GetName() const = 0;

    virtual int GetShape() const = 0;

    virtual wxBitmap& GetBitmap() const = 0;

    virtual SCH_ITEM* GetItem() const = 0;

    virtual SHEET_SYNCHRONIZATION_ITEM_KIND GetKind() const = 0;
};


class SCH_HIERLABEL_SYNCHRONIZATION_ITEM : public SHEET_SYNCHRONIZATION_ITEM
{
public:
    SCH_HIERLABEL_SYNCHRONIZATION_ITEM( SCH_HIERLABEL* aLabel, SCH_SHEET* aSheet );

    SCH_HIERLABEL* GetLabel() const { return m_label; }

    wxString GetName() const override;

    int GetShape() const override;

    wxBitmap& GetBitmap() const override;

    SCH_ITEM* GetItem() const override;

    SHEET_SYNCHRONIZATION_ITEM_KIND GetKind() const override;

private:
    SCH_HIERLABEL* m_label;
};


class SCH_SHEET_PIN_SYNCHRONIZATION_ITEM : public SHEET_SYNCHRONIZATION_ITEM
{
public:
    SCH_SHEET_PIN_SYNCHRONIZATION_ITEM( SCH_SHEET_PIN* aPin, SCH_SHEET* aSheet );

    SCH_SHEET_PIN* GetPin() const { return m_pin; }

    wxString GetName() const override;

    int GetShape() const override;

    wxBitmap& GetBitmap() const override;

    SCH_ITEM* GetItem() const override;

    SHEET_SYNCHRONIZATION_ITEM_KIND GetKind() const override;

private:
    SCH_SHEET_PIN* m_pin;
};


class ASSOCIATED_SCH_LABEL_PIN : public SHEET_SYNCHRONIZATION_ITEM
{
public:
    ASSOCIATED_SCH_LABEL_PIN( SCH_HIERLABEL* aLabel, SCH_SHEET_PIN* aPin );

    ASSOCIATED_SCH_LABEL_PIN( SCH_HIERLABEL_SYNCHRONIZATION_ITEM* aLabel,
                              SCH_SHEET_PIN_SYNCHRONIZATION_ITEM* aPin );

    SCH_HIERLABEL* GetLabel() const { return m_label; }

    SCH_SHEET_PIN* GetPin() const { return m_pin; }

    wxString GetName() const override;

    int GetShape() const override;

    wxBitmap& GetBitmap() const override;

    SCH_ITEM* GetItem() const override;

    SHEET_SYNCHRONIZATION_ITEM_KIND GetKind() const override;

private:
    SCH_HIERLABEL* m_label;
    SCH_SHEET_PIN* m_pin;
};


#endif
