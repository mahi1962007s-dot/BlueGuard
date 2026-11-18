/* ============================================================
 * BlueGuard - analytics & algorithm lab pages
 * Every number on these pages is computed by the C engine
 * (graph, hash table, benchmark, regression, k-means, z-scores);
 * this file only draws the results. Re-uses $, api, toast, ZONES,
 * fetchZones and badge from script.js.
 * ============================================================ */
const LAB = {};

const RISKC = { low: '#22a06b', medium: '#e0a02f', high: '#d9503f' };
const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const f1 = (v) => Number(v).toFixed(1);
const f2 = (v) => Number(v).toFixed(2);
const zoneName = (id) => { const z = ZONES.find((x) => x.zoneId === id); return z ? z.zoneName : id; };
const zoneById = (id) => ZONES.find((x) => x.zoneId === id);
const shortDate = (d) => d.split('/').slice(0, 2).join('/');

async function ensureZones() { if (!ZONES.length) await fetchZones(); return ZONES; }

function fillSelect(id, items, opts = {}) {
  const el = $(id);
  if (!el) return;
  const keep = el.value;
  el.innerHTML = (opts.none ? `<option value="">${opts.none}</option>` : '') +
    items.map((z) => `<option value="${z.zoneId}">${z.zoneId} · ${esc(z.zoneName)}</option>`).join('');
  if (keep && [...el.options].some((o) => o.value === keep)) el.value = keep;
  else if (opts.def !== undefined) el.value = opts.def;
}

const legendHtml = (items) => items.map((i) => `<span><i class="sw ${i.dash ? 'dash' : ''}" style="border-color:${i.color}"></i>${i.label}</span>`).join('');
const swatchHtml = (items) => items.map((i) => `<span><i class="dot-sq" style="background:${i.color}"></i>${i.label}</span>`).join('');

/* ------------------------------------------------------------
 * generic SVG line chart (linear or log y axis)
 * ------------------------------------------------------------ */
function svgChart(o) {
  const W = o.w || 760, H = o.h || 340, L = 66, R = 22, T = 18, B = 52;
  const pw = W - L - R, ph = H - T - B, n = o.xCount;
  const X = (i) => L + pw * ((i + 0.5) / n);
  const tr = (v) => (o.log ? (Math.log10(v) - Math.log10(o.yMin)) / (Math.log10(o.yMax) - Math.log10(o.yMin)) : (v - o.yMin) / (o.yMax - o.yMin));
  const Y = (v) => T + ph * (1 - Math.max(0, Math.min(1, tr(v))));
  let s = `<svg viewBox="0 0 ${W} ${H}" role="img" aria-label="${esc(o.label || 'chart')}">`;
  (o.yTicks || []).forEach((t) => {
    s += `<line x1="${L}" x2="${W - R}" y1="${Y(t).toFixed(1)}" y2="${Y(t).toFixed(1)}" stroke="#e1eef0"/>` +
      `<text x="${L - 10}" y="${(Y(t) + 4).toFixed(1)}" text-anchor="end" font-size="11.5" fill="#7d9aa3">${o.yFmt ? o.yFmt(t) : t}</text>`;
  });
  (o.refLines || []).forEach((r) => {
    s += `<line x1="${L}" x2="${W - R}" y1="${Y(r.v).toFixed(1)}" y2="${Y(r.v).toFixed(1)}" stroke="${r.color}" stroke-dasharray="2 5" opacity=".8"/>` +
      `<text x="${W - R - 4}" y="${(Y(r.v) - 5).toFixed(1)}" text-anchor="end" font-size="10.5" fill="${r.color}">${r.label}</text>`;
  });
  const every = Math.ceil(n / 9);
  (o.xLabels || []).forEach((lb, i) => {
    if (lb && (i % every === 0 || i === n - 1 || o.allLabels)) s += `<text x="${X(i).toFixed(1)}" y="${H - B + 20}" text-anchor="middle" font-size="11.5" fill="#7d9aa3">${lb}</text>`;
  });
  if (o.xTitle) s += `<text x="${L + pw / 2}" y="${H - 8}" text-anchor="middle" font-size="12" fill="#5d7c87">${o.xTitle}</text>`;
  if (o.yTitle) s += `<text transform="translate(16 ${T + ph / 2}) rotate(-90)" text-anchor="middle" font-size="12" fill="#5d7c87">${o.yTitle}</text>`;
  (o.series || []).forEach((sr) => {
    if (sr.band) {                                   /* shaded range between lo[] and hi[] */
      const idx = sr.band.lo.map((v, i) => (v === null ? null : i)).filter((i) => i !== null);
      if (idx.length > 1) {
        const up = idx.map((i) => `${X(i).toFixed(1)},${Y(sr.band.hi[i]).toFixed(1)}`);
        const dn = idx.slice().reverse().map((i) => `${X(i).toFixed(1)},${Y(sr.band.lo[i]).toFixed(1)}`);
        s += `<polygon points="${up.concat(dn).join(' ')}" fill="${sr.color}" opacity=".16"/>`;
      }
    }
    if (!sr.pts) return;
    let d = '', pen = false;
    sr.pts.forEach((v, i) => {
      if (v === null || v === undefined) { pen = false; return; }
      d += `${pen ? 'L' : 'M'}${X(i).toFixed(1)} ${Y(v).toFixed(1)} `;
      pen = true;
    });
    s += `<path d="${d}" fill="none" stroke="${sr.color}" stroke-width="${sr.width || 2.6}" ${sr.dash ? 'stroke-dasharray="7 6"' : ''} stroke-linejoin="round" stroke-linecap="round"/>`;
    if (sr.dots) {
      sr.pts.forEach((v, i) => {
        if (v === null || v === undefined || (sr.skipDot && sr.skipDot(i))) return;
        s += `<circle cx="${X(i).toFixed(1)}" cy="${Y(v).toFixed(1)}" r="${sr.dots}" fill="${sr.hollow ? '#fff' : sr.color}" stroke="${sr.color}" stroke-width="2"><title>${sr.name ? sr.name + ': ' : ''}${o.yFmt ? o.yFmt(v) : v}</title></circle>`;
      });
    }
  });
  return s + '</svg>';
}

