//
//  FileIO.cpp
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 16/02/2016.
//  Copyright © 2016 Stephen Harrison-Daly. All rights reserved.
//

#include "FileIO.h"
#include "Renderer.h"
#include "Application.h"
#include "external/rapidjson/document.h"
#include <sys/stat.h>
#include <string>

#define STBI_ASSERT(x) SHD_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_SUPPORT_ZLIB
#define STBI_NO_STDIO
#define STBI_MALLOC SHD_MALLOC
#define STBI_REALLOC SHD_REALLOC
#define STBI_FREE SHD_FREE
#include "external/stb/stb_image.h"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
shd::Texture shd::FileIO::m_textureHandles[SHD_MAX_TEXTURE_HANDLES];
#elif _WIN32
#include <d3d11_1.h>
#endif

#define SHD_SAVE_DATA_FILENAME	"/AppData/Roaming/Deathmatch/settings.dat"
#define SHD_SAVE_DATA_DIRECTORY "/AppData/Roaming/Deathmatch/"
#define SHD_SAVE_DATA_ENV_VAR	"USERPROFILE"

/* This is the format of the save data JSON v1.00
{
"Version": "1.00",
"vsyncEnabled" : true,
"isFullscreen" : true,
"maintainAspectRatio" : true,
"showFpsCounter" : true,
"audioVolumeMusic" : 10,
"audioVolumeSFX" : 10,
"teamColourPrimary" : 0,
"teamColourSecondary" : 1,
"teamColourPrimaryLocal" : 0,
"teamColourSecondaryLocal" : 1,
"screenResolution" : "Default",
"currentLanguage" : "En"
}*/

#define SHD_SAVE_DATA_FORMAT_V1_00 "{\n\"Version\": \"1.00\",\n\"vsyncEnabled\": %s,\n\"isFullscreen\": %s,\n\"maintainAspectRatio\": %s,\n\"showFpsCounter\": %s,\n\"audioVolumeMusic\": %i,\n\"audioVolumeSFX\": %i,\n\"teamColourPrimary\": %i,\n\"teamColourSecondary\": %i,\n\"teamColourPrimaryLocal\": %i,\n\"teamColourSecondaryLocal\": %i,\n\"screenResolution\": \"%s\",\n\"currentLanguage\": \"%s\"\n}"

#define SHD_SAVE_DATA_RESOLUTION_STR_DEFAULT		"Default"
#define SHD_SAVE_DATA_RESOLUTION_STR_3840_X_2160	"3840X2160"
#define SHD_SAVE_DATA_RESOLUTION_STR_2560_X_1440	"2560X1440"
#define SHD_SAVE_DATA_RESOLUTION_STR_1920_X_1080	"1920X1080"
#define SHD_SAVE_DATA_RESOLUTION_STR_1280_X_720		"1280X720"
#define SHD_SAVE_DATA_RESOLUTION_STR_800_X_600		"800X600"

#define SHD_SAVE_DATA_LANGUAGE_STR_EN				"en"
#define SHD_SAVE_DATA_LANGUAGE_STR_JP				"jp"

using namespace shd;

size_t FileIO::getFileSize(const char * filename)
{
    struct stat st;
    
#ifdef __APPLE__

    char tempPath[512]  = {'\0'};
    
    // Find filename without path and extension
    char * baseFilename = (char *)filename;
    char * extension = strstr(filename, ".");
    extension++;
    
    while(strstr(baseFilename, "/"))
    {
        baseFilename = strstr(baseFilename, "/") + 1;
    }
    
    strncpy(tempPath, baseFilename, extension - baseFilename - 1);
    
    NSString * fullPath = [[NSBundle mainBundle] pathForResource:[[NSString alloc] initWithCString:tempPath encoding:NSUTF8StringEncoding]
                                                          ofType:[[NSString alloc] initWithCString:extension encoding:NSUTF8StringEncoding]];
    
    if(fullPath == nil)
    {
        return 0;
    }
    
    stat([fullPath cStringUsingEncoding:NSASCIIStringEncoding], &st);
    
#elif _WIN32
	stat(filename, &st);
#else
    SHD_ASSERT(false);
#endif
    
    return st.st_size;
}

