
# Opposite Renderer
Forked from [apartridge/OppositeRenderer](https://github.com/apartridge/OppositeRenderer)

In short *Opposite Renderer* is a GPU Photon Mapping Rendering Tool implemented in [CUDA](https://wikipedia.org/wiki/CUDA) using [OptiX](https://en.wikipedia.org/wiki/OptiX) library. It allows importing [Collada](https://en.wikipedia.org/wiki/Collada) scenes files and then render them to an image using [Progressive Photon Mapping](http://www.cgg.unibe.ch/publications/2011/progressive-photon-mapping-a-probabilistic-approach).

#####Why this fork?

This fork is intended to extend the original project to help implementing ILP problems. See this [paper](](http://ima.udg.es/~dagush/papers/surveyInvLighting.pdf)) for more info on the topic.

## Where To Start?
If this is your first time hearing about *Opposite Renderer*, we recommend you start with the original website: [http://apartridge.github.io/OppositeRenderer/](http://apartridge.github.io/OppositeRenderer/).


> This project is a part of Stian Pedersen's master's thesis project at NTNU. This demo renderer contains a GPU implementation of the Progressive Photon Mapping algorithm. It is written in C++ using CUDA and OptiX. The renderer has a GUI and can load scenes from the well known Collada (.dae) scene format. It has a client-server architecture allowing multi-GPU (distributed) rendering with any number of GPUs. It has been tested with up to six GPUs via Gigabit ethernet with very good speedup. 

![Conference Room Screenshot](http://apartridge.github.io/OppositeRenderer/images/thumbs/oppositeRendererScreenshot.png)


## Building and Running

### Dependencies

- [Visual Studio 2012](http://www.visualstudio.com/) or better
- [CUDA](https://developer.nvidia.com/cuda-downloads) v5.x
- [Optix SDK](https://developer.nvidia.com/optix-download) v3.x. **Note:** You must register to [Nvidia Developer Zone](https://developer.nvidia.com/user/register) First
- [Qt SDK](http://qt-project.org/downloads) 5.x for Windows (VS 201X)
- [FreeGlut](http://www.transmissionzero.co.uk/software/freeglut-devel/) MSVC Package
- [GLEW](http://sourceforge.net/projects/glew/files/) - OpenGL Extension Wrangler Library  
- [Open Asset Import Library](http://sourceforge.net/projects/assimp/files/)
- A [CUDA compatible GPU](https://developer.nvidia.com/cuda-gpus). Almost all recent GeForce GPUs support CUDA.
- Windows 7 or newer, running on x64.

### Building

The project needs some [environment variables](http://environmentvariables.org/Main_Page#Environment_variables) to be set so it can build. If you don't define them you will get missing files errors while compiling.
 
* [Define](http://environmentvariables.org/Getting_and_setting_environment_variables) the following environment variables:

	- `QTDIR` should point to your QT instalation dir.
	- `GLEW_PATH` point to where you extracted GLEW.
	- `ASSIMP_PATH` should point to Asset Import installation dir 
	- `FREEGLUT_PATH` should point to where you extracted FreeGlu.
	- `OPTIX_PATH` points to OptiX installation directory
	
	For example
	
	    QTDIR=C:\Qt\Qt5.2.1\5.2.1\msvc2012_64_opengl
	    GLEW_PATH=C:\Program Files\Common Files\glew
	    ASSIMP_PATH=C:\Program Files\Assimp
	    FREEGLUT_PATH=C:\Program Files\Common Files\freeglut
	    OPTIX_PATH=C:\Program Files\NVIDIA Corporation\OptiX SDK 3.0.1

* Open the Visual Studio Solution `OppositeRenderer.sln` and build.
	* *Note:* Be sure that all projects are in x64 configuration

### Running

1. Go to the folder `%USER_ROOT%\VisualStudioBuilds\OppositeRenderer\Debug`
2. Open `Standalone.exe`
3. Enjoy!