/* ============================================================
 * FORECAST  (least-squares regression + moving average, in C)
 * ============================================================ */
LAB.forecast = async function () {
  await ensureZones();
  const worst = [...ZONES].sort((a, b) => a.healthScore - b.healthScore)[0];
  fillSelect('fcZone', ZONES, { def: worst.zoneId });
  await runForecast();
  loadOutlook();
};

async function runForecast() {
  const id = $('fcZone').value;
  const r = await api('/api/forecast/' + id);
  if (!r.ok) return toast(r.data.error || 'Forecast failed');
  const d = r.data;
  if (!d.enough) {
    $('fcMeta').innerHTML = '';
    $('fcChart').innerHTML = '';
    $('fcLegend').innerHTML = '';
    $('fcNote').innerHTML = `<b>${id}</b> has ${d.count} observation${d.count === 1 ? '' : 's'}. A forecast needs at least ${d.needed}. Record readings in the Observation Log, or use <b>Load sample history</b> to fill every empty zone with six weekly readings.`;
    return;
  }
  const n = d.count, obs = d.observations, fc = d.forecast;
  const total = n + 3;
  const col = d.trend === 'Improving' ? 'good-t' : d.trend === 'Worsening' ? 'bad-t' : 'warn-t';
  const last = fc[2];
  $('fcMeta').innerHTML = `
    <span>Trend <b class="${col}">${d.trend}</b></span>
    <span>Change per reading <b>${d.slope >= 0 ? '+' : ''}${d.slope.toFixed(2)}</b></span>
    <span>Fit quality R² <b>${d.r2.toFixed(2)}</b></span>
    <span>Next reading <b>${f1(fc[0].health)}</b></span>
    <span>In 3 readings <b>${f1(last.health)} · ${last.grade}</b></span>
    <span>Projected risk <b>${last.risk.replace(' RISK', '')}</b></span>`;

  const at = (arr, i) => (i < n ? arr[i] : null);
  const observed = Array.from({ length: total }, (_, i) => at(obs.map((o) => o.health), i));
  const sma = Array.from({ length: total }, (_, i) => at(obs.map((o) => o.sma), i));
  const fit = Array.from({ length: total }, (_, i) => (i < n ? obs[i].fit : fc[i - n].health));
  const fpts = Array.from({ length: total }, (_, i) => (i === n - 1 ? obs[n - 1].health : i >= n ? fc[i - n].health : null));
  const lo = Array.from({ length: total }, (_, i) => (i === n - 1 ? obs[n - 1].health : i >= n ? fc[i - n].low : null));
  const hi = Array.from({ length: total }, (_, i) => (i === n - 1 ? obs[n - 1].health : i >= n ? fc[i - n].high : null));
  const all = observed.concat(fit, lo, hi).filter((v) => v !== null);
  let yMin = Math.max(0, Math.floor(Math.min(...all) - 4)), yMax = Math.min(100, Math.ceil(Math.max(...all) + 4));
  if (yMax - yMin < 10) { yMax = Math.min(100, yMin + 10); yMin = Math.max(0, yMax - 10); }
  const step = Math.max(1, Math.round((yMax - yMin) / 5));
  const ticks = [];
  for (let t = yMin; t <= yMax; t += step) ticks.push(t);
  const refs = [];
  if (yMin < 65 && yMax > 65) refs.push({ v: 65, color: '#22a06b', label: 'Low risk from 65' });
  if (yMin < 45 && yMax > 45) refs.push({ v: 45, color: '#e0a02f', label: 'Medium risk from 45' });

  $('fcChart').innerHTML = svgChart({
    label: `Health forecast for ${id}`, xCount: total, allLabels: true,
    xLabels: obs.map((o) => shortDate(o.date)).concat(['+1', '+2', '+3']), yMin, yMax, yTicks: ticks, yFmt: (v) => Math.round(v * 10) / 10, yTitle: 'Health score', refLines: refs,
    series: [
      { band: { lo, hi }, color: '#1787a0' },
      { name: 'Trend line', color: '#5cc3cf', pts: fit, dash: true, width: 2 },
      { name: 'Moving average', color: '#e0a02f', pts: sma, width: 2 },
      { name: 'Observed', color: '#0f5f78', pts: observed, dots: 4.5 },
      { name: 'Forecast', color: '#0a3d52', pts: fpts, dash: true, dots: 4.5, hollow: true, skipDot: (i) => i === n - 1 },
    ],
  });
  $('fcLegend').innerHTML = legendHtml([
    { label: 'Observed', color: '#0f5f78' }, { label: 'Trend line', color: '#5cc3cf', dash: true },
    { label: '3-reading moving average', color: '#e0a02f' }, { label: 'Forecast', color: '#0a3d52', dash: true }]) +
    `<span><i class="dot-sq" style="background:rgba(23,135,160,.25)"></i>±1σ range (${d.sigma.toFixed(1)} pts)</span>`;
  const drops = d.drops.length ? `Sudden drops detected on ${d.drops.map((x) => `${x.date} (${x.change.toFixed(1)})`).join(', ')}.` : 'No sudden drops in the history.';
  $('fcNote').innerHTML = `Based on ${n} observations of <b>${esc(zoneName(id))}</b>. ${drops}`;
}

