"""Voice asset text selected from docs/VOICE_ASSETS.md."""

SUNNY_GENTLE_INSTRUCTION = (
    "阳光、温柔、亲近、自然，像一朵会追光的桌面小花在轻声陪伴；"
    "语速中等偏慢，带一点笑意，不要夸张。"
)


def _asset(
    track: int,
    trigger: str,
    state: str,
    text: str,
    tokens: int,
    max_new_tokens: int,
) -> dict:
    return {
        "track": track,
        "filename": f"{track:04d}.mp3",
        "trigger": trigger,
        "state": state,
        "text": text,
        "tokens": tokens,
        "max_new_tokens": max_new_tokens,
        "instruction": SUNNY_GENTLE_INSTRUCTION,
    }


VOICE_ASSET_VARIANTS = {
    "mandarin": {
        "language": "Chinese",
        "assets": [
            _asset(1, "小幻小幻 / 你好", "AWAKE", "我在呢，想让我陪你聊聊，还是帮你做点什么", 62, 88),
            _asset(2, "介绍自己", "CHAT", "我是HITune，一朵会追光的小花，可以陪你说话，也能提醒你休息", 76, 104),
            _asset(3, "我有点累 / 嗨一下", "EMOTION_TIRED", "听起来你有点累了，先放慢一点，喝口水，看看远处", 68, 96),
            _asset(4, "今天开心 / 走两步", "EMOTION_HAPPY", "太好啦，我也跟着亮起来了，希望这份开心多停一会儿", 68, 96),
            _asset(5, "我要走了 / 点头 / 休息语", "GOODBYE", "好的，待会儿见，我会继续面朝阳光等你回来", 62, 92),
            _asset(6, "疲惫情绪后自动触发", "MUSIC", "我给你放一段轻一点的音乐，先把节奏慢下来", 62, 88),
            _asset(7, "开心情绪后自动触发", "MUSIC", "那我放一段轻快的音乐，陪你把好心情延长一点", 64, 92),
            _asset(8, "立正 / 趴下 / 坐下 / 加速 / 减速等动作", "ACTION", "收到，动作指令已经记录，等执行模块接入后我就能动起来", 78, 108),
            _asset(9, "打开灯", "ACTION", "灯已经打开啦", 30, 56),
            _asset(10, "关闭灯", "ACTION", "灯已经关上啦", 30, 56),
            _asset(11, "停止 / 停下", "ACTION", "好的，先停下来", 32, 56),
            _asset(12, "超大音量 / 最大音量", "VOLUME", "音量已经调到最大", 34, 60),
            _asset(13, "减小音量", "VOLUME", "音量调小一点了", 34, 60),
            _asset(14, "中等音量", "VOLUME", "音量已经调到中等", 34, 60),
            _asset(15, "最小音量", "VOLUME", "音量已经调到最小", 34, 60),
            _asset(16, "休眠态直接说动作命令", "SLEEP", "先叫醒我，再让我做动作吧", 42, 68),
            _asset(17, "主动提醒 1", "REMINDER", "坐了好一会儿了，要不要伸个懒腰", 50, 76),
            _asset(18, "主动提醒 2", "REMINDER", "我有点想晒太阳了，你也抬头看看光吧", 54, 80),
            _asset(19, "主动提醒 3", "REMINDER", "休息一下眼睛吧，远处也有很多值得看的东西", 60, 88),
            _asset(20, "主动提醒 4", "REMINDER", "现在可以喝口水，给自己补一点能量", 52, 80),
        ],
    },
    "cantonese": {
        "language": "Cantonese",
        "assets": [
            _asset(1, "小幻小幻 / 你好", "AWAKE", "我喺度呀，想我陪你倾下偈，定系帮你做啲咩", 66, 96),
            _asset(2, "介绍自己", "CHAT", "我系HITune，一朵会追光嘅小花，可以陪你讲嘢，亦都会提你休息", 82, 112),
            _asset(3, "我有点累 / 嗨一下", "EMOTION_TIRED", "听落你好似有啲攰，先慢返少少，饮啖水，望下远处", 72, 104),
            _asset(4, "今天开心 / 走两步", "EMOTION_HAPPY", "太好啦，我都跟住光咗起来，希望呢份开心停耐少少", 72, 104),
            _asset(5, "我要走了 / 点头 / 休息语", "GOODBYE", "好呀，阵间见，我会继续向住阳光等你返嚟", 66, 96),
            _asset(6, "疲惫情绪后自动触发", "MUSIC", "我帮你播一段轻柔啲嘅音乐，先将节奏慢返落嚟", 70, 100),
            _asset(7, "开心情绪后自动触发", "MUSIC", "咁我播一段轻快啲嘅音乐，陪你将好心情留耐啲", 70, 100),
            _asset(8, "立正 / 趴下 / 坐下 / 加速 / 减速等动作", "ACTION", "收到，动作指令已经记低咗，等执行模块接好之后我就识郁啦", 82, 112),
            _asset(9, "打开灯", "ACTION", "灯已经开咗啦", 32, 60),
            _asset(10, "关闭灯", "ACTION", "灯已经关咗啦", 32, 60),
            _asset(11, "停止 / 停下", "ACTION", "好呀，先停低", 32, 60),
            _asset(12, "超大音量 / 最大音量", "VOLUME", "音量已经调到最大", 36, 64),
            _asset(13, "减小音量", "VOLUME", "音量调细少少啦", 36, 64),
            _asset(14, "中等音量", "VOLUME", "音量已经调到中等", 36, 64),
            _asset(15, "最小音量", "VOLUME", "音量已经调到最细", 36, 64),
            _asset(16, "休眠态直接说动作命令", "SLEEP", "先叫醒我，再叫我做动作啦", 46, 72),
            _asset(17, "主动提醒 1", "REMINDER", "坐咗好一阵啦，要唔要伸下懒腰", 54, 82),
            _asset(18, "主动提醒 2", "REMINDER", "我有啲想晒太阳啦，你都抬头望下啲光呀", 60, 88),
            _asset(19, "主动提醒 3", "REMINDER", "俾眼睛休息一下啦，远处都有好多值得睇嘅嘢", 66, 96),
            _asset(20, "主动提醒 4", "REMINDER", "而家可以饮啖水，俾自己补返少少能量", 58, 88),
        ],
    },
}

VOICE_ASSETS = VOICE_ASSET_VARIANTS["mandarin"]["assets"]
