const LEAFSENSE_PACKET_SIZE = 156; const LEAFSENSE_INVALID_TEMPERATURE =
-32768; const SENSOR_SIZE = 8; const CHANNEL_COUNT = 6; const
SETTINGS_KEY = 'leafsense-thermal-card-settings-v3';

function crc32(bytes) { let crc = 0xffffffff; for (let i = 0; i <
bytes.length; i += 1) { crc ^= bytes[i]; for (let bit = 0; bit < 8; bit
+= 1) { const mask = -(crc & 1); crc = (crc >>> 1) ^ (0xedb88320 &
mask); } } return (crc ^ 0xffffffff) >>> 0; }

function decodeBase64(encoded) { const binary = atob(encoded.trim());
const bytes = new Uint8Array(binary.length); for (let i = 0; i <
binary.length; i += 1) bytes[i] = binary.charCodeAt(i); return bytes; }

function parseThermalFramePacket(encoded) {
  const bytes = decodeBase64(encoded);

  if (bytes.length !== LEAFSENSE_PACKET_SIZE) {
    throw new Error(
      `Expected ${LEAFSENSE_PACKET_SIZE} bytes, got ${bytes.length}`,
    );
  }

  if (bytes[0] !== 0x4c || bytes[1] !== 0x53) {
    throw new Error('Invalid LeafSense packet magic');
  }

  if (bytes[2] !== 1) {
    throw new Error(`Unsupported LeafSense protocol version ${bytes[2]}`);
  }

  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);

  if (bytes[16] !== 8 || bytes[17] !== 8) {
    throw new Error('Only 8x8 AMG8833 packets are supported');
  }

  const expected = view.getUint32(bytes.length - 4, true);
  const actual = crc32(bytes.subarray(0, bytes.length - 4));

  if (expected !== actual) {
    throw new Error('LeafSense packet CRC mismatch');
  }

  const temperatures = [];
  for (let offset = 24; offset < 24 + 128; offset += 2) {
    const raw = view.getInt16(offset, true);
    temperatures.push(
      raw === LEAFSENSE_INVALID_TEMPERATURE ? Number.NaN : raw / 100,
    );
  }

  const minimumRaw = view.getInt16(20, true);
  const maximumRaw = view.getInt16(22, true);

  return {
    version: bytes[2],
    sequence: view.getUint32(4, true),
    timestampMs: view.getUint32(8, true),
    calibrationRevision: view.getUint32(12, true),
    width: bytes[16],
    height: bytes[17],
    minimum:
      minimumRaw === LEAFSENSE_INVALID_TEMPERATURE
        ? Number.NaN
        : minimumRaw / 100,
    maximum:
      maximumRaw === LEAFSENSE_INVALID_TEMPERATURE
        ? Number.NaN
        : maximumRaw / 100,
    temperatures,
  };
}

const PALETTES = { thermal:
[[0,[0,0,128]],[0.2,[0,128,255]],[0.4,[0,255,255]],[0.6,[0,255,0]],[0.8,[255,255,0]],[0.92,[255,80,0]],[1,[255,255,255]]],
iron:
[[0,[0,0,0]],[0.25,[70,0,90]],[0.5,[180,25,60]],[0.75,[255,140,20]],[1,[255,255,220]]],
greyscale: [[0,[0,0,0]],[1,[255,255,255]]], };

function clamp(value, low, high) { return Math.min(high, Math.max(low,
value)); } function round3(value) { return Number(value.toFixed(3)); }
function colourFor(value, paletteName = 'thermal') { const stops =
PALETTES[paletteName] || PALETTES.thermal; const x = clamp(value, 0, 1);
for (let i = 1; i < stops.length; i += 1) { if (x <= stops[i][0]) {
const a = stops[i - 1], b = stops[i], t = (x - a[0]) / (b[0] - a[0]);
return a[1].map((v, c) => Math.round(v + (b[1][c] - v) * t)); } } return
stops[stops.length - 1][1]; } function clone(value) { return
JSON.parse(JSON.stringify(value)); } function distance(a, b) { return
Math.hypot(a.x - b.x, a.y - b.y); } function centroid(points) { if
(!points.length) return { x: 4, y: 4 }; return points.reduce((a, p) =>
({ x: a.x + p.x / points.length, y: a.y + p.y / points.length }), { x:
0, y: 0 }); } function rotatePoint(point, centre, radians) { const dx =
point.x - centre.x, dy = point.y - centre.y, c = Math.cos(radians), s =
Math.sin(radians); return { x: centre.x + dx * c - dy * s, y: centre.y +
dx * s + dy * c }; } function rectangleCorners(a, b) { return
[{x:a.x,y:a.y},{x:b.x,y:a.y},{x:b.x,y:b.y},{x:a.x,y:b.y}]; } function
toDisplayTemperature(celsius, unit) { return unit === '°F' ? (celsius *
9 / 5) + 32 : celsius; } function fromDisplayTemperature(value, unit) {
return unit === '°F' ? (value - 32) * 5 / 9 : value; }

function toDisplayTemperatureDifference(celsiusDifference, unit) {
  return unit === '°F' ? celsiusDifference * 9 / 5 : celsiusDifference;
}

class LeafSenseThermalCard extends HTMLElement { setConfig(config) { if
(!config.entity) throw new Error('LeafSense card requires entity');

    this.config = {
      title: 'LeafSense Thermal View',
      palette: 'thermal',
      scale_mode: 'auto',
      fixed_min: 15,
      fixed_max: 40,
      channel: 1,
      service_prefix: 'esphome.leafsense_amg8833',
      temperature_unit: 'auto',
      calibration_entities: {},
      channel_entities: {},
      ...config,
    };

    this.settings = this.loadSettings();
    this.rois = Array.from(
      { length: CHANNEL_COUNT },
      (_, index) => ({
        channel: index + 1,
        type: 'disabled',
        points: [],
        source: 'disabled',
      }),
    );

