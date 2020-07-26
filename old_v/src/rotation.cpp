#include "rotation.cuh"
#include "cublas_v2.h"
#include "cublas_api.h"
#include "cuda_runtime.h"
#include "cuda_device_runtime_api.h"
#include <stdio.h>
#include "atom.cuh"
#include <iostream>
#include "math_constants.h"

#define NUM_OF_STREAMS 360

using namespace std;

inline cudaError_t checkCuda(cudaError_t result, int line)
{
  if (result != cudaSuccess) {
    fprintf(stderr, "CUDA Runtime Error: %s at line %d\n", cudaGetErrorString(result),line);
  }
  return result;
}


Rotation::Rotation(double3 vector){
    quaternion = make_double4(vector.x,vector.y,vector.z,0);
}

__host__ void Rotation::getQuaternion(double4* h_quat,double4* unit_quaternion){
    cudaMemcpy(h_quat,unit_quaternion,sizeof(double4),cudaMemcpyDeviceToHost);
}


__device__ void setUnitQuaternion(double* quaternion,double angle,double4& unit_quaternion,double*norm){ 

    double sin_2, cos_2;
    //sincos(angle/2, &sin_2, &cos_2);
    cos_2 = cos(angle/2);
    sin_2 = sin(angle/2);
    //double prova;
    //prova = norm3d(quaternion[0],quaternion[1],quaternion[2]);

    quaternion[0] = quaternion[0]/ *norm;
    quaternion[1] = quaternion[1]/ *norm;
    quaternion[2] = (quaternion[2]/ *norm);


    //quaternion[0] = quaternion[0]/ prova;
    //quaternion[1] = quaternion[1]/ prova;
    //quaternion[2] = quaternion[2]/ prova;

    unit_quaternion = make_double4(sin_2*(quaternion[0]),sin_2*(quaternion[1]),sin_2*(quaternion[2]),cos_2);
    
    
}


__device__ void initFirstRow(int tid, double* rotation_matrix, size_t& pitch, double4 & unit_quat){
    
    double* row_start = (double*)((char*)rotation_matrix  + tid*pitch);
    /*
    row_start[0] =  1-2*(unit_quat[1]*unit_quat[1] + powf(unit_quat[2],2));
    //row_start++;
    row_start[1] = 2*(unit_quat[0]*unit_quat[1] - unit_quat[2]*unit_quat[3]);
    //row_start++;
    row_start[2] = 2*(unit_quat[0]*unit_quat[2] + unit_quat[1]*unit_quat[3]);
    */  
    
    row_start[0] =  1-2*(pow(unit_quat.y,2) + pow(unit_quat.z,2));
    
    row_start[1] = 2*(unit_quat.x*unit_quat.y - unit_quat.z*unit_quat.w);

    row_start[2] = 2*(unit_quat.x*unit_quat.z + unit_quat.y*unit_quat.w);
    for(int i= 0;i < 3;i++){
        if(row_start[i] == -0) row_start[i] = 0;
    }
    //printf("row %d values : %lf %lf %lf\n", tid,row_start[0],row_start[1],row_start[2]);


}

__device__ void initFirstRow_v2(double rotation_matrix[3][3], double4 & unit_quat){
    
    
    
    rotation_matrix[0][0] =  1-2*(pow(unit_quat.y,2) + pow(unit_quat.z,2));
    
    rotation_matrix[0][1] = 2*(unit_quat.x*unit_quat.y - unit_quat.z*unit_quat.w);

    rotation_matrix[0][2] = 2*(unit_quat.x*unit_quat.z + unit_quat.y*unit_quat.w);
    for(int i= 0;i < 3;i++){
        if(rotation_matrix[0][i] == -0) rotation_matrix[0][i] = 0;
    }
    //printf("row %d values : %lf %lf %lf\n", tid,row_start[0],row_start[1],row_start[2]);


}

__device__ void initSecondRow(int tid, double* rotation_matrix, size_t& pitch, double4 & unit_quat){
    
    double* row_start = (double*)((char*)rotation_matrix  + tid*pitch);
    row_start[0] =  2*(unit_quat.x*unit_quat.y + unit_quat.z*unit_quat.w);

    row_start[1] = 1-2*(unit_quat.x*unit_quat.x + pow(unit_quat.z,2));//unit_quat.z*unit_quat.z);

    row_start[2] = 2*(unit_quat.y*unit_quat.z - unit_quat.x*unit_quat.w);
    for(int i= 0;i < 3;i++){
        if(row_start[i] == -0) row_start[i] = 0;
    }

    //printf("row %d values : %lf %lf %lf\n", tid,row_start[0],row_start[1],row_start[2]);

}

__device__ void initSecondRow_v2(double rotation_matrix[3][3], double4 & unit_quat){
    
    
    rotation_matrix[1][0] =  2*(unit_quat.x*unit_quat.y + unit_quat.z*unit_quat.w);

    rotation_matrix[1][1] = 1-2*(unit_quat.x*unit_quat.x + pow(unit_quat.z,2));//unit_quat.z*unit_quat.z);

    rotation_matrix[1][2] = 2*(unit_quat.y*unit_quat.z - unit_quat.x*unit_quat.w);
    for(int i= 0;i < 3;i++){
        if(rotation_matrix[1][i] == -0) rotation_matrix[1][i] = 0;
    }

    //printf("row %d values : %lf %lf %lf\n", tid,row_start[0],row_start[1],row_start[2]);

}

__device__ void initThirdRow(int tid, double* rotation_matrix, size_t & pitch, double4 & unit_quat){
    
    double* row_start = (double*)((char*)rotation_matrix  + tid*pitch);
    row_start[0] =  2*(unit_quat.x*unit_quat.z - unit_quat.y*unit_quat.w);

    row_start[1] = 2*(unit_quat.y*unit_quat.z + unit_quat.x*unit_quat.w);

    row_start[2] = 1-2*(pow(unit_quat.x,2) + pow(unit_quat.y,2));
    for(int i= 0;i < 3;i++){
        if(row_start[i] == -0) row_start[i] = 0;
    }
    //printf("row %d values : %lf %lf %lf\n", tid,row_start[0],row_start[1],row_start[2]);

}

