const LEAFSENSE_PACKET_SIZE = 156; const LEAFSENSE_INVALID_TEMPERATURE =
-32768; const SENSOR_SIZE = 8; const CHANNEL_COUNT = 6; const
SETTINGS_KEY = 'leafsense-thermal-card-settings-v3';
const ROI_STORAGE_VERSION = 3;
const ROI_COLOURS = ['#00e5ff', '#ffea00', '#ff4081', '#76ff03', '#ff9100', '#b388ff'];

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

    if (this.pendingSaves) {
      this.pendingSaves.forEach((timeout) => clearTimeout(timeout));
    }

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
    const emptyRois = Array.from(
      { length: CHANNEL_COUNT },
      (_, index) => ({
        channel: index + 1,
        type: 'disabled', pixels: [], name: `ROI ${index + 1}`,
      }),
    );
    this.rois = this.loadRois(emptyRois);

    this.selectedChannel = clamp(Number(this.config.channel) || 1, 1, CHANNEL_COUNT);
    this.tool = null;
    this.drag = null;
    this.frame = null;
    this.savedRois = clone(this.rois);
    this.pendingSaves = new Map();
    this.calibrationDrafts = { gain: null, offset: null, reference: null };
    this.nameDrafts = Array(CHANNEL_COUNT).fill(null);
    this.renderShell();

    if (this._pageHideHandler) window.removeEventListener('pagehide', this._pageHideHandler);
    if (this._visibilityHandler) document.removeEventListener('visibilitychange', this._visibilityHandler);
    this._pageHideHandler = () => this.flushRoisToBrowser();
    this._visibilityHandler = () => {
      if (document.visibilityState === 'hidden') this.flushRoisToBrowser();
    };
    window.addEventListener('pagehide', this._pageHideHandler);
    document.addEventListener('visibilitychange', this._visibilityHandler);

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
      this.updateChannelTabs();
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

roiStorageKey() {
    // The frame entity is the stable device identity. Do not include the
    // service prefix: users may rename or reconfigure it without changing the
    // LeafSense device, and doing so must not hide their saved ROIs.
    const identity = this.config.entity;
    return `leafsense-thermal-card-rois-v${ROI_STORAGE_VERSION}:${identity}`;
}

legacyRoiStorageKeys() {
    const keys = [
      `leafsense-thermal-card-rois-v2:${this.config.entity}|${this.config.service_prefix}`,
      `leafsense-thermal-card-rois-v1:${this.config.entity}|${this.config.service_prefix}`,
    ];
    // Alpha builds included service_prefix in the key. Search for the same
    // frame entity so data also migrates after a service-prefix rename.
    for (let index = 0; index < localStorage.length; index += 1) {
      const key = localStorage.key(index);
      if (
        key?.startsWith(`leafsense-thermal-card-rois-v2:${this.config.entity}|`) ||
        key?.startsWith(`leafsense-thermal-card-rois-v1:${this.config.entity}|`)
      ) keys.push(key);
    }
    return [...new Set(keys)];
}

loadRois(fallback) {
    try {
      const currentKey = this.roiStorageKey();
      let raw = localStorage.getItem(currentKey);
      let migratedFrom = null;
      if (!raw) {
        migratedFrom = this.legacyRoiStorageKeys().find((key) => localStorage.getItem(key));
        if (migratedFrom) raw = localStorage.getItem(migratedFrom);
      }
      const stored = JSON.parse(raw || 'null');
      if (!Array.isArray(stored) || stored.length !== CHANNEL_COUNT) return fallback;

      const loaded = stored.map((roi, index) => {
        const type = roi?.type === 'pixels' ? 'pixels' : 'disabled';
        const pixels = Array.isArray(roi?.pixels)
          ? [...new Set(roi.pixels.map(Number).filter((pixel) => Number.isInteger(pixel) && pixel >= 0 && pixel < 64))]
          : [];
        const points = Array.isArray(roi?.points)
          ? roi.points
            .filter((point) => Number.isFinite(point?.x) && Number.isFinite(point?.y))
            .map((point) => ({
              x: clamp(Number(point.x), 0, SENSOR_SIZE),
              y: clamp(Number(point.y), 0, SENSOR_SIZE),
            }))
          : [];
        const valid = type === 'disabled' || pixels.length > 0;

        return valid
          ? {
            channel: index + 1,
            type,
            pixels,
            name: String(roi?.name || `ROI ${index + 1}`).slice(0, 32),
          }
          : fallback[index];
      });
      if (migratedFrom) localStorage.setItem(currentKey, JSON.stringify(loaded));
      return loaded;
    } catch (_) {
      return fallback;
    }
}

