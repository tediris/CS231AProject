// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include "Shader.h"

#include <chrono>

#include "mesh_renderer.h"
#include "util/texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#include <fstream>
#include <sstream>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <aruco/aruco.h>

#include "reactphysics3d.h"

// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"

// #define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// #include "tiny_obj_loader.h"

using namespace aruco;
using namespace cv;
//using namespace std::literals::chrono_literals;

// we use a fixed timestep of 1 / (60 fps) = 16 milliseconds
constexpr std::chrono::milliseconds timestep(33);

string TheInputVideo;
string TheIntrinsicFile;
bool The3DInfoAvailable=false;
float TheMarkerSize=-1;
MarkerDetector PPDetector;
VideoCapture TheVideoCapturer;
vector<Marker> TheMarkers;
Mat TheInputImage,TheUndInputImage,TheResizedImage;
CameraParameters TheCameraParams;
Size TheGlWindowSize;
bool TheCaptureFlag=true;

bool readIntrinsicFile(string TheIntrinsicFile,Mat & TheIntriscCameraMatrix,Mat &TheDistorsionCameraParams,Size size);

bool readArguments ( int argc,char **argv )
{
	if (argc!=4) {
		cerr<<"Invalid number of arguments"<<endl;
		cerr<<"Usage: (in.avi|live)  intrinsics.yml   size "<<endl;
		return false;
	}
	TheInputVideo=argv[1];
	TheIntrinsicFile=argv[2];
	TheMarkerSize=atof(argv[3]);
	return true;
}

void vResize( GLsizei iWidth, GLsizei iHeight )
{
	TheGlWindowSize=Size(iWidth,iHeight);
	//not all sizes are allowed. OpenCv images have padding at the end of each line in these that are not aligned to 4 bytes
	if (iWidth*3%4!=0) {
		iWidth+=iWidth*3%4;//resize to avoid padding
		vResize(iWidth,TheGlWindowSize.height);
	}
	else {
		//resize the image to the size of the GL window
		if (TheUndInputImage.rows!=0)
			cv::resize(TheUndInputImage,TheResizedImage,TheGlWindowSize);
	}
}

// Function turn a cv::Mat into a texture, and return the texture ID as a GLuint for use
GLuint matToTexture(cv::Mat &mat, GLenum minFilter, GLenum magFilter, GLenum wrapFilter)
{
	// Generate a number for our textureID's unique handle
	GLuint textureID;
	glGenTextures(1, &textureID);

	// Bind to our texture handle
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Catch silly-mistake texture interpolation method for magnification
	if (magFilter == GL_LINEAR_MIPMAP_LINEAR  ||
		magFilter == GL_LINEAR_MIPMAP_NEAREST ||
		magFilter == GL_NEAREST_MIPMAP_LINEAR ||
		magFilter == GL_NEAREST_MIPMAP_NEAREST)
	{
		cout << "You can't use MIPMAPs for magnification - setting filter to GL_LINEAR" << endl;
		magFilter = GL_LINEAR;
	}

	// Set texture interpolation methods for minification and magnification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	// Set texture clamping method
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapFilter);

	// Set incoming texture format to:
	// GL_BGR       for CV_CAP_OPENNI_BGR_IMAGE,
	// GL_LUMINANCE for CV_CAP_OPENNI_DISPARITY_MAP,
	// Work out other mappings as required ( there's a list in comments in main() )
	//GLenum inputColourFormat = GL_BGR;
	GLenum inputColourFormat = GL_RGB;
	if (mat.channels() == 1)
	{
		inputColourFormat = GL_LUMINANCE;
	}

	// Create the texture
	glTexImage2D(GL_TEXTURE_2D,     // Type of texture
				 0,                 // Pyramid level (for mip-mapping) - 0 is the top level
				 GL_RGB,            // Internal colour format to convert to
				 mat.cols,          // Image width  i.e. 640 for Kinect in standard mode
				 mat.rows,          // Image height i.e. 480 for Kinect in standard mode
				 0,                 // Border width in pixels (can either be 1 or 0)
				 inputColourFormat, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
				 GL_UNSIGNED_BYTE,  // Image data type
				 mat.ptr());        // The actual image data itself

	// If we're using mipmaps then generate them. Note: This requires OpenGL 3.0 or higher
	if (minFilter == GL_LINEAR_MIPMAP_LINEAR  ||
		minFilter == GL_LINEAR_MIPMAP_NEAREST ||
		minFilter == GL_NEAREST_MIPMAP_LINEAR ||
		minFilter == GL_NEAREST_MIPMAP_NEAREST)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	return textureID;
}

