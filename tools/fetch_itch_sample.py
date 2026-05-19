#!/usr/bin/env python3
"""Download a Nasdaq TotalView-ITCH sample file from the public EMI directory.

Examples:
  python3 tools/fetch_itch_sample.py --file 01302019.NASDAQ_ITCH50.gz
  python3 tools/fetch_itch_sample.py --file 01302019.NASDAQ_ITCH50.gz --output-dir data/itch
"""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import urllib.request


DEFAULT_BASE_URL = "https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH"


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download(url: str, output_path: pathlib.Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url) as response, output_path.open("wb") as target:
        while True:
            chunk = response.read(1024 * 1024)
            if not chunk:
                break
            target.write(chunk)


def main() -> int:
    parser = argparse.ArgumentParser(description="Download a public Nasdaq ITCH sample file.")
    parser.add_argument("--file", required=True, help="File name from the EMI directory, for example 01302019.NASDAQ_ITCH50.gz")
    parser.add_argument("--output-dir", default="data/itch", help="Directory where the file will be saved")
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL, help="Base URL for the Nasdaq EMI ITCH directory")
    parser.add_argument("--skip-existing", action="store_true", help="Do not re-download if the file already exists")
    args = parser.parse_args()

    output_path = pathlib.Path(args.output_dir).expanduser().resolve() / args.file
    if output_path.exists() and args.skip_existing:
        print(f"Already exists: {output_path}")
        print(f"sha256={sha256_file(output_path)}")
        return 0

    url = f"{args.base_url.rstrip('/')}/{args.file}"
    print(f"Downloading {url}")
    print(f"Saving to {output_path}")
    download(url, output_path)
    print(f"Downloaded {output_path.stat().st_size} bytes")
    print(f"sha256={sha256_file(output_path)}")
    print("Next step: gunzip the file and feed the raw ITCH byte stream into your parser or replay harness.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())