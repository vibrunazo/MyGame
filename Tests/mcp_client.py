"""Minimal client for the Unreal MCP server the editor runs (ModelContextProtocol plugin, port 18765).

Unreal's server answers with an SSE stream that has no length and keeps the socket open, so this
reads raw bytes and stops at the first complete JSON-RPC message.

    from mcp_client import tool, call_tool
    tool("pie_player_state")                       # a MyGameTools hook, JSON-decoded
    call_tool("EditorToolset.EditorAppToolset", "StartPIE", {...})
"""
import json
import pathlib
import socket

HOST, PORT = "127.0.0.1", 18765
SESSION_FILE = pathlib.Path(__file__).resolve().parents[1] / "Saved" / "mcp_session.txt"
MYGAME_TOOLS = "mygame_tools.toolset.MyGameTools"


def _post(payload, sid=None):
    data = json.dumps(payload).encode()
    head = ["POST /mcp HTTP/1.1", f"Host: {HOST}:{PORT}", "Content-Type: application/json",
            "Accept: application/json, text/event-stream", f"Content-Length: {len(data)}"]
    if sid:
        head.append(f"Mcp-Session-Id: {sid}")
    s = socket.create_connection((HOST, PORT), timeout=600)
    s.sendall(("\r\n".join(head) + "\r\n\r\n").encode() + data)
    buf, body, new_sid, headers_done = b"", "", None, False
    while True:
        try:
            chunk = s.recv(65536)
        except socket.timeout:
            break
        if not chunk:
            break
        buf += chunk
        if not headers_done and b"\r\n\r\n" in buf:
            raw_head, buf = buf.split(b"\r\n\r\n", 1)
            headers_done = True
            for line in raw_head.decode("latin-1").split("\r\n"):
                if line.lower().startswith("mcp-session-id:"):
                    new_sid = line.split(":", 1)[1].strip()
            if "id" not in payload:  # notifications get no body
                break
        if headers_done:
            for line in buf.decode("utf-8", "replace").splitlines():
                cand = line[5:].strip() if line.startswith("data:") else line.strip()
                if cand.startswith("{"):
                    try:
                        json.loads(cand)
                        body = cand
                        break
                    except ValueError:
                        pass
            if body:
                break
    s.close()
    return (json.loads(body) if body else None), new_sid


def _session(fresh=False):
    if not fresh and SESSION_FILE.exists():
        return SESSION_FILE.read_text().strip()
    _, sid = _post({"jsonrpc": "2.0", "id": 0, "method": "initialize",
                    "params": {"protocolVersion": "2025-06-18", "capabilities": {},
                               "clientInfo": {"name": "mygame-tests", "version": "1"}}})
    _post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
    SESSION_FILE.parent.mkdir(parents=True, exist_ok=True)
    SESSION_FILE.write_text(sid or "")
    return sid


def call_tool(toolset, name, arguments=None):
    """Calls a toolset tool through the tool-search front door and returns its decoded returnValue."""
    payload = {"jsonrpc": "2.0", "id": 1, "method": "tools/call",
               "params": {"name": "call_tool", "arguments": {"toolset_name": toolset, "tool_name": name, "arguments": arguments or {}}}}
    res, _ = _post(payload, _session())
    if not res or "error" in res:  # stale session after an editor restart
        res, _ = _post(payload, _session(fresh=True))
    result = res.get("result", res)
    text = "".join(c.get("text", "") for c in result.get("content", []))
    if result.get("isError"):
        raise RuntimeError(f"{toolset}.{name}: {text[:500]}")
    value = json.loads(text).get("returnValue") if text.startswith("{") else text
    if isinstance(value, str) and value[:1] in "[{":
        value = json.loads(value)
    return value


def tool(name, **arguments):
    """Calls a MyGameTools hook."""
    return call_tool(MYGAME_TOOLS, name, arguments)
