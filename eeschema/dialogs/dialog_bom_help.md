# 1. Summary

The Bill of Materials tool creates a BOM which lists all of the components in the design.

The tool uses an external script to generate a BOM in the desired output format. Choosing a different script changes how the BOM is formatted.

Generating a BOM is described in more detail in the Schematic Editor manual.

# 2. Usage

Select a generator script in the **BOM generator scripts** list. Details for the selected generator are shown on the right of the dialog.

Clicking the **Generate** button creates a BOM file with the selected generator.

The default settings present several generator script options, although some additional scripts are installed with KiCad and can be added to the list with the **+** button.

**Note:** On Windows, there is an additional option **Show console window**. When this option is unchecked, BOM generators run in a hidden console window and any output is redirected and printed in the dialog. When this option is checked, BOM generators run in a visible console window.

# 3. Custom generators and command lines

Internally, KiCad creates an intermediate netlist file in XML format that contains information about all of the components in the design. A BOM generator script converts the intermediate netlist file to the desired output format. KiCad runs the BOM generator scripts according to the command line entered at the bottom of the BOM dialog.

The command line format accepts parameters for filenames. Each formatting parameter is replaced with a project-specific path or filename. The supported formatting parameters are:

 * `%I`: absolute path and filename of the intermediate netlist file, which is the input to the BOM generator
 * `%O`: absolute path and filename of the output BOM file (without file extension)
 * `%B`: base filename of the output BOM file (without file extension)
 * `%P`: absolute path of the project directory, without trailing slash

**Note:** the `%O` output file parameter does not include a file extension. KiCad will attempt to add an appropriate extension to the command line automatically, but an extension may need to be added by hand.

Python is the recommended tool for BOM generator scripts, but other tools can also be used.

## Example command lines for Python scripts

The command line format for a Python script is of the form:

```
python <script file name> <input filename> <output filename>
```

On Windows, if the desired generator script for a CSV BOM is `C:\Users\username\kicad\my_python_script.py`, the command line would be:

```
python.exe C:\Users\username\kicad\my_python_script.py "%I" "%O.csv"
```

On Linux, if the desired generator script for a CSV BOM is `/home/username/kicad/my_python_script.py`, the command line would be:

```
python /home/username/kicad/my_python_script.py "%I" "%O.csv"
```

Double quotes (`"`) around the arguments are recommended in case filenames contain spaces or special characters.
