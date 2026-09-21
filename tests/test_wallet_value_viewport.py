import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_wallet_value_viewport_native_contract(tmp_path: Path) -> None:
    binary = tmp_path / "wallet-value-viewport-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "tests/native/test_wallet_value_viewport.cpp",
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
    assert run_result.stdout.strip() == "wallet value viewport tests passed"


def test_value_selection_interaction_contract(tmp_path: Path) -> None:
    binary = tmp_path / "value-selection-interaction-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Itests/native/fakes",
            "-Isrc",
            "tests/native/test_value_selection_interaction.cpp",
            "src/Selections/ValueSelection.cpp",
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
    assert run_result.stdout.strip() == "value selection interaction tests passed"


def test_wallet_controller_value_entry_contract(tmp_path: Path) -> None:
    binary = tmp_path / "wallet-controller-value-entry-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Itests/native/controller_fakes",
            "-Isrc",
            "tests/native/test_wallet_controller_value_entry.cpp",
            "src/Controllers/WalletController.cpp",
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
    assert run_result.stdout.strip() == "wallet controller value entry tests passed"
