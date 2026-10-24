// Moon Phases HUD - standalone moon-phase widget.
// Same drawing style as the moon badge of "By Time A Clock HUD" (night sky, Masser + Secunda),
// but self-contained: no clock, no date lines, only the moons and an optional phase-name label.
// Everything is drawn from script; the plugin pushes configuration, phases and translated names.

var ready = false;

// ---- configuration (defaults mirror the C++ Settings::Data) -------------------------------
// ax: 0 left / 1 centre / 2 right   ay: 0 top / 1 middle / 2 bottom
// mxo/myo/ms, sxo/syo/ss: per-moon offset (pixels) and size (percent) inside the badge
// lsrc: 0 Masser / 1 Secunda   lx/ly/ls: label offset and size
var cfg = {ax:2, ay:0, mx:40, my:40, scale:100, alpha:100, sky:true, stars:true, custom:false, color:0xE9E1D0, showM:true, mxo:0, myo:0, ms:100, showS:true, sxo:0, syo:0, ss:100, label:true, lsrc:0, lx:0, ly:0, ls:100, text:0xF2EBDD, shadow:true};
var pm = 0;   // Masser phase 0..1 (0 new, 0.25 first quarter, 0.5 full)
var ps = 0;   // Secunda phase 0..1
var shown = false;
var curAlpha = 0;

// English fallback; the plugin sends the translated names through setStrings().
var NAMES = ["New moon", "Waxing crescent", "First quarter", "Waxing gibbous", "Full moon", "Waning gibbous", "Last quarter", "Waning crescent"];

var artMc = this.createEmptyMovieClip("artMc", 1);
var tfSh = this.createTextField("tfSh", 2, 0, 0, 10, 10);
var tf = this.createTextField("tf", 3, 0, 0, 10, 10);

function fieldInit(tf) {
	tf.selectable = false;
	tf.autoSize = "left";
	tf.multiline = false;
	tf.wordWrap = false;
	var f = new TextFormat();
	f.font = "$EverywhereMediumFont";
	f.size = 15;
	f.bold = false;
	f.color = 0xFFFFFF;
	tf.setNewTextFormat(f);
}
fieldInit(tfSh);
fieldInit(tf);

// ---- drawing helpers (adapted from By Time A Clock HUD) ------------------------------------

function mix(a, b, t) {
	var ar = (a >> 16) & 255;
	var ag = (a >> 8) & 255;
	var ab = a & 255;
	var br = (b >> 16) & 255;
	var bg = (b >> 8) & 255;
	var bb = b & 255;
	return (Math.round(ar + (br - ar) * t) << 16) | (Math.round(ag + (bg - ag) * t) << 8) | Math.round(ab + (bb - ab) * t);
}

function disc(mc, cx, cy, r, rgb, a) {
	var n = 36;
	mc.beginFill(rgb, a);
	mc.moveTo(cx + r, cy);
	for (var i = 1; i <= n; i++) {
		var t = i / n * 6.283185;
		mc.lineTo(cx + Math.cos(t) * r, cy + Math.sin(t) * r);
	}
	mc.endFill();
}

// arc along the ring between two day fractions (0 = midnight at bottom, 0.5 = noon at top)
function arc(mc, cx, cy, r, f0, f1) {
	var steps = Math.max(2, Math.ceil((f1 - f0) * 80));
	for (var i = 0; i <= steps; i++) {
		var f = f0 + (f1 - f0) * i / steps;
		var th = (f - 0.5) * 6.283185;
		var x = cx + Math.sin(th) * r;
		var y = cy - Math.cos(th) * r;
		if (i == 0) {
		mc.moveTo(x, y);
	} else {
		mc.lineTo(x, y);
	}
	}
}

function circle(m, cx, cy, r, rgb, a, lw) {
	m.lineStyle(lw, rgb, a);
	m.moveTo(cx + r, cy);
	for (var i = 1; i <= 48; i++) {
		var t = i / 48 * 6.283185;
		m.lineTo(cx + Math.cos(t) * r, cy + Math.sin(t) * r);
	}
	m.lineStyle(0, 0, 0);
}

