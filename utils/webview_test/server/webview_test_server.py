#!/usr/bin/env python3
"""
Development server for exercising the schematic remote symbol webview.

The server loads a JSON catalogue of parts, renders a small gallery UI, and
exposes buttons that stream each part's assets (footprint, models, etc.) to
KiCad via the WebView bridge. Each asset is compressed with zstd and then
base64-encoded before being embedded in the served HTML page.
"""

from __future__ import annotations

import argparse
import base64
import http.server
import hashlib
import json
import logging
import mimetypes
import socketserver
import urllib.parse
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence

import zstandard
import re


_DEFAULT_PORT = 8080
_DEFAULT_CONFIG = (Path(__file__).parent / "data" / "parts.json").resolve()
_ZSTD_COMPRESSOR = zstandard.ZstdCompressor(level=10)


def _content_type_for_command(command: str) -> str:
  mapping = {
    "DL_SYMBOL": "KICAD_SYMBOL_V1",
    "DL_FOOTPRINT": "KICAD_FOOTPRINT_V1",
    "DL_SPICE": "KICAD_SPICE_MODEL_V1",
    "DL_3DMODEL": "KICAD_3D_MODEL_STEP",
  }

  return mapping.get(command.upper(), "UNKNOWN")


_GENERIC_IMAGE_DATA_URI = (
    "data:image/svg+xml;base64,"
    + base64.b64encode(
        b"""<svg xmlns='http://www.w3.org/2000/svg' width='160' height='100' viewBox='0 0 160 100'>
  <defs>
    <linearGradient id='g' x1='0%' y1='0%' x2='100%' y2='100%'>
      <stop offset='0%' stop-color='#e0e0e0'/>
      <stop offset='100%' stop-color='#bdbdbd'/>
    </linearGradient>
  </defs>
  <rect width='160' height='100' rx='12' fill='url(#g)'/>
  <path d='M25 75 L60 25 L100 25 L135 75 Z' fill='none' stroke='#616161' stroke-width='4' stroke-linecap='round' stroke-linejoin='round'/>
  <circle cx='80' cy='50' r='10' fill='none' stroke='#757575' stroke-width='4'/>
  <text x='80' y='90' font-family='Arial, sans-serif' font-size='18' fill='#424242' text-anchor='middle'>No Image</text>
</svg>"""
    ).decode("ascii")
)
_HTML_TEMPLATE = """\
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>KiCad WebView Test Harness</title>
    <style>
      :root {{
        color-scheme: light;
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Arial, sans-serif;
        background: #e7eef8;
      }}

      body {{
        margin: 0;
        padding: 2rem;
        background: #e7eef8;
        color: #1f2933;
      }}

      .container {{
        max-width: 960px;
        margin: 0 auto;
        background: #fff;
        padding: 2rem;
        border-radius: 16px;
        box-shadow: 0 8px 30px rgba(15, 23, 42, 0.14);
      }}

      h1 {{
        margin-top: 0;
        margin-bottom: 0.5rem;
        font-size: 2rem;
      }}

      .intro {{
        margin-bottom: 2rem;
        color: #4b5563;
      }}

      .parts-grid {{
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
        gap: 1rem;
      }}

      .login-panel {{
        display: flex;
        align-items: center;
        gap: 1rem;
        margin-bottom: 1.5rem;
      }}

      .login-status {{
        font-weight: 600;
        color: #1f2933;
      }}

      .part-card {{
        background: linear-gradient(135deg, #f9fafb, #f3f4f6);
        border-radius: 16px;
        padding: 1.5rem;
        box-shadow: inset 0 0 0 1px rgba(148, 163, 184, 0.25);
        display: flex;
        flex-direction: column;
        gap: 1rem;
      }}

      .part-card img {{
        width: 100%;
        height: 140px;
        object-fit: contain;
        border-radius: 12px;
        background: #fff;
        box-shadow: inset 0 0 0 1px rgba(99, 102, 241, 0.15);
      }}

      .part-name {{
        font-size: 1.2rem;
        margin: 0;
      }}

      .asset-list {{
        margin: 0;
        padding-left: 1.2rem;
        color: #4b5563;
        font-size: 0.95rem;
      }}

      button {{
        font-size: 1rem;
        padding: 0.75rem 1.25rem;
        border-radius: 999px;
        border: none;
        cursor: pointer;
        background: #2563eb;
        color: #fff;
        transition: background 0.15s ease, transform 0.15s ease;
      }}

      button:hover:enabled {{
        background: #1d4ed8;
        transform: translateY(-1px);
      }}

      button:disabled {{
        opacity: 0.6;
        cursor: not-allowed;
      }}

      pre {{
        background: #0f172a;
        color: #d1d5db;
        padding: 1rem;
        border-radius: 12px;
        overflow-x: auto;
        line-height: 1.5;
        min-height: 120px;
        font-size: 0.9rem;
      }}
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Remote Symbol Demo Browser</h1>
      <p class="intro">
        Select a part to stream its assets to KiCad. Each asset is zstd-compressed,
        base64-encoded, and delivered in sequence with acknowledgement (ACK) handling.
      </p>
      <section class="login-panel">
        <button id="login-button" type="button">Log in via Browser</button>
        <span class="login-status" id="login-status">Not logged in</span>
      </section>
      <section class="parts-grid" id="parts-root"></section>
      <section>
        <h2>Transfer log</h2>
        <pre id="status-log">No transfers yet.</pre>
      </section>
    </div>
    <script>
      (function () {{
        const RPC_VERSION = 1;
        const PART_DEFINITIONS = Object.freeze({parts_json});
        const GENERIC_IMAGE = {generic_image};
        const STATUS_ELEMENT = document.getElementById("status-log");
        const PARTS_ROOT = document.getElementById("parts-root");
        const LOGIN_BUTTON = document.getElementById("login-button");
        const LOGIN_STATUS = document.getElementById("login-status");
        const LOGIN_ENDPOINT = new URL("/login", window.location.origin).toString();
        const RESPONSE_WAITERS = new Map();
        const BACKOFF_MS = [500, 1500, 3000];
        let messageCounter = 0;
        let sessionId = null;
        let loginInFlight = false;

        function postToKiCad(payload) {{
          if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.kicad) {{
            window.webkit.messageHandlers.kicad.postMessage(payload);
            return true;
          }}

          if (window.chrome && window.chrome.webview && window.chrome.webview.postMessage) {{
            window.chrome.webview.postMessage(payload);
            return true;
          }}

          if (window.external && typeof window.external.invoke === "function") {{
            try {{
              window.external.invoke(payload);
              return true;
            }} catch (err) {{
              console.error("external.invoke failed", err);
            }}
          }}

          console.warn("No KiCad bridge found; simulated delivery only.");
          return false;
        }}

        function appendLog(message) {{
          const timestamp = new Date().toLocaleTimeString();
          const entry = `[${{timestamp}}] ${{message}}`;
          STATUS_ELEMENT.textContent =
            STATUS_ELEMENT.textContent === "No transfers yet."
              ? entry
              : `${{entry}}\\n${{STATUS_ELEMENT.textContent}}`;
        }}

        function updateLoginStatus(message) {{
          if (LOGIN_STATUS) {{
            LOGIN_STATUS.textContent = message;
          }}
        }}

        function generateFallbackUuid() {{
          return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, (c) => {{
            const r = (Math.random() * 16) | 0;
            const v = c === "x" ? r : (r & 0x3) | 0x8;
            return v.toString(16);
          }});
        }}

        async function mockAuthentication(port) {{
          const userId =
            (window.crypto && window.crypto.randomUUID && window.crypto.randomUUID()) ||
            generateFallbackUuid();

          const callbackUrl = `http://localhost:${{port}}/login?user_id=${{encodeURIComponent(userId)}}`;

          try {{
            await fetch(callbackUrl, {{ method: "GET", mode: "cors" }});
          }} catch (err) {{
            console.warn("Login callback fetch failed", err);
          }}

          return userId;
        }}

        async function requestLogin(button) {{
          if (!sessionId) {{
            appendLog("Cannot log in until KiCad establishes a session.");
            return;
          }}

          if (loginInFlight) {{
            return;
          }}

          loginInFlight = true;
          if (button) {{
            button.disabled = true;
          }}

          updateLoginStatus("Waiting for browser login...");
          appendLog("Issuing REMOTE_LOGIN command...");

          try {{
            const response = await sendRpcCommand("REMOTE_LOGIN", {{ url: LOGIN_ENDPOINT }});
            const port = response?.parameters?.port ?? response?.port;
            const numericPort = Number(port);

            if (!Number.isFinite(numericPort) || numericPort <= 0) {{
              throw new Error("KiCad did not provide a callback port.");
            }}

            appendLog(`Please complete login in the external browser (port ${{numericPort}})...`);
            updateLoginStatus("Waiting for external login...");

          }} catch (err) {{
            appendLog(`Remote login failed: ${{err.message}}`);
            updateLoginStatus("Login failed");
            loginInFlight = false;
            if (button) {{
              button.disabled = false;
            }}
          }}
        }}

        function installKiClientBridge() {{
          const existing = window.kiclient || {{}};
          const previousPost = typeof existing.postMessage === "function" ? existing.postMessage.bind(existing) : null;

          existing.postMessage = function (incoming) {{
            handleIncomingMessage(incoming);
            if (previousPost) {{
              previousPost(incoming);
            }}
          }};

          window.kiclient = existing;
        }}

        function handleIncomingMessage(incoming) {{
          let payload = incoming;
          if (typeof payload === "string") {{
            try {{
              payload = JSON.parse(payload);
            }} catch (err) {{
              console.warn("Failed to parse KiCad response", err, incoming);
              return;
            }}
          }}

          if (!payload || typeof payload !== "object") {{
            return;
          }}

          if (payload.command === "NEW_SESSION" && payload.response_to === undefined) {{
            sessionId = payload.session_id;
            messageCounter = payload.message_id || 0;
            RESPONSE_WAITERS.clear();
            appendLog(`Session established: ${{payload.session_id}}`);

            if (payload.user_id) {{
              updateLoginStatus(`Logged in as ${{payload.user_id}}`);
            }}

            sendNewSessionResponse(payload);
            return;
          }}

          if (payload.response_to !== undefined) {{
            const key = String(payload.response_to);
            const waiter = RESPONSE_WAITERS.get(key);
            if (waiter) {{
              RESPONSE_WAITERS.delete(key);
              waiter.resolve(payload);
            }} else {{
              appendLog(`Unexpected response for message ${{payload.response_to}}.`);
            }}
            return;
          }}

          appendLog(`KiCad -> ${{JSON.stringify(payload)}}`);
        }}

        function sendNewSessionResponse(request) {{
          if (!request || typeof request.message_id !== "number" || !sessionId) {{
            return;
          }}

          const payload = {{
            version: RPC_VERSION,
            session_id: sessionId,
            message_id: ++messageCounter,
            response_to: request.message_id,
            command: "NEW_SESSION",
            status: "OK",
            parameters: {{
              server_name: "KiCad WebView Test Harness",
              server_version: "1.0.0"
            }}
          }};

          postToKiCad(JSON.stringify(payload));
        }}

        function waitForResponse(messageId, timeoutMs) {{
          return new Promise((resolve, reject) => {{
            const key = String(messageId);
            const timer = window.setTimeout(() => {{
              RESPONSE_WAITERS.delete(key);
              reject(new Error("Response timeout"));
            }}, timeoutMs);

            RESPONSE_WAITERS.set(key, {{
              resolve: (payload) => {{
                window.clearTimeout(timer);
                RESPONSE_WAITERS.delete(key);
                resolve(payload);
              }},
            }});
          }});
        }}

        function buildRpcEnvelope(command, parameters = {{}}, data = "") {{
          if (!sessionId) {{
            throw new Error("Session has not been established yet.");
          }}

          return {{
            version: RPC_VERSION,
            session_id: sessionId,
            message_id: ++messageCounter,
            command,
            parameters: JSON.parse(JSON.stringify(parameters)),
            data,
          }};
        }}

        async function sendRpcCommand(command, parameters = {{}}, data = "") {{
          const message = buildRpcEnvelope(command, parameters, data);
          const delivered = postToKiCad(JSON.stringify(message));

          if (!delivered) {{
            appendLog(`Could not deliver ${{command}} (message ${{message.message_id}}) to KiCad.`);
          }}

          const response = await waitForResponse(message.message_id, 4000);

          if (response.status && response.status !== "OK") {{
            const err = response.error_message || "Remote endpoint reported an error.";
            throw new Error(err);
          }}

          return response;
        }}

        async function sendAsset(asset) {{
          await sendRpcCommand(asset.command, asset.parameters, asset.data);
        }}

        async function sendWithRetries(asset) {{
          for (let attempt = 0; attempt < BACKOFF_MS.length; attempt += 1) {{
            try {{
              await sendAsset(asset);
              appendLog(`Transferred ${{asset.label}} (file: ${{asset.filename}})`);
              return;
            }} catch (err) {{
              appendLog(`Attempt ${{attempt + 1}} failed for ${{asset.label}}: ${{err.message}}`);

              if (attempt === BACKOFF_MS.length - 1) {{
                throw err;
              }}

              await new Promise((resolve) => window.setTimeout(resolve, BACKOFF_MS[attempt]));
            }}
          }}
        }}

        function formatBytes(bytes) {{
          if (!Number.isFinite(bytes)) {{
            return "";
          }}
          if (bytes < 1024) {{
            return `${{bytes}} B`;
          }}
          const kb = bytes / 1024;
          if (kb < 1024) {{
            return `${{kb.toFixed(1)}} KiB`;
          }}
          return `${{(kb / 1024).toFixed(2)}} MiB`;
        }}

        function renderParts() {{
          if (!PART_DEFINITIONS.length) {{
            PARTS_ROOT.innerHTML =
              '<p>No parts defined in the configuration file. Update data/parts.json to add entries.</p>';
            return;
          }}

          PART_DEFINITIONS.forEach((part) => {{
            const card = document.createElement("article");
            card.className = "part-card";

            const img = document.createElement("img");
            img.src = part.image || GENERIC_IMAGE;
            img.alt = `Preview for ${{part.name}}`;
            card.appendChild(img);

            const nameEl = document.createElement("h2");
            nameEl.className = "part-name";
            nameEl.textContent = part.name;
            card.appendChild(nameEl);

            const assetsList = document.createElement("ul");
            assetsList.className = "asset-list";

            part.assets.forEach((asset) => {{
              const item = document.createElement("li");
              const size = formatBytes(asset.size_bytes);
              item.textContent = size
                ? `${{asset.label}} · ${{asset.filename}} · ${{size}}`
                : `${{asset.label}} · ${{asset.filename}}`;
              assetsList.appendChild(item);
            }});

            card.appendChild(assetsList);

            const button = document.createElement("button");
            button.type = "button";
            button.textContent = `Send ${{part.name}} to KiCad`;
            button.addEventListener("click", () => handlePartTransfer(part, button));
            card.appendChild(button);

            PARTS_ROOT.appendChild(card);
          }});
        }}

        async function handlePartTransfer(part, button) {{
          if (!part.assets || !part.assets.length) {{
            appendLog(`No assets to transfer for ${{part.name}}.`);
            return;
          }}

          if (!sessionId) {{
            appendLog("Cannot transfer assets until KiCad establishes a session.");
            return;
          }}

          if (button.disabled) {{
            return;
          }}

          button.disabled = true;
          appendLog(`Beginning transfer for ${{part.name}}...`);

          try {{
            for (const asset of part.assets) {{
              await sendWithRetries(asset);
            }}
            appendLog(`Completed transfer for ${{part.name}}.`);
          }} catch (err) {{
            appendLog(`Transfer failed for ${{part.name}}: ${{err.message}}`);
          }} finally {{
            button.disabled = false;
          }}
        }}

        if (LOGIN_BUTTON) {{
          LOGIN_BUTTON.addEventListener("click", () => requestLogin(LOGIN_BUTTON));
        }}

        installKiClientBridge();
        renderParts();
      }})();
    </script>
  </body>
</html>
"""


