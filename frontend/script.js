const $ = (id) => document.getElementById(id);
let ZONES = [];

const PAGES = {
  overview: ['Beach Health Monitoring System', 'Data-driven monitoring for safer and healthier beaches'],
  zones: ['Beach Zones', 'Live condition of every monitored stretch of coastline'],
  readings: ['Environmental Readings', 'Measurements and computed health for each zone'],
  lookup: ['Find a Zone', 'Locate a zone by its ID'],
  rankings: ['Zone Rankings', 'Order zones by the measure that matters most'],
  log: ['Observation Log', 'Follow one zone from its first observation to the latest'],
  alerts: ['Environmental Alerts', 'The most recent alert is always shown first'],
  tasks: ['Cleanup Tasks', 'Field work scheduled and completed in order'],
  forecast: ['Health Forecast', 'Where each zone is heading next'],
  clusters: ['Risk Clusters', 'Zones grouped by their pollution profile'],
  anomalies: ['Anomaly Detection', 'Readings that stand out from the rest'],
  compare: ['Compare Zones', 'Put two or three zones side by side'],
  reports: ['Beach Report', 'A one-page summary ready to print'],
};

async function api(path, method, body) {
  const res = await fetch(path, {
    method: method || 'GET',
    headers: { 'Content-Type': 'application/json' },
    body: body ? JSON.stringify(body) : undefined,
  });
  let data = {};
  try { data = await res.json(); } catch (e) {}
  return { ok: res.ok, data };
}

function toast(msg) {
  const t = $('toast');
  t.textContent = msg;
  t.classList.add('show');
  clearTimeout(toast.t);
  toast.t = setTimeout(() => t.classList.remove('show'), 2400);
}

/* ---- dropdown menus in the top bar (menu is position:fixed so the scrolling nav never clips it) ---- */
function closeNavMenus() {
  document.querySelectorAll('.nav-group.open').forEach((g) => {
    g.classList.remove('open');
    g.querySelector('.nav-drop').setAttribute('aria-expanded', 'false');
  });
}
document.querySelectorAll('.nav-drop').forEach((btn) => {
  btn.addEventListener('click', (e) => {
    e.stopPropagation();
    const grp = btn.parentElement, wasOpen = grp.classList.contains('open');
    closeNavMenus();
    if (wasOpen) return;
    const r = btn.getBoundingClientRect(), menu = grp.querySelector('.nav-menu');
    menu.style.top = (r.bottom + 8) + 'px';
    menu.style.left = Math.max(8, Math.min(r.left, window.innerWidth - 220)) + 'px';
    grp.classList.add('open');
    btn.setAttribute('aria-expanded', 'true');
  });
});
document.addEventListener('click', closeNavMenus);
document.addEventListener('keydown', (e) => { if (e.key === 'Escape') closeNavMenus(); });
window.addEventListener('resize', closeNavMenus);
$('nav').addEventListener('scroll', closeNavMenus);

function showPage(name) {
  if (!PAGES[name]) name = 'overview';
  document.querySelectorAll('.page').forEach((p) => p.classList.toggle('active', p.id === 'page-' + name));
  document.querySelectorAll('#nav a').forEach((a) => a.classList.toggle('active', a.dataset.page === name));
  $('pageTitle').textContent = PAGES[name][0];
  $('pageSubtitle').textContent = PAGES[name][1];
  if (name === 'overview') loadOverview();
  if (name === 'zones') renderZones();
  if (name === 'readings') renderReadings();
  if (name === 'log') loadHistory();
  if (name === 'alerts') loadStack();
  if (name === 'tasks') loadQueue();
  if (typeof LAB !== 'undefined' && LAB[name]) LAB[name]();
  closeNavMenus();
  document.querySelectorAll('.nav-group').forEach((g) => g.querySelector('.nav-drop').classList.toggle('active', !!g.querySelector('a.active')));
  window.scrollTo({ top: 0, behavior: 'smooth' });
}

document.querySelectorAll('[data-page]').forEach((a) => {
  a.addEventListener('click', (e) => {
    e.preventDefault();
    history.replaceState(null, '', '#' + a.dataset.page);
    showPage(a.dataset.page);
  });
});