saveRois(rois = this.rois) {
    try {
      localStorage.setItem(this.roiStorageKey(), JSON.stringify(rois));
      return true;
    } catch (error) {
      this.setStatus?.(`Could not save ROI settings in this browser: ${error.message || error}`, true);
      return false;
    }
}

flushRoisToBrowser() {
    if (!this.rois) return;
    this.savedRois = clone(this.rois);
    this.saveRois(this.savedRois);
}

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
        .channelbtn.used{border-color:var(--roi-colour,#78909c);border-width:2px}
        .channelbtn{display:flex;flex-direction:column;align-items:center;gap:2px}
        .channelbtn small{font-size:10px;opacity:.78;font-weight:400}
        .primary{background:var(--primary-color);color:var(--text-primary-color)}
        .danger{color:var(--error-color)}
        .layout{position:relative}
        .thermalrow{
          display:grid;
          grid-template-columns:60px minmax(0,680px);
          width:100%;
          max-width:750px;
          margin:0 auto;
          justify-content:center;
          gap:10px;
          align-items:stretch
        }
        .thermalrow.legend-hidden{
          grid-template-columns:minmax(0,680px)
        }
        .stage{
          position:relative;
          width:100%;
          min-width:0;
          aspect-ratio:1;
          max-width:680px;
          margin:auto;
          background:#111;
          border-radius:12px;
          overflow:hidden;
          touch-action:none
        }
        .stage canvas{position:absolute;inset:0;width:100%;height:100%}
        .heat{z-index:0}
        .overlay{z-index:1}
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
        .drawmenu{
          display:none;
          grid-template-columns:repeat(2,minmax(120px,1fr));
          gap:7px;
          max-width:680px;
          margin:7px auto 0
        }
        .drawmenu.open{display:grid}
        .editorhint{
          max-width:680px;
          margin:7px auto 0;
          font-size:12px;
          color:var(--secondary-text-color)
        }
        .legendrow{display:flex;gap:5px;min-height:0}
        .legend{width:28px;min-height:100%;border-radius:9px}
        .legendlabels{
          display:flex;
          flex-direction:column-reverse;
          justify-content:space-between;
          font-size:12px;
          color:var(--secondary-text-color);
          white-space:nowrap
        }
        .statusrow{
          display:grid;
          grid-template-columns:2fr 1fr 1fr;
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
        .roiNameField{
          grid-template-columns:auto minmax(190px,300px);
          justify-content:center;
          max-width:440px;
          margin:10px auto 0
        }
        .roiNameField span{text-align:right}
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
        button:disabled,input:disabled{cursor:not-allowed;opacity:.48}
        @media (max-width:520px){
          .wrap{padding:10px}
          .channelbar{grid-template-columns:repeat(3,1fr)}
          .editbar{grid-template-columns:repeat(2,1fr)}
          .field{grid-template-columns:1fr 125px}
          .calibration-actions{grid-template-columns:1fr}
          .statusrow{grid-template-columns:1fr}
          .statusrow span,.statusrow span:nth-child(2),.statusrow span:last-child{text-align:left}
          .thermalrow{grid-template-columns:54px minmax(0,1fr);gap:6px}
          .thermalrow.legend-hidden{grid-template-columns:minmax(0,1fr)}
          .legendlabels{font-size:10px}
        }
      </style>

      <div class="wrap">
        <div class="header">
          <div class="title"></div>
          <button class="iconbtn settingsToggle" title="Settings">⚙</button>
        </div>

        <div class="layout">
          <div class="thermalrow">
            <div class="legendrow">
              <div class="legend"></div>
              <div class="legendlabels"><span class="legendMin"></span><span class="legendMax"></span></div>
            </div>
            <div class="stage">
              <canvas class="heat"></canvas>
              <canvas class="overlay"></canvas>
            </div>
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
              <label class="field"><span>Coordinates</span><input data-setting="showCoordinates" type="checkbox"></label>
              <div class="hint">
                Select a channel, choose Edit pixels, then click cells or drag to paint and erase. Pixel selections are restored after a browser refresh.
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
                <button class="primary calApply">Apply reference</button>
                <button class="toolbtn calSave">Save to ESP</button>
                <button class="danger calDefaults">Restore defaults</button>
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
          <button class="toolbtn" data-tool="pixels">Edit pixels</button>
          <button class="danger clearRoi">Clear ROI</button>
        </div>

        <label class="field roiNameField"><span>ROI name</span><input class="roiName" maxlength="32" type="text"></label>

        <div class="editorhint">
          Choose a channel, select Edit pixels, then click or drag across the thermal image. Names and selections are kept in this browser; masks are sent to the ESP32 automatically.
        </div>

        <div class="statusrow">
          <span class="frameStats">Full frame —</span>
          <span class="status"></span>
          <span><span class="cursor">Cursor —</span> · <span class="angle">Pixels —</span></span>
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
      (event) => {
        event.stopPropagation();
        this.toggleSettings(true);
      },
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

    this.root.querySelector('.settings').addEventListener(
      'pointerdown',
      (event) => event.stopPropagation(),
    );
    this._outsideSettingsHandler = (event) => {
      const settings = this.root?.querySelector('.settings');
      if (!settings?.classList.contains('open')) return;
      if (settings.contains(event.target) || this.root.querySelector('.settingsToggle').contains(event.target)) return;
      this.toggleSettings(false);
    };
    document.addEventListener('pointerdown', this._outsideSettingsHandler);

    this.root.querySelectorAll('[data-tool]').forEach((button) => {
      button.addEventListener('click', () => {
        this.tool = this.tool === button.dataset.tool ? null : button.dataset.tool;
        this.updateToolbar();
        this.drawOverlay();
      });
    });

    const roiNameInput = this.root.querySelector('.roiName');
    roiNameInput.addEventListener('focus', () => {
      this.nameDrafts[this.selectedChannel - 1] = roiNameInput.value;
    });
    roiNameInput.addEventListener('input', (event) => {
      const roi = this.selectedRoi();
      const draft = event.target.value.slice(0, 32);
      this.nameDrafts[roi.channel - 1] = draft;
      roi.name = draft;
      this.savedRois[roi.channel - 1].name = roi.name;
      this.saveRois(this.savedRois);
      this.updateChannelTabs();
      this.drawOverlay();
    });
    roiNameInput.addEventListener('blur', () => this.commitRoiName());

    this.root.querySelector('.clearRoi').addEventListener('click', () => this.clearSelectedRoi());

    [['.calGain', 'gain'], ['.calOffset', 'offset'], ['.calReference', 'reference']]
      .forEach(([selector, kind]) => {
        const input = this.root.querySelector(selector);
        input.inputMode = 'decimal';
        input.addEventListener('focus', () => {
          if (this.calibrationDrafts[kind] === null) {
            this.calibrationDrafts[kind] = input.value;
          }
        });
        input.addEventListener('input', () => {
          this.calibrationDrafts[kind] = input.value;
        });
        input.addEventListener('keydown', (event) => {
          if (event.key === 'Enter') {
            event.preventDefault();
            input.blur();
          }
        });
      });
    this.root.querySelector('.calApply').addEventListener('click', async () => {
      await this.applyTypedCalibration();
      this.pressCalibrationButton('apply');
    });
    this.root.querySelector('.calSave').addEventListener('click', async () => {
      await this.applyTypedCalibration();
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
      event.preventDefault();
    });

    this.syncSettingsControls();
    this.syncCalibrationControls();
    this.updateToolbar();
    this.updateLegend();

}

disconnectedCallback() {
    if (this._outsideSettingsHandler) {
      document.removeEventListener('pointerdown', this._outsideSettingsHandler);
    }
    this.flushRoisToBrowser();
    if (this._pageHideHandler) window.removeEventListener('pagehide', this._pageHideHandler);
    if (this._visibilityHandler) document.removeEventListener('visibilitychange', this._visibilityHandler);
    this.pendingSaves?.forEach((timeout) => clearTimeout(timeout));
}

buildChannelTabs() { const container =
this.root.querySelector('.channelbar'); container.innerHTML = '';

    for (let channel = 1; channel <= CHANNEL_COUNT; channel += 1) {
      const button = document.createElement('button');
      button.className = 'channelbtn';
      button.dataset.channel = String(channel);
      button.textContent = `ROI ${channel}`;
      button.addEventListener('click', () => {
        this.commitRoiName();
        this.selectedChannel = channel;
        this.tool = null;
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
      const colour = ROI_COLOURS[(channel - 1) % ROI_COLOURS.length];
      if (roi?.pixels?.length) button.style.setProperty('--roi-colour', colour);
      else button.style.removeProperty('--roi-colour');

      const type = roi?.type === 'pixels' ? `${roi.pixels.length} px` : stats.available ? 'Live' : 'Off';

      const unit = this.displayUnit();
      const average = stats.available
        ? `${toDisplayTemperature(stats.average, unit).toFixed(1)} ${unit}`
        : 'No data';
      const label = document.createElement('span');
      label.textContent = `${roi?.name || `ROI ${channel}`} · ${type}`;
      const reading = document.createElement('small');
      reading.textContent = average;
      button.replaceChildren(label, reading);
    });
    const nameInput = this.root.querySelector('.roiName');
    if (nameInput) {
      const draft = this.nameDrafts[this.selectedChannel - 1];
      nameInput.value = draft !== null
        ? draft
        : this.selectedRoi()?.name || `ROI ${this.selectedChannel}`;
    }

}

commitRoiName() {
    if (!this.rois || !this.savedRois || !this.nameDrafts) return;
    const index = this.selectedChannel - 1;
    const draft = this.nameDrafts[index];
    if (draft === null) return;
    const name = String(draft).trim().slice(0, 32) || `ROI ${this.selectedChannel}`;
    this.rois[index].name = name;
    this.savedRois[index].name = name;
    this.nameDrafts[index] = null;
    this.saveRois(this.savedRois);
    this.updateChannelTabs();
    this.drawOverlay();
}

toggleSettings(open) {
    const panel = this.root.querySelector('.settings');
    if (!open && panel.classList.contains('open')) {
      this.readSettingsControls();
      this.applyTypedCalibration();
    }
    panel.classList.toggle('open', open);
    if (open) this.syncCalibrationControls();
}

async applyTypedCalibration() {
    const controls = [
      ['gain', this.root.querySelector('.calGain')],
      ['offset', this.root.querySelector('.calOffset')],
      ['reference', this.root.querySelector('.calReference')],
    ];
    for (const [kind, input] of controls) {
      const draft = this.calibrationDrafts[kind];
      const inputValue = draft !== null ? draft : input?.value;
      if (!input || input.disabled || !String(inputValue).trim()) continue;
      const value = Number(inputValue);
      const entityValue = this.stateNumber(this.calibrationEntity(kind));
      if (Number.isFinite(value) && (!Number.isFinite(entityValue) || value !== entityValue)) {
        await this.setCalibrationNumber(kind, value);
      }
      // Retain the requested value until Home Assistant confirms it. Clearing
      // it here lets a delayed state update restore the previous value.
    }
}

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
});
}