_LOGIN_HTML_TEMPLATE = """\\
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>KiCad Login</title>
    <style>
      :root {
        color-scheme: light;
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Arial, sans-serif;
        background: #e7eef8;
      }
      body {
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
      }
      .login-card {
        background: #fff;
        padding: 2rem;
        border-radius: 16px;
        box-shadow: 0 8px 30px rgba(15, 23, 42, 0.14);
        width: 100%;
        max-width: 400px;
      }
      h1 { margin-top: 0; text-align: center; color: #1f2933; }
      .form-group { margin-bottom: 1rem; }
      label { display: block; margin-bottom: 0.5rem; font-weight: 600; color: #4b5563; }
      input {
        width: 100%;
        padding: 0.75rem;
        border: 1px solid #cbd5e1;
        border-radius: 8px;
        box-sizing: border-box;
        font-size: 1rem;
      }
      button {
        width: 100%;
        padding: 0.75rem;
        background: #2563eb;
        color: white;
        border: none;
        border-radius: 8px;
        font-size: 1rem;
        cursor: pointer;
        font-weight: 600;
        margin-top: 1rem;
      }
      button:hover { background: #1d4ed8; }
      #status { margin-top: 1rem; text-align: center; min-height: 1.5em; }
    </style>
  </head>
  <body>
    <div class="login-card">
      <h1>Sign In</h1>
      <form id="login-form">
        <div class="form-group">
          <label for="username">Username</label>
          <input type="text" id="username" name="username" required>
        </div>
        <div class="form-group">
          <label for="password">Password</label>
          <input type="password" id="password" name="password" required>
        </div>
        <button type="submit">Login</button>
      </form>
      <div id="status"></div>
    </div>
    <script>
      document.getElementById('login-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const params = new URLSearchParams(window.location.search);
        const port = params.get('port');
        const status = document.getElementById('status');

        if (!port) {
          status.textContent = "Error: No port provided in URL.";
          status.style.color = "#ef4444";
          return;
        }

        const username = document.getElementById('username').value;
        const uuid = (window.crypto && window.crypto.randomUUID) ? window.crypto.randomUUID() : "legacy-uuid-" + Math.random();

        // Construct the callback URL
        const callbackUrl = `http://localhost:${port}/login?user_id=${encodeURIComponent(uuid)}&username=${encodeURIComponent(username)}`;

        try {
          status.textContent = "Authenticating...";
          status.style.color = "#2563eb";

          await fetch(callbackUrl, { method: "GET", mode: "cors" });

          status.textContent = "Login successful! Redirecting...";
          status.style.color = "#16a34a";

          setTimeout(() => {
             window.location.href = "/";
          }, 1000);

        } catch (err) {
          console.error(err);
          status.textContent = "Login failed: " + err.message;
          status.style.color = "#ef4444";
        }
      });
    </script>
  </body>
</html>
"""


