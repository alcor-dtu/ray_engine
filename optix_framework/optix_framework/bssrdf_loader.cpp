#include "bssrdf_loader.h"
#include <fstream>
#include <sstream>
#include "logger.h"
#include "parserstringhelpers.h"

BSSRDFLoader::BSSRDFLoader(const std::string & filename)
{
	mFileName = filename;
	if(!parse_header())
	{
		Logger::error << "BSSRDF header parsing failed." << std::endl;
	}
}

void BSSRDFLoader::get_dimensions(std::vector<size_t>& dimensions)
{
	dimensions.clear();
	dimensions.insert(dimensions.begin(), mDimensions.begin(), mDimensions.end());
}

size_t BSSRDFLoader::get_material_slice_size()
{
	return get_hemisphere_size() * mDimensions[albedo_index] * mDimensions[g_index] * mDimensions[eta_index];
}

size_t BSSRDFLoader::get_hemisphere_size()
{
	return mDimensions[phi_o_index] * mDimensions[theta_o_index];
}

void BSSRDFLoader::load_material_slice(float * bssrdf_data, const std::vector<size_t>& idx)
{
	if (idx.size() != 3)
		Logger::error << "Material index is 3 dimensional." << std::endl;
	size_t pos = flatten_index({ idx[0], idx[1], idx[2], 0, 0, 0, 0, 0 }, mDimensions) * sizeof(float);
	std::ifstream ifs;
	ifs.open(mFileName, std::ofstream::in | std::ofstream::binary);
	ifs.seekg(pos + mBSSRDFStart);
	ifs.read(reinterpret_cast<char*>(bssrdf_data), get_material_slice_size() * sizeof(float));
	ifs.close();
}

void BSSRDFLoader::load_hemisphere(float * bssrdf_data, const std::vector<size_t>& idx)
{
	if (idx.size() != 6)
		Logger::error << "Hemisphere index is 6 dimensional." << std::endl;
	size_t pos = flatten_index({ idx[0], idx[1], idx[2], idx[3], idx[4], idx[5], 0, 0 }, mDimensions) * sizeof(float);
	std::ifstream ifs;
	ifs.open(mFileName, std::ofstream::in | std::ofstream::binary);
	ifs.seekg(pos + mBSSRDFStart);
	ifs.read(reinterpret_cast<char*>(bssrdf_data), get_hemisphere_size() * sizeof(float));
	ifs.close();
}
	
bool BSSRDFLoader::parse_header()
{

	std::ifstream file(mFileName, std::ofstream::in | std::ofstream::binary);

#define MAX_HEADER_SIZE 2048
	char header[MAX_HEADER_SIZE];
	file.read(header, MAX_HEADER_SIZE * sizeof(char));
	std::string str(header);

	size_t bssrdf_del = str.find(std::string("\n") + bssrdf_delimiter);

	bool parsed_dimensions = false;
	bool has_bssrdf_flag = false;

	mBSSRDFStart = str.find("\n", bssrdf_del + 1) + 1;

	std::stringstream ss(str);

	while (std::getline(ss, str)) {
		if (str.size() >= bssrdf_delimiter.size() && str.substr(0, bssrdf_delimiter.size()).compare(bssrdf_delimiter) == 0)
		{
			has_bssrdf_flag = true;
			break;
		}
		if (str.size() >= size_delimiter.size() && str.substr(0, size_delimiter.size()).compare(size_delimiter) == 0)
		{
			std::stringstream ss (str.substr(size_delimiter.size()));
			for (int i = 0; i < 8; i++)
			{
				size_t size;
				ss >> size; 
				if (ss.fail())
					return false;
				mDimensions.push_back(size);
			}
			parsed_dimensions = true;
		}
		if (str.size() > 0 && str[0] == '#')
			continue;
	}
	return has_bssrdf_flag && parsed_dimensions;
}

size_t flatten_index(const std::vector<size_t>& idx, const std::vector<size_t>& size)
{
	size_t id = idx[eta_index];
	for (int i = 1; i < 8; i++)
	{
		id = id * size[i] + idx[i];
	}
	return id;
}

BSSRDFExporter::BSSRDFExporter(const std::string & filename, const std::vector<size_t>& dimensions, const std::map<size_t, std::vector<float>> & parameters) : mFileName(filename), mDimensions(dimensions)
{
	size_t total_size = sizeof(float);
	for (size_t element : dimensions)
		total_size *= element;

	mBSSRDFStart = write_header(std::ofstream::out, parameters);

	std::ofstream of;
	of.open(mFileName, std::ofstream::in | std::ofstream::out | std::ofstream::binary);
	of.seekp(mBSSRDFStart + total_size - 1);
	of.put('\0');
	of.close();
	
}

size_t BSSRDFExporter::get_material_slice_size()
{
	return get_hemisphere_size() * mDimensions[albedo_index] * mDimensions[g_index] * mDimensions[eta_index];
}

size_t BSSRDFExporter::get_hemisphere_size()
{
	return mDimensions[phi_o_index] * mDimensions[theta_o_index];
}

void BSSRDFExporter::set_material_slice(const float * bssrdf_data, const std::vector<size_t>& idx)
{
	if (idx.size() != 3)
		Logger::error << "Material index is 3 dimensional." << std::endl;
	size_t pos = flatten_index({ idx[0], idx[1], idx[2], 0, 0, 0, 0, 0 }, mDimensions) * sizeof(float);
	std::ofstream ofs;
	ofs.open(mFileName, std::ofstream::in | std::ofstream::out | std::ofstream::binary);
	ofs.seekp(pos + mBSSRDFStart);
	ofs.write(reinterpret_cast<const char*>(bssrdf_data), get_material_slice_size() * sizeof(float));
	ofs.close();
}

void BSSRDFExporter::set_hemisphere(const float * bssrdf_data, const std::vector<size_t>& idx)
{
	if (idx.size() != 6)
		Logger::error << "Hemisphere index is 6 dimensional." << std::endl;
	size_t pos = flatten_index({ idx[0], idx[1], idx[2], idx[3], idx[4], idx[5], 0, 0 }, mDimensions) * sizeof(float);
	std::ofstream ofs;
	ofs.open(mFileName, std::ofstream::in | std::ofstream::out | std::ofstream::binary);
	ofs.seekp(pos + mBSSRDFStart);
	ofs.write(reinterpret_cast<const char*>(bssrdf_data), get_hemisphere_size() * sizeof(float));
	ofs.close();
}

size_t BSSRDFExporter::write_header(int mode, const std::map<size_t, std::vector<float>>& parameters)
{
	std::ofstream of;
	of.open(mFileName, mode);
	of << "# BSSRDF file format (version 0.1)" << std::endl;
	of << "# Index dimensions is at follows:" << std::endl;
	of << size_delimiter << " ";
	for (int i = 0; i < 8; i++)
	{
		of << mDimensions[i] << " ";
	}
	of << std::endl;
	of << "#eta\tg\talbedo\ttheta_s\tr\ttheta_i\tphi_o\ttheta_o" << std::endl;
	
	for (const auto & pair : parameters)
	{
		of << "PARAMETER " << pair.first << " " << stringize(pair.second.data(), pair.second.size(), 2)<< std::endl;
	}	
	
	of << bssrdf_delimiter << std::endl;
	size_t t = of.tellp();
	of.close();
	return t;
}