    this.selectedChannel = clamp(Number(this.config.channel) || 1, 1, CHANNEL_COUNT);
    this.tool = 'select';
    this.drag = null;
    this.history = [];
    this.future = [];
    this.frame = null;
    this.editSnapshot = clone(this.rois);
    this.pendingSave = null;
    this.renderShell();

}

set hass(hass) { this._hass = hass; if (!this.config || !this.root)
return;

    this.syncCalibrationControls();
    this.updateChannelTabs();

    const state = hass.states[this.config.entity];
    if (!state || !state.state || ['unknown', 'unavailable'].includes(state.state)) {
      this.frame = null;
      this.setStatus('Thermal frame unavailable', true);
      this.draw();
      return;
    }

    try {
      this.frame = parseThermalFramePacket(state.state);
      this.setStatus(
        `Frame ${this.frame.sequence} • CRC OK • calibration r${this.frame.calibrationRevision}`,
      );
      this.draw();
    } catch (error) {
      this.setStatus(error.message, true);
    }

}

getCardSize() { return 9; }

loadSettings() { const defaults = { temperatureUnit:
this.config.temperature_unit || 'auto', palette: this.config.palette ||
'thermal', scaleMode: this.config.scale_mode || 'auto', fixedMin:
Number(this.config.fixed_min), fixedMax: Number(this.config.fixed_max),
interpolation: 'pixelated', showGrid: true, showLegend: true,
showLabels: true, snapGrid: false, rotationSnap: 15, showCoordinates:
true, };

    try {
      return {
        ...defaults,
        ...JSON.parse(localStorage.getItem(SETTINGS_KEY) || '{}'),
      };
    } catch (_) {
      return defaults;
    }

}

saveSettings() { localStorage.setItem(SETTINGS_KEY,
JSON.stringify(this.settings)); this.draw(); }

displayUnit() { if (this.settings.temperatureUnit === 'celsius') return
'°C'; if (this.settings.temperatureUnit === 'fahrenheit') return '°F';

    const configured = this._hass?.config?.unit_system?.temperature;
    return configured === '°F' ? '°F' : '°C';

}

renderShell() { this.innerHTML = '';

    this.root = document.createElement('ha-card');
    this.root.classList.add('editing');
    this.root.innerHTML = `
      <style>
        *{box-sizing:border-box}
        .wrap{padding:14px}
        .header{display:flex;align-items:center;gap:8px;margin-bottom:10px}
        .title{font-size:20px;font-weight:650;flex:1}
        .iconbtn,.toolbtn,.primary,.danger,.channelbtn{
          border:1px solid var(--divider-color);
          background:var(--card-background-color);
          color:var(--primary-text-color);
          border-radius:9px;
          min-height:38px;
          padding:7px 10px;
          cursor:pointer
        }
        .iconbtn{min-width:40px}
        .toolbtn.active,.channelbtn.active{
          background:var(--primary-color);
          color:var(--text-primary-color);
          border-color:var(--primary-color)
        }
        .channelbtn.used:not(.active){border-color:var(--primary-color)}
        .primary{background:var(--primary-color);color:var(--text-primary-color)}
        .danger{color:var(--error-color)}
        .layout{position:relative}
        .stage{
          position:relative;
          aspect-ratio:1;
          max-width:680px;
          margin:auto;
          background:#111;
          border-radius:12px;
          overflow:hidden;
          touch-action:none
        }
        .stage canvas{position:absolute;inset:0;width:100%;height:100%}
        .heat.smooth{image-rendering:auto}
        .heat.pixelated{image-rendering:pixelated}
        .channelbar,.editbar{
          display:grid;
          gap:7px;
          max-width:680px;
          margin:10px auto 0
        }
        .channelbar{grid-template-columns:repeat(6,1fr)}
        .editbar{grid-template-columns:repeat(auto-fit,minmax(92px,1fr))}
        .editorhint{
          max-width:680px;
          margin:7px auto 0;
          font-size:12px;
          color:var(--secondary-text-color)
        }
        .legendrow{max-width:680px;margin:9px auto 0}
        .legend{height:13px;border-radius:7px}
        .legendlabels{
          display:flex;
          justify-content:space-between;
          font-size:12px;
          color:var(--secondary-text-color);
          margin-top:3px
        }
        .statusrow{
          display:grid;
          grid-template-columns:repeat(3,1fr);
          gap:7px;
          max-width:680px;
          margin:9px auto 0;
          font-size:12px;
          color:var(--secondary-text-color)
        }
        .statusrow span:nth-child(2){text-align:center}
        .statusrow span:last-child{text-align:right}
        .status.error{color:var(--error-color)}
        .settings{
          position:absolute;
          z-index:5;
          right:0;
          top:0;
          width:min(390px,96%);
          max-height:100%;
          overflow:auto;
          background:var(--card-background-color);
          border:1px solid var(--divider-color);
          border-radius:12px;
          padding:14px;
          box-shadow:0 8px 30px rgba(0,0,0,.35);
          display:none
        }
        .settings.open{display:block}
        .settings h3{margin:0 0 10px}
        .settings details{border-top:1px solid var(--divider-color);padding:9px 0}
        .settings summary{font-weight:600;cursor:pointer}
        .field{
          display:grid;
          grid-template-columns:1fr 145px;
          gap:8px;
          align-items:center;
          margin-top:8px
        }
        .field input,.field select{
          width:100%;
          min-height:36px;
          border-radius:7px;
          border:1px solid var(--divider-color);
          background:var(--card-background-color);
          color:var(--primary-text-color);
          padding:6px
        }
        .readonly{
          min-height:36px;
          display:flex;
          align-items:center;
          justify-content:flex-end;
          color:var(--secondary-text-color)
        }
        .calibration-actions{
          display:grid;
          grid-template-columns:repeat(3,1fr);
          gap:7px;
          margin-top:10px
        }
        .settings-actions{display:flex;gap:8px;margin-top:12px}
        .hint{font-size:12px;color:var(--secondary-text-color);margin-top:8px}
      </style>

      <div class="wrap">
        <div class="header">
          <div class="title"></div>
          <button class="iconbtn settingsToggle" title="Settings">⚙</button>
        </div>

        <div class="layout">
          <div class="stage">
            <canvas class="heat"></canvas>
            <canvas class="overlay"></canvas>
          </div>

          <aside class="settings">
            <h3>LeafSense settings</h3>

            <details open>
              <summary>Display</summary>
              <label class="field">
                <span>Temperature unit</span>
                <select data-setting="temperatureUnit">
                  <option value="auto">Auto</option>
                  <option value="celsius">Celsius</option>
                  <option value="fahrenheit">Fahrenheit</option>
                </select>
              </label>
              <label class="field">
                <span>Palette</span>
                <select data-setting="palette">
                  <option value="thermal">Thermal</option>
                  <option value="iron">Iron</option>
                  <option value="greyscale">Greyscale</option>
                </select>
              </label>
              <label class="field">
                <span>Interpolation</span>
                <select data-setting="interpolation">
                  <option value="pixelated">Pixelated 8×8</option>
                  <option value="smooth">Smooth</option>
                </select>
              </label>
              <label class="field"><span>Grid</span><input data-setting="showGrid" type="checkbox"></label>
              <label class="field"><span>Legend</span><input data-setting="showLegend" type="checkbox"></label>
              <label class="field"><span>ROI labels</span><input data-setting="showLabels" type="checkbox"></label>
            </details>

            <details>
              <summary>Temperature scale</summary>
              <label class="field">
                <span>Mode</span>
                <select data-setting="scaleMode">
                  <option value="auto">Automatic</option>
                  <option value="fixed">Fixed</option>
                </select>
              </label>
              <label class="field"><span>Minimum</span><input data-setting="fixedMin" type="number" step="0.1"></label>
              <label class="field"><span>Maximum</span><input data-setting="fixedMax" type="number" step="0.1"></label>
            </details>

            <details>
              <summary>ROI editor</summary>
              <label class="field"><span>Snap to grid</span><input data-setting="snapGrid" type="checkbox"></label>
              <label class="field">
                <span>Rotation snap</span>
                <select data-setting="rotationSnap">
                  <option value="0">Off</option>
                  <option value="5">5°</option>
                  <option value="15">15°</option>
                  <option value="30">30°</option>
                  <option value="45">45°</option>
                </select>
              </label>
              <label class="field"><span>Coordinates</span><input data-setting="showCoordinates" type="checkbox"></label>
              <div class="hint">
                Rectangle: drag once to create and save. Polygon: click vertices, then click the first point or double-click to finish and save. Select: drag shapes, corners, or the rotation handle; changes save automatically.
              </div>
            </details>

            <details open>
              <summary>Calibration</summary>
              <label class="field">
                <span>Gain</span>
                <input class="calGain" type="number" step="0.001">
              </label>
              <label class="field">
                <span>Offset</span>
                <input class="calOffset" type="number" step="0.1">
              </label>
              <label class="field calReferenceRow">
                <span>Reference temperature</span>
                <input class="calReference" type="number" step="0.1">
              </label>
              <div class="field"><span>Current reading</span><span class="readonly calCurrent">—</span></div>
              <div class="field calDifferenceRow"><span>Difference</span><span class="readonly calDifference">—</span></div>
              <div class="field"><span>Revision</span><span class="readonly calRevision">—</span></div>
              <div class="calibration-actions">
                <button class="primary calApply">Apply</button>
                <button class="toolbtn calSave">Save</button>
                <button class="danger calDefaults">Defaults</button>
              </div>
              <div class="hint calStatus">Calibration entities are detected automatically.</div>
            </details>

            <details>
              <summary>Diagnostics</summary>
              <div class="hint diagnostics">Waiting for frame…</div>
            </details>

            <div class="settings-actions">
              <button class="primary closeSettings">Done</button>
              <button class="toolbtn resetSettings">Display defaults</button>
            </div>
          </aside>
        </div>

        <div class="channelbar"></div>

        <div class="editbar">
          <button class="toolbtn" data-tool="select">Select / Move</button>
          <button class="toolbtn" data-tool="rectangle">Draw Rectangle</button>
          <button class="toolbtn" data-tool="polygon">Draw Polygon</button>
          <button class="toolbtn undo">Undo</button>
          <button class="toolbtn redo">Redo</button>
          <button class="danger delete">Disable ROI</button>
        </div>

        <div class="editorhint">
          Choose a channel, then draw directly on the thermal image. ROI changes are sent to the ESP32 automatically.
        </div>

        <div class="legendrow">
          <div class="legend"></div>
          <div class="legendlabels"><span class="legendMin"></span><span class="legendMax"></span></div>
        </div>

        <div class="statusrow">
          <span class="cursor">Cursor —</span>
          <span class="status"></span>
          <span class="angle">Angle —</span>
        </div>
      </div>
    `;

    this.appendChild(this.root);
    this.root.querySelector('.title').textContent = this.config.title;

    this.overlay = this.root.querySelector('.overlay');
    this.heat = this.root.querySelector('.heat');

    this.buildChannelTabs();

    this.root.querySelector('.settingsToggle').addEventListener(
      'click',
      () => this.toggleSettings(true),
    );
    this.root.querySelector('.closeSettings').addEventListener(
      'click',
      () => this.toggleSettings(false),
    );
    this.root.querySelector('.resetSettings').addEventListener('click', () => {
      localStorage.removeItem(SETTINGS_KEY);
      this.settings = this.loadSettings();
      this.syncSettingsControls();
      this.saveSettings();
    });

    this.root.querySelectorAll('[data-tool]').forEach((button) => {
      button.addEventListener('click', () => {
        this.tool = button.dataset.tool;
        this.updateToolbar();
        this.drawOverlay();
      });
    });

    this.root.querySelector('.undo').addEventListener('click', () => this.undo());
    this.root.querySelector('.redo').addEventListener('click', () => this.redo());
    this.root.querySelector('.delete').addEventListener('click', () => this.deleteSelected());

    this.root.querySelectorAll('[data-setting]').forEach((element) => {
      element.addEventListener('change', () => this.readSettingsControls());
    });

    this.root.querySelector('.calGain').addEventListener('change', (event) => {
      this.setCalibrationNumber('gain', Number(event.target.value));
    });
    this.root.querySelector('.calOffset').addEventListener('change', (event) => {
      this.setCalibrationNumber('offset', Number(event.target.value));
    });
    this.root.querySelector('.calReference').addEventListener('change', (event) => {
      this.setCalibrationNumber('reference', Number(event.target.value));
    });
    this.root.querySelector('.calApply').addEventListener('click', () => {
      this.pressCalibrationButton('apply');
    });
    this.root.querySelector('.calSave').addEventListener('click', () => {
      this.pressCalibrationButton('save');
    });
    this.root.querySelector('.calDefaults').addEventListener('click', () => {
      this.pressCalibrationButton('defaults');
    });

    this.overlay.addEventListener('pointerdown', (event) => this.pointerDown(event));
    this.overlay.addEventListener('pointermove', (event) => this.pointerMove(event));
    this.overlay.addEventListener('pointerup', (event) => this.pointerUp(event));
    this.overlay.addEventListener('pointercancel', (event) => this.pointerUp(event));
    this.overlay.addEventListener('dblclick', (event) => {
      event.preventDefault();
      if (this.tool === 'polygon') this.finishPolygon();
    });

    this.syncSettingsControls();
    this.syncCalibrationControls();
    this.updateToolbar();
    this.updateLegend();

}

buildChannelTabs() { const container =
this.root.querySelector('.channelbar'); container.innerHTML = '';

    for (let channel = 1; channel <= CHANNEL_COUNT; channel += 1) {
      const button = document.createElement('button');
      button.className = 'channelbtn';
      button.dataset.channel = String(channel);
      button.textContent = `ROI ${channel}`;
      button.addEventListener('click', () => {
        this.selectedChannel = channel;
        this.tool = 'select';
        this.updateToolbar();
        this.updateChannelTabs();
        this.drawOverlay();
      });
      container.appendChild(button);
    }

    this.updateChannelTabs();

}

updateChannelTabs() { if (!this.root) return;

    this.root.querySelectorAll('.channelbtn').forEach((button) => {
      const channel = Number(button.dataset.channel);
      const roi = this.rois[channel - 1];
      const stats = this.channelStats(channel);

      button.classList.toggle('active', channel === this.selectedChannel);
      button.classList.toggle(
        'used',
        roi?.type !== 'disabled' || stats.available,
      );

      const type =
        roi?.type === 'rectangle'
          ? 'Rect'
          : roi?.type === 'polygon'
            ? 'Poly'
            : stats.available
              ? 'Live'
              : 'Off';

      button.textContent = `${channel} · ${type}`;
    });

}

toggleSettings(open) {
this.root.querySelector('.settings').classList.toggle('open', open); if
(open) this.syncCalibrationControls(); }

syncSettingsControls() {
this.root.querySelectorAll('[data-setting]').forEach((element) => {
const value = this.settings[element.dataset.setting]; if (element.type
=== 'checkbox') element.checked = Boolean(value); else element.value =
String(value); }); }

readSettingsControls() {
this.root.querySelectorAll('[data-setting]').forEach((element) => {
this.settings[element.dataset.setting] = element.type === 'checkbox' ?
element.checked : element.type === 'number' ? Number(element.value) :
element.value; });

    this.settings.rotationSnap = Number(this.settings.rotationSnap);
    this.saveSettings();
    this.updateLegend();

}

updateToolbar() {
this.root.querySelectorAll('[data-tool]').forEach((button) => {
button.classList.toggle('active', button.dataset.tool === this.tool);
}); }

pushHistory() { this.history.push(clone(this.rois)); if
(this.history.length > 30) this.history.shift(); this.future = []; }

undo() { if (!this.history.length) return;
this.future.push(clone(this.rois)); this.rois = this.history.pop();
this.updateChannelTabs(); this.drawOverlay(); this.scheduleApplyRoi(); }

redo() { if (!this.future.length) return;
this.history.push(clone(this.rois)); this.rois = this.future.pop();
this.updateChannelTabs(); this.drawOverlay(); this.scheduleApplyRoi(); }

deleteSelected() { this.pushHistory(); this.rois[this.selectedChannel -
1] = { channel: this.selectedChannel, type: 'disabled', points: [],
source: 'disabled', }; this.updateChannelTabs(); this.drawOverlay();
this.applyRoi(); }

setStatus(text, error = false) { if (!this.root) return; const element =
this.root.querySelector('.status'); element.textContent = text;
element.classList.toggle('error', error); }

resizeCanvas(canvas) { const rect = canvas.getBoundingClientRect();
const dpr = window.devicePixelRatio || 1; const width = Math.max(1,
Math.round(rect.width * dpr)); const height = Math.max(1,
Math.round(rect.height * dpr));

    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }

