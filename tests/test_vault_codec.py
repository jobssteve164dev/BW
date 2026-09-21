import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_vault_codec_native_contract(tmp_path: Path) -> None:
    binary = tmp_path / "vault-codec-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "tests/native/test_vault_codec.cpp",
            "src/Services/VaultCodec.cpp",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert compile_result.returncode == 0, compile_result.stderr

    run_result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert run_result.returncode == 0, run_result.stderr
    assert run_result.stdout.strip() == "vault codec tests passed"


def test_wallet_distinguishes_empty_passphrase_from_unloaded_secrets(tmp_path: Path) -> None:
    binary = tmp_path / "wallet-secret-state-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "tests/native/test_wallet_secret_state.cpp",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert compile_result.returncode == 0, compile_result.stderr

    run_result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert run_result.returncode == 0, run_result.stderr
    assert run_result.stdout.strip() == "wallet secret state tests passed"
