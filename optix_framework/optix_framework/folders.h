#pragma once
#include <string>
#include "optix_serialize.h"
#include <fstream>

inline bool exists(const char *fileName)
{
    // Check if a file exists
    std::ifstream infile(fileName);
    return infile.good();
}

class Folders
{
public:
	static std::string data_folder;
	static std::string ptx_path;
};

static const char* const get_path_ptx(const std::string& base)
{
	static std::string path;
	path = Folders::ptx_path + "/framework_generated_" + base + ".ptx";
	return path.c_str();
}



