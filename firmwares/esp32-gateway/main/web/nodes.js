let isPolling = false;
function updateNodes() {
  if (isPolling) return;
  isPolling = true;
  fetch("/api/nodes")
    .then((response) => response.json())
    .then((nodes) => {
      const tbody = document.getElementById("node-table-body");
      tbody.innerHTML = "";

      nodes.sort((a, b) => a.id - b.id);

      nodes.forEach((node) => {
        const tr = document.createElement("tr");
        const isOffline = node.last_seen_ms > 3000;
        if (isOffline) tr.classList.add("offline");

        const healthClasses = ["health-0", "health-1", "health-2", "health-3"];
        const healthNames = ["OK", "WARNING", "ERROR", "CRITICAL"];
        const modeNames = {
          0: "OPERATIONAL",
          1: "INITIALIZATION",
          2: "MAINTENANCE",
          3: "SW_UPDATE",
          7: "OFFLINE",
        };

        let healthClass = healthClasses[node.health] || "health-2";
        let healthName = healthNames[node.health] || "UNKNOWN";
        let modeName = modeNames[node.mode] || "UNKNOWN";

        if (isOffline) {
          healthName = "OFFLINE";
          modeName = "OFFLINE";
          healthClass = "health-offline";
        }

        const formattedUid = node.uid.match(/.{1,2}/g).join(":");
        const shortName = node.name.includes(".")
          ? node.name.split(".").pop()
          : node.name;
        const uidParts = formattedUid.split(":");
        const shortUid = `${uidParts[0]}:${uidParts[1]}:...:${uidParts[uidParts.length - 2]}:${uidParts[uidParts.length - 1]}`;

        tr.innerHTML = `
                    <td class="node-id">${node.id}</td>
                    <td><span class="node-name" title="${node.name}">${shortName}</span></td>
                    <td><span class="node-uid" title="${formattedUid}">${shortUid}</span></td>
                    <td>
                        <span class="status-badge ${healthClass}">
                            ${!isOffline ? '<span class="pulse"></span>' : ""}
                            ${healthName}
                        </span>
                    </td>
                    <td style="font-size: 0.8rem; color: #aaa;">${modeName}</td>
                    <td>${formatUptime(node.uptime)}</td>
                    <td>${(node.last_seen_ms / 1000).toFixed(1)}s ago</td>
                `;
        tbody.appendChild(tr);
      });

      if (nodes.length === 0) {
        tbody.innerHTML =
          '<tr><td colspan="6" style="text-align:center; padding: 2rem; color: #666;">No nodes discovered yet...</td></tr>';
      }
    })
    .catch((err) => console.error("Error fetching nodes:", err))
    .finally(() => {
      isPolling = false;
    });
}

// Handle tap on truncated fields for mobile
document.addEventListener("click", function (e) {
  if (
    e.target.classList.contains("node-name") ||
    e.target.classList.contains("node-uid")
  ) {
    const title = e.target.classList.contains("node-name")
      ? "Node Name"
      : "Unique ID";
    const text = e.target.getAttribute("title");

    if (text && text.length > 0) {
      if (window.innerWidth < 800) {
        document.getElementById("dialog-title").innerText = title;
        document.getElementById("dialog-text").innerText = text;
        document.getElementById("info-dialog").showModal();
      }
    }
  }
});

setInterval(updateNodes, 1000);
updateNodes();
