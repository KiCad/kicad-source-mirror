# KiCad HTTP Library Test Server

This module implements a test HTTP server using FastAPI. It is a simple way to spin up a local API server to test the KiCad HTTP library feature.

FastAPI gives you a test interface at the `/docs` endpoint to observe the responses for debug.

## Dependencies / Setup

### Using uv (Recommended)

[uv](https://github.com/astral-sh/uv) is a fast Python package installer and resolver written in Rust. **uv works best for isolation** by automatically managing virtual environments and dependencies without affecting your system Python.

**Install uv:**
```bash
# On macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh

# On Windows
powershell -c "irm https://astral.sh/uv/install.ps1 | iex"

# Or with pip
pip install uv
```

**Install dependencies:**
```bash
cd qa/tests/eeschema
uv sync
```

That's it! `uv sync` automatically creates an isolated virtual environment and installs all dependencies.

### Using pip

```bash
# Navigate to directory
cd qa/tests/eeschema

# Create virtual environment (recommended for isolation)
python -m venv .venv
source .venv/bin/activate  # On Unix/macOS
.venv\Scripts\activate     # On Windows

# Install dependencies
pip install -r requirements.txt
```

## Usage

Run the HTTP test server:

```bash
uv run http_lib_test_server.py
```

Or use:
```bash
python http_lib_test_server.py
```

The server will start on `http://0.0.0.0:8000`

The server implements the KiCad HTTP Library API in the simplest use case, with one resistor category and one sample part (RC0603FR-0710KL). See the [KiCad HTTP Library documentation](https://dev-docs.kicad.org/en/apis-and-binding/pcbnew/) for API details.

## Testing with KiCad

1. Start the test server:
   ```bash
   uv run http_lib_test_server.py
   ```

2. In KiCad, load the HTTP library configuration file `http_test.kicad_httplib` (located in the same folder as the test script)

3. Add a symbol in the schematic editor.  your should find the http_test library in the list.

## License

This script is part of KiCad, licensed under the GNU General Public License v3.0.

For development and testing purposes only. Not intended for production use.
