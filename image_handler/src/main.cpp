#include "main.h"

// -------------------------------------------------------------------------------
// MAIN! preparing and looping through everything
int main(int argc, char * argv[]) try
{
	// default preparations like signal handler
	std::cout << setiosflags(ios::fixed) << setprecision(2);
	signal(SIGINT, sig_handler);
	

	// check cmd line arguments for e.g. result image variables
	if(argc > 3){
	
		IMAGE_WIDTH_RESULT = atoi(argv[1]);
		IMAGE_HEIGHT_RESULT = atoi(argv[2]);
		IMAGE_ROTATION_RESULT = atoi(argv[3]);
		to_node("STATUS", "parsing args");
	}

	//std::cout << "{\"STATUS\": \"starting with config: " <<  IMAGE_WIDTH_RESULT << " " << IMAGE_HEIGHT_RESULT << " " << IMAGE_ROTATION_RESULT << " \"}" << std::endl;
	

	// create camera graber and initialize it
	myCamera_grabber cam_grab;
	cam_grab.mg_init();

	// Pointer for threaded image pre prossesing like rotation and flipping...
	std::thread* prepare_rgb_image_thr;
	std::thread* prepare_depth_image_thr;
	std::thread* prepare_center_image_thr;

	// we will write with a thread to mjpeg writer, so create pointer to one
	std::thread* center_writer_thr;
	std::thread* stdin_thr = new std::thread(check_stdin);

	// get first image for setup of the mjpeg writer. It needs a image to be started.
	auto processed = cam_grab.get_cam_images();
	rs2::frame color = processed.get_color_frame(); // Find the color data
	cv::Mat rgb (COLOR_INPUT_HEIGHT, COLOR_INPUT_WIDTH, CV_8UC3, (uchar *) color.get_data());
	pre_draw_image = rgb;
	pre_draw_image.copyTo(post_draw_image);
	prepare_center_image_thr = new std::thread(prepare_center_image, std::ref(pre_draw_image));

	// Create mjpeg writer with a choosen port.
	// TODO maybe parameterize the port ???
	mjpeg_writer_ptr = new MJPEGWriter(7778);
	center_writer_thr = new std::thread(write_to_mjpeg_writer,std::ref(post_draw_image));
    mjpeg_writer_ptr->start(); //Starts the HTTP Server on the selected port

	// Create image output devices for each image that is shared
	writer_hd_image  	= new myImage_writer(gst_socket_path_hd_image, FRAMERATE, IMAGE_WIDTH_RESULT, IMAGE_HEIGHT_RESULT, true);
	writer_small_image 	= new myImage_writer(gst_socket_path_small_image, FRAMERATE, COLOR_SMALL_WIDTH, COLOR_SMALL_HEIGHT, true);
	writer_image_1m 	= new myImage_writer(gst_socket_path_image_1m, FRAMERATE, IMAGE_WIDTH_RESULT, IMAGE_HEIGHT_RESULT, true);
	//writer_depth_image 	= new myImage_writer(gst_socket_path_depth, FRAMERATE, IMAGE_WIDTH_RESULT, IMAGE_HEIGHT_RESULT, false);

	// Some GPUMats that will be used later..
	cv::cuda::GpuMat rgb_image (COLOR_INPUT_HEIGHT, COLOR_INPUT_WIDTH, CV_16U);
	cv::cuda::GpuMat rgb_scaled (COLOR_SMALL_HEIGHT,COLOR_SMALL_WIDTH,CV_16U);
	cv::cuda::GpuMat depth_image (COLOR_INPUT_HEIGHT, COLOR_INPUT_WIDTH, CV_8U);

	// Some Mats that will be used later..
	cv::Mat rgb_image_out;
	cv::Mat depth_image_out;
	cv::Mat rgb_back_image_out;
	cv::Mat small_rgb_image_out;

	// A gaussian filter used to blur the depth image a bit. this should remove artifacts and reduse noice.
	Ptr<cv::cuda::Filter> gaussian = cv::cuda::createGaussianFilter(depth_image.type(),depth_image.type(), Size(31,31),0);
	
	// And always look to the clock..
	auto now = std::chrono::system_clock::now();
	auto before = now;

	//std::cout << "{\"INIT\": \"DONE\"}" << std::endl;
	to_node("INIT", "DONE");
	//to_node("STATUS","starting with config: " + IMAGE_WIDTH_RESULT );

	// -------------------------------------------------------------------------------
	// THE MAIN LOOP STARTS HERE
	while (true){

		// get a new camera image pair. It will be aligned already
		auto processed = cam_grab.get_cam_images();

		prepare_rgb_image_thr = new std::thread(prepare_rgb_image, processed, std::ref(rgb_image));
		prepare_depth_image_thr = new std::thread(prepare_depth_image, processed, std::ref(depth_image));

		prepare_rgb_image_thr->join();
		rgb_image.download(rgb_image_out);

		prepare_depth_image_thr->join();
		depth_image.download(depth_image_out);

		// remove background from image		
		cv::cuda::GpuMat rgb_back_image = cut_background_of_rgb_image(rgb_image, depth_image, gaussian, DISTANS_TO_CROP, cam_grab.get_depth_scale());
		

		rgb_back_image.download(rgb_back_image_out);	
		cv::cuda::GpuMat background_rgb_scaled;
		cv::Mat background_rgb_scaled_out;

		// resize the image for the object and gesture detection and download it from the gpu.
		cv::cuda::resize(rgb_image, rgb_scaled, Size(COLOR_SMALL_WIDTH, COLOR_SMALL_HEIGHT),0,0, INTER_AREA);
		rgb_scaled.download(small_rgb_image_out); 

		writer_hd_image->write_image(rgb_image_out);
		writer_small_image->write_image(small_rgb_image_out);
		writer_image_1m->write_image(rgb_back_image_out);
		//writer_depth_image->write_image(depth_image_out);

		prepare_center_image_thr->join();
		center_writer_thr->join();

		pre_draw_image.copyTo(post_draw_image);
		rgb_back_image_out.copyTo(pre_draw_image);
		
		center_writer_thr = new std::thread(write_to_mjpeg_writer, std::ref(post_draw_image));
		prepare_center_image_thr = new std::thread(prepare_center_image, std::ref(pre_draw_image));
		
		// -------------------------------------------------------------------------------
		// sleep if to fast.. and print fps after 30 frames
		std::this_thread::sleep_until<std::chrono::system_clock>(before + std::chrono::milliseconds (1000/(FRAMERATE+1)));

		auto now = std::chrono::system_clock::now();
    	auto after = now;

		double curr = 1000. / std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
		framecounteracc += curr;
		before = after;
		framecounter += 1;

		if (framecounter > 30){
			std::cout << "{\"IMAGE_HANDLER_FPS\": " << (framecounteracc/framecounter) <<"}" << std::endl;
			framecounteracc = 0.0;
			framecounter = 0.0;
		}
	}

	// return if loop breaks
    return EXIT_SUCCESS;
// MAIN ENDS HERE
}
catch( const rs2::error & e )
{
       std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
       return EXIT_FAILURE;
}
catch( const std::exception & e )
{
       std::cerr << e.what() << std::endl;
       return EXIT_FAILURE;
}

