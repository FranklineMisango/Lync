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
    """Fallback hash function for existing files."""
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download_and_hash(url: str, output_path: pathlib.Path) -> str:
    """Downloads the file and calculates its SHA256 hash simultaneously."""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256()
    
    # Fake a real browser header so Nasdaq's CDN doesn't throttle/drop the connection
    req = urllib.request.Request(
        url, 
        headers={'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36'}
    )
    
    # Added a 15-second timeout so the script doesn't hang infinitely
    with urllib.request.urlopen(req, timeout=15) as response, output_path.open("wb") as target:
        total_size = int(response.headers.get('content-length', 0))
        downloaded = 0
        chunk_size = 1024 * 1024  # 1MB chunks
        
        while True:
            chunk = response.read(chunk_size)
            if not chunk:
                break
            target.write(chunk)
            digest.update(chunk)
            downloaded += len(chunk)
            
            # Print a real-time progress counter
            if total_size:
                percent = (downloaded / total_size) * 100
                print(f"\rProgress: {percent:.2f}% ({downloaded / (1024*1024):.1f} MB / {total_size / (1024*1024):.1f} MB)", end="")
            else:
                print(f"\rDownloaded: {downloaded / (1024*1024):.1f} MB", end="")
                
        print("\nDownload complete.")
            
    return digest.hexdigest()


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
    
    try:
        file_hash = download_and_hash(url, output_path)
        print(f"Downloaded {output_path.stat().st_size} bytes")
        print(f"sha256={file_hash}")
        print("Next step: gunzip the file and feed the raw ITCH byte stream into your parser or replay harness.")
    except Exception as e:
        print(f"\n[Error] Connection failed: {e}")
        print("Nasdaq's server may be throttling you or temporarily down.")
        return 1
        
    return 0


if __name__ == "__main__":
    raise SystemExit(main())