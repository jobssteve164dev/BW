from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_embedded_crypto_dependencies_are_available():
    required_headers = (
        ROOT / "lib/secp256k1/src/secp256k1.h",
        ROOT / "lib/secp256k1/secp256k1/include/secp256k1.h",
        ROOT / "lib/libwally-embedded/src/wally_core.h",
        ROOT / "lib/libwally-embedded/libwally-core/include/wally_core.h",
    )

    missing = [str(path.relative_to(ROOT)) for path in required_headers if not path.is_file()]
    assert not missing, f"缺少嵌入式密码学依赖：{', '.join(missing)}"
