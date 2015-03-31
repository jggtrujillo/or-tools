

You can use or-tools from sources or from binary archives.
This section describes using from sources.

Please refer to InstallingFromBinaryArchives to learn more about installing from binary archives or modules.

# Getting the source #

Please visit http://code.google.com/p/or-tools/source/checkout to checkout the sources of or-tools.

# Content of the Archive #

Upon untarring the given operation\_research tar file, you will get the following structure:
or-tools/
| LICENSE-2.0.txt | Apache License |
|:----------------|:---------------|
| Makefile | Main Makefile |
| README | This file |
| bin/  | Where all binary files will be created |
| dependencies/ | Where third\_party code will be downloaded and installed |
| examples/com/ | Directory containing all java samples |
| examples/csharp/ | Directory containing C# examples and a visual studio 2010 solution to build them |
| examples/cpp/ | C++ examples |
| examples/python/ | Python examples |
| examples/tests/ | Unit tests |
| lib/ | Where libraries and jar files will be created |
| makefiles/ | Directory that contains sub-makefiles |
| objs/ | Where C++ objs files will be stored |
| src/algorithms/ | A collection of OR algorithms (non graph related) |
| src/base/ | Directory containing basic utilities |
| src/com/ | Directory containing java and C# source code for the libraries |
| src/constraint\_solver/ | The main directory for the constraint solver library |
| src/gen/ | The root directory for all generated code (java classes, protocol buffers, swig files) |
| src/graph/ | Standard OR graph algorithms |
| src/linear\_solver/ | The main directory for the linear solver wrapper library |
| src/util/ | More utilities needed by various libraries |
| tools/ | Binaries and scripts needed by various platforms |

# Installation on unix platforms #

## Extra packages ##

If you wish to use glpk, please download glpk from http://ftp.gnu.org/gnu/glpk/glpk-4.53.tar.gz and put the archive in or-tools/dependencies/archives.
If you have the license, please download scipoptsuite-3.1.0.tgz from http://zibopt.zib.de/download.shtml and  put the archive in or-tools/dependencies/archives.

## Ubuntu specific part ##

For linux users, please intall zlib-devel, bison, flex, autoconf, libtool, python-setuptools, python-dev, and gawk.

The command on ubuntu < 12.04 is the following:
```
sudo apt-get install bison flex python-setuptools python-dev autoconf libtool zlib-devel texinfo gawk
```

The command on ubuntu >= 12.04 is the following:
```
sudo apt-get install bison flex python-setuptools python-dev autoconf libtool texinfo gawk
```

The command on ubuntu >= 13.04 is the following:
```
sudo apt-get install bison flex python-setuptools python-dev autoconf libtool zlib1g-dev texinfo gawk
```

The command on ubuntu >= 14.04 is the following:
```
sudo apt-get install bison flex python-setuptools python-dev autoconf libtool zlib1g-dev texinfo gawk g++ curl texlive subversion make gettext
```

To use java on ubuntu, you need to install the jdk

```
sudo apt-get install openjdk-7-jdk
```

To use .NET on ubuntu, you need to install mono

