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


def test_ending_signing_resets_secrets_flow_and_file_mode_together() -> None:
    global_manager = (ROOT / "src/Managers/GlobalManager.cpp").read_text()
    start = global_manager.index("void GlobalManager::endTransactionSigning()")
    end = global_manager.index("\n}\n", start)
    cleanup = global_manager[start:end]

    assert "clearLoadedWalletSecrets" in cleanup
    assert "getTransactionSigningFlow().cancel()" in cleanup
    assert "setTransactionOngoing(false)" in cleanup
    assert "setCurrentSelectedFileType(FileTypeEnum::WALLET)" in cleanup

    terminal_paths = (
        ROOT / "src/Controllers/FileBrowserController.cpp",
        ROOT / "src/Controllers/SeedController.cpp",
        ROOT / "src/Managers/SeedManager.cpp",
        ROOT / "src/Managers/FileBrowserManager.cpp",
    )
    for path in terminal_paths:
        assert "endTransactionSigning()" in path.read_text(), path

    browser = (ROOT / "src/Controllers/FileBrowserController.cpp").read_text()
    sd_failure = browser.index("if (!manager.sdService.getSdState())")
    sd_failure_return = browser.index("return;", sd_failure)
    assert 'currentPath = "/";' in browser[sd_failure:sd_failure_return]


def test_transaction_browser_is_scoped_to_the_selected_wallet() -> None:
    browser = (ROOT / "src/Controllers/FileBrowserController.cpp").read_text()
    assert "WalletFileScope::directory(" in browser
    assert 'listElements(currentPath, 0, "psbt")' in browser
    assert "WalletFileScope::contains(transactionRoot, currentPath)" in browser
    assert "currentPath = transactionRoot" in browser
    assert "已选 PSBT 不再可用" in browser
    resume_check = browser.index("已选 PSBT 不再可用")
    assert "manager.sdService.isFile(currentPath)" in browser[:resume_check]

    selection = (ROOT / "src/Selections/FilePathSelection.cpp").read_text()
    assert "if (filteredNames.empty())" in selection

    manager = (ROOT / "src/Managers/FileBrowserManager.cpp").read_text()
    assert "std::tolower(value)" in manager
