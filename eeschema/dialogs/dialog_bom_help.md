# 1 - Full documentation

The Eeschema documentation (*eeschema.html*) describes this intermediate netlist and gives examples(chapter ***creating customized netlists and bom files***).

# 2 - The intermediate Netlist File

BOM files (and netlist files) can be created from an *Intermediate netlist file* created by Eeschema.

This file uses XML syntax and is called the intermediate netlist. The intermediate netlist includes a large amount of data about your board and because of this, it can be used with post-processing to create a BOM or other reports.

Depending on the output (BOM or netlist), different subsets of the complete Intermediate Netlist file will be used in the post-processing.

# 3 - Conversion to a new format

By applying a post-processing filter to the Intermediate netlist file you can generate foreign netlist files as well as BOM files. Because this conversion is a text to text transformation, this post-processing filter can be written using *Python*, *XSLT*, or any other tool capable of taking XML as input.

XSLT itself is a XML language suitable for XML transformations. There is a free program called `xsltproc` that you can download and install. The `xsltproc` program can be used to read the Intermediate XML netlist input file, apply a style-sheet to transform the input, and save the results in an output file. Use of `xsltproc` requires a style-sheet file using XSLT conventions. The full conversion process is handled by Eeschema, after it is configured once to run `xsltproc` in a specific way.

A Python script is somewhat more easy to create.

# 4 - Initialization of the dialog window

You should add a new plugin (a script) in the plugin list by clicking on the Add Plugin button.

## 4.1 - Plugin Configuration Parameters

The Eeschema plug-in configuration dialog requires the following information:

 * The title: for instance, the name of the netlist format.
 * The command line to launch the converter (usually a script).

***Note (Windows only):***
*By default, the command line runs with hidden console window and output is redirected to "Plugin info" field. To show the window of the running command, set the checkbox "Show console window".*

Once you click on the generate button the following will happen:

1. Eeschema creates an intermediate netlist file \*.xml, for instance `test.xml`.
2. Eeschema runs the script from the command line to create the final output file.

## 4.2 - Generate netlist files with the command line

Assuming we are using the program `xsltproc.exe` to apply the sheet style to the intermediate file, `xsltproc.exe` is executed with the following command.

```
xsltproc.exe -o <output filename> <style-sheet filename> <input XML file to convert>
```

On Windows the command line is the following.

```
f:/kicad/bin/xsltproc.exe -o "%O" f:/kicad/bin/plugins/myconverter.xsl "%I"
```

On Linux the command becomes as following.

```
xsltproc -o "%O" /usr/local/kicad/bin/plugins/myconverter .xsl "%I"
```
where `myconverter.xsl` is the style-sheet that you are applying.

Do not forget the double quotes around the file names, this allows them to have spaces after the substitution by Eeschema.

If a Python script is used, the command line is something like (depending on the Python script):

```
python f:/kicad/bin/plugins/bom-in-python/myconverter.py "%I" "%O"
```

or

```
python /usr/local/kicad/bin/plugins/bom-in-python/myconverter .xsl "%I" "%O"
```

The command line format accepts parameters for filenames. The supported formatting parameters are:

 * `%B`: base filename of selected output file, minus path and extension.
 * `%P`: project directory, without name and without trailing '/'.
 * `%I`: complete filename and path of the temporary input file
(the intermediate net file).
 * `%O`: complete filename and path (but without extension) of the user
chosen output file.

`%I` will be replaced by the actual intermediate file name (usually the full root sheet filename with extension ".xml").
`%O` will be replaced by the actual output file name (the full root sheet filename minus extension).
`%B` will be replaced by the actual output short file name (the short root sheet filename minus extension).
`%P` will be replaced by the actual current project path.

## 4.3 - Command line format:

### 4.3.1 - Remark:

Most of time, the created file must have an extension, depending on its type.
Therefore you have to add to the option ***%O*** the right file extension.

For instance:

 * **%O.csv** to create a .csv file (comma separated value file).
 * **%O.htm** to create a .html file.
 * **%O.bom** to create a .bom file.

### 4.3.2 Example for xsltproc:

The command line format for xsltproc is the following:

```
<path of xsltproc> xsltproc <xsltproc parameters>
```

On Windows:
```
f:/kicad/bin/xsltproc.exe -o "%O.bom" f:/kicad/bin/plugins/netlist_form_pads-pcb.xsl "%I"
```

On Linux:
```
xsltproc -o "%O.bom" /usr/local/kicad/bin/plugins/netlist_form_pads-pcb.xsl "%I"
```

The above examples assume `xsltproc` is installed on your PC under Windows and xsl files located in `<path_to_kicad>/kicad/bin/plugins/`.


### 4.3.3 Example for Python scripts:

Assuming python is installed on your PC, and python scripts are located in

 `<path_to_kicad>/kicad/bin/plugins/bom-in-python/`,

the command line format for python is something like:

```
python <script file name> <input filename> <output filename>
```

On Windows:
```
python.exe f:/kicad/bin/plugins/bom-in-python/my_python_script.py "%I" "%O.html"
```

On Linux:
```
python /usr/local/kicad/bin/plugins/bom-in-python/my_python_script.py "%I" "%O.csv"
```