    return { w: width, h: height, dpr };

}

updateLegend(min, max) { if (!this.root) return;

    const palette = this.settings.palette;
    const colours = (PALETTES[palette] || PALETTES.thermal)
      .map((stop) => `rgb(${stop[1].join(',')}) ${stop[0] * 100}%`)
      .join(',');

    const row = this.root.querySelector('.legendrow');
    row.style.display = this.settings.showLegend ? '' : 'none';
    this.root.querySelector('.legend').style.background =
      `linear-gradient(90deg,${colours})`;

    if (Number.isFinite(min) && Number.isFinite(max)) {
      const unit = this.displayUnit();
      this.root.querySelector('.legendMin').textContent =
        `${toDisplayTemperature(min, unit).toFixed(1)} ${unit}`;
      this.root.querySelector('.legendMax').textContent =
        `${toDisplayTemperature(max, unit).toFixed(1)} ${unit}`;
    }

}

draw() { if (!this.heat) return;

    const { w, h } = this.resizeCanvas(this.heat);
    const context = this.heat.getContext('2d');
    context.clearRect(0, 0, w, h);
    this.heat.className = `heat ${this.settings.interpolation}`;

    if (!this.frame) {
      context.fillStyle = '#111';
      context.fillRect(0, 0, w, h);
      this.drawOverlay();
      return;
    }

    let minimum = this.frame.minimum;
    let maximum = this.frame.maximum;
    const unit = this.displayUnit();

    if (
      this.settings.scaleMode !== 'fixed' &&
      (!Number.isFinite(minimum) || !Number.isFinite(maximum))
    ) {
      const validTemperatures = this.frame.temperatures.filter(Number.isFinite);
      if (validTemperatures.length) {
        minimum = Math.min(...validTemperatures);
        maximum = Math.max(...validTemperatures);
      }
    }

    if (this.settings.scaleMode === 'fixed') {
      minimum = fromDisplayTemperature(Number(this.settings.fixedMin), unit);
      maximum = fromDisplayTemperature(Number(this.settings.fixedMax), unit);
    }

    if (!(maximum > minimum)) {
      minimum -= 0.5;
      maximum += 0.5;
    }

    const image = context.createImageData(w, h);

    for (let y = 0; y < h; y += 1) {
      for (let x = 0; x < w; x += 1) {
        const sensorX = clamp(Math.floor((x / w) * SENSOR_SIZE), 0, SENSOR_SIZE - 1);
        const sensorY = clamp(Math.floor((y / h) * SENSOR_SIZE), 0, SENSOR_SIZE - 1);
        const temperature =
          this.frame.temperatures[sensorY * SENSOR_SIZE + sensorX];
        const index = (y * w + x) * 4;

        if (Number.isFinite(temperature)) {
          const colour = colourFor(
            (temperature - minimum) / (maximum - minimum),
            this.settings.palette,
          );
          image.data[index] = colour[0];
          image.data[index + 1] = colour[1];
          image.data[index + 2] = colour[2];
          image.data[index + 3] = 255;
        } else {
          image.data[index] = 0;
          image.data[index + 1] = 0;
          image.data[index + 2] = 0;
          image.data[index + 3] = 255;
        }
      }
    }

    context.putImageData(image, 0, 0);
    this.updateLegend(minimum, maximum);

    this.root.querySelector('.diagnostics').textContent =
      `Frame ${this.frame.sequence}\n` +
      `Protocol ${this.frame.version}\n` +
      `CRC OK\n` +
      `Calibration revision ${this.frame.calibrationRevision}\n` +
      `Sensor ${this.frame.width}×${this.frame.height}`;

    this.drawOverlay();

}

