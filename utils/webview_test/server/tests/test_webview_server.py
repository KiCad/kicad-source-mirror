"""
Tests for the KiCad WebView test server.

Run with:  pytest utils/webview_test/server/tests/
"""

from __future__ import annotations

import base64
import json
import socketserver
import sys
import threading
import urllib.error
import urllib.request
from pathlib import Path
from typing import Generator

import pytest
import zstandard

# Allow importing the server module from its parent directory.
sys.path.insert(0, str(Path(__file__).parent.parent))

from webview_test_server import (
    _ServerState,
    _build_handler,
    _content_type_for_command,
    _encode_bytes,
    _extract_asset_name,
)

_DATA_DIR = Path(__file__).parent.parent / "data"
_CONFIG_FILE = _DATA_DIR / "parts.json"


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------


@pytest.fixture(scope="module")
def server_state() -> _ServerState:
    return _ServerState(config_path=_CONFIG_FILE)


class _ReuseAddrServer(socketserver.TCPServer):
    allow_reuse_address = True


@pytest.fixture(scope="module")
def live_server() -> Generator[int, None, None]:
    """Start a live HTTP server on an OS-assigned port and yield that port."""
    state = _ServerState(config_path=_CONFIG_FILE)
    handler_cls = _build_handler(state)

    with _ReuseAddrServer(("127.0.0.1", 0), handler_cls) as httpd:
        port = httpd.server_address[1]
        thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        thread.start()
        yield port
        httpd.shutdown()


# ---------------------------------------------------------------------------
# Provider metadata
# ---------------------------------------------------------------------------


_BASE_URL = "http://localhost:8080"


class TestProviderMetadata:
    def test_returns_bytes(self, server_state: _ServerState) -> None:
        result = server_state.build_provider_metadata(_BASE_URL)
        assert isinstance(result, bytes)
        assert len(result) > 0

    def test_valid_json(self, server_state: _ServerState) -> None:
        metadata = json.loads(server_state.build_provider_metadata(_BASE_URL))
        assert isinstance(metadata, dict)

    def test_required_fields_present(self, server_state: _ServerState) -> None:
        required = [
            "provider_name",
            "provider_version",
            "api_base_url",
            "panel_url",
            "auth",
            "capabilities",
            "max_download_bytes",
            "supported_asset_types",
        ]
        metadata = json.loads(server_state.build_provider_metadata(_BASE_URL))

        for field in required:
            assert field in metadata, f"Required field '{field}' missing from provider metadata"

    def test_capabilities(self, server_state: _ServerState) -> None:
        metadata = json.loads(server_state.build_provider_metadata(_BASE_URL))
        caps = metadata["capabilities"]
        assert caps.get("web_ui_v1") is True, "web_ui_v1 must be true"
        assert caps.get("inline_payloads_v1") is True, "inline_payloads_v1 must be true"

    def test_auth_type_none(self, server_state: _ServerState) -> None:
        metadata = json.loads(server_state.build_provider_metadata(_BASE_URL))
        assert metadata["auth"]["type"] == "none"

    def test_allow_insecure_localhost(self, server_state: _ServerState) -> None:
        metadata = json.loads(server_state.build_provider_metadata(_BASE_URL))
        assert metadata.get("allow_insecure_localhost") is True

    def test_supported_asset_types(self, server_state: _ServerState) -> None:
        metadata = json.loads(server_state.build_provider_metadata(_BASE_URL))
        types = metadata["supported_asset_types"]
        assert len(types) >= 1
        for expected in ("symbol", "footprint"):
            assert expected in types

    def test_max_download_bytes_positive(self, server_state: _ServerState) -> None:
        metadata = json.loads(server_state.build_provider_metadata(_BASE_URL))
        assert metadata["max_download_bytes"] > 0

    def test_base_url_reflected_in_urls(self, server_state: _ServerState) -> None:
        metadata = json.loads(server_state.build_provider_metadata("http://localhost:12345"))
        assert "12345" in metadata["api_base_url"]
        assert "12345" in metadata["panel_url"]


