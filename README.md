# Win32 Acrylic Effect
A Demonstration of Acrylic Effect on C++ Win32 applications using Direct Composition and DWM private APIs.

<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#overview">Overview</a>
      <ul>
        <li type="disc">A Short History</li>
        <li type="disc">About This Project</li>
      </ul>
    </li>
    <li>
      <a href="#possible-methods">Possible Methods</a>
      <ul>
        <li type="disc">SetWindowCompositionAttribute()</li>
        <li type="disc">Windows.UI.Composition - Interop</li>
        <li type="disc">Windows Maginification API</li>
        <li type="disc">Desktop Duplication API</li>
        <li type="disc">DWM Private APIs</li>
      </ul>
    </li>
    <li>
      <a href="#backdrop-sources">Backdrop Sources</a>
      <ul>
        <li type="disc">Desktop Backdrop</li>
        <li type="disc">Host Backdrop</li>
      </ul>
    </li>
    <li>
      <a href="#features">Features</a>
    </li>
    <li>
      <a href="#known-limitations">Known Limitations</a>
    </li>
    <li>
      <a href="#supported-versions">Supported Versions</a>
    </li>
  </ol>
</details>

## Overview

![sample](https://user-images.githubusercontent.com/72641365/121798817-2d04d300-cc46-11eb-845b-0f2863d7cfde.png)

### A Short History
The Windows 10 Acrylic Effects was first introduced on UWP Apps after the Windows Fall Creators Update on 2017, Since Then Developers were trying to implement the acrylic effect on normal Windows Applications such as WPF, Win32 etc.The most Commonly used method was `SetWindowCompositionAttribute()`, but it didn't last long, from Windows 10 1903 the window starts to become laggy while dragging *(This issue seems to be fixed in windows insider previews but not on any stable versions of windows)*.  

### About This Project 
This project is just a demonstration about implementing *(or possible ways to implement)* a custom acrylic effect on Win32 Application using DWM APIs and Direct Composition. This is just a basic implementation, Still there is a lot of room for improvement such as noise overlay and other blending effects. We will also discuss about possible ways to achieve Acrylic Effects.

## Possible Methods
There are different ways to achieve Acrylic Effect on Windows 10 and some might be able to work on Windows 7,8 *(i haven't tested yet)*.

### SetWindowCompositionAttribute()

`SetWindowCompositionAttribute()` is an undocumented api in windows which allows us to enable Acrylic Effect, DWM Blur *(same as `DwmEnableBlurBehindWindow()`)* and transparency for Window. Since it is an undocumented API this api might be changed or removed in future versions of windows. As i had mentioned earlier creating acrylic effect using this API will cause lag while dragging or resizing window, And also the blur effect will disappear during maximize until the window is completely maximized
### Windows.UI.Composition - Interop

The <a href="https://github.com/microsoft/Windows.UI.Composition-Win32-Samples">Windows.UI.Composition-Win32-Samples</a> on microsoft repository actually provides a way to implement acrylic effect on Windows Forms and WPF. This was one of the closest way to achieve acrylic effect in WPF. But WPF uses diffrent rendering technology and the acrylic effect is rendered using direct composition, So it always cause <a href="https://github.com/dotnet/wpf/issues/152">Airspace Issue.</a> There is only two possible ways to overcome this issue:
#### 1. Overlapping Windows:
You can create multiple WPF windows one for rendereing the WPF Content and one for Acrylic Effect, These windows should be transparent and must communicate with each other to sync the window resize, window drag and other events, this is possible by overriding `WndProc`. But this method will cause Flickerng on resize and also affects the Acrylic Effect during maximize animation. *(This has been tested before)*
#### 2. WPF Swapchain Hack:
This is another possible way that i haven't tested yet, but theoretically it might be working. This can be done by hacking into WPF Swapchain to obtain its back buffer and copy it into an ID2D1Bitmap or similar and then passing it to the Windows.UI.Composition.Visual by using somethng like <a href="https://docs.microsoft.com/en-us/windows/uwp/composition/composition-native-interop">Composition Native Interoperation.</a>  By this way we may be able to overcome the Airspace Issue with WPF Acrylic Effect. *(:sweat_smile: The truth is Currently i was only able to get the swapchain backbuffer from WPF)*

### Windows Maginification API
The Windows Manification API can also provide the visual behind a window *(Backdrop)* by using the `MagImageScalingCallback()`, but it has been deprecated after windows 7.There might be a chance for creating the blur effect on windows 7 by passing the bitmap obtained from `MagImageScalingCallback()` to D2D1Effects such as gaussian blur. The `MagImageScalingCallback()` also throws exception if the source rectangle coordinates is out of desktop coordinates.

### Desktop Duplication API
The Windows Desktop Duplication API is also one possible way for creating an acrylic effect, The Desktop Duplication API can be used to capture the entire desktop including the window itself, but on windows 10 2004 Edition Microsoft has added a new parameter `WDA_EXCLUDEFROMCAPTURE` in `SetWindowDisplayAffinity()` function which will help us to capture the whole desktop but excluding the windows for which `WDA_EXCLUDEFROMCAPTURE` is applied. But everything comes with a price, by doing so you wont be able to capture that window by screenshot or screencapture and earlier versions of windows doesn't support `WDA_EXCLUDEFROMCAPTURE`.

### DWM Private API
The `dwmapi.dll` consist of some private api which can be used to capture each windows or a group of windows into an `IDCompositionVisual`. If you decompile the dwmapi.dll using some decompiler like <a href="https://hex-rays.com/ida-free/">IDA By HexRays</a> and explore `DwmEnableBlurBehindWindow()` function you will be able to find `DwmpCreateSharedThumbnailVisual()` function which helps us to capture a window into `IDCompositionVisual`. In this sample we mainly use two functions `DwmpCreateSharedThumbnailVisual()` and `DwmpCreateSharedVirtualDesktopVisual()` for creating the visual and we use `DwmpUpdateSharedVirtualDesktopVisual()` and `DwmpUpdateDesktopThumbnail()` for updating the visual.  

<b>A special thanks to <a href="https://github.com/ADeltaX">ADeltaX</a> for the implementation.*(<a href="https://gist.github.com/ADeltaX/aea6aac248604d0cb7d423a61b06e247">DWM Thumbnail/VirtualDesktop IDCompositionVisual example</a>)*</b>

Once the visual is obtained, The graphics effect such as Gaussian blur,Saturation etc.. is applied to the visual and finally it is rendered back to the window by using Direct Composition. This method also has several limitations such as Airspace Issue while using with other platforms like WPF.

## Backdrop Sources
In this sample we have defined two backdrop sources for acrylic effect, They are:
#### 1. Desktop Backdrop
A Desktop Backdrop use the desktop visual as the source for acrylic effect. so no other windows under the host window will be captured for blurring. it will be the plain desktop wallpaper. This is done by using `DwmpCreateSharedThumbnailVisual()`.
#### 2. Host Backdrop
A Host Backdrop uses Desktop Backdrop  as the background visual and also captures all the windows which are under the host window for blurring. This is done by using DwmpCreateSharedVirtualDesktopVisual() function.

You can adjust the backdrop source in this example using `BackdropSource` enum.

```C++
// For Host Backdrop
compositor->SetAcrylicEffect(hwnd, AcrylicCompositor::BACKDROP_SOURCE_HOSTBACKDROP, param); 
// For Desktop Backdrop
compositor->SetAcrylicEffect(hwnd, AcrylicCompositor::BACKDROP_SOURCE_DESKTOP , param);     
```
  
## Features
  <ul>
    <li>Reduced flickering while resizing or dragging</li>
    <li>Multiple Backdrop Sources</li>
    <li>You can exclude a particular window from acrylic like the host window does for itself.</li>
  </ul>
  
## Known Limitations
<ul>
  <li>Currently supported only on 64 bit Windows 10.</li>
  <li>Sometimes the Desktop icons are missing in the acrylic effect.</li>
  <li>There is no straight way to implement this in WPF or Win UI3.</li>
  <li>Airspace Issue : Currently you can only use Direct Composition to draw something on top of acrylic effect.</li>
  <li>Realtime Acrylic not supported, which means the host window redraw acrylic only when it is activated so fallback color is used.</li>
  <li>Exclusion Blending and Noise effect not added (Can be implemented, Pull requests are welcome)</li>
</ul>

## Supported Versions
This sample has been currently tested on:


| Operating System | Version | Build |
| ------------- | ------------- | ------------- |
| Windows 10 Pro  | 20H2  | 19042.1052|

<hr/>

## Join with us :triangular_flag_on_post:

You can join our <a href="https://discord.gg/PEqkwGcEtu">Discord Channel</a> to connect with us. You can share your findings, thoughts and ideas on improving / implementing the Acrylic Effect on normal apps.