snapPoint(point) { if (this.settings.snapGrid) { return { x:
clamp(Math.round(point.x), 0, SENSOR_SIZE), y:
clamp(Math.round(point.y), 0, SENSOR_SIZE), }; }

    return {
      x: clamp(point.x, 0, SENSOR_SIZE),
      y: clamp(point.y, 0, SENSOR_SIZE),
    };

}

pointFromEvent(event) { const rect =
this.overlay.getBoundingClientRect(); return this.snapPoint({ x:
((event.clientX - rect.left) / rect.width) * SENSOR_SIZE, y:
((event.clientY - rect.top) / rect.height) * SENSOR_SIZE, }); }

selectedRoi() { return this.rois[this.selectedChannel - 1]; }

nearestVertex(roi, point, radius = 0.35) { let best = -1; let
bestDistance = radius;

    roi.points.forEach((vertex, index) => {
      const currentDistance = distance(vertex, point);
      if (currentDistance < bestDistance) {
        best = index;
        bestDistance = currentDistance;
      }
    });

    return best;

}

rotationHandle(roi) { const centre = centroid(roi.points); const top =
roi.points.reduce( (best, point) => (point.y < best.y ? point : best),
roi.points[0] || centre, );

    return {
      x: centre.x,
      y: clamp(top.y - 0.55, 0, SENSOR_SIZE),
    };

}

