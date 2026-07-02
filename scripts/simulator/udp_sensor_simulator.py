#!/usr/bin/env python3

"""Send simple radar-style measurements over UDP as CSV text."""

from __future__ import annotations

import argparse
import itertools
import socket
import time
from typing import Iterator


def parse_args() -> argparse.Namespace:
    """Parse command-line options for the UDP simulator."""
    parser = argparse.ArgumentParser(
        description="Send radar-style CSV measurements over UDP."
    )
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="Destination host. Default: 127.0.0.1",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=5005,
        help="Destination UDP port. Default: 5005",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=1.0,
        help="Seconds between packets. Default: 1.0",
    )
    parser.add_argument(
        "--mode",
        choices=("static", "moving"),
        default="static",
        help="Measurement mode. Default: static",
    )
    parser.add_argument(
        "--azimuth-step",
        type=float,
        default=5.0,
        help="Azimuth step per packet in moving mode. Default: 5.0",
    )
    return parser.parse_args()


def static_measurement_stream() -> itertools.cycle[tuple[float, float, float, float]]:
    """Yield a repeating set of example radar-like measurements."""
    measurements = (
        (1200.0, 45.0, 3.0, 0.82),
        (950.0, 70.0, 1.5, 0.64),
        (1500.0, 120.0, 2.0, 0.73),
        (600.0, 315.0, 0.5, 0.91),
        (820.0, 15.0, 1.0, 0.58),
    )
    return itertools.cycle(measurements)


def moving_measurement_stream(
    azimuth_step: float,
) -> Iterator[tuple[float, float, float, float]]:
    """Yield one moving target whose azimuth changes each packet."""
    range_m = 900.0
    azimuth_deg = 0.0
    elevation_deg = 2.0
    intensity = 0.85

    while True:
        yield (range_m, azimuth_deg, elevation_deg, intensity)
        azimuth_deg = (azimuth_deg + azimuth_step) % 360.0


def format_measurement(
    measurement: tuple[float, float, float, float]
) -> str:
    """Format one measurement as CSV text for a single UDP packet."""
    range_m, azimuth_deg, elevation_deg, intensity = measurement
    return (
        f"{range_m:.1f},"
        f"{azimuth_deg:.1f},"
        f"{elevation_deg:.1f},"
        f"{intensity:.2f}"
    )


def main() -> int:
    args = parse_args()

    # UDP is connectionless, so we simply send one CSV measurement per packet.
    destination = (args.host, args.port)
    if args.mode == "moving":
        measurements = moving_measurement_stream(args.azimuth_step)
    else:
        measurements = static_measurement_stream()

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
        print(
            "Sending UDP radar measurements to "
            f"{args.host}:{args.port} every {args.interval:.2f} s "
            f"in {args.mode} mode"
        )

        try:
            for measurement in measurements:
                payload = format_measurement(measurement)
                udp_socket.sendto(payload.encode("utf-8"), destination)
                print(payload)
                time.sleep(args.interval)
        except KeyboardInterrupt:
            print("\nStopped.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