@dataclass(frozen=True)
class AssetRecord:
    """In-memory representation of a downloadable asset."""

    command: str
    label: str
    filename: str
    data: str
    parameters: Dict[str, Any]
    size_bytes: int

    def to_dict(self) -> Dict[str, Any]:
        return {
            "command": self.command,
            "label": self.label,
            "filename": self.filename,
            "data": self.data,
            "parameters": self.parameters,
            "size_bytes": self.size_bytes,
        }


@dataclass(frozen=True)
class PartRecord:
    """Renderable part entry."""

    name: str
    image: str
    assets: Sequence[AssetRecord]

    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "image": self.image,
            "assets": [asset.to_dict() for asset in self.assets],
        }


class _ServerState:
    """Holds parsed configuration and rendered HTML."""

    def __init__(self, config_path: Path) -> None:
        self.config_path = config_path
        self._parts = self._load_parts()
        self._page_bytes = self._render_page()
        self._login_page_bytes = _LOGIN_HTML_TEMPLATE.encode("utf-8")

    @property
    def page(self) -> bytes:
        return self._page_bytes

    @property
    def login_page(self) -> bytes:
        return self._login_page_bytes

    def _render_page(self) -> bytes:
        parts_payload = [part.to_dict() for part in self._parts]
        parts_json = json.dumps(parts_payload, separators=(",", ":")).replace("</", "<\\/")
        html = _HTML_TEMPLATE.format(parts_json=parts_json, generic_image=json.dumps(_GENERIC_IMAGE_DATA_URI))
        return html.encode("utf-8")

    def _load_parts(self) -> List[PartRecord]:
        if not self.config_path.is_file():
            raise FileNotFoundError(f"Config file '{self.config_path}' does not exist.")

        data = json.loads(self.config_path.read_text(encoding="utf-8"))
        if isinstance(data, dict):
            parts_data = data.get("Parts") or data.get("parts")
        elif isinstance(data, list):
            parts_data = data
        else:
            raise ValueError("Configuration must be a list or contain a 'Parts' array.")

        if not isinstance(parts_data, list):
            raise ValueError("Invalid configuration: expected an array of part objects.")

        parts: List[PartRecord] = []
        for index, entry in enumerate(parts_data, start=1):
            if not isinstance(entry, dict):
                raise ValueError(f"Part entry #{index} is not an object.")

            part = self._build_part(entry, index)
            parts.append(part)

        logging.info("Loaded %d part(s) from %s", len(parts), self.config_path)
        return parts

    def _build_part(self, entry: Dict[str, Any], index: int) -> PartRecord:
        name = self._get_field(entry, "Name")
        if not name:
            raise ValueError(f"Part #{index} is missing the 'Name' field.")

        symbol_path = self._get_field(entry, "Symbol")
        if not symbol_path:
            raise ValueError(f"Part '{name}' is missing the required 'Symbol' field.")

        assets: List[AssetRecord] = []
        component_entries: List[Dict[str, Any]] = []
        library_name = Path(symbol_path).stem

        field_plan = [
          ("Footprint", "DL_FOOTPRINT", "SAVE", "footprint"),
          ("SPICE Model", "DL_SPICE", "SAVE", None),
          ("3D Model", "DL_3DMODEL", "SAVE", "3dmodel"),
          ("Symbol", "DL_SYMBOL", "PLACE", "symbol"),
        ]

        for label, command, mode, component_type in field_plan:
          path_value = self._get_field(entry, label)
          if not path_value:
            continue

          file_path = self._resolve_asset_path(path_value)
          raw_bytes = file_path.read_bytes()
          encoded = _encode_bytes(raw_bytes)
          extracted_name = _extract_asset_name(file_path, command)

          assets.append(
            AssetRecord(
              command=command,
              label=label,
              filename=file_path.name,
              data=encoded,
              parameters={
                "mode": mode,
                "compression": "ZSTD",
                "content_type": _content_type_for_command(command),
                "library": library_name,
                "name": extracted_name,
              },
              size_bytes=len(raw_bytes),
            )
          )

          if component_type:
            if component_type == "3dmodel":
              component_name = file_path.name
            else:
              component_name = extracted_name or file_path.stem

            component_entries.append(
              {
                "type": component_type,
                "name": component_name,
                "checksum": _hash_bytes(raw_bytes),
                "compression": "ZSTD",
                "content": encoded,
              }
            )

        if not assets:
          raise ValueError(f"Part '{name}' did not produce any downloadable assets.")

        if component_entries:
          component_json = json.dumps(component_entries, separators=(",", ":")).encode("utf-8")
          component_bundle = _encode_bytes(component_json)
          assets.insert(
            0,
            AssetRecord(
              command="DL_COMPONENT",
              label="Component Bundle",
              filename=f"{library_name}_component_bundle.json",
              data=component_bundle,
              parameters={
                "compression": "ZSTD",
                "library": library_name,
              },
              size_bytes=len(component_json),
            ),
          )

        image_path_value = self._get_field(entry, "Image")
        image_data = self._encode_image(image_path_value)

        return PartRecord(name=name, image=image_data, assets=assets)

    def _get_field(self, entry: Dict[str, Any], key: str) -> Optional[str]:
        direct = entry.get(key)
        if direct:
            return str(direct)

        lowered = key.lower()
        for candidate in (lowered, key.replace(" ", ""), key.replace(" ", "_").lower()):
            value = entry.get(candidate)
            if value:
                return str(value)

        return None

    def _resolve_asset_path(self, relative_path: str) -> Path:
        base = self.config_path.parent
        candidate = (base / relative_path).resolve()

        try:
            candidate.relative_to(base)
        except ValueError as err:
            raise ValueError(f"Asset path '{relative_path}' must stay within {base}") from err

        if not candidate.is_file():
            raise FileNotFoundError(f"Asset '{relative_path}' does not exist under {base}")

        return candidate

    def _encode_image(self, relative_path: Optional[str]) -> str:
        if not relative_path:
            return _GENERIC_IMAGE_DATA_URI

        image_path = self._resolve_asset_path(relative_path)
        mime = mimetypes.guess_type(image_path.name)[0] or "application/octet-stream"
        encoded = base64.b64encode(image_path.read_bytes()).decode("ascii")
        return f"data:{mime};base64,{encoded}"