clearSelectedRoi() { const previous = this.selectedRoi(); this.rois[this.selectedChannel -
1] = { channel: this.selectedChannel, type: 'disabled', pixels: [],
name: previous.name || `ROI ${this.selectedChannel}`, };
this.tool = null; this.updateToolbar(); this.updateChannelTabs(); this.drawOverlay();
this.applyRoi(this.selectedChannel); }

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
    const thermalRow = this.root.querySelector('.thermalrow');
    const legendVisible = Boolean(this.settings.showLegend);
    row.style.display = legendVisible ? '' : 'none';
    thermalRow.classList.toggle('legend-hidden', !legendVisible);
    this.root.querySelector('.legend').style.background =
      `linear-gradient(0deg,${colours})`;

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
      this.root.querySelector('.frameStats').textContent = 'Full frame —';
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

    const validTemperatures = this.frame.temperatures.filter(Number.isFinite);
    if (validTemperatures.length) {
      const frameMinimum = Math.min(...validTemperatures);
      const frameMaximum = Math.max(...validTemperatures);
      const frameAverage = validTemperatures.reduce((sum, value) => sum + value, 0) / validTemperatures.length;
      this.root.querySelector('.frameStats').textContent =
        `Full frame: Min ${toDisplayTemperature(frameMinimum, unit).toFixed(1)} ${unit} · ` +
        `Avg ${toDisplayTemperature(frameAverage, unit).toFixed(1)} ${unit} · ` +
        `Max ${toDisplayTemperature(frameMaximum, unit).toFixed(1)} ${unit}`;
    } else {
      this.root.querySelector('.frameStats').textContent = 'Full frame —';
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

discardIncompletePolygon() {
    const roi = this.selectedRoi();
    if (this.tool !== 'polygon' || roi.type !== 'polygon' || roi.points.length >= 3) {
      return;
    }

    this.rois[this.selectedChannel - 1] = clone(
      this.savedRois[this.selectedChannel - 1],
    );
    this.updateChannelTabs();
    this.drawOverlay();
}

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

pointerDown(event) { const point = this.pointFromEvent(event); this.updateCursor(point);
    if (this.tool !== 'pixels') return;
    this.overlay.setPointerCapture(event.pointerId);
    const roi = this.selectedRoi();
    if (!Array.isArray(roi.pixels)) roi.pixels = [];
    const pixel = clamp(Math.floor(point.y), 0, 7) * 8 + clamp(Math.floor(point.x), 0, 7);
    this.drag = { kind: 'paint-pixels', erase: roi.pixels.includes(pixel), visited: new Set() };
    this.paintPixel(pixel);
}

pointerMove(event) { const point = this.pointFromEvent(event); this.updateCursor(point);
    if (this.drag?.kind !== 'paint-pixels') return;
    const pixel = clamp(Math.floor(point.y), 0, 7) * 8 + clamp(Math.floor(point.x), 0, 7);
    this.paintPixel(pixel);
}

pointerUp(event) { const completedDrag = this.drag;
    if (completedDrag?.kind === 'paint-pixels') {
      this.updateChannelTabs();
      this.scheduleApplyRoi(this.selectedChannel);
    }

    this.drag = null;

    try {
      this.overlay.releasePointerCapture(event.pointerId);
    } catch (_) {}

    this.drawOverlay();

}

paintPixel(pixel) {
    if (this.drag?.visited.has(pixel)) return;
    this.drag.visited.add(pixel);
    const roi = this.selectedRoi();
    const selected = new Set(roi.pixels || []);
    if (this.drag.erase) selected.delete(pixel); else selected.add(pixel);
    roi.pixels = [...selected].sort((a, b) => a - b);
    roi.type = roi.pixels.length ? 'pixels' : 'disabled';
    roi.name ||= `ROI ${roi.channel}`;
    // Persist synchronously. The ESP update remains debounced, but a browser
    // refresh must never be able to discard the most recent painted pixel.
    this.savedRois[roi.channel - 1] = clone(roi);
    this.saveRois(this.savedRois);
    this.drawOverlay();
}

finishPolygon() { const roi = this.selectedRoi();

    if (roi.type !== 'polygon' || roi.points.length < 3) {
      this.setStatus('Polygon requires at least three points', true);
      return;
    }

    const last = roi.points[roi.points.length - 1];
    const previous = roi.points[roi.points.length - 2];
    if (previous && distance(last, previous) < 0.15) {
      roi.points.pop();
    }

    if (roi.points.length < 3) {
      this.setStatus('Polygon requires at least three distinct points', true);
      return;
    }

    const finalPoint = roi.points[roi.points.length - 1];
    if (
      roi.points.length > 3 &&
      distance(finalPoint, roi.points[0]) < 0.15
    ) {
      roi.points.pop();
    }

    this.tool = 'select';
    this.updateToolbar();
    this.updateChannelTabs();
    this.drawOverlay();
    this.applyRoi(this.selectedChannel);

}

scheduleApplyRoi(channel = this.selectedChannel) {
    const existing = this.pendingSaves.get(channel);
    if (existing) clearTimeout(existing);

    const timeout = setTimeout(() => {
      this.pendingSaves.delete(channel);
      this.applyRoi(channel);
    }, 350);
    this.pendingSaves.set(channel, timeout);
}

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
      if (roi.type !== 'pixels' || !roi.pixels?.length) continue;
      const selected = roi.channel === this.selectedChannel;
      const colour = ROI_COLOURS[(roi.channel - 1) % ROI_COLOURS.length];
      const cellWidth = w / SENSOR_SIZE;
      const cellHeight = h / SENSOR_SIZE;
      context.fillStyle = selected ? `${colour}72` : `${colour}4a`;
      context.strokeStyle = colour;
      context.lineWidth = selected ? 3 : 2;
      const centres = [];
      roi.pixels.forEach((pixel) => {
        const x = pixel % SENSOR_SIZE;
        const y = Math.floor(pixel / SENSOR_SIZE);
        context.fillRect(x * cellWidth, y * cellHeight, cellWidth, cellHeight);
        context.strokeRect(x * cellWidth + 1, y * cellHeight + 1, cellWidth - 2, cellHeight - 2);
        centres.push({ x: x + 0.5, y: y + 0.5 });
      });
      if (this.settings.showLabels) this.drawRoiLabel(context, roi, centres, w, h);
    }

    const roi = this.selectedRoi();
    this.root.querySelector('.angle').textContent = `Pixels ${roi.pixels?.length || 0}`;

}