__device__ void initThirdRow_v2(double rotation_matrix[3][3], double4 & unit_quat){
    
    
    rotation_matrix[2][0] =  2*(unit_quat.x*unit_quat.z - unit_quat.y*unit_quat.w);

    rotation_matrix[2][1] = 2*(unit_quat.y*unit_quat.z + unit_quat.x*unit_quat.w);

    rotation_matrix[2][2] = 1-2*(pow(unit_quat.x,2) + pow(unit_quat.y,2));
    for(int i= 0;i < 3;i++){
        if(rotation_matrix[2][i] == -0) rotation_matrix[2][i] = 0;
    }
    //printf("row %d values : %lf %lf %lf\n", tid,row_start[0],row_start[1],row_start[2]);

}

__global__ void init_rot_matrix_kernel(double* quat,double angle,
                                        double* rotation_matrix, size_t pitch,double * norm ){
    
    double4 unit_quaternion;
    int tid = threadIdx.x; //+ blockIdx.x*blockDim.x;


    
    setUnitQuaternion(quat,angle,unit_quaternion, norm);
    
    //initFirstRow(0,rotation_matrix,pitch,unit_quaternion);
    //initSecondRow(1,rotation_matrix,pitch,unit_quaternion);
    //initThirdRow(2,rotation_matrix,pitch,unit_quaternion);
    
    //printf("unit_q %lf x %lf y %lf z %lf w\n", unit_quaternion.x,unit_quaternion.y,unit_quaternion.z,unit_quaternion.w);

    
    if(tid < 3){
        if(tid == 0) initFirstRow(tid,rotation_matrix,pitch,unit_quaternion);
        else if(tid == 1) initSecondRow(tid,rotation_matrix,pitch,unit_quaternion);
        else if(tid == 2) initThirdRow(tid,rotation_matrix,pitch,unit_quaternion);
    }
}

__global__ void init_rot_matrix_kernel_v2(double* rotation_matrix, size_t pitch, double4* unit_quaternion ){
    
    int tid = threadIdx.x; //+ blockIdx.x*blockDim.x;

    //printf("unit_q %lf x %lf y %lf z %lf w\n", unit_quaternion.x, *unit_quaternion.y, *unit_quaternion.z, *unit_quaternion.w);

    
    if(tid < 3){
        if(tid == 0) initFirstRow(tid,rotation_matrix,pitch,*unit_quaternion);
        else if(tid == 1) initSecondRow(tid,rotation_matrix,pitch,*unit_quaternion);
        else if(tid == 2) initThirdRow(tid,rotation_matrix,pitch,*unit_quaternion);
    }

}

__global__ void init_rot_matrix_kernel_v3(double* rotation_matrix, size_t pitch, double4* unit_quaternion,int s ){
    
    int tid = threadIdx.x; //+ blockIdx.x*blockDim.x;

    //printf("unit_q %lf x %lf y %lf z %lf w\n", unit_quaternion[s].x,unit_quaternion[s].y,unit_quaternion[s].z,unit_quaternion[s].w);

    
    if(tid < 3){
        if(tid == 0) initFirstRow(tid,rotation_matrix,pitch,unit_quaternion[s]);
        else if(tid == 1) initSecondRow(tid,rotation_matrix,pitch,unit_quaternion[s]);
        else if(tid == 2) initThirdRow(tid,rotation_matrix,pitch,unit_quaternion[s]);
    }
    
    /*if(tid == 0){
        double* row_start = (double*)((char*)rotation_matrix  + 0*pitch);
        printf("%d %lf %lf %lf\n", tid, row_start[0],row_start[1],row_start[2]);
    }*/

}

__global__ void rotation_kernel(atom_st* res, atom_st* atoms, double* rotation_matrix, size_t pitch,
                                int number_of_atoms, double3 pp){
    
    
    double* first_row = (double*)((char*)rotation_matrix  + 0*pitch);
    double* second_row = (double*)((char*)rotation_matrix + 1*pitch);
    double* third_row = (double*)((char*)rotation_matrix  + 2*pitch);

    int tid = threadIdx.x;
    if(tid < number_of_atoms){
        
        res[tid].id = atoms[tid].id;

        res[tid].position.x = atoms[tid].position.x * first_row[0] + \
                              atoms[tid].position.y * first_row[1] + \
                              atoms[tid].position.z * first_row[2] + pp.x;
        
        res[tid].position.y = atoms[tid].position.x * second_row[0] + \
                              atoms[tid].position.y * second_row[1] + \
                              atoms[tid].position.z * second_row[2] + pp.y;
        
        res[tid].position.z = atoms[tid].position.x * third_row[0] + \
                              atoms[tid].position.y * third_row[1] + \
                              atoms[tid].position.z * third_row[2] + pp.z;
    
    printf("%d %lf x %lf y %lf z\n", tid,res[tid].position.x,res[tid].position.y,res[tid].position.z);
    }

}

