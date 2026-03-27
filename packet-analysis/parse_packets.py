#!/usr/bin/env python3
"""
parse_packets.py - Parse Frida packet capture logs into structured JSON.

Educational/portfolio example showing the parsing technique used to
convert raw packet captures into a diffable format for protocol validation.

Usage:
    python parse_packets.py capture.log              # file argument
    python parse_packets.py < capture.log             # stdin
    python parse_packets.py capture.log -o output.json
"""

import re
import json
import sys
import argparse

# Matches the header line produced by frida_capture.js
# Example: >>> [12:34:56.789] RECV #42 | ZoneServer | NCID 0x0134 | 109 bytes
HEADER_RE = re.compile(
    r'>>> \[(\d{2}:\d{2}:\d{2}\.\d{3})\] '
    r'RECV #(\d+) \| '
    r'(\w+) \| '
    r'NCID (0x[0-9A-Fa-f]{4}) \| '
    r'(\d+) bytes'
)

# Matches hex dump lines: "  0000: 34 01 2a 00 ..."
HEX_RE = re.compile(r'^\s+[0-9a-f]{4}: ([0-9a-f ]+)')


def parse_capture(lines):
    """Parse capture log lines into a list of packet dicts."""
    packets = []
    current = None

    for line in lines:
        line = line.rstrip('\n')

        header = HEADER_RE.search(line)
        if header:
            if current:
                packets.append(current)
            current = {
                'timestamp': header.group(1),
                'seq':       int(header.group(2)),
                'server':    header.group(3),
                'ncid':      header.group(4),
                'size':      int(header.group(5)),
                'hex':       ''
            }
            continue

        if current:
            hex_match = HEX_RE.match(line)
            if hex_match:
                current['hex'] += hex_match.group(1).strip() + ' '

    if current:
        packets.append(current)

    # Clean trailing spaces from hex
    for pkt in packets:
        pkt['hex'] = pkt['hex'].strip()

    return packets


def main():
    parser = argparse.ArgumentParser(description='Parse Frida packet capture logs')
    parser.add_argument('input', nargs='?', help='Input log file (default: stdin)')
    parser.add_argument('-o', '--output', help='Output JSON file (default: stdout)')
    args = parser.parse_args()

    if args.input:
        with open(args.input, 'r') as f:
            lines = f.readlines()
    else:
        lines = sys.stdin.readlines()

    packets = parse_capture(lines)

    result = json.dumps(packets, indent=2)
    if args.output:
        with open(args.output, 'w') as f:
            f.write(result + '\n')
        print(f"Wrote {len(packets)} packets to {args.output}", file=sys.stderr)
    else:
        print(result)


if __name__ == '__main__':
    main()
