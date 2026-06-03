# MOSS-TTS 预录语音生成

本项目使用本地 `MOSS-TTS-v1.5` 为闲聊花生成 TF 卡预录 MP3。环境只用 `uv` 管理，不使用 conda 或全局 pip；依赖中也不安装 `flash-attn`。

## 环境

模型权重默认读取：

`/data2/guquansheng/MOSS-TTS-v1.5`

首次同步环境：

```bash
uv sync
```

如果中国大陆网络下载受阻，使用本机 7890 端口代理后再运行 uv：

```bash
export http_proxy=http://127.0.0.1:7890
export https_proxy=http://127.0.0.1:7890
uv sync
```

首次使用前下载 MOSS audio tokenizer：

```bash
uv run python scripts/download_moss_audio_tokenizer.py
```

下载后的默认路径为：

`/home/guquansheng/.cache/modelscope/hub/models/openmoss/MOSS-Audio-Tokenizer`

如果需要指定 Python，可使用 uv 管理的解释器，例如：

```bash
uv python install 3.12
uv sync --python 3.12
```

## 生成第一阶段音频

按 `docs/PLAN.md` 和 `docs/VOICE_ASSETS.md` 生成 `0001.mp3` 到 `0005.mp3`。默认生成普通话版本：

```bash
CUDA_VISIBLE_DEVICES=4 uv run python scripts/generate_tts.py --preset mvp --output-dir voice_assets --device cuda
```

这里 `CUDA_VISIBLE_DEVICES=4` 表示只使用逻辑第 4 张、物理第 5 张 GPU；进程内仍显示为 `cuda:0`。

生成后把 `voice_assets/0001.mp3` 到 `voice_assets/0005.mp3` 复制到 TF 卡根目录。

需要阳光、温柔、统一风格的普通话和粤语两版时，使用：

```bash
CUDA_VISIBLE_DEVICES=4 uv run python scripts/generate_tts.py \
  --preset mvp \
  --variant all \
  --output-dir voice_assets \
  --device cuda
```

输出目录为：

- `voice_assets/mandarin/0001.mp3` 到 `voice_assets/mandarin/0005.mp3`
- `voice_assets/cantonese/0001.mp3` 到 `voice_assets/cantonese/0005.mp3`

粤语版本会显式传入 MOSS-TTS-v1.5 的 `language="Cantonese"` 标签。

## 生成自定义文本

```bash
CUDA_VISIBLE_DEVICES=4 uv run python scripts/generate_tts.py \
  --text "该休息一下啦，看看远处，让眼睛也晒晒太阳" \
  --output voice_assets/0006.mp3 \
  --instruction "温柔、轻快、像桌面陪伴小花一样提醒用户休息。" \
  --device cuda
```

## 可选参数

- `--model-path`：本地 MOSS-TTS-v1.5 路径，默认 `/data2/guquansheng/MOSS-TTS-v1.5`。
- `--codec-path`：MOSS audio tokenizer 路径，默认 `/home/guquansheng/.cache/modelscope/hub/models/openmoss/MOSS-Audio-Tokenizer`。
- `--reference`：参考音频路径，用于克隆指定音色。
- `--device`：`auto`、`cuda` 或 `cpu`。
- `--dtype`：`auto`、`bfloat16`、`float16` 或 `float32`。
- `--variant`：预设语音版本，`mandarin`、`cantonese` 或 `all`。

脚本固定使用 CUDA `sdpa` 或 CPU `eager` attention，不会选择 `flash_attention_2`。
