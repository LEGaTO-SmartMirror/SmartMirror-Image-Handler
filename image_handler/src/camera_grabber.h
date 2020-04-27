#ifndef MYCAMERA_H
#define MYCAMERA_H


#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

class myCamera_grabber {
    public:
        myCamera_grabber(){};
        ~myCamera_grabber();
        void mg_init();
        rs2::frameset get_cam_images();
        float get_depth_scale(){return depth_scale;};


    private:
        int  COLOR_INPUT_WIDTH      = 1920;
        int  COLOR_INPUT_HEIGHT     = 1080;
        int  DEPTH_INPUT_WIDTH      = 1280;
        int  DEPTH_INPUT_HEIGHT     = 720;
        int  FRAMERATE              = 30;
        int  COLOR_SMALL_WIDTH      = 416;
        int  COLOR_SMALL_HEIGHT     = 416;

        int IMAGE_WIDTH_RESULT      = 1080;
        int IMAGE_HEIGHT_RESULT     = 1920;
        int IMAGE_ROTATION_RESULT   = 90;

        rs2::pipeline pipe;
        rs2::config cfg;
        rs2_stream align_to; 
        rs2::align* align;
        float depth_scale;


        rs2_stream find_stream_to_align(const std::vector<rs2::stream_profile>& streams);
        float get_depth_scale(rs2::device dev);

};

#endif