# cmake_minimum_required(VERSION 3.21)

# project(mygame LANGUAGES C ASM)

# # 1. Build the engine as a static library
# add_subdirectory(../engine)

# 2. Add your game executable
psn00bsdk_add_executable(mygame GPREL
    src/main.c
)

# 3. Link the engine
target_link_libraries(mygame PRIVATE engine)

# 4. Embed a TIM sprite atlas as a binary blob
#    The symbol _binary_TILESET_TIM is then available as an extern uint32_t[].
psn00bsdk_target_incbin(mygame PRIVATE _binary_TILESET_TIM tileset.tim)

# 5. Build a CD image
psn00bsdk_add_cd_image(mygame_iso mygame iso.xml DEPENDS mygame system.cnf)