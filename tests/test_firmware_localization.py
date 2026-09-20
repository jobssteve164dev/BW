import os
from pathlib import Path


FIRMWARE = Path(os.environ.get("BW_FIRMWARE", "dist/BW-Cardputer-ADV-full.bin"))


def test_compiled_firmware_contains_chinese_user_interface() -> None:
    firmware = FIRMWARE.read_bytes()

    expected_labels = ["钱包列表", "创建钱包", "恢复助记词", "签名交易"]
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