function sparkle(m, cx, cy, r, rgb, a) {
	m.lineStyle(0, 0, 0);
	m.beginFill(rgb, a);
	m.moveTo(cx, cy - r); m.lineTo(cx + r * 0.22, cy - r * 0.22); m.lineTo(cx + r, cy);
	m.lineTo(cx + r * 0.22, cy + r * 0.22); m.lineTo(cx, cy + r); m.lineTo(cx - r * 0.22, cy + r * 0.22);
	m.lineTo(cx - r, cy); m.lineTo(cx - r * 0.22, cy - r * 0.22); m.lineTo(cx, cy - r);
	m.endFill();
}

// moon with phase p (0 new, 0.5 full, 1 new). Lit on the right while waxing, on the left while waning.
function moonPhase(m, cx, cy, r, p, litRgb, darkRgb) {
	disc(m, cx, cy, r, darkRgb, 100);
	var side = 1;
	var q = p;
	if (p > 0.5) { side = -1; q = 1 - p; }
	var c = Math.cos(q * 6.283185);
	m.lineStyle(0, 0, 0);
	m.beginFill(litRgb, 100);
	for (var i = 0; i <= 12; i++) {
		var t = (i / 12 - 0.5) * 3.141593;
		var x = cx + side * Math.cos(t) * r;
		var y = cy + Math.sin(t) * r;
		if (i == 0) {
			m.moveTo(x, y);
		} else {
			m.lineTo(x, y);
		}
	}
	for (var j = 12; j >= 0; j--) {
		var t2 = (j / 12 - 0.5) * 3.141593;
		m.lineTo(cx + side * Math.cos(t2) * r * c, cy + Math.sin(t2) * r);
	}
	m.endFill();
	m.lineStyle(1, litRgb, 35);
	arc(m, cx, cy, r, 0, 1);
}

// tint helper: keeps the badge's lights and shadows when a custom colour is active
function tint(rgb) {
	if (!cfg.custom) {
		return rgb;
	}
	var r = (rgb >> 16) & 255;
	var g = (rgb >> 8) & 255;
	var b = rgb & 255;
	var L = (0.3 * r + 0.59 * g + 0.11 * b) / 255;
	if (L < 0.62) {
		return mix(0x000000, cfg.color, L / 0.62);
	}
	return mix(cfg.color, 0xFFFFFF, (L - 0.62) / 0.38 * 0.5);
}

// ---- the badge: 72 x 72 native; Masser centre (27,31) r 15, Secunda (51,49) r 8.5 ----------

function drawArt() {
	var m = artMc;
	m.clear();
	if (cfg.sky) {
		// night sky plate
		disc(m, 36, 36, 34, 0x0D1120, 78);
		var rim = tint(0xC8D6F0);
		circle(m, 36, 36, 33, rim, 80, 2);
		circle(m, 36, 36, 29.5, rim, 24, 1);
		// scattered stars
		if (cfg.stars) {
			sparkle(m, 14, 22, 1.4, tint(0xF2EBDD), 90);
			sparkle(m, 59, 17, 1.0, tint(0xF2EBDD), 70);
			sparkle(m, 63, 37, 0.8, tint(0xF2EBDD), 55);
			sparkle(m, 10, 49, 0.9, tint(0xF2EBDD), 60);
		}
	}
	// Masser (large, warm pale) and Secunda (small, cool blue), each in its current phase
	if (cfg.showM) {
		moonPhase(m, 27 + cfg.mxo, 31 + cfg.myo, 15 * cfg.ms / 100, pm, tint(0xE9E1D0), 0x161C2E);
	}
	if (cfg.showS) {
		moonPhase(m, 51 + cfg.sxo, 49 + cfg.syo, 8.5 * cfg.ss / 100, ps, tint(0xC8D6F0), 0x161C2E);
	}
}

// eight classical phases, same buckets as the plugin-side naming
function phaseName(p) {
	var i = Math.floor(p * 8 + 0.5);
	i %= 8;
	if (i < 0) {
		i += 8;
	}
	return NAMES[i];
}

