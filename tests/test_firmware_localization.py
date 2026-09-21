import os
import re
from pathlib import Path


FIRMWARE = Path(os.environ.get("BW_FIRMWARE", "dist/BW-Cardputer-ADV-full.bin"))
ROOT = Path(__file__).resolve().parents[1]


def test_compiled_firmware_contains_chinese_user_interface() -> None:
    firmware = FIRMWARE.read_bytes()

    expected_labels = [
        "钱包列表",
        "创建钱包",
        "恢复助记词",
        "签名交易",
        "网络手续费",
        "显示签名二维码",
    ]
    for label in expected_labels:
        assert label.encode("utf-8") in firmware, f"固件缺少中文界面文案：{label}"


def test_compiled_firmware_no_longer_contains_primary_english_menu() -> None:
    firmware = FIRMWARE.read_bytes()

    forbidden_labels = [
        b"PORTFOLIO",
        b"NEW WALLET",
        b"RESTORE SEED",
        b"SIGN TRANSACTIONS",
        b"Type to search",
        b"No results",
    ]
    for label in forbidden_labels:
        assert label not in firmware, f"固件仍包含未汉化菜单：{label.decode()}"


def test_every_display_text_size_is_at_least_the_clear_title_size() -> None:
    header = (ROOT / "src/Views/CardputerView.h").read_text(encoding="utf-8")
    view = (ROOT / "src/Views/CardputerView.cpp").read_text(encoding="utf-8")
    usb = (ROOT / "src/Services/UsbService.cpp").read_text(encoding="utf-8")
    definitions = dict(re.findall(r"^#define\s+(TEXT_[A-Z_]+)\s+([^\s]+)", header, re.MULTILINE))

    def resolve(value: str) -> float:
        seen: set[str] = set()
        while value in definitions:
            assert value not in seen, f"字号宏存在循环引用：{value}"
            seen.add(value)
            value = definitions[value]
        return float(value.rstrip("fF"))

    title_size = resolve("TEXT_BIG")
    title_pixels = 16 * title_size
    source_without_comments = re.sub(r"//.*", "", view)
    sizes = re.findall(r"setTextSize\(([^)]+)\)", source_without_comments)
    too_small = [(value.strip(), resolve(value.strip())) for value in sizes if resolve(value.strip()) < title_size]

    assert title_pixels == 16.0, "“比特币钱包”标题必须保持为清晰可见的 16px 基准"
    assert not too_small, f"存在小于标题字号的界面文字：{too_small}"
    assert "FreeSans9pt7b" not in view, "钱包详情仍切换到小于标题的 9pt 字体"
    point_sizes = [int(size) for size in re.findall(r"Free[A-Za-z]+?(\d+)pt7b", view)]
    assert point_sizes and min(point_sizes) >= 12, f"存在低于约 16px 的点阵字体：{point_sizes}"
    initialize = re.search(r"void CardputerView::initialize\(\) \{([\s\S]*?)\n\}", view)
    assert initialize and "setTextSize(TEXT_BIG)" in initialize.group(1), "显示初始化必须立即应用 16px 最低字号"
    assert "setFont(&fonts::efontCN_16)" in usb and "setTextSize(1.0f)" in usb, "USB 错误界面必须显式使用 16px 字体"


def test_large_menu_text_uses_two_line_rows_instead_of_utf8_byte_offsets() -> None:
    view = (ROOT / "src/Views/CardputerView.cpp").read_text(encoding="utf-8")

    assert "selectionStrings[currentIndex].size() * 15" not in view
    row_height = int(re.search(r"sizeY = hasDescriptions \? (\d+) : 22", view).group(1))
    row_step = int(re.search(r"stepY = hasDescriptions \? (\d+) : 26", view).group(1))
    title_y = int(re.search(r"startText = hasDescriptions \? (\d+) : 41", view).group(1))
    rows = int(re.search(r"rowsPerScreen = hasDescriptions \? (\d+) : 4", view).group(1))
    description_gap = int(re.search(r"descriptionY = startText \+ (\d+) \+ stepY", view).group(1))

    assert rows == 2
    for row in range(rows):
        top = 30 + row_step * row
        bottom = top + row_height
        title_center = title_y + row_step * row
        description_center = title_center + description_gap
        assert top <= title_center - 8
        assert title_center + 8 <= description_center - 8
        assert description_center + 8 <= bottom
    assert 30 + row_step * (rows - 1) + row_height <= 135


def test_wallet_value_preview_stays_above_the_action_buttons() -> None:
    view = (ROOT / "src/Views/CardputerView.cpp").read_text(encoding="utf-8")

    max_width_margin = int(re.search(r"fitTextToWidth\(value, Display->width\(\) - (\d+)\)", view).group(1))
    preview_y = int(re.search(r"setCursor\(previewX, (\d+)\)", view).group(1))
    action_y = int(re.search(r"fillRoundRect\(40, (\d+), 20, 15", view).group(1))
    fit_function = re.search(r"std::string CardputerView::fitTextToWidth[\s\S]*?\n\}", view).group(0)

    assert 240 - max_width_margin == 228
    assert preview_y + 29 < action_y
    assert "textWidth(text.c_str()) <= maxWidth" in fit_function
    assert "textWidth((fitted + \"...\").c_str()) > maxWidth" in fit_function
    assert "fitted.pop_back()" in fit_function
    assert "setTextWrap(false)" in view
    assert "setTextWrap(true)" in view
    assert "auto limit = 110" not in view
