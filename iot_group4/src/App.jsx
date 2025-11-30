import { useEffect, useState } from "react";

const API_URL = "https://j9ln1xklt3.execute-api.ap-southeast-1.amazonaws.com/wristbands";
const LOST_TIMEOUT_MS = 15000;

// simple utility: scale node coords into SVG space
// simple utility: scale node coords into SVG space with (0,0) at the centre
function normalisePositions(nodes, width, height, padding = 40) {
    if (!nodes.length) return {};

    // Find largest absolute extent
    const maxAbsX = Math.max(...nodes.map(n => Math.abs(n.x)), 1e-6);
    const maxAbsY = Math.max(...nodes.map(n => Math.abs(n.y)), 1e-6);

    // Available half-width/half-height inside padding
    const halfWidth = (width / 2) - padding;
    const halfHeight = (height / 2) - padding;

    const scaleX = halfWidth / maxAbsX;
    const scaleY = halfHeight / maxAbsY;
    const scale = Math.min(scaleX, scaleY);

    const cx = width / 2;
    const cy = height / 2;

    const out = {};
    for (const n of nodes) {
        out[n.id] = {
            sx: cx + n.x * scale,   // x right
            sy: cy - n.y * scale,   // y up (SVG is inverted)
        };
    }
    return out;
}

// Align backend layout to user-chosen anchor positions
function alignNodesToAnchors(nodes, anchorAId, anchorBId, targetA, targetB) {
    if (!nodes.length) return nodes;

    const byId = Object.fromEntries(nodes.map(n => [n.id, n]));
    const aNode = byId[anchorAId];
    const bNode = byId[anchorBId];

    if (!aNode || !bNode) {
        // Anchors not present, just return original
        return nodes;
    }

    const ax = aNode.x, ay = aNode.y;
    const bx = bNode.x, by = bNode.y;

    const vBx = bx - ax;
    const vBy = by - ay;
    const vUx = targetB.x - targetA.x;
    const vUy = targetB.y - targetA.y;

    const lenB = Math.hypot(vBx, vBy) || 1;
    const lenU = Math.hypot(vUx, vUy) || 1;

    const scale = lenU / lenB;

    // rotation that maps vB -> vU
    const dot = (vBx * vUx + vBy * vUy) / (lenB * lenU);
    const det = (vBx * vUy - vBy * vUx) / (lenB * lenU);
    const angle = Math.atan2(det, dot);
    const cos = Math.cos(angle);
    const sin = Math.sin(angle);

    return nodes.map(n => {
        // shift so that anchorA is origin
        const px = n.x - ax;
        const py = n.y - ay;

        // rotate + scale
        const rx = scale * (px * cos - py * sin);
        const ry = scale * (px * sin + py * cos);

        // shift into target space
        return {
            ...n,
            x: targetA.x + rx,
            y: targetA.y + ry,
        };
    });
}


// Detect crowd density hotspots using grid-based analysis
function detectCrowdDensity(nodes, gridSize, threshold) {
    if (!nodes.length) return [];

    const grid = {};
    
    // Group nodes into grid cells
    for (const node of nodes) {
        const cellX = Math.floor(node.x / gridSize);
        const cellY = Math.floor(node.y / gridSize);
        const key = `${cellX},${cellY}`;
        
        if (!grid[key]) {
            grid[key] = {
                nodes: [],
                cellX,
                cellY,
                centerX: (cellX + 0.5) * gridSize,
                centerY: (cellY + 0.5) * gridSize,
            };
        }
        grid[key].nodes.push(node);
    }

    // Find cells exceeding threshold
    const alerts = [];
    for (const [key, cell] of Object.entries(grid)) {
        const density = cell.nodes.length;
        if (density >= threshold) {
            alerts.push({
                ...cell,
                density,
                severity: density >= threshold * 2 ? "critical" : "warning",
            });
        }
    }

    return alerts;
}