async function loadOutlook() {
  const rs = await Promise.all(ZONES.map((z) => api('/api/forecast/' + z.zoneId)));
  const rows = rs.map((r, i) => ({ z: ZONES[i], d: r.data })).filter((x) => x.d && x.d.enough);
  if (!rows.length) { $('fcTable').innerHTML = '<span class="empty">No zone has three observations yet. Use “Load sample history” above.</span>'; return; }
  rows.sort((a, b) => a.d.forecast[2].health - b.d.forecast[2].health);
  $('fcTable').innerHTML = `<table class="table"><thead><tr><th>Zone</th><th>Name</th><th class="num">Latest</th><th>Trend</th><th class="num">Per reading</th><th class="num">In 3 readings</th><th>Projected risk</th></tr></thead><tbody>${
    rows.map(({ z, d }) => {
      const cls = d.trend === 'Improving' ? 'up-t' : d.trend === 'Worsening' ? 'down-t' : '';
      const arrow = d.trend === 'Improving' ? '▲' : d.trend === 'Worsening' ? '▼' : '►';
      const p = d.forecast[2];
      return `<tr style="cursor:pointer" data-id="${z.zoneId}"><td><b>${z.zoneId}</b></td><td>${esc(z.zoneName)}</td>
        <td class="num">${f1(d.observations[d.count - 1].health)}</td><td class="${cls}">${arrow} ${d.trend}</td>
        <td class="num">${d.slope >= 0 ? '+' : ''}${d.slope.toFixed(2)}</td><td class="num"><b>${f1(p.health)}</b> (${p.grade})</td><td>${badge(p.risk)}</td></tr>`;
    }).join('')}</tbody></table>`;
  $('fcTable').querySelectorAll('tr[data-id]').forEach((tr) => tr.addEventListener('click', () => { $('fcZone').value = tr.dataset.id; runForecast(); window.scrollTo({ top: 0, behavior: 'smooth' }); }));
}

$('btnForecast').addEventListener('click', runForecast);
$('fcZone').addEventListener('change', runForecast);
$('btnSeed').addEventListener('click', async () => {
  const r = await api('/api/history/seed', 'POST');
  if (!r.ok) return toast(r.data.error || 'Could not load the sample history');
  toast(r.data.added ? `Added ${r.data.added} sample observations` : 'Every zone already has observations');
  await runForecast();
  loadOutlook();
});

/* ============================================================
 * RISK CLUSTERS  (k-means in C)
 * ============================================================ */
const METRICS = {
  plasticWaste: 'Plastic waste', waterQuality: 'Water quality', erosionLevel: 'Erosion', wasteCoverage: 'Waste coverage',
  visitorLoad: 'Visitor load', environmentalAnomaly: 'Environmental anomaly', healthScore: 'Health score',
};
const CLC = {
  2: ['#22a06b', '#d9503f'], 3: ['#22a06b', '#e0a02f', '#d9503f'],
  4: ['#22a06b', '#1787a0', '#e0a02f', '#d9503f'], 5: ['#0f5f78', '#22a06b', '#e0a02f', '#e8844a', '#d9503f'],
};
let CL = null;

