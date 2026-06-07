from __future__ import annotations

import argparse
import json
import re
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
    parser.add_argument("--seed", type=int, default=20260607)
    parser.add_argument("--text-temperature", type=float, default=0.8)
    parser.add_argument("--text-top-p", type=float, default=0.9)
    parser.add_argument("--text-top-k", type=int, default=30)
    parser.add_argument("--audio-temperature", type=float, default=0.8)
    parser.add_argument("--audio-top-p", type=float, default=0.7)
    parser.add_argument("--audio-top-k", type=int, default=15)
    parser.add_argument("--audio-repetition-penalty", type=float, default=1.0)

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
    mode.add_argument(
        "--assets-md",
        type=Path,
        help="Markdown asset table containing audio filename, state, and TTS text columns.",
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


def _clean_markdown_cell(value: str) -> str:
    value = value.strip()
    if value.startswith("`") and value.endswith("`") and len(value) >= 2:
        value = value[1:-1]
    return value.strip()


def _estimate_tokens(text: str) -> int:
    content = re.sub(r"[\s，。！？；：、“”‘’……~,.!?;:'\"()（）\\-]+", "", text)
    punctuation_count = len(re.findall(r"[，。！？；：,.!?;:、……~]", text))
    return max(36, min(420, round(len(content) * 3.2 + punctuation_count * 3 + 20)))


def _load_assets_markdown(path: Path) -> tuple[str | None, list[dict]]:
    instruction = None
    assets: list[dict] = []

    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if stripped.startswith("TTS 风格："):
            instruction = stripped.split("：", 1)[1].strip()
            continue
        if not stripped.startswith("|") or "---" in stripped:
            continue

        cells = [_clean_markdown_cell(cell) for cell in stripped.strip("|").split("|")]
        if len(cells) < 3 or cells[0] == "音频文件名":
            continue

        filename, state, text = cells[:3]
        if not filename.endswith(".mp3") or not text:
            continue

        tokens = _estimate_tokens(text)
        assets.append(
            {
                "filename": filename,
                "state": state,
                "text": text,
                "tokens": tokens,
                "max_new_tokens": max(tokens + 40, 80),
            }
        )

    if not assets:
        raise ValueError(f"no MP3 assets found in Markdown table: {path}")
    return instruction, assets


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
        return

    if args.assets_md:
        markdown_instruction, assets = _load_assets_markdown(args.assets_md)
        output_dir = (
            args.assets_md.parent / "00"
            if args.output_dir == Path("voice_assets")
            else args.output_dir
        )
        for item in assets:
            instruction = args.instruction or markdown_instruction
            language = args.language or "Chinese"
            tokens = args.tokens if args.tokens is not None else item["tokens"]
            max_new_tokens = max(args.max_new_tokens, int(item["max_new_tokens"]))
            yield (
                str(output_dir / item["filename"]),
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
        seed=args.seed,
        text_temperature=args.text_temperature,
        text_top_p=args.text_top_p,
        text_top_k=args.text_top_k,
        audio_temperature=args.audio_temperature,
        audio_top_p=args.audio_top_p,
        audio_top_k=args.audio_top_k,
        audio_repetition_penalty=args.audio_repetition_penalty,
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
