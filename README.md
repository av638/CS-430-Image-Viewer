# CS-430-Image-Viewer
In this project we are tasked with creating an image viewer application
that can read in either a P6 or P3 image and then do affine transformations 
on it. The image does not have to be saved after doing the transformations

In order to run this you will need the entire repo since we need gles2 and so on,
once that is all grabed just run nmake in the visual studio command prompt. 
Finally just type the name of the output exe as well as the input ppm file.

Ex. main work.ppm


Finally in order to do the affine transformations you must use these keys:


// Escape is quit

// Q is turn image left 90 degrees

// E is turn image right 90 degrees

// W is shear image up

// S is shear image down

// A is shear image left

// D is shear image right

// - is zoom out

// = is zoom in

// up arrow is move image up

// down arrow is move image down

// left arrow is move image left

// right arrow is move image right