LAB.clusters = async function () {
  await ensureZones();
  [['clX', 'plasticWaste'], ['clY', 'waterQuality']].forEach(([id, def]) => {
    const el = $(id);
    if (!el.options.length) { el.innerHTML = Object.entries(METRICS).map(([k, v]) => `<option value="${k}">${v}</option>`).join(''); el.value = def; }
  });
  runClusters();
};

async function runClusters() {
  const r = await api('/api/clusters?k=' + $('clK').value);
  if (!r.ok) return toast(r.data.error || 'Clustering failed');
  CL = r.data;
  $('clMeta').innerHTML = `<span>Groups <b>${CL.k}</b></span><span>Converged in <b>${CL.iterations} rounds</b></span><span>Total spread <b>${f1(CL.inertia)}</b></span>`;
  drawClusters();
}

function drawClusters() {
  if (!CL) return;
  const xk = $('clX').value, yk = $('clY').value, pal = CLC[CL.k] || CLC[3];
  const rank = {};
  CL.assign.forEach((a) => { rank[a.zoneId] = a.cluster; });
  const W = 640, H = 430, L = 62, R = 24, T = 20, B = 54, pw = W - L - R, ph = H - T - B;
  const dom = (k) => {
    const v = ZONES.map((z) => z[k]), lo = Math.min(...v), hi = Math.max(...v), pad = Math.max(4, (hi - lo) * 0.12);
    return [Math.max(0, Math.floor(lo - pad)), Math.min(100, Math.ceil(hi + pad))];
  };
  const [x0, x1] = dom(xk), [y0, y1] = dom(yk);
  const X = (v) => L + pw * ((v - x0) / (x1 - x0 || 1)), Y = (v) => T + ph * (1 - (v - y0) / (y1 - y0 || 1));
  const ticks = (a, b) => { const st = Math.max(1, Math.round((b - a) / 5)); const t = []; for (let v = a; v <= b; v += st) t.push(v); return t; };
  let s = `<svg viewBox="0 0 ${W} ${H}" role="img" aria-label="Zones plotted by ${METRICS[xk]} and ${METRICS[yk]}, coloured by cluster">`;
  ticks(x0, x1).forEach((t) => { s += `<line x1="${X(t)}" x2="${X(t)}" y1="${T}" y2="${T + ph}" stroke="#e8f1f3"/><text x="${X(t)}" y="${H - B + 18}" text-anchor="middle" font-size="11.5" fill="#7d9aa3">${t}</text>`; });
  ticks(y0, y1).forEach((t) => { s += `<line x1="${L}" x2="${L + pw}" y1="${Y(t)}" y2="${Y(t)}" stroke="#e8f1f3"/><text x="${L - 10}" y="${Y(t) + 4}" text-anchor="end" font-size="11.5" fill="#7d9aa3">${t}</text>`; });
  s += `<text x="${L + pw / 2}" y="${H - 10}" text-anchor="middle" font-size="12.5" fill="#5d7c87">${METRICS[xk]}</text>` +
       `<text transform="translate(16 ${T + ph / 2}) rotate(-90)" text-anchor="middle" font-size="12.5" fill="#5d7c87">${METRICS[yk]}</text>`;
  CL.clusters.forEach((c) => {                      /* centroid rings */
    const mem = c.members.map(zoneById).filter(Boolean);
    if (!mem.length) return;
    const cx = mem.reduce((a, z) => a + z[xk], 0) / mem.length, cy = mem.reduce((a, z) => a + z[yk], 0) / mem.length;
    s += `<circle cx="${X(cx).toFixed(1)}" cy="${Y(cy).toFixed(1)}" r="24" fill="${pal[c.rank]}" opacity=".12" stroke="${pal[c.rank]}" stroke-dasharray="4 4"><title>${c.tag} centre</title></circle>`;
  });
  ZONES.forEach((z) => {
    const c = pal[rank[z.zoneId]] || '#999';
    s += `<g><circle cx="${X(z[xk]).toFixed(1)}" cy="${Y(z[yk]).toFixed(1)}" r="10" fill="${c}" stroke="#fff" stroke-width="2.5"><title>${z.zoneId} ${esc(z.zoneName)}\n${METRICS[xk]}: ${z[xk]}\n${METRICS[yk]}: ${z[yk]}</title></circle>
      <text x="${X(z[xk]).toFixed(1)}" y="${(Y(z[yk]) - 15).toFixed(1)}" text-anchor="middle" font-size="11.5" font-weight="bold" fill="#12303b">${z.zoneId}</text></g>`;
  });
  $('clPlot').innerHTML = s + '</svg>';

  $('clList').innerHTML = CL.clusters.map((c) => {
    const ce = c.centroid;
    return `<div class="cl-card" style="border-left-color:${pal[c.rank]}">
      <div class="cl-h"><b>${c.tag}</b><small>${c.size} zone${c.size === 1 ? '' : 's'} · average health ${f1(c.avgHealth)}</small></div>
      <div class="chips">${c.members.map((id) => `<span class="chip" title="${esc(zoneName(id))}">${id}</span>`).join('') || '<span class="empty">no zones</span>'}</div>
      <small style="display:block;margin-top:8px">Typical: plastic ${Math.round(ce.plasticWaste)}% · water ${Math.round(ce.waterQuality)}% · erosion ${Math.round(ce.erosionLevel)}% · waste ${Math.round(ce.wasteCoverage)}%</small></div>`;
  }).join('');

  const e = CL.elbow, maxI = Math.max(...e.map((x) => x.inertia));
  const top = Math.ceil(maxI / 500) * 500 || 500;
  $('clElbow').innerHTML = svgChart({
    label: 'Total spread by number of groups', w: 760, h: 250, xCount: e.length, xLabels: e.map((x) => x.k + (x.k === 1 ? ' group' : ' groups')), allLabels: true,
    yMin: 0, yMax: top, yTicks: [0, top / 4, top / 2, (top * 3) / 4, top].map(Math.round), yTitle: 'Total spread',
    series: [{ name: 'Spread', color: '#1787a0', pts: e.map((x) => x.inertia), dots: 5 },
             { name: 'Chosen', color: '#d9503f', pts: e.map((x) => (x.k === CL.k ? x.inertia : null)), dots: 8, width: 0.01 }],
  });
}