bool FileIO::readFile(const char * filename, uint8_t * buffer, size_t bufSize, size_t * bytesRead)
{

#ifdef __APPLE__

    memset(buffer, 0x0, bufSize);
    char tempPath[512]  = {'\0'};
    
    // Find filename without path and extension
    char * baseFilename = (char *)filename;
    char * extension = strstr(filename, ".");
    extension++;
    
    while(strstr(baseFilename, "/"))
    {
        baseFilename = strstr(baseFilename, "/") + 1;
    }
    
    strncpy(tempPath, baseFilename, extension - baseFilename - 1);
    
    NSString * path = [[NSBundle mainBundle] pathForResource:[[NSString alloc] initWithCString:tempPath encoding:NSUTF8StringEncoding]
                                                      ofType:[[NSString alloc] initWithCString:extension encoding:NSUTF8StringEncoding]];
    
    if(path == nil)
    {
        return false;
    }
    
    FILE * fin = fopen([path cStringUsingEncoding:NSASCIIStringEncoding], "r");
    if (fin == NULL)
        return false;
    
    if(bytesRead)
    {
        *bytesRead = fread(buffer, 1, bufSize, fin);
    }
    else
    {
        fread(buffer, 1, bufSize, fin);
    }
    
    fclose(fin);

#elif _WIN32
	FILE * fin = fopen(filename, "rb");
	if (fin == NULL)
		return false;

	if (bytesRead)
	{
		*bytesRead = fread(buffer, 1, bufSize, fin);
	}
	else
	{
		fread(buffer, 1, bufSize, fin);
	}

	fclose(fin);
#else
    
    SHD_ASSERT(false);

#endif
    
    return true;
}

bool FileIO::writeFile(const char * filename, uint8_t * buffer, size_t bufSize)
{

#ifdef _WIN32

	FILE * fileHandle = fopen(filename, "wb");
	if (fileHandle == NULL)
		return false;

	size_t bytesWritten = fwrite(buffer, sizeof(uint8_t), bufSize, fileHandle);
	fclose(fileHandle);

	if (bytesWritten != bufSize)
	{
		return false;
	}

#else

	SHD_ASSERT(false);

#endif

	return true;
}

bool FileIO::createDirectory(const char * path)
{

#ifdef _WIN32

	bool ret = CreateDirectoryA(path, nullptr);
	if (ret == false)
	{
		return false;
	}

#else

	SHD_ASSERT(false);

#endif

	return true;
}

bool FileIO::loadTexture(const char * filename, Texture * tex)
{
    int width = 0;
    int height = 0;
    int numComponents = 0;
    size_t bytesRead = 0;
    size_t buffSize = FileIO::getFileSize(filename);
    uint8_t * texBuffer = (uint8_t *)SHD_MALLOC(buffSize);
    SHD_ASSERT(texBuffer != nullptr);
    
    bool ret = FileIO::readFile(filename, texBuffer, buffSize, &bytesRead);
    if(ret == false)
    {
        SHD_FREE(texBuffer);
        return ret;
    }

    uint8_t * data = stbi_load_from_memory(texBuffer, (int)buffSize, &width, &height, &numComponents, 0);
    SHD_FREE(texBuffer);
    if(data == nullptr)
    {
        return false;
    }
    
    ret = loadTextureFromMemory(data, width, height, numComponents, tex);
    stbi_image_free(data);
    if(ret == false)
    {
        return ret;
    }
    
    return true;
}

bool FileIO::loadTextureFromMemory(const uint8_t * buff, uint32_t width, uint32_t height, uint32_t numComponents, Texture * tex)
{
    if(buff == nullptr || tex == nullptr || width < 1 || height < 1)
    {
        return false;
    }
    
#ifdef __APPLE__

    MTLPixelFormat pixelFormat;
    
    if(numComponents == 1)
    {
        pixelFormat = MTLPixelFormatA8Unorm;
    }
    else if(numComponents == 4)
    {
#ifdef SB_PLATFORM_IOS
        pixelFormat = MTLPixelFormatBGRA8Unorm;
#else
        pixelFormat = MTLPixelFormatRGBA8Unorm;
#endif
    }
    else
    {
        SHD_ASSERT(false);
    }
    
    MTLTextureDescriptor *pTexDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                        width:width
                                                                                       height:height
                                                                                    mipmapped:NO];
    if(!pTexDesc)
    {
        return false;
    }
    
    tex->textureHandle = [shd::Application::getInstance().renderer.getDevice() newTextureWithDescriptor:pTexDesc];
    if(tex == nil)
    {
        return false;
    }
    
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        
    [tex->textureHandle replaceRegion:region
                          mipmapLevel:0
                            withBytes:buff
                          bytesPerRow:width * numComponents];

