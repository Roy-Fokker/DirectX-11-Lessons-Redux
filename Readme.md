# Direct X 11 Lessons
Need to recollect how this API works.
Since learning DX 12 is proving difficult.

## References
- https://bell0bytes.eu/game-programming/
- http://www.rastertek.com/tutdx11s2.html

## Tools
- Visual Studio 2019 Community - Latest

## Dependencies
Library dependencies are installed via [vcpkg](https://github.com/microsoft/vcpkg). 
- [fmt](https://fmt.dev/latest/index.html)
- [cppitertools](https://github.com/ryanhaining/cppitertools)
- [DirectXTK](https://github.com/microsoft/DirectXTK) (for DDSTextureLoader)

## Project Configuration
- C++: Latest (2a)
- Subsystem: Windows
- Entry point: main
- Defines: UNICODE, NOMINMAX, WIN32_LEAN_AND_MEAN  ☜(ﾟヮﾟ☜)

## Subprojects
- L1.Basic_Window: Create empty window displaying flat color.
- L2.Draw_Cube: Draw a cube, use VS, PS and prespective projection.
- L3.Textured_Cube: Draw a cube with textured faces.
- L4.Direct2D: Draw text using Direct2D and DirectWrite.
- L5.Lighting: Diffuse and Specular Lights on Cube.