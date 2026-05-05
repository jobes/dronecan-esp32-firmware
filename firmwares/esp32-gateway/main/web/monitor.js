let monitors = JSON.parse(localStorage.getItem("dc_monitors") || "[]");
let history = {}; // id -> array of values
let isPolling = false;

const PRESETS = {
  pressure: {
    name: "Air Pressure",
    id: 1028,
    signature: "0x44DC4133A6B487BA",
    bit: 0,
    len: 32,
    type: "float",
  },
  temperature: {
    name: "Air Temperature",
    id: 1029,
    signature: "0x19932AA9E9558988",
    bit: 0,
    len: 16,
    type: "float16",
  },
};

function normalizeSignature(sig) {
  if (sig === undefined || sig === null) return "";
  let s = String(sig).trim();
  if (s === "") return "";
  if (!s.startsWith("0x") && !s.startsWith("0X")) {
    s = `0x${s}`;
  }
  return s;
}

function applyPreset(key) {
  const p = PRESETS[key];
  if (p) {
    document.getElementById("m-name").value = p.name;
    document.getElementById("m-id").value = p.id;
    document.getElementById("m-signature").value = p.signature;
    document.getElementById("m-bit").value = p.bit;
    document.getElementById("m-len").value = p.len;
    document.getElementById("m-type").value = p.type;
  }
}

function saveMonitors() {
  localStorage.setItem("dc_monitors", JSON.stringify(monitors));
}

function openAddMonitor() {
  const dialog = document.getElementById("monitor-dialog");
  document.getElementById("monitor-form").reset();
  document.getElementById("monitor-index").value = "";
  document.getElementById("m-presets").value = "";
  dialog.showModal();
}

function closeMonitorDialog() {
  document.getElementById("monitor-dialog").close();
}

document.getElementById("monitor-form").addEventListener("submit", (e) => {
  const idx = document.getElementById("monitor-index").value;
  const monitor = {
    uid: idx === "" ? Date.now().toString() : monitors[parseInt(idx)].uid,
    name: document.getElementById("m-name").value,
    id: parseInt(document.getElementById("m-id").value),
    signature: normalizeSignature(document.getElementById("m-signature").value),
    bit: parseInt(document.getElementById("m-bit").value),
    len: parseInt(document.getElementById("m-len").value),
    type: document.getElementById("m-type").value,
  };

  if (idx === "") {
    monitors.push(monitor);
  } else {
    monitors[parseInt(idx)] = monitor;
  }
  saveMonitors();
  renderTable();
});

function moveMonitor(idx, direction) {
  const newIdx = idx + direction;
  if (newIdx >= 0 && newIdx < monitors.length) {
    const temp = monitors[idx];
    monitors[idx] = monitors[newIdx];
    monitors[newIdx] = temp;
    saveMonitors();
    renderTable();
  }
}

function deleteMonitor(idx) {
  const monitor = monitors[idx];
  document.getElementById("delete-name").textContent = monitor.name;
  document.getElementById("delete-index").value = idx;
  document.getElementById("delete-dialog").showModal();
}

function closeDeleteDialog() {
  document.getElementById("delete-dialog").close();
}

function confirmDelete() {
  const idx = parseInt(document.getElementById("delete-index").value);
  const monitor = monitors[idx];
  if (monitor) {
    delete history[monitor.uid];
    expandedRows.delete(monitor.uid);
  }
  monitors.splice(idx, 1);
  saveMonitors();
  renderTable();
  closeDeleteDialog();
}

function renderTable() {
  const tbody = document.getElementById("monitor-list");
  tbody.innerHTML = "";
  monitors.forEach((m, i) => {
    if (!m.uid) m.uid = i.toString() + "_" + Date.now(); // Migration for old data
    const isExpanded = expandedRows.has(m.uid);
    const tr = document.createElement("tr");
    tr.id = `row-${m.uid}`;
    if (isExpanded) tr.classList.add("expanded");
    tr.innerHTML = `
            <td onclick="toggleExpand('${m.uid}')" style="cursor:pointer; font-weight:600;">
                <span class="row-indicator ${isExpanded ? "rotated" : ""}">▶</span> ${m.name}
            </td>
            <td class="monitor-value" id="val-${m.uid}">--</td>
            <td class="actions-cell">
                <button class="btn-icon" title="Move Up" onclick="moveMonitor(${i}, -1)">↑</button>
                <button class="btn-icon" title="Move Down" onclick="moveMonitor(${i}, 1)">↓</button>
                <button class="btn-icon btn-delete" title="Delete" onclick="deleteMonitor(${i})">🗑️</button>
            </td>
        `;
    tbody.appendChild(tr);

    const expandTr = document.createElement("tr");
    expandTr.id = `expand-${m.uid}`;
    expandTr.style.display = isExpanded ? "table-row" : "none";
    expandTr.innerHTML = `
            <td colspan="4" style="padding: 0 12px 12px 12px;">
                <div class="graph-container">
                    <canvas id="canvas-${m.uid}" style="width:100%; height:100%;"></canvas>
                </div>
            </td>
        `;
    tbody.appendChild(expandTr);
    if (isExpanded) setTimeout(() => renderGraph(m.uid), 0);
  });
}

let expandedRows = new Set();
function toggleExpand(uid) {
  const expandTr = document.getElementById(`expand-${uid}`);
  const rowTr = document.getElementById(`row-${uid}`);
  const indicator = rowTr.querySelector(".row-indicator");
  if (expandedRows.has(uid)) {
    expandedRows.delete(uid);
    expandTr.style.display = "none";
    rowTr.classList.remove("expanded");
    if (indicator) indicator.classList.remove("rotated");
  } else {
    expandedRows.add(uid);
    expandTr.style.display = "table-row";
    rowTr.classList.add("expanded");
    if (indicator) indicator.classList.add("rotated");
    setTimeout(() => renderGraph(uid), 0);
  }
}

