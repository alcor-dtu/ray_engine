#include "presampled_surface_bssrdf_shader.h"
#include "material_library.h"
#include "scattering_material.h"
#include "immediate_gui.h"
#include "optix_host_utils.h"
#include "object_host.h"

using optix::Buffer;
using optix::float3;
using optix::int3;

PresampledSurfaceBssrdf::PresampledSurfaceBssrdf(PresampledSurfaceBssrdf & copy) : Shader(copy)
{
	entry_point = copy.entry_point; // They can share the same entry point 
	mSampleBuffer = clone_buffer(copy.mSampleBuffer, RT_BUFFER_INPUT);
	mSamples = 1000;
	mArea = copy.mArea;
	mCdfBuffer = clone_buffer(copy.mCdfBuffer, RT_BUFFER_INPUT);
	auto type = copy.mBSSRDF->get_type();
	mBSSRDF = BSSRDF::create(context, type);
}

void PresampledSurfaceBssrdf::initialize_shader(optix::Context ctx)
{
    Shader::initialize_shader(ctx);
    //in static constructor

    std::string ptx_path = Folders::get_path_to_ptx("sample_camera.cu");
    optix::Program ray_gen_program = context->createProgramFromPTXFile( ptx_path, "sample_camera" );
	
    entry_point = add_entry_point(context, ray_gen_program);
    
    // We tell optix we "are doing stuff" in the context with these names --> otherwise compilation fails
    Buffer empty_buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, 1);
    Buffer empty_bufferi3 = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, 1);
    
    context["sampling_vertex_buffer"]->setBuffer(empty_buffer);
    context["sampling_normal_buffer"]->setBuffer(empty_buffer);
    context["sampling_vindex_buffer"]->setBuffer(empty_bufferi3);
    context["sampling_nindex_buffer"]->setBuffer(empty_bufferi3);
	context["exclude_backfaces"]->setInt(mExcludeBackFaces? 0 : 1);

    mSampleBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT);
    mSampleBuffer->setFormat(RT_FORMAT_USER);
    mSampleBuffer->setElementSize(sizeof(PositionSample));
    mSampleBuffer->setSize(mSamples);

	auto s = ScatteringDipole::DIRECTIONAL_DIPOLE_BSSRDF; // FIXME SERIALIZE
	mBSSRDF = BSSRDF::create(context, s);
	mCdfBuffer = context->createBuffer(RT_BUFFER_INPUT);
}

void PresampledSurfaceBssrdf::initialize_material(MaterialHost &object)
{

	ScatteringDipole::Type t = mBSSRDF->get_type();
	object.get_optix_material()["selected_bssrdf"]->setUserData(sizeof(ScatteringDipole::Type), &t);

    Shader::initialize_material(object);
}


void PresampledSurfaceBssrdf::pre_trace_mesh(Object& obj)
{
    optix::Geometry g = obj.get_geometry()->get_geometry();
    // precompute triangle area cdf
    static bool initialized = false;

    if(!initialized)
    {
        initialized = true;
        Buffer v_buff = g["vertex_buffer"]->getBuffer();
        Buffer v_idx_buff = g["vindex_buffer"]->getBuffer();
        float3 *vbuffer_data = static_cast<float3 *>(v_buff->map());
        int3 *vbuffer_idx_data = static_cast<int3 *>(v_idx_buff->map());
        RTsize buffer_width;
        v_idx_buff->getSize(buffer_width);
        unsigned int triangles = static_cast<unsigned int>(buffer_width);

        float *cdf = new float[triangles];
        float totalArea = 0.0f;
        for (unsigned int i = 0; i < triangles; ++i)
        {
            int3 index = vbuffer_idx_data[i];
            float3 v0 = vbuffer_data[index.x];
            float3 v1 = vbuffer_data[index.y];
            float3 v2 = vbuffer_data[index.z];
            float area = 0.5f * length(cross(v1 - v0, v2 - v0));
            cdf[i] = area + (i > 0 ? cdf[i - 1] : 0.0f);
            totalArea += area;
        }
        v_idx_buff->unmap();
        v_buff->unmap();
        mArea = totalArea;
        std::cout << "Area: " << totalArea << std::endl;

        for (unsigned int i = 0; i < triangles; ++i)
        {
            cdf[i] /= totalArea;
        }
        mCdfBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, triangles);
        memcpy(mCdfBuffer->map(), cdf, triangles * sizeof(float));
        mCdfBuffer->unmap();
        delete[] cdf;
    }

    auto buf = BufPtr<PositionSample>(mSampleBuffer->getId());
    optix::GeometryInstance object = obj.get_geometry_instance();
    object["sampling_output_buffer"]->setUserData(sizeof(BufPtr<PositionSample>), &buf);
    object["total_area"]->setFloat(mArea);
    object["area_cdf"]->setBuffer(mCdfBuffer);
    context["sampling_vertex_buffer"]->setBuffer(g["vertex_buffer"]->getBuffer());
    context["sampling_normal_buffer"]->setBuffer(g["normal_buffer"]->getBuffer());
    context["sampling_vindex_buffer"]->setBuffer(g["vindex_buffer"]->getBuffer());
    context["sampling_nindex_buffer"]->setBuffer(g["nindex_buffer"]->getBuffer());
	context["sampling_output_buffer"]->setUserData(sizeof(BufPtr<PositionSample>), &buf);
    context["bssrdf_enabled"]->setUint(0);	// Disabling BSSRDF computation on sample collection.
    context->launch(entry_point, mSamples);
    context["bssrdf_enabled"]->setUint(1);

}

void PresampledSurfaceBssrdf::load_data(MaterialHost &obj)
{
    mBSSRDF->load(obj.get_data().index_of_refraction, obj.get_data().scattering_properties);
    ScatteringDipole::Type t = mBSSRDF->get_type();
    obj.get_optix_material()["selected_bssrdf"]->setUserData(sizeof(ScatteringDipole::Type), &t);
}

bool PresampledSurfaceBssrdf::on_draw()
{
	bool changed = false;

	static ScatteringDipole::Type dipole = mBSSRDF->get_type();
	if (BSSRDF::bssrdf_selection_gui(dipole))
	{
		changed = true;
		mBSSRDF.reset();
		mBSSRDF = BSSRDF::create(context, dipole);
	}
	if(ImmediateGUIDraw::Checkbox("Exclude backfaces", &mExcludeBackFaces))
	{
		context["exclude_backfaces"]->setInt(mExcludeBackFaces? 0 : 1);
	}

	ImmediateGUIDraw::Text("BSSRDF properties:");
	mBSSRDF->on_draw();
	if (ImmediateGUIDraw::InputInt("Area samples", (int*)&mSamples))
	{
		changed = true;
		mSampleBuffer->setSize(mSamples);
	}
	return changed;
}
