from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Iterable

from .assets import VOICE_ASSET_VARIANTS
from .moss import DEFAULT_CODEC_PATH, DEFAULT_MODEL_PATH, MossTTSConfig, MossTTSEngine


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate HITune MP3 voice assets with local MOSS-TTS-v1.5."
    )
    parser.add_argument("--model-path", type=Path, default=DEFAULT_MODEL_PATH)
    parser.add_argument(
        "--codec-path",
        default=DEFAULT_CODEC_PATH,
        help="MOSS audio tokenizer path. Use '' only if processor config already supplies it.",
    )
    parser.add_argument("--device", default="auto", choices=["auto", "cuda", "cpu"])
    parser.add_argument(
        "--dtype",
        default="auto",
        choices=["auto", "float16", "bfloat16", "float32"],
    )
    parser.add_argument("--max-new-tokens", type=int, default=128)
    parser.add_argument("--bitrate", default="128k")
    parser.add_argument("--language", help="Language tag passed to MOSS-TTS.")
    parser.add_argument("--reference", help="Optional reference audio for voice cloning.")
    parser.add_argument("--tokens", type=int, help="Optional duration token target.")

    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--text", help="Text to synthesize as one MP3.")
    mode.add_argument(
        "--preset",
        choices=["mvp"],
        help="Generate the MVP voice assets from docs/PLAN.md.",
    )
    mode.add_argument(
        "--manifest",
        type=Path,
        help="JSON file containing a list of {filename,text,instruction} objects.",
    )

    parser.add_argument(
        "--output",
        type=Path,
        help="Output MP3 path for --text mode. Defaults to voice_assets/custom.mp3.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("voice_assets"),
        help="Output directory for --preset or --manifest mode.",
    )
    parser.add_argument(
        "--variant",
        default="mandarin",
        choices=["mandarin", "cantonese", "all"],
        help="Voice asset language variant for --preset mode.",
    )
    parser.add_argument(
        "--instruction",
        help="Speaking style instruction. If omitted in --preset mode, each asset uses its planned emotion.",
    )
    return parser


def _load_manifest(path: Path) -> list[dict]:
    with path.open("r", encoding="utf-8") as file:
        data = json.load(file)
    if not isinstance(data, list):
        raise ValueError("manifest must be a JSON list")
    for item in data:
        if not isinstance(item, dict) or "filename" not in item or "text" not in item:
            raise ValueError("each manifest item must contain filename and text")
    return data


def _iter_jobs(
    args: argparse.Namespace,
) -> Iterable[tuple[str, str, str | None, str, int | None, int]]:
    if args.text:
        output = args.output or Path("voice_assets/custom.mp3")
        yield (
            str(output),
            args.text,
            args.instruction,
            args.language or "Chinese",
            args.tokens,
            args.max_new_tokens,
        )
        return

    if args.preset == "mvp":
        variants = (
            VOICE_ASSET_VARIANTS.keys()
            if args.variant == "all"
            else (args.variant,)
        )
        for variant in variants:
            variant_config = VOICE_ASSET_VARIANTS[variant]
            language = args.language or variant_config["language"]
            output_dir = args.output_dir / variant if args.variant == "all" else args.output_dir
            for asset in variant_config["assets"]:
                instruction = args.instruction or asset["instruction"]
                tokens = args.tokens if args.tokens is not None else asset["tokens"]
                max_new_tokens = min(args.max_new_tokens, asset["max_new_tokens"])
                yield (
                    str(output_dir / asset["filename"]),
                    asset["text"],
                    instruction,
                    language,
                    tokens,
                    max_new_tokens,
                )
        return

    if args.manifest:
        for item in _load_manifest(args.manifest):
            instruction = args.instruction or item.get("instruction")
            language = args.language or item.get("language") or "Chinese"
            tokens = args.tokens if args.tokens is not None else item.get("tokens")
            max_new_tokens = int(item.get("max_new_tokens", args.max_new_tokens))
            yield (
                str(args.output_dir / item["filename"]),
                item["text"],
                instruction,
                language,
                tokens,
                max_new_tokens,
            )


def main() -> None:
    parser = _build_parser()
    args = parser.parse_args()
    codec_path = args.codec_path if args.codec_path else None
    config = MossTTSConfig(
        model_path=args.model_path,
        codec_path=codec_path,
        device=args.device,
        dtype=args.dtype,
        max_new_tokens=args.max_new_tokens,
        bitrate=args.bitrate,
    )
    engine = MossTTSEngine(config)

    for output, text, instruction, language, tokens, max_new_tokens in _iter_jobs(args):
        path = engine.synthesize_mp3(
            text,
            Path(output),
            instruction=instruction,
            language=language,
            reference=args.reference,
            tokens=tokens,
            max_new_tokens=max_new_tokens,
        )
        print(path)


if __name__ == "__main__":
    main()
