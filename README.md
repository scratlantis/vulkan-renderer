Vulkan Renderer accompanying my poster presentation at i3D Symposium 2021:

"Rendering Decals and Many Lights with Ray Tracing Acceleration Structures"

Instructions for Windows 10, Visual Studio 2019 and RTX Nvidia Graphics Card:
(Tested with RTX 2070 SUPER)

Step 1: Install Vulkan SDK 1.2.141.2
https://vulkan.lunarg.com/

Step 2: Install Nvidia Beta Drivers Version 457.33. The new beta drivers will NOT WORK, as we currently rely on the Kronos provisional extensions!!! https://developer.nvidia.com/vulkan-driver

Step 3: In Visual Studio: Propties (Alt+Enter) -> C/C++ -> General -> Additional Include Directories: Change Path from "C:\VulkanSDK\1.2.141.2\Include" -> "'your vulkan path'\Include"

Step 4: In Visual Studio: Propties (Alt+Enter) -> Linker -> General -> Additional Include Directories: Change Path from "C:\VulkanSDK\1.2.141.2\Lib" -> "'your vulkan path'\Lib"

Step 5: In VulkanContext.h, add correct VULKAN_PATH and DEVICE_INDEX of your Nvidia GPU.
(device indices and names of all GPUs are printed to the command line, on startup of the renderer)


Renderer Controls:
Movement: W,A,S,D,Crtl,Space
To change renderers at runtime:
X: forward
C: deferred
V: deferred with deferred decals
B: visibility buffer

To enable/disable features:
1,2,3...9,0

Current configuration is displayed in title bar

Move primary Light source / change intensity: Numpad

Editor:
Toggle On, Off: 0
Toggle Decals, Lights, Camera speed: F
Toggle Decal Type/Light intensity, Decal Layer/ Light R, Decal Weight/ Light G, Decal Size/ Light B: T
Increas: Q
Decreas: E


More detailed instructions to come...
