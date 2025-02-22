# This program source code file is part of KiCad, a free EDA CAD application.
# Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# https://www.gnu.org/licenses/gpl-3.0.en.html
# or you may search the http://www.gnu.org website for the version 32 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

"""
KiCad HTTP Lib Test Server
==========================

This module implements a test HTTP server using FastAPI.  It is a simple way to spin up a local
API server to test the Kicad HTTP feature.

fastAPI gives you a test interface at the /docs endpoint to observe the responses.

Usage
-----
Run the server using:
    python http_lib_test_server.py

The server will start on http://0.0.0.0:8000

Mock Data
---------
The current implementation includes mock data for testing. 

- One resistor category
- One sample resistor part (RC0603FR-0710KL)
- It assumes the default Kicad libraries are setup in the symbol/footprint table.
  it uses the basic resistor symbol and an 0603 footprint

Dependencies
-----------
- FastAPI: Web framework
- Pydantic: Data validation
- uvicorn: ASGI server

For development and testing purposes only. Not intended for production use.
"""
 

from typing import List, Optional, Dict

import uvicorn
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

app = FastAPI()

class Category(BaseModel):
    id: str
    name: str
    description: Optional[str] = None

class PartsChooser(BaseModel):
    id: str
    name: str
    description: Optional[str] = None

class PartField(BaseModel):
    value: str
    visible: Optional[str] = "true"

class DetailedPart(BaseModel):
    id: str
    name: Optional[str] = None
    symbolIdStr: str
    exclude_from_bom: str
    exclude_from_board: str
    exclude_from_sim: str
    fields: Dict[str, PartField]

categories = [
    Category(id="1", name="Resistors", description = "Resistors"),
]

parts_by_category: Dict[str, List[PartsChooser]] = {
    "1": [
        PartsChooser(
            id="1",
            name="RC0603FR-0710KL",
            description="10 kOhms ±1% 0.1W, 1/10W Chip Resistor 0603 (1608 Metric) Moisture Resistant Thick Film"
        )
    ]
}

detailed_parts = {
    "1": DetailedPart(
        id="1",
        name="RC0603FR-0710KL",
        symbolIdStr = "Device:R",
        exclude_from_bom="False",
        exclude_from_board="False",
        exclude_from_sim="True",
        fields={
            "footprint": PartField(
                value="Resistor_SMD:R_0603_1608Metric",
                visible="False"
            ),
            "datasheet": PartField(
                value="www.kicad.org",
                visible="False"
            ),
            "value": PartField(
                value="10k"
            ),
            "reference": PartField(
                value="R"
            ),
            "description": PartField(
                value="10 kOhms ±1% 0.1W, 1/10W Chip Resistor 0603 (1608 Metric) Moisture Resistant Thick Film",
                visible="False"
            ),
            "keywords": PartField(
                value="RES passive smd",
                visible="False"
            )
        }
    )
}

@app.get("/v1/")
async def get_endpoints():
    return {
        "categories": "",
        "parts": ""
    }

@app.get("/v1/categories.json", response_model=List[Category])
async def get_categories():

    return categories

@app.get("/v1/parts/category/{category_id}.json", response_model=List[PartsChooser])
async def get_parts_by_category(category_id: str):

    if category_id not in parts_by_category:
        raise HTTPException(status_code=404, detail=f"Category {category_id} not found")

    return parts_by_category[category_id]

@app.get("/v1/parts/{part_id}.json", response_model=DetailedPart)
async def get_part_details(part_id: str):

    if part_id not in detailed_parts:
        raise HTTPException(status_code=404, detail=f"Part {part_id} not found")

    return detailed_parts[part_id]

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)