$('btnCluster').addEventListener('click', runClusters);
$('clK').addEventListener('change', runClusters);
$('clX').addEventListener('change', drawClusters);
$('clY').addEventListener('change', drawClusters);

/* ============================================================
 * ANOMALIES  (z-scores in C)
 * ============================================================ */
LAB.anomalies = () => runAnomalies();

async function runAnomalies() {
  await ensureZones();
  const thr = $('anThr').value;
  const r = await api('/api/anomalies?threshold=' + thr);
  if (!r.ok) return toast(r.data.error || 'Scan failed');
  const d = r.data;
  const concerns = d.zoneFlags.filter((f) => f.concern).length;
  $('anMeta').innerHTML = `<span>Threshold <b>${d.threshold} σ</b></span><span>Flagged readings <b>${d.flagCount}</b></span><span>Of concern <b>${concerns}</b></span><span>Zones scanned <b>${d.zonesChecked}</b></span>`;

  $('anTable').innerHTML = d.zoneFlags.length ? `<table class="table"><thead><tr><th>Zone</th><th>Reading</th><th class="num">Value</th><th class="num">Typical</th><th>Distance from typical</th><th>Verdict</th></tr></thead><tbody>${
    d.zoneFlags.map((f) => {
      const w = Math.min(Math.abs(f.z), 4) / 4 * 50, c = f.concern ? '#d9503f' : '#22a06b';
      const bar = `<div class="zbar"><i style="${f.z > 0 ? 'left:50%' : 'right:50%'};width:${w}%;background:${c}"></i></div>`;
      return `<tr><td><b>${f.zoneId}</b> ${esc(f.zoneName)}</td><td>${f.metric}</td><td class="num"><b>${f1(f.value)}</b></td>
        <td class="num">${f1(f.mean)} ± ${f1(f.std)}</td><td><div style="display:flex;align-items:center;gap:10px">${bar}<span class="${f.concern ? 'bad-t' : 'good-t'}" style="font-weight:bold">${f.z > 0 ? '+' : ''}${f.z.toFixed(2)}σ</span></div></td>
        <td><span class="tag ${f.concern ? 'concern' : 'good'}">${f.concern ? (f.direction === 'high' ? 'Unusually high' : 'Unusually low') : (f.direction === 'high' ? 'Better: high' : 'Better: low')}</span></td></tr>`;
    }).join('')}</tbody></table>` : '<div class="note">No reading stands out at this threshold. Try a more sensitive setting.</div>';

  $('anHist').innerHTML = d.historyFlags.length ? `<table class="table"><thead><tr><th>Zone</th><th class="num">Latest reading</th><th class="num">Latest change</th><th class="num">Usual change</th><th class="num">Distance</th><th>Direction</th></tr></thead><tbody>${
    d.historyFlags.map((h) => `<tr><td><b>${h.zoneId}</b> ${esc(h.zoneName)}</td><td class="num"><b>${f1(h.latest)}</b></td>
      <td class="num ${h.direction === 'down' ? 'bad-t' : 'good-t'}"><b>${h.change >= 0 ? '+' : ''}${f1(h.change)}</b></td><td class="num">${h.usualChange >= 0 ? '+' : ''}${f1(h.usualChange)} ± ${f1(h.usualStd)}</td>
      <td class="num ${h.direction === 'down' ? 'bad-t' : 'good-t'}"><b>${h.z > 0 ? '+' : ''}${h.z.toFixed(2)}σ</b></td><td><span class="tag ${h.direction === 'down' ? 'concern' : 'good'}">${h.direction === 'down' ? 'Sharp drop' : 'Sharp rise'}</span></td></tr>`).join('')}</tbody></table>`
    : `<div class="note">${d.historyZonesChecked === 0
      ? 'No zone has five or more observations yet. Record readings in the Observation Log, or load a sample history on the Forecast page.'
      : `The latest change in each of the ${d.historyZonesChecked} zones with history is in line with how that zone usually moves.`}</div>`;

  $('anBase').innerHTML = `<table class="table"><thead><tr><th>Reading</th><th class="num">Average</th><th class="num">Spread (σ)</th><th class="num">Flag below</th><th class="num">Flag above</th></tr></thead><tbody>${
    d.baseline.map((b) => `<tr><td>${b.metric}</td><td class="num">${f1(b.mean)}</td><td class="num">${f1(b.std)}</td><td class="num">${f1(b.mean - d.threshold * b.std)}</td><td class="num">${f1(b.mean + d.threshold * b.std)}</td></tr>`).join('')}</tbody></table>`;
}