// -------------------------------------------------------------------------------
// Function description below

// rotate and flip input image according to IMAGE_ROTATION_RESULT
// A flip is always needed due to being a mirror
void rotate_image(cv::cuda::GpuMat& input){
	//cv::cuda::GpuMat input_fliped;
	cv::cuda::GpuMat tmp;

	if(IMAGE_ROTATION_RESULT == 90){
		cv::cuda::flip(input, tmp,1);
		cv::cuda::rotate(tmp, input, Size(IMAGE_WIDTH_RESULT, IMAGE_HEIGHT_RESULT), 90.0, 0, COLOR_INPUT_WIDTH);
	}else if(IMAGE_ROTATION_RESULT == -90) {
		cv::cuda::flip(input, tmp,0);
		cv::cuda::rotate(tmp, input, Size(IMAGE_WIDTH_RESULT,IMAGE_HEIGHT_RESULT), 90.0, 0, COLOR_INPUT_WIDTH);
	}else {
		cv::cuda::flip(input, tmp,1);
		input = tmp;
	}
	//return output;
}

// Create color image
void prepare_rgb_image (rs2::frameset processed_frameSet,cv::cuda::GpuMat& rgb_image ){
	rs2::frame color = processed_frameSet.get_color_frame();
	cv::Mat rgb (COLOR_INPUT_HEIGHT, COLOR_INPUT_WIDTH, CV_8UC3, (uchar *) color.get_data());
	rgb_image.upload(rgb);
	rotate_image(rgb_image);
}

