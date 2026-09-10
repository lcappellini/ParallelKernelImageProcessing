from PIL import Image

image = Image.open("original.jpg")

# 16:9
resolutions = [
    (640, 360, "nHD"),
    (960, 540, "qHD"),
    (1280, 720, "HD"),
    (1366, 768, "WXGA"),
    (1600, 900, "HD+"),
    (1920, 1080, "Full HD"),
    (2560, 1440, "QHD"),
    (3840, 2160, "4K UHD"),
    (5120, 2880, "5K"),
    (7680, 4320, "8K UHD"),
    (15360, 8640, "16K UHD"),
]

for width, height, name in resolutions:
    resized = image.resize((width, height), Image.Resampling.LANCZOS)
    filename = f"in_{width}x{height} ({name}).bmp"
    resized.save(filename)

    print(filename)