async function checkConnection() {
  const el = $('connStatus');
  const r = await api('/api/health').catch(() => ({ ok: false, data: {} }));
  if (r.ok && r.data.source === 'c-core') {
    el.className = 'status ok'; $('connStatusText').textContent = 'Engine online';
    const src = prettySource(r.data.dataSource || '');
    $('dataSource').textContent = src ? 'Data source: ' + src : '';
    $('footerSource').textContent = src ? 'Data: ' + src : 'Beach water-quality monitoring data';
  }
  else if (r.ok) { el.className = 'status bad'; $('connStatusText').textContent = 'Engine not built'; }
  else { el.className = 'status bad'; $('connStatusText').textContent = 'Server offline'; }
}

function prettySource(raw) {
  if (!raw) return '';
  if (/ny_beach/i.test(raw)) return 'NY State Parks Beach Water Testing (data.ny.gov)';
  const file = raw.split(/[\\/]/).pop();
  return /built-in/i.test(raw) ? 'built-in reference data' : 'reference dataset (' + file + ')';
}

async function fetchZones() {
  const r = await api('/api/zones');
  ZONES = r.data.zones || [];
  return ZONES;
}

const riskKey = (r) => (r.startsWith('LOW') ? 'low' : r.startsWith('MEDIUM') ? 'medium' : 'high');
const badge = (r) => `<span class="badge b-${riskKey(r)}">${r.replace(' RISK', '')}</span>`;

async function loadOverview() {
  const h = await api('/api/health');
  if (h.ok) {
    $('statTotal').textContent = h.data.totalZones;
    $('statAvg').textContent = h.data.averageHealth;
    $('statLow').textContent = h.data.lowRisk;
    $('statMed').textContent = h.data.mediumRisk;
    $('statHigh').textContent = h.data.highRisk;
  }
  await fetchZones();
  renderShoreline();
  const ids = ZONES.map((z) => z.zoneId.replace('Z', ''));
  bars('chartHealthByZone', ids, ZONES.map((z) => z.healthScore), ['#5cc3cf', '#0f5f78']);
  bars('chartWaterByZone', ids, ZONES.map((z) => z.waterQuality), ['#8fd9e2', '#1787a0']);
  bars('chartPlasticByZone', ids, ZONES.map((z) => z.plasticWaste), ['#f0b48a', '#d9503f']);
  const c = { low: 0, medium: 0, high: 0 };
  ZONES.forEach((z) => c[riskKey(z.riskLevel)]++);
  donut('chartRiskDonut', [
    ['Low', c.low, '#22a06b'], ['Medium', c.medium, '#e0a02f'], ['High', c.high, '#d9503f'],
  ]);
}

function renderShoreline() {
  const n = ZONES.length;
  if (!n) { $('shoreline').innerHTML = ''; return; }
  const W = 1000, H = 190, pad = 50;
  const col = { low: '#22a06b', medium: '#e0a02f', high: '#d9503f' };
  const pts = ZONES.map((z, i) => ({
    x: n === 1 ? W / 2 : pad + (i * (W - 2 * pad)) / (n - 1),
    y: 92 + 20 * Math.sin(i * 0.75), z,
  }));
  let d = `M0 ${pts[0].y + 6}`;
  pts.forEach((p) => { d += ` L${p.x.toFixed(1)} ${p.y.toFixed(1)}`; });
  d += ` L${W} ${pts[n - 1].y + 6} L${W} ${H} L0 ${H} Z`;
  const r = Math.max(10, Math.min(22, 420 / n));
  const marks = pts.map((p) => `
    <g class="mk" data-id="${p.z.zoneId}">
      <title>${p.z.zoneId} ${p.z.zoneName}: ${p.z.healthScore.toFixed(1)} (${p.z.advisory})</title>
      <circle cx="${p.x.toFixed(1)}" cy="${(p.y - 4).toFixed(1)}" r="${r}" fill="${col[riskKey(p.z.riskLevel)]}" stroke="#fff" stroke-width="3"/>
      <text x="${p.x.toFixed(1)}" y="${(p.y + 1).toFixed(1)}" text-anchor="middle" font-size="${Math.round(r * 0.8)}" font-weight="bold" fill="#fff">${p.z.grade}</text>
      <text x="${p.x.toFixed(1)}" y="${(p.y + r + 22).toFixed(1)}" text-anchor="middle" font-size="12" fill="#12303b">${p.z.zoneId}</text>
    </g>`).join('');
  $('shoreline').innerHTML = `
    <svg viewBox="0 0 ${W} ${H}" role="img" aria-label="Shoreline with zone status markers">
      <defs><linearGradient id="seaG" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#bfe8ee"/><stop offset="1" stop-color="#e6f5f6"/></linearGradient></defs>
      <rect width="${W}" height="${H}" rx="14" fill="url(#seaG)"/>
      <path d="${d}" fill="#f1e6cc"/>
      ${marks}
    </svg>`;
  document.querySelectorAll('#shoreline .mk').forEach((g) => g.addEventListener('click', () => openZone(g.dataset.id)));
}

