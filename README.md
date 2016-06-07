To compile the app, you need to have a build of OpenCV installed and accessible
by the makefile (Specify its location in the INLCUDE_PATHS and LIBRARY_PATHS).
You will also need to build reactphysics3d, which is included with the repo, but
can also be fetched from github here: https://github.com/DanielChappuis/reactphysics3d/.
Finally, you need to download and build the aruco library from here:
http://www.uco.es/investiga/grupos/ava/node/26. Once everything is built,
you can just run make, and then run ./new_main (live|movie_file) camera_params_file marker_size.
To see an example, run ./new_main ar_three.move camera_params.yml 0.085.
