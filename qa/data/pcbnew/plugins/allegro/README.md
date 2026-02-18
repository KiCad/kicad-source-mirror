# Allegro unit tests

The Allegro format is complicated and opaque, so we have to lean heavily on unit tests to avoid
regressions and lock in the most correct parsing we can.

Current test suites:

- `AllegroImport` - tests on the full import of entire board files with some general checks on the
    overall import results in terms of tests on the `BOARD` object.
- `AllegroComprehensive` - more in-depth tests on the full import of entire board files. Some of
    these also do cross-checks against `.alg` ASCII formats of the same board files.
- `AllegroBlocks` - tests on parsing of individual binary blocks. These are much faster to
    run than the full import tests, and help to isolate issues with version-dependent parsing logic
    and the conversion of binary data to the intermediate DB_OBJ form.
- `AllegroBoards` - generalised tests on the full import of entire board files, with test cases
    generated from a registry of board files and the general board "expectation" data.
  - Slow boards in this suite are labelled as `slow-board` and can be filtered out of the test runs when needed.
  - These test have the following sub-test units:
    - `Import` - tests that the board file can be imported without errors
    - `Expectations` - tests that check the imported board against the expectations defined for that board in the registry
      (depends on the `Import` test, so will be skipped if the board fails to import)
    - `Header` - tests on the header data parsing and any specific checks related to the header data
    - `Blocks` - tests on the parsing of individual blocks and/or conversion of block data to DB_OBJ form.
      These tests are strictly block-specific, so they don't have the wide board context.

Tests in the `AllegroBlocks` and `AllegroBoards` suites are generated from the data in the `board_data_registry.json` file.
These tests are organised into sub-suites based on the board name.

## Running tests

The usual Boost test filtering works, so you can run tests like this:

- Run all Allegro tests: `qa_pcbnew -t 'Allegro*'`
- Run just the AllegroBlocks tests: `qa_pcbnew -t 'AllegroBlocks'`
- Run just the AllegroBlocks test for the `Foo` boards: `qa_pcbnew -t 'AllegroBlocks/Foo'`
- Run just the AllegroBoards tests, excluding slow boards: `qa_pcbnew -t 'AllegroBoards' -t '!@slow-board'` (note - single quotes avoid having the `!` interpreted by the shell)
- Run block tests for the `Bar` board: `qa_pcbnew -t 'AllegroBlocks/Bar'`
- Run just the loading test for all boards: `qa_pcbnew -t 'AllegroBoards/*/Import'`
- Run block tests for `0x20` blocks in all boards: `qa_pcbnew -t 'AllegroBlocks/*/*0x20'`

## Expectations

"Expectations" are declarative tests that can be run on the results of importing a board file.

Every board can have a list of expectations defined in the `board_data_registry.json` file, and these are run as part of the `AllegroBoards` test suite.

Each expectation is defined as a JSON object with a `type` field that specifies the type of expectation, and other fields that provide the data for the test. For example, this `footprint`
expectation checks that the number of footprints imported from the board file matches the expected count:

```json
{
  "type": "footprint",
  "count": {
    "minimum": 42,
  }
}
```

## Types

Current expectation types include:

- `footprint` - checks footprint properties
  - `count` - checks the number of footprints
    - `exact` - the count must match this value exactly
    - `minimum` - the count must be at least this value
    - `maximum` - the count must be at most this value (can be used together with `minimum` to specify a range)
- `net` - checks net properties
  - `count` - checks the number of nets (same structure as `footprint` count)
  - `name` - checks that there are nets with names matching this pattern (can be a glob). If count is
    not specified, at least one match is required for each pattern. Otherwise, the count is expected
    to match the total number of matches across all patterns.
