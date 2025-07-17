#include "../scratch/image.hpp"
#include "image.hpp"
#include "render.hpp"
#include <iostream>
#include "../scratch/unzip.hpp"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

std::vector<Image::ImageRGBA> Image::imageRBGAs;
std::unordered_map<std::string,SDL_Image*> images;

void Image::loadImages(mz_zip_archive *zip){
    std::cout << "Loading images..." << std::endl;
    int file_count = (int)mz_zip_reader_get_num_files(zip);

    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(zip, i, &file_stat)) continue;

        std::string zipFileName = file_stat.m_filename;

        // Check if file is PNG, JPG, or SVG
    if (zipFileName.size() >= 4 && 
        (zipFileName.substr(zipFileName.size() - 4) == ".png" || zipFileName.substr(zipFileName.size() - 4) == ".PNG"\
        || zipFileName.substr(zipFileName.size() - 4) == ".jpg" || zipFileName.substr(zipFileName.size() - 4) == ".JPG"\
        || zipFileName.substr(zipFileName.size() - 4) == ".svg" || zipFileName.substr(zipFileName.size() - 4) == ".SVG")) {

            size_t file_size;
            void* file_data = mz_zip_reader_extract_to_heap(zip, i, &file_size, 0);
            if (!file_data) {
                std::cout << "Failed to extract: " << zipFileName << std::endl;
                continue;
            }

            SDL_Surface* surface = nullptr;
            
            // Check if this is an SVG file
            if (zipFileName.substr(zipFileName.size() - 4) == ".svg" || zipFileName.substr(zipFileName.size() - 4) == ".SVG") {
                // Parse SVG from memory
                NSVGimage* svg_image = nsvgParseFromMemory((const char*)file_data, file_size, "px", 96.0f);
                if (svg_image) {
                    // Determine size for rasterization
                    int width = (int)svg_image->width;
                    int height = (int)svg_image->height;
                    
                    // Clamp dimensions to reasonable sizes
                    if (width > 1024) width = 1024;
                    if (height > 1024) height = 1024;
                    if (width < 16) width = 16;
                    if (height < 16) height = 16;
                    
                    // Create rasterizer
                    NSVGrasterizer* rast = nsvgCreateRasterizer();
                    if (rast) {
                        // Allocate RGBA buffer
                        unsigned char* rgba_data = (unsigned char*)malloc(width * height * 4);
                        if (rgba_data) {
                            // Calculate scale to fit the SVG into our desired size
                            float scale_x = (float)width / svg_image->width;
                            float scale_y = (float)height / svg_image->height;
                            float scale = scale_x < scale_y ? scale_x : scale_y;
                            
                            // Rasterize SVG to RGBA
                            nsvgRasterize(rast, svg_image, 0, 0, scale, rgba_data, width, height, width * 4);
                            
                            // Create SDL surface from RGBA data
                            surface = SDL_CreateRGBSurfaceFrom(rgba_data, width, height, 32, width * 4,
                                                              0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
                            
                            std::cout << "Successfully rasterized SVG: " << zipFileName << " (" << width << "x" << height << ")" << std::endl;
                            
                            // Note: rgba_data will be freed when the surface is freed
                        }
                        nsvgDeleteRasterizer(rast);
                    }
                    nsvgDelete(svg_image);
                }
            } else {
                // Use SDL_RWops to load image from memory (PNG/JPG)
                SDL_RWops* rw = SDL_RWFromMem(file_data, file_size);
                if (!rw) {
                    std::cout << "Failed to create RWops for: " << zipFileName << std::endl;
                    mz_free(file_data);
                    continue;
                }

                surface = IMG_Load_RW(rw, 0);
                SDL_RWclose(rw);
            }
            
            mz_free(file_data);

            if (!surface) {
                std::cout << "Failed to load image from memory: " << zipFileName << std::endl;
                continue;
            }

            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!texture) {
                std::cout << "Failed to create texture: " << zipFileName << std::endl;
                SDL_FreeSurface(surface);
                continue;
            }

            SDL_FreeSurface(surface);

            // Build SDL_Image object
            SDL_Image* image = new SDL_Image();
            image->spriteTexture = texture;
            SDL_QueryTexture(texture, nullptr, nullptr, &image->width, &image->height);
            image->renderRect = {0, 0, image->width, image->height};
            image->textureRect = {0, 0, image->width, image->height};

            // Strip extension from filename for the ID
            std::string imageId = zipFileName.substr(0, zipFileName.find_last_of('.'));
            images[imageId] = image;
        }
    }
}
void Image::loadImageFromFile(std::string filePath){
    // Try SVG first
    std::string svgPath = filePath + ".svg";
    FILE* file = fopen(svgPath.c_str(), "rb");
    if (file) {
        // Load SVG file
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char* svg_data = (char*)malloc(size + 1);
        if (svg_data) {
            fread(svg_data, 1, size, file);
            svg_data[size] = '\0';
            
            // Parse SVG
            NSVGimage* svg_image = nsvgParse(svg_data, "px", 96.0f);
            if (svg_image) {
                // Determine size for rasterization
                int width = (int)svg_image->width;
                int height = (int)svg_image->height;
                
                // Clamp dimensions to reasonable sizes
                if (width > 1024) width = 1024;
                if (height > 1024) height = 1024;
                if (width < 16) width = 16;
                if (height < 16) height = 16;
                
                // Create rasterizer
                NSVGrasterizer* rast = nsvgCreateRasterizer();
                if (rast) {
                    // Allocate RGBA buffer
                    unsigned char* rgba_data = (unsigned char*)malloc(width * height * 4);
                    if (rgba_data) {
                        // Calculate scale to fit the SVG into our desired size
                        float scale_x = (float)width / svg_image->width;
                        float scale_y = (float)height / svg_image->height;
                        float scale = scale_x < scale_y ? scale_x : scale_y;
                        
                        // Rasterize SVG to RGBA
                        nsvgRasterize(rast, svg_image, 0, 0, scale, rgba_data, width, height, width * 4);
                        
                        // Create SDL surface from RGBA data
                        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(rgba_data, width, height, 32, width * 4,
                                                                      0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
                        
                        if (surface) {
                            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                            if (texture) {
                                SDL_Image* image = new SDL_Image();
                                image->spriteTexture = texture;
                                SDL_QueryTexture(texture, nullptr, nullptr, &image->width, &image->height);
                                image->renderRect = {0, 0, image->width, image->height};
                                image->textureRect = {0, 0, image->width, image->height};
                                
                                images[filePath] = image;
                                std::cout << "Successfully loaded SVG: " << filePath << " (" << width << "x" << height << ")" << std::endl;
                            }
                            SDL_FreeSurface(surface);
                        }
                        
                        free(rgba_data);
                    }
                    nsvgDeleteRasterizer(rast);
                }
                nsvgDelete(svg_image);
            }
            free(svg_data);
        }
        fclose(file);
    } else {
        // Fall back to regular bitmap loading
        SDL_Image* image = new SDL_Image(filePath);
        images[filePath] = image;
    }
}
void Image::freeImage(const std::string& costumeId){
    auto image = images.find(costumeId);
    if(image != images.end()){
        images.erase(image);
    }
}

void Image::FlushImages(){
    std::vector<std::string> toDelete;
    
    for(auto& [id, img] : images){
        if(img->freeTimer <= 0){
            toDelete.push_back(id);
        } else {
            img->freeTimer -= 1;
        }
    }
    
    for(const std::string& id : toDelete){
        Image::freeImage(id);
    }
}

SDL_Image::SDL_Image(){}


SDL_Image::SDL_Image(std::string filePath){
    spriteSurface = IMG_Load(filePath.c_str());
    if (spriteSurface == NULL) {
        std::cout << "Error loading image: " << IMG_GetError();
        return;
    }
    spriteTexture = SDL_CreateTextureFromSurface(renderer, spriteSurface);
    if (spriteTexture == NULL) {
        std::cout << "Error creating texture";
        return;
    }
    SDL_FreeSurface(spriteSurface);

    // get width and height of image
    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(spriteTexture,NULL,NULL,&texW,&texH);
    width = texW;
    height = texH;
    renderRect.w = width;
    renderRect.h = height;
    textureRect.w = width;
    textureRect.h = height;
    textureRect.x = 0;
    textureRect.y = 0;

}

SDL_Image::~SDL_Image(){
    SDL_DestroyTexture(spriteTexture);
}

void SDL_Image::setScale(float amount){
    scale = amount;
    renderRect.w = width * amount;
    renderRect.h = height * amount;
}

void SDL_Image::setRotation(float rotate){
    rotation = rotate;
}