__global__ void rotation_kernel_v2(atom_st* res, atom_st* atoms, double* rotation_matrix, size_t pitch,
                                int number_of_atoms, double3 pp){
    
    
    double* first_row = (double*)((char*)rotation_matrix  + 0*pitch);
    double* second_row = (double*)((char*)rotation_matrix + 1*pitch);
    double* third_row = (double*)((char*)rotation_matrix  + 2*pitch);
    
    int tid = threadIdx.x;
    if(tid < number_of_atoms){
        
        res[tid].id = atoms[tid].id;

        res[tid].position.x = atoms[tid].position.x * first_row[0] + \
                              atoms[tid].position.y * first_row[1] + \
                              atoms[tid].position.z * first_row[2] + pp.x;
        
        res[tid].position.y = atoms[tid].position.x * second_row[0] + \
                              atoms[tid].position.y * second_row[1] + \
                              atoms[tid].position.z * second_row[2] + pp.y;
        
        res[tid].position.z = atoms[tid].position.x * third_row[0] + \
                              atoms[tid].position.y * third_row[1] + \
                              atoms[tid].position.z * third_row[2] + pp.z;
    
    //printf("kernel_v2 %d %lf x %lf y %lf z\n", tid,res[tid].position.x,res[tid].position.y,res[tid].position.z);
    }

}

__global__ void rotation_kernel_v3(atom_st* res, atom_st* atoms, double* rotation_matrix, size_t pitch,
                                int number_of_atoms, double3 pp, double4* unit_quaternion, int s){
    

    int tid = threadIdx.x; //+ blockIdx.x*blockDim.x;

    //printf("unit_q %lf x %lf y %lf z %lf w\n", unit_quaternion[s].x,unit_quaternion[s].y,unit_quaternion[s].z,unit_quaternion[s].w);
    
    
    if(tid < 3){
        if(tid == 0) initFirstRow(tid,rotation_matrix,pitch,unit_quaternion[s]);
        else if(tid == 1) initSecondRow(tid,rotation_matrix,pitch,unit_quaternion[s]);
        else if(tid == 2) initThirdRow(tid,rotation_matrix,pitch,unit_quaternion[s]);
    }
    
    __syncthreads();

    double* first_row = (double*)((char*)rotation_matrix  + 0*pitch);
    double* second_row = (double*)((char*)rotation_matrix + 1*pitch);
    double* third_row = (double*)((char*)rotation_matrix  + 2*pitch);
    
    //int tid = threadIdx.x;
    if(tid < number_of_atoms){
        
        res[tid].id = atoms[tid].id;

        res[tid].position.x = atoms[tid].position.x * first_row[0] + \
                              atoms[tid].position.y * first_row[1] + \
                              atoms[tid].position.z * first_row[2] + pp.x;
        
        res[tid].position.y = atoms[tid].position.x * second_row[0] + \
                              atoms[tid].position.y * second_row[1] + \
                              atoms[tid].position.z * second_row[2] + pp.y;
        
        res[tid].position.z = atoms[tid].position.x * third_row[0] + \
                              atoms[tid].position.y * third_row[1] + \
                              atoms[tid].position.z * third_row[2] + pp.z;
    
    //printf("kernel_v2 %d %lf x %lf y %lf z\n", tid,res[tid].position.x,res[tid].position.y,res[tid].position.z);
    }

}

__global__ void rotation_kernel_v4(atom_st* res, atom_st* atoms,
                                int number_of_atoms, double3 pp, double4* unit_quaternion, int s){
    

    int tid = threadIdx.x; //+ blockIdx.x*blockDim.x;

    //printf("unit_q %lf x %lf y %lf z %lf w\n", unit_quaternion[s].x,unit_quaternion[s].y,unit_quaternion[s].z,unit_quaternion[s].w);
    __shared__ double rot_matrix[3][3];
    
    if(tid < 3){
        if(tid == 0) initFirstRow_v2(rot_matrix,unit_quaternion[s]);
        else if(tid == 1) initSecondRow_v2(rot_matrix,unit_quaternion[s]);
        else if(tid == 2) initThirdRow_v2(rot_matrix,unit_quaternion[s]);
    }
    
    __syncthreads();

    //if(tid == 0)
    //    printf(" angle %d unit_q %lf x %lf y %lf z %lf w\n",s, unit_quaternion[s].x,unit_quaternion[s].y,unit_quaternion[s].z,unit_quaternion[s].w);
    
    //int tid = threadIdx.x;
    if(tid < number_of_atoms){
        
        res[tid].id = atoms[tid].id;

        res[tid].position.x = atoms[tid].position.x * rot_matrix[0][0] + \
                              atoms[tid].position.y * rot_matrix[0][1] + \
                              atoms[tid].position.z * rot_matrix[0][2] + pp.x;
    
        res[tid].position.y = atoms[tid].position.x * rot_matrix[1][0] + \
                              atoms[tid].position.y * rot_matrix[1][1] + \
                              atoms[tid].position.z * rot_matrix[1][2] + pp.y;
        
        res[tid].position.z = atoms[tid].position.x * rot_matrix[2][0] + \
                              atoms[tid].position.y * rot_matrix[2][1] + \
                              atoms[tid].position.z * rot_matrix[2][2] + pp.z;
    }
    //printf("kernel_v2 %d %lf x %lf y %lf z\n", tid,res[tid].position.x,res[tid].position.y,res[tid].position.z);
    

}


