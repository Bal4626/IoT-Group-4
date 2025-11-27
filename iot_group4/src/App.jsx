import { useEffect, useState } from "react";

const API_URL = "https://j9ln1xklt3.execute-api.ap-southeast-1.amazonaws.com/wristbands";

// simple utility: scale node coords into SVG space
function normalisePositions(nodes, width, height, padding = 40) {
    if (!nodes.length) return {};

    const xs = nodes.map((n) => n.x);
    const ys = nodes.map((n) => n.y);

    const minX = Math.min(...xs);
    const maxX = Math.max(...xs);
    const minY = Math.min(...ys);
    const maxY = Math.max(...ys);

    const spanX = maxX - minX || 1;
    const spanY = maxY - minY || 1;

    const scaleX = (width - 2 * padding) / spanX;
    const scaleY = (height - 2 * padding) / spanY;
    const scale = Math.min(scaleX, scaleY);

    const out = {};
    for (const n of nodes) {
        out[n.id] = {
            sx: (n.x - minX) * scale + padding,
            sy: height - ((n.y - minY) * scale + padding), // flip Y for SVG
        };
    }
    return out;
}

function App() {
    const [data, setData] = useState({ nodes: [], edges: [] });
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState("");
    const [isAutoRefresh, setIsAutoRefresh] = useState(true);

    const fetchData = async () => {
        try {
            setLoading(true);
            setError("");
            const res = await fetch(API_URL);
            if (!res.ok) {
                throw new Error(`HTTP ${res.status}`);
            }
            const json = await res.json();
            setData(json);
        } catch (e) {
            console.error(e);
            setError(e.message || "Failed to fetch");
        } finally {
            setLoading(false);
        }
    };

    // Auto-refresh every 3 seconds
    useEffect(() => {
        fetchData(); // initial load

        if (!isAutoRefresh) return;

        const interval = setInterval(() => {
            fetchData();
        }, 3000);

        return () => clearInterval(interval);
    }, [isAutoRefresh]);

    const width = 800;
    const height = 600;
    const positions = normalisePositions(data.nodes, width, height);

    return (
        <div style={{ fontFamily: "system-ui, sans-serif", padding: "1rem" }}>
            <h1>Wristband Graph Viewer</h1>
            <p style={{ fontSize: "0.9rem", color: "#555" }}>
                Fetching from: <code>{API_URL}</code>
            </p>

            <div style={{ marginBottom: "1rem", display: "flex", gap: "0.5rem", alignItems: "center" }}>
                <button onClick={fetchData} disabled={loading}>
                    {loading ? "Refreshing..." : "Refresh Now"}
                </button>
                <button onClick={() => setIsAutoRefresh(!isAutoRefresh)}>
                    {isAutoRefresh ? "⏸ Pause Auto-refresh" : "▶ Resume Auto-refresh"}
                </button>
                <span style={{ fontSize: "0.9rem", color: "#666", marginLeft: "1rem" }}>
                    {isAutoRefresh ? "Auto-refreshing every 3 seconds" : "Manual refresh mode"}
                </span>
            </div>

            {error && (
                <div style={{ color: "red", marginBottom: "0.5rem" }}>
                    Error: {error}
                </div>
            )}

            <svg
                width={width}
                height={height}
                style={{
                    border: "1px solid #ccc",
                    borderRadius: 8,
                    background: "#fafafa",
                }}
            >
                {/* edges */}
                {data.edges.map((e, idx) => {
                    const src = positions[e.source];
                    const tgt = positions[e.target];
                    if (!src || !tgt) return null;
                    return (
                        <g key={idx}>
                            <line
                                x1={src.sx}
                                y1={src.sy}
                                x2={tgt.sx}
                                y2={tgt.sy}
                                stroke="#888"
                                strokeWidth={1}
                            />
                            {/* optional: RSSI label midpoint */}
                            <text
                                x={(src.sx + tgt.sx) / 2}
                                y={(src.sy + tgt.sy) / 2}
                                fontSize="8"
                                fill="#666"
                            >
                                {e.rssi}
                            </text>
                        </g>
                    );
                })}

                {/* nodes */}
                {data.nodes.map((n) => {
                    const pos = positions[n.id];
                    if (!pos) return null;

                    const isAnchor = n.id === "WB1" || n.id === "ANCHOR-TEST";
                    const radius = isAnchor ? 10 : 6;


                    return (
                        <g key={n.id}>
                            <circle
                                cx={pos.sx}
                                cy={pos.sy}
                                r={radius}
                                fill={isAnchor ? "#ff9800" : "#1976d2"}
                                stroke="#fff"
                                strokeWidth={1.5}
                            />
                            <text
                                x={pos.sx}
                                y={pos.sy - radius - 4}
                                fontSize="10"
                                textAnchor="middle"
                                fill="#333"
                            >
                                {n.id}
                            </text>
                        </g>
                    );
                })}
            </svg>
        </div>
    );
}

export default App;