function setup(id) {
  const cv = $(id);
  const dpr = window.devicePixelRatio || 1;
  const w = cv.parentElement.clientWidth - 52;
  const h = parseInt(cv.getAttribute('height'), 10);
  cv.style.width = w + 'px';
  cv.style.height = h + 'px';
  cv.width = w * dpr;
  cv.height = h * dpr;
  const ctx = cv.getContext('2d');
  ctx.scale(dpr, dpr);
  return { ctx, w, h };
}

function bars(id, labels, values, colors) {
  const { ctx, w, h } = setup(id);
  const top = 12, bottom = 26, gap = 8;
  const bw = (w - gap * (values.length - 1)) / values.length;
  const ch = h - top - bottom;
  ctx.strokeStyle = '#e1eef0';
  ctx.fillStyle = '#7d9aa3';
  ctx.font = '11px Trebuchet MS, sans-serif';
  ctx.textAlign = 'center';
  [0, 25, 50, 75, 100].forEach((v) => {
    const y = top + ch - (v / 100) * ch;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  });
  values.forEach((v, i) => {
    const bh = (v / 100) * ch;
    const x = i * (bw + gap);
    const y = top + ch - bh;
    const g = ctx.createLinearGradient(0, y, 0, y + bh);
    g.addColorStop(0, colors[0]);
    g.addColorStop(1, colors[1]);
    ctx.fillStyle = g;
    ctx.beginPath();
    if (ctx.roundRect) ctx.roundRect(x, y, bw, bh, [6, 6, 0, 0]); else ctx.rect(x, y, bw, bh);
    ctx.fill();
    ctx.fillStyle = '#7d9aa3';
    ctx.fillText(labels[i], x + bw / 2, h - 8);
  });
}

function donut(id, parts) {
  const { ctx, w, h } = setup(id);
  const total = parts.reduce((s, p) => s + p[1], 0) || 1;
  const cx = w * 0.32, cy = h / 2, r = 78;
  let a = -Math.PI / 2;
  parts.forEach(([, v, col]) => {
    const s = (v / total) * Math.PI * 2;
    ctx.beginPath();
    ctx.arc(cx, cy, r, a, a + s);
    ctx.strokeStyle = col;
    ctx.lineWidth = 26;
    ctx.stroke();
    a += s;
  });
  ctx.fillStyle = '#0a3d52';
  ctx.font = '34px Palatino Linotype, Georgia, serif';
  ctx.textAlign = 'center';
  ctx.fillText(total, cx, cy + 6);
  ctx.font = '11px Trebuchet MS, sans-serif';
  ctx.fillStyle = '#7d9aa3';
  ctx.fillText('ZONES', cx, cy + 26);
  parts.forEach(([name, v, col], i) => {
    const y = cy - 40 + i * 38;
    ctx.fillStyle = col;
    ctx.beginPath(); ctx.arc(w * 0.62, y, 6, 0, Math.PI * 2); ctx.fill();
    ctx.fillStyle = '#12303b';
    ctx.textAlign = 'left';
    ctx.font = '14px Trebuchet MS, sans-serif';
    ctx.fillText(`${name} risk  ·  ${v}`, w * 0.62 + 16, y + 5);
  });
}

