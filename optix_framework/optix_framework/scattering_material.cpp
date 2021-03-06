#include "scattering_material.h"
#include "optics_utils.h"
#include <functional>
#include "immediate_gui.h"
#include "sampling_helpers.h"
#include <algorithm>

#include "string_utils.h"


ScatteringMaterial::ScatteringMaterial(optix::float3 absorption, optix::float3 scattering, optix::float3 meancosine, float scale, const char * name)
	: scale(scale), name(name)
{
	this->absorption = absorption;
	this->scattering = scattering;
	this->asymmetry = meancosine;
	mStandardMaterial = DefaultScatteringMaterial::Count; // Custom
	dirty = true;
}

ScatteringMaterial::ScatteringMaterial(DefaultScatteringMaterial material, float prop_scale)
{
	scale = prop_scale;
	mStandardMaterial = static_cast<int>(material);
	getDefaultMaterial(material);
	dirty = true;
}

ScatteringMaterial& ScatteringMaterial::operator=(const ScatteringMaterial& cp)
{
    absorption = cp.absorption;
    scattering = cp.scattering;
    asymmetry = cp.asymmetry;
    name = cp.name;
    scale = cp.scale;
    dirty = true;
    mStandardMaterial = cp.mStandardMaterial;
    return *this;
}

ScatteringMaterial::ScatteringMaterial(const ScatteringMaterial& cp)
{
    absorption = cp.absorption;
    scattering = cp.scattering;
    asymmetry = cp.asymmetry;
    name = cp.name;
    scale = cp.scale;
    mStandardMaterial = cp.mStandardMaterial;
	dirty = true;
}

void ScatteringMaterial::initializeDefaultMaterials()
{
	defaultMaterials.clear();
    std::vector<ScatteringMaterial> res;
    for (int i = 0; i < DefaultScatteringMaterial::Count; i++)
    {
		defaultMaterials.push_back(ScatteringMaterial(static_cast<DefaultScatteringMaterial>(i)));
    }
}

std::vector<ScatteringMaterial> ScatteringMaterial::defaultMaterials;

void ScatteringMaterial::getDefaultMaterial(DefaultScatteringMaterial material)
{
  absorption = optix::make_float3(0.0f);
  scattering = optix::make_float3(0.0f);
  asymmetry = optix::make_float3(0.0f);
  scale = 100.0f;

  switch(material)
  {
  case Chicken:
    absorption = optix::make_float3(0.015f, 0.077f, 0.19f);
    scattering = optix::make_float3(0.15f, 0.21f, 0.38f);
    asymmetry = optix::make_float3(0.0f, 0.0f, 0.0f);
    name = "chicken";
  break;
  case Skin:
    absorption = optix::make_float3(0.032f, 0.17f, 0.48f);
    scattering = optix::make_float3(0.74f, 0.88f, 1.01f);
    asymmetry = optix::make_float3(0.0f, 0.0f, 0.0f);
    name = "skin";
    break;
  case Wholemilk:
    absorption = optix::make_float3(0.0011f,0.0024f,0.014f);
    scattering = optix::make_float3(2.55f,3.21f,3.77f);
    asymmetry = optix::make_float3(0.0f, 0.0f, 0.0f);
    name = "whole_milk";
    break;
  case Whitegrapefruit:
    absorption = optix::make_float3(0.096f, 0.131f, 0.395f);
    scattering = optix::make_float3(3.513f, 3.669f, 5.237f);
    asymmetry = optix::make_float3(0.548f, 0.545f, 0.565f);
    name = "white_grapefruit";
  break;
  case Beer:
    absorption = optix::make_float3(0.1449f,0.3141f,0.7286f);
    scattering = optix::make_float3(0.0037f,0.0069f,0.0074f);
    asymmetry = optix::make_float3(0.917f, 0.956f, 0.982f);
    name = "beer";
  break;
  case Soymilk:
    absorption = optix::make_float3(0.0001f,0.0005f,0.0034f);
    scattering = optix::make_float3(2.433f,2.714f,4.563f);
    asymmetry = optix::make_float3(0.873f, 0.858f, 0.832f);
    name = "soy_milk";
  break;
  case Coffee:
    absorption = optix::make_float3(0.1669f,0.2287f,0.3078f);
    scattering = optix::make_float3(0.2707f,0.2828f,0.297f);
    asymmetry = optix::make_float3(0.907f, 0.896f, 0.88f);
    name = "coffee";
  break;
  case Marble:
    absorption = optix::make_float3(0.0021f,0.0041f,0.0071f);
    scattering = optix::make_float3(2.19f,2.62f,3.00f);
    asymmetry = optix::make_float3(0.0f, 0.0f, 0.0f);
    name = "marble";
  break;
  case Potato:
    absorption = optix::make_float3(0.0024f,0.0090f,0.12f);
    scattering = optix::make_float3( 0.68f,0.70f,0.55f);
    asymmetry = optix::make_float3(0.0f, 0.0f, 0.0f);
    name = "potato";
  break;
  case Ketchup:
    absorption = optix::make_float3(0.061f,0.97f,1.45f);
    scattering = optix::make_float3(0.18f,0.07f,0.03f);
    asymmetry = optix::make_float3(0.0f, 0.0f, 0.0f);
    name = "ketchup";
  break;
  case Apple:
    absorption = optix::make_float3(0.0030f,0.0034f,0.0046f);
    scattering = optix::make_float3(2.29f,2.39f,1.97f);
    asymmetry = optix::make_float3(0.0f, 0.0f, 0.0f);
    name = "apple";
  break;
    case ChocolateMilk:
    absorption = optix::make_float3(0.007f, 0.03f, 0.1f);
    scattering = optix::make_float3(7.352f, 9.142f, 10.588f);
    asymmetry = optix::make_float3(0.862f, 0.838f, 0.806f);
	scale = 10.f;
    name = "chocolate_milk";
  break;
    case ReducedMilk:
    absorption = optix::make_float3(0.0001f, 0.0002f, 0.0005f);
    scattering = optix::make_float3(10.748f, 12.209f, 13.931f);
    asymmetry = optix::make_float3(0.819f, 0.797f, 0.746f);
    name = "reduced_milk";
  break;
    case Mustard:
    scattering = optix::make_float3(16.447f,18.536f,6.457f);
    absorption = optix::make_float3(0.057f,0.061f,0.451f);
    asymmetry = optix::make_float3(0.155f, 0.173f, 0.351f);
	scale = 1;
    name = "mustard";
    break;
    case Shampoo:
    scattering = optix::make_float3(8.111f,9.919f,10.575f);
    absorption = optix::make_float3(0.178f,0.328f,0.439f);
    asymmetry = optix::make_float3(0.907f, 0.882f, 0.874f);
    name = "shampoo";
    break;
    case MixedSoap:
    scattering = optix::make_float3(3.923f, 4.018f, 4.351f);
    absorption = optix::make_float3(0.003f, 0.005f, 0.013f);
    asymmetry = optix::make_float3(0.330f, 0.322f, 0.316f);
    name = "mixed_soap";
    break;
    case GlycerineSoap:
    scattering = optix::make_float3(0.201f, 0.202f, 0.221f);
    absorption = optix::make_float3(0.001f, 0.001f, 0.002f);
    asymmetry = optix::make_float3(0.955f, 0.949f, 0.943f);
    name = "glycerine_soap";
    break;
  case Count: break;
  default: ;
  }
  dirty = true;
}

