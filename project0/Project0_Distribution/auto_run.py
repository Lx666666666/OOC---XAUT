# -*- coding: utf-8 -*-
from __future__ import annotations

import os
import platform
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

APP_NAME = "任务0：考勤管理系统"
ROOT = Path(__file__).resolve().parent
BIN_DIR = ROOT / "bin"
DATA_DIR = BIN_DIR / "data"
RUNTIME_ZIP = ROOT / "runtime.zip"
RUNTIME_DIR = ROOT / "runtime"
STATIC_EXE = BIN_DIR / "Project0_static.exe"
FALLBACK_EXE = BIN_DIR / "Project0.exe"
REQUIRED_DLLS = ["libwinpthread-1.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll"]


def pause(message="按回车键退出..."):
    try:
        input(message)
    except EOFError:
        pass


def fail(message, detail=""):
    print("\n[错误] " + message)
    if detail:
        print(detail)
    pause()
    return 1


def info(message):
    print("[信息] " + message)


def ensure_data():
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    outer = ROOT / "data"
    if outer.exists():
        for item in outer.iterdir():
            if item.is_file() and not (DATA_DIR / item.name).exists():
                shutil.copy2(item, DATA_DIR / item.name)


def dll_exists(name):
    return any((base / name).exists() for base in (BIN_DIR, RUNTIME_DIR, ROOT))


def prepare_runtime():
    missing = [name for name in REQUIRED_DLLS if not dll_exists(name)]
    if missing:
        if not RUNTIME_ZIP.exists():
            raise FileNotFoundError("缺少 runtime.zip，无法自动补齐运行库：" + ", ".join(missing))
        RUNTIME_DIR.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(RUNTIME_ZIP, "r") as zf:
            zf.extractall(RUNTIME_DIR)
        info("已从 runtime.zip 解压运行库。")
    for name in REQUIRED_DLLS:
        target = BIN_DIR / name
        if target.exists():
            continue
        for source in (RUNTIME_DIR / name, ROOT / name):
            if source.exists():
                shutil.copy2(source, target)
                break


def add_dll_path():
    paths = [str(BIN_DIR), str(RUNTIME_DIR), str(ROOT)]
    os.environ["PATH"] = os.pathsep.join(paths + [os.environ.get("PATH", "")])
    if hasattr(os, "add_dll_directory"):
        for path in paths:
            if Path(path).exists():
                os.add_dll_directory(path)


def choose_exe():
    if STATIC_EXE.exists():
        return STATIC_EXE
    if FALLBACK_EXE.exists():
        return FALLBACK_EXE
    raise FileNotFoundError("bin 目录下没有可执行程序。")


def main():
    print("=" * 60)
    print(APP_NAME + " - 自动运行检测脚本")
    print("=" * 60)
    if platform.system().lower() != "windows":
        return fail("该程序是 Windows 图形程序，只能在 Windows 系统运行。")
    if not (platform.machine().endswith("64") or sys.maxsize > 2**32):
        return fail("当前系统不是 64 位系统，本运行包按 64 位 MinGW/EasyX 编译。")
    try:
        ensure_data()
        prepare_runtime()
        add_dll_path()
        exe = choose_exe()
    except Exception as exc:
        return fail("运行环境准备失败。", str(exc))
    info(f"启动：{exe.name}")
    try:
        subprocess.Popen([str(exe)], cwd=str(BIN_DIR), close_fds=True)
        return 0
    except OSError as exc:
        return fail("程序启动失败。", str(exc))


if __name__ == "__main__":
    raise SystemExit(main())