__global__ void rotation_kernel_v5(atom_st* res, atom_st* atoms,
                                int number_of_atoms, double3 pp, double4* unit_quaternion, int s){
    

    int tid = threadIdx.x; //+ blockIdx.x*blockDim.x;
    if(s+blockIdx.x < 360){
    //printf("unit_q %lf x %lf y %lf z %lf w\n", unit_quaternion[s].x,unit_quaternion[s].y,unit_quaternion[s].z,unit_quaternion[s].w);
        __shared__ double rot_matrix[3][3];
    
        
        if(tid == 0) initFirstRow_v2(rot_matrix,unit_quaternion[s+blockIdx.x]);
        else if(tid == 1) initSecondRow_v2(rot_matrix,unit_quaternion[s+blockIdx.x]);
        else if(tid == 2) initThirdRow_v2(rot_matrix,unit_quaternion[s+blockIdx.x]);

    
        __syncthreads();

    //if(tid == 0)
    //    printf(" angle %d unit_q %lf x %lf y %lf z %lf w\n",s, unit_quaternion[s].x,unit_quaternion[s].y,unit_quaternion[s].z,unit_quaternion[s].w);
    
        int index = threadIdx.x + blockIdx.x*number_of_atoms;
        if(index < number_of_atoms*(blockIdx.x+1) && number_of_atoms*blockIdx.x <= index){
            //printf("kernel_v5 %d block %d\n", index,blockIdx.x);
            res[index].id = atoms[tid].id;

            res[index].position.x = atoms[tid].position.x * rot_matrix[0][0] + \
                                atoms[tid].position.y * rot_matrix[0][1] + \
                                atoms[tid].position.z * rot_matrix[0][2] + pp.x;
        
            res[index].position.y = atoms[tid].position.x * rot_matrix[1][0] + \
                                atoms[tid].position.y * rot_matrix[1][1] + \
                                atoms[tid].position.z * rot_matrix[1][2] + pp.y;
            
            res[index].position.z = atoms[tid].position.x * rot_matrix[2][0] + \
                                atoms[tid].position.y * rot_matrix[2][1] + \
                                atoms[tid].position.z * rot_matrix[2][2] + pp.z;
            //if(blockIdx.x == 7)printf("kernel_v5 %d %lf x %lf y %lf z blockId %d\n", index,res[index].position.x,res[index].position.y,res[index].position.z,blockIdx.x);
        }
        
    }

}


__global__ void first_translation(atom_st* atoms,double3 pp,int number_of_atoms){

    int tidx = threadIdx.x;
     
    if(tidx < number_of_atoms){
        atoms[tidx].position.x -= pp.x;
        atoms[tidx].position.y -= pp.y;
        atoms[tidx].position.z -= pp.z;
    }

}

__global__ void back_translation(atom_st* atoms,double3 pp,int number_of_atoms){

    int tidx = threadIdx.x;
     
    if(tidx < number_of_atoms){
        atoms[tidx].position.x += pp.x;
        atoms[tidx].position.y += pp.y;
        atoms[tidx].position.z += pp.z;
    }

}

void Rotation::normalize(double* quat, double* norm){
    cublasHandle_t handle;
    cublasStatus_t stat;
    
    stat = cublasCreate(&handle);

    stat = cublasDnrm2(handle,4,quat,1,norm);
    
    cublasDestroy(handle);

}

atom_st* Rotation::rotate(double angle, std::vector<atom_st>& atoms_st, double3& pp){

    int deviceId;
    double *quat;
    atom_st *atoms;
    double* rotation_matrix;
    size_t pitch;
    atom_st * res;

    cudaError_t err;

    int number_of_atoms = atoms_st.size();
    int size_of_atoms = number_of_atoms*sizeof(atom_st);

    checkCuda( cudaGetDevice(&deviceId), __LINE__ );
    checkCuda( cudaMallocManaged(&quat, 4*sizeof(double)), __LINE__ );
    cudaMallocManaged(&atoms, size_of_atoms);
    cudaMallocManaged(&res, size_of_atoms);
    //initialize vector of quaternion
    quat[0] = quaternion.x;
    quat[1] = quaternion.y;
    quat[2] = quaternion.z;
    quat[3] = quaternion.w;
    
    //initialize vector of atoms
    int i = 0;
    for(auto at : atoms_st){
        atoms[i] = at;
        i++;
    }
    
    
    //cudaDeviceGetAttribute(&numberOfSMs, cudaDevAttrMultiProcessorCount, deviceId);

    cudaMemPrefetchAsync(quat,4*sizeof(double),deviceId);
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    cudaMemPrefetchAsync(res,size_of_atoms,deviceId);

    cudaMallocPitch(&rotation_matrix, &pitch, 3*sizeof(double), 3);

    //double4* unit_quaternion;
    //cudaMallocManaged(&unit_quaternion, sizeof(double4));

    //cout << atoms[0].position.x << " " << atoms[0].position.y << " " << atoms[0].position.z << endl;
    double3 passingPoint = pp;
    
    
    first_translation<<<1,number_of_atoms>>>(atoms,passingPoint, number_of_atoms);

    cudaDeviceSynchronize();
    //cout << "traslation\n";
    //cout << atoms[0].position.x << " " << atoms[0].position.y << " " << atoms[0].position.z << endl;

    double norm;
    
    double *g_norm;
    cudaMalloc(&g_norm, sizeof(double));

    normalize(quat,&norm);
    
    
    cudaMemcpy(g_norm, &norm, sizeof(double), cudaMemcpyHostToDevice);
    err = cudaGetLastError();
    if(err != cudaSuccess){
        printf("Error %s \n", cudaGetErrorString(err));
    }

    cudaMemPrefetchAsync(quat,4*sizeof(double),deviceId);
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    cudaMemPrefetchAsync(res,size_of_atoms,deviceId);

    init_rot_matrix_kernel<<<1,3>>>(quat, angle, rotation_matrix, pitch, g_norm);
    cudaDeviceSynchronize();

    /*cudaMemPrefetchAsync(quat,4*sizeof(double),deviceId);
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    cudaMemPrefetchAsync(res,size_of_atoms,deviceId);
    */
    rotation_kernel<<<1,number_of_atoms>>>(res,atoms,rotation_matrix,pitch,number_of_atoms,passingPoint); 
    
    err = cudaGetLastError();
    if(err != cudaSuccess){
        printf("Error %s \n", cudaGetErrorString(err));
    }

    cudaDeviceSynchronize();
    //back_translation<<<1,number_of_atoms>>>(atoms,passingPoint,number_of_atoms);
    cudaMemPrefetchAsync(res,1*sizeof(double), cudaCpuDeviceId);
    err = cudaGetLastError();
    if(err != cudaSuccess){
        cout << __LINE__ << endl;
        printf("Error %s \n", cudaGetErrorString(err));
    }

    //cout << "norm of quat: " << norm << endl;

    //cout << quaternion.x << " " << quaternion.y << " " << quaternion.z << endl;

    /*err = cudaGetLastError();
    if(err != cudaSuccess){
        printf("Error %s \n", cudaGetErrorString(err));
    }*/
    
    /*float* h_rot_matrix = (float*)malloc(3*3 *sizeof(float));
    /*cout << "first atom : " << __LINE__ << endl;
    for(int i = 0; i < number_of_atoms; i++){
        cout << res[i].id << " " << res[i].position.x << " " << res[i].position.y << " " << res[i].position.z << endl;
    }*/
    /*
    for(int i = 0; i< 3;i++){
        h_rot_matrix[i] = (float *)malloc(3*sizeof(float));
    }

    for(int i = 0; i< 3;i++){
        for(int j = 0;j<3;j++){
            h_rot_matrix[i][j] = i+j; 
        }
    }

    size_t h_pitch = pitch;
    cudaMemcpy2D(h_rot_matrix,3*sizeof(float),rotation_matrix,pitch,3*(sizeof(float)),3, cudaMemcpyDeviceToHost);

    for(int i = 0; i< 3; i++){
        for(int j = 0; j<3;j++){
            cout << h_rot_matrix[i*3+j] << " ";
        }
        cout << endl;
    }
    
    //cout << h_rot_matrix << endl;
    /*
    for(int i = 0; i< 4;i++) cout << quat[i] << " ";
    cout << endl;
    cout << "here\n";
    */
    //float4 * h_uq;

    //getQuaternion(h_uq,unit_quaternion);

    //cudaFree(h_uq);
    //cudaFree(res);
    checkCuda( cudaFree(quat), __LINE__ );
    checkCuda( cudaFree(atoms), __LINE__ );
    //cudaFree(unit_quaternion);
    //checkCuda( cudaFree(g_norm) );
    checkCuda( cudaFree(rotation_matrix), __LINE__);
    //free(h_rot_matrix);

    

    return res;
}

