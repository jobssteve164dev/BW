import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_power_management_native_contract(tmp_path: Path) -> None:
    binary = tmp_path / "power-management-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "tests/native/test_power_management.cpp",
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
    assert run_result.stdout.strip() == "power management tests passed"