$('btnAnom').addEventListener('click', runAnomalies);
$('anThr').addEventListener('change', runAnomalies);

/* ============================================================
 * COMPARE
 * ============================================================ */
const CMP_COL = ['#1787a0', '#e0a02f', '#22a06b'];
const CMP_ROWS = [
  ['healthScore', 'Health score', 1], ['plasticWaste', 'Plastic waste', -1], ['waterQuality', 'Water quality', 1], ['erosionLevel', 'Erosion', -1],
  ['wasteCoverage', 'Waste coverage', -1], ['visitorLoad', 'Visitor load', -1], ['environmentalAnomaly', 'Environmental anomaly', -1],
];
const RADAR = [
  ['plasticWaste', 'Low plastic', -1], ['waterQuality', 'Water quality', 1], ['erosionLevel', 'Low erosion', -1],
  ['wasteCoverage', 'Low waste', -1], ['visitorLoad', 'Low crowding', -1], ['environmentalAnomaly', 'Low anomaly', -1],
];

LAB.compare = async function () {
  await ensureZones();
  fillSelect('cmpA', ZONES, { def: ZONES[0].zoneId });
  fillSelect('cmpB', ZONES, { def: ZONES[ZONES.length - 1].zoneId });
  fillSelect('cmpC', ZONES, { none: 'No third zone', def: '' });
  renderCompare();
};