async function renderZones() {
  await fetchZones();
  $('zoneGrid').innerHTML = ZONES.map((z) => `
    <div class="zone risk-${riskKey(z.riskLevel)}" data-id="${z.zoneId}">
      <div class="zone-top">
        <div><div class="zid">${z.zoneId}</div><div class="zname">${z.zoneName}</div></div>
        ${badge(z.riskLevel)}
      </div>
      <div class="score">${z.healthScore.toFixed(1)}<small class="grade g-${z.grade}">${z.grade}</small></div>
      <div class="bar"><i style="width:${z.healthScore}%"></i></div>
      <div class="mini">
        <span>Plastic <b>${z.plasticWaste}%</b></span><span>Water <b>${z.waterQuality}%</b></span>
        <span>Erosion <b>${z.erosionLevel}%</b></span><span>Waste <b>${z.wasteCoverage}%</b></span>
        <span>Visitors <b>${z.visitorLoad}%</b></span><span>Anomaly <b>${z.environmentalAnomaly}%</b></span>
      </div>
      <button class="btn ghost">View details</button>
    </div>`).join('');
  document.querySelectorAll('.zone').forEach((c) => c.addEventListener('click', () => openZone(c.dataset.id)));
}

function advice(z) {
  if (z.riskLevel.startsWith('HIGH')) return 'Immediate cleanup and water-quality inspection recommended.';
  if (z.riskLevel.startsWith('MEDIUM')) return 'Schedule routine cleanup and keep monitoring.';
  return 'Schedule routine cleanup.';
}

const advClass = (a) => (a === 'Safe to swim' ? 'good' : a === 'Not recommended' ? 'bad' : 'warn');

async function zoneTrend(id) {
  const r = await api('/api/history/zone/' + id);
  const vals = ((r.data && r.data.records) || []).map((x) => Number(x.healthScore));
  if (vals.length < 2) return `<div class="spark">Trend: only ${vals.length} observation recorded so far. Add more in the Observation Log.</div>`;
  const min = Math.min(...vals, 0), max = Math.max(...vals, 100), w = 300, h = 56;
  const pts = vals.map((v, i) => `${((i / (vals.length - 1)) * w).toFixed(1)},${(h - 4 - ((v - min) / (max - min)) * (h - 8)).toFixed(1)}`).join(' ');
  const diff = vals[vals.length - 1] - vals[0];
  const label = diff > 2 ? 'Improving' : diff < -2 ? 'Worsening' : 'Stable';
  return `<div class="spark">Trend over ${vals.length} observations: <b>${label}</b> (${diff >= 0 ? '+' : ''}${diff.toFixed(1)})
    <svg viewBox="0 0 ${w} ${h}" preserveAspectRatio="none"><polyline points="${pts}" fill="none" stroke="#1787a0" stroke-width="2.5" stroke-linejoin="round"/></svg></div>`;
}

async function openZone(id) {
  const z = ZONES.find((x) => x.zoneId === id);
  if (!z) return;
  const rows = [
    ['Health score', z.healthScore.toFixed(2) + ' / 100  (grade ' + z.grade + ')'], ['Plastic waste', z.plasticWaste + '%'],
    ['Water quality', z.waterQuality + '%'], ['Erosion', z.erosionLevel + '%'],
    ['Waste coverage', z.wasteCoverage + '%'], ['Visitor load', z.visitorLoad + '%'],
    ['Environmental anomaly', z.environmentalAnomaly + '%'],
  ];
  $('modalBody').innerHTML = `
    <div class="zid">${z.zoneId}</div>
    <h2>${z.zoneName}</h2>${badge(z.riskLevel)}<span class="advisory adv-${advClass(z.advisory)}">${z.advisory}</span>
    <div style="margin-top:14px">${rows.map((r) => `<div class="mrow"><span>${r[0]}</span><span>${r[1]}</span></div>`).join('')}</div>
    <div class="action"><b>Recommended action:</b> ${advice(z)}</div>
    <div id="zoneTrend"></div>`;
  $('modalBackdrop').classList.add('open');
  const html = await zoneTrend(id);
  if ($('zoneTrend')) $('zoneTrend').innerHTML = html;
}
$('modalClose').addEventListener('click', () => $('modalBackdrop').classList.remove('open'));
$('modalBackdrop').addEventListener('click', (e) => { if (e.target.id === 'modalBackdrop') e.currentTarget.classList.remove('open'); });

async function renderReadings() {
  await fetchZones();
  $('matrixTableBody').innerHTML = ZONES.map((z) => `
    <tr><td><b>${z.zoneId}</b> &nbsp;${z.zoneName}</td><td>${z.plasticWaste}</td><td>${z.waterQuality}</td>
    <td>${z.erosionLevel}</td><td>${z.wasteCoverage}</td><td>${z.visitorLoad}</td><td>${z.environmentalAnomaly}</td>
    <td><b>${z.healthScore.toFixed(1)}</b></td><td>${badge(z.riskLevel)}</td></tr>`).join('');
}

