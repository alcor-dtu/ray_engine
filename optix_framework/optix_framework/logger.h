#pragma once
#ifndef LOGGER_H
#define LOGGER_H
#include <iostream>
#include <string>


class Logger {

public:
	explicit Logger(std::ostream& _out, const char * _color_symbol, const char * _color_string) : out(&_out), color_symbol(_color_symbol), color_string(_color_string) {}

	Logger& operator<<(const std::string& a)
	{
		if (start_of_line)
		{
			std::string color = is_color_enabled ? color_symbol : "";
			std::string color_end = is_color_enabled ? RESET : "";
			*out << color << "[" << color_string << "] " << color_end;
			start_of_line = false;
		}
		*out << a;
		if (a.back() == '\n')
			start_of_line = true;
		return *this;
	}

	Logger& operator<<(const char* a)
	{
		if (start_of_line)
		{
			std::string color = is_color_enabled ? color_symbol : "";
			std::string color_end = is_color_enabled ? RESET : "";
			*out << color << "[" << color_string << "] " << color_end;
			start_of_line = false;
		}
		*out << a;
		return *this;
	}

	template<typename T>
	Logger& operator<<(const T v)  
	{ 	
		if (start_of_line)
		{
			std::string color = is_color_enabled ? color_symbol : "";
			std::string color_end = is_color_enabled ? RESET : "";
			*out << color << "[" << color_string << "] " << color_end;
			start_of_line = false;
		}
		std::string a = std::to_string(v);
		*out << a;
		if (a.back() == '\n')
			start_of_line = true;
		return *this; 
	}

//	Logger& operator<<<const char*>(const char* v)
//	{
	//	return this->operator<<<std::string>(std::string(v));
	//}


	Logger& operator<<(std::ostream& (*F)(std::ostream&)) { F(*out); start_of_line = true;  return *this; }


	static Logger info;
	static Logger debug;
	static Logger error;
	static Logger warning;
	static bool is_color_enabled;

	static void set_logger_output(std::ostream& out);

protected:
	std::ostream * out;
	std::string color_symbol;
	std::string color_string;


	bool start_of_line = true;

private:
	static const char * BLUE; 
	static const char * RED;
	static const char * GREEN;
	static const char * RESET; 
	static const char * YELLOW;
	void set_output(std::ostream & out);
};



#endif
