"""Check real PS/2 cursor damage and window geometry using QMP scanout."""
from pathlib import Path
from time import monotonic, sleep

INK = bytes.fromhex("06100a")
LIGHT = bytes.fromhex("e3f5e8")
TITLE = bytes.fromhex("467c53")


def read_frame(path):
    header, dims, maximum, rgb = Path(path).read_bytes().split(b"\n", 3)
    width, height = map(int, dims.split())
    assert header == b"P6" and maximum == b"255" and len(rgb) == 3 * width * height
    return width, height, rgb


def cursor(frame):
    width, height, rgb = frame
    i = rgb.find(INK)
    while i >= 0:
        x, y = (i // 3) % width, (i // 3) // width
        step = width * 3
        if (i % 3 == 0 and x + 1 < width and y + 12 < height
                and rgb[i + step:i + step + 6] == INK + LIGHT
                and rgb[i + 12 * step:i + 12 * step + 3] == INK):
            return x, y
        i = rgb.find(INK, i + 1)
    return None


def check_pointer(rpc, directory, device="PS/2"):
    path = Path(directory, "pointer.ppm").resolve()

    def frame():
        rpc("screendump", {"filename": str(path)})
        return read_frame(path)

    def events(*items):
        rpc("input-send-event", {"events": list(items)})

    def motion(dx, dy):
        events(*[{"type": "rel", "data": {"axis": axis, "value": value}}
                 for axis, value in (("x", dx), ("y", dy)) if value])

    def button(down):
        events({"type": "btn", "data": {"button": "left", "down": down}})
        sleep(0.12)

    def await_frame(predicate):
        end = monotonic() + 5
        while monotonic() < end:
            view = frame()
            if predicate(view):
                return view
            sleep(0.04)
        raise AssertionError("guest framebuffer did not reach expected state")

    background = frame()
    assert cursor(background) is None, "unexpected pointer before the first PS/2 packet"
    motion(3, 4)
    first = await_frame(cursor)
    old = cursor(first)
    motion(23, 7)
    second = await_frame(lambda view: cursor(view) not in (None, old))
    new = cursor(second)
    width, height, _ = second

    # Compose only the expected cursor onto the captured pre-input frame.
    # Comparing the whole frame also checks that the old cursor disappeared.
    for view, (x, y) in ((first, old), (second, new)):
        assert view[:2] == background[:2]
        expected = bytearray(background[2])
        for dy in range(min(13, height - y)):
            row_width = min(dy + 1 if dy < 8 else 4, width - x)
            start = ((y + dy) * width + x) * 3
            expected[start:start + row_width * 3] = INK + LIGHT * (row_width - 1)
        assert view[2] == expected, "cursor shape, restored background or surrounding scene differs"

    def move_to(x, y):
        for _ in range(100):
            prev = cursor(frame())
            assert prev, "cursor missing"
            if prev == (x, y):
                return
            dx, dy = x - prev[0], y - prev[1]
            motion(max(-32, min(32, dx)), max(-32, min(32, dy)))
            await_frame(lambda v: cursor(v) not in (prev, None))
        raise AssertionError("PS/2 relative pointer did not reach title bar")

    def pixel(view, x, y):
        pos = (y * view[0] + x) * 3
        return view[2][pos:pos + 3]

    z = 2 if width >= 1280 and height >= 720 else 1
    ox, oy = (width - 640 * z) // 2, (height - 360 * z) // 2
    # Tab selected Notes in live_boot.py. Its initial rectangle is (98,36,448,242).
    tx, ty = ox + 98 * z, oy + 36 * z
    assert pixel(second, tx + 2*z, ty + 2*z) == TITLE
    move_to(tx + 12*z, ty + 8*z)
    button(True)
    move_to(tx + 36*z, ty + 24*z)
    button(False)
    after = await_frame(lambda v: pixel(v, tx+2*z, ty+2*z) != TITLE)
    assert pixel(after, tx+26*z, ty+18*z) == TITLE
    # Moving after release must leave the new title bar in place.
    move_to(tx + 80*z, ty + 45*z)
    assert pixel(frame(), tx+26*z, ty+18*z) == TITLE
    print(f"PASS: {device} {width}x{height}: cursor {old}->{new}, background restored; Notes dragged 24x16 logical pixels and released")
    gx, gy = tx+(24+448-5)*z, ty+(16+242-5)*z
    grip = bytes.fromhex("345f43")
    assert pixel(frame(), gx, gy) == grip
    move_to(gx, gy)
    button(True)
    move_to(gx-36*z, gy-20*z)
    button(False)
    move_to(tx+80*z, ty+45*z)
    resized = frame()
    assert pixel(resized, gx-36*z, gy-20*z) == grip
    assert pixel(resized, gx, gy) != grip
    print(f"PASS: {device} resize grip moved 36x20 logical pixels and old grip was restored")
