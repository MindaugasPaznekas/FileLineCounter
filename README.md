### Overview
CLI program for Windows OS that calculates a total of lines in all files in a given directory.
Assumes that files found in the given directory are text files. Able to handle nested directories.
Note: Uses https://en.cppreference.com/w/cpp/filesystem so at least C++17 is needed to compile.

### How to build it
1. Clone the project.
2. Compile.
  I used MSVC compiler built in Visual Studio. (VS 2022 Community version with workload 'Desktop Development with C++' and make sure MSVC v143 is used).
  Open the included VS project and build (Ctrl + B). Note: You may need to turn on C++17/20 in Project->FileLineCounter properties->C/C++->Language->C++ Language standart
  I quickly checked on MinGW(10.0-22621) g++ seems to work fine [g++ FileLineCounter.cpp -o FileLineCounter.exe].

3. Finally run the built executable. Provide it with folder You want to be searched and counted. 
Example: FileLineCounter.exe C:\Folder