void captureImage() {
	//capture image
	TheVideoCapturer.grab();
	TheVideoCapturer.retrieve( TheInputImage);
	TheUndInputImage.create(TheInputImage.size(),CV_8UC3);
	//transform color that by default is BGR to RGB because windows systems do not allow reading BGR images with opengl properly
	cv::cvtColor(TheInputImage,TheInputImage,CV_BGR2RGB);
	//remove distorion in image
	cv::undistort(TheInputImage,TheUndInputImage, TheCameraParams.CameraMatrix, TheCameraParams.Distorsion);
	//detect markers
	PPDetector.detect(TheUndInputImage,TheMarkers, TheCameraParams.CameraMatrix,Mat(),TheMarkerSize,false);
	//resize the image to the size of the GL window
	cv::resize(TheUndInputImage,TheResizedImage,TheGlWindowSize);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

GLfloat quad_vertices[] = {
        // Left bottom triangle
        -1.0f, 1.0f, 0.99f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.99f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.99f, 1.0f, 1.0f,
        // Right top triangle
        1.0f, -1.0f, 0.99f, 1.0, 1.0f,
        1.0f, 1.0f, 0.99f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.99f, 0.0f, 0.0
};

int main(int argc, char** argv) {
	if (readArguments (argc,argv)==false) return 0;
	if (TheInputVideo=="live") TheVideoCapturer.open(0);
	else TheVideoCapturer.open(TheInputVideo);
	if (!TheVideoCapturer.isOpened()) {
		cerr<<"Could not open video"<<endl;
		return -1;
	}

	//read first image
	TheVideoCapturer>>TheInputImage;
	//read camera paramters if passed
	TheCameraParams.readFromXMLFile(TheIntrinsicFile);
	TheCameraParams.resize(TheInputImage.size());

	TheGlWindowSize=TheInputImage.size();
	vResize(TheGlWindowSize.width,TheGlWindowSize.height);

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// OSX Specific compiler hint
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// create the window
	//GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
	GLFWwindow* window = glfwCreateWindow(TheInputImage.size().width, TheInputImage.size().height, "LearnOpenGL", nullptr, nullptr);
	if (window == nullptr) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	// initialize glew
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	// create the viewport, with OSX Retina support
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	Shader colorShader("coordinate.vs", "texture.frag");
	Shader cameraShader("camera.vs", "camera.frag");

	GLuint texture1, texture2;
	texture1 = loadTexture("container.jpg");
	texture2 = loadTexture("awesomeface.png");

	/////////////////////

	// create the vertex buffer
	GLuint camVBO, camVAO;
	glGenBuffers(1, &camVBO);
	//glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &camVAO);
	// bind the vertex array object
	glBindVertexArray(camVAO);

	// bind the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, camVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	// specify how the attributes should be parsed
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*) 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*) (3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// unbind the array object
	glBindVertexArray(0);

	////////////////////

	MeshRenderer renderer(&colorShader, texture1);
	//renderer.initialize(shapes[0].mesh.positions, shapes[0].mesh.texcoords, shapes[0].mesh.indices);
	renderer.initialize("b_cube.obj");
	//renderer.initialize("Deer.obj");

	// enable depth info
	glEnable(GL_DEPTH_TEST);

	using clock = std::chrono::high_resolution_clock;

	std::chrono::milliseconds lag(0);
	auto time_start = clock::now();

	// Constant physics time step
	const float pTimeStep = 1.0 / 30.0;

	/////////-------- PHYSICS ----------- //////////////
	// Gravity vector
	//rp3d::Vector3 gravity(0.0, -9.81, 0.0);
	rp3d::Vector3 gravity(0.0, 0.0, 0.0);

	// Create the dynamics world
	rp3d::DynamicsWorld world(gravity);

	// store our temporary rigid bodies
	rp3d::RigidBody* baseBody;
	rp3d::RigidBody* body;

	rp3d::Transform boxBasetransform;
	rp3d::Transform boxTransform;

	// Half extents of the box in the x, y and z directions
	rp3d::Vector3 halfExtents(0.5, 0.5, 0.5);

	// Create the box shape
	rp3d::BoxShape boxShape(halfExtents);

	// Initial position and orientation of the rigid body
	rp3d::Vector3 initPosition(0.0, 0.0, 0.5);
	rp3d::Quaternion initOrientation = rp3d::Quaternion::identity();
	//rp3d::Transform basetransform(initPosition, initOrientation);
	boxBasetransform = rp3d::Transform(initPosition, initOrientation);


	// Create a rigid body in the world
	baseBody = world.createRigidBody(boxBasetransform);
	//baseBody->setType(rp3d::STATIC);
	// Mass of the collision shape (in kilograms)
	rp3d::decimal mass = rp3d::decimal(1.0);
	rp3d::Transform ident_transform = rp3d::Transform::identity();

	baseBody->addCollisionShape(&boxShape, ident_transform, mass);

	// Create the second, dynamic body
	rp3d::Vector3 offsetPosition(0.0, 2.0, 0.5);
	boxTransform = rp3d::Transform(offsetPosition, initOrientation);

	// add a rigidbody for the other box
	body = world.createRigidBody(boxTransform);
	body->addCollisionShape(&boxShape, ident_transform, mass);

	rp3d::Vector3 testVelocity(0.0, 0.5, 0.0);
	baseBody->setLinearVelocity(testVelocity);


	rp3d::RigidBody* player1Body;
	rp3d::RigidBody* player2Body;

	rp3d::Vector3 highPos(0.0, 0.0, 5.0);
	rp3d::Transform player1Transform(highPos, initOrientation);
	rp3d::Transform player2Transform(highPos, initOrientation);

	player1Body = world.createRigidBody(player1Transform);
	player2Body = world.createRigidBody(player2Transform);
	player1Body->setType(rp3d::KINEMATIC);
	player2Body->setType(rp3d::KINEMATIC);

	player1Body->addCollisionShape(&boxShape, ident_transform, mass);
	player2Body->addCollisionShape(&boxShape, ident_transform, mass);

	///////////// ------------------- ///////////////
	glm::mat4 storedMV;

	// render loop
	while (!glfwWindowShouldClose(window)) {
		auto delta_time = clock::now() - time_start;
		time_start = clock::now();
		lag += std::chrono::duration_cast<std::chrono::milliseconds>(delta_time);
		// process events
		glfwPollEvents();

		// update game logic as lag permits
		int counter = 0;
		while(lag >= timestep) {
			lag -= timestep;
			//update(&current_state); // update at a fixed rate each time
			// Update the Dynamics world with a constant time step
			world.update(pTimeStep);
			counter++;
		}
		//cout << "Did " << counter << " updates" << endl;

		// calculate how close or far we are from the next timestep
		auto alpha = (float) lag.count() / timestep.count();

		captureImage();
		// render commands
		// clear the buffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// get the image from the camera
		GLuint camTexture = matToTexture(TheResizedImage, GL_LINEAR, GL_LINEAR, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, camTexture);
		cameraShader.Use();
		// draw the background
		glBindVertexArray(camVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// draw the cubes
		glm::mat4 view(1.0);
		glm::mat4 scale, trans_scale;
		glm::vec3 scale_vec(0.05);
		//glm::vec3 scale_vec(0.015);
		glm::vec3 trans_vec(0, 0, 0.5);
		scale = glm::scale(scale, scale_vec);

		double proj_matrix[16];
		TheCameraParams.glGetProjectionMatrix(TheInputImage.size(),TheGlWindowSize,proj_matrix,0.05,10);
		GLfloat proj_gl[16];
		glm::mat4 projection = glm::make_mat4(proj_matrix);
		double modelview_matrix[16];

		glm::vec4 origin(0.0, 0.0, 0.0, 1.0);
		glm::vec4 x_axis(1.0, 0.0, 0.0, 1.0);
		glm::vec4 y_axis(0.0, 1.0, 0.0, 1.0);
		glm::vec4 z_axis(0.0, 0.0, 1.0, 1.0);
		glm::vec4 new_origin(storedMV * origin);
		glm::vec4 new_x(storedMV * x_axis - new_origin);
		glm::vec4 new_y(storedMV * y_axis - new_origin);
		glm::vec4 new_z(storedMV * z_axis - new_origin);

		for (unsigned int m=0;m<TheMarkers.size();m++)
		{
			//cout << TheMarkers[m].id << endl;

			// TheMarkers[m].glGetModelViewMatrix(modelview_matrix);
			// glm::mat4 modelView = glm::make_mat4(modelview_matrix);
			// renderer.render(trans_scale, modelView, view, projection);
			if (TheMarkers[m].id == 3) {

				// Get the OpenGL matrix array of the transform
				rp3d::Transform retr_transform = body->getTransform();
				float trans_matrix[16];
				retr_transform.getOpenGLMatrix(trans_matrix);

				rp3d::Transform retr_base_transform = baseBody->getTransform();
				float trans_baseMatrix[16];
				retr_base_transform.getOpenGLMatrix(trans_baseMatrix);

				glm::mat4 base_trans = glm::make_mat4(trans_baseMatrix);
				//cout << glm::to_string(base_trans) << endl;
				glm::mat4 _trans = glm::make_mat4(trans_matrix);
				//cout << glm::to_string(_trans) << endl;

				TheMarkers[m].glGetModelViewMatrix(modelview_matrix);
				glm::mat4 modelView = glm::make_mat4(modelview_matrix);
				storedMV = modelView;
				renderer.render(base_trans, scale, modelView, view, projection);
				// render something with more of an offset
				renderer.render(_trans, scale, modelView, view, projection);
			} else {
				TheMarkers[m].glGetModelViewMatrix(modelview_matrix);
				glm::mat4 modelView = glm::make_mat4(modelview_matrix);
				glm::mat4 ident(1.0);
				//renderer.render(ident, scale, modelView, view, projection);
				glm::vec4 mPoint(modelView * origin - new_origin);
				glm::vec3 mProj(glm::dot(mPoint, new_x), glm::dot(mPoint, new_y), glm::dot(mPoint, new_y));
				//cout << "Marker " << TheMarkers[m].id << ": " << glm::to_string(mProj) << std::endl;
				if (TheMarkers[m].id == 0) {
					// set the box location
					rp3d::Vector3 player1Last = player1Body->getTransform().getPosition();
					rp3d::Vector3 markerPos(mProj.x / 0.05, mProj.y / 0.05, 0.5);
					rp3d::Vector3 dist = player1Last - markerPos;
					player1Transform.setPosition(markerPos);
					player1Body->setTransform(player1Transform);
					float trans_matrix[16];
					player1Transform.getOpenGLMatrix(trans_matrix);
					glm::mat4 base_trans = glm::make_mat4(trans_matrix);
					renderer.render(base_trans, scale, storedMV, view, projection);
				} else if (TheMarkers[m].id == 1) {
					rp3d::Vector3 markerPos(mProj.x / 0.05, mProj.y / 0.05, 0.5);
					player2Transform.setPosition(markerPos);
					player2Body->setTransform(player2Transform);
					float trans_matrix[16];
					player2Transform.getOpenGLMatrix(trans_matrix);
					glm::mat4 base_trans = glm::make_mat4(trans_matrix);
					renderer.render(base_trans, scale, storedMV, view, projection);
				} else {
					renderer.render(ident, scale, modelView, view, projection);
				}
			}
		}

		// swap the buffers
		glfwSwapBuffers(window);
	}
	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

bool readIntrinsicFile(string TheIntrinsicFile,Mat & TheIntriscCameraMatrix,Mat &TheDistorsionCameraParams,Size size)
{
	//open file
	ifstream InFile(TheIntrinsicFile.c_str());
	if (!InFile) return false;
	char line[1024];
	InFile.getline(line,1024);	 //skype first line that should contain only comments
	InFile.getline(line,1024);//read the line with real info

	//transfer to a proper container
	stringstream InLine;
	InLine<<line;
	//Create the matrices
	TheDistorsionCameraParams.create(4,1,CV_32FC1);
	TheIntriscCameraMatrix=Mat::eye(3,3,CV_32FC1);


	//read intrinsic matrix
	InLine>>TheIntriscCameraMatrix.at<float>(0,0);//fx
	InLine>>TheIntriscCameraMatrix.at<float>(1,1); //fy
	InLine>>TheIntriscCameraMatrix.at<float>(0,2); //cx
	InLine>>TheIntriscCameraMatrix.at<float>(1,2);//cy
	//read distorion parameters
	for(int i=0;i<4;i++) InLine>>TheDistorsionCameraParams.at<float>(i,0);

	//now, read the camera size
	float width,height;
	InLine>>width>>height;
	//resize the camera parameters to fit this image size
	float AxFactor= float(size.width)/ width;
	float AyFactor= float(size.height)/ height;
	TheIntriscCameraMatrix.at<float>(0,0)*=AxFactor;
	TheIntriscCameraMatrix.at<float>(0,2)*=AxFactor;
	TheIntriscCameraMatrix.at<float>(1,1)*=AyFactor;
	TheIntriscCameraMatrix.at<float>(1,2)*=AyFactor;

	//debug
	cout<<"fx="<<TheIntriscCameraMatrix.at<float>(0,0)<<endl;
	cout<<"fy="<<TheIntriscCameraMatrix.at<float>(1,1)<<endl;
	cout<<"cx="<<TheIntriscCameraMatrix.at<float>(0,2)<<endl;
	cout<<"cy="<<TheIntriscCameraMatrix.at<float>(1,2)<<endl;
	cout<<"k1="<<TheDistorsionCameraParams.at<float>(0,0)<<endl;
	cout<<"k2="<<TheDistorsionCameraParams.at<float>(1,0)<<endl;
	cout<<"p1="<<TheDistorsionCameraParams.at<float>(2,0)<<endl;
	cout<<"p2="<<TheDistorsionCameraParams.at<float>(3,0)<<endl;

	return true;
}