def _encode_bytes(payload: bytes) -> str:
  compressed = _ZSTD_COMPRESSOR.compress(payload)
  return base64.b64encode(compressed).decode("ascii")


def _encode_file(file_path: Path) -> str:
  """Read, zstd-compress, and base64-encode a file."""

  return _encode_bytes(file_path.read_bytes())


def _hash_bytes(payload: bytes) -> str:
  return hashlib.sha256(payload).hexdigest()


def _extract_asset_name(file_path: Path, command: str) -> str:
  """Try to extract the internal symbol/footprint name from the file.

  Falls back to the filename stem when extraction fails or for binary
  assets.
  """
  try:
    text = file_path.read_text(encoding="utf-8", errors="ignore")
  except Exception:
    return file_path.stem

  cmd = (command or "").upper()

  # KiCad symbol libraries use s-expressions: (symbol "NAME" ...)
  if cmd == "DL_SYMBOL":
    m = re.search(r'\(symbol\s+"([^"]+)"', text)
    if m:
      return m.group(1)
    # Legacy library format (DEF ...)
    m = re.search(r'^\s*DEF\s+(\S+)', text, flags=re.MULTILINE)
    if m:
      return m.group(1)

  # Footprints: (footprint "NAME" ...) in modern .kicad_mod files
  if cmd == "DL_FOOTPRINT":
    m = re.search(r'\(footprint\s+"([^"]+)"', text)
    if m:
      return m.group(1)
    # Another possible older token is 'module <name>'
    m = re.search(r'^\s*module\s+([^\s(]+)', text, flags=re.MULTILINE)
    if m:
      return m.group(1)

  return file_path.stem