#elif _WIN32

	DXGI_FORMAT pixelFormat;

	if (numComponents == 1)
	{
		pixelFormat = DXGI_FORMAT_A8_UNORM;
	}
	else if (numComponents == 4)
	{
		pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		SHD_ASSERT(false);
	}

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = pixelFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	ID3D11Texture2D *pTexture = NULL;
 	HRESULT hr = Application::getInstance().renderer.getDevice()->CreateTexture2D(&desc, NULL, &pTexture);
	if (FAILED(hr) || pTexture == nullptr)
	{
		return false;
	}

	D3D11_BOX destRegion;
	destRegion.left = 0;
	destRegion.right = width;
	destRegion.top = 0;
	destRegion.bottom = height;
	destRegion.front = 0;
	destRegion.back = 1;

	Application::getInstance().renderer.getDeviceContext()->UpdateSubresource(
		pTexture,
		0,
		&destRegion,
		buff,
		width * numComponents,
		width * height * numComponents);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	memset(&SRVDesc, 0, sizeof(SRVDesc));
	SRVDesc.Format = pixelFormat;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = Application::getInstance().renderer.getDevice()->CreateShaderResourceView(
		pTexture,
		&SRVDesc,
		&tex->textureHandle);
	if (FAILED(hr))
	{
		pTexture->Release();
		return false;
	}

	pTexture->Release();

#endif

    return true;
}

