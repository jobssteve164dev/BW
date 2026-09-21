import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_transaction_review_and_bbqr_native_contract(tmp_path: Path) -> None:
    binary = tmp_path / "transaction-signing-flow-test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "tests/native/test_transaction_signing_flow.cpp",
            "src/Services/TransactionReview.cpp",
            "src/Services/BbqrEncoder.cpp",
            "src/Services/SegwitAddressEncoder.cpp",
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
    assert run_result.stdout.strip() == "transaction signing flow tests passed"


def test_signed_psbt_is_verified_before_unsigned_source_is_deleted() -> None:
    source = (ROOT / "src/Managers/FileBrowserManager.cpp").read_text()
    verified_write = source.index("sdService.replaceBinaryFile(")
    source_delete = source.index("sdService.deleteFile(currentPath.c_str())", verified_write)
    assert verified_write < source_delete
    assert "签名文件校验失败，原文件已保留" in source
    assert "签名已保存，原文件未删除" in source


def test_every_input_must_belong_to_and_be_signed_by_the_selected_wallet() -> None:
    source = (ROOT / "src/Services/CryptoService.cpp").read_text()
    assert "if (!ownedByWallet)" in source
    assert "signedInputs != psbt.tx.inputsNumber" in source
    assert "signaturesLen <= signaturesBefore[index]" in source
    assert "originalSignatureIsPreserved" in source
    assert "newSignatureMatchesKeypath" in source

    manager = (ROOT / "src/Managers/FileBrowserManager.cpp").read_text()
    assert "mergeSignedBitcoinTransaction(" in manager
    assert "fileContent, signedTransactionBytes, verifiedSignedTransaction" in manager
