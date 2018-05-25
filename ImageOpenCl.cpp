#include <fstream>
#include <iostream>
#include <vector>
#include <cstdio>
#include <OpenCL/opencl.h>
#include <FreeImagePlus.h>
// #include <opencv2/opencv.hpp>

void cl_error( const char * msg, cl_int s ) {
	fprintf( stderr, "%s: %d\n", msg, s );
	exit( -1 );
}

// const cv::Mat LoadImage( const char * path ) {
// 	cv::Mat image;
// 	image = cv::imread( path, CV_LOAD_IMAGE_COLOR );
// 	return image;
// }

std::string LoadKernel( const char * name ) {
	std::ifstream in( name );
	std::string result(
		( std::istreambuf_iterator<char>( in )),
		std::istreambuf_iterator<char>()
	);
	return result;
}

cl_program CreateProgram( const std::string & source, cl_context context ) {
	size_t lengths[ 1 ] = { source.size() };
	const char * sources[ 1 ] = { source.data() };
	cl_int status;
	cl_program program = clCreateProgramWithSource( context, 1, sources, lengths, &status );
	if ( status != CL_SUCCESS ) cl_error( "clCreateProgramWithSource()", status );

	return program;
}

int main ( int argc, char ** argv ) {

	/* Utility */
	cl_int status;
	char buffer[ BUFSIZ ];

	cl_uint platformIDCount = 0;
	status = clGetPlatformIDs( 0, nullptr, &platformIDCount );
	if ( status != CL_SUCCESS ) cl_error( "clGetPlatformIDs()", status );

	std::vector<cl_platform_id> platforms(platformIDCount);
	status = clGetPlatformIDs( platformIDCount, platforms.data(), nullptr );
	if ( status != CL_SUCCESS ) cl_error( "clGetPlatformIDs()", status );

	std::memset( buffer, '\0', BUFSIZ );
	std::cout << "platformIDCount: " << platformIDCount << std::endl;
	for ( const cl_platform_id platform : platforms ) {
		status = clGetPlatformInfo( platform, CL_PLATFORM_NAME, BUFSIZ, buffer, nullptr );
		if ( status != CL_SUCCESS ) cl_error( "clGetPlatformInfo()", status );
		std::cout << "Platform Name: " << buffer << std::endl;
	}

	cl_device_id device;
	cl_uint deviceIDCount = 0;
	status = clGetDeviceIDs( platforms[ 0 ], CL_DEVICE_TYPE_ALL, 0, nullptr, &deviceIDCount );
	if ( status != CL_SUCCESS ) cl_error( "clGetDeviceIDs()", status );

	std::vector<cl_device_id> devices( deviceIDCount );
	status = clGetDeviceIDs( platforms[ 0 ], CL_DEVICE_TYPE_ALL, deviceIDCount, devices.data(), nullptr );
	if ( status != CL_SUCCESS ) cl_error( "clGetDeviceIDs()", status );

	std::memset( buffer, '\0', BUFSIZ );
	std::cout << "DeviceCount: " << deviceIDCount << std::endl;
	for ( const cl_device_id device : devices ) {
		status = clGetDeviceInfo( device, CL_DEVICE_PROFILE, BUFSIZ, buffer, nullptr );
		if ( status != CL_SUCCESS ) cl_error( "clGetDeviceInfo()", status );
		std::cout << "Device Profile: " << buffer << std::endl;
	}

	const cl_context_properties contextProperties[] = {
		CL_CONTEXT_PLATFORM,
		reinterpret_cast<cl_context_properties>( platforms[ 0 ] ), 0, 0
	};

	cl_context context = clCreateContext( contextProperties, deviceIDCount, devices.data(), nullptr, nullptr, &status );
	if ( status != CL_SUCCESS ) cl_error( "clCreateContext()", status ); 

	cl_program program = CreateProgram( LoadKernel( "kernels/filter.cl" ));
	status = clBuildProgram( program, deviceIDCount, devices.data(), "-D FILTER_SIZE=1", nullptr, nullptr );
	if ( status != CL_SUCCESS ) cl_error( "clBuildProgram()", status ); 

	cl_kernel kernel = clCreateKernel( program, "Filter", &status );
	if ( status != CL_SUCCESS ) cl_error( "clCreateKernel()", status ); 


	/* Image Preparation */
	// if ( argc != 2 ) exit( 1 );
	// const cv::Mat image = LoadImage( argv[ 1 ] );
	// const cv::Size size = image.size();
	// const unsigned height = size.height;
	// const unsigned width = size.width;

	// Simple Gaussian blur filter
	float filter [] = {
		1, 2, 1,
		2, 4, 2,
		1, 2, 1
	};

	// Normalize the filter
	for (int i = 0; i < 9; ++i) {
		filter [i] /= 16.0f;
	}

	fipImage img;
	if ( argc != 2 ) exit( 1 );
	img.load( argv[ 1 ] );
	const unsigned width = img.getWidth();
	const unsigned height = img.getHeight();
	char * image = new char[ width * height * 4 ]; // RGBA
	std::memcpy( image, FreeImage_GetBits( img ), width * height * 4 );
	FreeImage_Unload(img);

	static const cl_image_format format = { CL_RGBA, CL_UNORM_INT8 };

	cl_mem inputImage = clCreateImage2D(
		context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&format,
		width,
		height,
		0,
		image,
		&status
	);
	if ( status != CL_SUCCESS ) cl_error( "clCreateImage2D(){A}", status ); 

	cl_mem outputImage = clCreateImage2D(
		context,
		CL_MEM_WRITE_ONLY,
		&format,
		width,
		height,
		0,
		image,
		&status
	);
	if ( status != CL_SUCCESS ) cl_error( "clCreateImage2D(){B}", status ); 

	cl_mem filterWeightsBuffer = clCreateBuffer(
		context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(float) * 9,
		)

	status = clEnqueueNDRangeKernel( queue, kernel )

	return 0;
}