bool FileIO::loadSavedData()
{
	bool ret = false;
	rapidjson::Document document;

	char * envVarPath = nullptr;
	const size_t saveDataPathSize = 512;
	char saveDataPath[saveDataPathSize] = { '\0' };

	envVarPath = getenv(SHD_SAVE_DATA_ENV_VAR);
	if (envVarPath == nullptr)
	{
		return false;
	}

	strncpy(saveDataPath, envVarPath, saveDataPathSize);
	strncat(saveDataPath, SHD_SAVE_DATA_FILENAME, saveDataPathSize - strlen(saveDataPath));

	size_t jsonSize = FileIO::getFileSize(saveDataPath);
	if (jsonSize == 0)
	{
		return false;
	}

	char * json = (char *)SHD_MALLOC(jsonSize + 1);
	if (json == nullptr)
	{
		return false;
	}
	memset(json, 0, jsonSize + 1);

	ret = FileIO::readFile(saveDataPath, (uint8_t *)json, jsonSize, nullptr);
	if (ret == false)
	{
		SHD_FREE(json);
		return false;
	}

	// In-situ parsing, decode strings directly in the source string. Source must be string.
	if (document.ParseInsitu(json).HasParseError())
	{
		SHD_FREE(json);
		return false;
	}

	/*{
		"Version": "1.00",
			"vsyncEnabled" : false,
			"isFullscreen" : false,
			"maintainAspectRatio" : true,
			"showFpsCounter" : true,
			"audioVolumeMusic" : 10,
			"audioVolumeSFX" : 10,
			"teamColourPrimary" : 0,
			"teamColourSecondary" : 1,
			"teamColourPrimaryLocal" : 0,
			"teamColourSecondaryLocal" : 1,
			"screenResolution" : "800X600",
			"currentLanguage" : "en"
	}*/

	if (strncmp(document["Version"].GetString(), "1.00", strlen("1.00")) == 0)
	{
		if (document.HasMember("vsyncEnabled"))
		{
			Application::getInstance().globalSettings.vsyncEnabled = document["vsyncEnabled"].GetBool();
		}

		if (document.HasMember("isFullscreen"))
		{
			Application::getInstance().globalSettings.isFullscreen = document["isFullscreen"].GetBool();
			Application::getInstance().renderer.setFullscreen(Application::getInstance().globalSettings.isFullscreen);
		}

		if (document.HasMember("maintainAspectRatio"))
		{
			Application::getInstance().globalSettings.maintainAspectRatio = document["maintainAspectRatio"].GetBool();
		}

		if (document.HasMember("showFpsCounter"))
		{
			Application::getInstance().globalSettings.showFpsCounter = document["showFpsCounter"].GetBool();
		}

		if (document.HasMember("audioVolumeMusic"))
		{
			shd::Atomic::exchange32(&Application::getInstance().globalSettings.audioVolumeMusic, document["audioVolumeMusic"].GetInt());
		}

		if (document.HasMember("audioVolumeSFX"))
		{
			shd::Atomic::exchange32(&Application::getInstance().globalSettings.audioVolumeSFX, document["audioVolumeSFX"].GetInt());
		}

		if (document.HasMember("teamColourPrimary"))
		{
			Application::getInstance().globalSettings.teamColourPrimary = (Team::TeamColour)document["teamColourPrimary"].GetInt();
		}

		if (document.HasMember("teamColourSecondary"))
		{
			Application::getInstance().globalSettings.teamColourSecondary = (Team::TeamColour)document["teamColourSecondary"].GetInt();
		}

		if (document.HasMember("teamColourPrimaryLocal"))
		{
			Application::getInstance().globalSettings.teamColourPrimaryLocal = (Team::TeamColour)document["teamColourPrimaryLocal"].GetInt();
		}

		if (document.HasMember("teamColourSecondaryLocal"))
		{
			Application::getInstance().globalSettings.teamColourSecondaryLocal = (Team::TeamColour)document["teamColourSecondaryLocal"].GetInt();
		}

		if (document.HasMember("screenResolution"))
		{
			if (strncmp(document["screenResolution"].GetString(), SHD_SAVE_DATA_RESOLUTION_STR_DEFAULT, strlen(SHD_SAVE_DATA_RESOLUTION_STR_DEFAULT)) == 0)
			{
				Application::getInstance().globalSettings.screenResolution = Renderer::SCREEN_RES_DEFAULT;
			}
			else if (strncmp(document["screenResolution"].GetString(), SHD_SAVE_DATA_RESOLUTION_STR_3840_X_2160, strlen(SHD_SAVE_DATA_RESOLUTION_STR_3840_X_2160)) == 0)
			{
				Application::getInstance().globalSettings.screenResolution = Renderer::SCREEN_RES_3840_X_2160;
			}
			else if (strncmp(document["screenResolution"].GetString(), SHD_SAVE_DATA_RESOLUTION_STR_2560_X_1440, strlen(SHD_SAVE_DATA_RESOLUTION_STR_2560_X_1440)) == 0)
			{
				Application::getInstance().globalSettings.screenResolution = Renderer::SCREEN_RES_2560_X_1440;
			}
			else if (strncmp(document["screenResolution"].GetString(), SHD_SAVE_DATA_RESOLUTION_STR_1920_X_1080, strlen(SHD_SAVE_DATA_RESOLUTION_STR_1920_X_1080)) == 0)
			{
				Application::getInstance().globalSettings.screenResolution = Renderer::SCREEN_RES_1920_X_1080;
			}
			else if (strncmp(document["screenResolution"].GetString(), SHD_SAVE_DATA_RESOLUTION_STR_1280_X_720, strlen(SHD_SAVE_DATA_RESOLUTION_STR_1280_X_720)) == 0)
			{
				Application::getInstance().globalSettings.screenResolution = Renderer::SCREEN_RES_1280_X_720;
			}
			else if (strncmp(document["screenResolution"].GetString(), SHD_SAVE_DATA_RESOLUTION_STR_800_X_600, strlen(SHD_SAVE_DATA_RESOLUTION_STR_800_X_600)) == 0)
			{
				Application::getInstance().globalSettings.screenResolution = Renderer::SCREEN_RES_800_X_600;
			}

			switch (Application::getInstance().globalSettings.screenResolution)
			{
			case Renderer::SCREEN_RES_DEFAULT:
				Application::getInstance().renderer.resize(100, 100, true);
				break;
			case Renderer::SCREEN_RES_3840_X_2160:
				Application::getInstance().renderer.resize(3840, 2160, true);
				break;
			case Renderer::SCREEN_RES_2560_X_1440:
				Application::getInstance().renderer.resize(2560, 1440, true);
				break;
			case Renderer::SCREEN_RES_1920_X_1080:
				Application::getInstance().renderer.resize(1920, 1080, true);
				break;
			case Renderer::SCREEN_RES_1280_X_720:
				Application::getInstance().renderer.resize(1280, 720, true);
				break;
			case Renderer::SCREEN_RES_800_X_600:
				Application::getInstance().renderer.resize(800, 600, true);
				break;
			default:
				SHD_ASSERT(false);
				break;
			}
		}
	}

	SHD_FREE(json);

	return true;
}

