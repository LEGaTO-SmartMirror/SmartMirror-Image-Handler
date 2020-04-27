#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <signal.h>
#include "camera_grabber.h"
#include "image_appsink_writer.h"
#include "MJPEGWriter.h"
#include <nlohmann/json.hpp>
#include "SafeQueue.h"

// for convenience
using json = nlohmann::json;

using namespace std;
using namespace rs2;
using namespace cv;

// Camera input attributes
int const COLOR_INPUT_WIDTH      = 1920;
int const COLOR_INPUT_HEIGHT     = 1080;
int const DEPTH_INPUT_WIDTH      = 1280;
int const DEPTH_INPUT_HEIGHT     = 720;
int const FRAMERATE       	 	 = 30;

// Output attribute defaults, will be overwritten by cmd line arguments
int IMAGE_WIDTH_RESULT 			= 1080;
int IMAGE_HEIGHT_RESULT 		= 1920;
int IMAGE_ROTATION_RESULT 		= 90;
int COLOR_SMALL_WIDTH     		= 416;
int COLOR_SMALL_HEIGHT    		= 416;

// image processing variables
const int  DISTANS_TO_CROP = 60000; //60000;

// Output declarations.
// where are the shm objects and pointer to the handler objects.
string gst_socket_path_hd_image = "/dev/shm/camera_image";
string gst_socket_path_small_image = "/dev/shm/camera_small";
string gst_socket_path_image_1m = "/dev/shm/camera_1m";
//string gst_socket_path_depth = "/dev/shm/camera_depth";

myImage_writer* writer_hd_image;
myImage_writer* writer_small_image;
myImage_writer* writer_image_1m;
//myImage_writer* writer_depth_image;

// For the visualisation on the mirror, a mjpeg writer is used
// define pointer to handler, and a function to write in a thread.
MJPEGWriter* mjpeg_writer_ptr;

void write_to_mjpeg_writer(cv::Mat& rgb_image_out){
	mjpeg_writer_ptr->write(rgb_image_out);
};

cv::Mat pre_draw_image;
cv::Mat post_draw_image;


// bools what is visible
bool is_ai_art = false;
bool show_captions_face = false;
bool show_captions_objects = false;
bool show_captions_gestures = false;
bool show_captions_persons = false;
bool show_camera = false;
bool show_camera_1m = false;
bool show_style_transfer = false;

// the actual json which will be drawn
json json_face;
json json_gesture;
json json_object;
json json_person;

// Queue for drawing of the detection or person infos.
SafeQueue<json> json_faces;
SafeQueue<json> json_gestures;
SafeQueue<json> json_objects;
SafeQueue<json> json_persons;

// default stuff, like kill sig handler, variables for fps counter or function headers (defined below the main)
double framecounteracc = 0.0;
double framecounter = 0.0;

// Function headers
void sig_handler(int sig);
void rotate_image(cv::cuda::GpuMat& input);
void prepare_rgb_image (rs2::frameset processed_frameSet, cv::cuda::GpuMat& rgb_image );
void prepare_depth_image (rs2::frameset processed_frameSet, cv::cuda::GpuMat& depth_image);
cv::cuda::GpuMat cut_background_of_rgb_image(cv::cuda::GpuMat& rgb, cv::cuda::GpuMat& depth,Ptr<cuda::Filter>& gaussianFilter, int distance_to_crop, float camera_depth_scale );
void check_stdin();
void to_node(std::string topic, std::string payload);
void prepare_center_image(cv::Mat& pre_draw_image);
void draw_objects(cv::Mat& pre_draw_image);
void draw_gestures(cv::Mat& pre_draw_image);
void draw_faces(cv::Mat& pre_draw_image);
void draw_persons(cv::Mat& pre_draw_image);
int main(int argc, char * argv[]);