drawRoiLabel(context, roi, polygon, width, height) { const centre =
centroid(polygon); const stats = this.channelStats(roi.channel); const
unit = this.displayUnit();

    const lines = [roi.name || `ROI ${roi.channel}`];

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

    const entitiesAvailable =
      Number.isFinite(minimum) &&
      Number.isFinite(maximum) &&
      Number.isFinite(average);

    if (!entitiesAvailable) {
      const roi = this.rois?.[channel - 1];
      const selectedTemperatures = roi?.type === 'pixels' && this.frame
        ? (roi.pixels || [])
          .map((pixel) => this.frame.temperatures?.[pixel])
          .filter(Number.isFinite)
        : [];
      if (selectedTemperatures.length) {
        return {
          minimum: Math.min(...selectedTemperatures),
          maximum: Math.max(...selectedTemperatures),
          average: selectedTemperatures.reduce((sum, value) => sum + value, 0) /
            selectedTemperatures.length,
          pixelCount: selectedTemperatures.length,
          available: true,
          source: 'frame',
        };
      }
    }

    return {
      minimum,
      maximum,
      average,
      pixelCount,
      available: entitiesAvailable,
      source: entitiesAvailable ? 'entity' : 'none',
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

    [['gain', gain], ['offset', offset], ['reference', reference]]
      .forEach(([kind, entityValue]) => {
        const draft = Number(this.calibrationDrafts[kind]);
        if (this.calibrationDrafts[kind] !== null &&
            Number.isFinite(draft) && Number.isFinite(entityValue) &&
            Math.abs(draft - entityValue) < 0.0001) {
          this.calibrationDrafts[kind] = null;
        }
      });

    gainInput.disabled = !gainEntity;
    offsetInput.disabled = !offsetEntity;
    referenceInput.disabled = !referenceEntity;

    if (Number.isFinite(gain) && document.activeElement !== gainInput && this.calibrationDrafts.gain === null) {
      gainInput.value = String(gain);
    }
    if (Number.isFinite(offset) && document.activeElement !== offsetInput && this.calibrationDrafts.offset === null) {
      offsetInput.value = String(offset);
    }
    if (Number.isFinite(reference) && document.activeElement !== referenceInput && this.calibrationDrafts.reference === null) {
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

async applyRoi(channel = this.selectedChannel) { const roi = this.rois[channel - 1];

    // Browser persistence is independent of the ESP/HA connection. This keeps
    // the user's mask and name across a refresh even when an ESP service call
    // is temporarily unavailable.
    this.savedRois[roi.channel - 1] = clone(roi);
    this.saveRois(this.savedRois);

    if (!this._hass) {
      this.setStatus('ROI kept in browser; Home Assistant connection unavailable', true);
      return;
    }

    try {
      if (roi.type === 'disabled') {
        await this.callLeafSenseService(
          'leafsense_channel_disable',
          { channel: roi.channel },
        );
      } else {
        const selected = new Set(roi.pixels || []);
        const payload = { channel: roi.channel };
        for (let y = 0; y < SENSOR_SIZE; y += 1) {
          let row = 0;
          for (let x = 0; x < SENSOR_SIZE; x += 1) {
            if (selected.has(y * SENSOR_SIZE + x)) row |= (1 << x);
          }
          payload[`row_${y}`] = row;
        }
        await this.callLeafSenseService(
          'leafsense_channel_set_pixel_mask',
          payload,
        );
      }

      this.setStatus(`ROI ${roi.channel} saved to ESP and browser`);
      this.updateChannelTabs();
    } catch (error) {
      this.setStatus(
        `ROI kept in browser; ESP update failed: ${error.message || error}`,
        true,
      );
    }

} }

customElements.define('leafsense-thermal-card', LeafSenseThermalCard);
window.LeafSenseThermal = { LeafSenseThermalCard, parseThermalFramePacket, crc32, colourFor,
rectangleCorners, rotatePoint, toDisplayTemperature,
toDisplayTemperatureDifference, fromDisplayTemperature }; window.customCards = window.customCards || [];
window.customCards.push({ type: 'leafsense-thermal-card', name:
'LeafSense Thermal Card', description: 'Live AMG8833 thermal image with six editable pixel-mask ROIs' });
