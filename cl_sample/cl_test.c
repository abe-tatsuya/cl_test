#include "erl_nif.h"

#include <stdio.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif //__APPLE__

#define MAX_SOURCE_SIZE 0x100000

const int fail = 0;
const int success = 1;

//enif_get_int_vec_from_list is function that get int vector for C language from erl_nif_term 
int
enif_get_int_vec_from_list(ErlNifEnv *env, ERL_NIF_TERM list, int *vec, unsigned int size)
{
	ERL_NIF_TERM head, tail;

	int acc = true;
	int tmp_r;
	for(int i=0; i<size; i++){
		enif_get_list_cell(env, list, &head, &tail);
		tmp_r = enif_get_int(env, head, &vec[i]);
		list = tail;
		acc &= tmp_r;
	}
	if(__builtin_expect((tmp_r == false), false)) {
		return fail;
	}
	return success;
}

//enif_make_list_from_int_vec is function that make erl_term list from list of C language 
ERL_NIF_TERM
enif_make_list_from_int_vec(ErlNifEnv *env, const int *vec, const unsigned int size)
{
	ERL_NIF_TERM list = enif_make_list(env, 0);
	for(int i = size; i>0; i--) {
		ERL_NIF_TERM tail = list;
		ERL_NIF_TERM head = enif_make_int(env, vec[i-1]);
		list = enif_make_list_cell(env, head, tail);
	}
	return list;
}

static void printError(const cl_int err);

/*
----------------------------------------------------
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
############ main function ######################
VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
----------------------------------------------------
*/

static ERL_NIF_TERM
cl_test(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  int err;

//size get is OK
  unsigned int size;
  enif_get_list_length(env, argv[0], &size);
/*----------------------------------------------------*/


//file set maybe OK
  //read kernel code
    FILE *fp;
    char filename[] = "./sample.cl";
    char *KernelSource;
    size_t source_size;
    fp = fopen(filename, "r");
    if(!fp)
    {
        exit(1);
    }
    KernelSource = (char *)malloc(MAX_SOURCE_SIZE);
    source_size = fread(KernelSource, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);
/*----------------------------------------------------*/


//data set up is OK
  int *data;
  data = (int *)malloc( size* sizeof(int));

  if (__builtin_expect((enif_get_int_vec_from_list(env, argv[0], data, size) == fail), false)) {
    return enif_make_badarg(env);
  }
/*----------------------------------------------------*/

//out set is OK
  int *out;
  out = (int *)malloc( size* sizeof(int));
/*----------------------------------------------------*/

//get platform is OK
    cl_platform_id platform_id = NULL;
    cl_uint num_devices, num_platforms;

    err = clGetPlatformIDs(1, &platform_id, &num_platforms);
    if(err != CL_SUCCESS){
        printf("clgetplatforms is failed.\n");
        printError(err);
        return 1;
    }

    cl_device_id device_id;
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, &num_devices);
    if(err != CL_SUCCESS){
        printf("clgetdevice id is failed.\n");
        printError(err);
        return 1;
    }
/*------------------------------------------------------*/

//cretae a compute context is OK
    cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if(err != CL_SUCCESS){
        printf("clcraetecontext is failed.\n");
        printError(err);
        return 2;
    }
/*------------------------------------------------------*/

//create command queue is OK
    cl_command_queue queue = clCreateCommandQueue(context, device_id, 0, &err);
    if(err != CL_SUCCESS){
        printf("clcreatecommand queue is failed\n");
        printError(err);
        return EXIT_FAILURE;
    }
/*------------------------------------------------------*/

// create program object is OK
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&KernelSource, NULL, &err);
    if(err != CL_SUCCESS){
        printf("clcreateProgramWithSource is failed.\n");
        printError(err);
        return EXIT_FAILURE;
    }
/*------------------------------------------------------*/

//build program
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS){
	size_t len;
	char buffer[2048];

        printf("clBuildProgram is failed.\n");
	clGetProgramBuildInfo(program,device_id, CL_PROGRAM_BUILD_LOG,sizeof(buffer), buffer, &len);
	printf("%s\n", buffer);
        printError(err);
	exit(1);
    }
/*-------------------------------------------------------*/


//create kernel
    cl_kernel kernel = clCreateKernel(program, "sample", &err);
    if(err != CL_SUCCESS) {
        printf("clcreatekernel is failed.\n");
        printError(err);
        exit(1);
    }
/*-------------------------------------------------------*/

//make memory object
    cl_mem memIn = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(int)*size, data, &err);
    if(err != CL_SUCCESS) {
        printf("clcreateBuffer is failed.\n");
        printError(err);
        exit(1);
    }

    cl_mem memOut = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int)*size, NULL, &err);
    if(err != CL_SUCCESS) {
        printf("clcreateBuffer is failed.\n");
        printError(err);
        exit(1);
    }
/*-------------------------------------------------------*/

//set kernel arg
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memIn);
    if(err != CL_SUCCESS) {
        printf("clsetkernelarg in is failed.\n");
        printError(err);
        exit(1);
    }

	err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&memOut);
    if(err != CL_SUCCESS) {
        printf("clsetkernelarg out is failed.\n");
        printError(err);
        exit(1);
    }
/*-------------------------------------------------------*/

//kernet execute
    size_t local;
    err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
    if (err != CL_SUCCESS){
        printf("Error: failed to retrieve kernel work group info\n");
    }
    size_t global = size;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);

    if(err != CL_SUCCESS) {
        printf("clenqueuendrangekernal out is failed.\n");
        printError(err);
        exit(1);
    }

    err = clEnqueueReadBuffer(queue, memOut, CL_TRUE, 0, sizeof(cl_int)*size,out, 0, NULL, NULL);
    if(err != CL_SUCCESS){
        printf("readbuffer  is failed.\n");
        printError(err);
        exit(1);
    }

