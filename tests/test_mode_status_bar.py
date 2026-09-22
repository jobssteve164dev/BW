import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_main_menu_uses_the_aligned_top_bar_contract(tmp_path: Path) -> None:
    binary = tmp_path / "mode-status-bar-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Itests/native/mode_fakes",
            "-Isrc",
            "tests/native/test_mode_status_bar.cpp",
            "src/Selections/ModeSelection.cpp",
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
    assert run_result.stdout.strip() == "mode status bar test passed"
