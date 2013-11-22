What is memory-dumper
=============

memory-dumper is a tool for dumping files from process's memory.
The main purpose is to find patterns inside the process's memory,
which is done by plugins, and dump segments of memory to files.

Why would I use memory-dumper
=============

Virtually memory-dumper can dump anything, it's up to you find it
any use. That said, I use it to dump Flash files (```SWF```). There are
many ```SWF``` encripted files that can't be decrypted easily. The only
easy way is make them decrypt themself and them dump them directly
from memory.

New plugins for dumping any other type of data can be created
easily.

Ok, I'd like to dump ```XYZ```
=============

You just need to create a plugin! It's that easy. Just look inside
the plugin folder. Your plugin should have two main functions.
The first one is ```init``` which will be used to init the plugin
itself and pass it some useful function; and the second one is ```match```,
which is used to pass a memory block to the plugin so it can search
and dump it's content.

TO-DO:
=============

* Currently memory-dumper works only on Linux. Maybe I'll port it to 
Windows at some point in the future, but I don't want to promise 
anything. Anyways, I'll accept a patch for this :)

* I'm planning to write some more plugins. Probably a dumper for ```PDF```
files. If you want a plugin for some specific data, use the ```New issue```
button :)
