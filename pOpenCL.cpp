#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <vector>
// #include <OpenCL/cl.h>
// #include <OpenCL/cl_platform.h>
#include <fstream>
#include <OpenCL/opencl.h>

static const char * program;

void cl_error(const char * msg, cl_int s) {
	fprintf(stderr, "%s: %s: %d\n", program, msg, s);
	exit(-1);
}

/* Opens Kernel File and reads contents into string */

std::string LoadKernel (const char* name) {
	std::ifstream in (name);
	std::string result(
		(std::istreambuf_iterator<char> (in)),
		std::istreambuf_iterator<char> ());
	return result;
}

/* Create a program object with a single kernel */

cl_program CreateProgram (const std::string& source, cl_context context) {
	size_t lengths[] = { source.size() };		/* Size of Kernel */
	const char* sources[] = { source.data() }; 	/* Kernel Source */

	cl_int status = 0;

	/* Create Program OBJECT(not yet compiled) for context(platforms & devices we're using) using loaded source code */ 

	cl_program program = clCreateProgramWithSource (context, 1, sources, lengths, &status);
	if ( status != CL_SUCCESS ) cl_error( "clCreateProgramWithSource()", status ); 

	return program;
}

int main(int argc, char ** argv) {
	cl_int status;
	cl_device_id device;

	/* Get number of platforms */
	cl_uint platform_id_count = 0; 
	status = clGetPlatformIDs( 0, nullptr, &platform_id_count );
	if ( status != CL_SUCCESS ) cl_error( "clGetPlatformIDs()", status ); 

	/* Get list of platforms */
	std::vector<cl_platform_id> platformIds(platform_id_count);
	status = clGetPlatformIDs( platform_id_count, platformIds.data(), nullptr );
	if ( status != CL_SUCCESS ) cl_error( "clGetPlatformIDs()", status ); 


	/* Get device count from platforms (just the first platform we find in this case) */
	cl_uint device_id_count = 0;
	status = clGetDeviceIDs( platformIds[ 0 ], CL_DEVICE_TYPE_ALL, 0, nullptr, &device_id_count);
	if ( status != CL_SUCCESS ) cl_error( "clGetDeviceIDs()", status ); 

	/* Get list of devices from first platform */
	std::vector<cl_device_id> deviceIds(device_id_count);
	status = clGetDeviceIDs(platformIds[ 0 ], CL_DEVICE_TYPE_ALL, device_id_count, deviceIds.data(), nullptr);
	if ( status != CL_SUCCESS ) cl_error( "clGetDeviceIDs()", status );

	/* Test Data */
	size_t testDataSize = 1 << 10;
	std::vector<float> a(testDataSize), b(testDataSize);

	for ( int i = 0; i < testDataSize; i++ ) {
		a[ i ] = static_cast<float>( 23 ^ i );
		b[ i ] = static_cast<float>( 19 ^ i );
	}

	/* Context properties

	Context Properties is a list of property names followed by their corresponding values. The list is terminated with 0.
	Can be null which just means the platform used is implementation defined. Here we use the first platform.

	*/

	const cl_context_properties contextProperties[] = {
		CL_CONTEXT_PLATFORM,
		reinterpret_cast<cl_context_properties>(platformIds[ 0 ]),
		0,
		0
	};

	/* Create Context - Platform we're using(set in context properties) and devices */
	cl_context context = clCreateContext( contextProperties, device_id_count, deviceIds.data(), nullptr, nullptr, &status );
	if ( status != CL_SUCCESS ) cl_error( "clCreateContext()", status ); 

	/* Create Program */

	/* Encapsulates:
	1) Kernel source code ie. a function in the program. Here, the kernel is loaded into a string with LoadKernel.
	2) Context - Created above.
	3) Platform and Device Target. Encapsulated by the context.

	*/

	/* Create Program Object using source and context */

	cl_program program = CreateProgram(LoadKernel("kernels/saxpy.cl"), context);

	/* Create command Queue - commands are passed via this queue to the device running the kernel. Commands include:
	1) Kernel Executions, 2) Memory Object Management, 3) Synchronization */

	cl_command_queue queue = clCreateCommandQueue(context, deviceIds[ 0 ], 0, &status);
	if ( status != CL_SUCCESS ) cl_error( "clCreateCommandQueue()", status );

	/* Compile and link program object */

	status = clBuildProgram(program, device_id_count, deviceIds.data(), nullptr, nullptr, nullptr);
	if ( status != CL_SUCCESS ) cl_error( "clBuildStatus()", status ); 

	/* Create Kernel object with compiled program and specify function with __kernel specifier */

	cl_kernel kernel = clCreateKernel( program, "SAXPY", &status );
	if ( status != CL_SUCCESS ) cl_error( "clCreateKernel()", status ); 

	/* Create Buffers for Kernel - for Arguments and for returned calculations */

	/* Contains argument values, and will be used to store final calculations */
	cl_mem aBuffer = clCreateBuffer( context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * testDataSize, a.data(), &status );
	if ( status != CL_SUCCESS ) cl_error( "clCreateBuffer()", status ); 

	/* Contains only argument values */
	cl_mem bBuffer = clCreateBuffer( context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * testDataSize, a.data(), &status );
	if ( status != CL_SUCCESS ) cl_error( "clCreateBuffer()", status );

	/* Set Kernel Arguments

	SAXPY -> aBuffer[i] = 2*bBuffer[i] + aBuffer[i]

	*/

	clSetKernelArg( kernel, 0, sizeof(cl_mem), &aBuffer );
	clSetKernelArg( kernel, 1, sizeof(cl_mem), &bBuffer );
	static const float two = 2.0f;
	clSetKernelArg( kernel, 2, sizeof(cl_mem), &two );

	const size_t globalWorkSize[] = { testDataSize, 0, 0 };
	status = clEnqueueNDRangeKernel( queue, kernel, 1, nullptr, globalWorkSize, nullptr, 0, nullptr, nullptr );
	if ( status != CL_SUCCESS ) cl_error( "clEnqueueNDRangeKernel()", status ); 

	clReleaseCommandQueue(queue);
	clReleaseMemObject(bBuffer);
	clReleaseMemObject(aBuffer);

	clReleaseKernel(kernel);
	clReleaseProgram(program);

	clReleaseContext(context);

	return 0;
}