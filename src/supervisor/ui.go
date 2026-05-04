package main

const indexHTML = `<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="UTF-8">
<title>RetractorDB Supervisor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js"></script>
<style>
  * { box-sizing: border-box; }
  body { font-family: monospace; margin: 0; padding: 16px; background: #1e1e1e; color: #ccc; }
  h1 { color: #4ec9b0; margin: 0 0 12px; }
  h2 { color: #9cdcfe; border-bottom: 1px solid #333; padding-bottom: 4px; margin: 0 0 10px; }
  .card { background: #252526; padding: 14px; border-radius: 0 4px 4px 4px; }
  input, textarea { background: #3c3c3c; color: #ccc; border: 1px solid #555; padding: 4px 8px;
    font-family: monospace; width: 100%; }
  input[type=checkbox] { width: auto; }
  button { background: #0e639c; color: white; border: none; padding: 6px 14px;
    cursor: pointer; font-family: monospace; margin-top: 6px; }
  button:hover { background: #1177bb; }
  button.danger { background: #8b1a1a; }
  button.danger:hover { background: #b22222; }
  .row { display: flex; gap: 8px; align-items: flex-end; }
  .row input { flex: 1; }
  pre { background: #1a1a1a; padding: 8px; overflow: auto; font-size: 12px;
    max-height: 180px; margin: 8px 0 0; white-space: pre-wrap; }
  #logbox { max-height: 340px; overflow-y: scroll; font-size: 11px; padding: 6px; background: #1a1a1a; }
  .ll { margin: 1px 0; }
  .lx { color: #4ec9b0; } .lq { color: #9cdcfe; } .ld { color: #ce9178; }
  .ok { color: #4ec9b0; } .bad { color: #f44747; }
  canvas { margin-top: 10px; max-width: 100%; }
  #streamData { max-height: 140px; }
  #statusbar { background: #252526; padding: 8px 14px; border-radius: 4px; margin-bottom: 12px;
    display: flex; align-items: center; gap: 16px; }
  .tabs { display: flex; gap: 2px; margin-bottom: 0; }
  .tab { padding: 6px 16px; background: #2d2d2d; color: #888; cursor: pointer;
    border-radius: 4px 4px 0 0; border: 1px solid #333; border-bottom: none; user-select: none; }
  .tab:hover { color: #ccc; }
  .tab.active { background: #252526; color: #4ec9b0; border-color: #444; }
  .tabpanel { display: none; }
  .tabpanel.active { display: block; }
</style>
</head>
<body>
<h1>RetractorDB Supervisor</h1>

<div id="statusbar">
  <span id="status">...</span>
  <button onclick="fetchStatus()" style="margin-top:0">Odśwież</button>
  <button class="danger" onclick="stopXretractor()" style="margin-top:0">Zatrzymaj xretractor</button>
</div>

<div class="tabs">
  <div class="tab active" onclick="showTab('reload')">Przeładowanie</div>
  <div class="tab" onclick="showTab('adhoc')">Adhoc</div>
  <div class="tab" onclick="showTab('queries')">Zapytania</div>
  <div class="tab" onclick="showTab('stream')">Podgląd</div>
  <div class="tab" onclick="showTab('xtrdb')">xtrdb</div>
  <div class="tab" onclick="showTab('logs')">Logi</div>
</div>

<div id="tab-reload" class="tabpanel active card">
  <h2>Przeładowanie</h2>
  <div class="row">
    <input id="rqlFile" type="text" placeholder="ścieżka do pliku .rql" />
    <button onclick="reload()">Przeładuj</button>
  </div>
  <pre id="reloadResult"></pre>
</div>

<div id="tab-adhoc" class="tabpanel card">
  <h2>Zapytanie adhoc <small style="color:#666">(tymczasowe, nie przetrwa restartu)</small></h2>
  <textarea id="adhocQuery" rows="3" placeholder="SELECT ..."></textarea>
  <button onclick="adhoc()">Wyślij</button>
  <pre id="adhocResult"></pre>
</div>

<div id="tab-queries" class="tabpanel card">
  <h2>Drzewo zapytań</h2>
  <button onclick="fetchQueries()">Pobierz</button>
  <pre id="queriesResult"></pre>
</div>

<div id="tab-stream" class="tabpanel card">
  <h2>Podgląd strumienia (SSE)</h2>
  <div class="row">
    <input id="streamName" type="text" placeholder="nazwa strumienia" />
    <input id="windowSize" type="number" value="50" min="1" max="1000" style="width:80px" title="rozmiar okna (liczba linii)" />
    <button onclick="startStream()">Start</button>
    <button class="danger" onclick="stopStream()">Stop</button>
  </div>
  <pre id="streamData"></pre>
  <canvas id="streamChart" height="120"></canvas>
</div>

<div id="tab-xtrdb" class="tabpanel card">
  <h2>xtrdb</h2>
  <div class="row">
    <input id="xtrdbArgs" type="text" placeholder="argumenty (np. -d plik.bin)" />
    <button onclick="runXtrdb()">Uruchom</button>
  </div>
  <pre id="xtrdbResult"></pre>
</div>

<div id="tab-logs" class="tabpanel card">
  <h2>Logi procesów</h2>
  <label><input type="checkbox" id="autoLogs" checked> auto-odśwież (3s)</label>
  <button onclick="fetchLogs()" style="margin-left:8px">Odśwież</button>
  <div id="logbox"></div>
</div>

<script>
let sse = null, chart = null;
const chartDataset = { labels: [], datasets: [{ label: 'wartość', data: [],
  borderColor: '#4ec9b0', backgroundColor: 'rgba(78,201,176,0.1)', tension: 0.2, pointRadius: 2 }] };

function showTab(name) {
  document.querySelectorAll('.tab').forEach((t, i) => {
    const panels = document.querySelectorAll('.tabpanel');
    t.classList.toggle('active', t.textContent.trim() === {
      reload:'Przeładowanie', adhoc:'Adhoc', queries:'Zapytania',
      stream:'Podgląd', xtrdb:'xtrdb', logs:'Logi'
    }[name]);
  });
  document.querySelectorAll('.tabpanel').forEach(p => {
    p.classList.toggle('active', p.id === 'tab-' + name);
  });
}

async function api(url, opts) {
  try { return await fetch(url, opts); }
  catch(e) { console.error(url, e); return null; }
}

async function stopXretractor() {
  const r = await api('/api/stop', { method:'POST' });
  const d = r ? await r.json() : {};
  if (d.error) alert('Błąd: ' + d.error + (d.output ? '\n' + d.output : ''));
  fetchStatus();
}

async function fetchStatus() {
  const r = await api('/health');
  if (!r) return;
  const d = await r.json();
  const cls = d.xretractor ? 'ok' : 'bad';
  document.getElementById('status').innerHTML =
    '<span class="' + cls + '">■ xretractor: ' + (d.xretractor ? 'running' : 'stopped') + '</span>' +
    (d.queryFile ? ' &nbsp;<code>' + d.queryFile + '</code>' : '');
}

async function reload() {
  const file = document.getElementById('rqlFile').value.trim();
  if (!file) return;
  const r = await api('/api/reload', { method:'POST',
    headers:{'Content-Type':'application/json'}, body: JSON.stringify({file}) });
  document.getElementById('reloadResult').textContent = r ? JSON.stringify(await r.json(), null, 2) : 'error';
  fetchStatus();
}

async function adhoc() {
  const query = document.getElementById('adhocQuery').value.trim();
  if (!query) return;
  const r = await api('/api/adhoc', { method:'POST',
    headers:{'Content-Type':'application/json'}, body: JSON.stringify({query}) });
  document.getElementById('adhocResult').textContent = r ? JSON.stringify(await r.json(), null, 2) : 'error';
}

async function fetchQueries() {
  const r = await api('/api/queries');
  const d = r ? await r.json() : {};
  document.getElementById('queriesResult').textContent = d.output || d.error || 'brak danych';
}

function startStream() {
  if (sse) sse.close();
  const name = document.getElementById('streamName').value.trim();
  if (!name) return;
  const el = document.getElementById('streamData');
  el.textContent = '';

  const ctx = document.getElementById('streamChart').getContext('2d');
  if (chart) chart.destroy();
  chartDataset.labels = [];
  chartDataset.datasets[0].data = [];
  chart = new Chart(ctx, { type: 'line', data: chartDataset,
    options: { animation: false, responsive: true,
      plugins: { legend: { labels: { color: '#ccc' } } },
      scales: { x: { ticks: { color:'#888', maxTicksLimit: 10 } },
                y: { ticks: { color:'#888' } } } } });

  sse = new EventSource('/api/stream/' + encodeURIComponent(name));
  sse.onmessage = (e) => {
    const win = Math.max(1, parseInt(document.getElementById('windowSize').value, 10) || 50);
    const lines = el.textContent.split('\n').filter(l => l !== '');
    lines.push(e.data);
    if (lines.length > win) lines.splice(0, lines.length - win);
    el.textContent = lines.join('\n');
    el.scrollTop = el.scrollHeight;

    const m = e.data.match(/[-\d.]+/);
    if (m && chart) {
      const t = new Date().toLocaleTimeString('pl');
      chartDataset.labels.push(t);
      chartDataset.datasets[0].data.push(parseFloat(m[0]));
      if (chartDataset.labels.length > win) {
        chartDataset.labels.shift();
        chartDataset.datasets[0].data.shift();
      }
      chart.update();
    }
  };
  sse.onerror = () => { el.textContent += '\n[połączenie zakończone]'; };
}

function stopStream() {
  if (sse) { sse.close(); sse = null; }
}

async function runXtrdb() {
  const raw = document.getElementById('xtrdbArgs').value.trim();
  const args = raw ? raw.split(/\s+/) : [];
  const qs = args.map(a => 'arg=' + encodeURIComponent(a)).join('&');
  const r = await api('/api/xtrdb' + (qs ? '?' + qs : ''));
  const d = r ? await r.json() : {};
  document.getElementById('xtrdbResult').textContent = d.output || d.error || 'brak danych';
}

async function fetchLogs() {
  const r = await api('/api/logs');
  if (!r) return;
  const logs = await r.json();
  const box = document.getElementById('logbox');
  const atBottom = box.scrollHeight - box.scrollTop - box.clientHeight < 20;
  const cls = { xretractor: 'lx', xqry: 'lq', xtrdb: 'ld' };
  box.innerHTML = logs.map(l =>
    '<div class="ll ' + (cls[l.process] || '') + '">' +
    l.t + ' [' + l.process + '] ' + escHtml(l.line) + '</div>'
  ).join('');
  if (atBottom) box.scrollTop = box.scrollHeight;
}

function escHtml(s) {
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

fetchStatus();
fetchQueries();
fetchLogs();
setInterval(() => {
  fetchStatus();
  if (document.getElementById('autoLogs').checked) fetchLogs();
}, 3000);
</script>
</body>
</html>`
