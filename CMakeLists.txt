# PSX2D CMake script
# (C) 2026 fvciprian - MPL licensed

cmake_minimum_required(VERSION 3.21)

project(
	PSX2D
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "PSX2D is a 2D game engine for the PlayStation 1, written in C and using the PSN00B SDK."
	HOMEPAGE_URL ""
)

add_subdirectory(engine)
add_subdirectory(mygame)