/* BLUEGUARD backend
 * A thin web layer: every data-structure operation (arrays, search, sort,
 * linked list, stack, queue) is executed by the C program `c/beachdsa`,
 * which is started ONCE in --server mode and keeps all state in memory.
 * If the executable is missing, the server compiles it automatically.
 */
const express = require('express');
const path = require('path');
const fs = require('fs');
const { spawn, spawnSync } = require('child_process');

const app = express();
const PORT = process.env.PORT || 3000;
const C_DIR = path.join(__dirname, '..', 'c');
const IS_WIN = process.platform === 'win32';
const BIN = path.join(C_DIR, IS_WIN ? 'beachdsa.exe' : 'beachdsa');
const C_SOURCES = ['main.c', 'zone.c', 'health_score.c', 'array_ops.c', 'search.c',
  'sorting.c', 'linked_list.c', 'stack.c', 'queue.c', 'ny_dataset.c', 'advanced.c'];

app.use(express.json());
/* never let the browser reuse an old copy of the page, styles or scripts */
const FRONTEND_DIR = path.join(__dirname, '..', 'frontend');
app.use(express.static(FRONTEND_DIR, {
  etag: false, lastModified: false,
  setHeaders: (res) => res.setHeader('Cache-Control', 'no-store, max-age=0'),
}));

/* ---------- build the C engine if needed ---------- */
function buildCore() {
  for (const cc of [process.env.CC, 'gcc', 'cc', 'clang'].filter(Boolean)) {
    const r = spawnSync(cc, ['-std=c99', '-O2', '-o', BIN, ...C_SOURCES], { cwd: C_DIR, encoding: 'utf-8' });
    if (!r.error && r.status === 0) return { ok: true, compiler: cc };
    if (!r.error && r.status !== 0) return { ok: false, message: r.stderr };
  }
  return { ok: false, message: 'No C compiler (gcc / cc / clang) found. Install MinGW-w64 or GCC.' };
}

function ensureBinary() {
  if (fs.existsSync(BIN)) {
    const binTime = fs.statSync(BIN).mtimeMs;
    const stale = C_SOURCES.some((f) => fs.statSync(path.join(C_DIR, f)).mtimeMs > binTime);
    if (!stale) return { ok: true };
  }
  console.log('Compiling C engine ...');
  return buildCore();
}

/* ---------- persistent C process with a FIFO command queue ---------- */
let core = null;
let pending = [];
let buffer = '';

function startCore() {
  const built = ensureBinary();
  if (!built.ok) throw new Error('C engine build failed: ' + built.message);

  core = spawn(BIN, ['--server'], { cwd: C_DIR, env: { ...process.env, BEACH_DATA_DIR: path.join(__dirname, '..', 'data') } });
  buffer = '';
  core.stdout.on('data', (chunk) => {
    buffer += chunk.toString();
    let nl;
    while ((nl = buffer.indexOf('\n')) >= 0) {
      const line = buffer.slice(0, nl).trim();
      buffer = buffer.slice(nl + 1);
      const job = pending.shift();
      if (!job) continue;
      try { job.resolve(JSON.parse(line)); }
      catch (e) { job.reject(new Error('Bad output from C engine: ' + line.slice(0, 120))); }
    }
  });
  core.stderr.on('data', (d) => process.stdout.write(d));
  core.on('exit', () => {
    pending.forEach((j) => j.reject(new Error('C engine stopped')));
    pending = [];
    core = null;
  });
  core.on('error', (e) => { pending.forEach((j) => j.reject(e)); pending = []; core = null; });
}

function callCore(...parts) {
  return new Promise((resolve, reject) => {
    try { if (!core) startCore(); } catch (e) { return reject(e); }
    pending.push({ resolve, reject });
    core.stdin.write(parts.map(clean).join('|') + '\n');
  });
}

const clean = (v) => String(v === undefined || v === null ? '' : v).replace(/[|\r\n]/g, ' ').trim();

/* Sends the request to C and maps {ok:false,code,error} to HTTP errors. */
function route(handler) {
  return async (req, res) => {
    try {
      const out = await handler(req);
      if (out && out.ok === false) return res.status(out.code || 400).json({ error: out.error });
      const status = out && out._status ? out._status : 200;
      if (out && out._status) delete out._status;
      res.status(status).json(out);
    } catch (err) {
      res.status(503).json({ error: err.message });
    }
  };
}

/* ---------- zones / health ---------- */
app.get('/api/zones', route(async () => ({ source: 'c-core', zones: await callCore('zones') })));

app.get('/api/zones/:id', route(async (req) => {
  const zones = await callCore('zones');
  const z = zones.find((x) => x.zoneId === req.params.id.toUpperCase());
  return z || { ok: false, code: 404, error: 'Zone not found' };
}));

app.get('/api/health', route(async () => ({ source: 'c-core', ...(await callCore('health')) })));