pointInPolygon(point, points) { let inside = false;

    for (
      let index = 0, previous = points.length - 1;
      index < points.length;
      previous = index, index += 1
    ) {
      const currentPoint = points[index];
      const previousPoint = points[previous];

      if (
        (currentPoint.y > point.y) !== (previousPoint.y > point.y) &&
        point.x <
          ((previousPoint.x - currentPoint.x) *
            (point.y - currentPoint.y)) /
            ((previousPoint.y - currentPoint.y) || 1e-9) +
            currentPoint.x
      ) {
        inside = !inside;
      }
    }

    return inside;

}

pointerDown(event) { const point = this.pointFromEvent(event);
this.updateCursor(point);

    this.overlay.setPointerCapture(event.pointerId);
    const roi = this.selectedRoi();

    if (this.tool === 'rectangle') {
      this.pushHistory();
      this.rois[this.selectedChannel - 1] = {
        channel: this.selectedChannel,
        type: 'rectangle',
        source: 'rectangle',
        points: [point, point],
        angle: 0,
      };
      this.drag = {
        kind: 'draw-rectangle',
        start: point,
      };
    } else if (this.tool === 'polygon') {
      if (roi.source !== 'polygon') {
        this.pushHistory();
        this.rois[this.selectedChannel - 1] = {
          channel: this.selectedChannel,
          type: 'polygon',
          source: 'polygon',
          points: [],
          angle: 0,
        };
      }

      const active = this.selectedRoi();

      if (
        active.points.length >= 3 &&
        distance(active.points[0], point) < 0.45
      ) {
        this.finishPolygon();
        return;
      }

      active.points.push(point);
      this.drawOverlay();
      return;
    } else if (this.tool === 'select' && roi.type !== 'disabled') {
      const polygon = this.roiPolygon(roi);
      const handle = this.rotationHandle({ points: polygon });
      const vertex = this.nearestVertex({ points: polygon }, point);

      if (distance(handle, point) < 0.42) {
        this.pushHistory();
        const centre = centroid(polygon);
        this.drag = {
          kind: 'rotate',
          centre,
          startAngle: Math.atan2(
            point.y - centre.y,
            point.x - centre.x,
          ),
          original: clone(polygon),
        };
      } else if (vertex >= 0) {
        this.pushHistory();
        if (roi.source === 'rectangle') {
          roi.points = polygon;
          roi.source = 'polygon';
          roi.type = 'polygon';
        }
        this.drag = {
          kind: 'vertex',
          index: vertex,
        };
      } else if (this.pointInPolygon(point, polygon)) {
        this.pushHistory();
        this.drag = {
          kind: 'move',
          start: point,
          original: clone(roi.points),
        };
      }
    }

    this.drawOverlay();

}

pointerMove(event) { const point = this.pointFromEvent(event);
this.updateCursor(point);

    if (!this.drag) return;

    const roi = this.selectedRoi();

    if (this.drag.kind === 'draw-rectangle') {
      roi.points[1] = point;
    } else if (this.drag.kind === 'vertex') {
      roi.points[this.drag.index] = point;
    } else if (this.drag.kind === 'move') {
      const requestedDeltaX = point.x - this.drag.start.x;
      const requestedDeltaY = point.y - this.drag.start.y;
      const xs = this.drag.original.map((vertex) => vertex.x);
      const ys = this.drag.original.map((vertex) => vertex.y);

      const deltaX = clamp(
        requestedDeltaX,
        -Math.min(...xs),
        SENSOR_SIZE - Math.max(...xs),
      );
      const deltaY = clamp(
        requestedDeltaY,
        -Math.min(...ys),
        SENSOR_SIZE - Math.max(...ys),
      );

      roi.points = this.drag.original.map((vertex) => ({
        x: vertex.x + deltaX,
        y: vertex.y + deltaY,
      }));
    } else if (this.drag.kind === 'rotate') {
      let angle =
        Math.atan2(
          point.y - this.drag.centre.y,
          point.x - this.drag.centre.x,
        ) - this.drag.startAngle;

      const snap = Number(this.settings.rotationSnap);
      if (snap > 0) {
        const step = snap * Math.PI / 180;
        angle = Math.round(angle / step) * step;
      }

      const rotated = this.drag.original.map((vertex) =>
        rotatePoint(vertex, this.drag.centre, angle),
      );

      const insideSensor = rotated.every(
        (vertex) =>
          vertex.x >= 0 &&
          vertex.x <= SENSOR_SIZE &&
          vertex.y >= 0 &&
          vertex.y <= SENSOR_SIZE,
      );

      if (insideSensor) {
        roi.points = rotated.map((vertex) => this.snapPoint(vertex));
        roi.type = 'polygon';
        roi.source = 'polygon';
        roi.angle = angle * 180 / Math.PI;
      }
    }

    this.drawOverlay();

}

