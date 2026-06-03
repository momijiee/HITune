"""Voice asset text selected from docs/PLAN.md."""

SUNNY_GENTLE_INSTRUCTION = (
    "阳光、温柔、亲近、自然，像一朵会追光的桌面小花在轻声陪伴；"
    "语速中等偏慢，带一点笑意，不要夸张。"
)

VOICE_ASSET_VARIANTS = {
    "mandarin": {
        "language": "Chinese",
        "assets": [
            {
                "track": 1,
                "filename": "0001.mp3",
                "trigger": "你好",
                "state": "GREETING",
                "text": "你好，有什么可以帮您",
                "tokens": 38,
                "max_new_tokens": 72,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 2,
                "filename": "0002.mp3",
                "trigger": "介绍自己",
                "state": "SELF_INTRO",
                "text": "我是一朵会追光的闲聊花，可以陪你说说话",
                "tokens": 64,
                "max_new_tokens": 88,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 3,
                "filename": "0003.mp3",
                "trigger": "我有点累",
                "state": "TIRED_RESPONSE",
                "text": "那就休息一下吧，喝口水，看看远处",
                "tokens": 56,
                "max_new_tokens": 84,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 4,
                "filename": "0004.mp3",
                "trigger": "今天开心",
                "state": "HAPPY_RESPONSE",
                "text": "太好啦，希望这份开心一直陪着你",
                "tokens": 52,
                "max_new_tokens": 88,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 5,
                "filename": "0005.mp3",
                "trigger": "我要走了",
                "state": "GOODBYE",
                "text": "好的，待会儿见，我会继续面朝阳光",
                "tokens": 60,
                "max_new_tokens": 96,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
        ],
    },
    "cantonese": {
        "language": "Cantonese",
        "assets": [
            {
                "track": 1,
                "filename": "0001.mp3",
                "trigger": "你好",
                "state": "GREETING",
                "text": "你好，有咩可以帮到你",
                "tokens": 40,
                "max_new_tokens": 76,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 2,
                "filename": "0002.mp3",
                "trigger": "介绍自己",
                "state": "SELF_INTRO",
                "text": "我系一朵会追光嘅闲聊花，可以陪你倾下偈",
                "tokens": 68,
                "max_new_tokens": 96,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 3,
                "filename": "0003.mp3",
                "trigger": "我有点累",
                "state": "TIRED_RESPONSE",
                "text": "咁就休息一下啦，饮啖水，望下远处",
                "tokens": 58,
                "max_new_tokens": 88,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 4,
                "filename": "0004.mp3",
                "trigger": "今天开心",
                "state": "HAPPY_RESPONSE",
                "text": "太好啦，希望呢份开心一直陪住你",
                "tokens": 56,
                "max_new_tokens": 92,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
            {
                "track": 5,
                "filename": "0005.mp3",
                "trigger": "我要走了",
                "state": "GOODBYE",
                "text": "好呀，阵间见，我会继续向住阳光",
                "tokens": 58,
                "max_new_tokens": 96,
                "instruction": SUNNY_GENTLE_INSTRUCTION,
            },
        ],
    },
}

VOICE_ASSETS = VOICE_ASSET_VARIANTS["mandarin"]["assets"]
