#! /usr/bin/awk -f
#
# awk script to convert KiCAD font.

BEGIN {
	fn_header = "font_header.cpp"
	while (getline line < fn_header)
		print line
	close(fn_header)

	missed = 0
	printstats = 1

	code0 = 82 # ascii code for R
	scale = 50
	base = 9
	fontend = ""
	symdef = "DEL"
	cap_height = -21
	x_height = -14
	sym_height = -16
	sup_offset = -13
	sub_offset = 6
	# transformations props
	#              SX SY OY
	transf["!"] = "-1 +1 0"          # revert
	transf["-"] = "+1 -1 "x_height   # invert small
	transf["="] = "+1 -1 "cap_height # invert cap
	transf["~"] = "+1 -1 "sym_height # invert symbol
	transf["+"] = "-1 -1 "x_height   # turn small
	transf["%"] = "-1 -1 "cap_height # turn cap
	transf["*"] = "-1 -1 "sym_height # turn symbol
	transf["^"] = "+1 +1 "sup_offset # superscript
	transf["`"] = "-1 +1 "sup_offset # superscript reversed
	transf["."] = "+1 +1 "sub_offset # subscript
	transf[","] = "-1 +1 "sub_offset # subscript reversed
}

function ntochar(x) {
	return sprintf("%c",x+code0)
}

function cname(glyph) {
	if(substr(glyph,1,1) in transf) {
		return substr(glyph,2)
	} else {
		return glyph
	}
}

function gsx(glyph,   tr) {
	if(substr(glyph,1,1) in transf) {
		tr = transf[substr(glyph,1,1)]
		return 0+substr(tr,1,2)
	} else {
		return 1;
	}
}

function gsy(glyph,   tr) {
	if(substr(glyph,1,1) in transf) {
		tr = transf[substr(glyph,1,1)]
		return 0+substr(tr,4,2)
	} else {
		return 1;
	}
}

function goy(glyph,   tr) {
	#print "// "substr(glyph,1,1)
	if(substr(glyph,1,1) in transf) {
		tr = transf[substr(glyph,1,1)]
		return 0+substr(tr,7)
	} else {
		return 0;
	}
}

function graph(glyph, subst, ofx, ofy,   sx, sy, cn, data, i, j, o, c) {
	if(subst=="")subst=symdef
	if(ofx=="") ofx = 0
	if(ofy=="") ofy = 0
	if(!(cname(glyph) in fontnst)) {
		print "    /* glyph "glyph" not found!!! */"
		glyph = subst
	}
	sx = gsx(glyph)
	sy = gsy(glyph)
	ofy += goy(glyph) + base
	#print "/* g ofx="ofx" ofy="ofy" */"
	cn = cname(glyph)
	data = ""
	for(i = 0; i<fontnst[cn]; i++) {
		for(j = 0; j<fontsl[cn" "i]; j++) {
			c = ntochar(fontx[cn" "i" "j]*sx+ofx) ntochar(fonty[cn" "i" "j]*sy+ofy)
			if(c!=o) {
				if (j==0 && i>0)
					# A new stroke is started only when the previous one
					# ended at a point different from the starting point of this one.
					data = data " R"
				data = data c
				o = c
			}
		}
	}
	fontuse[cn] += 1
	return data
}
function metric(glyph, subst,   sx, cn, ml, mr) {
	if(subst=="")subst=symdef
	if(!(cname(glyph) in fontnst)) glyph = subst
	sx = gsx(glyph)
	cn = cname(glyph)
	ml = sx<0?fontmr[cn]:fontml[cn]
	mr = sx<0?fontml[cn]:fontmr[cn]
	return ntochar(sx*ml) ntochar(sx*mr)
}
function metric2(glyph1, glyph2, ofx1, ofx2, subst,   sx, cn, ml, mr) {
	if(subst=="")subst=symdef
	if(!(cname(glyph1) in fontnst)) glyph1 = subst
	sx1 = gsx(glyph1)
	cn1 = cname(glyph1)
	if(!(cname(glyph2) in fontnst)) glyph2 = subst
	sx2 = gsx(glyph2)
	cn2 = cname(glyph2)
	ml = sx1<0?fontmr[cn1]:fontml[cn1]
	mr = sx2<0?fontml[cn2]:fontmr[cn2]
	return ntochar(sx1*ml+ofx1) ntochar(sx2*mr+ofx2)
}
function dist(glyph1, glyph2, subst,   sx, cn, ml, mr) {
	if(subst=="")subst=symdef
	if(!(cname(glyph1) in fontnst)) glyph1 = subst
	sx1 = gsx(glyph1)
	cn1 = cname(glyph1)
	if(!(cname(glyph2) in fontnst)) glyph2 = subst
	sx2 = gsx(glyph2)
	cn2 = cname(glyph2)
	return sx1*fontmr[cn1] - sx2*fontml[cn2]
}
function offsetx(char, accent, anchor,  del, cc, ca, asx, csx) {
	if("" == anchor || "#" == anchor) return 0;
	del = index(anchor, "=")
	if(del) {
		cc = cname(char)" "substr(anchor,1,del-1)
		ca = cname(accent)" "substr(anchor,del+1)
	} else {
		cc = cname(char)" "anchor
		ca = cname(accent)" "anchor
	}
	if((cc in fontancx) && (ca in fontancx)) {
		asx = gsx(accent)
		csx = gsx(char)
		#print "    /* ox="csx*fontancx[cc]-asx*fontancx[ca]" */"
		return csx*fontancx[cc]-asx*fontancx[ca]
	} else {
		return 0
	}
}