pointerUp(event) { const completedDrag = this.drag;

    if (completedDrag?.kind === 'draw-rectangle') {
      const roi = this.selectedRoi();

      if (distance(roi.points[0], roi.points[1]) < 0.15) {
        if (this.history.length) {
          this.rois = this.history.pop();
          this.future = [];
          this.updateChannelTabs();
        }
      } else {
        this.tool = 'select';
        this.updateToolbar();
        this.updateChannelTabs();
        this.applyRoi();
      }
    } else if (completedDrag) {
      this.updateChannelTabs();
      this.scheduleApplyRoi();
    }

    this.drag = null;

    try {
      this.overlay.releasePointerCapture(event.pointerId);
    } catch (_) {}

    this.drawOverlay();

}

finishPolygon() { const roi = this.selectedRoi();

    if (roi.type !== 'polygon' || roi.points.length < 3) {
      this.setStatus('Polygon requires at least three points', true);
      return;
    }

    const last = roi.points[roi.points.length - 1];
    if (
      roi.points.length > 3 &&
      distance(last, roi.points[0]) < 0.15
    ) {
      roi.points.pop();
    }

    this.tool = 'select';
    this.updateToolbar();
    this.updateChannelTabs();
    this.drawOverlay();
    this.applyRoi();

}

scheduleApplyRoi() { if (this.pendingSave)
clearTimeout(this.pendingSave); this.pendingSave = setTimeout(() => {
this.pendingSave = null; this.applyRoi(); }, 350); }

updateCursor(point) { if (!this.frame) return;

    const x = clamp(Math.floor(point.x), 0, SENSOR_SIZE - 1);
    const y = clamp(Math.floor(point.y), 0, SENSOR_SIZE - 1);
    const value = this.frame.temperatures[y * SENSOR_SIZE + x];
    const unit = this.displayUnit();

    this.root.querySelector('.cursor').textContent =
      Number.isFinite(value)
        ? `Pixel ${x},${y}: ${toDisplayTemperature(value, unit).toFixed(1)} ${unit}`
        : `Pixel ${x},${y}: invalid`;

}

roiPolygon(roi) { return roi.source === 'rectangle' && roi.points.length
=== 2 ? rectangleCorners(roi.points[0], roi.points[1]) : roi.points; }

drawOverlay() { if (!this.overlay) return;

    const { w, h } = this.resizeCanvas(this.overlay);
    const context = this.overlay.getContext('2d');
    context.clearRect(0, 0, w, h);

    if (this.settings.showGrid) {
      context.strokeStyle = 'rgba(255,255,255,.22)';
      context.lineWidth = 1;

      for (let index = 1; index < SENSOR_SIZE; index += 1) {
        context.beginPath();
        context.moveTo((index / SENSOR_SIZE) * w, 0);
        context.lineTo((index / SENSOR_SIZE) * w, h);
        context.stroke();

        context.beginPath();
        context.moveTo(0, (index / SENSOR_SIZE) * h);
        context.lineTo(w, (index / SENSOR_SIZE) * h);
        context.stroke();
      }
    }

    for (const roi of this.rois) {
      const polygon = this.roiPolygon(roi);
      if (roi.type === 'disabled' || polygon.length < 2) continue;

      const selected = roi.channel === this.selectedChannel;
      context.strokeStyle = selected
        ? '#ffffff'
        : 'rgba(255,255,255,.75)';
      context.fillStyle = selected
        ? 'rgba(255,255,255,.13)'
        : 'rgba(255,255,255,.055)';
      context.lineWidth = selected ? 3 : 2;

      context.beginPath();
      polygon.forEach((point, index) => {
        const x = (point.x / SENSOR_SIZE) * w;
        const y = (point.y / SENSOR_SIZE) * h;
        if (index === 0) context.moveTo(x, y);
        else context.lineTo(x, y);
      });
      if (polygon.length > 2) context.closePath();
      context.fill();
      context.stroke();

      if (this.settings.showLabels) {
        this.drawRoiLabel(context, roi, polygon, w, h);
      }

      if (selected && this.tool === 'select') {
        context.fillStyle = '#ffffff';

        polygon.forEach((point) => {
          context.beginPath();
          context.arc(
            (point.x / SENSOR_SIZE) * w,
            (point.y / SENSOR_SIZE) * h,
            6,
            0,
            Math.PI * 2,
          );
          context.fill();
        });

        const rotationHandle = this.rotationHandle({ points: polygon });
        const centre = centroid(polygon);

        context.strokeStyle = '#ffffff';
        context.beginPath();
        context.moveTo(
          (centre.x / SENSOR_SIZE) * w,
          (centre.y / SENSOR_SIZE) * h,
        );
        context.lineTo(
          (rotationHandle.x / SENSOR_SIZE) * w,
          (rotationHandle.y / SENSOR_SIZE) * h,
        );
        context.stroke();

        context.beginPath();
        context.arc(
          (rotationHandle.x / SENSOR_SIZE) * w,
          (rotationHandle.y / SENSOR_SIZE) * h,
          8,
          0,
          Math.PI * 2,
        );
        context.fill();
      }
    }

    const roi = this.selectedRoi();
    this.root.querySelector('.angle').textContent =
      roi.angle ? `Angle ${roi.angle.toFixed(0)}°` : 'Angle 0°';

}

drawRoiLabel(context, roi, polygon, width, height) { const centre =
centroid(polygon); const stats = this.channelStats(roi.channel); const
unit = this.displayUnit();

    const lines = [`ROI ${roi.channel}`];

    if (stats.available) {
      lines.push(
        `Avg ${toDisplayTemperature(stats.average, unit).toFixed(1)} ${unit}`,
      );
      lines.push(
        `Min ${toDisplayTemperature(stats.minimum, unit).toFixed(1)} ${unit}`,
      );
      lines.push(
        `Max ${toDisplayTemperature(stats.maximum, unit).toFixed(1)} ${unit}`,
      );
    } else {
      lines.push('Waiting for data');
    }

    const fontSize = Math.max(11, Math.round(width / 48));
    const lineHeight = fontSize + 3;
    context.font = `600 ${fontSize}px sans-serif`;

    const textWidth =
      Math.max(...lines.map((line) => context.measureText(line).width)) + 12;
    const boxHeight = lines.length * lineHeight + 8;

    let x = (centre.x / SENSOR_SIZE) * width - textWidth / 2;
    let y = (centre.y / SENSOR_SIZE) * height - boxHeight / 2;

    x = clamp(x, 4, width - textWidth - 4);
    y = clamp(y, 4, height - boxHeight - 4);

    context.fillStyle = 'rgba(0,0,0,.68)';
    context.fillRect(x, y, textWidth, boxHeight);

    context.strokeStyle = 'rgba(255,255,255,.78)';
    context.lineWidth = 1;
    context.strokeRect(x, y, textWidth, boxHeight);

    context.fillStyle = '#ffffff';
    context.textBaseline = 'top';

    lines.forEach((line, index) => {
      context.fillText(line, x + 6, y + 4 + index * lineHeight);
    });

}