I would recommend copying the mono archive (http://download.mono-project.com/sources/mono/mono-3.10.0.tar.bz2) to dependencies/archives instead of using the packaged mono-devel for ubuntu as 2.10.8 is old.

## Fedora specific part ##

The fedora command is:
```
 sudo yum install subversion bison flex python-setuptools python-dev autoconf libtool zlib-devel gawk
```

## Debian part ##

Please install the following packages on debian:

```
sudo apt-get install bison flex python-setuptools python-dev autoconf libtool texinfo texlive gawk g++ zlib1g-dev
```

## Mac OS X Specific part ##

On mac OS X, you need to install xcode command line tools. We are dropping support for version before Mountain Lion as we need updated version of the clang compiler/

  * You need to install the latest version of the command line tools for xcode (full xcode is not needed).
  * To install javac, just run `javac`, it will start the procedure to download java and install it.

To use .NET, you need mono. On Mac OS X, you need 64 bit support. Thus you need to build mono by hand. Copy the mono archive http://download.mono-project.com/sources/mono/mono-3.10.0.tar.bz2 to dependencies/archives.
You can use `dependencies/install/bin/mcs` to compile C# files and `dependencies/install/bin/mono` to run resulting .exe files.

## Build third party software ##

run:
```
   make third_party
   make install_python_modules
```

If you are on opensuse and maybe redhat, the `make install_python_module` will fail.
One workaround is described on this page http://stackoverflow.com/questions/4495120/combine-user-with-prefix-error-with-setup-py-install.

If you have root privilieges, you can replace the last line and install the python modules for all users with the following command:
```
  cd dependencies/sources/google_apputils_python_14
  sudo python2.7 setup.py install
  cd dependencies/sources/protobuf-512/python
  python2.7 setup.py build
  sudo python2.7 setup.py install
```

It should create the Makefile.local automatically.

Please note that the command:
```
  make clean_third_party
```

will clean all downloaded sources, all compiled dependencies, and Makefile.local.
It is useful to get a clean state, or if you have added an archive in dependencies.archives.

# Installation on Windows #

Create the or-tools svn copy where you want to work.

At this point, you must choose if you are going to compile in 32 or 64 bit mode. If windows is 32 bit, you have no choice but to use the 32 bit version.
If windows is 64 bit, you can choose. We recommend compiling in 64 bit mode. This choice affects the following steps. You must install the corresponding java and python versions.
You must also use a same compilation terminal from the visual studio tools menu.

Install python from http://www.python.org/download/releases/2.7/

Install java JDK from the oracle site.

You need to install python-setuptools for windows. See the installation instructions at: http://pypi.python.org/pypi/setuptools#windows

If you wish to use glpk, please download glpk from http://ftp.gnu.org/gnu/glpk/glpk-4.52.tar.gz and put the archive in or-tools/dependencies/archives.

Then launch a terminal from the visual studio tools menu (32 or 64 bit according to your installation, this must be consistent with the java and python versions installed before) and use it to launch the following commands.

**Please make sure that svn.exe, nmake.exe and cl.exe are in your path**.

If you do not find svn.exe, please install a svn version that offers the command line tool. If cl.exe or nmame.exe are not in your path, it means that the terminal is not the one found in the visual studio tools menu but a generic one.

Then you can download all dependencies and build them using:

```
   make third_party
```

then edit Makefile.local to point to the correct python and java installation. For instance, on my system, it is:
```
WINDOWS_JDK_DIR = c:\\Program Files\\Java\\jdk1.7.0_51
WINDOWS_PYTHON_VERSION = 27
WINDOWS_PYTHON_PATH = C:\\python27
```

Please note that you should not put "" aroud the jdk path to take care of the space. This is taken care of in the makefiles.

Afterwards, to use python, you need to install google-apputils.

```
  copy dependencies\install\bin\protoc.exe   dependencies\sources\protobuf\src
  cd dependencies/sources/google-apputils
  c:\python27\python.exe setup.py install
  cd dependencies/sources/protobuf/python
  c:\python27\python.exe setup.py build
  c:\python27\python.exe setup.py install
```

Please note that the command:
```
  make clean_third_party
```

will clean all downloaded sources, all compiled dependencies, and Makefile.local. It is useful to get a clean state, or if you have added an archive in dependencies.archives.

# Compiling libraries and running examples #

## Compiling libraries ##

All build rules use make (gnu make), even on windows. A make.exe binary is provided in the tools sub-directory.

You can query the list of targets just by typing

`make`

You can then compile the library, examples and python, java, and .NET wrappings for the constraint solver, the linear solver wrappers, and the algorithms:

`make all`


To compile in debug mode, please use

`make DEBUG=-g all`

or

`make DEBUG="/Od /Zi" all`

under windows.


You can clean everything using

`make clean`

When everything is compiled, you will find under or-tools/bin and or-tools/lib:
  * some static libraries (libcp.a, libutil.a and libbase.a, and more)
  * One binary per C++ example (e.g. nqueens)
  * C++ wrapping libraries (_pywrapcp.so, linjniwrapconstraint\_solver.so)
  * Java jars (com.google.ortools.constraintsolver.jar...)
  * C# assemblies_

## C++ examples ##

You can execute C++ examples just by running then:

`./bin/magic_square `


## Python examples ##

For the python examples, as we have not installed the constraint\_solver module, we need to use the following command:

on windows:
`set PYTHONPATH=%PYTHONPATH%;<path to or-tools>\src`, then `c:\Python27\python.exe example\python\sample.py`.

On unix:
`PYTHONPATH=src <python_binary> examples/python/<sample.py>`

As in

`PYTHONPATH=src python2.6 examples/python/golomb8.py`

There is a special target in the makefile to run python examples. The above example can be run with

```
  make rpy EX=golomb8
```

## Java examples ##

You can run java examples with the
` run_<name> makefile` target as in:

`make run_RabbitsPheasants`

There is a special target in the makefile to run java examples. The above example can be run with

```
  make rjava EX=RabbitsPheasants
```

## .NET examples ##

If you have .NET support compiled in, you can build .NET libraries with the command: `make csharp`.

You can compile C# examples typing: `make csharpexe`.

To run a C# example, on windows, just type the name

```
  bin\csflow.exe
```

On unix, use the mono interpreter:

```
  mono bin/csflow.exe
```

There is a special target in the makefile to run C# examples. The above example can be run with

```
  make rcs EX=csflow
```

# Running tests #

You can check that everything is running correctly by running:
```
  make test
```

If everything is OK, it will run a selection of examples from all technologies in C++, python, java, and C#.

# Troubleshooting #

If you have trouble compiling or-tools, please write to or-tools-discuss@googlegroups.com.
I will follow up from there.

Here are some common cases:

## Cannot download pcre ##

The message is
```
svn co svn://vcs.exim.org/pcre/code/trunk -r 1336 dependencies/sources/pcre-1336 svn: E000101: Unable to connect to a repository at URL 'svn://vcs.exim.org/pcre/code/trunk' svn: E000101: Can't connect to host 'vcs.exim.org': Network is unreachable make: [dependencies/sources/pcre-1336/autogen.sh] Error 1
```

Unfortunately, pcre servers are flaky. Just retry until it works.

## Missing gflags.h ##

The message is
```
src\base/commandlineflags.h(17) : fatal error C1083: Cannot open include file: 'gflags/gflags.h': No such file or directory tools\make: [objs/alldiff_cst.obj] Error 2
```

This means that either you did not run make third\_party, or it did not succeed. Please run make third\_party and report errors.

## Error C2143 ##

Most likely, you are using visual studio 2010, please upgrade to a later version (2012 or up).

## Make test crashes on mtsearch\_test ##

I know there is a rare race condition in my code. I had not the time to debug it. Just ignore, or-tools do not use parallelism except if you specifically use some dedicated parallel local search.