atom_st* Rotation::rotate_v2(int angle, std::vector<atom_st>& atoms_st, double3& pp, double4 unit_quaternion){

    int deviceId;
    //double *quat;
    atom_st *atoms;
    double* rotation_matrix;
    size_t pitch;
    atom_st * res;
    double4* uq;
    cudaMalloc(&uq, 1*sizeof(double4));
    cudaMemcpy(uq,&unit_quaternion, 1*sizeof(double4), cudaMemcpyHostToDevice);

    cudaError_t err;

    int number_of_atoms = atoms_st.size();
    int size_of_atoms = number_of_atoms*sizeof(atom_st);

    checkCuda( cudaGetDevice(&deviceId), __LINE__ );
    //checkCuda( cudaMallocManaged(&quat, 4*sizeof(double)));
    cudaMallocManaged(&atoms, size_of_atoms);
    cudaMallocManaged(&res, size_of_atoms);
    //initialize vector of quaternion
    //quat[0] = quaternion.x;
    //quat[1] = quaternion.y;
    //quat[2] = quaternion.z;
    //quat[3] = quaternion.w;
    
    //initialize vector of atoms
    int i = 0;
    for(auto at : atoms_st){
        atoms[i] = at;
        i++;
    }
    
    
    //cudaDeviceGetAttribute(&numberOfSMs, cudaDevAttrMultiProcessorCount, deviceId);

    //cudaMemPrefetchAsync(quat,4*sizeof(double), deviceId);
    cudaMemPrefetchAsync(atoms,size_of_atoms, deviceId);
    cudaMemPrefetchAsync(res,size_of_atoms, deviceId);
    

    cudaMallocPitch(&rotation_matrix, &pitch, 3*sizeof(double), 3);

    //double4* unit_quaternion;
    //cudaMallocManaged(&unit_quaternion, sizeof(double4));

    //cout << atoms[0].position.x << " " << atoms[0].position.y << " " << atoms[0].position.z << endl;
    double3 passingPoint = pp;
    
    
    first_translation<<<1,number_of_atoms>>>(atoms,passingPoint, number_of_atoms);

    cudaDeviceSynchronize();
    //cout << "traslation\n";
    //cout << atoms[0].position.x << " " << atoms[0].position.y << " " << atoms[0].position.z << endl;


    //cudaMemPrefetchAsync(quat,4*sizeof(double),deviceId);
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    cudaMemPrefetchAsync(res,size_of_atoms,deviceId);

    init_rot_matrix_kernel_v2<<<1,3>>>(rotation_matrix, pitch,uq);
    cudaDeviceSynchronize();

    /*cudaMemPrefetchAsync(quat,4*sizeof(double),deviceId);
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    cudaMemPrefetchAsync(res,size_of_atoms,deviceId);
    */
    rotation_kernel<<<1,number_of_atoms>>>(res,atoms,rotation_matrix,pitch,number_of_atoms,passingPoint); 
    
    err = cudaGetLastError();
    if(err != cudaSuccess){
        printf("Error %s \n", cudaGetErrorString(err));
    }

    cudaDeviceSynchronize();
    //back_translation<<<1,number_of_atoms>>>(atoms,passingPoint,number_of_atoms);
    cudaMemPrefetchAsync(res,1*sizeof(double), cudaCpuDeviceId);
    err = cudaGetLastError();
    if(err != cudaSuccess){
        cout << __LINE__ << endl;
        printf("Error %s \n", cudaGetErrorString(err));
    }

    
    //checkCuda( cudaFree(quat) );
    checkCuda( cudaFree(atoms) , __LINE__);
    //cudaFree(unit_quaternion);
    //checkCuda( cudaFree(g_norm) );
    checkCuda( cudaFree(rotation_matrix), __LINE__ );
    checkCuda( cudaFree(uq), __LINE__ );
    //free(h_rot_matrix);

    return res;
}

