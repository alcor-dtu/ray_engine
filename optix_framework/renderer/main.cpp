// 02576 OptiX Rendering Framework
// Written by Jeppe Revall Frisvad, 2011
// Copyright (c) DTU Informatics 2011
#include "optix_serialize.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <GLFWDisplay.h>
#include "obj_scene.h"
#include "render_task.h"
#include "scattering_material.h"
#include "sky_model.h"
#include "immediate_gui.h"

#include "hemisphere_partitions.h"
namespace
{
	void lower_case(char& x)
  { 
    x = tolower(x); 
  }

	inline void lower_case_string(std::string& s)
	{
    for_each(s.begin(), s.end(), lower_case);
	}
}

void printUsageAndExit( const std::string& argv0 ) 
{
	std::cerr << "Usage  : " << argv0 << " [options] any_object.obj [another.obj ...]" << std::endl
       << "options: --help            | -h            Print this usage message" << std::endl
       << "         --shader         | -sh <shader>  specify the closest hit program to be used for shading" << std::endl
	   << "options: --rectangle <ox oy w h>     renctangle to render." << std::endl
	   << std::endl;

  GLFWDisplay::printUsage();

  exit(0);
}


int main( int argc, char** argv ) 
{
	std::vector<float> r;
	std::vector<int> i;
	get_hemisphere_subdivisions(288, r, i);
	std::vector<std::string> filenames;
	std::string filename = "";
	std::string output_file = "rendering.raw";
	std::string config_file = "config.xml";
	bool accel_caching_on = false;
	optix::int4 rendering_rect = optix::make_int4(-1);
	//std::map<std::string, std::string> parameters;
	bool auto_mode = false;
	int frames = -1;
	float time = -1.0f;
	std::string material_override_mtl = "";
	std::vector<std::string> additional_parameters;
	bool nodisplay = false;

	for ( int i = 1; i < argc; ++i )
	{
	std::string arg( argv[i]);
	if (arg == "--no-display")
	{
		nodisplay = true;
	}
	if( arg == "-h" || arg == "--help" )
	{
		printUsageAndExit( argv[0] ); 
	}
	else if (arg == "-c" || arg == "--config")
	{
		if (i == argc - 1)
			printUsageAndExit(argv[0]);
		config_file = argv[++i];
		lower_case_string(config_file);
	}
	else if (arg == "-o" || arg == "--output")
	{
		if (i == argc - 1)
			printUsageAndExit(argv[0]);
		auto_mode = true;
		output_file = argv[++i];
		lower_case_string(output_file);
	}
	else if (arg == "--material_override")
	{
		if (i == argc - 1)
			printUsageAndExit(argv[0]);
		material_override_mtl = argv[++i];
		lower_case_string(material_override_mtl);
	}
	else if (arg == "--parameter_override")
	{
		do
		{
			i++;
			additional_parameters.push_back(std::string(argv[i]));
		}
		while (i+1 < argc && std::string(argv[i+1])[0] != '-');
	}
	else if (arg == "-t" || arg == "--time")
	{
		auto_mode = true;
		time = std::stof(argv[++i]);
	}
	else if (arg == "-f" || arg == "--frames")
	{
		auto_mode = true;
		frames = std::stoi(argv[++i]);
	}
	else if (arg == "--rectangle")
	{
		if (i == argc - 1)
				printUsageAndExit( argv[0] );
		rendering_rect.x = std::stoi(argv[++i]);
		if (i == argc - 1)
			printUsageAndExit(argv[0]);
		rendering_rect.y = std::stoi(argv[++i]);
		if (i == argc - 1)
			printUsageAndExit(argv[0]);
		rendering_rect.z = std::stoi(argv[++i]);
		if (i == argc - 1)
			printUsageAndExit(argv[0]);
		rendering_rect.w = std::stoi(argv[++i]);
	}
	else 
	{
		filename = argv[i];
		std::string file_extension;
		size_t idx = filename.find_last_of('.');
		if(idx < filename.length())
		{
			file_extension = filename.substr(idx, filename.length() - idx);
			lower_case_string(file_extension);
		}

		if(file_extension == ".obj")
		{
			filenames.push_back(filename);
			lower_case_string(filenames.back());
		}
	}
	}
	//if ( filenames.size() == 0 ) 
	//  filenames.push_back(string("./meshes/") + "closed_bunny_vn.obj");
	ConfigParameters::init(config_file);
	ConfigParameters::override_parameters(additional_parameters);

	GLFWDisplay::setRequiresDisplay(!nodisplay);
	GLFWDisplay::init( argc, argv );
	
	std::unique_ptr<RenderTask> task = nullptr;
	if (auto_mode)
	{
		if(frames > 0 && time > 0)
		{
			task = std::make_unique<RenderTaskTimeorFrames>(frames, time, output_file, true);
		}
		else if (frames > 0)
		{
			task = std::make_unique<RenderTaskFrames>(frames, output_file, true);
		}
		else if (time > 0.0f)
		{
			task = std::make_unique<RenderTaskTime>(time, output_file, true);
		}
		else
		{
			Logger::error << "Time or frames not specified" << std::endl;
			exit(2);
		}
	}


	try 
	{
		ObjScene * scene_o = new ObjScene(filenames, rendering_rect);
		SampleScene * scene = scene_o;
		if (material_override_mtl.size() > 0)
			scene_o->add_override_material_file(material_override_mtl);
		if (auto_mode)
		{
			scene_o->set_render_task(task);
			scene_o->start_render_task_on_scene_ready();
		}	

		GLFWDisplay::run( "Optix Renderer", scene );

	} 
	catch(optix::Exception & e )
	{
	Logger::error<<  (e.getErrorString().c_str());
	exit(1);
	}
	return 0;
}