bool FileIO::saveSavedData()
{
	bool ret = false;
	
	const size_t resMaxSize = 32;
	char resolution[resMaxSize] = { '\0' };

	const size_t langMaxSize = 32;
	char language[langMaxSize] = { '\0' };

	const size_t saveDataMaxSize = 1024;
	char saveDataBuff[saveDataMaxSize] = { '\0' };

	char * envVarPath = nullptr;
	const size_t saveDataPathSize = 512;
	char saveDataPath[saveDataPathSize] = { '\0' };

	envVarPath = getenv(SHD_SAVE_DATA_ENV_VAR);
	if (envVarPath == nullptr)
	{
		return false;
	}

	strncpy(saveDataPath, envVarPath, saveDataPathSize);
	strncat(saveDataPath, SHD_SAVE_DATA_DIRECTORY, saveDataPathSize - strlen(saveDataPath));

	// We need to make sure that we create the directory if it doesn't already exist
	FileIO::createDirectory(saveDataPath);

	// Convert the resolution enum into a string
	switch (Application::getInstance().globalSettings.screenResolution)
	{
	case Renderer::SCREEN_RES_DEFAULT:
		strncpy(resolution, SHD_SAVE_DATA_RESOLUTION_STR_DEFAULT, resMaxSize - 1);
		break;
	case Renderer::SCREEN_RES_3840_X_2160:
		strncpy(resolution, SHD_SAVE_DATA_RESOLUTION_STR_3840_X_2160, resMaxSize - 1);
		break;
	case Renderer::SCREEN_RES_2560_X_1440:
		strncpy(resolution, SHD_SAVE_DATA_RESOLUTION_STR_2560_X_1440, resMaxSize - 1);
		break;
	case Renderer::SCREEN_RES_1920_X_1080:
		strncpy(resolution, SHD_SAVE_DATA_RESOLUTION_STR_1920_X_1080, resMaxSize - 1);
		break;
	case Renderer::SCREEN_RES_1280_X_720:
		strncpy(resolution, SHD_SAVE_DATA_RESOLUTION_STR_1280_X_720, resMaxSize - 1);
		break;
	case Renderer::SCREEN_RES_800_X_600:
		strncpy(resolution, SHD_SAVE_DATA_RESOLUTION_STR_800_X_600, resMaxSize - 1);
		break;
	default:
		SHD_ASSERT(false);
		strncpy(resolution, SHD_SAVE_DATA_RESOLUTION_STR_DEFAULT, resMaxSize - 1);
	}

	// Convert the language enum into a string
	switch (Application::getInstance().globalSettings.currentLanguage)
	{
	case FontManager::SHD_LANG_ENGLISH:
		strncpy(language, SHD_SAVE_DATA_LANGUAGE_STR_EN, langMaxSize - 1);
		break;
	case FontManager::SHD_LANG_JAPANESE:
		strncpy(language, SHD_SAVE_DATA_LANGUAGE_STR_JP, langMaxSize - 1);
		break;
	default:
		SHD_ASSERT(false);
		strncpy(language, SHD_SAVE_DATA_LANGUAGE_STR_EN, langMaxSize - 1);
	}

	snprintf(	saveDataBuff,
				saveDataMaxSize - 1,
				SHD_SAVE_DATA_FORMAT_V1_00,
				(Application::getInstance().globalSettings.vsyncEnabled) ? "true" : "false",
				(Application::getInstance().globalSettings.isFullscreen) ? "true" : "false",
				(Application::getInstance().globalSettings.maintainAspectRatio) ? "true" : "false",
				(Application::getInstance().globalSettings.showFpsCounter) ? "true" : "false",
				Application::getInstance().globalSettings.audioVolumeMusic,
				Application::getInstance().globalSettings.audioVolumeSFX,
				Application::getInstance().globalSettings.teamColourPrimary,
				Application::getInstance().globalSettings.teamColourSecondary,
				Application::getInstance().globalSettings.teamColourPrimaryLocal,
				Application::getInstance().globalSettings.teamColourSecondaryLocal,
				resolution,
				language);

	strncpy(saveDataPath, envVarPath, saveDataPathSize);
	strncat(saveDataPath, SHD_SAVE_DATA_FILENAME, saveDataPathSize - strlen(saveDataPath));

	ret = FileIO::writeFile(saveDataPath, (uint8_t *)saveDataBuff, strlen(saveDataBuff));
	if (ret == false)
	{
		return false;
	}

	return true;
}