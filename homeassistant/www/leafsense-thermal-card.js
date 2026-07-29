const LEAFSENSE_PACKET_SIZE = 156;
const LEAFSENSE_INVALID_TEMPERATURE = -32768;

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
    version: bytes[2],
    sequence: view.getUint32(4, true),
    timestampMs: view.getUint32(8, true),
    calibrationRevision: view.getUint32(12, true),
    width: bytes[16],
    height: bytes[17],
    minimum: view.getInt16(20, true) / 100,
    maximum: view.getInt16(22, true) / 100,
    temperatures,
  };
}

const PALETTES = {
  thermal: [[0,[0,0,128]],[0.2,[0,128,255]],[0.4,[0,255,255]],[0.6,[0,255,0]],[0.8,[255,255,0]],[0.92,[255,80,0]],[1,[255,255,255]]],
  iron: [[0,[0,0,0]],[0.25,[70,0,90]],[0.5,[180,25,60]],[0.75,[255,140,20]],[1,[255,255,220]]],
  greyscale: [[0,[0,0,0]],[1,[255,255,255]]],
};

function clamp(value, low, high) { return Math.min(high, Math.max(low, value)); }
function colourFor(value, paletteName='thermal') {
  const stops = PALETTES[paletteName] || PALETTES.thermal;
  const x = clamp(value, 0, 1);
  for (let i=1;i<stops.length;i+=1) {
    if (x <= stops[i][0]) {
      const a=stops[i-1], b=stops[i], t=(x-a[0])/(b[0]-a[0]);
      return a[1].map((v,c)=>Math.round(v+(b[1][c]-v)*t));
    }
  }
  return stops[stops.length-1][1];
}

class LeafSenseThermalCard extends HTMLElement {
  setConfig(config) {
    if (!config.entity) throw new Error('LeafSense card requires entity');
    this.config = {
      title: 'LeafSense Thermal View', palette: 'thermal', scale_mode: 'auto',
      fixed_min: 15, fixed_max: 40, channel: 1, service: 'esphome.leafsense_set_measurement_channel',
      ...config,
    };
    this.rois = Array.from({length:6}, (_,i)=>({channel:i+1,type:'disabled',points:[]}));
    this.selectedChannel = Number(this.config.channel) || 1;
    this.editMode = 'rectangle';
    this.drag = null;
    this.renderShell();
  }

  set hass(hass) {
    this._hass = hass;
    if (!this.config || !this.root) return;
    const state = hass.states[this.config.entity];
    if (!state || !state.state || ['unknown','unavailable'].includes(state.state)) {
      this.setStatus('Thermal frame unavailable'); return;
    }
    try {
      this.frame = parseThermalFramePacket(state.state);
      this.setStatus(`Frame ${this.frame.sequence} • ${this.frame.minimum.toFixed(2)}–${this.frame.maximum.toFixed(2)} °C`);
      this.draw();
    } catch (error) { this.setStatus(error.message, true); }
  }

  getCardSize() { return 7; }

  renderShell() {
    this.innerHTML = '';
    this.root = document.createElement('ha-card');
    this.root.innerHTML = `
      <style>
        .wrap{padding:16px}.title{font-size:20px;font-weight:600;margin-bottom:10px}.stage{position:relative;aspect-ratio:1;max-width:640px;margin:auto;background:#111;border-radius:10px;overflow:hidden;touch-action:none}.stage canvas{position:absolute;inset:0;width:100%;height:100%;image-rendering:auto}.toolbar{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:8px;margin-top:12px}.toolbar select,.toolbar button{min-height:40px;border-radius:8px;border:1px solid var(--divider-color);background:var(--card-background-color);color:var(--primary-text-color);padding:6px}.status{margin-top:10px;color:var(--secondary-text-color);font-size:13px}.error{color:var(--error-color)}.legend{height:14px;border-radius:7px;margin-top:10px;background:linear-gradient(90deg,#000080,#0080ff,#00ffff,#00ff00,#ffff00,#ff5000,#fff)}
      </style>
      <div class="wrap"><div class="title"></div><div class="stage"><canvas class="heat"></canvas><canvas class="overlay"></canvas></div><div class="legend"></div><div class="toolbar"><select class="channel"></select><select class="mode"><option value="rectangle">Rectangle</option><option value="polygon">Polygon</option><option value="disabled">Disabled</option></select><button class="apply">Apply ROI</button><button class="clear">Clear</button></div><div class="status"></div></div>`;
    this.appendChild(this.root);
    this.root.querySelector('.title').textContent=this.config.title;
    const channel=this.root.querySelector('.channel');
    for(let i=1;i<=6;i+=1){const o=document.createElement('option');o.value=i;o.textContent=`Channel ${i}`;channel.appendChild(o);} channel.value=this.selectedChannel;
    channel.addEventListener('change',()=>{this.selectedChannel=Number(channel.value);this.drawOverlay();});
    this.root.querySelector('.mode').addEventListener('change',(e)=>{this.editMode=e.target.value;});
    this.root.querySelector('.clear').addEventListener('click',()=>{this.rois[this.selectedChannel-1]={channel:this.selectedChannel,type:'disabled',points:[]};this.drawOverlay();});
    this.root.querySelector('.apply').addEventListener('click',()=>this.applyRoi());
    this.overlay=this.root.querySelector('.overlay'); this.heat=this.root.querySelector('.heat');
    this.overlay.addEventListener('pointerdown',(e)=>this.pointerDown(e));
    this.overlay.addEventListener('pointermove',(e)=>this.pointerMove(e));
    this.overlay.addEventListener('pointerup',(e)=>this.pointerUp(e));
    this.overlay.addEventListener('dblclick',()=>{if(this.editMode==='polygon')this.applyRoi();});
  }