configuredEntity(group, key) { const configured = this.config[group]; if
(!configured) return null;

    if (typeof configured === 'object') {
      return configured[key] || null;
    }

    return null;

}

findEntity(candidates, suffixes = []) { if (!this._hass) return null;

    for (const candidate of candidates.filter(Boolean)) {
      if (this._hass.states[candidate]) return candidate;
    }

    const entityIds = Object.keys(this._hass.states);

    for (const suffix of suffixes) {
      const match = entityIds.find((entityId) => entityId.endsWith(suffix));
      if (match) return match;
    }

    return null;

}

stateNumber(entityId) { if (!entityId || !this._hass?.states[entityId])
return Number.NaN; const value =
Number(this._hass.states[entityId].state); return Number.isFinite(value)
? value : Number.NaN; }

channelEntity(channel, metric) { const configuredChannel =
this.config.channel_entities?.[channel] ||
this.config.channel_entities?.[String(channel)];

    if (configuredChannel?.[metric]) return configuredChannel[metric];

    const metricSuffix = metric === 'pixelCount'
      ? 'pixel_count'
      : metric;

    return this.findEntity(
      [
        `sensor.leafsense_amg8833_leafsense_channel_${channel}_${metricSuffix}`,
        `sensor.leafsense_channel_${channel}_${metricSuffix}`,
      ],
      [
        `_leafsense_channel_${channel}_${metricSuffix}`,
        `_channel_${channel}_${metricSuffix}`,
      ],
    );

}

channelStats(channel) { const minimum =
this.stateNumber(this.channelEntity(channel, 'minimum')); const maximum
= this.stateNumber(this.channelEntity(channel, 'maximum')); const
average = this.stateNumber(this.channelEntity(channel, 'average'));
const pixelCount = this.stateNumber(this.channelEntity(channel,
'pixelCount'));

    return {
      minimum,
      maximum,
      average,
      pixelCount,
      available:
        Number.isFinite(minimum) &&
        Number.isFinite(maximum) &&
        Number.isFinite(average),
    };

}

calibrationEntity(kind) { const configured =
this.config.calibration_entities?.[kind]; if (configured) return
configured;

    const definitions = {
      gain: {
        candidates: [
          'number.leafsense_amg8833_leafsense_calibration_gain',
          'number.leafsense_calibration_gain',
        ],
        suffixes: [
          '_leafsense_calibration_gain',
          '_calibration_gain',
        ],
      },
      offset: {
        candidates: [
          'number.leafsense_amg8833_leafsense_calibration_offset',
          'number.leafsense_calibration_offset',
        ],
        suffixes: [
          '_leafsense_calibration_offset',
          '_calibration_offset',
        ],
      },
      reference: {
        candidates: [
          'number.leafsense_amg8833_leafsense_reference_temperature',
          'number.leafsense_reference_temperature',
        ],
        suffixes: [
          '_leafsense_reference_temperature',
          '_reference_temperature',
        ],
      },
      current: {
        candidates: [
          'sensor.leafsense_amg8833_leafsense_calibration_current_reading',
          'sensor.leafsense_calibration_current_reading',
        ],
        suffixes: [
          '_leafsense_calibration_current_reading',
          '_calibration_current_reading',
        ],
      },
      difference: {
        candidates: [
          'sensor.leafsense_amg8833_leafsense_calibration_difference',
          'sensor.leafsense_calibration_difference',
        ],
        suffixes: [
          '_leafsense_calibration_difference',
          '_calibration_difference',
        ],
      },
      revision: {
        candidates: [
          'sensor.leafsense_amg8833_leafsense_calibration_revision',
          'sensor.leafsense_calibration_revision',
        ],
        suffixes: [
          '_leafsense_calibration_revision',
          '_calibration_revision',
        ],
      },
      apply: {
        candidates: [
          'button.leafsense_amg8833_leafsense_apply_reference_calibration',
          'button.leafsense_apply_reference_calibration',
        ],
        suffixes: [
          '_leafsense_apply_reference_calibration',
          '_apply_reference_calibration',
        ],
      },
      save: {
        candidates: [
          'button.leafsense_amg8833_leafsense_save_calibration',
          'button.leafsense_save_calibration',
        ],
        suffixes: [
          '_leafsense_save_calibration',
          '_save_calibration',
        ],
      },
      defaults: {
        candidates: [
          'button.leafsense_amg8833_leafsense_restore_calibration_defaults',
          'button.leafsense_restore_calibration_defaults',
        ],
        suffixes: [
          '_leafsense_restore_calibration_defaults',
          '_restore_calibration_defaults',
        ],
      },
    };

    const definition = definitions[kind];
    if (!definition) return null;

    return this.findEntity(definition.candidates, definition.suffixes);

}

