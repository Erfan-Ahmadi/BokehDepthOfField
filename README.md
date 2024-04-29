
# Bokeh Depth Of Field

Implementing Different Algorithms to mimic Bokeh Depth Of Field: A Physical Camera Effect created due to Focal Length, Aperture size, shape

This Project is using [The Forge Rendering API](https://github.com/ConfettiFX/The-Forge), a cross-platform rendering, and targeted for these devices: PC, Android, macOS, IOS, IPad OS devices.

❗️❗️ This Project is Further Developed on [The Forge Rendering API](https://github.com/ConfettiFX/The-Forge) as a UnitTest and **no longer maintained in this repository page**. But you can still learn something from reading the shaders ❗️❗️ 

## Motivation

To mimic the physical camera in real-time in an efficient way to reduce postfx overheads.

## Sample Real Camera Effects

<p align="center">
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/real/newyork_maybe.jpg" alt="" width="300"/>
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/real/beauty.jpg" alt="" width="300"/>
</p>

Here is the 3 different methods implemented explained briefly:

*I will soon write a blog post with a lot more detail and pros-and-cons on it.*

## Techniques Brief Description:

### 1. Circular Seperable Depth of Field

- [x] Computation in 1/2 Resolution
- [x] Seperable Filter
- [x] Seperate Near and Far
- [x] Multiple Passess
- [x] Scatter-as-Gather

**Circular Sperable DOF** by [Kleber Garcia](https://github.com/kecho/CircularDofFilterGenerator/blob/master/circulardof.pdf) at Frostbite EA which was shipped with **FIFA17** , **NHS**, **Mass Effect Andromeda**, **Anthem** and is going to be shipped with the new **Need For Speed Heat**.

This technique is a seperable convolution filter like the Gaussian Filter and this makes it super faster than the "1-Pass 2D Kernel".

Derivation of the Kernel Weights and the Math includes **Complex Numbers** and **Fourier Transforms** explained in [Olli Niemitalo's blog post](http://yehar.com/blog/?p=1495).

In his paper some important notes were missing like how we do the "blending" so I had to get creative and do a lot of thinking myself.

This method is operating on Near, Far Field Seperatly on multiple passes 

- [Circular Seperable Depth of Field](https://github.com/Erfan-Ahmadi/BokehDepthOfField/tree/master/src/CircularDOF) 
<p align="center">
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/simulation/circular-dof/1.jpg" alt="" width="400"/>
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/simulation/circular-dof/2.jpg" alt="" width="400"/>
</p>

### 2. Practical Gather-based Bokeh Depth of Field

- [x] Computation in 1/2 Resolution
- [ ] Seperable Filter
- [x] Seperate Near and Far
- [x] Multiple Passess
- [x] Scatter-as-Gather

**Practical Gather-Based Depth of Field** which is fully described in [GPU-Zen Book](https://www.amazon.com/GPU-Zen-Advanced-Rendering-Techniques-ebook/dp/B0711SD1DW). 

This approach is also Gather-Based but the sampling and computation is **not** seperable and is circular sampling with 48 samples.

- [Practical Gather-based Bokeh Depth of Field](https://github.com/Erfan-Ahmadi/BokehDepthOfField/tree/master/src/GatherBasedBokeh)
<p align="center">
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/simulation/gather-based/1.jpg" alt="" width="400"/>
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/simulation/gather-based/3.jpg" alt="" width="400"/>
</p>

### 3. Single Pass Depth of Field

- [ ] Computation in 1/2 Resolution
- [x] Computation in Full Resolution
- [ ] Seperable Filter
- [ ] Seperate Near and Far
- [x] Single Pass
- [x] Scatter-as-Gather

**Depth of Field in a Single Pass** which is described in Dennis Gustafsson awsome [blog post](http://blog.tuxedolabs.com/2018/05/04/bokeh-depth-of-field-in-single-pass.html).

This Depth of Field effect is done in a **Single Pass**.

Due to this technique being in full-res and needing a lot more sample and calculations It performance is now worse than the other two.

There are a lot of optimizations for this technique but since I forced it to be in a single pass my hands were tight (by myself).

- [Single Pass Depth of Field](https://github.com/Erfan-Ahmadi/BokehDepthOfField/tree/master/src/SinglePassBokeh)
<p align="center">
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/simulation/single-pass/2.jpg" alt="" width="400"/>
  <img src="https://github.com/Erfan-Ahmadi/BokehDepthOfField/raw/master/screenshots/simulation/single-pass/4.jpg" alt="" width="400"/>
</p>

## Implemented Techniques

- [Circular Seperable Depth of Field](https://github.com/Erfan-Ahmadi/BokehDepthOfField/tree/master/src/CircularDOF) - [Resources](#CircularDOF)
- [Practical Gather-based Bokeh Depth of Field](https://github.com/Erfan-Ahmadi/BokehDepthOfField/tree/master/src/GatherBasedBokeh) - [Resources](#GatherBased)
- [Single Pass Depth of Field](https://github.com/Erfan-Ahmadi/BokehDepthOfField/tree/master/src/SinglePassBokeh) - [Resources](#SinglePass)

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

## Resources 

All Bokeh Links and Book Chapters gathered for R&D are in [this github gist](https://gist.github.com/Erfan-Ahmadi/e27842ce9daa163ec10e28ee1fc72659); for detailed resources and links see below:

- <a name="CircularDOF"></a>Circular Depth of Field (Kleber Garcia)
  - [Circular Separable Convolution Depth of Field Paper - Keleber Garcia](https://github.com/kecho/CircularDofFilterGenerator/blob/master/circulardof.pdf)
  - [Circularly Symmetric Convolution and Lens Blur - Olli Niemitalo](http://yehar.com/blog/?p=1495)
- <a name="GatherBased"></a>GPU Zen (Wolfgang Engel) : Screen Space/Practical Gather-based Bokeh Depth of Field
  - [Book Amazon](https://www.amazon.com/GPU-Zen-Advanced-Rendering-Techniques-ebook/dp/B0711SD1DW)
  - [GitHub](https://github.com/wolfgangfengel/GPUZen)
- <a name="SinglePass"></a>[Bokeh depth of field in a single pass - Dennis Gustafsson](http://blog.tuxedolabs.com/2018/05/04/bokeh-depth-of-field-in-single-pass.html)

## Built and Tested Devices (See UnitTest 29_DepthOfField in TheForge)

- [x] PC
  - [x] Windows 10 - **DX12**
  - [x] Windows 10 - **Vulkan 1.1**
  - [ ] Windows 7 - **Fallback DX11**
  - [x] Linux Ubuntu 18.04 LTS - **Vulkan 1.1**
- [x] Android Pie - **Vulkan 1.1**
- [x] macOS / iOS / iPad OS - **Metal 2.2**
