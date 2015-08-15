
# Opposite Renderer
Forked from [apartridge/OppositeRenderer](https://github.com/apartridge/OppositeRenderer)

In short *Opposite Renderer* is a GPU Photon Mapping Rendering Tool implemented in [CUDA](https://wikipedia.org/wiki/CUDA) using [OptiX](https://en.wikipedia.org/wiki/OptiX) library. It allows importing [Collada](https://en.wikipedia.org/wiki/Collada) scenes files and then render them to an image using [Progressive Photon Mapping](http://www.cgg.unibe.ch/publications/2011/progressive-photon-mapping-a-probabilistic-approach).

#####Why this fork?

This fork is intended to extend the original project to help implementing ILP problems. See this [paper](http://ima.udg.es/~dagush/papers/surveyInvLighting.pdf) for more info on the topic.

## Where To Start?
If this is your first time hearing about *Opposite Renderer*, we recommend you start with the original website: [http://apartridge.github.io/OppositeRenderer/](http://apartridge.github.io/OppositeRenderer/).


> This project is a part of Stian Pedersen's master's thesis project at NTNU. This demo renderer contains a GPU implementation of the Progressive Photon Mapping algorithm. It is written in C++ using CUDA and OptiX. The renderer has a GUI and can load scenes from the well known Collada (.dae) scene format. It has a client-server architecture allowing multi-GPU (distributed) rendering with any number of GPUs. It has been tested with up to six GPUs via Gigabit ethernet with very good speedup. 

![Conference Room Screenshot](http://apartridge.github.io/OppositeRenderer/images/thumbs/oppositeRendererScreenshot.png)


## Building and Running

### Dependencies

- [Visual Studio 2013](http://www.visualstudio.com/)
- [CUDA](https://developer.nvidia.com/cuda-downloads) v7.0 
- [OptiX](https://developer.nvidia.com/download) v3.8
   - **Note:** You must register first by completing a form. Then download information is emailed to you. 
- [Qt Open Source](http://www.qt.io/download-open-source/) 5.5 for Windows 
  - Specifically the version `msvc2013_64` for 5.5
- [FreeGlut](http://www.transmissionzero.co.uk/software/freeglut-devel/) MSVC Package
- [GLEW](http://sourceforge.net/projects/glew/files/) - OpenGL Extension Wrangler Library  
- [Open Asset Import Library](http://sourceforge.net/projects/assimp/files/)
- A [CUDA compatible GPU](https://developer.nvidia.com/cuda-gpus) 2.0 or greater. 
  - Almost all recent GeForce GPUs support CUDA.
  - This is a run only dependency, as the project can be built if the system doesn't
    have a CUDA compatible card.
- Windows 8.1 or newer, running on 64 bits.

Notes:

- This project may work with with Qt 5.2, CUDA 5.5, Visual Studio 2012 and OptiX 3.5.
But require slight dependency changes. 

## Building

The project needs some [environment variables](http://environmentvariables.org/Main_Page#Environment_variables) to be set so it can build. If you don't define them you will get missing files errors while compiling.
 
* [Define](http://environmentvariables.org/Getting_and_setting_environment_variables) the following environment variables:

	- `QTDIR` should point to your QT instalation dir.
	- `GLEW_PATH` point to where you extracted GLEW.
	- `ASSIMP_PATH` should point to Asset Import installation dir 
	- `FREEGLUT_PATH` should point to where you extracted FreeGlut.
	- `OPTIX_PATH` points to OptiX installation directory
	
	For example
	
	    QTDIR=C:\Qt\5.5\msvc2013_64
	    GLEW_PATH=C:\Program Files\Common Files\glew
	    ASSIMP_PATH=C:\Program Files\Assimp
	    FREEGLUT_PATH=C:\Program Files\Common Files\freeglut
	    OPTIX_PATH=C:\ProgramData\NVIDIA Corporation\OptiX SDK 3.8.0

* Open the Visual Studio Solution `OppositeRenderer.sln` and build.

## Running 

### Running OppositeRenderer on a example scene

This can be done inside Visual Studio. There are tested `*.dae` scenes on the `OppositeRenderer\ILPSolver\examples` folder

1. Select `Standalone` as the primary project. This is the renderer.
2. Hit on Debug. Compilation can take several minutes.
3. Wait for the GUI program to open, and go to File, Open
4. Select any of the example files
5. Enjoy!

### Running the ILPSolver on a example scene

If you really want, you can try to solve a sample ILP, using the `ILPSolver` library.

1. Select `ILPSolver` as the primary project. 
2. Hit on Debug. Compilation can take several minutes.
3. Wait for the command line program to finish. It may take a minute or two
   depending on the CUDA core count of the running GPU
4. Go to the folder `OppositeRenderer\ILPSolver\examples\output` and there
   you will see the output of the run
   - `solutions.csv` with the optimal configurations
   - A collection of images for the optimal results

## Known issues

- Changing the Rendering Method (Photon Mapping, Progressive Photon Mapping, etc.) makes the program to crash due to OptiX Context reallocation errors.
- Sometimes the program crash while exiting due to OptiX Context destruction errors.