# OBJS specifies which files to compile as part of the project
#OBJS = main.cpp Shader.cpp
OBJS = game.cpp Shader.cpp

# CC specifies which compiler we're using
CC = g++

# INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS = -I/usr/local/include -I/opt/X11/include -I./ -I./react/
# -I/usr/local/opt/opencv3/include

# LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = -L/usr/local/lib -I/opt/X11/lib -L./lib
# -L/usr/local/opt/opencv3/lib

# COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS = -w -std=c++11 -stdlib=libc++

# LINKER_FLAGS specifies the libraries we're linking against
# Cocoa, IOKit, and CoreVideo are needed for static GLFW3.
LINKER_FLAGS = -framework OpenGL -lglfw3 -lglew -laruco -lopencv_core -lopencv_calib3d -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_objdetect -lreactphysics3d -framework GLUT

# OBJ_NAME specifies the name of our exectuable
OBJ_NAME = main

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