//simple caluculate
/*
  for(int i=0; i<size; i++){
	  out[i] = (22 * data[i] * (data[i] + 1)) % 6700417;
      out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
      out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
      out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
      out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
      out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
      out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
      out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  }
  */
  printf("%d\n", data[1]);


//return list is OK
  ERL_NIF_TERM list = enif_make_list_from_int_vec(env, out, size);
  return list;
/*----------------------------------------------------*/

}

static void 
printError(const cl_int err)
{
    switch (err){
        case CL_BUILD_PROGRAM_FAILURE:
            printf("Error: build program failure\n");
            break;
        case CL_COMPILER_NOT_AVAILABLE:
            printf("Error: OpenCL compiler is not available\n");
            break;
        case CL_DEVICE_NOT_AVAILABLE:
            printf("Error: device is not available\n");
            break;
        case CL_DEVICE_NOT_FOUND:
            printf("Error: device is not found\n");
            break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            printf("Error: image format is not supported\n");
            break;
        case CL_IMAGE_FORMAT_MISMATCH:
            printf("Error: image format mismatch\n");
            break;
        case CL_INVALID_ARG_INDEX:
            printf("Error: ivalid arg index\n");
            break;
        case CL_INVALID_ARG_SIZE:
            printf("Error:invalid arg size \n");
            break;
        case CL_INVALID_ARG_VALUE:
            printf("Error: invalod argb value\n");
            break;
        case CL_INVALID_BINARY:
            printf("Error: invalid binary\n");
            break;
        case CL_INVALID_BUFFER_SIZE:
            printf("Error: invalid buffer size \n");
            break;
        case CL_INVALID_BUILD_OPTIONS:
            printf("Error: invalid build options\n");
            break;
        case CL_INVALID_COMMAND_QUEUE:
            printf("Error: invalid ommand queue\n");
            break;
        case CL_INVALID_CONTEXT:
            printf("Error: invalid context\n");
            break;
        case CL_INVALID_DEVICE:
            printf("Error: invalid device\n");
            break;
        case CL_INVALID_DEVICE_TYPE:
            printf("Error: invalid device type \n");
            break;
        case CL_INVALID_EVENT:
            printf("Error: invalid event\n");
            break;
        case CL_INVALID_EVENT_WAIT_LIST:
            printf("Error: invalid event wait list\n");
            break;
        case CL_INVALID_GL_OBJECT:
            printf("Error: invalid eb¥vglobject\n");
            break;
        case CL_INVALID_GLOBAL_OFFSET:
            printf("Error: invalid global offset\n");
            break;
        case CL_INVALID_HOST_PTR:
            printf("Error: invalid host pointer\n");
            break;
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            printf("Error: invalid format descriptor\n");
            break;
        case CL_INVALID_IMAGE_SIZE:
            printf("Error: invalid image size\n");
            break;
        case CL_INVALID_KERNEL:
            printf("Error: invalid kernel\n");
            break;
        case CL_INVALID_KERNEL_ARGS:
            printf("Error: in¥valid kernel args\n");
            break;
        case CL_INVALID_KERNEL_DEFINITION:
            printf("Error: invalid kerneldefinition\n");
            break;
        case CL_INVALID_KERNEL_NAME:
            printf("Error: invalid kernel name\n");
            break;
        case CL_INVALID_MEM_OBJECT:
            printf("Error: invalid mem object\n");
            break;
        case CL_INVALID_MIP_LEVEL:
            printf("Error: invalid mip level\n");
            break;
        case CL_INVALID_OPERATION:
            printf("Error: invalid operation\n");
            break;
        case CL_INVALID_PLATFORM:
            printf("Error: invalid platform\n");
            break;
        case CL_INVALID_PROGRAM:
            printf("Error: invalid program\n");
            break;
        case CL_INVALID_PROGRAM_EXECUTABLE:
            printf("Error: invalid program executable\n");
            break;
        case CL_INVALID_QUEUE_PROPERTIES:
            printf("Error: invalid queue properties\n");
            break;
        case CL_INVALID_VALUE:
            printf("Error: invalid value\n");
            break;
        case CL_INVALID_WORK_DIMENSION:
            printf("Error: invalid work dimension\n");
            break;
        case CL_INVALID_WORK_GROUP_SIZE:
            printf("Error: invalid work group size\n");
            break;
        case CL_INVALID_WORK_ITEM_SIZE:
            printf("Error: invalid work item size\n");
            break;
        case CL_MAP_FAILURE:
            printf("Error: memory mapping failure\n");
            break;
        case CL_MEM_COPY_OVERLAP:
            printf("Error: copying loverlapped memory address\n");
            break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            printf("Error: memory object allovcation failyee\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            printf("Error: out of host mmory\n");
            break;
        case CL_OUT_OF_RESOURCES:
            printf("Error: out of resources\n");
            break;
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            printf("Error: profilinginfo is not available\n");
            break;
        case CL_SUCCESS:
            printf("succeeded\n");
            break;
        default:
            printf("unknown\n");
            break;
    }
}

// Let's define the array of ErlNifFunc beforehand:
static ErlNifFunc nif_funcs[] = {
  // {erl_function_name, erl_function_arity, c_function}
  {"cl_test", 1, cl_test}
};

ERL_NIF_INIT(Elixir.CLTest, nif_funcs, NULL, NULL, NULL, NULL)
