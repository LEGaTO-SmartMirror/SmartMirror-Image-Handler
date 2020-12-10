#include "image_appsink_writer.h"

#include <stdio.h>
#include <iostream>

myImage_writer::myImage_writer(std::string socket_path, int FRAMERATE, int image_width, int image_height, bool with_color, long buffersize){

    writer_socket_path = socket_path;

    remove(writer_socket_path.c_str());

    std::string gst_str = "appsrc ! shmsink socket-path=";
    gst_str.append(writer_socket_path);
    gst_str.append(" sync=false wait-for-connection=false shm-size=");// 100000000");
    gst_str.append(std::to_string(buffersize));
    gst_str.append(" buffer-time=100");
    std::cout << "{\"STATUS\": \"creating shared memory object \"}" << std::endl;
    out = cv::VideoWriter(gst_str ,0 , FRAMERATE, cv::Size(image_width, image_height), with_color);

};

myImage_writer::~myImage_writer(){

    std::cout << "{\"warning\": \"destroying pipe! \"}" << std::endl;    
    out.release();
    //remove(writer_socket_path.c_str());
}


void myImage_writer::write_image(cv::Mat Image){

    out.write(Image);
};