vector<atom_st*> Rotation::rotate_v3(int angle, std::vector<atom_st>& atoms_st, double3& pp, double4* unit_quaternion){

    int deviceId;
    int number_of_atoms = atoms_st.size();
    int size_of_atoms = number_of_atoms*sizeof(atom_st);

    atom_st *atoms;
    double* rotation_matrix[NUM_OF_STREAMS];
    //double* rotation_matrix;
    size_t pitch;

    double4* uq;
    checkCuda( cudaMalloc(&uq, 2*360*sizeof(double4)), __LINE__ ) ;
    checkCuda( cudaMemcpy(uq,unit_quaternion, 2*360*sizeof(double4), cudaMemcpyHostToDevice), __LINE__ );
    
    cudaError_t err;

    static atom_st * h_res[NUM_OF_STREAMS];
    atom_st * d_res[NUM_OF_STREAMS];    


    checkCuda( cudaGetDevice(&deviceId), __LINE__ );
    cudaMallocManaged(&atoms, size_of_atoms);
    for(int n = 0; n < NUM_OF_STREAMS; n++){
        h_res[n] = (atom_st*)malloc(size_of_atoms);
    }
    
    for(int n=0 ; n < NUM_OF_STREAMS;n++)
        checkCuda( cudaMalloc(&d_res[n],size_of_atoms), __LINE__);

    
    //initialize vector of atoms
    int i = 0;
    //cout << atoms_st[0].position.x << " prova" << endl;
    for(auto at : atoms_st){
        atoms[i] = at;
        i++;
    }
    //cout << atoms[0].position.x << " prova" << endl;
    
    
    //cudaDeviceGetAttribute(&numberOfSMs, cudaDevAttrMultiProcessorCount, deviceId);
    
    cudaMemPrefetchAsync(atoms,size_of_atoms, deviceId);
    //cudaMemPrefetchAsync(res,size_of_atoms, deviceId);
    
    vector<double*> all_rotation_matrix(NUM_OF_STREAMS);
    vector<size_t> pitches(NUM_OF_STREAMS);
    
    
    for(int m = 0; m < NUM_OF_STREAMS ; m++){

        //cudaMallocPitch(&rotation_matrix, &pitches[m], 3*sizeof(double), 3);
        checkCuda( cudaMallocPitch(&rotation_matrix[m], &pitches[m], 3*sizeof(double), 3), __LINE__ );
       
    }

    //cudaMallocManaged(test_rotation_matrix, NUM_OF_STREAMS* sizeof(rotation_matrix));
    

    //all_rotation_matrix.push_back(rotation_matrix);
    //pitches.push_back(pitch);

    double3 passingPoint = pp;
    
    
    first_translation<<<1,number_of_atoms>>>(atoms,passingPoint, number_of_atoms);

    cudaDeviceSynchronize();
    
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    //cudaMemPrefetchAsync(res,size_of_atoms,deviceId);

    //int num_of_streams = NUM_OF_STREAMS;

    cudaStream_t* streams = (cudaStream_t*)malloc(NUM_OF_STREAMS*sizeof(cudaStream_t*));

    for(int s = 0; s < NUM_OF_STREAMS; s++){
        cudaStreamCreateWithFlags(&(streams[s]),cudaStreamNonBlocking);
        double * tmp = rotation_matrix[s];
        //init_rot_matrix_kernel_v3<<<1, 3, 0, streams[s]>>>(rotation_matrix[s], pitches[s],uq, s+angle);
        rotation_kernel_v3<<<1,256,0,streams[s]>>>(d_res[s],atoms,rotation_matrix[s],pitches[s],number_of_atoms,passingPoint,uq,s+angle);
        //rotation_kernel_v4<<<1,256,0,streams[s]>>>(d_res[s],atoms,number_of_atoms,passingPoint,uq,s+angle);
    
    }

    
    //cout << atoms[0].position.x << " prova" << endl;
    //init_rot_matrix_kernel_v2<<<1,3>>>(rotation_matrix, pitch,uq);
    //cudaDeviceSynchronize();
    /*
    for(int s = 0; s < NUM_OF_STREAMS; s++){
        checkCuda( cudaStreamDestroy(streams[s]), __LINE__ );
    }

    checkCuda( cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId), __LINE__ );
    
    for(int s = 0; s < NUM_OF_STREAMS; s++){
        cudaStreamCreate(&(streams[s]));
       
        rotation_kernel_v2<<<1,number_of_atoms,0,streams[s]>>>(d_res[s],atoms,rotation_matrix[s],pitches[s],number_of_atoms,passingPoint); 
    }
    
    err = cudaGetLastError();
    if(err != cudaSuccess){
        printf("Error %s line: %d\n", cudaGetErrorString(err), __LINE__);
    }

    cudaDeviceSynchronize();
    */
   

    // cudaMemPrefetchAsync(res,number_of_atoms*sizeof(double), cudaCpuDeviceId);
    err = cudaGetLastError();
    if(err != cudaSuccess){
        cout << __LINE__ << endl;
        printf("Error %s \n", cudaGetErrorString(err));
    }
    
    //cout << atoms[0].position.x << endl;
    checkCuda( cudaFree(atoms), __LINE__ );

    for(int m = 0;m < NUM_OF_STREAMS; m++ ) checkCuda( cudaFree(rotation_matrix[m]), __LINE__);
    
    checkCuda( cudaFree(uq), __LINE__ );
    
    vector<atom_st*> result_to_return;
    //cout << __LINE__ << endl;
    
    
    for(int n = 0; n < NUM_OF_STREAMS; n++){
        checkCuda( cudaMemcpyAsync(h_res[n], d_res[n], size_of_atoms, cudaMemcpyDeviceToHost,streams[n]), __LINE__ );
    }
    //cout << __LINE__ << endl;
     for(int s = 0; s < NUM_OF_STREAMS; s++){
        checkCuda( cudaStreamDestroy(streams[s]), __LINE__ );
    }
    //cout << h_res[0][0].id << " " << h_res[0][0].position.x << endl;
    
    for(int m = 0;m < NUM_OF_STREAMS; m++ ) result_to_return.push_back(h_res[m]);

    //result_to_return[0][0].id = h_res[0][0].id;
    //result_to_return[0][0].position.x = h_res[0][0].position.x;

    //cout << __LINE__ << endl;
    
    //cout << "test " << result_to_return[0][1].id << " " << result_to_return[0][1].position.x << endl;

    //for(int m = 0;m <NUM_OF_STREAMS; m++ ) free(h_res[m]); // if frees the h_res arrays the id and position.x values of the first atom goes random...
    //cout << __LINE__ << endl;    
    for(int m = 0;m <NUM_OF_STREAMS; m++ ) checkCuda( cudaFree(d_res[m]),__LINE__ );
    //free(h_res);
    //cout << __LINE__ << endl;

    //cout << h_res[0][0].position.x << endl;
    


    return result_to_return;
}

