# StrawPoll
Straw Poll uWebSockets/FlatBuffers code based on Lukas Bergdoll “Web | C++” CppCon 2017 presentation:  
https://www.youtube.com/watch?v=l4ZZPrH95mM  
  
Before building the server, install _uWebSockets_ dependencies as described in its documentation.  
In Windows, it's easy to use _vcpkg_ as:  
```shell
vcpkg install openssl:x64-windows zlib:x64-windows libuv:x64-windows
```  
(Make usre that, when installing _vcpkg_, you enable its use for all projects.)
In Ubuntu:  
```shell
sudo apt-get install libssl-dev zlib1g-dev
```
  
After all _uWebSockets_ dependencies are installed, build the server using the provided CMakeLists.txt. For your convenience, the build script, `build.sh`, can be used in both Linux and Windows (in Git Bash in Windows).  
If you don't want to use `build.sh` on Windows, you can do out-of-source build manually:  
```shell
mkdir build.Windows
cd build.Windows
cmake -G "Visual Studio 15 2017 Win64" ..
cmake --build . --target ALL_BUILD --config Debug
```  
This will build _StrawPollSever.exe_ in _build.Windows/Debug_ and coppy the dependant DLLs there. When you run this executable, it starts a WebSocket server and listens on port 3003.  
  
To start the client, navigate to _client_ folder and perform the usual webdev operations:  
```shell
npm install
npm start
```
This will download all _npm_ dependencies and start an HTTP server on port 3000. 
