//
//  FileIO.h
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 16/02/2016.
//  Copyright © 2016 Stephen Harrison-Daly. All rights reserved.
//

#pragma once

#include "Common.h"

#ifdef __APPLE__
#import <MetalKit/MetalKit.h>
#endif

namespace shd
{
    // Forward declaration
    struct Texture;
    
    // The file management class
    class FileIO
    {
        
    public:
        
        // Get the size of a file in bytes
        static size_t getFileSize(const char * filename);
        
        // Open specified file as text
        static bool readFile(const char * filename, uint8_t * buffer, size_t bufSize, size_t * bytesRead);

		// Write specified data to a file
		static bool writeFile(const char * filename, uint8_t * buffer, size_t bufSize);

		// Create a directory if one does not already exist
		static bool createDirectory(const char * path);
        
        // Loads a texture
        static bool loadTexture(const char * filename, Texture * tex);
        
        // Loads a texture from memory
        static bool loadTextureFromMemory(const uint8_t * buff, uint32_t width, uint32_t height, uint32_t numComponents, Texture * tex);
        
		// Load saved data
		static bool loadSavedData();

		// Save saved data
		static bool saveSavedData();

    private:

    };
}