  setStatus(text,error=false){if(!this.root)return;const el=this.root.querySelector('.status');el.textContent=text;el.classList.toggle('error',error);}
  resizeCanvas(canvas){const r=canvas.getBoundingClientRect(),dpr=window.devicePixelRatio||1,w=Math.max(1,Math.round(r.width*dpr)),h=Math.max(1,Math.round(r.height*dpr));if(canvas.width!==w||canvas.height!==h){canvas.width=w;canvas.height=h;}return {w,h,dpr};}
  draw(){if(!this.frame)return;const {w,h}=this.resizeCanvas(this.heat),ctx=this.heat.getContext('2d');const image=ctx.createImageData(w,h);let min=this.frame.minimum,max=this.frame.maximum;if(this.config.scale_mode==='fixed'){min=Number(this.config.fixed_min);max=Number(this.config.fixed_max);}if(!(max>min)){min-=0.5;max+=0.5;}for(let y=0;y<h;y+=1){for(let x=0;x<w;x+=1){const sx=clamp(Math.round((x/(w-1))*7),0,7),sy=clamp(Math.round((y/(h-1))*7),0,7),temp=this.frame.temperatures[sy*8+sx],idx=(y*w+x)*4;if(Number.isFinite(temp)){const c=colourFor((temp-min)/(max-min),this.config.palette);image.data[idx]=c[0];image.data[idx+1]=c[1];image.data[idx+2]=c[2];image.data[idx+3]=255;}else{image.data[idx+3]=255;}}}ctx.putImageData(image,0,0);this.drawOverlay();}
  pointFromEvent(e){const r=this.overlay.getBoundingClientRect();return {x:clamp(((e.clientX-r.left)/r.width)*8,0,8),y:clamp(((e.clientY-r.top)/r.height)*8,0,8)};}
  pointerDown(e){this.overlay.setPointerCapture(e.pointerId);const p=this.pointFromEvent(e),roi=this.rois[this.selectedChannel-1];if(this.editMode==='disabled'){this.rois[this.selectedChannel-1]={channel:this.selectedChannel,type:'disabled',points:[]};this.drawOverlay();return;}if(this.editMode==='rectangle'){this.drag=p;this.rois[this.selectedChannel-1]={channel:this.selectedChannel,type:'rectangle',points:[p,p]};}else{if(roi.type!=='polygon')this.rois[this.selectedChannel-1]={channel:this.selectedChannel,type:'polygon',points:[]};this.rois[this.selectedChannel-1].points.push(p);}this.drawOverlay();}
  pointerMove(e){if(this.editMode!=='rectangle'||!this.drag)return;this.rois[this.selectedChannel-1].points[1]=this.pointFromEvent(e);this.drawOverlay();}
  pointerUp(e){if(this.editMode==='rectangle'&&this.drag){this.rois[this.selectedChannel-1].points[1]=this.pointFromEvent(e);this.drag=null;this.drawOverlay();}}
  drawOverlay(){if(!this.overlay)return;const {w,h}=this.resizeCanvas(this.overlay),ctx=this.overlay.getContext('2d');ctx.clearRect(0,0,w,h);for(const roi of this.rois){if(roi.type==='disabled'||!roi.points.length)continue;const selected=roi.channel===this.selectedChannel;ctx.strokeStyle=selected?'#ffffff':'rgba(255,255,255,.65)';ctx.fillStyle=selected?'rgba(255,255,255,.12)':'rgba(255,255,255,.06)';ctx.lineWidth=selected?3:2;ctx.beginPath();if(roi.type==='rectangle'&&roi.points.length===2){const [a,b]=roi.points;ctx.rect(a.x/8*w,a.y/8*h,(b.x-a.x)/8*w,(b.y-a.y)/8*h);}else{roi.points.forEach((p,i)=>{const x=p.x/8*w,y=p.y/8*h;i?ctx.lineTo(x,y):ctx.moveTo(x,y);});if(roi.points.length>2)ctx.closePath();}ctx.fill();ctx.stroke();ctx.fillStyle='#fff';ctx.font='bold 14px sans-serif';ctx.fillText(String(roi.channel),roi.points[0].x/8*w+6,roi.points[0].y/8*h+18);}}
  async applyRoi(){const roi=this.rois[this.selectedChannel-1];if(!this._hass){this.setStatus('Home Assistant connection unavailable',true);return;}if(roi.type==='rectangle'&&roi.points.length!==2){this.setStatus('Draw a rectangle first',true);return;}if(roi.type==='polygon'&&roi.points.length<3){this.setStatus('Polygon requires at least three points',true);return;}const [domain,service]=this.config.service.split('.');const points=roi.points.map(p=>({x:Number(p.x.toFixed(3)),y:Number(p.y.toFixed(3))}));try{await this._hass.callService(domain,service,{channel:roi.channel,type:roi.type,points:JSON.stringify(points)});this.setStatus(`Channel ${roi.channel} ${roi.type} sent to LeafSense`);}catch(error){this.setStatus(`ROI service failed: ${error.message||error}`,true);}}
}

customElements.define('leafsense-thermal-card', LeafSenseThermalCard);
window.LeafSenseThermal = { parseThermalFramePacket, crc32, colourFor };
window.customCards = window.customCards || [];
window.customCards.push({type:'leafsense-thermal-card',name:'LeafSense Thermal Card',description:'Live AMG8833 thermal image and six editable measurement channels'});
