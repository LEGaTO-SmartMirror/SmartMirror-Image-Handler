#include "camera_grabber.h"
#include <stdio.h>
#include <iostream>

using namespace std;

void myCamera_grabber::mg_init(){

    std::cout << "{\"STATUS\": \"my_camera_grabber is starting to initialise \"}" << std::endl;

    cfg.enable_stream(RS2_STREAM_DEPTH, DEPTH_INPUT_WIDTH, DEPTH_INPUT_HEIGHT, RS2_FORMAT_Z16, FRAMERATE);
	cfg.enable_stream(RS2_STREAM_COLOR, COLOR_INPUT_WIDTH, COLOR_INPUT_HEIGHT, RS2_FORMAT_BGR8, FRAMERATE);

    rs2::pipeline_profile profile = pipe.start(cfg);

    // Each depth camera might have different units for depth pixels, so we get it here
	// Using the pipeline's profile, we can retrieve the device that the pipeline uses
	depth_scale = get_depth_scale(profile.get_device());

	//std::cout << ("depth scale = %d",depth_scale) << std::endl;

	//Pipeline could choose a device that does not have a color stream
	//If there is no color stream, choose to align depth to another stream
	align_to = find_stream_to_align(profile.get_streams());

	// Create a rs2::align object.
	// rs2::align allows us to perform alignment of depth frames to others frames
	//The "align_to" is the stream type to which we plan to align depth frames.
	align = new rs2::align(align_to);

	// Define a variable for controlling the distance to clip
	float depth_clipping_distance = 1.f;

}

myCamera_grabber::~myCamera_grabber(){

delete align;

}

rs2::frameset myCamera_grabber::get_cam_images(){

    // Using the align object, we block the application until a frameset is available
	rs2::frameset frameset = pipe.wait_for_frames();

	//Get processed aligned frame
	auto processed = align->process(frameset);
    return processed;

}

rs2_stream myCamera_grabber::find_stream_to_align(const std::vector<rs2::stream_profile>& streams)
{
    //Given a vector of streams, we try to find a depth stream and another stream to align depth with.
    //We prioritize color streams to make the view look better.
    //If color is not available, we take another stream that (other than depth)
    rs2_stream align_to = RS2_STREAM_ANY;
    bool depth_stream_found = false;
    bool color_stream_found = false;
    for (rs2::stream_profile sp : streams)
    {
        rs2_stream profile_stream = sp.stream_type();
        if (profile_stream != RS2_STREAM_DEPTH)
        {
            if (!color_stream_found)         //Prefer color
                align_to = profile_stream;

            if (profile_stream == RS2_STREAM_COLOR)
            {
                color_stream_found = true;
            }
        }
        else
        {
            depth_stream_found = true;
        }
    }

    if(!depth_stream_found)
        throw std::runtime_error("No Depth stream available");

    if (align_to == RS2_STREAM_ANY)
        throw std::runtime_error("No stream found to align with Depth");

    return align_to;
}

float myCamera_grabber::get_depth_scale(rs2::device dev)
{
    // Go over the device's sensors
    for (rs2::sensor& sensor : dev.query_sensors())
    {
        // Check if the sensor if a depth sensor
        if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
        {
            return dpt.get_depth_scale();
        }
    }
    throw std::runtime_error("Device does not have a depth sensor");
}