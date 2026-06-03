from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
from pathlib import Path


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Normalize MP3 voice asset loudness.")
    parser.add_argument("paths", nargs="*", type=Path, help="MP3 files or directories to normalize.")
    parser.add_argument("--root", type=Path, default=Path("voice_assets"))
    parser.add_argument("--target-i", type=float, default=-18.0, help="Integrated loudness target in LUFS.")
    parser.add_argument("--target-lra", type=float, default=7.0, help="Loudness range target.")
    parser.add_argument("--target-tp", type=float, default=-1.5, help="True peak target in dBTP.")
    parser.add_argument("--bitrate", default="128k")
    return parser


def _iter_mp3(paths: list[Path], root: Path) -> list[Path]:
    candidates = paths or [root]
    files: list[Path] = []
    for path in candidates:
        if path.is_dir():
            files.extend(sorted(path.rglob("*.mp3")))
        elif path.suffix.lower() == ".mp3":
            files.append(path)
    return sorted(dict.fromkeys(files))


def _normalize_file(
    ffmpeg: str,
    path: Path,
    *,
    target_i: float,
    target_lra: float,
    target_tp: float,
    bitrate: str,
) -> None:
    with tempfile.TemporaryDirectory(prefix="hitune-normalize-") as tmpdir:
        output = Path(tmpdir) / path.name
        command = [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(path),
            "-af",
            f"loudnorm=I={target_i}:LRA={target_lra}:TP={target_tp}:linear=true",
            "-codec:a",
            "libmp3lame",
            "-b:a",
            bitrate,
            str(output),
        ]
        subprocess.run(command, check=True)
        shutil.move(output, path)


def main() -> None:
    args = _build_parser().parse_args()
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        raise RuntimeError("ffmpeg is required but was not found in PATH.")

    files = _iter_mp3(args.paths, args.root)
    if not files:
        raise RuntimeError("no mp3 files found")

    for path in files:
        _normalize_file(
            ffmpeg,
            path,
            target_i=args.target_i,
            target_lra=args.target_lra,
            target_tp=args.target_tp,
            bitrate=args.bitrate,
        )
        print(path)


if __name__ == "__main__":
    main()
