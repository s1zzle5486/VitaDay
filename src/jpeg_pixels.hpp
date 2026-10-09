#pragma once
#include <cstdio>
#include <cstdlib>
#include <csetjmp>
#include <cstring>
#include <jpeglib.h>
#include <vector>
#include <string>
struct CpuJpeg {jpeg_decompress_struct decoder; jpeg_error_mgr error; jmp_buf jump; FILE* file; unsigned char* pixels; bool created;};
static void jpegPixelError(j_common_ptr common){auto* job=(CpuJpeg*)common->client_data;longjmp(job->jump,1);}
// Keep setjmp inside a POD/malloc boundary; no C++ objects are unwound by it.
static unsigned char* jpegPixelData(const char* path,unsigned* width,unsigned* height){
    auto* job=(CpuJpeg*)calloc(1,sizeof(CpuJpeg));if(!job)return nullptr;
    job->file=fopen(path,"rb");if(!job->file){free(job);return nullptr;}
    job->decoder.err=jpeg_std_error(&job->error);job->error.error_exit=jpegPixelError;job->decoder.client_data=job;
    if(setjmp(job->jump)){if(job->created)jpeg_destroy_decompress(&job->decoder);fclose(job->file);free(job->pixels);free(job);return nullptr;}
    job->created=true;jpeg_create_decompress(&job->decoder);job->decoder.client_data=job;jpeg_stdio_src(&job->decoder,job->file);
    jpeg_read_header(&job->decoder,TRUE);
    if(!job->decoder.image_width||!job->decoder.image_height||job->decoder.image_width>2048||job->decoder.image_height>2048){jpeg_destroy_decompress(&job->decoder);fclose(job->file);free(job);return nullptr;}
    job->decoder.out_color_space=JCS_RGB;job->decoder.scale_num=1;job->decoder.scale_denom=4;
    jpeg_start_decompress(&job->decoder);*width=job->decoder.output_width;*height=job->decoder.output_height;
    if(job->decoder.output_components!=3){jpeg_destroy_decompress(&job->decoder);fclose(job->file);free(job);return nullptr;}
    job->pixels=(unsigned char*)malloc(size_t(*width)*(*height)*3);if(!job->pixels){jpeg_destroy_decompress(&job->decoder);fclose(job->file);free(job);return nullptr;}
    while(job->decoder.output_scanline<*height){JSAMPROW row=job->pixels+size_t(job->decoder.output_scanline)*(*width)*3;jpeg_read_scanlines(&job->decoder,&row,1);}
    jpeg_finish_decompress(&job->decoder);jpeg_destroy_decompress(&job->decoder);fclose(job->file);auto* result=job->pixels;free(job);return result;
}
static bool readJpegPixels(const std::string& path,std::vector<unsigned char>& pixels,unsigned& width,unsigned& height){
    auto* rgb=jpegPixelData(path.c_str(),&width,&height);if(!rgb)return false;
    pixels.resize(size_t(width)*height*4);for(size_t i=0;i<size_t(width)*height;++i){memcpy(pixels.data()+i*4,rgb+i*3,3);pixels[i*4+3]=255;}free(rgb);return true;
}
