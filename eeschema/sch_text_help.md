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
        <td>&nbsp;<br><samp>${REVISION}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>2020.1</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:field}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><i>field_value</i> of symbol <i>refdes</i></samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${R3:VALUE}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>150K</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${ROW}</samp> (in tables)</td>
        <td></td>
        <td>&nbsp;<br><samp>0, 1, 2...</samp> (0-based)</td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${COL}</samp> (in tables)</td>
        <td></td>
        <td>&nbsp;<br><samp>0, 1, 2...</samp> (0-based)</td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${ADDR}</samp> (in tables)</td>
        <td></td>
        <td>&nbsp;<br><samp>A0, B1, C2...</samp> (0-based)</td>
    </tr>
    <tr>
        <td><br></td>
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
        <td>&nbsp;<br><samp>@{${ROW} + 1}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>4</samp> (when ROW=3)</td>
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
        <th>Symbol Pin Functions</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:REFERENCE(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Full reference with unit for pin</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:REFERENCE(3)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>J1B</samp> (for multi-unit symbol)</td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:SHORT_REFERENCE(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Reference without unit letter for pin</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:SHORT_REFERENCE(3)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>J1</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:UNIT(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Unit letter only for pin</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:UNIT(3)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>B</samp> (unit letter for pin 3)</td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:NET_NAME(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Net name connected to pin</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${R1:NET_NAME(1)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>VCC</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:PIN_NAME(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Pin name or selected alternate</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:PIN_NAME(5)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>USART1_TX</samp> (alternate) or <samp>PA9</samp> (base)</td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:PIN_BASE_NAME(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Base pin name (ignoring alternates)</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:PIN_BASE_NAME(5)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>PA9</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:PIN_ALT_LIST(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>All alternate pin functions (excludes base name)</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${U1:PIN_ALT_LIST(5)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>USART1_TX, TIM1_CH2, I2C1_SCL</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:SHORT_NET_NAME(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Short net name or NC if unconnected</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:SHORT_NET_NAME(3)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>GND</samp> or <samp>NC</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:NET_CLASS(pin)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Net class for pin</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:NET_CLASS(1)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Power</samp></td>
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
        <td>&nbsp;<br><samp>${J1:REFERENCE(@{${ROW}+2})}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>J1B</samp> (when ROW=0, pin 2 in unit B)</td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${J1:NET_NAME(${COL})}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Dynamic net lookup in tables</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Table Cell References</th>
        <th></th>
        <th></th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${CELL("A0")}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Evaluated value from cell A0</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${CELL(0, 1)}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Value from row 0, column 1</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${CELL(${ADDR})}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Dynamic cell reference</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${CELL(${ROW}-1, ${COL})}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Value from cell above (if ROW > 0)</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Bus Definition</th>
        <th>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</th>
        <th>Resultant Nets</th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>prefix[m..n]</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>prefixm to prefixn</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>D[0..7]</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>D0, D1, D2, D3, D4, D5, D6, D7</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>{net1 net2 ...}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>net1, net2, ...</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>{SCL SDA}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>SCL, SDA</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>prefix{net1 net2 ...}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>prefix.net1, prefix.net2, ...</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>USB1{D+ D-}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>USB1.D+, USB1.D-</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>MEM{D[1..2] LATCH}</samp></td>
        <td></td>
        <td>&nbsp;<br> <samp>MEM.D1, MEM.D2, MEM.LATCH</samp> </td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>MEM{D_{[1..2]} ~{LATCH}}</samp></td>
        <td></td>
        <td> <samp>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<sub>&nbsp;</sub>&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<sub>&nbsp;</sub>&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;<u>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</u></samp><br> <samp>MEM.D<sub>1</sub>, MEM.D<sub>2</sub>, MEM.LATCH</samp> </td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
</table>
<p></p>
<p></p>
<i>Note that markup has precedence over bus definitions.</i>
<p></p>
<p><b>Pin Functions:</b> Automatically find the correct unit placement. For multi-unit symbols, functions like <samp>NET_NAME(pin)</samp> work even if the pin is in a different unit than the one on the current sheet.</p>
<p><b>Table Cell References:</b> The <samp>CELL()</samp> function works only in table cells. Use <samp>${CELL("A0")}</samp> or <samp>${CELL(row, col)}</samp> to reference other cells in the same table. Row and column numbers are 0-based (A0 is the first row, first column). CELL returns the evaluated/displayed value, not the raw cell text.</p>
<p><b>Nested Variables:</b> Variables can contain other variables. Inner variables are expanded first. Maximum nesting depth: 6 levels.</p>
<p><b>Error Messages:</b></p>
<ul>
<li><samp>&lt;UNRESOLVED: token&gt;</samp> - Variable or function cannot be resolved</li>
<li><samp>&lt;Unit X not placed&gt;</samp> - Pin is in a unit not placed on any sheet</li>
<li><samp>&lt;Unresolved: Cell X not found&gt;</samp> - Cell address is out of table bounds</li>
</ul>

