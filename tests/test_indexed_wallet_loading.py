import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_indexed_wallet_loading_native_contract(tmp_path: Path) -> None:
    binary = tmp_path / "indexed-wallet-loading-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "tests/native/test_indexed_wallet_loading.cpp",
            "src/Contexts/SelectionContext.cpp",
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
    assert run_result.stdout.strip() == "indexed wallet loading tests passed"
