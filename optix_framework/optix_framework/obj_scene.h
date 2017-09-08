// 02576 OptiX Rendering Framework
// Written by Jeppe Revall Frisvad, 2011
// Copyright (c) DTU Informatics 2011

#ifndef OBJSCENE_H
#define OBJSCENE_H

#include <string>
#include <vector>
#include <optixu/optixu_matrix_namespace.h>
#include <optixu/optixpp_namespace.h>
#include <SampleScene.h>
#include "rendering_method.h"
#include "parameter_parser.h"
#include "sky_model.h"
#include "area_light.h"
#include "mesh.h"
#include "enums.h"
#include "immediate_gui.h"

//#define IOR_EST


#ifdef IOR_EST
#include "indexofrefractionmatcher.h"
#endif

#include "structs.h"
#include <functional>
#include "camera.h"
#include "camera_host.h"

class ObjScene : public SampleScene
{
public:


	ObjScene(const std::vector<std::string>& obj_filenames, const std::string& shader_name, const std::string& config_file, optix::int4 rendering_r = make_int4(-1))
        : context(m_context),
          current_scene_type(Scene::OPTIX_ONLY), current_miss_program(), filenames(obj_filenames), method(nullptr),        m_frame(0u),
          deforming(false),
          config_file(config_file)
    {
        calc_absorption[0] = calc_absorption[1] = calc_absorption[2] = 0.0f;
		custom_rr = rendering_r;
        mMeshes.clear();
    }

	ObjScene()
		: context(m_context),
		current_scene_type(Scene::OPTIX_ONLY), filenames(1, "test.obj"), 
		  m_frame(0u),
		  deforming(true),
		  config_file("config.xml")
	{
		calc_absorption[0] = calc_absorption[1] = calc_absorption[2] = 0.0f;
        mMeshes.clear();
		custom_rr = make_int4(-1);
	}

	virtual ~ObjScene()
	{
		ParameterParser::free();
		cleanUp();
	}

	virtual void cleanUp()
	{
		context->destroy();
		m_context = 0;
	}

	bool drawGUI();

    void create_3d_noise(float frequency);
    float noise_frequency = 25;
	int use_heterogenous_materials = 0;

	virtual void initScene(InitialCameraData& camera_data) override;
	virtual void trace(const RayGenCameraData& camera_data, bool& display) override;

	virtual void trace(const RayGenCameraData& camera_data) override
	{
		bool display = true;
		trace(camera_data, display);
	}

	void collect_image(unsigned int frame);
	virtual bool keyPressed(unsigned char key, int x, int y) override;
	virtual optix::Buffer getOutputBuffer() override;
	virtual void doResize(unsigned int width, unsigned int height) override;
	virtual void resize(unsigned int width, unsigned int height) override;
	void postDrawCallBack() override;
	void setDebugPixel(int i, int y);
	bool mousePressed(int button, int state, int x, int y) override;
	bool mouseMoving(int x, int y) override;
	void reset_renderer();

#ifdef IOR_EST
	IndexMatcher * index_matcher;
#endif

	virtual void getDebugText(std::string& text, float& x, float& y) override
	{
		text = "";// Scene::Enum2String(current_scene_type);
		x = 10.0f;
		y = 36.0f;
	}

	void setAutoMode();
	void setOutputFile(const string& cs);
	void setFrames(int frames);

private:
	Context context;
	bool debug_mode_enabled = true;

	Scene::EnumType current_scene_type;

	BackgroundType::EnumType current_miss_program;
	bool collect_images = false;
	bool show_difference_image = false;
	Aabb m_scene_bounding_box;
	bool mAutoMode = false;
	string mOutputFile = "rendering.raw";
	int mFrames = -1;
	optix::Buffer createPBOOutputBuffer(const char* name, RTformat format, RTbuffertype type, unsigned width, unsigned height);

	void add_lights(vector<TriangleLight>& area_lights);
	void set_miss_program();
	optix::TextureSampler environment_sampler;
    std::unique_ptr<MissProgram> miss_program = nullptr;

	// Geometry and transformation getters
	optix::GeometryGroup get_geometry_group(unsigned int idx);
	optix::Matrix4x4 get_object_transform(std::string filename);

	bool export_raw(std::string& name);
	void set_rendering_method(RenderingMethodType::EnumType t);
	std::vector<std::string> filenames;

	Group scene;
	//std::vector<optix::uint2> lights;
	RenderingMethod* method;

	unsigned int m_frame;
	bool deforming;
	std::unique_ptr<Camera> camera = nullptr;
	optix::Buffer output_buffer;
	
   
	ImmediateGUI* new_gui = nullptr;
	void add_result_image(const string& image_file);
    std::vector<std::unique_ptr<Mesh>> mMeshes;
    std::shared_ptr<MaterialHost> material_ketchup;

    void execute_on_scene_elements(function<void(Mesh&)> operation);

	void setDebugEnabled(bool var);
	float tonemap_multiplier = 1.0f;
	float tonemap_exponent = 1.0f;

	optix::float3 global_absorption_override = optix::make_float3(0.0f);
	float global_absorption_inv_multiplier = 1.0f;
	float global_ior_override = 1.5f;
	float calc_absorption[3];
	void updateGlassObjects();
	std::vector<MPMLMedium*> available_media;
	int current_medium = 0;

	optix::Buffer rendering_output_buffer;
	optix::Buffer tonemap_output_buffer;
	optix::Buffer debug_output_buffer;
	optix::Buffer returned_buffer;

	int mtx_method = 1;
	
	optix::TextureSampler comparison_image;
	float comparison_image_weight = 0.0;

	void load_camera_extrinsics(InitialCameraData & data);

	const std::string config_file = "config.xml";
	int4 custom_rr;

	uint4 zoom_debug_window = make_uint4(20,20,300,300);
	uint4 zoomed_area = make_uint4(0);
};

#endif // OBJSCENE_H