$('btnExportCsv').addEventListener('click', async () => {
  await fetchZones();
  if (!ZONES.length) return toast('No data to export');
  const head = ['zone_id', 'zone_name', 'plastic', 'water_quality', 'erosion', 'waste_coverage', 'visitor_load', 'anomaly', 'health_score', 'grade', 'risk', 'advisory'];
  const q = (v) => '"' + String(v).replace(/"/g, '""') + '"';
  const rows = ZONES.map((z) => [z.zoneId, z.zoneName, z.plasticWaste, z.waterQuality, z.erosionLevel, z.wasteCoverage,
    z.visitorLoad, z.environmentalAnomaly, z.healthScore, z.grade, z.riskLevel, z.advisory].map(q).join(','));
  const csv = '\ufeff' + head.join(',') + '\n' + rows.join('\n');   /* \ufeff = BOM so Excel reads it correctly */
  const url = URL.createObjectURL(new Blob([csv], { type: 'text/csv;charset=utf-8' }));
  const a = document.createElement('a');
  a.href = url;
  a.download = 'blueguard-zone-report.csv';
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  setTimeout(() => URL.revokeObjectURL(url), 1000);
  toast('Report downloaded');
});
$('btnPrint').addEventListener('click', () => window.print());

$('btnSearch').addEventListener('click', async () => {
  const zoneId = $('searchInput').value.trim().toUpperCase();
  const box = $('searchResult');
  if (!zoneId) return toast('Enter a zone ID');
  const [bin, lin] = await Promise.all([
    api('/api/search', 'POST', { method: 'binary', zoneId }),
    api('/api/search', 'POST', { method: 'linear', zoneId }),
  ]);
  box.classList.add('live');
  if (!bin.ok) { box.textContent = bin.data.error || 'Search failed'; return; }
  const d = bin.data;
  const cmp = `Binary search: ${d.comparisons} comparisons (O(log n)) &nbsp;|&nbsp; a linear scan would need ${lin.ok ? lin.data.comparisons : '?'} (O(n))`;
  box.innerHTML = d.found
    ? `<b>${d.zone.zoneId} · ${d.zone.zoneName}</b> ${badge(d.zone.riskLevel)}<br>Health score ${d.zone.healthScore.toFixed(1)} &nbsp;|&nbsp; Grade ${d.zone.grade}<br>${cmp}`
    : `No zone matches <b>${zoneId}</b>.<br>${cmp}`;
});

/* rank 1 = deepest sea blue, last rank = lightest foam (matches the site theme) */
function rankColor(rank, total) {
  const t = total > 1 ? (rank - 1) / (total - 1) : 0;          /* 0 (first) .. 1 (last) */
  const hue = 198 - t * 12;                                    /* deep blue -> aqua */
  const sat = 74 - t * 20;
  const light = 22 + t * 66;                                   /* 22% dark -> 88% light */
  return { bg: `hsl(${hue}, ${sat}%, ${light}%)`, fg: light < 55 ? '#fff' : '#0a3d52' };
}

$('btnRunSort').addEventListener('click', async () => {
  const r = await api('/api/sort', 'POST', {
    algorithm: $('sortAlgo').value, key: $('sortKey').value, direction: $('sortDir').value,
  });
  if (!r.ok) { $('sortMeta').innerHTML = `<span>${r.data.error || 'Sort failed'}</span>`; return; }
  const d = r.data;
  const key = $('sortKey').value;
  const keyLabel = { health: 'Health score', plastic: 'Plastic waste', water: 'Water quality', zoneid: 'Health score' }[key];
  const val = (z) => ({ health: z.healthScore, plastic: z.plasticWaste, water: z.waterQuality, zoneid: z.healthScore }[key]);
  const n = d.after.length;

  /* zone -> its final rank, so the same zone keeps the same colour in BEFORE and AFTER */
  const rankOf = {};
  d.after.forEach((z, i) => { rankOf[z.zoneId] = i + 1; });

  const chip = (z, showRank) => {
    const rk = rankOf[z.zoneId];
    const c = rankColor(rk, n);
    return `<span class="chip ranked" style="background:${c.bg};color:${c.fg};border-color:${c.bg}" ` +
      `title="${z.zoneName} · ${keyLabel} ${Number(val(z)).toFixed(1)} · Rank ${rk}">` +
      (showRank ? `<em>#${rk}</em>` : '') + `${z.zoneId}</span>`;
  };

  $('sortMeta').innerHTML =
    `<span>Algorithm <b>${d.algorithm}</b></span>` +
    `<span>Complexity <b>${d.timeComplexity}</b></span>` +
    `<span>Comparisons <b>${d.comparisons}</b></span>` +
    `<span>Moves <b>${d.moves}</b></span>` +
    `<span>Time <b>${Number(d.executionTimeMs).toFixed(4)} ms</b></span>` +
    `<span>Records <b>${d.recordCount}</b></span>`;

  $('rankTable').innerHTML =
    `<div class="rank-legend"><span>Rank 1</span><i></i><span>Rank ${n}</span>` +
    `<small>Darker = higher rank · colour follows each zone's final position</small></div>` +
    `<div class="cols">` +
      `<div><div class="cap">Before</div><div class="chips">${d.before.map((z) => chip(z, false)).join('')}</div></div>` +
      `<div><div class="cap">After (ranked)</div><div class="chips after">${d.after.map((z) => chip(z, true)).join('')}</div></div>` +
    `</div>`;
});

function setLogDate() {
  const d = new Date();
  d.setMinutes(d.getMinutes() - d.getTimezoneOffset());   /* local time, not UTC */
  $('logDate').value = d.toISOString().slice(0, 10);
}

/* fills the zone drop-down once, keeping the current choice */
async function fillLogZones() {
  if (!ZONES.length) await fetchZones();
  const sel = $('logZone');
  if (sel.options.length === ZONES.length) return;
  const keep = sel.value;
  sel.innerHTML = ZONES.map((z) => `<option value="${z.zoneId}">${z.zoneId} · ${z.zoneName}</option>`).join('');
  if (keep) sel.value = keep;
}

/* shows ONE zone's observations as a pipeline: 1st -> 2nd -> 3rd ... */
async function loadHistory() {
  await fillLogZones();
  const id = $('logZone').value;
  if (!id) { $('historyChain').innerHTML = '<span class="empty">No zones loaded.</span>'; return; }
  $('pipeTitle').textContent = `Observation pipeline · ${id}`;
  const r = await api('/api/history/zone/' + id);
  const list = (r.data && r.data.records) || [];
  if (!list.length) {
    $('historyChain').innerHTML = `<span class="empty">No observations yet for ${id}. Add the first one below.</span>`;
    return;
  }
  const nodes = list.map((n, i) => {
    const h = Number(n.healthScore);
    let delta = '<span class="delta">first reading</span>';
    if (i > 0) {
      const d = h - Number(list[i - 1].healthScore);
      delta = `<span class="delta ${d >= 0 ? 'up' : 'down'}">${d >= 0 ? '▲ +' : '▼ '}${d.toFixed(1)}</span>`;
    }
    return `<div class="node" style="animation-delay:${i * 60}ms">
      <span class="obs-no">Observation ${i + 1}</span>
      <b>${n.date}</b>Health ${h.toFixed(1)} ${delta}
      <small>Plastic ${n.plasticWaste} · Water ${n.waterQuality} · Erosion ${n.erosionLevel}</small></div>`;
  });
  $('historyChain').innerHTML = '<span class="cap">Start</span><span class="link">→</span>' +
    nodes.join('<span class="link">→</span>') + '<span class="link">→</span><span class="cap">Latest</span>';
}

$('logZone').addEventListener('change', loadHistory);

$('historyForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const body = Object.fromEntries(new FormData(e.target).entries());
  body.zoneId = $('logZone').value;                    /* zone comes from the drop-down */
  ['plasticWaste', 'waterQuality', 'erosionLevel'].forEach((k) => (body[k] = parseFloat(body[k])));
  const [yy, mm, dd] = body.date.split('-');          /* date picker gives yyyy-mm-dd */
  body.date = `${dd}/${mm}/${yy}`;
  const r = await api('/api/history', 'POST', body);
  toast(r.ok ? `Recorded: health ${Number(r.data.healthScore).toFixed(1)}, grade ${r.data.grade}` : r.data.error);
  if (r.ok) { e.target.reset(); setLogDate(); await fetchZones(); loadHistory(); }
});

