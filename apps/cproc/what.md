

We need to define a height, width, and depth of environment. we are focusing on image processing
but we are representing each segment of (RGBA GBRA etc) is a plane of depth. Our processing doesnt
concern itself with WHAT the data is, just its range and configured meaning.

We load the "source" either from and image -> data or static random data. then, we define a cell size.
the cell is a segment of the screen, a discrete ratio, and we define how to handle offset/boundaries if
not perfectly divisible (overhang vs wrap vs truncate :: overhang woudl ahve aspects of the cell not visible, wrap would - like line wrap start the cell on the "next line", and truncate would leave "dead space" unused)




----


I want to use tcc as a scripting language executor. what im thinking is that we can define, in the engine that is installed on the users machine, a c header to simpley / quickly work with input arguments in c.

if we do this, then we can define something like "cell.c" where the int argc, char **argc is the raw
data present "in" the cell when its instantiated. if we maintain state in a data structure and ALSO
pass that structure as an argument into the program then we can write the cell logic that occurs
every "tick" where that cells state is handed along with environemtn values (block of pixel data) and
the other env state information, then it can return a list of the changes (rather than the whole block)

we then make a 3rd "overlay" on the image/env. this 3d overlay encompases groups of cells. this overlay is the thread overaly. we use the kernel thread pool to have 1 pool per grid space, and then N cells (configurable) per thread exec cycle.

Each "thread cycle" we call tcc to exec cell.c with the given infromation and update the target env data.

we can then have a thread reading the data being worked on and we can visualize the transformations in
something like SDL. So, if we had an image a, a cell would be a group of pixels, and each main.c would view it and maniouaklte it as a 3 dimensional array of uint8_t. we HAPPEN to be able to do this with images, its not he purpose neceissarily. we could ahve it be 4 dimensional, and conventioanlly map RGBA to this, but the engine is meant to take any HxWxD "source" and manipulate it with cells on the threads etc
