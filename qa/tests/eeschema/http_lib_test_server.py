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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

"""
KiCad HTTP / Remote Provider Test Server
========================================

Local fixture server for two paths:

- legacy HTTP symbol library endpoints under `/v1/`
- new remote provider metadata/manifest/download endpoints

The provider flow includes a provider-owned embedded page plus lightweight
OAuth and download stubs so KiCad can exercise the full panel flow without a
third-party service.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
from typing import Dict, List, Optional

import uvicorn
from fastapi import FastAPI, Form, HTTPException, Request
from fastapi.responses import PlainTextResponse, RedirectResponse, Response
from pydantic import BaseModel

app = FastAPI()


def base_url(request: Request) -> str:
    return str(request.base_url).rstrip("/")


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
    Category(id="1", name="Resistors", description="Resistors"),
]

parts_by_category: Dict[str, List[PartsChooser]] = {
    "1": [
        PartsChooser(
            id="1",
            name="RC0603FR-0710KL",
            description="10 kOhms ±1% 0.1W, 1/10W Chip Resistor 0603 (1608 Metric) Moisture Resistant Thick Film",
        )
    ]
}

detailed_parts = {
    "1": DetailedPart(
        id="1",
        name="RC0603FR-0710KL",
        symbolIdStr="Device:R",
        exclude_from_bom="False",
        exclude_from_board="False",
        exclude_from_sim="True",
        fields={
            "footprint": PartField(
                value="Resistor_SMD:R_0603_1608Metric",
                visible="False",
            ),
            "datasheet": PartField(
                value="www.kicad.org",
                visible="False",
            ),
            "value": PartField(value="10k"),
            "reference": PartField(value="R"),
            "description": PartField(
                value="10 kOhms ±1% 0.1W, 1/10W Chip Resistor 0603 (1608 Metric) Moisture Resistant Thick Film",
                visible="False",
            ),
            "keywords": PartField(value="RES passive smd", visible="False"),
        },
    )
}

provider_parts = [
    {
        "part_id": "acme-res-10k",
        "display_name": "RC0603FR-0710KL",
        "symbol_name": "R",
        "library_name": "Device",
        "summary": "10 kOhms ±1% 0603 thick film resistor",
        "manufacturer": "Yageo",
        "mpn": "RC0603FR-0710KL",
        "license": "CC-BY-4.0",
        "auth_required": False,
        "available_assets": ["symbol", "footprint"],
    },
    {
        "part_id": "acme-res-1k",
        "display_name": "RC0603FR-071KL",
        "symbol_name": "R",
        "library_name": "Device",
        "summary": "1 kOhms ±1% 0603 thick film resistor",
        "manufacturer": "Yageo",
        "mpn": "RC0603FR-071KL",
        "license": "CC-BY-4.0",
        "auth_required": False,
        "available_assets": ["symbol"],
    },
]


def symbol_payload(symbol_name: str) -> str:
    return f"""(kicad_symbol_lib (version 20220914) (generator kicad_symbol_editor)
  (symbol "{symbol_name}" (in_bom yes) (on_board yes)
    (property "Reference" "R" (at 0 0 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "{symbol_name}" (at 0 0 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "" (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (property "Datasheet" "" (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (symbol "{symbol_name}_0_1"
      (rectangle (start -1.27 -1.27) (end 1.27 1.27)
        (stroke (width 0) (type default))
        (fill (type background))
      )
    )
    (symbol "{symbol_name}_1_1"
      (pin passive line (at -3.81 0 0) (length 2.54)
        (name "PIN" (effects (font (size 1.27 1.27))))
        (number "1" (effects (font (size 1.27 1.27))))
      )
    )
  )
)
"""


def asset_size_and_sha256(asset_name: str) -> tuple[int, str]:
    payload, _content_type = download_payloads[asset_name]
    payload_bytes = payload.encode("utf-8")
    return len(payload_bytes), hashlib.sha256(payload_bytes).hexdigest()


def manifest_payload(request: Request, part_id: str) -> Dict[str, object]:
    root = base_url(request)

    manifests = {
        "acme-res-10k": {
            "part_id": "acme-res-10k",
            "display_name": "RC0603FR-0710KL",
            "summary": "10 kOhms ±1% 0603 thick film resistor",
            "license": "CC-BY-4.0",
            "assets": [
                {
                    "asset_type": "symbol",
                    "name": "acme-res-10k.kicad_sym",
                    "target_library": "Device",
                    "target_name": "RC0603FR-0710KL",
                    "content_type": "application/x-kicad-symbol",
                    "size_bytes": asset_size_and_sha256("acme-res-10k.kicad_sym")[0],
                    "sha256": asset_size_and_sha256("acme-res-10k.kicad_sym")[1],
                    "download_url": f"{root}/downloads/acme-res-10k.kicad_sym",
                    "required": True,
                },
                {
                    "asset_type": "footprint",
                    "name": "R_0603.pretty",
                    "target_library": "Resistor_SMD",
                    "target_name": "R_0603_1608Metric",
                    "content_type": "application/x-kicad-footprint",
                    "size_bytes": 44,
                    "sha256": "8d8090740282c9ec23541a148af0ae57543e0da581e00e714e066dc4a1adefb0",
                    "download_url": f"{root}/downloads/R_0603.pretty",
                    "required": False,
                },
            ],
        },
        "acme-res-1k": {
            "part_id": "acme-res-1k",
            "display_name": "RC0603FR-071KL",
            "summary": "1 kOhms ±1% 0603 thick film resistor",
            "license": "CC-BY-4.0",
            "assets": [
                {
                    "asset_type": "symbol",
                    "name": "acme-res-1k.kicad_sym",
                    "target_library": "Device",
                    "target_name": "RC0603FR-071KL",
                    "content_type": "application/x-kicad-symbol",
                    "size_bytes": asset_size_and_sha256("acme-res-1k.kicad_sym")[0],
                    "sha256": asset_size_and_sha256("acme-res-1k.kicad_sym")[1],
                    "download_url": f"{root}/downloads/acme-res-1k.kicad_sym",
                    "required": True,
                }
            ],
        },
    }

    if part_id not in manifests:
        raise HTTPException(status_code=404, detail=f"Part {part_id} not found")

    return manifests[part_id]


download_payloads = {
    "acme-res-10k.kicad_sym": (
        symbol_payload("RC0603FR-0710KL"),
        "application/x-kicad-symbol",
    ),
    "acme-res-1k.kicad_sym": (
        symbol_payload("RC0603FR-071KL"),
        "application/x-kicad-symbol",
    ),
    "R_0603.pretty": (
        "(module \"R_0603_1608Metric\" (layer \"F.Cu\"))\n",
        "application/x-kicad-footprint",
    ),
    "preview.svg": (
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"96\" height=\"32\"><rect width=\"96\" height=\"32\" fill=\"#f5f1e8\"/><text x=\"8\" y=\"20\" font-size=\"12\">RC0603FR</text></svg>\n",
        "image/svg+xml",
    ),
}


def inline_component_bundle(request: Request, part_id: str) -> str:
    manifest = manifest_payload(request, part_id)
    entries = []

    for asset in manifest["assets"]:
        payload, _content_type = download_payloads[asset["name"]]
        entries.append(
            {
                "type": asset["asset_type"],
                "name": asset.get("target_name") or asset["name"],
                "content": base64.b64encode(payload.encode("utf-8")).decode("ascii"),
                "compression": "NONE",
            }
        )

    return base64.b64encode(json.dumps(entries).encode("utf-8")).decode("ascii")


@app.get("/")
async def root():
    return RedirectResponse(url="/app")


@app.get("/app")
async def provider_app(request: Request):
    manifests = {part["part_id"]: manifest_payload(request, part["part_id"]) for part in provider_parts}
    inline_payloads = {
        part_id: inline_component_bundle(request, part_id) for part_id in manifests.keys()
    }
    provider_session = request.cookies.get("kicad_provider_session") == "active"

    html = f"""
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Embedded Provider Demo</title>
  <style>
    :root {{
      --paper: #f4efe2;
      --ink: #1e2430;
      --accent: #ae3f2f;
      --accent-2: #1f5c52;
      --line: #d3c7ab;
      --card: #fffaf0;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      font-family: "Iowan Old Style", "Palatino Linotype", serif;
      background:
        radial-gradient(circle at top left, rgba(174,63,47,0.09), transparent 30%),
        linear-gradient(180deg, #f8f3e8, var(--paper));
      color: var(--ink);
    }}
    main {{ padding: 20px; }}
    .header {{ display: flex; gap: 16px; align-items: end; justify-content: space-between; margin-bottom: 20px; }}
    .title {{ max-width: 42rem; }}
    h1 {{ margin: 0; font-size: 2rem; letter-spacing: 0.03em; }}
    .sub {{ margin-top: 8px; font-size: 0.95rem; opacity: 0.8; }}
    .auth {{
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      padding: 14px;
      min-width: 260px;
      box-shadow: 0 10px 30px rgba(30,36,48,0.08);
    }}
    .search {{
      width: 100%;
      padding: 12px 14px;
      border: 1px solid var(--line);
      border-radius: 999px;
      background: rgba(255,255,255,0.8);
      margin-bottom: 16px;
      font: inherit;
    }}
    .grid {{ display: grid; gap: 14px; }}
    .card {{
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 16px;
      padding: 16px;
      box-shadow: 0 10px 30px rgba(30,36,48,0.08);
    }}
    .card h2 {{ margin: 0 0 6px; font-size: 1.05rem; }}
    .meta {{ font-size: 0.85rem; opacity: 0.74; }}
    .actions {{ display: flex; flex-wrap: wrap; gap: 8px; margin-top: 12px; }}
    button {{
      border: 0;
      border-radius: 999px;
      padding: 10px 14px;
      font: inherit;
      cursor: pointer;
      color: white;
      background: var(--accent);
    }}
    button.alt {{ background: var(--accent-2); }}
    button.ghost {{
      background: transparent;
      color: var(--ink);
      border: 1px solid var(--line);
    }}
    pre {{
      margin: 18px 0 0;
      padding: 12px;
      border-radius: 12px;
      background: #231f20;
      color: #f3ead8;
      min-height: 100px;
      white-space: pre-wrap;
      font: 12px/1.5 ui-monospace, monospace;
    }}
  </style>
</head>
<body>
  <main>
    <section class="header">
      <div class="title">
        <h1>Embedded Provider Demo</h1>
        <div class="sub">Provider-owned UI in the KiCad panel. KiCad bridge handles auth, import, download, and place.</div>
      </div>
      <div class="auth">
        <div id="auth-state">Not connected to KiCad yet.</div>
        <div class="actions" style="margin-top:10px">
          <button id="signin">Sign In</button>
          <button id="signout" class="ghost">Sign Out</button>
          <button id="refresh" class="ghost">Refresh State</button>
        </div>
      </div>
    </section>

    <input id="query" class="search" type="search" placeholder="Filter demo parts" />
    <section id="parts" class="grid"></section>
    <pre id="log">Waiting for KiCad session...</pre>
  </main>
  <script>
    (() => {{
      const RPC_VERSION = 1;
      const PARTS = {json.dumps(provider_parts)};
      const MANIFESTS = {json.dumps(manifests)};
      const INLINE_BUNDLES = {json.dumps(inline_payloads)};
      const SERVER_SESSION_ACTIVE = {json.dumps(provider_session)};
      const waiters = new Map();
      let sessionId = null;
      let messageCounter = 0;
      let authenticated = false;

      const logEl = document.getElementById("log");
      const authEl = document.getElementById("auth-state");
      const partsEl = document.getElementById("parts");
      const queryEl = document.getElementById("query");

      function log(message) {{
        const stamp = new Date().toLocaleTimeString();
        logEl.textContent = `[${{stamp}}] ${{message}}\\n` + logEl.textContent;
      }}

      function updateAuthLabel() {{
        authEl.textContent = authenticated
          ? `Authenticated. Provider session cookie: ${{SERVER_SESSION_ACTIVE ? "yes" : "pending reload"}}`
          : "Signed out.";
      }}

      function postToKiCad(payload) {{
        if (window.webkit?.messageHandlers?.kicad) {{
          window.webkit.messageHandlers.kicad.postMessage(payload);
          return;
        }}
        if (window.chrome?.webview?.postMessage) {{
          window.chrome.webview.postMessage(payload);
          return;
        }}
        if (window.external?.invoke) {{
          window.external.invoke(payload);
          return;
        }}
        log("No KiCad bridge found.");
      }}

      function envelope(command, parameters = {{}}, data = "") {{
        if (!sessionId) throw new Error("No KiCad session yet.");
        return {{
          version: RPC_VERSION,
          session_id: sessionId,
          message_id: ++messageCounter,
          command,
          parameters,
          data
        }};
      }}

      function rpc(command, parameters = {{}}, data = "") {{
        const payload = envelope(command, parameters, data);
        return new Promise((resolve, reject) => {{
          const timer = window.setTimeout(() => {{
            waiters.delete(payload.message_id);
            reject(new Error("RPC timeout"));
          }}, 15000);

          waiters.set(payload.message_id, (response) => {{
            window.clearTimeout(timer);
            if (response.status === "ERROR") {{
              reject(new Error(response.error_message || response.error_code || "RPC error"));
              return;
            }}
            resolve(response);
          }});

          postToKiCad(JSON.stringify(payload));
        }});
      }}

      function handleIncoming(raw) {{
        let payload = raw;
        if (typeof payload === "string") {{
          try {{
            payload = JSON.parse(payload);
          }} catch (_err) {{
            return;
          }}
        }}

        if (!payload || typeof payload !== "object") return;

        if (payload.command === "NEW_SESSION" && payload.response_to === undefined) {{
          sessionId = payload.session_id;
          messageCounter = payload.message_id || 0;
          postToKiCad(JSON.stringify({{
            version: RPC_VERSION,
            session_id: sessionId,
            message_id: ++messageCounter,
            response_to: payload.message_id,
            command: "NEW_SESSION",
            status: "OK",
            parameters: {{
              server_name: "Embedded Provider Demo",
              server_version: "1.0.0"
            }}
          }}));
          log(`Session ready: ${{sessionId}}`);
          refreshState();
          return;
        }}

        if (payload.response_to !== undefined) {{
          const waiter = waiters.get(payload.response_to);
          if (waiter) {{
            waiters.delete(payload.response_to);
            waiter(payload);
          }}
        }}
      }}

      window.kiclient = window.kiclient || {{}};
      window.kiclient.postMessage = handleIncoming;

      async function refreshState() {{
        try {{
          const source = await rpc("GET_SOURCE_INFO");
          authenticated = !!source.parameters?.authenticated;
          updateAuthLabel();
        }} catch (err) {{
          log(`State refresh failed: ${{err.message}}`);
        }}
      }}

      async function signIn() {{
        try {{
          await rpc("REMOTE_LOGIN", {{ interactive: true }});
          log("Browser auth started. Complete sign-in in the system browser.");
        }} catch (err) {{
          log(`Sign-in failed: ${{err.message}}`);
        }}
      }}

      async function signOut() {{
        try {{
          await rpc("REMOTE_LOGIN", {{ sign_out: true }});
          authenticated = false;
          accessToken = null;
          updateAuthLabel();
          log("Signed out.");
        }} catch (err) {{
          log(`Sign-out failed: ${{err.message}}`);
        }}
      }}

      async function transfer(partId, command, inlineMode = false) {{
        const part = PARTS.find((item) => item.part_id === partId);
        try {{
          if (inlineMode) {{
            await rpc("DL_COMPONENT", {{
              compression: "NONE",
              library: part.library_name
            }}, INLINE_BUNDLES[partId]);
          }} else {{
            await rpc(command, {{
              part_id: part.part_id,
              display_name: part.display_name,
              symbol_name: part.symbol_name,
              library_name: part.library_name,
              assets: MANIFESTS[partId].assets
            }});
          }}
          log(`${{inlineMode ? "DL_COMPONENT inline" : command}} ok for ${{part.display_name}}`);
        }} catch (err) {{
          log(`${{inlineMode ? "DL_COMPONENT inline" : command}} failed for ${{part.display_name}}: ${{err.message}}`);
        }}
      }}

      function renderParts(filter = "") {{
        const q = filter.trim().toLowerCase();
        const visible = PARTS.filter((part) =>
          !q ||
          part.display_name.toLowerCase().includes(q) ||
          part.mpn.toLowerCase().includes(q) ||
          part.summary.toLowerCase().includes(q)
        );

        partsEl.innerHTML = visible.map((part) => `
          <article class="card">
            <h2>${{part.display_name}}</h2>
            <div class="meta">${{part.manufacturer}} | ${{part.mpn}}</div>
            <p>${{part.summary}}</p>
            <div class="actions">
              <button data-url="${{part.part_id}}">Download URL</button>
              <button data-inline="${{part.part_id}}" class="alt">Download Inline</button>
              <button data-place="${{part.part_id}}" class="ghost">Place</button>
            </div>
          </article>
        `).join("");

        partsEl.querySelectorAll("[data-url]").forEach((button) => {{
          button.addEventListener("click", () => transfer(button.dataset.url, "DL_COMPONENT", false));
        }});
        partsEl.querySelectorAll("[data-inline]").forEach((button) => {{
          button.addEventListener("click", () => transfer(button.dataset.inline, "DL_COMPONENT", true));
        }});
        partsEl.querySelectorAll("[data-place]").forEach((button) => {{
          button.addEventListener("click", () => transfer(button.dataset.place, "PLACE_COMPONENT", false));
        }});
      }}

      document.getElementById("signin").addEventListener("click", signIn);
      document.getElementById("signout").addEventListener("click", signOut);
      document.getElementById("refresh").addEventListener("click", refreshState);
      queryEl.addEventListener("input", () => renderParts(queryEl.value));

      renderParts();
      updateAuthLabel();
    }})();
  </script>
</body>
</html>
"""

    return Response(content=html, media_type="text/html")


@app.get("/.well-known/kicad-remote-provider")
async def provider_metadata(request: Request):
    root = base_url(request)
    return {
        "provider_name": "KiCad Local Test Provider",
        "provider_version": "1.0.0",
        "api_base_url": root,
        "panel_url": f"{root}/app",
        "session_bootstrap_url": f"{root}/session/bootstrap",
        "auth": {
            "type": "oauth2",
            "metadata_url": f"{root}/.well-known/oauth-authorization-server",
            "client_id": "kicad-desktop-test",
            "scopes": ["openid", "parts.read"],
        },
        "capabilities": {
            "web_ui_v1": True,
            "parts_v1": True,
            "direct_downloads_v1": True,
            "inline_payloads_v1": True,
        },
        "max_download_bytes": 1048576,
        "supported_asset_types": ["symbol", "footprint"],
        "parts": {
            "endpoint_template": "/v1/parts/{part_id}",
        },
        "documentation_url": f"{root}/docs",
        "allow_insecure_localhost": True,
    }


@app.post("/session/bootstrap")
async def session_bootstrap(
    request: Request,
    access_token: str = Form(...),
    next_url: str = Form("/app"),
):
    if access_token != "local-test-token":
        raise HTTPException(status_code=401, detail="Invalid access token")

    root = base_url(request)
    if not next_url.startswith(root):
        raise HTTPException(status_code=400, detail="next_url must stay on provider origin")

    response = RedirectResponse(url=next_url, status_code=303)
    response.set_cookie(
        "kicad_provider_session",
        "active",
        httponly=True,
        samesite="lax",
    )
    return response


@app.get("/.well-known/oauth-authorization-server")
async def oauth_metadata(request: Request):
    root = base_url(request)
    return {
        "issuer": root,
        "authorization_endpoint": f"{root}/oauth/authorize",
        "token_endpoint": f"{root}/oauth/token",
        "revocation_endpoint": f"{root}/oauth/revoke",
    }


@app.get("/oauth/authorize")
async def oauth_authorize(redirect_uri: str, state: str):
    separator = "&" if "?" in redirect_uri else "?"
    return RedirectResponse(
        url=f"{redirect_uri}{separator}code=local-test-code&state={state}"
    )


@app.post("/oauth/token")
async def oauth_token():
    return {
        "token_type": "Bearer",
        "access_token": "local-test-token",
        "refresh_token": "local-refresh-token",
        "expires_in": 3600,
        "scope": "openid parts.read",
    }


@app.post("/oauth/revoke")
async def oauth_revoke():
    return Response(status_code=200)


@app.get("/v1/parts/{part_id}")
async def remote_provider_manifest(request: Request, part_id: str):
    return manifest_payload(request, part_id)


@app.get("/downloads/{asset_name}")
async def download_asset(asset_name: str):
    if asset_name not in download_payloads:
        raise HTTPException(status_code=404, detail=f"Asset {asset_name} not found")

    body, content_type = download_payloads[asset_name]
    return PlainTextResponse(body, media_type=content_type)


@app.get("/v1/")
async def get_endpoints():
    return {"categories": "", "parts": ""}


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


def main():
    parser = argparse.ArgumentParser(description="Run the KiCad HTTP/provider test server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", default=8000, type=int)
    args = parser.parse_args()

    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