function renderCompare() {
  const ids = [...new Set([$('cmpA').value, $('cmpB').value, $('cmpC').value].filter(Boolean))];
  const zs = ids.map(zoneById).filter(Boolean);
  if (zs.length < 2) { $('cmpTable').innerHTML = '<div class="note">Choose at least two different zones.</div>'; $('cmpRadar').innerHTML = ''; return; }

  const cx = 210, cy = 205, Rr = 132, m = RADAR.length;
  const ang = (i) => -Math.PI / 2 + (i * 2 * Math.PI) / m;
  const pt = (i, v) => [cx + Rr * (v / 100) * Math.cos(ang(i)), cy + Rr * (v / 100) * Math.sin(ang(i))];
  let s = `<svg viewBox="0 0 420 410" role="img" aria-label="Radar chart comparing zones; further out is better">`;
  [25, 50, 75, 100].forEach((lv) => { s += `<polygon points="${RADAR.map((_, i) => pt(i, lv).map((c) => c.toFixed(1)).join(',')).join(' ')}" fill="${lv === 100 ? '#f1fafb' : 'none'}" stroke="#d3e6e9"/>`; });
  RADAR.forEach((r, i) => {
    const [ex, ey] = pt(i, 100), [lx, ly] = pt(i, 118);
    s += `<line x1="${cx}" y1="${cy}" x2="${ex.toFixed(1)}" y2="${ey.toFixed(1)}" stroke="#d3e6e9"/><text x="${lx.toFixed(1)}" y="${(ly + 4).toFixed(1)}" text-anchor="${Math.abs(lx - cx) < 12 ? 'middle' : lx > cx ? 'start' : 'end'}" font-size="12" fill="#5d7c87">${r[1]}</text>`;
  });
  zs.forEach((z, k) => {
    const pts = RADAR.map((r, i) => pt(i, r[2] === 1 ? z[r[0]] : 100 - z[r[0]]));
    s += `<polygon points="${pts.map((p) => p.map((c) => c.toFixed(1)).join(',')).join(' ')}" fill="${CMP_COL[k]}" fill-opacity=".16" stroke="${CMP_COL[k]}" stroke-width="2.5" stroke-linejoin="round"/>`;
    pts.forEach((p) => { s += `<circle cx="${p[0].toFixed(1)}" cy="${p[1].toFixed(1)}" r="3.5" fill="${CMP_COL[k]}"/>`; });
  });
  $('cmpRadar').innerHTML = s + '</svg><div class="legend" style="justify-content:center">' +
    swatchHtml(zs.map((z, k) => ({ label: `${z.zoneId} ${esc(z.zoneName)}`, color: CMP_COL[k] }))) + '</div><div class="hint-line" style="text-align:center;margin:8px 0 0">Further from the centre is better on every axis.</div>';

  const wins = zs.map(() => 0);
  const rows = CMP_ROWS.map(([k, label, dir]) => {
    const vals = zs.map((z) => z[k]), best = dir === 1 ? Math.max(...vals) : Math.min(...vals);
    const tie = vals.filter((v) => v === best).length === vals.length;
    return `<tr><td>${label}</td>${zs.map((z, i) => {
      const isBest = !tie && z[k] === best;
      if (isBest && k !== 'healthScore') wins[i]++;
      return `<td class="num"><span class="${isBest ? 'best' : ''}" style="padding:2px 9px">${k === 'healthScore' ? f1(z[k]) : z[k] + '%'}</span></td>`;
    }).join('')}</tr>`;
  }).join('');
  $('cmpTable').innerHTML = `<table class="table"><thead><tr><th></th>${zs.map((z, i) => `<th class="num" style="color:${CMP_COL[i]}">${z.zoneId}<br><span style="font-weight:normal;text-transform:none;letter-spacing:0">${esc(z.zoneName)}</span></th>`).join('')}</tr></thead><tbody>${rows}
    <tr><td>Grade</td>${zs.map((z) => `<td class="num"><span class="grade g-${z.grade}" style="padding:2px 10px;border-radius:8px;font-weight:bold">${z.grade}</span></td>`).join('')}</tr>
    <tr><td>Risk</td>${zs.map((z) => `<td class="num">${badge(z.riskLevel)}</td>`).join('')}</tr>
    <tr><td><b>Readings won</b></td>${zs.map((z, i) => `<td class="num"><b>${wins[i]}</b> of ${CMP_ROWS.length - 1}</td>`).join('')}</tr></tbody></table>`;
}

['cmpA', 'cmpB', 'cmpC'].forEach((id) => $(id).addEventListener('change', renderCompare));

/* ============================================================
 * REPORT  (aggregates zones, alerts, tasks, clusters, anomalies)
 * ============================================================ */
let REPORT = null;

function mainIssue(z) {                              /* same rules the C engine uses for alerts */
  if (z.waterQuality <= 50) return 'Poor water quality';
  if (z.plasticWaste >= 65) return 'High plastic pollution';
  if (z.erosionLevel >= 60) return 'Severe erosion';
  if (z.wasteCoverage >= 60) return 'Waste coverage rising';
  if (z.environmentalAnomaly >= 35) return 'Environmental anomaly';
  if (z.riskLevel.startsWith('HIGH')) return 'Overall health critical';
  return 'Routine monitoring';
}