# ---------------------------------------------------------------------------
# Asset encoding
# ---------------------------------------------------------------------------


class TestEncodeBytes:
    def test_roundtrip_text(self) -> None:
        original = b"Hello, KiCad!"
        encoded = _encode_bytes(original)
        compressed = base64.b64decode(encoded)
        decompressor = zstandard.ZstdDecompressor()
        assert decompressor.decompress(compressed) == original

    def test_roundtrip_binary(self) -> None:
        original = bytes(range(256))
        encoded = _encode_bytes(original)
        compressed = base64.b64decode(encoded)
        decompressor = zstandard.ZstdDecompressor()
        assert decompressor.decompress(compressed) == original

    def test_produces_valid_base64(self) -> None:
        encoded = _encode_bytes(b"test payload")
        base64.b64decode(encoded)  # must not raise

    def test_empty_input(self) -> None:
        encoded = _encode_bytes(b"")
        compressed = base64.b64decode(encoded)
        decompressor = zstandard.ZstdDecompressor()
        assert decompressor.decompress(compressed) == b""

    def test_compression_reduces_repetitive_data(self) -> None:
        original = b"AAAA" * 1000
        encoded = _encode_bytes(original)
        assert len(base64.b64decode(encoded)) < len(original)


# ---------------------------------------------------------------------------
# Asset name extraction
# ---------------------------------------------------------------------------


class TestExtractAssetName:
    def test_kicad_symbol(self, tmp_path: Path) -> None:
        sym = tmp_path / "r.kicad_sym"
        sym.write_text('(kicad_symbol_lib\n  (symbol "R"\n  )\n)')
        assert _extract_asset_name(sym, "DL_SYMBOL") == "R"

    def test_kicad_symbol_quoted_name_with_spaces(self, tmp_path: Path) -> None:
        sym = tmp_path / "cap.kicad_sym"
        sym.write_text('(kicad_symbol_lib\n  (symbol "C_Small"\n  )\n)')
        assert _extract_asset_name(sym, "DL_SYMBOL") == "C_Small"

    def test_kicad_footprint(self, tmp_path: Path) -> None:
        fp = tmp_path / "r.kicad_mod"
        fp.write_text('(footprint "R_0603_1608Metric" (layer "F.Cu"))')
        assert _extract_asset_name(fp, "DL_FOOTPRINT") == "R_0603_1608Metric"

    def test_symbol_fallback_to_stem(self, tmp_path: Path) -> None:
        sym = tmp_path / "my_symbol.kicad_sym"
        sym.write_text("(kicad_symbol_lib)")
        assert _extract_asset_name(sym, "DL_SYMBOL") == "my_symbol"

    def test_binary_file_fallback(self, tmp_path: Path) -> None:
        step = tmp_path / "R_0603.step"
        step.write_bytes(b"\x00\x01\x02\x03ISO-10303-21")
        assert _extract_asset_name(step, "DL_3DMODEL") == "R_0603"

    def test_spice_falls_back_to_stem(self, tmp_path: Path) -> None:
        cir = tmp_path / "resistor.cir"
        cir.write_text("* SPICE model\n.SUBCKT R A B\n.ENDS\n")
        assert _extract_asset_name(cir, "DL_SPICE") == "resistor"


# ---------------------------------------------------------------------------
# Content type mapping
# ---------------------------------------------------------------------------


class TestContentTypeForCommand:
    def test_dl_symbol(self) -> None:
        assert _content_type_for_command("DL_SYMBOL") == "KICAD_SYMBOL_V1"

    def test_dl_footprint(self) -> None:
        assert _content_type_for_command("DL_FOOTPRINT") == "KICAD_FOOTPRINT_V1"

    def test_dl_spice(self) -> None:
        assert _content_type_for_command("DL_SPICE") == "KICAD_SPICE_MODEL_V1"

    def test_dl_3dmodel(self) -> None:
        assert _content_type_for_command("DL_3DMODEL") == "KICAD_3D_MODEL_STEP"

    def test_case_insensitive(self) -> None:
        assert _content_type_for_command("dl_symbol") == "KICAD_SYMBOL_V1"

    def test_unknown_command(self) -> None:
        assert _content_type_for_command("UNKNOWN_CMD") == "UNKNOWN"