vector<vector<atom_st>> Rotation::rotate_v4(int angle, std::vector<atom_st>& atoms_st, double3& pp, double4* unit_quaternion){

    int deviceId;
    int number_of_atoms = atoms_st.size();
    int size_of_atoms = number_of_atoms*sizeof(atom_st);

    atom_st *atoms;
    double* rotation_matrix[NUM_OF_STREAMS];
    //double* rotation_matrix;
    size_t pitch;

    double4* uq;
    checkCuda( cudaMalloc(&uq, 2*360*sizeof(double4)), __LINE__ ) ;
    checkCuda( cudaMemcpy(uq,unit_quaternion, 2*360*sizeof(double4), cudaMemcpyHostToDevice), __LINE__ );
    
    cudaError_t err;

    static atom_st * h_res[NUM_OF_STREAMS];
    atom_st * d_res[NUM_OF_STREAMS];    


    checkCuda( cudaGetDevice(&deviceId), __LINE__ );
    cudaMallocManaged(&atoms, size_of_atoms);
    for(int n = 0; n < NUM_OF_STREAMS; n++){
        h_res[n] = (atom_st*)malloc(size_of_atoms);
    }
    
    for(int n=0 ; n < NUM_OF_STREAMS;n++)
        checkCuda( cudaMalloc(&d_res[n],size_of_atoms), __LINE__);

    
    //initialize vector of atoms
    int i = 0;
    //cout << atoms_st[0].position.x << " prova" << endl;
    for(auto at : atoms_st){
        atoms[i] = at;
        i++;
    }
    //cout << atoms[0].position.x << " prova" << endl;
    
    
    //cudaDeviceGetAttribute(&numberOfSMs, cudaDevAttrMultiProcessorCount, deviceId);
    
    cudaMemPrefetchAsync(atoms,size_of_atoms, deviceId);
    //cudaMemPrefetchAsync(res,size_of_atoms, deviceId);
    
    vector<double*> all_rotation_matrix(NUM_OF_STREAMS);
    vector<size_t> pitches(NUM_OF_STREAMS);
    
    
    for(int m = 0; m < NUM_OF_STREAMS ; m++){

        //cudaMallocPitch(&rotation_matrix, &pitches[m], 3*sizeof(double), 3);
        checkCuda( cudaMallocPitch(&rotation_matrix[m], &pitches[m], 3*sizeof(double), 3), __LINE__ );
       
    }

    //cudaMallocManaged(test_rotation_matrix, NUM_OF_STREAMS* sizeof(rotation_matrix));
    

    //all_rotation_matrix.push_back(rotation_matrix);
    //pitches.push_back(pitch);

    double3 passingPoint = pp;
    
    
    first_translation<<<1,number_of_atoms>>>(atoms,passingPoint, number_of_atoms);

    cudaDeviceSynchronize();
    
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    //cudaMemPrefetchAsync(res,size_of_atoms,deviceId);

    //int num_of_streams = NUM_OF_STREAMS;

    cudaStream_t* streams = (cudaStream_t*)malloc(NUM_OF_STREAMS*sizeof(cudaStream_t*));

    for(int s = 0; s < NUM_OF_STREAMS; s++){
        cudaStreamCreateWithFlags(&(streams[s]),cudaStreamNonBlocking);
        double * tmp = rotation_matrix[s];
        //init_rot_matrix_kernel_v3<<<1, 3, 0, streams[s]>>>(rotation_matrix[s], pitches[s],uq, s+angle);
        //rotation_kernel_v3<<<1,256,0,streams[s]>>>(d_res[s],atoms,rotation_matrix[s],pitches[s],number_of_atoms,passingPoint,uq,s+angle);
        rotation_kernel_v4<<<1,256,0,streams[s]>>>(d_res[s],atoms,number_of_atoms,passingPoint,uq,s+angle);
    
    }

    
    err = cudaGetLastError();
    if(err != cudaSuccess){
        cout << __LINE__ << endl;
        printf("Error %s \n", cudaGetErrorString(err));
    }
    
    //cout << atoms[0].position.x << endl;
    checkCuda( cudaFree(atoms), __LINE__ );

    for(int m = 0;m < NUM_OF_STREAMS; m++ ) checkCuda( cudaFree(rotation_matrix[m]), __LINE__);
    
    checkCuda( cudaFree(uq), __LINE__ );
    
    vector<vector<atom_st>> result_to_return;
    //cout << __LINE__ << endl;
    
    
    for(int n = 0; n < NUM_OF_STREAMS; n++){
        checkCuda( cudaMemcpyAsync(h_res[n], d_res[n], size_of_atoms, cudaMemcpyDeviceToHost,streams[n]), __LINE__ );
    }
    //cout << __LINE__ << endl;
     for(int s = 0; s < NUM_OF_STREAMS; s++){
        checkCuda( cudaStreamDestroy(streams[s]), __LINE__ );
    }
    //cout << h_res[0][0].id << " " << h_res[0][0].position.x << endl;
    vector<atom_st> tmp;
    for(int m = 0;m < NUM_OF_STREAMS; m++ ){
        for(int i = 0; i < atoms_st.size();i++) 
            tmp.push_back(h_res[m][i]);
        result_to_return.push_back(tmp);
        tmp.clear();
    }
    //result_to_return[0][0].id = h_res[0][0].id;
    //result_to_return[0][0].position.x = h_res[0][0].position.x;

    //cout << __LINE__ << endl;
    
    //cout << "test " << result_to_return[0][1].id << " " << result_to_return[0][1].position.x << endl;

    for(int m = 0;m <NUM_OF_STREAMS; m++ ) free(h_res[m]); // if frees the h_res arrays the id and position.x values of the first atom goes random...
    //cout << __LINE__ << endl;    
    for(int m = 0;m <NUM_OF_STREAMS; m++ ) checkCuda( cudaFree(d_res[m]),__LINE__ );
    //free(h_res);
    //cout << __LINE__ << endl;

    //cout << h_res[0][0].position.x << endl;
    


    return result_to_return;
}