$('btnHistoryDelete').addEventListener('click', async () => {
  const id = $('logZone').value;
  const r = await api('/api/history/' + id, 'DELETE');
  toast(r.ok ? `Oldest observation of ${id} removed` : r.data.error);
  if (r.ok) loadHistory();
});

async function loadStack() {
  const list = (await api('/api/alerts')).data.alerts || [];
  $('stackVisual').innerHTML = list.length
    ? list.map((a, i) => `<div class="item ${i === 0 ? 'first' : ''}" data-tag="LATEST"><span>${a.message}</span><span class="sev-${a.severity}">${a.severity}</span></div>`).join('')
    : '<span class="empty">No active alerts.</span>';
}

$('stackForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const body = Object.fromEntries(new FormData(e.target).entries());
  body.zoneId = body.zoneId.toUpperCase();
  const r = await api('/api/alerts', 'POST', body);
  toast(r.ok ? 'Alert raised' : r.data.error);
  $('stackMsg').textContent = r.ok ? `Raised: ${body.message}` : r.data.error;
  if (r.ok) { e.target.reset(); loadStack(); }
});
$('btnStackPop').addEventListener('click', async () => {
  const r = await api('/api/alerts', 'DELETE');
  $('stackMsg').textContent = r.ok ? `Resolved: ${r.data.alert.message}` : r.data.error;
  toast(r.ok ? 'Alert resolved' : r.data.error);
  if (r.ok) loadStack();
});
$('btnStackPeek').addEventListener('click', async () => {
  const list = (await api('/api/alerts')).data.alerts || [];
  $('stackMsg').textContent = list.length ? `Latest: ${list[0].message}` : 'No active alerts.';
});
$('btnStackClear').addEventListener('click', async () => {
  await api('/api/alerts/all', 'DELETE');
  $('stackMsg').textContent = 'All alerts cleared.';
  loadStack();
});

