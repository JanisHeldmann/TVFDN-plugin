import hashlib
import json
import platform
from pathlib import Path
import re
import subprocess
import sys
import zipfile


source, build, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
match = re.search(r"project\s*\(\s*TVFDN\s+VERSION\s+(\d+\.\d+\.\d+)", (source / "CMakeLists.txt").read_text())
if not match:
    raise SystemExit("Cannot determine TVFDN version")
version = match[1]
mac = sys.platform == "darwin"
system = "macOS-universal" if mac else "Windows-x64"
if not mac and sys.platform != "win32":
    raise SystemExit("Unsupported packaging platform")
for kind, extension in ([('AU', 'component'), ('VST3', 'vst3')] if mac else [('VST3', 'vst3')]):
    bundle = build / "TVFDN_artefacts" / "Release" / kind / ("TVFDN 64." + extension)
    if not bundle.exists():
        raise SystemExit(f"Missing plugin: {bundle}")
    archive = output / f"TVFDN-{version}-{system}-{kind}.zip"
    if mac:
        subprocess.run(["codesign", "--verify", "--verbose=2", str(bundle)], check=True)
        subprocess.run(["lipo", str(bundle / "Contents/MacOS/TVFDN 64"), "-verify_arch", "arm64", "x86_64"], check=True)
        subprocess.run(["ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", str(bundle), str(archive)], check=True)
    else:
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as stream:
            for file in sorted(bundle.rglob("*")):
                if file.is_file():
                    stream.write(file, file.relative_to(bundle.parent).as_posix())
    with zipfile.ZipFile(archive) as stream:
        if stream.testzip() is not None:
            raise SystemExit(f"Invalid archive: {archive}")
commit = subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
changes = []
changelog = source / "CHANGELOG.md"
if changelog.exists():
    section = re.search(rf"^## {re.escape(version)}\b.*?\n(.*?)(?=^## |\Z)", changelog.read_text(), re.M | re.S)
    if section:
        changes = [line for line in section[1].splitlines() if line.startswith("- ")]
metadata = {"version": version, "commit": commit, "platform": system, "changes": changes,
            "runtime": platform.machine()}
(output / f"metadata-{system}.json").write_text(json.dumps(metadata, indent=2) + "\n")
for file in sorted(output.glob("*.zip")):
    print(file.name, hashlib.sha256(file.read_bytes()).hexdigest())