function offsety(char, accent, anchor,   del, cc, ca, asy, csy, aoy, coy) {
	if("" == anchor || "#" == anchor) return 0;
	del = index(anchor, "=")
	#print "    /* del="del" */"
	if(del) {
		cc = cname(char)" "substr(anchor,1,del-1)
		ca = cname(accent)" "substr(anchor,del+1)
	} else {
		cc = cname(char)" "anchor
		ca = cname(accent)" "anchor
	}
	if((cc in fontancy) && (ca in fontancy)) {
		asy = gsy(accent)
		csy = gsy(char)
		aoy = goy(accent)
		coy = goy(char)
		#print "    /* oy="csy*fontancy[cc]+coy-asy*fontancy[ca]-aoy" */"
		return csy*fontancy[cc]+coy-asy*fontancy[ca]-aoy
	} else {
		print "    /* anchor "(cc in fontancy?"":cc) (cc in fontancy||ca in fontancy?"":" and ") (ca in fontancy?"":ca)" not found! */"
		return 0
	}
}

function cesc(s) {
	gsub(/[\\"']/,"\\\\&",s)
	return s
}

function gaccent(b,g,anc,cx) {
	if(!(cname(g) in fontnst)) {
		missed += 1
	}
	
	if(anc=="" || anc=="#") anc=""
	ofx = offsetx(b,g,anc)
	ofy = offsety(b,g,anc)
	return " R"graph(g,accdef,cx+ofx,ofy)
}
function compose(cx,   st) {
	if(!(cname($2) in fontnst)) {
		missed += 1
	}
	st = graph($2,symdef,cx,0)
	if($3!="" && $3!="#") {
		st = st gaccent($2,$3,$4,cx)
		ofx2 = ofx
		if($4!="#" && $5!="" && $5!="#") {
			st = st gaccent($2,$5,$6,cx)
			if($6!="#" && $7!="" && $7!="#") {
				st = st gaccent($2,$7,$8,cx)
			}
		}
	}
	return st
}

# parsing glyph library
$1 == "DEF" {
	name = $2
	data = ""
	prepend = ""
	nst = 0
	xleft = 0
	xright = 0
	fontancx[name" -"] = 0
	fontancy[name" -"] = 0
}

$1 == "P" { # drawings
	n = $2*2+6
	# print "Poly of "$2
	data = data prepend
	data_rev = data_rev prepend
	ox = -100
	oy = -100
	j = 0
	for(i = 6; i<n; i+=2) {
		x = ($i)/scale
		y = -($(i+1))/scale
		#print "X: "x" Y: "y
		if(x!=ox || y!=oy) {
			data = data ntochar(x) ntochar(y+base)
			fontx[name" "nst" "j] = x
			fonty[name" "nst" "j] = y
			j++
			ox = x
			oy = y
		}
	}
	fontsl[name" "nst] = j
	nst++
	prepend = " R"
}

$1 == "X" { # i use pins as metric and anchors
	fontancx[name" "$2] = $4/scale
	fontancy[name" "$2] = -$5/scale
	if($4>0 && $2=="~" || $2=="S") {
		xright = $4/scale
		fontmr[name] = $4/scale
		# print "//Right:" xright
	} else if($4<=0 && $2=="~" || $2=="P") {
		xleft = $4/scale
		fontml[name] = $4/scale
		# print "//Left:" xleft
	}
}

$1 == "ENDDEF" {
	metr = ntochar(xleft) ntochar(xright)
	fontnst[name] = nst
	fontuse[name] = 0
	fi[name] = name
}

# parsing font index
$1 == "font" {
	print fontend
	print "\n"
	print "const char* const "$2"[] ="
	print "{"
	fontend = "};\nconst int "$2"_bufsize = sizeof("$2")/sizeof("$2"[0]);\n"
}
$1 == "startchar" {
	codeno = $2
}
$1 == "+" || $1 == "+w" || $1 == "+p" {
	comx = 0
	rem = (codeno%16)?"":sprintf(" /* U+%X %s %s */", codeno, $2, $3)
	#print "// c "codeno
	codeno+=1
	str = compose()
	met = ("+w"==$1) ? metric2($2, $3, 0, ofx2) : ("+p"==$1) ? metric2($3, $2, ofx2, 0) : metric($2)
	print "    \""cesc(met str) "\","rem
}
$1 == "+(" {
	comx = 0
	rem = (codeno%16)?"":sprintf(" /* U+%X %s %s */", codeno, $2, $3)
	#print "// c "codeno
	codeno+=1
	str = compose(comx)
	gf = $2
	gp = $2
}
$1 == "+|" {
	comx = comx + dist(gp, $2)
	#print "// "comx
	str = str " R" compose(comx)
	gp = $2
}
$1 == "+)" {
	comx = comx + dist(gp, $2)
	#print "// "comx
	str = str " R" compose(comx)
	met = metric2(gf, $2, 0, comx)
	print "    \"" cesc(met str) "\","rem
}

$1 == "skipcodes" {
	#print "    // skip "$2
	skipper = codeno > 0x9000 ? "0" : symdef
	for(i = 0; i<$2; i++) {
		print "    \""cesc(metric(skipper) graph(skipper))"\"," ((codeno%16)?"":sprintf(" /* U+%X */", codeno))
		codeno += 1
	}
}
$1 == "//" {
	print "    /* "$0" */"
}
$1 == "stats" {
	printstats = $2
}

END {
	print fontend
	if(printstats) {
		if(missed>0) print "/* Missed glyphs: " missed " */"
		print "/* --- unused glyphs --- */"
		PROCINFO["sorted_in"] = "@val_str_asc"
		for(f in fi) {
			if(0==fontuse[f]) print "/*  "f"  */"
		}
	}
}