function App() {
    const [data, setData] = useState({ nodes: [], edges: [] });
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState("");
    const [isAutoRefresh, setIsAutoRefresh] = useState(true);
    
    // Crowd density settings
    const [gridSize, setGridSize] = useState(2.0); // meters
    const [densityThreshold, setDensityThreshold] = useState(5); // nodes per cell
    const [showAlerts, setShowAlerts] = useState(true);
    
    // History tracking
    const [history, setHistory] = useState([]);
    const [showHistory, setShowHistory] = useState(false);
    const [selectedHistoryIndex, setSelectedHistoryIndex] = useState(null);

    // Anchor alignment (user-chosen reference points)
    const [anchorAId, setAnchorAId] = useState("WB3");
    const [anchorBId, setAnchorBId] = useState("WB4");

    // target positions in "real" grid space (e.g. meters)
    const [anchorAX, setAnchorAX] = useState(0);   // WB3 at (0,0)
    const [anchorAY, setAnchorAY] = useState(0);
    const [anchorBX, setAnchorBX] = useState(10);  // WB4 at (10,0) by default
    const [anchorBY, setAnchorBY] = useState(0);

    const fetchData = async () => {
        try {
            setLoading(true);
            setError("");
            const res = await fetch(API_URL);
            if (!res.ok) {
                throw new Error(`HTTP ${res.status}`);
            }
            const json = await res.json();
            
            // Create history entry
            const historyEntry = {
                timestamp: new Date().toISOString(),
                data: json,
                nodeCount: json.nodes?.length || 0,
                edgeCount: json.edges?.length || 0,
            };
            
            // Add to history (keep last 100 entries)
            setHistory(prev => {
                const updated = [historyEntry, ...prev].slice(0, 100);
                // Save to localStorage
                localStorage.setItem('wristband-history', JSON.stringify(updated));
                return updated;
            });
            
            // Only update current data if not viewing history
            if (selectedHistoryIndex === null) {
                setData(json);
            }
        } catch (e) {
            console.error(e);
            setError(e.message || "Failed to fetch");
        } finally {
            setLoading(false);
        }
    };
    
    // Load history from localStorage on mount
    useEffect(() => {
        const saved = localStorage.getItem('wristband-history');
        if (saved) {
            try {
                setHistory(JSON.parse(saved));
            } catch (e) {
                console.error('Failed to load history:', e);
            }
        }
    }, []);
    
    // View a historical snapshot
    const viewHistoryItem = (index) => {
        setSelectedHistoryIndex(index);
        setData(history[index].data);
        setIsAutoRefresh(false); // Pause auto-refresh when viewing history
    };
    
    // Return to live view
    const returnToLive = () => {
        setSelectedHistoryIndex(null);
        setIsAutoRefresh(true);
        fetchData(); // Fetch latest data
    };
    
    // Clear all history
    const clearHistory = () => {
        if (confirm('Are you sure you want to clear all history?')) {
            setHistory([]);
            localStorage.removeItem('wristband-history');
        }
    };
    
    // Download history as JSON
    const downloadHistory = () => {
        const dataStr = JSON.stringify(history, null, 2);
        const dataBlob = new Blob([dataStr], { type: 'application/json' });
        const url = URL.createObjectURL(dataBlob);
        const link = document.createElement('a');
        link.href = url;
        link.download = `wristband-history-${new Date().toISOString()}.json`;
        link.click();
        URL.revokeObjectURL(url);
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

    // Apply anchor-based alignment before normalising
    const alignedNodes = alignNodesToAnchors(
        data.nodes,
        anchorAId,
        anchorBId,
        { x: anchorAX, y: anchorAY },
        { x: anchorBX, y: anchorBY }
    );

    const positions = normalisePositions(alignedNodes, width, height);

    // Use aligned coordinates for density (so it's in your grid space)
    const alerts = detectCrowdDensity(alignedNodes, gridSize, densityThreshold);

    const now = Date.now();

    // All node IDs currently present
    const currentIds = new Set(alignedNodes.map(n => n.id));

    // Build lastSeen map from history
    const lastSeen = {};
    for (const entry of history) {
        const t = new Date(entry.timestamp).getTime();
        if (!Number.isFinite(t)) continue;

        const nodes = entry.data?.nodes || [];
        for (const n of nodes) {
            const prev = lastSeen[n.id];
            if (!prev || t > prev) {
                lastSeen[n.id] = t;
            }
        }
    }

    // Filter to nodes that are NOT in the current payload and have been missing long enough
    const lostNodes = Object.entries(lastSeen)
        .filter(([id, t]) => !currentIds.has(id) && now - t > LOST_TIMEOUT_MS)
        .map(([id, t]) => ({
            id,
            lastSeenAt: new Date(t),
            secondsAgo: Math.round((now - t) / 1000),
        }))
        // most recently seen first
        .sort((a, b) => b.lastSeenAt - a.lastSeenAt);

    return (
        <div style={{ 
            fontFamily: "system-ui, sans-serif", 
            padding: "1rem",
            background: "#fff",
            minHeight: "100vh",
            colorScheme: "light"
        }}>
            
            <p style={{ fontSize: "0.9rem", color: "#333", margin: "0 0 1rem 0" }}>
                Fetching from: <code style={{ background: "#f0f0f0", padding: "2px 6px", borderRadius: 3, color: "#000" }}>{API_URL}</code>
            </p>

            {/* Tab Navigation */}
            <div style={{ 
                display: "flex", 
                gap: "0.5rem", 
                marginBottom: "1rem",
                borderBottom: "2px solid #e0e0e0"
            }}>
                <button
                    onClick={() => setShowHistory(false)}
                    style={{
                        padding: "0.75rem 1.5rem",
                        background: !showHistory ? "#fff" : "transparent",
                        color: !showHistory ? "#2196F3" : "#666",
                        border: "none",
                        borderBottom: !showHistory ? "3px solid #2196F3" : "3px solid transparent",
                        cursor: "pointer",
                        fontWeight: 600,
                        fontSize: "1rem",
                        transition: "all 0.2s"
                    }}
                >
                    🔴 Live View
                </button>
                <button
                    onClick={() => setShowHistory(true)}
                    style={{
                        padding: "0.75rem 1.5rem",
                        background: showHistory ? "#fff" : "transparent",
                        color: showHistory ? "#673AB7" : "#666",
                        border: "none",
                        borderBottom: showHistory ? "3px solid #673AB7" : "3px solid transparent",
                        cursor: "pointer",
                        fontWeight: 600,
                        fontSize: "1rem",
                        transition: "all 0.2s"
                    }}
                >
                    📊 History ({history.length})
                </button>
            </div>

            {/* Controls */}
            <div style={{ 
                marginBottom: "1rem", 
                display: "flex", 
                gap: "1rem", 
                alignItems: "center",
                flexWrap: "wrap"
            }}>
                <button 
                    onClick={fetchData} 
                    disabled={loading}
                    style={{
                        padding: "0.5rem 1rem",
                        background: loading ? "#ccc" : "#2196F3",
                        color: "#fff",
                        border: "none",
                        borderRadius: 4,
                        cursor: loading ? "not-allowed" : "pointer",
                        fontWeight: 500
                    }}
                >
                    {loading ? "Refreshing..." : "Refresh Now"}
                </button>
                <button 
                    onClick={() => setIsAutoRefresh(!isAutoRefresh)}
                    style={{
                        padding: "0.5rem 1rem",
                        background: isAutoRefresh ? "#ff9800" : "#4CAF50",
                        color: "#fff",
                        border: "none",
                        borderRadius: 4,
                        cursor: "pointer",
                        fontWeight: 500
                    }}
                >
                    {isAutoRefresh ? "⏸ Pause Auto-refresh" : "▶ Resume Auto-refresh"}
                </button>
                {selectedHistoryIndex !== null && (
                    <button 
                        onClick={returnToLive}
                        style={{
                            padding: "0.5rem 1rem",
                            background: "#f44336",
                            color: "#fff",
                            border: "none",
                            borderRadius: 4,
                            cursor: "pointer",
                            fontWeight: 500
                        }}
                    >
                        ⬅ Return to Live
                    </button>
                )}
                <span style={{ fontSize: "0.9rem", color: "#333", fontWeight: 500 }}>
                    {selectedHistoryIndex !== null 
                        ? `Viewing History: ${new Date(history[selectedHistoryIndex].timestamp).toLocaleString()}`
                        : isAutoRefresh ? "Auto-refreshing every 3s" : "Manual mode"
                    }
                </span>
            </div>

            {error && (
                <div style={{ 
                    color: "red", 
                    marginBottom: "1rem",
                    padding: "0.5rem",
                    background: "#ffebee",
                    borderRadius: 4
                }}>
                    Error: {error}
                </div>
            )}

            {/* History Panel */}
            {showHistory && (
                <div style={{
                    background: "#f5f5f5",
                    border: "2px solid #673AB7",
                    borderRadius: 8,
                    padding: "1rem",
                    marginBottom: "1rem",
                    maxHeight: "300px",
                    overflow: "auto"
                }}>
                    <div style={{ 
                        display: "flex", 
                        justifyContent: "space-between", 
                        alignItems: "center",
                        marginBottom: "1rem"
                    }}>
                        <h3 style={{ margin: 0, color: "#000" }}>
                            Data History ({history.length} snapshots)
                        </h3>
                        <div style={{ display: "flex", gap: "0.5rem" }}>
                            <button 
                                onClick={downloadHistory}
                                disabled={history.length === 0}
                                style={{
                                    padding: "0.4rem 0.8rem",
                                    background: "#4CAF50",
                                    color: "#fff",
                                    border: "none",
                                    borderRadius: 4,
                                    cursor: history.length === 0 ? "not-allowed" : "pointer",
                                    fontSize: "0.85rem",
                                    fontWeight: 500
                                }}
                            >
                                💾 Download JSON
                            </button>
                            <button 
                                onClick={clearHistory}
                                disabled={history.length === 0}
                                style={{
                                    padding: "0.4rem 0.8rem",
                                    background: "#f44336",
                                    color: "#fff",
                                    border: "none",
                                    borderRadius: 4,
                                    cursor: history.length === 0 ? "not-allowed" : "pointer",
                                    fontSize: "0.85rem",
                                    fontWeight: 500
                                }}
                            >
                                🗑️ Clear All
                            </button>
                        </div>
                    </div>
                    
                    {history.length === 0 ? (
                        <p style={{ color: "#666", textAlign: "center", margin: "2rem 0" }}>
                            No history yet. Data will be automatically saved as you fetch from the API.
                        </p>
                    ) : (
                        <div style={{ display: "grid", gap: "0.5rem" }}>
                            {history.map((entry, idx) => (
                                <div 
                                    key={idx}
                                    onClick={() => viewHistoryItem(idx)}
                                    style={{
                                        padding: "0.75rem",
                                        background: selectedHistoryIndex === idx ? "#E1BEE7" : "#fff",
                                        border: selectedHistoryIndex === idx ? "2px solid #9C27B0" : "1px solid #ddd",
                                        borderRadius: 6,
                                        cursor: "pointer",
                                        transition: "all 0.2s",
                                    }}
                                    onMouseEnter={(e) => {
                                        if (selectedHistoryIndex !== idx) {
                                            e.currentTarget.style.background = "#f0f0f0";
                                        }
                                    }}
                                    onMouseLeave={(e) => {
                                        if (selectedHistoryIndex !== idx) {
                                            e.currentTarget.style.background = "#fff";
                                        }
                                    }}
                                >
                                    <div style={{ 
                                        display: "flex", 
                                        justifyContent: "space-between",
                                        alignItems: "center"
                                    }}>
                                        <div>
                                            <strong style={{ color: "#000" }}>
                                                {new Date(entry.timestamp).toLocaleString()}
                                            </strong>
                                            <div style={{ fontSize: "0.85rem", color: "#666", marginTop: "0.25rem" }}>
                                                {entry.nodeCount} nodes, {entry.edgeCount} edges
                                            </div>
                                        </div>
                                        {selectedHistoryIndex === idx && (
                                            <span style={{ 
                                                color: "#9C27B0", 
                                                fontWeight: "bold",
                                                fontSize: "0.9rem"
                                            }}>
                                                ● VIEWING
                                            </span>
                                        )}
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            )}

            {/* Main Layout: Map on Left, Settings on Right */}
            <div style={{ display: "flex", gap: "1rem", alignItems: "flex-start" }}>
                
                {/* SVG Visualization */}
                <svg
                    width={width}
                    height={height}
                    style={{
                        border: "1px solid #ccc",
                        borderRadius: 8,
                        background: "#fafafa",
                        flexShrink: 0,
                    }}
                >
                    {/* Draw alert zones */}
                    {showAlerts && alerts.map((alert, idx) => {
                        // Convert real-world coords to SVG coords
                        const nodes = alignedNodes;
                        if (!nodes.length) return null;

                        const xs = nodes.map(n => n.x);
                        const ys = nodes.map(n => n.y);
                        const minX = Math.min(...xs);
                        const minY = Math.min(...ys);
                        const maxX = Math.max(...xs);
                        const maxY = Math.max(...ys);
                        const spanX = maxX - minX || 1;
                        const spanY = maxY - minY || 1;
                        const padding = 40;
                        const scaleX = (width - 2 * padding) / spanX;
                        const scaleY = (height - 2 * padding) / spanY;
                        const scale = Math.min(scaleX, scaleY);

                        const cellLeft = alert.cellX * gridSize;
                        const cellBottom = alert.cellY * gridSize;
                        const cellRight = cellLeft + gridSize;
                        const cellTop = cellBottom + gridSize;

                        const x1 = (cellLeft - minX) * scale + padding;
                        const y1 = height - ((cellBottom - minY) * scale + padding);
                        const x2 = (cellRight - minX) * scale + padding;
                        const y2 = height - ((cellTop - minY) * scale + padding);

                        return (
                            <g key={`alert-${idx}`}>
                                <rect
                                    x={Math.min(x1, x2)}
                                    y={Math.min(y1, y2)}
                                    width={Math.abs(x2 - x1)}
                                    height={Math.abs(y2 - y1)}
                                    fill={alert.severity === "critical" ? "rgba(244, 67, 54, 0.2)" : "rgba(255, 152, 0, 0.2)"}
                                    stroke={alert.severity === "critical" ? "#f44336" : "#ff9800"}
                                    strokeWidth={2}
                                    strokeDasharray="4 2"
                                />
                                <text
                                    x={(x1 + x2) / 2}
                                    y={(y1 + y2) / 2}
                                    fontSize="12"
                                    fontWeight="bold"
                                    textAnchor="middle"
                                    fill={alert.severity === "critical" ? "#c62828" : "#e65100"}
                                >
                                    {alert.density}
                                </text>
                            </g>
                        );
                    })}

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
                    {alignedNodes.map((n) => {
                        const pos = positions[n.id];
                        if (!pos) return null;

                        const isAnchor = n.id === anchorAId || n.id === anchorBId;  
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

                {/* Right Sidebar: Density Settings & Alerts */}
                <div style={{ 
                    flex: 1, 
                    minWidth: "300px",
                    background: "#fff"
                }}>
                    
                    {/* Density Settings */}
                    <div style={{
                        background: "#fff",
                        padding: "1rem",
                        borderRadius: 8,
                        marginBottom: "1rem",
                        border: "2px solid #2196F3",
                        colorScheme: "light"
                    }}>

                        {/* Anchor Alignment Settings */}
<div
  style={{
    background: "#fff",
    padding: "1rem",
    borderRadius: 8,
    marginBottom: "1rem",
    border: "2px solid #4CAF50",
    colorScheme: "light",
  }}
>
  <h3
    style={{
      margin: "0 0 0.75rem 0",
      fontSize: "1.1rem",
      color: "#1B5E20",
    }}
  >
    Anchor Alignment
  </h3>

  {/* Anchor IDs */}
  <div
    style={{
      display: "grid",
      gridTemplateColumns: "1fr 1fr",
      gap: "0.75rem",
      marginBottom: "0.75rem",
    }}
  >
    <div>
      <label
        style={{
          display: "block",
          fontSize: "0.85rem",
          marginBottom: "0.25rem",
          color: "#000",
          fontWeight: 600,
        }}
      >
        Anchor A ID
      </label>
      <input
        type="text"
        value={anchorAId}
        onChange={(e) => setAnchorAId(e.target.value.trim())}
        style={{
          width: "100%",
          padding: "0.3rem 0.4rem",
          borderRadius: 4,
          border: "1px solid #ccc",
          fontSize: "0.85rem",
        }}
      />
    </div>
    <div>
      <label
        style={{
          display: "block",
          fontSize: "0.85rem",
          marginBottom: "0.25rem",
          color: "#000",
          fontWeight: 600,
        }}
      >
        Anchor B ID
      </label>
      <input
        type="text"
        value={anchorBId}
        onChange={(e) => setAnchorBId(e.target.value.trim())}
        style={{
          width: "100%",
          padding: "0.3rem 0.4rem",
          borderRadius: 4,
          border: "1px solid #ccc",
          fontSize: "0.85rem",
        }}
      />
    </div>
  </div>

  {/* Anchor target positions */}
  <div
    style={{
      display: "grid",
      gridTemplateColumns: "1fr 1fr",
      gap: "0.75rem",
    }}
  >
    <div>
      <div
        style={{
          fontSize: "0.85rem",
          fontWeight: 600,
          marginBottom: "0.25rem",
          color: "#000",
        }}
      >
        Anchor A (WB3) target
      </div>
      <div
        style={{
          display: "grid",
          gridTemplateColumns: "1fr 1fr",
          gap: "0.25rem",
        }}
      >
        <input
          type="number"
          value={anchorAX}
          onChange={(e) =>
            setAnchorAX(Number.isNaN(parseFloat(e.target.value)) ? 0 : parseFloat(e.target.value))
          }
          placeholder="x"
          step="0.5"
          style={{
            padding: "0.3rem 0.4rem",
            borderRadius: 4,
            border: "1px solid #ccc",
            fontSize: "0.85rem",
          }}
        />
        <input
          type="number"
          value={anchorAY}
          onChange={(e) =>
            setAnchorAY(Number.isNaN(parseFloat(e.target.value)) ? 0 : parseFloat(e.target.value))
          }
          placeholder="y"
          step="0.5"
          style={{
            padding: "0.3rem 0.4rem",
            borderRadius: 4,
            border: "1px solid #ccc",
            fontSize: "0.85rem",
          }}
        />
      </div>
    </div>

    <div>
      <div
        style={{
          fontSize: "0.85rem",
          fontWeight: 600,
          marginBottom: "0.25rem",
          color: "#000",
        }}
      >
        Anchor B (WB4) target
      </div>
      <div
        style={{
          display: "grid",
          gridTemplateColumns: "1fr 1fr",
          gap: "0.25rem",
        }}
      >
        <input
          type="number"
          value={anchorBX}
          onChange={(e) =>
            setAnchorBX(Number.isNaN(parseFloat(e.target.value)) ? 0 : parseFloat(e.target.value))
          }
          placeholder="x"
          step="0.5"
          style={{
            padding: "0.3rem 0.4rem",
            borderRadius: 4,
            border: "1px solid #ccc",
            fontSize: "0.85rem",
          }}
        />
        <input
          type="number"
          value={anchorBY}
          onChange={(e) =>
            setAnchorBY(Number.isNaN(parseFloat(e.target.value)) ? 0 : parseFloat(e.target.value))
          }
          placeholder="y"
          step="0.5"
          style={{
            padding: "0.3rem 0.4rem",
            borderRadius: 4,
            border: "1px solid #ccc",
            fontSize: "0.85rem",
          }}
        />
      </div>
    </div>
  </div>

  <p
    style={{
      marginTop: "0.6rem",
      fontSize: "0.8rem",
      color: "#555",
    }}
  >
    These coordinates are in your real-world grid (meters). The layout aligns the live graph so
    that Anchor A and B land at these target positions.
  </p>
</div>

                        <h3 style={{ margin: "0 0 0.75rem 0", fontSize: "1.1rem", color: "#000" }}>
                            Crowd Density Settings
                        </h3>

                                            {/* Lost Nodes */}
                    <div style={{
                        background: "#fff",
                        padding: "1rem",
                        borderRadius: 8,
                        marginBottom: "1rem",
                        border: "2px solid #9C27B0",
                        colorScheme: "light"
                    }}>
                        <h3 style={{ margin: "0 0 0.75rem 0", fontSize: "1.1rem", color: "#4A148C" }}>
                            Lost Wristbands
                        </h3>

                        {lostNodes.length === 0 ? (
                            <p style={{ fontSize: "0.9rem", color: "#555", margin: 0 }}>
                                All tracked wristbands are currently visible.
                            </p>
                        ) : (
                            <ul style={{ 
                                listStyle: "none", 
                                padding: 0, 
                                margin: 0, 
                                fontSize: "0.9rem",
                                maxHeight: "160px",
                                overflowY: "auto"
                            }}>
                                {lostNodes.map((n) => (
                                    <li 
                                        key={n.id}
                                        style={{
                                            padding: "0.4rem 0.3rem",
                                            borderBottom: "1px solid #eee",
                                            display: "flex",
                                            justifyContent: "space-between",
                                            alignItems: "center"
                                        }}
                                    >
                                        <span style={{ fontWeight: 600, color: "#6A1B9A" }}>
                                            {n.id}
                                        </span>
                                        <span style={{ color: "#555" }}>
                                            {n.secondsAgo}s ago
                                        </span>
                                    </li>
                                ))}
                            </ul>
                        )}
                    </div>
                        
                        <div>
                            <label style={{ display: "block", fontSize: "0.9rem", marginBottom: "0.5rem", color: "#000", fontWeight: 600 }}>
                                Grid Size: {gridSize.toFixed(1)}m
                            </label>
                            <input
                                type="range"
                                min="0.5"
                                max="10"
                                step="0.5"
                                value={gridSize}
                                onChange={(e) => setGridSize(parseFloat(e.target.value))}
                                style={{ width: "100%", height: "6px" }}
                            />
                        </div>
                        
                        <div style={{ marginTop: "1rem" }}>
                            <label style={{ display: "block", fontSize: "0.9rem", marginBottom: "0.5rem", color: "#000", fontWeight: 600 }}>
                                Alert Threshold: {densityThreshold} nodes/cell
                            </label>
                            <input
                                type="range"
                                min="2"
                                max="20"
                                step="1"
                                value={densityThreshold}
                                onChange={(e) => setDensityThreshold(parseInt(e.target.value))}
                                style={{ width: "100%", height: "6px" }}
                            />
                        </div>

                        <div style={{ marginTop: "1rem" }}>
                            <label style={{ fontSize: "0.9rem", display: "flex", alignItems: "center", gap: "0.5rem", color: "#000", fontWeight: 600, cursor: "pointer" }}>
                                <input
                                    type="checkbox"
                                    checked={showAlerts}
                                    onChange={(e) => setShowAlerts(e.target.checked)}
                                    style={{ width: "18px", height: "18px", cursor: "pointer" }}
                                />
                                Show density alerts on map
                            </label>
                        </div>
                    </div>

                    {/* Alert Summary */}
                    {alerts.length > 0 && (
                        <div style={{
                            background: alerts.some(a => a.severity === "critical") ? "#ffebee" : "#fff3e0",
                            padding: "1rem",
                            borderRadius: 8,
                            border: `2px solid ${alerts.some(a => a.severity === "critical") ? "#f44336" : "#ff9800"}`
                        }}>
                            <h3 style={{ 
                                margin: "0 0 0.5rem 0", 
                                fontSize: "1rem",
                                color: alerts.some(a => a.severity === "critical") ? "#c62828" : "#e65100"
                            }}>
                                ⚠️ {alerts.length} High Density Area{alerts.length > 1 ? "s" : ""} Detected
                            </h3>
                            
                            <div style={{ fontSize: "0.85rem" }}>
                                {alerts.map((alert, idx) => (
                                    <div key={idx} style={{ 
                                        marginBottom: "0.5rem",
                                        padding: "0.5rem",
                                        background: "rgba(255,255,255,0.5)",
                                        borderRadius: 4
                                    }}>
                                        <strong>
                                            {alert.severity === "critical" ? "🔴 CRITICAL" : "🟠 WARNING"}
                                        </strong>
                                        {" - "}
                                        <strong>{alert.density}</strong> nodes 
                                        {" at "}
                                        ({alert.centerX.toFixed(1)}m, {alert.centerY.toFixed(1)}m)
                                        <div style={{ fontSize: "0.75rem", color: "#666", marginTop: "0.25rem" }}>
                                            Nodes: {alert.nodes.map(n => n.id).join(", ")}
                                        </div>
                                    </div>
                                ))}
                            </div>
                        </div>
                    )}

                    {/* Stats */}
                    <div style={{ 
                        marginTop: "1rem", 
                        fontSize: "0.9rem", 
                        color: "#000",
                        background: "#f0f0f0",
                        padding: "0.75rem",
                        borderRadius: 6,
                        border: "1px solid #ddd",
                        colorScheme: "light"
                    }}>
                        <div><strong>Total Nodes:</strong> {data.nodes.length}</div>
                        <div><strong>Edges:</strong> {data.edges.length}</div>
                        <div><strong>Active Alerts:</strong> {alerts.length}</div>
                    </div>
                </div>

            </div>
        </div>
    );
}

export default App;