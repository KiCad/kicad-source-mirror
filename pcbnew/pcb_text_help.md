<table>
    <tr>
        <th>Markup</th>
        <th></th>
        <th>Result</th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>^{superscript}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><sup>superscript</sup>&nbsp;</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>Driver Board^{Rev A}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Driver Board<sup>Rev A</sup></samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>_{subscript}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><sub>subscript</sub>&nbsp;</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>D_{0} - D_{15}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>D<sub>0</sub> - D<sub>15</sub></samp></td>
    </tr>
    <tr>
        <td></td>
    </tr>
    <tr>
        <td> &nbsp;<br><samp>~{overbar}</samp><br> &nbsp;<br><samp>~{CLK}</samp></td>
        <td></td>
        <td> <samp><u>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</u></samp><br> <samp>overbar</samp><br> <samp><u>&nbsp;&nbsp;&nbsp;</u></samp><br> <samp>CLK</samp></td>
    </tr>
    <tr>
        <td></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${variable}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><i>variable_value</i></samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${PROJECTNAME}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>MyBoard</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>PCB Variables</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${FILENAME}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>board.kicad_pcb</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${FILEPATH}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>/path/to/board.kicad_pcb</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${LAYER}</samp> (in text/tables)</td>
        <td></td>
        <td>&nbsp;<br><samp>F.Cu, B.Cu, F.SilkS...</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Footprint Cross-References</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:field}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><i>field_value</i> of footprint <i>refdes</i></samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:REFERENCE}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>U1</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:VALUE}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>STM32F407VGT6</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:FOOTPRINT_LIBRARY}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Package_QFP</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:FOOTPRINT_NAME}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>LQFP-100_14x14mm_P0.5mm</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Pad/Pin Functions</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:NET_NAME(pad)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Net name connected to pad</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:NET_NAME(5)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>VCC</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:SHORT_NET_NAME(pad)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Short net name or NC</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:SHORT_NET_NAME(3)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>GND</samp> or <samp>NC</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:NET_CLASS(pad)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Net class for pad</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:NET_CLASS(1)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Power</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:PIN_NAME(pad)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Pin function/name</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:PIN_NAME(5)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>USART1_TX</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Table Variables</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${ROW}</samp> (in tables)</td>
        <td></td>
        <td>&nbsp;<br><samp>1, 2, 3...</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${COL}</samp> (in tables)</td>
        <td></td>
        <td>&nbsp;<br><samp>1, 2, 3...</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${ADDR}</samp> (in tables)</td>
        <td></td>
        <td>&nbsp;<br><samp>A1, B2, C3...</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Math Expressions</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{expression}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><i>evaluated_result</i></samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{2 + 3}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>5</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{${ROW} - 1}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>3</samp> (when ROW=4)</td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>String Comparison & Conditional Text</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{"text" == "text"}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>1</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{"text" != "other"}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>1</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{if(condition, true_val, false_val)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Conditional text display</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{if("${LAYER}" == "F.Cu", "TOP", "BOTTOM")}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>TOP</samp> (on front layer) or <samp>BOTTOM</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>@{if(${ROW} > 5, "High", "Low")}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Numeric comparisons work too</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Escape Sequences</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>\${LITERAL}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>${LITERAL}</samp> (not expanded)</td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>Price: \$25.00</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Price: $25.00</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>\@{x+y}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>@{x+y}</samp> (not evaluated)</td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Nested Variables</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:NET_NAME(@{${ROW}-2})}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Dynamic net lookup in tables</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U${ROW}:VALUE}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Dynamic footprint reference</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
</table>
<p></p>
<p></p>
<i>Note that markup has precedence over variable expansion.</i>
<p></p>
<p><b>Footprint Cross-References:</b> Reference fields and pads on other footprints using <samp>${refdes:field}</samp> or <samp>${refdes:function(pad)}</samp> syntax.</p>
<p><b>Nested Variables:</b> Variables can contain other variables. Inner variables are expanded first. Maximum nesting depth: 10 levels.</p>
<p><b>Error Messages:</b></p>
<ul>
<li><samp>&lt;UNRESOLVED: token&gt;</samp> - Variable or function cannot be resolved</li>
<li><samp>&lt;Unknown reference: U1&gt;</samp> - Footprint not found</li>
</ul>

