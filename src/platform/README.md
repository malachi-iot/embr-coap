This folder includes source code which may or may not be pertinent 
to the platform which you are compiling

In essence these are modular platform-specific features which must be 
manually enabled

Presently these are all included in core lib CMakeLists.txt but that
may not be the case in the future.  Undecided whether we want to:

a) use compiler switches to enable those platform features
b) demand consuming programmer (you) to manually find and include CPP files in your project

Leaning towards a, but we will see

# NOTE

Platform probably should be named 'modules' , will likely be renamed as such

Also much of this platform code relies on netbuf mk. 1 which is obsolete and is not to be used at all.