async function loadQueue() {
  const list = (await api('/api/tasks')).data.tasks || [];
  $('queueVisual').innerHTML = list.length
    ? list.map((t, i) => `<div class="item ${i === 0 ? 'first' : ''}" data-tag="NEXT"><span>${t.description}</span><span class="sev-${t.priority}">${t.priority}</span></div>`).join('')
    : '<span class="empty">No tasks scheduled.</span>';
}

$('queueForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const body = Object.fromEntries(new FormData(e.target).entries());
  body.zoneId = body.zoneId.toUpperCase();
  const r = await api('/api/tasks', 'POST', body);
  toast(r.ok ? 'Task scheduled' : r.data.error);
  $('queueMsg').textContent = r.ok ? `Scheduled: ${body.description}` : r.data.error;
  if (r.ok) { e.target.reset(); loadQueue(); }
});
$('btnQueueDequeue').addEventListener('click', async () => {
  const r = await api('/api/tasks', 'DELETE');
  $('queueMsg').textContent = r.ok ? `Completed: ${r.data.task.description}` : r.data.error;
  toast(r.ok ? 'Task completed' : r.data.error);
  if (r.ok) loadQueue();
});
$('btnQueuePeek').addEventListener('click', async () => {
  const list = (await api('/api/tasks')).data.tasks || [];
  $('queueMsg').textContent = list.length ? `Next: ${list[0].description}` : 'No tasks scheduled.';
});
$('btnQueueClear').addEventListener('click', async () => {
  await api('/api/tasks/all', 'DELETE');
  $('queueMsg').textContent = 'All tasks cleared.';
  loadQueue();
});

$('btnPlan').addEventListener('click', async () => {
  const r = await api('/api/tasks/plan', 'POST', { count: $('planCount').value });
  $('queueMsg').textContent = r.ok ? `Scheduled ${r.data.added} task(s) for the weakest zones.` : r.data.error;
  toast(r.ok ? 'Cleanup planned' : r.data.error);
  if (r.ok) loadQueue();
});

window.addEventListener('load', () => {
  setLogDate();
  checkConnection();
  showPage(location.hash.replace('#', '') || 'overview');
});
window.addEventListener('resize', () => { if ($('page-overview').classList.contains('active')) loadOverview(); });