void ScatteringMaterial::set_absorption(optix::float3 abs)
{
	absorption = abs;
	dirty = true;
}

void ScatteringMaterial::set_scattering(optix::float3 sc)
{
	scattering = sc;
	dirty = true;
}

void ScatteringMaterial::set_asymmetry(float asymm)
{
	asymmetry = optix::make_float3(asymm);
	dirty = true;
}

bool ScatteringMaterial::on_draw(std::string id)
{
	ImGui::Separator();
	ImGui::Text("Scattering properties:");

	std::vector<std::string> vv;
	for (int i = 0; i < defaultMaterials.size(); i++)
	{
		vv.push_back(defaultMaterials[i].name);
	}
	vv.push_back("Custom");
	std::vector<const char*> v;
	for (auto& c : vv) v.push_back(c.c_str());
	static int mat = mStandardMaterial;
#define ID_STRING(x,id) (std::string(x) + "##" + id + x).c_str()

	if (ImmediateGUIDraw::InputFloat3(ID_STRING("Absorption", id), (float*)&absorption))
	{
		dirty = true;
		mat = DefaultScatteringMaterial::Count;
	}
	if (ImmediateGUIDraw::InputFloat3(ID_STRING("Scattering", id), (float*)&scattering))
	{
		dirty = true;
		mat = DefaultScatteringMaterial::Count;
	}

	if (ImmediateGUIDraw::InputFloat3(ID_STRING("Asymmetry", id), (float*)&asymmetry))
	{
		dirty = true;
		mat = DefaultScatteringMaterial::Count;
	}

	if (ImmediateGUIDraw::InputFloat(ID_STRING("Scale", id), (float*)&scale))
	{
		dirty = true;
	}
	

	if (ImmediateGUIDraw::Combo(ID_STRING("Change material", id), &mat, v.data(), (int)v.size(), (int)v.size()))
	{
		if (mat < DefaultScatteringMaterial::Count)
		{
			dirty = true;
			mStandardMaterial = static_cast<int>(mat);
			getDefaultMaterial(static_cast<DefaultScatteringMaterial>(mat));
		}
	}


	return dirty;
#undef ID_STRING
}



void ScatteringMaterial::computeCoefficients(float ior)
{
	fill_scattering_parameters(properties, scale, ior, absorption, scattering, asymmetry);

	dirty = false;
}

bool ScatteringMaterial::hasChanged()
{
	return dirty;
}

ScatteringMaterialProperties ScatteringMaterial::get_data()
{
    return properties;
}