const LEAFSENSE_PACKET_SIZE = 156;
const LEAFSENSE_INVALID_TEMPERATURE = -32768;
const SENSOR_SIZE = 8;
const CHANNEL_COUNT = 6;
const SETTINGS_KEY = 'leafsense-thermal-card-settings-v2';

function crc32(bytes) {
  let crc = 0xffffffff;
  for (let i = 0; i < bytes.length; i += 1) {
    crc ^= bytes[i];
    for (let bit = 0; bit < 8; bit += 1) {
      const mask = -(crc & 1);
      crc = (crc >>> 1) ^ (0xedb88320 & mask);
    }
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function decodeBase64(encoded) {
  const binary = atob(encoded.trim());
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i += 1) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

function parseThermalFramePacket(encoded) {
  const bytes = decodeBase64(encoded);
  if (bytes.length !== LEAFSENSE_PACKET_SIZE) throw new Error(`Expected ${LEAFSENSE_PACKET_SIZE} bytes, got ${bytes.length}`);
  if (bytes[0] !== 0x4c || bytes[1] !== 0x53) throw new Error('Invalid LeafSense packet magic');
  if (bytes[2] !== 1) throw new Error(`Unsupported LeafSense protocol version ${bytes[2]}`);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (bytes[16] !== 8 || bytes[17] !== 8) throw new Error('Only 8x8 AMG8833 packets are supported');
  const expected = view.getUint32(bytes.length - 4, true);
  const actual = crc32(bytes.subarray(0, bytes.length - 4));
  if (expected !== actual) throw new Error('LeafSense packet CRC mismatch');
  const temperatures = [];
  for (let offset = 24; offset < 24 + 128; offset += 2) {
    const raw = view.getInt16(offset, true);
    temperatures.push(raw === LEAFSENSE_INVALID_TEMPERATURE ? Number.NaN : raw / 100);
  }
  return {
    version: bytes[2], sequence: view.getUint32(4, true), timestampMs: view.getUint32(8, true),
    calibrationRevision: view.getUint32(12, true), width: bytes[16], height: bytes[17],
    minimum: view.getInt16(20, true) / 100, maximum: view.getInt16(22, true) / 100, temperatures,
  };
}

const PALETTES = {
  thermal: [[0,[0,0,128]],[0.2,[0,128,255]],[0.4,[0,255,255]],[0.6,[0,255,0]],[0.8,[255,255,0]],[0.92,[255,80,0]],[1,[255,255,255]]],
  iron: [[0,[0,0,0]],[0.25,[70,0,90]],[0.5,[180,25,60]],[0.75,[255,140,20]],[1,[255,255,220]]],
  greyscale: [[0,[0,0,0]],[1,[255,255,255]]],
};

function clamp(value, low, high) { return Math.min(high, Math.max(low, value)); }
function round3(value) { return Number(value.toFixed(3)); }
function colourFor(value, paletteName = 'thermal') {
  const stops = PALETTES[paletteName] || PALETTES.thermal;
  const x = clamp(value, 0, 1);
  for (let i = 1; i < stops.length; i += 1) {
    if (x <= stops[i][0]) {
      const a = stops[i - 1], b = stops[i], t = (x - a[0]) / (b[0] - a[0]);
      return a[1].map((v, c) => Math.round(v + (b[1][c] - v) * t));
    }
  }
  return stops[stops.length - 1][1];
}
function clone(value) { return JSON.parse(JSON.stringify(value)); }
function distance(a, b) { return Math.hypot(a.x - b.x, a.y - b.y); }
function centroid(points) {
  if (!points.length) return { x: 4, y: 4 };
  return points.reduce((a, p) => ({ x: a.x + p.x / points.length, y: a.y + p.y / points.length }), { x: 0, y: 0 });
}
function rotatePoint(point, centre, radians) {
  const dx = point.x - centre.x, dy = point.y - centre.y, c = Math.cos(radians), s = Math.sin(radians);
  return { x: centre.x + dx * c - dy * s, y: centre.y + dx * s + dy * c };
}
function rectangleCorners(a, b) {
  return [{x:a.x,y:a.y},{x:b.x,y:a.y},{x:b.x,y:b.y},{x:a.x,y:b.y}];
}
function toDisplayTemperature(celsius, unit) { return unit === '°F' ? (celsius * 9 / 5) + 32 : celsius; }
function fromDisplayTemperature(value, unit) { return unit === '°F' ? (value - 32) * 5 / 9 : value; }

class LeafSenseThermalCard extends HTMLElement {
  setConfig(config) {
    if (!config.entity) throw new Error('LeafSense card requires entity');
    this.config = {
      title: 'LeafSense Thermal View', palette: 'thermal', scale_mode: 'auto', fixed_min: 15, fixed_max: 40,
      channel: 1, service: 'esphome.leafsense_set_measurement_channel', temperature_unit: 'auto', ...config,
    };
    this.settings = this.loadSettings();
    this.rois = Array.from({ length: CHANNEL_COUNT }, (_, i) => ({ channel: i + 1, type: 'disabled', points: [], source: 'disabled' }));
    this.selectedChannel = Number(this.config.channel) || 1;
    this.tool = 'select'; this.drag = null; this.history = []; this.future = []; this.pointerTemperature = null;
    this.renderShell();
  }

  set hass(hass) {
    this._hass = hass;
    if (!this.config || !this.root) return;
    const state = hass.states[this.config.entity];
    if (!state || !state.state || ['unknown', 'unavailable'].includes(state.state)) {
      this.frame = null; this.setStatus('Thermal frame unavailable', true); this.draw(); return;
    }
    try {
      this.frame = parseThermalFramePacket(state.state);
      this.setStatus(`Frame ${this.frame.sequence} • CRC OK • calibration r${this.frame.calibrationRevision}`);
      this.draw();
    } catch (error) { this.setStatus(error.message, true); }
  }

  getCardSize() { return 8; }
  loadSettings() {
    const defaults = { temperatureUnit: this.config.temperature_unit || 'auto', palette: this.config.palette || 'thermal', scaleMode: this.config.scale_mode || 'auto', fixedMin: Number(this.config.fixed_min), fixedMax: Number(this.config.fixed_max), interpolation: 'smooth', showGrid: true, showLegend: true, showLabels: true, snapGrid: false, rotationSnap: 15, showCoordinates: true };
    try { return { ...defaults, ...JSON.parse(localStorage.getItem(SETTINGS_KEY) || '{}') }; } catch (_) { return defaults; }
  }
  saveSettings() { localStorage.setItem(SETTINGS_KEY, JSON.stringify(this.settings)); this.draw(); }
  displayUnit() {
    if (this.settings.temperatureUnit === 'celsius') return '°C';
    if (this.settings.temperatureUnit === 'fahrenheit') return '°F';
    const configured = this._hass?.config?.unit_system?.temperature;
    return configured === '°F' ? '°F' : '°C';
  }

  renderShell() {
    this.innerHTML = '';
    this.root = document.createElement('ha-card');
    this.root.innerHTML = `
      <style>
        *{box-sizing:border-box}.wrap{padding:14px}.header{display:flex;align-items:center;gap:8px;margin-bottom:10px}.title{font-size:20px;font-weight:650;flex:1}.iconbtn,.toolbtn,.primary,.danger{border:1px solid var(--divider-color);background:var(--card-background-color);color:var(--primary-text-color);border-radius:9px;min-height:38px;padding:7px 10px;cursor:pointer}.iconbtn{min-width:40px}.toolbtn.active{background:var(--primary-color);color:var(--text-primary-color);border-color:var(--primary-color)}.primary{background:var(--primary-color);color:var(--text-primary-color)}.danger{color:var(--error-color)}.layout{position:relative}.stage{position:relative;aspect-ratio:1;max-width:680px;margin:auto;background:#111;border-radius:12px;overflow:hidden;touch-action:none}.stage canvas{position:absolute;inset:0;width:100%;height:100%}.heat.smooth{image-rendering:auto}.heat.pixelated{image-rendering:pixelated}.editbar{display:none;grid-template-columns:repeat(auto-fit,minmax(82px,1fr));gap:7px;margin:10px auto 0;max-width:680px}.editing .editbar{display:grid}.channelbar{display:flex;gap:8px;align-items:center;margin:10px auto 0;max-width:680px}.channelbar select{flex:1;min-height:40px;border-radius:9px;border:1px solid var(--divider-color);background:var(--card-background-color);color:var(--primary-text-color);padding:7px}.legendrow{max-width:680px;margin:9px auto 0}.legend{height:13px;border-radius:7px}.legendlabels{display:flex;justify-content:space-between;font-size:12px;color:var(--secondary-text-color);margin-top:3px}.statusrow{display:grid;grid-template-columns:repeat(3,1fr);gap:7px;max-width:680px;margin:9px auto 0;font-size:12px;color:var(--secondary-text-color)}.statusrow span:nth-child(2){text-align:center}.statusrow span:last-child{text-align:right}.status.error{color:var(--error-color)}.settings{position:absolute;z-index:5;right:0;top:0;width:min(360px,95%);max-height:100%;overflow:auto;background:var(--card-background-color);border:1px solid var(--divider-color);border-radius:12px;padding:14px;box-shadow:0 8px 30px rgba(0,0,0,.35);display:none}.settings.open{display:block}.settings h3{margin:0 0 10px}.settings details{border-top:1px solid var(--divider-color);padding:9px 0}.settings summary{font-weight:600;cursor:pointer}.field{display:grid;grid-template-columns:1fr 145px;gap:8px;align-items:center;margin-top:8px}.field input,.field select{width:100%;min-height:36px;border-radius:7px;border:1px solid var(--divider-color);background:var(--card-background-color);color:var(--primary-text-color);padding:6px}.settings-actions{display:flex;gap:8px;margin-top:12px}.hint{font-size:12px;color:var(--secondary-text-color);margin-top:8px}
      </style>
      <div class="wrap"><div class="header"><div class="title"></div><button class="iconbtn editToggle" title="Edit regions">✎</button><button class="iconbtn settingsToggle" title="Settings">⚙</button></div>
      <div class="layout"><div class="stage"><canvas class="heat"></canvas><canvas class="overlay"></canvas></div>
      <aside class="settings"><h3>LeafSense settings</h3>
        <details open><summary>Display</summary>
          <label class="field"><span>Temperature unit</span><select data-setting="temperatureUnit"><option value="auto">Auto</option><option value="celsius">Celsius</option><option value="fahrenheit">Fahrenheit</option></select></label>
          <label class="field"><span>Palette</span><select data-setting="palette"><option value="thermal">Thermal</option><option value="iron">Iron</option><option value="greyscale">Greyscale</option></select></label>
          <label class="field"><span>Interpolation</span><select data-setting="interpolation"><option value="smooth">Smooth</option><option value="pixelated">Pixelated</option></select></label>
          <label class="field"><span>Grid</span><input data-setting="showGrid" type="checkbox"></label><label class="field"><span>Legend</span><input data-setting="showLegend" type="checkbox"></label><label class="field"><span>ROI labels</span><input data-setting="showLabels" type="checkbox"></label>
        </details>
        <details><summary>Temperature scale</summary>
          <label class="field"><span>Mode</span><select data-setting="scaleMode"><option value="auto">Automatic</option><option value="fixed">Fixed</option></select></label>
          <label class="field"><span>Minimum</span><input data-setting="fixedMin" type="number" step="0.1"></label><label class="field"><span>Maximum</span><input data-setting="fixedMax" type="number" step="0.1"></label>
        </details>
        <details><summary>ROI editor</summary>
          <label class="field"><span>Snap to grid</span><input data-setting="snapGrid" type="checkbox"></label><label class="field"><span>Rotation snap</span><select data-setting="rotationSnap"><option value="0">Off</option><option value="5">5°</option><option value="15">15°</option><option value="30">30°</option><option value="45">45°</option></select></label><label class="field"><span>Coordinates</span><input data-setting="showCoordinates" type="checkbox"></label>
        </details>
        <details><summary>Diagnostics</summary><div class="hint diagnostics">Waiting for frame…</div></details>
        <details><summary>Calibration</summary><div class="hint">Calibration remains sensor-wide and is applied in firmware before ROI processing. Use the existing calibration dashboard for gain, offset, reference, save and restore controls.</div></details>
        <div class="settings-actions"><button class="primary closeSettings">Done</button><button class="toolbtn resetSettings">Defaults</button></div>
      </aside></div>
      <div class="channelbar"><select class="channel"></select><button class="toolbtn duplicate">Duplicate</button></div>
      <div class="editbar"><button class="toolbtn" data-tool="select">Select</button><button class="toolbtn" data-tool="rectangle">Rectangle</button><button class="toolbtn" data-tool="polygon">Polygon</button><button class="toolbtn undo">Undo</button><button class="toolbtn redo">Redo</button><button class="danger delete">Delete</button><button class="primary save">Save</button><button class="toolbtn cancel">Cancel</button></div>
      <div class="legendrow"><div class="legend"></div><div class="legendlabels"><span class="legendMin"></span><span class="legendMax"></span></div></div>
      <div class="statusrow"><span class="cursor">Cursor —</span><span class="status"></span><span class="angle">Angle —</span></div></div>`;
    this.appendChild(this.root);
    this.root.querySelector('.title').textContent = this.config.title;
    this.overlay = this.root.querySelector('.overlay'); this.heat = this.root.querySelector('.heat');
    const channel = this.root.querySelector('.channel');
    for (let i = 1; i <= CHANNEL_COUNT; i += 1) { const o = document.createElement('option'); o.value = i; o.textContent = `Channel ${i}`; channel.appendChild(o); }
    channel.value = this.selectedChannel;
    channel.addEventListener('change', () => { this.selectedChannel = Number(channel.value); this.tool = 'select'; this.updateToolbar(); this.drawOverlay(); });
    this.root.querySelector('.editToggle').addEventListener('click', () => this.toggleEditing());
    this.root.querySelector('.settingsToggle').addEventListener('click', () => this.toggleSettings(true));
    this.root.querySelector('.closeSettings').addEventListener('click', () => this.toggleSettings(false));
    this.root.querySelector('.resetSettings').addEventListener('click', () => { localStorage.removeItem(SETTINGS_KEY); this.settings = this.loadSettings(); this.syncSettingsControls(); this.saveSettings(); });
    this.root.querySelectorAll('[data-tool]').forEach((b) => b.addEventListener('click', () => { this.tool = b.dataset.tool; this.updateToolbar(); }));
    this.root.querySelector('.undo').addEventListener('click', () => this.undo()); this.root.querySelector('.redo').addEventListener('click', () => this.redo());
    this.root.querySelector('.delete').addEventListener('click', () => this.deleteSelected()); this.root.querySelector('.save').addEventListener('click', () => this.applyRoi());
    this.root.querySelector('.cancel').addEventListener('click', () => this.cancelEdit()); this.root.querySelector('.duplicate').addEventListener('click', () => this.duplicateSelected());
    this.root.querySelectorAll('[data-setting]').forEach((el) => el.addEventListener('change', () => this.readSettingsControls()));
    this.overlay.addEventListener('pointerdown', (e) => this.pointerDown(e)); this.overlay.addEventListener('pointermove', (e) => this.pointerMove(e)); this.overlay.addEventListener('pointerup', (e) => this.pointerUp(e)); this.overlay.addEventListener('pointercancel', (e) => this.pointerUp(e));
    this.overlay.addEventListener('dblclick', () => { if (this.tool === 'polygon') this.tool = 'select'; this.updateToolbar(); this.drawOverlay(); });
    this.syncSettingsControls(); this.updateToolbar(); this.updateLegend();
  }

  toggleEditing(force) {
    const next = typeof force === 'boolean' ? force : !this.root.classList.contains('editing');
    this.root.classList.toggle('editing', next); this.editSnapshot = next ? clone(this.rois) : null; this.tool = 'select'; this.updateToolbar(); this.drawOverlay();
  }
  toggleSettings(open) { this.root.querySelector('.settings').classList.toggle('open', open); }
  syncSettingsControls() {
    this.root.querySelectorAll('[data-setting]').forEach((el) => { const value = this.settings[el.dataset.setting]; if (el.type === 'checkbox') el.checked = Boolean(value); else el.value = String(value); });
  }
  readSettingsControls() {
    this.root.querySelectorAll('[data-setting]').forEach((el) => { this.settings[el.dataset.setting] = el.type === 'checkbox' ? el.checked : (el.type === 'number' ? Number(el.value) : el.value); });
    this.settings.rotationSnap = Number(this.settings.rotationSnap); this.saveSettings(); this.updateLegend();
  }
  updateToolbar() { this.root.querySelectorAll('[data-tool]').forEach((b) => b.classList.toggle('active', b.dataset.tool === this.tool)); }
  pushHistory() { this.history.push(clone(this.rois)); if (this.history.length > 30) this.history.shift(); this.future = []; }
  undo() { if (!this.history.length) return; this.future.push(clone(this.rois)); this.rois = this.history.pop(); this.drawOverlay(); }
  redo() { if (!this.future.length) return; this.history.push(clone(this.rois)); this.rois = this.future.pop(); this.drawOverlay(); }
  cancelEdit() { if (this.editSnapshot) this.rois = clone(this.editSnapshot); this.toggleEditing(false); }
  deleteSelected() { this.pushHistory(); this.rois[this.selectedChannel - 1] = { channel: this.selectedChannel, type: 'disabled', points: [], source: 'disabled' }; this.drawOverlay(); }
  duplicateSelected() {
    const source = this.rois[this.selectedChannel - 1]; if (source.type === 'disabled') return;
    const target = this.rois.find((r) => r.type === 'disabled'); if (!target) { this.setStatus('All six channels are already in use', true); return; }
    this.pushHistory(); const moved = clone(source); moved.channel = target.channel; moved.points = moved.points.map((p) => ({ x: clamp(p.x + 0.25, 0, 8), y: clamp(p.y + 0.25, 0, 8) })); this.rois[target.channel - 1] = moved; this.selectedChannel = target.channel; this.root.querySelector('.channel').value = String(target.channel); this.drawOverlay();
  }

  setStatus(text, error = false) { if (!this.root) return; const el = this.root.querySelector('.status'); el.textContent = text; el.classList.toggle('error', error); }
  resizeCanvas(canvas) { const r = canvas.getBoundingClientRect(), dpr = window.devicePixelRatio || 1, w = Math.max(1, Math.round(r.width * dpr)), h = Math.max(1, Math.round(r.height * dpr)); if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; } return { w, h, dpr }; }
  updateLegend(min, max) {
    if (!this.root) return; const palette = this.settings.palette; const colours = (PALETTES[palette] || PALETTES.thermal).map((s) => `rgb(${s[1].join(',')}) ${s[0] * 100}%`).join(',');
    const row = this.root.querySelector('.legendrow'); row.style.display = this.settings.showLegend ? '' : 'none'; this.root.querySelector('.legend').style.background = `linear-gradient(90deg,${colours})`;
    if (Number.isFinite(min) && Number.isFinite(max)) { const unit = this.displayUnit(); this.root.querySelector('.legendMin').textContent = `${toDisplayTemperature(min, unit).toFixed(1)} ${unit}`; this.root.querySelector('.legendMax').textContent = `${toDisplayTemperature(max, unit).toFixed(1)} ${unit}`; }
  }
  draw() {
    if (!this.heat) return; const { w, h } = this.resizeCanvas(this.heat), ctx = this.heat.getContext('2d'); ctx.clearRect(0, 0, w, h); this.heat.className = `heat ${this.settings.interpolation}`;
    if (!this.frame) { ctx.fillStyle = '#111'; ctx.fillRect(0, 0, w, h); this.drawOverlay(); return; }
    let min = this.frame.minimum, max = this.frame.maximum; const unit = this.displayUnit();
    if (this.settings.scaleMode === 'fixed') { min = fromDisplayTemperature(Number(this.settings.fixedMin), unit); max = fromDisplayTemperature(Number(this.settings.fixedMax), unit); }
    if (!(max > min)) { min -= 0.5; max += 0.5; }
    const image = ctx.createImageData(w, h);
    for (let y = 0; y < h; y += 1) for (let x = 0; x < w; x += 1) {
      const sx = clamp(Math.round((x / Math.max(1, w - 1)) * 7), 0, 7), sy = clamp(Math.round((y / Math.max(1, h - 1)) * 7), 0, 7), temp = this.frame.temperatures[sy * 8 + sx], idx = (y * w + x) * 4;
      if (Number.isFinite(temp)) { const c = colourFor((temp - min) / (max - min), this.settings.palette); image.data[idx] = c[0]; image.data[idx + 1] = c[1]; image.data[idx + 2] = c[2]; image.data[idx + 3] = 255; } else image.data[idx + 3] = 255;
    }
    ctx.putImageData(image, 0, 0); this.updateLegend(min, max); this.root.querySelector('.diagnostics').textContent = `Frame ${this.frame.sequence}\nProtocol ${this.frame.version}\nCRC OK\nCalibration revision ${this.frame.calibrationRevision}\nSensor ${this.frame.width}×${this.frame.height}`; this.drawOverlay();
  }
  snapPoint(p) { return this.settings.snapGrid ? { x: clamp(Math.round(p.x), 0, 8), y: clamp(Math.round(p.y), 0, 8) } : { x: clamp(p.x, 0, 8), y: clamp(p.y, 0, 8) }; }
  pointFromEvent(e) { const r = this.overlay.getBoundingClientRect(); return this.snapPoint({ x: ((e.clientX - r.left) / r.width) * 8, y: ((e.clientY - r.top) / r.height) * 8 }); }
  selectedRoi() { return this.rois[this.selectedChannel - 1]; }
  nearestVertex(roi, p, radius = 0.3) { let best = -1, bestD = radius; roi.points.forEach((v, i) => { const d = distance(v, p); if (d < bestD) { best = i; bestD = d; } }); return best; }
  rotationHandle(roi) { const c = centroid(roi.points); const top = roi.points.reduce((best, p) => p.y < best.y ? p : best, roi.points[0] || c); return { x: c.x, y: clamp(top.y - 0.55, 0, 8) }; }
  pointInPolygon(point, points) { let inside = false; for (let i = 0, j = points.length - 1; i < points.length; j = i++) { const a = points[i], b = points[j]; if (((a.y > point.y) !== (b.y > point.y)) && point.x < ((b.x - a.x) * (point.y - a.y)) / ((b.y - a.y) || 1e-9) + a.x) inside = !inside; } return inside; }

  pointerDown(e) {
    const p = this.pointFromEvent(e); this.updateCursor(p); if (!this.root.classList.contains('editing')) return;
    this.overlay.setPointerCapture(e.pointerId); const roi = this.selectedRoi(); this.pushHistory();
    if (this.tool === 'rectangle') { this.rois[this.selectedChannel - 1] = { channel: this.selectedChannel, type: 'rectangle', source: 'rectangle', points: [p, p], angle: 0 }; this.drag = { kind: 'draw-rectangle', start: p }; }
    else if (this.tool === 'polygon') { if (roi.source !== 'polygon') this.rois[this.selectedChannel - 1] = { channel: this.selectedChannel, type: 'polygon', source: 'polygon', points: [] }; this.selectedRoi().points.push(p); this.history.pop(); }
    else if (this.tool === 'select' && roi.type !== 'disabled') {
      const handle = this.rotationHandle(roi), vertex = this.nearestVertex(roi, p);
      if (distance(handle, p) < 0.42) { const c = centroid(roi.points); this.drag = { kind: 'rotate', centre: c, startAngle: Math.atan2(p.y - c.y, p.x - c.x), original: clone(roi.points) }; }
      else if (vertex >= 0) this.drag = { kind: 'vertex', index: vertex };
      else if (this.pointInPolygon(p, this.roiPolygon(roi))) this.drag = { kind: 'move', start: p, original: clone(roi.points) };
      else this.history.pop();
    } else this.history.pop();
    this.drawOverlay();
  }
  pointerMove(e) {
    const p = this.pointFromEvent(e); this.updateCursor(p); if (!this.drag) return; const roi = this.selectedRoi();
    if (this.drag.kind === 'draw-rectangle') roi.points[1] = p;
    else if (this.drag.kind === 'vertex') roi.points[this.drag.index] = p;
    else if (this.drag.kind === 'move') { const dx = p.x - this.drag.start.x, dy = p.y - this.drag.start.y; roi.points = this.drag.original.map((v) => ({ x: clamp(v.x + dx, 0, 8), y: clamp(v.y + dy, 0, 8) })); }
    else if (this.drag.kind === 'rotate') { let angle = Math.atan2(p.y - this.drag.centre.y, p.x - this.drag.centre.x) - this.drag.startAngle; const snap = Number(this.settings.rotationSnap); if (snap > 0) angle = Math.round(angle / (snap * Math.PI / 180)) * snap * Math.PI / 180; roi.points = this.drag.original.map((v) => rotatePoint(v, this.drag.centre, angle)).map(this.snapPoint.bind(this)); roi.type = 'polygon'; roi.angle = angle * 180 / Math.PI; }
    this.drawOverlay();
  }
  pointerUp(e) { if (this.drag?.kind === 'draw-rectangle') { const roi = this.selectedRoi(); if (distance(roi.points[0], roi.points[1]) < 0.15) this.undo(); } this.drag = null; try { this.overlay.releasePointerCapture(e.pointerId); } catch (_) {} this.drawOverlay(); }
  updateCursor(p) {
    if (!this.frame) return; const x = clamp(Math.floor(p.x), 0, 7), y = clamp(Math.floor(p.y), 0, 7), value = this.frame.temperatures[y * 8 + x], unit = this.displayUnit();
    this.root.querySelector('.cursor').textContent = Number.isFinite(value) ? `Pixel ${x},${y}: ${toDisplayTemperature(value, unit).toFixed(1)} ${unit}` : `Pixel ${x},${y}: invalid`;
  }
  roiPolygon(roi) { return roi.source === 'rectangle' && roi.points.length === 2 ? rectangleCorners(roi.points[0], roi.points[1]) : roi.points; }
  drawOverlay() {
    if (!this.overlay) return; const { w, h } = this.resizeCanvas(this.overlay), ctx = this.overlay.getContext('2d'); ctx.clearRect(0, 0, w, h);
    if (this.settings.showGrid) { ctx.strokeStyle = 'rgba(255,255,255,.18)'; ctx.lineWidth = 1; for (let i = 1; i < 8; i += 1) { ctx.beginPath(); ctx.moveTo(i / 8 * w, 0); ctx.lineTo(i / 8 * w, h); ctx.stroke(); ctx.beginPath(); ctx.moveTo(0, i / 8 * h); ctx.lineTo(w, i / 8 * h); ctx.stroke(); } }
    for (const roi of this.rois) {
      const poly = this.roiPolygon(roi); if (roi.type === 'disabled' || poly.length < 2) continue; const selected = roi.channel === this.selectedChannel;
      ctx.strokeStyle = selected ? '#fff' : 'rgba(255,255,255,.68)'; ctx.fillStyle = selected ? 'rgba(255,255,255,.14)' : 'rgba(255,255,255,.06)'; ctx.lineWidth = selected ? 3 : 2; ctx.beginPath(); poly.forEach((p, i) => i ? ctx.lineTo(p.x / 8 * w, p.y / 8 * h) : ctx.moveTo(p.x / 8 * w, p.y / 8 * h)); if (poly.length > 2) ctx.closePath(); ctx.fill(); ctx.stroke();
      if (this.settings.showLabels) { const c = centroid(poly); ctx.fillStyle = '#fff'; ctx.font = 'bold 14px sans-serif'; ctx.fillText(String(roi.channel), c.x / 8 * w + 5, c.y / 8 * h - 5); }
      if (selected && this.root.classList.contains('editing') && this.tool === 'select') {
        ctx.fillStyle = '#fff'; poly.forEach((p) => { ctx.beginPath(); ctx.arc(p.x / 8 * w, p.y / 8 * h, 6, 0, Math.PI * 2); ctx.fill(); }); const rh = this.rotationHandle({ points: poly }); const c = centroid(poly); ctx.strokeStyle = '#fff'; ctx.beginPath(); ctx.moveTo(c.x / 8 * w, c.y / 8 * h); ctx.lineTo(rh.x / 8 * w, rh.y / 8 * h); ctx.stroke(); ctx.beginPath(); ctx.arc(rh.x / 8 * w, rh.y / 8 * h, 8, 0, Math.PI * 2); ctx.fill();
      }
    }
    const roi = this.selectedRoi(); this.root.querySelector('.angle').textContent = roi.angle ? `Angle ${roi.angle.toFixed(0)}°` : 'Angle 0°';
  }

  servicePayload(roi) {
    if (roi.type === 'disabled') return { channel: roi.channel, type: 'disabled', points: '[]' };
    const rotatedRectangle = roi.source === 'rectangle' && roi.type === 'polygon';
    const type = rotatedRectangle ? 'polygon' : roi.type;
    const rawPoints = roi.source === 'rectangle' && roi.type === 'rectangle' ? roi.points : this.roiPolygon(roi);
    return { channel: roi.channel, type, points: JSON.stringify(rawPoints.map((p) => ({ x: round3(p.x), y: round3(p.y) }))) };
  }
  async applyRoi() {
    const roi = this.selectedRoi(); const poly = this.roiPolygon(roi);
    if (!this._hass) { this.setStatus('Home Assistant connection unavailable', true); return; }
    if (roi.type === 'rectangle' && roi.points.length !== 2) { this.setStatus('Draw a rectangle first', true); return; }
    if (roi.type === 'polygon' && poly.length < 3) { this.setStatus('Polygon requires at least three points', true); return; }
    const [domain, service] = this.config.service.split('.');
    try { await this._hass.callService(domain, service, this.servicePayload(roi)); this.setStatus(`Channel ${roi.channel} saved`); this.editSnapshot = clone(this.rois); }
    catch (error) { this.setStatus(`ROI service failed: ${error.message || error}`, true); }
  }
}

customElements.define('leafsense-thermal-card', LeafSenseThermalCard);
window.LeafSenseThermal = { parseThermalFramePacket, crc32, colourFor, rectangleCorners, rotatePoint, toDisplayTemperature, fromDisplayTemperature };
window.customCards = window.customCards || [];
window.customCards.push({ type: 'leafsense-thermal-card', name: 'LeafSense Thermal Card', description: 'Live AMG8833 thermal image with six editable and rotatable measurement channels' });
