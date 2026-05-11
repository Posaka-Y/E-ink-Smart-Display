"""
debug_cal.py — Calendar + Gemini pipeline test (no hardware needed)

Usage:
    python debug_cal.py

Requires:
    pip install requests
"""

import requests
import json
import sys

# ── Settings (copied from config.h) ──────────────────────────────────────────
GCAL_ENDPOINT  = (
    # 個人Gmailで作り直した後はここを更新する
    # 正しい形式: https://script.google.com/macros/s/<ID>/exec
    "https://script.google.com/macros/s/AKfycby-xXCR7swg-wDrJL8N18dEihJtzldLSwJNsIrXrsxq6SGQ9Qq89tmeC_bwQGRJWVQaSA/exec"
)
SEP = "─" * 60

# ── Step 1: Google Apps Script ────────────────────────────────────────────────
print(f"\n{SEP}")
print("Step 1: Fetching calendar events")
print(SEP)
print(f"URL: {GCAL_ENDPOINT}\n")

try:
    r = requests.get(GCAL_ENDPOINT, allow_redirects=True, timeout=10)
    print(f"HTTP status : {r.status_code}")
    print(f"Final URL   : {r.url}")
    print(f"Body (raw)  : {r.text[:600]}")
except requests.exceptions.RequestException as e:
    print(f"Request failed: {e}")
    sys.exit(1)

if r.status_code != 200:
    print("\n[ERROR] Non-200 response. Possible causes:")
    print("  - Apps Script not deployed as 'Anyone' access")
    print("  - /a/macros/ URL requires Google login — try standard URL:")
    print("    https://script.google.com/macros/s/<SCRIPT_ID>/exec")
    sys.exit(1)

# Check for error object
try:
    data = r.json()
except Exception:
    print("[ERROR] Response is not valid JSON")
    sys.exit(1)

if isinstance(data, dict) and "error" in data:
    print(f"\n[ERROR] Script returned error: {data['error']}")
    sys.exit(1)

if not isinstance(data, list):
    print(f"\n[ERROR] Expected JSON array, got: {type(data).__name__}")
    sys.exit(1)

events = data
print(f"\nFetched {len(events)} event(s):")
for i, ev in enumerate(events):
    print(f"  [{i}] title={ev.get('title','')!r}  time={ev.get('time','')!r}  allDay={ev.get('allDay',False)}")

print(f"\n{SEP}")
print("All steps passed.")
