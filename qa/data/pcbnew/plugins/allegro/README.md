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
- Run just the AllegroBoards tests, excluding slow boards: `qa_pcbnew -t 'AllegroBoards' -t '!@slow-board'`
  (note - single quotes avoid having the `!` interpreted by the shell)
- Run block tests for the `Bar` board: `qa_pcbnew -t 'AllegroBlocks/Bar'`
- Run just the loading test for all boards: `qa_pcbnew -t 'AllegroBoards/*/Import'`
- Run block tests for `0x20` blocks in all boards: `qa_pcbnew -t 'AllegroBlocks/*/*0x20'`
- Run expectation tests for all boards: `qa_pcbnew -t 'AllegroBoards/*/Expectations'`

## Expectations

"Expectations" are declarative tests that can be run on the results of importing a board file.

Every board can have a list of expectations defined in the
`board_data_registry.json` file, and these are run as part of the `Expectations`
test suite below the board suite.

Each expectation is defined as a JSON object with a `type` field that specifies
the type of expectation, and other fields that provide the data for the test.
For example, this `footprint` expectation checks that the number of footprints
imported from the board file matches the expected count:

```json
{
  "type": "expr",
  "testName": "Footprint count",
  "comment": "More verbose description of what this expectation is checking",
  "itemType": "footprint",
  "count": {
    "minimum": 42,
  }
}
```

Common expectation fields:

- `testName` - a short name for the expectation, used to filter the tests if needed
- `comment` - a longer description of the expectation, which will be printed in the test
- `skip` - if true, the expectation will be skipped (e.g. to temporarily disable
  a failing test while working on a fix)


## Types

Current expectation types include:

- `item` - checks for item existence
  - `itemType`: - the type of item:
    - `any` (matches any item type - slow!). This is the default if `itemType` is not specified,
      but it's better to specify the expected item type if possible to speed up the tests.
    - `footprint`
    - `pad`
    - `track`
    - `zone`
    - `board_graphic` (board-level graphic item)
    - `board_group`
    - `board_zone`
    - `fp_graphic` (footprint-level graphic item)
    - `fp_group`
    - `fp_field`

  - `expr` - KiCad evaluator expression to match (e.g. `A.Diameter == 16 mil`)
    - If empty, the matched count is the number of items of the given type (i.e. no filter)
  - `count` - checks the number of footprints
    - `exact` - the count must match this value exactly
    - `minimum` - the count must be at least this value
    - `maximum` - the count must be at most this value (can be used together with `minimum` to specify a
                  range)
    - If not given, the expectation is that there should be at least one matching item
  - `parentExpr`: if the item has a parent (e.g. footprint or group), the expression that should match
    and items that match `expr`.
- `net` - checks net properties
  - `count` - checks the number of nets (same structure as `expr` count)
  - `name` - checks that there are nets with names matching this pattern (can be a glob). If count is
    not specified, at least one match is required for each pattern. Otherwise, the count is expected
    to match the total number of matches across all patterns.
- `netclass` - checks netclass properties
  - `name` - checks that there are netclasses with names matching this pattern or patterns (can be globs)
  - `trackWidth` - checks the track width for all matching netclasses
  - `clearance` - checks the clearance for all matching netclasses
  - `dpGap` - checks the differential pair gap for all matching netclasses
  - `dpWidth` - checks the differential pair width for all matching netclasses
  - `netCount` - checks the number of nets that belong to the matching netclasses (same structure as `expr` count)