def _build_handler(state: _ServerState) -> type[http.server.BaseHTTPRequestHandler]:
    class _WebViewTestHandler(http.server.BaseHTTPRequestHandler):
        def do_GET(self) -> None:
            parsed = urllib.parse.urlparse(self.path)
            path = parsed.path

            if path in ("/", "/index.html"):
                content = state.page
            elif path == "/login":
                content = state.login_page
            else:
                self.send_error(404)
                return

            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            self.wfile.write(content)

        def log_message(self, format: str, *args: Any) -> None:  # noqa: A003
            logging.info("%s - %s", self.client_address[0], format % args)

    return _WebViewTestHandler


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Serve the KiCad WebView test page.")
    parser.add_argument(
        "--port",
        type=int,
        default=_DEFAULT_PORT,
        help=f"Local TCP port to bind (default: {_DEFAULT_PORT})",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=_DEFAULT_CONFIG,
        help=f"Path to the parts configuration JSON file (default: {_DEFAULT_CONFIG})",
    )

    return parser.parse_args()


def main() -> None:
    args = _parse_args()
    config_path = args.config.resolve()
    state = _ServerState(config_path=config_path)

    handler_cls = _build_handler(state)
    socketserver.TCPServer.allow_reuse_address = True

    with socketserver.TCPServer(("", args.port), handler_cls) as httpd:
        logging.info(
            "Serving WebView test page on http://localhost:%d/ using config %s",
            args.port,
            config_path,
        )
        logging.info("Press Ctrl+C to stop.")

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            logging.info("Stopping server.")


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    main()