vector<vector<atom_st>> Rotation::rotate_v5(int angle, std::vector<atom_st>& atoms_st, double3& pp, double4* unit_quaternion){

    int deviceId;
    int number_of_atoms = atoms_st.size();
    int size_of_atoms = number_of_atoms*sizeof(atom_st);

    atom_st *atoms;

    double4* uq;
    checkCuda( cudaMalloc(&uq, 2*360*sizeof(double4)), __LINE__ ) ;
    checkCuda( cudaMemcpy(uq,unit_quaternion, 2*360*sizeof(double4), cudaMemcpyHostToDevice), __LINE__ );
    
    cudaError_t err;

     atom_st * h_res;
    atom_st * d_res;    


    checkCuda( cudaGetDevice(&deviceId), __LINE__ );
    cudaMallocManaged(&atoms, size_of_atoms);
    
    checkCuda( cudaMallocHost(&h_res, size_of_atoms*NUM_OF_STREAMS),__LINE__);
    
    checkCuda( cudaMalloc(&d_res,size_of_atoms*NUM_OF_STREAMS), __LINE__);

    
    //initialize vector of atoms
    int i = 0;
    //cout << atoms_st[0].position.x << " prova" << endl;
    for(auto at : atoms_st){
        atoms[i] = at;
        i++;
    }
    //cout << atoms[0].position.x << " prova" << endl;
    
    
    //cudaDeviceGetAttribute(&numberOfSMs, cudaDevAttrMultiProcessorCount, deviceId);
    
    cudaMemPrefetchAsync(atoms,size_of_atoms, deviceId);
    //cudaMemPrefetchAsync(res,size_of_atoms, deviceId);
    
    vector<double*> all_rotation_matrix(NUM_OF_STREAMS);
    vector<size_t> pitches(NUM_OF_STREAMS);
    
    double3 passingPoint = pp;
    
    
    first_translation<<<1,number_of_atoms>>>(atoms,passingPoint, number_of_atoms);

    cudaDeviceSynchronize();
    
    cudaMemPrefetchAsync(atoms,size_of_atoms,deviceId);
    //cudaMemPrefetchAsync(res,size_of_atoms,deviceId);

    //int num_of_streams = NUM_OF_STREAMS;

    //cudaStream_t* streams = (cudaStream_t*)malloc(NUM_OF_STREAMS*sizeof(cudaStream_t*));

    //printf("num of atoms %d line %d\n", number_of_atoms,__LINE__);
    rotation_kernel_v5<<<NUM_OF_STREAMS,64,0>>>(d_res,atoms,number_of_atoms,passingPoint,uq,angle);
    

    checkCuda( cudaDeviceSynchronize(),__LINE__);
    err = cudaGetLastError();
    if(err != cudaSuccess){
        cout << __LINE__ << endl;
        printf("Error %s \n", cudaGetErrorString(err));
    }
    
    //cout << atoms[0].position.x << endl;
    checkCuda( cudaFree(atoms), __LINE__ );

    
    checkCuda( cudaFree(uq), __LINE__ );
    
    vector<vector<atom_st>> result_to_return;
    //cout << __LINE__ << endl;
    
    
    
    checkCuda( cudaMemcpy(h_res, d_res, size_of_atoms * NUM_OF_STREAMS, cudaMemcpyDeviceToHost), __LINE__ );
    
    //cout << __LINE__ << endl;
    //for(int i = 0; i < number_of_atoms*NUM_OF_STREAMS; i++) printf("res[%d] %lf x %lf y %lf z\n",i,h_res[i].position.x,h_res[i].position.y,h_res[i].position.z);
    //cout << h_res[0][0].id << " " << h_res[0][0].position.x << endl;
    vector<atom_st> tmp;
    for(int i = 0; i < NUM_OF_STREAMS; i++ ){
        for(int c = atoms_st.size()*i; c < atoms_st.size()*(i+1); c++){
            tmp.push_back(h_res[c]);
        }
        result_to_return.push_back(tmp);
        tmp.clear();
    }
    //result_to_return[0][0].id = h_res[0][0].id;
    //result_to_return[0][0].position.x = h_res[0][0].position.x;

    //cout << __LINE__ << endl;
    
    //cout << "test " << result_to_return[0][1].id << " " << result_to_return[0][1].position.x << endl;

    checkCuda( cudaFreeHost(h_res), __LINE__); // if frees the h_res arrays the id and position.x values of the first atom goes random...
    //cout << __LINE__ << endl;    
    checkCuda( cudaFree(d_res),__LINE__ );
    //free(h_res);
    //cout << __LINE__ << endl;

    //cout << h_res[0][0].position.x << endl;
    


    return result_to_return;
}
