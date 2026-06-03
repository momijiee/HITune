from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import torch
import torchaudio


DEFAULT_MODEL_PATH = Path("/data2/guquansheng/MOSS-TTS-v1.5")
DEFAULT_CODEC_PATH = "/home/guquansheng/.cache/modelscope/hub/models/openmoss/MOSS-Audio-Tokenizer"


@dataclass(frozen=True)
class MossTTSConfig:
    model_path: Path = DEFAULT_MODEL_PATH
    codec_path: Optional[str] = DEFAULT_CODEC_PATH
    device: str = "auto"
    dtype: str = "auto"
    max_new_tokens: int = 128
    bitrate: str = "128k"


class MossTTSEngine:
    def __init__(self, config: MossTTSConfig) -> None:
        self.config = config
        self.device = self._resolve_device(config.device)
        self.dtype = self._resolve_dtype(config.dtype, self.device)
        self._processor = None
        self._model = None

    @staticmethod
    def _resolve_device(device: str) -> str:
        if device != "auto":
            return device
        return "cuda" if torch.cuda.is_available() else "cpu"

    @staticmethod
    def _resolve_dtype(dtype: str, device: str) -> torch.dtype:
        if dtype == "auto":
            return torch.bfloat16 if device.startswith("cuda") else torch.float32
        mapping = {
            "float16": torch.float16,
            "bfloat16": torch.bfloat16,
            "float32": torch.float32,
        }
        try:
            return mapping[dtype]
        except KeyError as exc:
            raise ValueError(f"unsupported dtype: {dtype}") from exc

    @staticmethod
    def _attn_implementation(device: str) -> str:
        # Do not select flash_attention_2: the project explicitly avoids flash-attn.
        return "sdpa" if device.startswith("cuda") else "eager"

    def load(self) -> None:
        if self._processor is not None and self._model is not None:
            return

        if not self.config.model_path.exists():
            raise FileNotFoundError(f"MOSS-TTS model path not found: {self.config.model_path}")

        try:
            from modelscope import AutoModel, AutoProcessor
        except ImportError:
            from transformers import AutoModel, AutoProcessor

        torch.backends.cuda.enable_cudnn_sdp(False)
        torch.backends.cuda.enable_flash_sdp(True)
        torch.backends.cuda.enable_mem_efficient_sdp(True)
        torch.backends.cuda.enable_math_sdp(True)

        processor_kwargs = {"trust_remote_code": True}
        if self.config.codec_path:
            processor_kwargs["codec_path"] = self.config.codec_path

        processor = AutoProcessor.from_pretrained(
            self.config.model_path,
            **processor_kwargs,
        )
        processor.audio_tokenizer = processor.audio_tokenizer.to(self.device)

        model = AutoModel.from_pretrained(
            self.config.model_path,
            trust_remote_code=True,
            attn_implementation=self._attn_implementation(self.device),
            torch_dtype=self.dtype,
        ).to(self.device)
        model.eval()

        self._processor = processor
        self._model = model

    def synthesize_mp3(
        self,
        text: str,
        output_path: Path,
        *,
        instruction: Optional[str] = None,
        language: str = "Chinese",
        reference: Optional[str] = None,
        tokens: Optional[int] = None,
        max_new_tokens: Optional[int] = None,
    ) -> Path:
        self.load()
        assert self._processor is not None
        assert self._model is not None

        output_path.parent.mkdir(parents=True, exist_ok=True)
        reference_list = [reference] if reference else None
        message = self._processor.build_user_message(
            text=text,
            reference=reference_list,
            instruction=instruction,
            language=language,
            tokens=tokens,
        )
        batch = self._processor([[message]], mode="generation")
        input_ids = batch["input_ids"].to(self.device)
        attention_mask = batch["attention_mask"].to(self.device)

        with torch.no_grad():
            outputs = self._model.generate(
                input_ids=input_ids,
                attention_mask=attention_mask,
                max_new_tokens=max_new_tokens or self.config.max_new_tokens,
            )

        decoded = list(self._processor.decode(outputs))
        if not decoded or not decoded[0].audio_codes_list:
            raise RuntimeError("MOSS-TTS did not return audio.")

        audio = decoded[0].audio_codes_list[0].detach().cpu().unsqueeze(0)
        sampling_rate = int(self._processor.model_config.sampling_rate)

        with tempfile.TemporaryDirectory(prefix="hitune-tts-") as tmpdir:
            wav_path = Path(tmpdir) / "speech.wav"
            torchaudio.save(wav_path, audio, sampling_rate)
            self._convert_wav_to_mp3(wav_path, output_path)

        return output_path

    def _convert_wav_to_mp3(self, wav_path: Path, output_path: Path) -> None:
        ffmpeg = shutil.which("ffmpeg")
        if not ffmpeg:
            raise RuntimeError("ffmpeg is required to write mp3 files, but it was not found in PATH.")

        command = [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            os.fspath(wav_path),
            "-codec:a",
            "libmp3lame",
            "-b:a",
            self.config.bitrate,
            os.fspath(output_path),
        ]
        subprocess.run(command, check=True)
