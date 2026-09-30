# ODB++ export options

read_when: changing ODB++ job serialization, CLI or API options, matrix filtering, or the export dialog

`JOB_EXPORT_PCB_ODB` owns the settings used by jobsets, the CLI, the API, and the dialog. With no new settings, the exporter keeps its previous ODB++ output. The job JSON keys are `origin` (`absolute`, `aux`, `grid`), `product_name`, `data_set` (`all`, `fabrication`, `assembly`, `test`, `stackup`), `sections`, `net_names` (`include`, `anonymize`), and `layers`. Each `layers` entry names a canonical KiCad layer and may set `include`, `odb_name`, or `odb_type`. Invalid layer overrides warn and retain automatic metadata.

The CLI accepts `--origin`, `--product-name`, `--data-set`, and `--net-names`. Section keys and layer overrides are jobset settings. The API appends typed origin, data set, and net-name fields plus product name to `RunBoardJobExportODB` at field numbers 11–14. Unknown API enum values use the legacy defaults.

`fab_sections` shares the IPC-2581 Table 4 section keys. ODB++ maps those keys to matrix rows and the EDA and cadnet files. A component section requires packages and EDA NET records. Y controls cadnet. G controls EDA NET records when there are no components. A physical netlist with intentional net ties also keeps EDA NET records and shortf so the net tie is not reported as a short. Filtering resolves matrix IDs and spans after the final layer names are known. Core dielectric references only its originally adjacent copper; if that copper is excluded, the reference is omitted.

Exporter warnings reach the supplied `REPORTER`. A completed write determines `ODB_EXPORT_RESULT::m_ok`.

The ODB++ and IPC-2581 dialogs share the content controls. ODB++ can export the current, selected, or all board variants as separate outputs or one combined package. The ODB++ layer grid previews the matrix rows for the selected data set; only board layers accept include, name, and type overrides. Generated drill, dielectric, and auxiliary rows are display-only. Hidden layer overrides remain in the job when switching data sets. The interactive ODB++ dialog stays open after export and displays exporter messages in its report panel.

ODB++ entity names for layers, products, variants, and feature-text attributes lowercase ASCII letters and replace each unsupported Unicode character with one underscore, regardless of system locale. Feature text reaches the attribute writer as UTF-8 before normalization. Separate-variant output names are checked for both Unicode and ASCII case collisions before any file is written. An IPC-2581 job restores the board's current variant after export, so a later ODB++ job uses the board's original selection.

Component and mapped part names use printable ASCII, replacing each unsupported Unicode character with one underscore. Component property keys and values, toeprint names, package names, and pin names are written as UTF-8, independent of the system code page.
