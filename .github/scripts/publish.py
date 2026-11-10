import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def validate(ref, metadata):
    tag = ref.removeprefix("refs/tags/")
    if not re.fullmatch(r"v\d+\.\d+\.\d+", tag):
        raise ValueError("Publishing requires a stable vMAJOR.MINOR.PATCH tag")
    if {item["platform"] for item in metadata} != {"Windows-x64", "macOS-universal"} or len(metadata) != 2:
        raise ValueError("Both tested Windows and macOS builds are required")
    if any(item["version"] != tag[1:] for item in metadata):
        raise ValueError("Release tag does not match CMake version")
    if len({item["commit"] for item in metadata}) != 1:
        raise ValueError("Builds came from different source commits")
    return tag


def main():
    directory = Path(sys.argv[1])
    metadata = [json.loads(path.read_text()) for path in sorted(directory.glob("metadata-*.json"))]
    tag = validate(os.environ["BUILD_REF"], metadata)
    repository = os.environ["GITHUB_REPOSITORY"]

    def gh(*args):
        return subprocess.check_output(["gh", *args], text=True).strip()

    obj = json.loads(gh("api", f"repos/{repository}/git/ref/tags/{tag}"))["object"]
    while obj["type"] == "tag":
        obj = json.loads(gh("api", f"repos/{repository}/git/tags/{obj['sha']}"))["object"]
    if obj["type"] != "commit" or obj["sha"] != metadata[0]["commit"]:
        raise ValueError("Artifacts do not match the published source tag")
    version = tag[1:]
    files = sorted(directory.glob("*.zip"))
    expected = {f"TVFDN-{version}-macOS-universal-AU.zip", f"TVFDN-{version}-macOS-universal-VST3.zip",
                f"TVFDN-{version}-Windows-x64-VST3.zip"}
    if {path.name for path in files} != expected:
        raise ValueError("Expected exactly three plugin archives")
    checksum = directory / "SHA256SUMS.txt"
    checksum.write_text("".join(hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.name + "\n" for path in files))
    notes = directory / "release-notes.md"
    notes.write_text(f"TVFDN {version}\n\n" + "\n".join(metadata[0]["changes"]) + f"""

## Downloads

- macOS 11+: universal Apple Silicon/Intel AU and VST3 archives.
- Windows x64: VST3 archive; extract `TVFDN 64.vst3` into `C:\\Program Files\\Common Files\\VST3\\`.
- Checksums: `SHA256SUMS.txt`. Source archives and [version history](https://github.com/{repository}/blob/{tag}/CHANGELOG.md) are available with this release.

On macOS, extract the AU into `~/Library/Audio/Plug-Ins/Components/` and VST3 into `~/Library/Audio/Plug-Ins/VST3/`. The bundles are ad-hoc signed, not Developer ID signed or notarized; macOS may require approval to load downloaded files. Restart/rescan the host, select **TVFDN 64**, and configure 64 track channels with explicit output routing/downmixing.

## Automated validation

Both platforms passed offline VST3 tests: 64-in/64-out layout, stereo rejection, finite nonzero channel-64 impulse response at 48 kHz, and all eight parameters restored in a fresh instance. The macOS AU passed its state round trip, strict Apple validation, and channel-64 impulse tests at 44.1/48/96 kHz. macOS runtime tests ran on {next(item['runtime'] for item in metadata if item['platform'] == 'macOS-universal')}; the universal bundle also contains Intel code.

These checks do not replace Windows DAW/playback testing or physical routing validation. Legacy VST2 binaries are unchanged and must remain installed for existing VST2 projects. Parameter state does not preserve the ringing tail. Use a fresh instance when changing sample rate; the existing first-call-only DSP preparation remains.

Source commit: `{metadata[0]['commit']}`. [Build run](https://github.com/{repository}/actions/runs/{os.environ['GITHUB_RUN_ID']}).
""")
    exists = subprocess.run(["gh", "release", "view", tag, "--repo", repository], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    if not exists:
        gh("release", "create", tag, "--repo", repository, "--verify-tag", "--draft", "--title", f"TVFDN {version}", "--notes-file", str(notes))
    gh("release", "upload", tag, *map(str, files), str(checksum), "--repo", repository, "--clobber")
    assets = json.loads(gh("release", "view", tag, "--repo", repository, "--json", "assets"))["assets"]
    for path in files + [checksum]:
        asset = next(item for item in assets if item["name"] == path.name)
        if asset["digest"] != "sha256:" + hashlib.sha256(path.read_bytes()).hexdigest():
            raise ValueError(f"Uploaded checksum mismatch: {path.name}")
    gh("release", "edit", tag, "--repo", repository, "--draft=false", "--prerelease=false", "--latest", "--notes-file", str(notes))


if __name__ == "__main__":
    main()