// Create depth image
void prepare_depth_image (rs2::frameset processed_frameSet, cv::cuda::GpuMat& depth_image){
	cv::Mat depth8u;	
	rs2::frame depth = processed_frameSet.get_depth_frame();
	cv::Mat depth16 (COLOR_INPUT_HEIGHT, COLOR_INPUT_WIDTH, CV_16U,(uchar *) depth.get_data());
	cv::convertScaleAbs(depth16,depth8u, 0.03);
	depth_image.upload(depth8u);
	rotate_image(depth_image);
}

// cut everything away that is to far away.
cv::cuda::GpuMat cut_background_of_rgb_image(cv::cuda::GpuMat& rgb, cv::cuda::GpuMat& depth,Ptr<cuda::Filter>& gaussianFilter, int distance_to_crop, float camera_depth_scale ){
	cv::cuda::GpuMat output;
	cv::cuda::GpuMat depth_tmp;
	auto thresh = double(distance_to_crop * camera_depth_scale );
	cv::cuda::threshold(depth,depth,thresh,255,THRESH_TOZERO_INV);
	gaussianFilter->apply(depth, depth_tmp);	
	cv::cuda::threshold(depth_tmp,depth,double(5),1,THRESH_BINARY);
	cv::cuda::GpuMat rgb_back_image;
	rgb.copyTo(output,depth);
	return output;
}

// Function to write in correct format
void to_node(std::string topic, std::string payload) {
    json j;
    j[topic] = payload;
    cout << j.dump() << endl;
}

// signal handler
// if sigint arrives, delete everything accordingly
void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:
        delete writer_hd_image;
		delete writer_small_image;
		delete writer_image_1m;
		//delete writer_depth_image;
		delete mjpeg_writer_ptr;
		exit(0);
    default:
        fprintf(stderr, "wasn't expecting that!\n");
        abort();
    }
}

void set_command(string setting) {
    if (setting == "TOGGLE") {
            show_camera = !show_camera;
            show_style_transfer = false;
    } else if (setting == "DISTANCE") {
        show_camera_1m = !show_camera_1m;
    } else if (setting == "FACE") {
        show_captions_face = !show_captions_face;
    } else if (setting == "OBJECT") {
        show_captions_objects = !show_captions_objects;
    } else if (setting == "GESTURE") {
        show_captions_gestures = !show_captions_gestures;
    } else if (setting == "PERSON") {
        show_captions_persons = !show_captions_persons;
    }  else if (setting == "STYLE_TRANSFERE" && is_ai_art) {
        show_camera = false;
        show_camera_1m = false;
        show_captions_face = false;
        show_captions_objects = false;
        show_captions_gestures = false;
        show_captions_persons = false;
        show_style_transfer = !show_style_transfer;
    } else if (setting == "HIDEALL") {
        show_camera = false;
        show_camera_1m = false;
        show_captions_face = false;
        show_captions_objects = false;
        show_captions_gestures = false;
        show_captions_persons = false;
        show_style_transfer = false;
    } else if (setting == "SHOWALL") {
        show_camera = true;
        show_camera_1m = false;
        show_captions_face = true;
        show_captions_objects = true;
        show_captions_gestures = true;
        show_captions_persons = true;
        show_style_transfer = false;
    }
}

string get_stdin() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void check_stdin(){
	vector<string> lines;
    while (true) {
		try{
			auto line = get_stdin();
			auto args = json::parse(line);
			//to_node("STATUS", "Got stdin line: " + args.dump());

			if (args.count("SET") == 1) {
                string setting = args["SET"];
                set_command(setting);
			} else if (args.count("DETECTED_FACES")==1) {
                json_faces.push(args["DETECTED_FACES"]);
                // to_node("STATUS", json_faces.front().dump());
            } else if (args.count("DETECTED_GESTURES")==1) {
                json_gestures.push(args["DETECTED_GESTURES"]);
                // to_node("status", json_gestures.dump());
            } else if (args.count("DETECTED_OBJECTS")==1) {
                json_objects.push(args["DETECTED_OBJECTS"]);
                // to_node("status", json_objects.dump());
            } else if (args.count("RECOGNIZED_PERSONS")==1) {
                json_persons.push(args["RECOGNIZED_PERSONS"]);
                // to_node("status", json_persons.dump());
            }

		} catch (json::exception& e)
        {
              to_node("STATUS","CPP Error: " + string(e.what()) + "; Line was ");
        }
	}
}

