# Bokeh Depth Of Field

Implementing Different Algorithms to mimic Bokeh Depth Of Field: A Physical Camera Effect created due to Aperture size and shape

This Project is using [The Forge Rendering API](https://github.com/ConfettiFX/The-Forge), a cross-platform rendering, and targeted for these devices: PC, Android, macOS, IOS, IPad OS devices.

## Motivation

To mimic the physical camera in real-time in an efficient way to reduce postfx overheads.

## Sample Real Camera Effects

<p align="center">
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/real/night.jpg" alt="" width="600"/>
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/real/subway.jpg" alt="" width="600"/>
</p>

## Real-Time Bokeh Screen Shots

- None Yet :)

## Techniques

- [x] [Circular Depth of Field (Kleber Garcia)](#Resources)

## Build
  - [x] Visual Studio 2017:
    * Put Repository folder next to cloned [The-Forge API](https://github.com/ConfettiFX/The-Forge) repository folder
    * Build [The-Forge API](https://github.com/ConfettiFX/The-Forge) Renderer and Tools and place library files in **build/VisualStudio2017/$(Configuration)** for example build/VisualStudio2017/ReleaseDx
    * Assimp: 
      - Build Assimp for Projects involving Loading Models
      - Don't change the directory of assimp.lib, the Default is: 
         - **The-Forge\Common_3\ThirdParty\OpenSource\assimp\4.1.0\win64\Bin**

## Issues

Report any bug on your devices with most detail [here](https://github.com/Erfan-Ahmadi/BokehDepthOfField/issues)

## <a name="Resources"></a> Resources 

All Bokeh Links and Book Chapters gathered for R&D are in [this github gist](https://gist.github.com/Erfan-Ahmadi/e27842ce9daa163ec10e28ee1fc72659); for detailed resources and links see below:

- Circular Depth of Field (Kleber Garcia)
  - [Circular Separable Convolution Depth of Field Paper - Keleber Garcia](https://github.com/kecho/CircularDofFilterGenerator/blob/master/circulardof.pdf)
  - [Circularly Symmetric Convolution and Lens Blur - Olli Niemitalo](http://yehar.com/blog/?p=1495)

## Built and Tested Devices

- [x] PC
  - [x] Windows 10 - **DX12**
  - [ ] Windows 7 - **Fallback DX11**
  - [ ] Linux Ubuntu 18.04 LTS - **Vulkan 1.1**
- [ ] Android Pie - **Vulkan 1.1**
- [ ] macOS / iOS / iPad OS - **Metal 2.2**
