"""Build the public showcase from original recordings (Python + imageio-ffmpeg).

Usage: python Scripts/render_showcase.py --source /path/to/recordings
The edit intentionally uses no soundtrack; original recordings are not uploaded.
"""

import argparse
from pathlib import Path
import subprocess
import tempfile

import imageio_ffmpeg


SHOTS = [
    ("coche1.mp4", 1.2, 4.5, "01 / VEHICLE SCENE"),
    ("Grabación 2026-09-19 172107.mp4", 0.8, 4.5, "02 / THERMAL MATERIAL RESPONSE"),
    ("Grabación 2026-09-19 172540.mp4", 14.0, 5.0, "03 / LIVE PHYSICAL PARAMETERS"),
    ("Grabación 2026-09-19 182722.mp4", 3.0, 6.0, "04 / HANGAR SCENE"),
    ("Grabación 2026-09-19 184634.mp4", 0.1, 5.0, "05 / INDUSTRIAL ENVIRONMENT"),
    ("Grabación 2026-09-19 185134.mp4", 16.0, 5.0, "06 / ATMOSPHERE AND SKY"),
    ("Grabación 2026-09-19 190815.mp4", 0.5, 4.0, "07 / DETECTOR PROCESSING DEMO"),
    ("Grabación 2026-09-19 210221.mp4", 5.0, 4.5, "08 / INTEGRATED VISUALIZATION"),
]


def run(ffmpeg, args):
    subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "error", "-y", *args], check=True)


def text_filter(text, x, y, size, color="white"):
    font = ":fontfile='C\\:/Windows/Fonts/segoeui.ttf'" if Path("C:/Windows/Fonts/segoeui.ttf").is_file() else ""
    return f"drawtext=text='{text}':x={x}:y={y}:fontsize={size}:fontcolor={color}{font}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path("Documentation/Media"))
    args = parser.parse_args()
    for filename, *_ in SHOTS:
        if not (args.source / filename).is_file():
            parser.error(f"Missing recording: {filename}")
    args.output.mkdir(parents=True, exist_ok=True)
    ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
    encode = ["-an", "-c:v", "libx264", "-preset", "fast", "-crf", "24", "-pix_fmt", "yuv420p", "-r", "24"]

    with tempfile.TemporaryDirectory(prefix="irsim-showcase-") as task_tmp:
        temp = Path(task_tmp)
        segments = []
        intro = temp / "00.mp4"
        run(ffmpeg, ["-f", "lavfi", "-i", "color=c=0x0b1020:s=1280x720:r=24", "-t", "2.5",
                     "-vf", ",".join([
                         text_filter("IRSIM", 90, 220, 100),
                         text_filter("Physics-based infrared rendering", 94, 345, 36),
                         text_filter("C++17 / Unreal Engine 5 / LWIR", 94, 415, 23, "0xffb86b"),
                         "fade=t=in:d=0.25", "fade=t=out:st=2.2:d=0.3"]), *encode, str(intro)])
        segments.append(intro)
        for index, (filename, start, duration, label) in enumerate(SHOTS, 1):
            output = temp / f"{index:02d}.mp4"
            vf = ",".join([
                "scale=1280:720:force_original_aspect_ratio=decrease:flags=lanczos",
                "pad=1280:720:(ow-iw)/2:(oh-ih)/2:0x0b1020", "setsar=1", "fps=24",
                "drawbox=x=0:y=640:w=1280:h=80:color=0x0b1020@0.94:t=fill",
                text_filter(label, 42, 661, 26),
                text_filter("IRSIM / UE5", 1070, 669, 18, "0xffb86b"),
                "fade=t=in:d=0.16", f"fade=t=out:st={duration - 0.16}:d=0.16"])
            run(ffmpeg, ["-ss", str(start), "-i", str(args.source / filename), "-t", str(duration),
                         "-vf", vf, *encode, str(output)])
            segments.append(output)
            print(f"Rendered {label}", flush=True)
        outro = temp / "99.mp4"
        run(ffmpeg, ["-f", "lavfi", "-i", "color=c=0x0b1020:s=1280x720:r=24", "-t", "2.5",
                     "-vf", ",".join([
                         text_filter("FROM SCENE TO RADIANCE", 90, 270, 52),
                         text_filter("Jose Moreno Barbero", 94, 355, 30),
                         text_filter("github.com/JoseMorenob/thermalSimulationUnreal", 94, 415, 23, "0xffb86b"),
                         "fade=t=in:d=0.2", "fade=t=out:st=2.2:d=0.3"]), *encode, str(outro)])
        segments.append(outro)
        manifest = temp / "segments.txt"
        manifest.write_text("".join(f"file '{s.as_posix()}'\n" for s in segments), encoding="utf-8")
        video = args.output / "irsim-showcase.mp4"
        run(ffmpeg, ["-f", "concat", "-safe", "0", "-i", str(manifest), "-c", "copy", "-movflags", "+faststart", str(video)])
        gif_filter = (
            "[0:v]trim=start=7.5:end=10.5,setpts=PTS-STARTPTS[a];"
            "[0:v]trim=start=12.5:end=15.5,setpts=PTS-STARTPTS[b];"
            "[0:v]trim=start=23.5:end=26.5,setpts=PTS-STARTPTS[c];"
            "[0:v]trim=start=36.5:end=39.5,setpts=PTS-STARTPTS[d];"
            "[a][b][c][d]concat=n=4:v=1:a=0,fps=8,scale=768:-1:flags=lanczos,split[x][y];"
            "[x]palettegen=max_colors=128[p];[y][p]paletteuse=dither=bayer:bayer_scale=3[g]"
        )
        run(ffmpeg, ["-i", str(video), "-filter_complex", gif_filter, "-map", "[g]", "-loop", "0", str(args.output / "irsim-preview.gif")])
        run(ffmpeg, ["-ss", "13.5", "-i", str(video), "-frames:v", "1", "-q:v", "2", str(args.output / "irsim-poster.jpg")])
        run(ffmpeg, ["-i", str(video), "-vf", "fps=1/5,scale=384:216,tile=3x3", "-frames:v", "1", "-q:v", "2", str(temp / "contact-sheet.jpg")])
        # QA artifact is kept local rather than included in the public media set.
        qa = Path(task_tmp).parent / "irsim-showcase-contact-sheet.jpg"
        qa.write_bytes((temp / "contact-sheet.jpg").read_bytes())
    for name in ("irsim-showcase.mp4", "irsim-preview.gif", "irsim-poster.jpg"):
        path = args.output / name
        print(f"{name}: {path.stat().st_size:,} bytes")


if __name__ == "__main__":
    main()