// Prepare the center image 
void prepare_center_image(cv::Mat& pre_draw_image){
	draw_objects(std::ref(pre_draw_image));
	draw_gestures(std::ref(pre_draw_image));
	draw_faces(std::ref(pre_draw_image));
	draw_persons(std::ref(pre_draw_image));
}

// converting x,y and w,h to a cv rect
cv::Rect convert_back(float x, float y, float w, float h){
	auto abs_w = w * IMAGE_WIDTH_RESULT;
	auto abs_h = h * IMAGE_HEIGHT_RESULT;
	auto abs_x = (x * IMAGE_WIDTH_RESULT) - (abs_w/2);
	auto abs_y = (y * IMAGE_HEIGHT_RESULT) - (abs_h/2);
    return Rect (abs_x, abs_y, abs_w, abs_h );
} 

// Draw detected objects in to referenced image.
void draw_objects(cv::Mat& pre_draw_image){
	int i = json_objects.size();
	while (i > 0){
		json_objects.pop(json_object);
		i--;
	}
	if (!json_object.is_null() && show_captions_objects){
        for (auto& element : json_object) {
            Rect rect = convert_back(element["center"][0].get<float>(), element["center"][1].get<float>(), element["w_h"][0].get<float>(), element["w_h"][1].get<float>());
            rectangle(pre_draw_image, rect, Scalar(55,255,55));
            putText(pre_draw_image, element["name"].get<std::string>() + "/ID:" + to_string(element["TrackID"].get<int>()),Point(rect.x, rect.y),FONT_HERSHEY_COMPLEX,1,Scalar(55,255,55),3);
        }
    } 
}

// Draw detected gestures in to referenced image.
void draw_gestures(cv::Mat& pre_draw_image){
	int i = json_gestures.size();
	while (i > 0){
		json_gestures.pop(json_gesture);
		i--;
	}
	if (!json_gesture.is_null() && show_captions_gestures ){
        for (auto& element : json_gesture) {
            Rect rect = convert_back(element["center"][0].get<float>(), element["center"][1].get<float>(), element["w_h"][0].get<float>(), element["w_h"][1].get<float>());
            rectangle(pre_draw_image, rect, Scalar(55,255,55));
            putText(pre_draw_image, element["name"].get<std::string>() + "/ID:" + to_string(element["TrackID"].get<int>()),Point(rect.x, rect.y),FONT_HERSHEY_COMPLEX,1,Scalar(55,255,55),3);
        }
    } 
}

// Draw detected faces in to referenced image.
void draw_faces(cv::Mat& pre_draw_image){
	int i = json_faces.size();
	while (i> 0){
		json_faces.pop(json_face);
		i--;
	}

	if (!json_face.is_null() && show_captions_face){
        for (auto& element : json_face) {
            Rect rect = convert_back(element["center"][0].get<float>(), element["center"][1].get<float>(), element["w_h"][0].get<float>(), element["w_h"][1].get<float>());
            rectangle(pre_draw_image, rect, Scalar(255,55,55));
            putText(pre_draw_image, element["name"].get<std::string>() + "/ID:" + to_string(element["TrackID"].get<int>()),Point(rect.x, rect.y),FONT_HERSHEY_COMPLEX,1,Scalar(255,55,55),3);
        }
    } 
}

// Draw persons in to referenced image.
void draw_persons(cv::Mat& pre_draw_image){
	int i = json_persons.size();
	while (i> 0){
		json_persons.pop(json_person);
		i--;
	}

	if (!json_person.is_null() && show_captions_persons){
        for (auto& element : json_person) {
            Rect rect = convert_back(element["center"][0].get<float>(), element["center"][1].get<float>(), element["w_h"][0].get<float>(), element["w_h"][1].get<float>());
            rectangle(pre_draw_image, rect, Scalar(255,255,255));
            putText(pre_draw_image, element["name"].get<std::string>() + "/ID:" + to_string(element["TrackID"].get<int>()),Point(rect.x, rect.y),FONT_HERSHEY_COMPLEX,1,Scalar(255,255,255),3);
        }
    } 
}
