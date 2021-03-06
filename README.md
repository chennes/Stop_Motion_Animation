# PLS Stop Motion Creator
A C++ and Qt based stop-motion animation app.

This program provides a simplified stop-motion animation program suitable for ages 6 and up. It is designed 
specifically to minimize the number of options and reduce interface clutter so that kids can focus on creating 
their movie, and not on fighting with the user interface. It provides encoding via the FFmpeg library 
(included, or available at https://ffmpeg.org). Background music and sound effects can be added directly 
from the interface, so for most classroom uses it functions as a stand-alone stop motion solution.

The program was written by Chris Hennes, STEAM Specialist at Norman Public Library Central, a branch
of the [Pioneer Library System](http://pioneerlibrarysystem.org) in central Oklahoma, USA. It was designed for use in the library's stop-motion programs. 

You may download and distribute the software freely subject to the terms in the LICENSE file included
in the download. Note that the software makes use of two other open-source libraries (Qt and FFmpeg) whose
licensing is slightly more restrictive (LGPLv2 and LGPLv3, respectively). Source code for those libraries is
available from their websites, or by writing to [chennes@pioneerlibrarysystem.org](mailto:chennes@pioneerlibrarysystem.org). 

## Building from Source ##
If you'd like to compile the source code yourself, rather than using one of the [releases available here](https://github.com/chennes/Stop_Motion_Animation/releases), you will need:
* A C++ compiler with support for C++17 (development used Visual Studio 2017)
* Qt version 5.12
* QtCreator
* FFmpeg shared libraries

The code uses QtCreator's QMake-based build system. To build it:
1. Open the project file (StopMotionAnimation.pro)
1. Edit StopMotionAnimation.rc to update the paths for your FFmpeg libraries
1. In the Build menu choose Build All
