#ifndef MYIMAGE_WRITER_H
#define MYIMAGE_WRITER_H

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>



class myImage_writer {
    public:
        myImage_writer(std::string socket_path, int FRAMERATE, int image_width, int image_height,bool with_color);
        ~myImage_writer();
        void write_image(cv::Mat Image);


    private:
        cv::VideoWriter out;
        std::string writer_socket_path;

};

#endif