function relayout() {
	drawArt();
	var nm = phaseName(cfg.lsrc == 1 ? ps : pm);
	tf.text = nm;
	tfSh.text = nm;
	var f = tf.getTextFormat();
	f.color = cfg.text;
	tf.setTextFormat(f);
	tf.setNewTextFormat(f);
	var g = tfSh.getTextFormat();
	g.color = 0x000000;
	tfSh.setTextFormat(g);
	tfSh.setNewTextFormat(g);
	// label placement: centred under the badge, with its own offset and size
	tf._xscale = cfg.ls;
	tf._yscale = cfg.ls;
	tfSh._xscale = cfg.ls;
	tfSh._yscale = cfg.ls;
	tf._x = 36 - tf._width / 2 + cfg.lx;
	tf._y = 74 + cfg.ly;
	tfSh._x = tf._x + 1;
	tfSh._y = tf._y + 1;
	tf._visible = cfg.label;
	tfSh._visible = cfg.label && cfg.shadow;

	// anchor on the visible stage (local box 72 wide; label may deepen it)
	var vr = Stage["visibleRect"];
	if (vr == undefined) {
		vr = {x:0, y:0, width:1280, height:720};
	}
	var W = 72;
	var H = 72;
	if (cfg.label && tf._y + tf._height > H) {
		H = tf._y + tf._height;
	}
	var sc = cfg.scale / 100;
	this._xscale = cfg.scale;
	this._yscale = cfg.scale;
	var px = 0;
	var py = 0;
	if (cfg.ax == 0) {
		px = vr.x + cfg.mx;
	} else if (cfg.ax == 1) {
		px = vr.x + (vr.width - W * sc) / 2 + cfg.mx;
	} else {
		px = vr.x + vr.width - W * sc - cfg.mx;
	}
	if (cfg.ay == 0) {
		py = vr.y + cfg.my;
	} else if (cfg.ay == 1) {
		py = vr.y + (vr.height - H * sc) / 2 + cfg.my;
	} else {
		py = vr.y + vr.height - H * sc - cfg.my;
	}
	this._x = px;
	this._y = py;
}

// ---- API called from the plugin -------------------------------------------------------------

function setConfig(ax, ay, mx, my, scale, alpha, sky, stars, custom, colorRgb, showM, mxo, myo, ms, showS, sxo, syo, ss, label, lsrc, lx, ly, ls, textRgb, shadow) {
	cfg.ax = ax; cfg.ay = ay; cfg.mx = mx; cfg.my = my;
	cfg.scale = scale; cfg.alpha = alpha;
	cfg.sky = sky; cfg.stars = stars; cfg.custom = custom; cfg.color = colorRgb;
	cfg.showM = showM; cfg.mxo = mxo; cfg.myo = myo; cfg.ms = ms;
	cfg.showS = showS; cfg.sxo = sxo; cfg.syo = syo; cfg.ss = ss;
	cfg.label = label; cfg.lsrc = lsrc; cfg.lx = lx; cfg.ly = ly; cfg.ls = ls;
	cfg.text = textRgb; cfg.shadow = shadow;
	relayout();
}

function setPhases(masserPhase, secundaPhase) {
	pm = masserPhase;
	ps = secundaPhase;
	relayout();
}

function setStrings(n0, n1, n2, n3, n4, n5, n6, n7) {
	NAMES = [n0, n1, n2, n3, n4, n5, n6, n7];
	relayout();
}

function setShown(v) {
	shown = v;
}

this.onEnterFrame = function() {
	var tgt = shown ? 100 : 0;
	if (curAlpha != tgt) {
		curAlpha += (tgt - curAlpha) * 0.2;
		if (Math.abs(tgt - curAlpha) < 1) {
			curAlpha = tgt;
		}
	}
	this._alpha = curAlpha * cfg.alpha / 100;
	this._visible = curAlpha > 0;
};

this._alpha = 0;
this._visible = false;
relayout();
ready = true;