LAB.reports = async function () {
  await fetchZones();
  const [h, al, tk, cl, an] = await Promise.all([api('/api/health'), api('/api/alerts'), api('/api/tasks'), api('/api/clusters?k=3'), api('/api/anomalies?threshold=2')]);
  const zs = [...ZONES].sort((a, b) => a.healthScore - b.healthScore);
  const alerts = (al.data && al.data.alerts) || [], tasks = (tk.data && tk.data.tasks) || [];
  const clusters = cl.ok ? cl.data.clusters : [], flags = an.ok ? an.data.zoneFlags.filter((f) => f.concern).slice(0, 6) : [];
  REPORT = { zs, alerts, tasks, h: h.data };
  const hd = h.data || {};
  const tableRows = (list, off) => list.map((z, i) => `<tr><td><b>${off + i + 1}</b></td><td><b>${z.zoneId}</b></td><td>${esc(z.zoneName)}</td><td class="num"><b>${f1(z.healthScore)}</b></td>
    <td><span class="grade g-${z.grade}" style="padding:2px 10px;border-radius:8px;font-weight:bold">${z.grade}</span></td><td>${badge(z.riskLevel)}</td><td>${mainIssue(z)}</td></tr>`).join('');
  const head = '<thead><tr><th>#</th><th>Zone</th><th>Name</th><th class="num">Health</th><th>Grade</th><th>Risk</th><th>Main issue</th></tr></thead>';
  const best = [...zs].reverse().slice(0, 3);
  const pal = CLC[3];

  $('reportBody').innerHTML = `
    <div class="card"><div class="report-head"><div><h2>Beach health report</h2><span>Generated ${new Date().toLocaleString()} · ${esc(hd.dataSource || '')}</span></div></div></div>
    <div class="stats">
      <div class="stat"><span>Zones</span><b>${hd.totalZones ?? zs.length}</b></div>
      <div class="stat"><span>Average health</span><b>${hd.averageHealth ?? '-'}</b></div>
      <div class="stat good"><span>Low risk</span><b>${hd.lowRisk ?? 0}</b></div>
      <div class="stat warn"><span>Medium risk</span><b>${hd.mediumRisk ?? 0}</b></div>
      <div class="stat bad"><span>High risk</span><b>${hd.highRisk ?? 0}</b></div>
    </div>
    <div class="card"><h3>Zones needing attention</h3><div class="table-wrap"><table class="table">${head}<tbody>${tableRows(zs.slice(0, 5), 0)}</tbody></table></div></div>
    <div class="grid2">
      <div class="card"><h3>Best performing zones</h3><div class="table-wrap"><table class="table">${head}<tbody>${tableRows(best, 0)}</tbody></table></div></div>
      <div class="card"><h3>Risk groups</h3>${clusters.length ? clusters.map((c) => `
        <div class="cl-card" style="border-left-color:${pal[c.rank] || '#1787a0'}"><div class="cl-h"><b>${c.tag}</b><small>${c.size} zones · avg health ${f1(c.avgHealth)}</small></div>
        <div class="chips">${c.members.map((m) => `<span class="chip">${m}</span>`).join('')}</div></div>`).join('') : '<span class="empty">Not available.</span>'}</div>
    </div>
    <div class="grid2">
      <div class="card"><h3>Active alerts (${alerts.length})</h3><div class="report-list">${alerts.slice(0, 6).map((a) => `<div class="item"><span>${esc(a.message)}</span><span class="sev-${a.severity}">${a.severity}</span></div>`).join('') || '<span class="empty">No active alerts.</span>'}${alerts.length > 6 ? `<span class="hint-line">and ${alerts.length - 6} more</span>` : ''}</div></div>
      <div class="card"><h3>Cleanup schedule (${tasks.length})</h3><div class="report-list">${tasks.slice(0, 6).map((t, i) => `<div class="item ${i === 0 ? 'first' : ''}" data-tag="${i === 0 ? 'NEXT' : ''}"><span>${esc(t.description)}</span><span class="sev-${t.priority}">${t.priority}</span></div>`).join('') || '<span class="empty">No tasks scheduled.</span>'}</div></div>
    </div>
    <div class="card"><h3>Readings that stand out</h3>${flags.length ? `<table class="table"><thead><tr><th>Zone</th><th>Reading</th><th class="num">Value</th><th class="num">Typical</th><th class="num">Distance</th></tr></thead><tbody>${
      flags.map((f) => `<tr><td><b>${f.zoneId}</b> ${esc(f.zoneName)}</td><td>${f.metric}</td><td class="num"><b>${f1(f.value)}</b></td><td class="num">${f1(f.mean)}</td><td class="num bad-t"><b>${f.z > 0 ? '+' : ''}${f.z.toFixed(2)}σ</b></td></tr>`).join('')}</tbody></table>`
      : '<span class="empty">No reading is unusually far from the typical level.</span>'}</div>`;
};

$('btnReportPrint').addEventListener('click', () => { document.body.classList.add('print-report'); window.print(); });
window.addEventListener('afterprint', () => document.body.classList.remove('print-report'));

$('btnReportCsv').addEventListener('click', () => {
  if (!REPORT) return toast('Report is still loading');
  const q = (v) => '"' + String(v).replace(/"/g, '""') + '"';
  const line = (arr) => arr.map(q).join(',');
  const rows = [line(['BlueGuard beach health report', new Date().toLocaleString()]), '', line(['Zones, worst first']),
    line(['rank', 'zone_id', 'zone_name', 'health_score', 'grade', 'risk', 'main_issue'])];
  REPORT.zs.forEach((z, i) => rows.push(line([i + 1, z.zoneId, z.zoneName, z.healthScore, z.grade, z.riskLevel, mainIssue(z)])));
  rows.push('', line(['Active alerts, latest first']), line(['zone_id', 'severity', 'message']));
  REPORT.alerts.forEach((a) => rows.push(line([a.zoneId, a.severity, a.message])));
  rows.push('', line(['Cleanup schedule, next first']), line(['zone_id', 'priority', 'description']));
  REPORT.tasks.forEach((t) => rows.push(line([t.zoneId, t.priority, t.description])));
  const url = URL.createObjectURL(new Blob(['\ufeff' + rows.join('\n')], { type: 'text/csv;charset=utf-8' }));
  const a = document.createElement('a');
  a.href = url; a.download = 'blueguard-beach-report.csv';
  document.body.appendChild(a); a.click(); document.body.removeChild(a);
  setTimeout(() => URL.revokeObjectURL(url), 1000);
  toast('Report downloaded');
});
