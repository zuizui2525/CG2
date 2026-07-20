document.addEventListener("DOMContentLoaded", () => {
    // UI Elements
    const triggerReason = document.getElementById("trigger-reason");
    const triggerTime = document.getElementById("trigger-time");
    const exitBtn = document.getElementById("exit-btn");
    
    const replayVideo = document.getElementById("replay-video");
    const noReplayMsg = document.getElementById("no-replay-msg");
    const frameCounter = document.getElementById("frame-counter");

    const avgFpsEl = document.getElementById("avg-fps");
    const maxMemEl = document.getElementById("max-mem");
    const reportDetail = document.getElementById("report-detail");

    const copyBtn = document.getElementById("copy-btn");
    const promptText = document.getElementById("prompt-text");

    // Chart Instance
    let metricsChart = null;

    // Initialize Page
    init();

    async function init() {
        try {
            // Load JSON Data
            const res = await fetch("/data/system_log.json");
            if (!res.ok) throw new Error("Log JSON not found");
            const data = await res.json();

            // Populate Metadata
            triggerReason.textContent = data.reason || "UNKNOWN";
            if (data.reason === "LONG_LOAD") {
                triggerReason.classList.add("long-load");
            }
            triggerTime.textContent = data.time_triggered || "Unknown date";
            reportDetail.textContent = data.detail || "No details available.";

            // Load Prompt
            const promptRes = await fetch("/data/prompt.md");
            if (promptRes.ok) {
                promptText.value = await promptRes.text();
            }

            // Setup Performance Metrics & Chart
            setupMetrics(data.logs);

            // Load MP4 Replay
            setupReplayVideo();

        } catch (err) {
            console.error("Initialization error:", err);
            noReplayMsg.classList.remove("hidden");
            if (replayVideo) replayVideo.classList.add("hidden");
            reportDetail.textContent = "Failed to load report data: " + err.message;
        }
    }

    // Performance Data Calculation & Chart Rendering
    function setupMetrics(logs) {
        if (!logs || logs.length === 0) return;

        let totalFps = 0;
        let peakMem = 0;
        const labels = [];
        const fpsData = [];
        const memData = [];

        logs.forEach((log) => {
            totalFps += log.fps;
            if (log.memory_mb > peakMem) {
                peakMem = log.memory_mb;
            }
            labels.push(log.time.toFixed(1) + "s");
            fpsData.push(log.fps);
            memData.push(log.memory_mb);
        });

        const avgFps = totalFps / logs.length;
        avgFpsEl.textContent = `Avg FPS: ${avgFps.toFixed(1)}`;
        maxMemEl.textContent = `Peak Mem: ${peakMem.toFixed(1)} MB`;

        const ctx = document.getElementById('metricsChart').getContext('2d');
        metricsChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    {
                        label: 'FPS',
                        data: fpsData,
                        borderColor: '#00f2fe',
                        backgroundColor: 'rgba(0, 242, 254, 0.1)',
                        borderWidth: 3,
                        pointBackgroundColor: '#00f2fe',
                        pointRadius: 4,
                        yAxisID: 'y-fps',
                        tension: 0.3
                    },
                    {
                        label: 'Memory (MB)',
                        data: memData,
                        borderColor: '#a78bfa',
                        backgroundColor: 'rgba(167, 139, 250, 0.1)',
                        borderWidth: 2,
                        pointBackgroundColor: '#a78bfa',
                        pointRadius: 0,
                        borderDash: [5, 5],
                        yAxisID: 'y-mem',
                        tension: 0.1
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        labels: { color: '#e2e8f0', font: { family: 'Outfit', size: 12 } }
                    }
                },
                scales: {
                    x: {
                        grid: { color: 'rgba(255,255,255,0.05)' },
                        ticks: { color: '#94a3b8', font: { family: 'Outfit' } }
                    },
                    'y-fps': {
                        type: 'linear',
                        position: 'left',
                        grid: { color: 'rgba(255,255,255,0.05)' },
                        ticks: { color: '#00f2fe', font: { family: 'Outfit' } },
                        min: 0,
                        max: 90
                    },
                    'y-mem': {
                        type: 'linear',
                        position: 'right',
                        grid: { display: false },
                        ticks: { color: '#a78bfa', font: { family: 'Outfit' } }
                    }
                }
            }
        });
    }

    // Video Setup
    async function setupReplayVideo() {
        const url = "/data/replay.mp4";
        try {
            const checkRes = await fetch(url, { method: "HEAD" });
            if (!checkRes.ok) {
                throw new Error("Video not found");
            }
            
            if (replayVideo) {
                replayVideo.src = url;
                replayVideo.load();
            }
            if (frameCounter) {
                frameCounter.textContent = "MP4 Replay Loaded";
            }
        } catch (err) {
            console.error("Video load error:", err);
            noReplayMsg.classList.remove("hidden");
            if (replayVideo) replayVideo.classList.add("hidden");
        }
    }

    // Copy Prompt Button Control
    copyBtn.addEventListener("click", async () => {
        try {
            await navigator.clipboard.writeText(promptText.value);
            copyBtn.textContent = "Copied!";
            copyBtn.style.background = "linear-gradient(135deg, #10b981 0%, #059669 100%)";
            copyBtn.style.boxShadow = "0 4px 15px rgba(16, 185, 129, 0.4)";
            
            setTimeout(() => {
                copyBtn.textContent = "Copy Prompt";
                copyBtn.style.background = "";
                copyBtn.style.boxShadow = "";
            }, 2000);
        } catch (err) {
            alert("Failed to copy prompt to clipboard.");
        }
    });

    // Close and Shutdown Server
    exitBtn.addEventListener("click", async () => {
        if (confirm("Close viewer and shutdown the local server?")) {
            try {
                await fetch("/exit");
            } catch (err) {
                // Expected disconnect error
            }
            window.close();
            document.body.innerHTML = `
                <div style="display:flex; justify-content:center; align-items:center; height:100vh; background:#0a0e1a; color:#ef4444; font-family:Outfit, sans-serif;">
                    <div style="text-align:center;">
                        <h1 style="font-size:32px; margin-bottom:16px;">Server Shutdown Successfully</h1>
                        <p style="color:#64748b;">You can safely close this browser window now.</p>
                    </div>
                </div>
            `;
        }
    });
});