syncCalibrationControls() { if (!this.root || !this._hass) return;

    const unit = this.displayUnit();
    const gainEntity = this.calibrationEntity('gain');
    const offsetEntity = this.calibrationEntity('offset');
    const referenceEntity = this.calibrationEntity('reference');
    const currentEntity = this.calibrationEntity('current');
    const differenceEntity = this.calibrationEntity('difference');
    const revisionEntity = this.calibrationEntity('revision');

    const gain = this.stateNumber(gainEntity);
    const offset = this.stateNumber(offsetEntity);
    const reference = this.stateNumber(referenceEntity);
    const current = this.stateNumber(currentEntity);
    const difference = this.stateNumber(differenceEntity);
    const revision = this.stateNumber(revisionEntity);

    const gainInput = this.root.querySelector('.calGain');
    const offsetInput = this.root.querySelector('.calOffset');
    const referenceInput = this.root.querySelector('.calReference');

    gainInput.disabled = !gainEntity;
    offsetInput.disabled = !offsetEntity;
    referenceInput.disabled = !referenceEntity;

    if (Number.isFinite(gain) && document.activeElement !== gainInput) {
      gainInput.value = String(gain);
    }
    if (Number.isFinite(offset) && document.activeElement !== offsetInput) {
      offsetInput.value = String(offset);
    }
    if (Number.isFinite(reference) && document.activeElement !== referenceInput) {
      referenceInput.value = String(reference);
    }

    this.root.querySelector('.calReferenceRow').style.display =
      referenceEntity ? '' : 'none';
    this.root.querySelector('.calDifferenceRow').style.display =
      differenceEntity ? '' : 'none';

    this.root.querySelector('.calCurrent').textContent =
      Number.isFinite(current)
        ? `${toDisplayTemperature(current, unit).toFixed(2)} ${unit}`
        : '—';

    this.root.querySelector('.calDifference').textContent =
      Number.isFinite(difference)
        ? `${toDisplayTemperatureDifference(difference, unit).toFixed(2)} ${unit}`
        : '—';

    this.root.querySelector('.calRevision').textContent =
      Number.isFinite(revision) ? String(Math.round(revision)) : '—';

    const applyButton = this.root.querySelector('.calApply');
    const saveButton = this.root.querySelector('.calSave');
    const defaultsButton = this.root.querySelector('.calDefaults');

    applyButton.disabled = !this.calibrationEntity('apply');
    saveButton.disabled = !this.calibrationEntity('save');
    defaultsButton.disabled = !this.calibrationEntity('defaults');

    const found = [
      gainEntity,
      offsetEntity,
      currentEntity,
      revisionEntity,
    ].filter(Boolean).length;

    this.root.querySelector('.calStatus').textContent =
      found >= 3
        ? 'Calibration controls connected.'
        : 'Some calibration entities were not found. Add explicit calibration_entities in the card YAML if their IDs differ.';

}

async setCalibrationNumber(kind, value) { const entityId =
this.calibrationEntity(kind);

    if (!entityId || !Number.isFinite(value)) {
      this.root.querySelector('.calStatus').textContent =
        `Calibration ${kind} entity is unavailable.`;
      return;
    }

    try {
      await this._hass.callService('number', 'set_value', {
        entity_id: entityId,
        value,
      });
      this.root.querySelector('.calStatus').textContent =
        `${kind[0].toUpperCase()}${kind.slice(1)} updated.`;
    } catch (error) {
      this.root.querySelector('.calStatus').textContent =
        `Calibration update failed: ${error.message || error}`;
    }

}

async pressCalibrationButton(kind) { const entityId =
this.calibrationEntity(kind);

    if (!entityId) {
      this.root.querySelector('.calStatus').textContent =
        `Calibration ${kind} button is unavailable.`;
      return;
    }

    try {
      await this._hass.callService('button', 'press', {
        entity_id: entityId,
      });
      this.root.querySelector('.calStatus').textContent =
        `${kind[0].toUpperCase()}${kind.slice(1)} command sent.`;
    } catch (error) {
      this.root.querySelector('.calStatus').textContent =
        `Calibration command failed: ${error.message || error}`;
    }

}

resolveLeafSenseService(name) {
    const explicit = `${this.config.service_prefix}_${name}`;
    const separator = explicit.indexOf('.');

    if (separator > 0) {
      const domain = explicit.slice(0, separator);
      const service = explicit.slice(separator + 1);

      if (this._hass?.services?.[domain]?.[service]) {
        return { domain, service };
      }
    }

    const esphomeServices = Object.keys(this._hass?.services?.esphome || {});
    const service = esphomeServices.find(
      (candidate) =>
        candidate === name ||
        candidate.endsWith(`_${name}`),
    );

    if (service) return { domain: 'esphome', service };

    if (separator > 0) {
      return {
        domain: explicit.slice(0, separator),
        service: explicit.slice(separator + 1),
      };
    }

    throw new Error(`Unable to resolve LeafSense action ${name}`);

}

async callLeafSenseService(name, payload) { const resolved =
this.resolveLeafSenseService(name); await this._hass.callService(
resolved.domain, resolved.service, payload, ); }

async applyRoi() { const roi = this.selectedRoi(); const polygon =
this.roiPolygon(roi);

    if (!this._hass) {
      this.setStatus('Home Assistant connection unavailable', true);
      return;
    }

    if (roi.type === 'rectangle' && roi.points.length !== 2) {
      this.setStatus('Draw a rectangle first', true);
      return;
    }

    if (roi.type === 'polygon' && polygon.length < 3) {
      this.setStatus('Polygon requires at least three points', true);
      return;
    }

    try {
      if (roi.type === 'disabled') {
        await this.callLeafSenseService(
          'leafsense_channel_disable',
          { channel: roi.channel },
        );
      } else if (roi.type === 'rectangle') {
        const first = roi.points[0];
        const second = roi.points[1];

        await this.callLeafSenseService(
          'leafsense_channel_set_rectangle',
          {
            channel: roi.channel,
            x: round3(Math.min(first.x, second.x)),
            y: round3(Math.min(first.y, second.y)),
            width: round3(Math.abs(second.x - first.x)),
            height: round3(Math.abs(second.y - first.y)),
          },
        );
      } else {
        await this.callLeafSenseService(
          'leafsense_channel_polygon_begin',
          {
            channel: roi.channel,
            point_count: polygon.length,
          },
        );

        try {
          for (let index = 0; index < polygon.length; index += 1) {
            await this.callLeafSenseService(
              'leafsense_channel_polygon_point',
              {
                channel: roi.channel,
                point_index: index,
                x: round3(polygon[index].x),
                y: round3(polygon[index].y),
              },
            );
          }

          await this.callLeafSenseService(
            'leafsense_channel_polygon_commit',
            { channel: roi.channel },
          );
        } catch (error) {
          await this.callLeafSenseService(
            'leafsense_channel_polygon_cancel',
            { channel: roi.channel },
          );
          throw error;
        }
      }

      this.setStatus(`ROI ${roi.channel} saved automatically`);
      this.editSnapshot = clone(this.rois);
      this.updateChannelTabs();
    } catch (error) {
      if (this.editSnapshot) {
        this.rois = clone(this.editSnapshot);
        this.updateChannelTabs();
        this.drawOverlay();
      }

      this.setStatus(
        `ROI action failed: ${error.message || error}`,
        true,
      );
    }

} }

customElements.define('leafsense-thermal-card', LeafSenseThermalCard);
window.LeafSenseThermal = { parseThermalFramePacket, crc32, colourFor,
rectangleCorners, rotatePoint, toDisplayTemperature,
toDisplayTemperatureDifference, fromDisplayTemperature }; window.customCards = window.customCards || [];
window.customCards.push({ type: 'leafsense-thermal-card', name:
'LeafSense Thermal Card', description: 'Live AMG8833 thermal image with six editable and rotatable measurement channels' });