function decodeFloat16(binary) {
  const exponent = (binary & 0x7c00) >> 10;
  const fraction = binary & 0x03ff;
  const sign = binary & 0x8000 ? -1 : 1;

  if (exponent === 0) return sign * Math.pow(2, -14) * (fraction / 1024);
  if (exponent === 31) return fraction ? NaN : sign * Infinity;
  return sign * Math.pow(2, exponent - 15) * (1 + fraction / 1024);
}

function extractBits(hex, startBit, length, type) {
  if (!hex) return null;
  const bytes = new Uint8Array(
    hex.match(/.{1,2}/g).map((byte) => parseInt(byte, 16)),
  );
  let val = 0n;
  for (let i = 0; i < length; i++) {
    let bitPos = startBit + i;
    let byteIdx = Math.floor(bitPos / 8);
    let bitIdx = bitPos % 8;
    if (byteIdx < bytes.length) {
      if (bytes[byteIdx] & (1 << bitIdx)) {
        val |= 1n << BigInt(i);
      }
    }
  }

  if (type === "float") {
    let buf = new ArrayBuffer(4);
    let view = new DataView(buf);
    view.setUint32(0, Number(val & 0xffffffffn), true);
    return view.getFloat32(0, true).toFixed(3);
  }
  if (type === "float16") {
    return decodeFloat16(Number(val & 0xffffn)).toFixed(3);
  }
  if (type === "int") {
    if (val & (1n << BigInt(length - 1))) {
      val -= 1n << BigInt(length);
    }
    return Number(val);
  }
  if (type === "bool") {
    return val !== 0n ? "TRUE" : "FALSE";
  }
  return Number(val);
}

async function poll() {
  if (monitors.length === 0 || isPolling) return;
  isPolling = true;
  const unique = [];
  const seen = new Set();
  for (const m of monitors) {
    const sig = normalizeSignature(m.signature);
    if (!sig) {
      continue;
    }
    const key = `${m.id}:${sig.toLowerCase()}`;
    if (!seen.has(key)) {
      seen.add(key);
      unique.push({ id: m.id, signature: sig });
    }
  }

  if (unique.length === 0) {
    monitors.forEach((m) => {
      const valEl = document.getElementById(`val-${m.uid}`);
      if (valEl) valEl.textContent = "NO SIG";
    });
    isPolling = false;
    return;
  }

  const ids = unique.map((x) => x.id).join(",");
  const sigs = unique.map((x) => x.signature).join(",");
  try {
    const resp = await fetch(`/api/data?ids=${ids}&sigs=${sigs}`);
    const data = await resp.json();

    monitors.forEach((m, i) => {
      const entry = data[m.id];
      if (entry) {
        const val = extractBits(entry.payload, m.bit, m.len, m.type);
        const valEl = document.getElementById(`val-${m.uid}`);
        if (valEl) valEl.textContent = val;

        let numericVal = parseFloat(val);
        if (!isNaN(numericVal)) {
          if (!history[m.uid]) history[m.uid] = [];
          history[m.uid].push(numericVal);
          if (history[m.uid].length > 100) history[m.uid].shift();
          if (expandedRows.has(m.uid)) renderGraph(m.uid);
        }
      } else {
        const valEl = document.getElementById(`val-${m.uid}`);
        if (valEl) valEl.textContent = "TIMEOUT";
      }
    });
  } catch (e) {
    console.error("Polling error", e);
  } finally {
    isPolling = false;
  }
}

function renderGraph(uid) {
  const canvas = document.getElementById(`canvas-${uid}`);
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  const data = history[uid] || [];

  canvas.width = canvas.offsetWidth;
  canvas.height = canvas.offsetHeight;
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  if (data.length < 2) {
    ctx.fillStyle = "#555";
    ctx.font = "12px sans-serif";
    ctx.textAlign = "center";
    ctx.fillText(
      "Waiting for more data points...",
      canvas.width / 2,
      canvas.height / 2,
    );
    return;
  }

  const dmin = Math.min(...data);
  const dmax = Math.max(...data);
  const range = dmax - dmin || 1;

  // Grid lines and labels
  ctx.strokeStyle = "rgba(255, 255, 255, 0.08)";
  ctx.lineWidth = 1;
  ctx.fillStyle = "#0be881";
  ctx.font = "bold 9px monospace";
  ctx.textAlign = "left";

  for (let g = 0; g <= 4; g++) {
    let gy = 12 + (g / 4) * (canvas.height - 24);
    ctx.beginPath();
    ctx.moveTo(0, gy);
    ctx.lineTo(canvas.width, gy);
    ctx.stroke();

    let gval = dmax - (g / 4) * range;
    ctx.fillText(gval.toFixed(2), 5, gy - 2);
  }

  // Line
  ctx.beginPath();
  ctx.strokeStyle = "#0fbcf9";
  ctx.lineWidth = 3;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";

  // Shadow/Glow
  ctx.shadowBlur = 10;
  ctx.shadowColor = "rgba(15, 188, 249, 0.5)";

  data.forEach((v, j) => {
    const x = (j / (data.length - 1)) * canvas.width;
    const y = canvas.height - 12 - ((v - dmin) / range) * (canvas.height - 24);
    if (j === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();

  ctx.shadowBlur = 0;
}

renderTable();
setInterval(poll, 1000);
poll();

window.addEventListener("resize", () => {
  expandedRows.forEach((uid) => renderGraph(uid));
});
