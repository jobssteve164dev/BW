import subprocess
import re
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]


def compile_and_run(tmp_path: Path, name: str, sources: list[str], expected: str) -> None:
    binary = tmp_path / name
    result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            *sources,
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    assert run.returncode == 0, run.stderr
    assert run.stdout.strip() == expected


def test_untrusted_files_are_read_through_a_hard_limit(tmp_path: Path) -> None:
    compile_and_run(
        tmp_path,
        "bounded-file-reader-test",
        ["tests/native/test_bounded_file_reader.cpp"],
        "bounded file reader tests passed",
    )


def test_rfid_v2_format_distinguishes_authenticated_backups(tmp_path: Path) -> None:
    compile_and_run(
        tmp_path,
        "rfid-backup-format-test",
        [
            "tests/native/test_rfid_backup_format.cpp",
            "src/Services/RfidBackupFormat.cpp",
        ],
        "rfid backup format tests passed",
    )


def test_release_workflow_uses_immutable_actions_and_least_privilege() -> None:
    workflow = yaml.safe_load((ROOT / ".github/workflows/firmware.yml").read_text())
    assert workflow["permissions"] == {"contents": "read"}
    assert workflow["jobs"]["build"].get("permissions", {"contents": "read"}) == {
        "contents": "read"
    }

    release = workflow["jobs"]["release"]
    assert release["permissions"] == {"contents": "write"}
    assert "refs/tags/v" in release["if"]

    for job in workflow["jobs"].values():
        for step in job.get("steps", []):
            action = step.get("uses")
            if action:
                assert re.fullmatch(r"[^@]+@[0-9a-f]{40}", action), action

    install = next(
        step["run"]
        for step in workflow["jobs"]["build"]["steps"]
        if step.get("name") == "安装构建工具"
    )
    assert re.search(r"platformio==[0-9]+\.[0-9]+\.[0-9]+", install)
    assert re.search(r"pytest==[0-9]+\.[0-9]+\.[0-9]+", install)


def test_platformio_dependencies_do_not_follow_moving_version_ranges() -> None:
    configuration = (ROOT / "platformio.ini").read_text()
    assert "#release/" not in configuration
    assert "@^" not in configuration
    assert "@ *" not in configuration
