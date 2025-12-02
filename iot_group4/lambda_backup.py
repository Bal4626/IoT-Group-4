import json
import math
import time
import boto3
from decimal import Decimal

dynamodb = boto3.resource("dynamodb")
history_table = dynamodb.Table("StateHistory")


def to_json_compatible(obj):
    """Convert DynamoDB Decimals into floats so json.dumps doesn't crash."""
    if isinstance(obj, list):
        return [to_json_compatible(x) for x in obj]
    if isinstance(obj, dict):
        return {k: to_json_compatible(v) for k, v in obj.items()}
    if isinstance(obj, Decimal):
        return float(obj)
    return obj


def fetch_recent_edges(window_seconds=300):
    """
    Read recent StateHistory items and aggregate average RSSI for each (anchor, wristband) pair.
    Returns:
      nodes: set of IDs
      edges: list of {a, b, rssi}
    """
    now_ts = int(time.time())
    cutoff = now_ts - window_seconds

    nodes = set()
    pair_stats = {}  # (a,b) -> {sum, count}

    resp = history_table.scan()
    items = resp.get("Items", [])

    for raw in items:
        item = to_json_compatible(raw)
        ts = item.get("timestamp")
        if ts is None or ts < cutoff:
            continue

        wristband_id = item.get("wristbandId")
        anchor_id = item.get("anchorId")
        rssi = item.get("rssi")

        if not wristband_id or not anchor_id or rssi is None:
            continue

        a, b = str(anchor_id), str(wristband_id)
        nodes.add(a)
        nodes.add(b)

        key = tuple(sorted([a, b]))
        if key not in pair_stats:
            pair_stats[key] = {"sum": 0.0, "count": 0}
        pair_stats[key]["sum"] += float(rssi)
        pair_stats[key]["count"] += 1

    edges = []
    for (a, b), stat in pair_stats.items():
        avg_rssi = stat["sum"] / max(stat["count"], 1)
        edges.append({"a": a, "b": b, "rssi": avg_rssi})

    return nodes, edges


def rssi_to_distance(rssi, rssi0=-50.0, n=2.5, d0=1.0):
    """
    Convert RSSI to an approximate distance using a simple log-distance path loss model.
        d = d0 * 10 ** ((rssi0 - rssi) / (10 * n))
    Just for relative scale, not physical accuracy.
    """
    try:
        return d0 * (10 ** ((rssi0 - rssi) / (10.0 * n)))
    except Exception:
        return d0


def spring_layout(nodes, edges, root_id="WB1", iterations=200, learning_rate=0.01):
    """
    Spring layout with fixed anchors:
      - WB1 pinned at (0,0)
      - WB2 pinned at (10,0) to lock orientation
    """
    if not nodes:
        return {}

    node_list = list(nodes)
    positions = {}
    R_init = 5.0

    # Initial positions on a circle
    for idx, nid in enumerate(node_list):
        angle = 2 * math.pi * idx / len(node_list)
        positions[nid] = [R_init * math.cos(angle), R_init * math.sin(angle)]

    # Fixed anchors to prevent drift/rotation
    fixed_positions = {
    "WB3": (0.0, 0.0), 
    "WB4": (1.0, 1.0),  
    }
    

    # Override initial positions for fixed nodes if they exist
    for n, (fx, fy) in fixed_positions.items():
        if n in positions:
            positions[n] = [fx, fy]

    # Precompute target distances
    edge_list = []
    for e in edges:
        a, b, rssi = e["a"], e["b"], e["rssi"]
        d_target = rssi_to_distance(rssi)
        edge_list.append((a, b, d_target))

    if not edge_list:
        return {nid: {"x": positions[nid][0], "y": positions[nid][1]} for nid in nodes}

    # Iterative relaxation
    for _ in range(iterations):
        for (a, b, d_target) in edge_list:
            if a not in positions or b not in positions:
                continue

            ax, ay = positions[a]
            bx, by = positions[b]
            dx = bx - ax
            dy = by - ay
            dist = math.sqrt(dx * dx + dy * dy) or 1e-6

            err = dist - d_target
            ux = dx / dist
            uy = dy / dist

            # move a,b unless they are fixed anchors
            if a not in fixed_positions:
                positions[a][0] += learning_rate * err * ux
                positions[a][1] += learning_rate * err * uy
            if b not in fixed_positions:
                positions[b][0] -= learning_rate * err * ux
                positions[b][1] -= learning_rate * err * uy

    # Force exact fixed positions again at end (just in case)
    for n, (fx, fy) in fixed_positions.items():
        if n in positions:
            positions[n] = [fx, fy]

    return {nid: {"x": positions[nid][0], "y": positions[nid][1]} for nid in nodes}


def lambda_handler(event, context):
    print("EVENT:", json.dumps(event))

    # 1. Build graph from recent StateHistory
    nodes, edges = fetch_recent_edges(window_seconds=300)  # last 5 minutes

    # 2. Layout with WB1/WB2 pinned
    positions = spring_layout(nodes, edges, root_id="WB1", iterations=200, learning_rate=0.01)

    # 3. Build response structure
    node_list = []
    for n in nodes:
        pos = positions.get(n, {"x": 0.0, "y": 0.0})
        node_list.append(
            {
                "id": n,
                "x": pos["x"],
                "y": pos["y"],
            }
        )

    edge_list = [
        {"source": e["a"], "target": e["b"], "rssi": e["rssi"]}
        for e in edges
    ]

    body = {
        "nodes": node_list,
        "edges": edge_list,
    }

    return {
        "statusCode": 200,
        "headers": {
            "Content-Type": "application/json",
            "Access-Control-Allow-Origin": "*",
        },
        "body": json.dumps(body),
    }
