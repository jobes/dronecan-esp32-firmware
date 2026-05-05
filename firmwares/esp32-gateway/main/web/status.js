let isPolling = false;
function updateUptime() {
  if (isPolling) return;
  isPolling = true;

  fetch("/api/status")
    .then((response) => response.json())
    .then((data) => {
      document.getElementById("uptime-val").innerText = formatUptime(
        data.uptime,
      );
    })
    .catch((err) => console.error("Error polling uptime:", err))
    .finally(() => {
      isPolling = false;
    });
}
setInterval(updateUptime, 1000);
updateUptime();
