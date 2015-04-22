/*
 * ThermAppRos.cpp
 *
 *  Created on: Apr 18, 2015
 *      Author: cyborg-x1
 */

#include "ThermAppRos.h"
#include <libusb-1.0/libusb.h>
#include <sensor_msgs/image_encodings.h>


namespace ThermApp {

ThermAppRos::ThermAppRos()
 :n(),
 nh("~"),
 it(n)
{
	int a;
	n.param("narf",a,1);
	pub_cam_thermal=it.advertiseCamera("thermapp_camera/image", 1);
	reconfCbType = boost::bind(&ThermApp::ThermAppRos::reconfigCb, this, _1, _2);
	reconfServer.setCallback(reconfCbType);

	pub_debug_image=it.advertise("debugimage",1,1);
}

ThermAppRos::~ThermAppRos() {
}


void ThermAppRos::reconfigCb(thermapp_camera::thermapp_camera_nodeConfig &config, uint32_t level)
{
	reconf_mutex.lock();
	int temp_1=config.temp_1;
	int temp_2=config.temp_2;
	reconf_mutex.unlock();
}

void ThermAppRos::run()
{

	cv_bridge::CvImage out_msg;

//	ThermApp::ThermAppCameraBulk usb_bulk;
//	if(usb_bulk.openDevice())
//	{
//		ROS_ERROR("Could not open device!");
//		return;
//	}



	libusb_context *context=NULL;
    libusb_device **list=NULL;
    libusb_device_handle *handle=NULL;
    libusb_init(&context);
    libusb_set_debug(context,3);


    ssize_t count = libusb_get_device_list(context, &list);


    for (size_t idx = 0; idx < count; ++idx)
    {
        libusb_device *device = list[idx];
        struct libusb_device_descriptor desc = {0};



        libusb_get_device_descriptor(device, &desc);

        if(desc.idVendor==0x1772 && desc.idProduct==0x0002)
        {

            libusb_open(device,&handle);
            libusb_claim_interface(handle,0);
        }
    }


            unsigned char ENDPOINT_DOWN = 0x81;
			unsigned char ENDPOINT_UP = 0x02;
			int actual_length;
			unsigned char dataUp[64];
			unsigned char dataDown[61440];

			memset(dataDown,0, sizeof(dataDown));
			memset(dataUp,0, sizeof(dataUp));





			unsigned char config2[64] =
			{
				0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xd5, 0xa5,	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x20, 0x01, 0x80, 0x01, 0x20, 0x01,	0x80, 0x01, 0x19, 0x00,	0x00, 0x00, 0x00, 0x00,
				0xae, 0x07, 0x00, 0x00, 0x8f, 0x05, 0xa2, 0x08,	0x6d, 0x0b, 0x85, 0x0b,	0x00, 0x00, 0x00, 0x00,
				0x98, 0x09, 0x40, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0xff, 0x0f
			};

			dataDown[0]=1;
			libusb_bulk_transfer(handle, ENDPOINT_DOWN, dataDown,
										0, &actual_length, 10);




			bool headerwait=true;
			bool imagestarted=false;
			unsigned byteno=0;
			std::vector <unsigned char> imvec;















			typedef
			enum
			{
				HEADER_COMPARE,
				SKIP,
				IMAGE,
			}state_t;


			state_t state=HEADER_COMPARE;
			std::vector<unsigned char> header;
			unsigned int compare=9;
			unsigned int compared=0;
			unsigned int skip=64;
			unsigned int skipped;


			cv::Mat color;
			color.create(288,384, CV_8UC3);
			std::vector<uint8_t> im_data;

			unsigned char config[64] =
			{
				0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xd5, 0xa5,
				0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00,

				/*Resolution twice ->*/0x20, 0x01, 0x80, 0x01,/**/ 0x20, 0x01,	0x80, 0x01,/**/

				0x19, 0x00,	0x00, 0x00, 0x00, 0x00, 0xae, 0x07, 0x00, 0x00,
				/*Range ->*/0x8f, 0x05, 0xa2, 0x08,	0x6d, 0x0b, 0x85, 0x0b,	0x00, 0x00, 0x00, 0x00,
				0x98, 0x09, 0x40, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0xff, 0x0f
			};
			ros::Time current_start=ros::Time::now();
			ros::Duration loop_time(0,1000000/8.7);
			while(ros::ok())
			{




					actual_length=0;

					libusb_bulk_transfer(handle, ENDPOINT_UP, config,
							64, &actual_length, 1);
					actual_length=0;

					ros::Duration d=ros::Time::now()-current_start;
					//ROS_INFO_STREAM(d.sec<<"--"<<d.nsec);
					if(d>=loop_time)
					{
						current_start=ros::Time::now();
						int a = libusb_bulk_transfer(handle, ENDPOINT_DOWN, dataDown,
								sizeof(dataDown), &actual_length, 120);


					std::vector<uint8_t> usb_data(dataDown, dataDown+actual_length);

					for(std::vector<uint8_t>::iterator it=usb_data.begin(); it!=usb_data.end();it++)
					{
						if(*it==config[0])
						{
							int h=0;
							for(std::vector<uint8_t>::iterator it2=it; it2!=usb_data.end();it2++)
							{

								if(*it2==config[h])
								{
									h++;
								}
								else
								{
									break;
								}

								if(h==9)
								{;
									h=0;
									im_data.clear();
								}

							}
						}


						im_data.push_back(*it);


						if(im_data.size()==384*288*2)
						{

							cv::Mat myuv(288,384, CV_16UC1, im_data.data());


							myuv*=20;


							for (int row = 0; row < 288; ++row)
							{
								for (int col = 0; col < 384; ++col)
								{

									color.at<cv::Vec3b>(row,col)[0]=myuv.at<unsigned short>(row,col)/100;
									color.at<cv::Vec3b>(row,col)[1]=255;
									color.at<cv::Vec3b>(row,col)[2]=255;

									if(row==288*3/4/2 && col==384/2)
									{
										unsigned short value=myuv.at<unsigned short>(row,col);
										//value=((value&0xFF)<<8) | ((value&0xFF00)>>8);


										printf("%0.2X -> %i\n",value, value);
									}

								}
							}

							cv::cvtColor(color,color,CV_HSV2RGB);

							color.at<cv::Vec3b>(288/2,384/2)[0]=0;
							color.at<cv::Vec3b>(288/2,384/2)[1]=0;
							color.at<cv::Vec3b>(288/2,384/2)[2]=0;
							cv::circle(color,cv::Point(288/4,384/2),10,cv::Scalar(255,0,0));

							im_data.clear();


							cv_bridge::CvImagePtr imgPtrThermal, imgPtrRGB;


							out_msg.header.seq++;
							out_msg.header.stamp=ros::Time::now();
							out_msg.header.frame_id="therm_app";
							out_msg.header.seq++;// Same timestamp and tf frame as input image
							out_msg.encoding = sensor_msgs::image_encodings::BGR8; // Or whatever
							out_msg.image    = color; // Your cv::Mat

							ROS_INFO("PUBLISH!");
							pub_debug_image.publish(out_msg.toImageMsg());

							std::cout<<im_data.size()<<std::endl;

							ros::spinOnce();
						}


					}//FOR usb_data
					}





			}//WHILE



			return;
//	while(ros::ok())
//	{
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//		cv::Mat img_rgb;
//		cv::Mat img_thermal;
//
//
//
//		usb_bulk.streamer();
//
////		sensor_msgs::Image image;
////		sensor_msgs::CameraInfo info;
//
//		//img_thermal;
//
//
//
//		try
//		{
//			//imgPtrRGB->image = img_rgb;//cv_bridge::toCvCopy(depth_msg, "16UC1");
//			//imgPtrThermal->image = img_thermal;// cv_bridge::toCvCopy(rgb_msg, "bgr8");
//		} catch (cv_bridge::Exception& e)
//		{
//			ROS_ERROR("cv_bridge exception: %s", e.what());
//			return;
//		}
//
//		//sensor_msgs::CameraInfo info;
//
//		ROS_INFO("Spin!");
//		//pub_debug_image.publish(imgPtrRGB->toImageMsg());
//		//pub_image.publish(img_rgb,,ros::now());
//		ros::spinOnce();
//		loop_rate.sleep();
//	}
//	//usb_bulk.closeDevice();
//

}


} /* namespace ThermApp */

