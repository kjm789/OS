# Operating System's
Implementations of common algorithms used in a Unix-like Operating System.

To be able to run these programs, you will need a Unix-like operating system. You will also need to have installed CMake package.
	
To install CMake, open up a terminal on your local machine and type the command: 
	
				sudo apt-get install cmake

Also, to run these programs, you will need to build the OS library such that you can dynamic link this library to cache shared libraries on your system.
	
To do this, please clone the OS repository where you will change directories into the lib directory. In the lib directory, please follow the sequence of commands:

				mkdir build 		[press enter]
				cd build			[press enter]
				cmake ..			[press enter]
				sudo make install	[press enter]
				sudo ldconfig		[press enter]

After you have completed this, you are all set to begin running programs found in this repository. If you have any problems/errors with these commands above, feel free to shoot me an email: koboldtmichael@gmail.com



	

