# PlatformIO extra script: compile Waveshare font files from parent directory
# This avoids duplicating the large font bitmap arrays in our project.
Import("env")
import os

waveshare_dir = os.path.normpath(
    os.path.join(env.get("PROJECT_DIR"), "..", "5in79_e-Paper_B_ESP32", "ESP32")
)

# Add Waveshare dir to include path (after src/ so our fonts.h takes priority)
env.Append(CPPPATH=[waveshare_dir])

# Compile font files from the Waveshare directory
for fname in ["font8.cpp", "font12.cpp", "font16.cpp", "font20.cpp", "font24.cpp"]:
    env.BuildSources(
        os.path.join("$BUILD_DIR", "waveshare_fonts"),
        waveshare_dir,
        "+<%s>" % fname
    )