/* ---------- search / sort ---------- */
app.post('/api/search', route(async (req) => {
  const { method, zoneId } = req.body;
  if (!zoneId) return { ok: false, code: 400, error: 'zoneId is required' };
  return callCore('search', method === 'binary' ? 'binary' : 'linear', zoneId);
}));

app.post('/api/sort', route(async (req) => {
  const { algorithm, key, direction } = req.body;
  const algo = ['bubble', 'insertion', 'merge', 'quick', 'heap', 'radix'].includes(algorithm) ? algorithm : 'bubble';
  const k = ['health', 'plastic', 'water', 'zoneid'].includes(key) ? key : 'health';
  return callCore('sort', algo, k, direction === 'desc' ? 'desc' : 'asc');
}));

/* ---------- monitoring history (linked list in C) ---------- */
app.get('/api/history', route(() => callCore('hist_list')));
app.get('/api/history/search/:id', route((req) => callCore('hist_search', req.params.id)));
app.get('/api/history/zone/:id', route((req) => callCore('hist_zone', req.params.id)));

app.post('/api/history', route(async (req) => {
  const b = req.body;
  if (!b.zoneId || !b.date) return { ok: false, code: 400, error: 'zoneId and date are required' };
  /* health score is computed by the C engine, never sent by the browser */
  const out = await callCore('hist_add', b.atBeginning ? 1 : 0, b.zoneId, b.date,
    b.plasticWaste, b.waterQuality, b.erosionLevel);
  if (out.ok) out._status = 201;
  return out;
}));

app.delete('/api/history/:id', route((req) => callCore('hist_del', req.params.id)));

app.put('/api/history/:id', route((req) => {
  const b = req.body;
  return callCore('hist_upd', req.params.id, b.plasticWaste, b.waterQuality, b.erosionLevel, b.healthScore);
}));

/* ---------- alerts (stack in C) ---------- */
app.get('/api/alerts', route(() => callCore('alert_list')));

app.post('/api/alerts', route(async (req) => {
  const { zoneId, message, severity } = req.body;
  const out = await callCore('alert_push', zoneId, severity || 'MEDIUM', message);
  if (out.ok) out._status = 201;
  return out;
}));

app.delete('/api/alerts', route(() => callCore('alert_pop')));
app.delete('/api/alerts/all', route(() => callCore('alert_clear')));

/* ---------- cleanup tasks (queue in C) ---------- */
app.get('/api/tasks', route(() => callCore('task_list')));

app.post('/api/tasks', route(async (req) => {
  const { zoneId, description, priority } = req.body;
  const out = await callCore('task_push', zoneId, priority || 'MEDIUM', description);
  if (out.ok) out._status = 201;
  return out;
}));

app.delete('/api/tasks', route(() => callCore('task_pop')));
app.delete('/api/tasks/all', route(() => callCore('task_clear')));

/* sorts zones worst-first in C and schedules cleanup for the worst N */
app.post('/api/tasks/plan', route((req) => callCore('plan', parseInt(req.body.count, 10) || 3)));

/* ---------- advanced analytics (all computed by the C engine) ---------- */
const intParam = (v, def, lo, hi) => { const n = parseInt(v, 10); return Number.isFinite(n) ? Math.min(hi, Math.max(lo, n)) : def; };

/* zone graph: BFS pollution spread, Dijkstra routes, cleanup tour */
app.get('/api/graph', route(() => callCore('graph')));
app.get('/api/graph/spread/:id', route((req) => callCore('bfs', req.params.id)));
app.get('/api/graph/route', route((req) => callCore('route', req.query.from, req.query.to)));
app.get('/api/graph/tour', route((req) => callCore('tour', intParam(req.query.count, 4, 2, 8), req.query.depot || '')));

/* hash table vs linear vs binary lookup */
app.get('/api/lookup/:id?', route((req) => callCore('lookup', req.params.id || '')));

/* sorting benchmark (quadratic sorts are skipped above 20 000 records) */
app.get('/api/benchmark', route((req) => callCore('bench', intParam(req.query.max, 10000, 100, 100000))));

/* regression forecast, k-means clusters, z-score anomalies */
app.get('/api/forecast/:id', route((req) => callCore('forecast', req.params.id)));
app.get('/api/clusters', route((req) => callCore('clusters', intParam(req.query.k, 3, 2, 5))));
app.get('/api/anomalies', route((req) => callCore('anomaly', parseFloat(req.query.threshold) || 1.5)));

/* weekly sample observations for zones that have none (feeds the forecast) */
app.post('/api/history/seed', route((req) => callCore('hist_seed')));

/* ---------- start ---------- */
app.listen(PORT, () => {
  console.log(`Serving frontend from: ${FRONTEND_DIR}`);
  console.log(`BLUEGUARD backend running at http://localhost:${PORT}`);
  try {
    startCore();
    console.log('C engine started: ' + BIN);
  } catch (e) {
    console.log('WARNING: ' + e.message);
  }
});

process.on('exit', () => { if (core) core.kill(); });
process.on('SIGINT', () => process.exit(0));