# ---------------------------------------------------------------------------
# Parts loading
# ---------------------------------------------------------------------------


class TestPartsLoading:
    def test_loads_expected_parts(self, server_state: _ServerState) -> None:
        page_html = server_state.page.decode("utf-8")
        assert "Resistor 0603 1k" in page_html
        assert "Capacitor 0603 100nF" in page_html

    def test_page_contains_session_logic(self, server_state: _ServerState) -> None:
        page_html = server_state.page.decode("utf-8")
        assert "NEW_SESSION" in page_html
        assert "kiclient" in page_html

    def test_page_contains_dl_component(self, server_state: _ServerState) -> None:
        page_html = server_state.page.decode("utf-8")
        assert "DL_COMPONENT" in page_html

    def test_page_is_valid_html(self, server_state: _ServerState) -> None:
        page_html = server_state.page.decode("utf-8")
        assert page_html.startswith("<!DOCTYPE html>")
        assert "</html>" in page_html

    def test_missing_config_raises(self, tmp_path: Path) -> None:
        with pytest.raises(FileNotFoundError):
            _ServerState(config_path=tmp_path / "nonexistent.json")

    def test_page_uses_current_remote_login_protocol(self, server_state: _ServerState) -> None:
        page_html = server_state.page.decode("utf-8")
        assert 'sendRpcCommand("REMOTE_LOGIN", { interactive: true })' in page_html
        assert "const LOGIN_ENDPOINT" not in page_html
        assert "mockAuthentication(" not in page_html

    def test_page_registers_waiter_for_new_session_ack(self, server_state: _ServerState) -> None:
        page_html = server_state.page.decode("utf-8")
        assert "const msgId = ++messageCounter;" in page_html
        assert "waitForResponse(msgId, 4000).catch(() => {});" in page_html


# ---------------------------------------------------------------------------
# HTTP endpoint integration tests
# ---------------------------------------------------------------------------


class TestHttpEndpoints:
    def test_root_returns_200(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/"
        with urllib.request.urlopen(url) as resp:
            assert resp.status == 200

    def test_root_content_type(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/"
        with urllib.request.urlopen(url) as resp:
            assert "text/html" in resp.headers.get("Content-Type", "")

    def test_root_contains_parts(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/"
        with urllib.request.urlopen(url) as resp:
            body = resp.read().decode("utf-8")
        assert "Resistor 0603 1k" in body

    def test_provider_metadata_endpoint(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/.well-known/kicad-remote-provider"
        with urllib.request.urlopen(url) as resp:
            assert resp.status == 200
            content_type = resp.headers.get("Content-Type", "")
            assert "application/json" in content_type
            metadata = json.loads(resp.read())
        assert metadata["capabilities"]["web_ui_v1"] is True
        assert metadata["auth"]["type"] == "none"

    def test_provider_metadata_port_matches_server(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/.well-known/kicad-remote-provider"
        with urllib.request.urlopen(url) as resp:
            metadata = json.loads(resp.read())
        assert str(live_server) in metadata["api_base_url"]
        assert str(live_server) in metadata["panel_url"]

    def test_unknown_path_returns_404(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/no-such-path"
        with pytest.raises(urllib.error.HTTPError) as exc_info:
            urllib.request.urlopen(url)
        assert exc_info.value.code == 404

    def test_index_html_alias(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/index.html"
        with urllib.request.urlopen(url) as resp:
            assert resp.status == 200

    def test_login_page(self, live_server: int) -> None:
        url = f"http://127.0.0.1:{live_server}/login"
        with urllib.request.urlopen(url) as resp:
            assert resp.status == 200
            assert b"Sign In" in resp.read()
