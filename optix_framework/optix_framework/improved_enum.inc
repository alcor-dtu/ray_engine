// In DefineImprovedEnum.h

////////////////////////////////////////////////////////////////////////
// IMPORTANT NOTE:
// This is a "batch file of preprocessing directives"
// (because this cannot be done with a macro).
// Each time you include this file you are calling a batch file,
// it doesn't work as a macro include.
// If you want to declare several different enum types,
// you have to include this file several times.
// Do not use "#pragma once" directive, because it would have
// unexpected behaviour and results.
// Do not use directives like:
// #ifndef _IMPROVED_ENUM_H_ ; #define _IMPROVED_ENUM_H_ (same reason).
////////////////////////////////////////////////////////////////////////
// AUTHOR:      Hugo Gonzalez Castro
// TITLE:       Improving C++ Enum: Adding Serialization,
//                                  Inheritance and Iteration.
// DESCRIPTION: A different approach to C++ enums: enum to string,
//              enum extension and enum iteration.
// VERSION:     v5.0 - 2009/04/13
// LICENSE:     CPOL (Code Project Open License).
//              Please, do not remove nor modify this header.
// URL:         ImprovedEnum.aspx
////////////////////////////////////////////////////////////////////////
// INPUT PARAMETERS:
// This file needs the following input parameters to be defined
// before including it:
// Input parameter: the name of the enumeration
// #define IMPROVED_ENUM_NAME [NameOfYourEnum]
// Input parameter: the file with the enum items
// #define IMPROVED_ENUM_FILE ["EnumItemFile"]
////////////////////////////////////////////////////////////////////////
// ENUMITEM FILE:
// The EnumItemFile is a list (one per line) of:
// ENUMITEM(EnumItem) or ENUMITEM_VALUE(EnumItem, Value)
////////////////////////////////////////////////////////////////////////
// ALTERNATIVE TO ENUMITEM FILE:
// IMPROVED_ENUM_LIST instead of IMPROVED_ENUM_FILE
// #define IMPROVED_ENUM_LIST  ENUMITEM(Item1) ... ENUMITEM(LastItem)
// #define IMPROVED_ENUM_LIST  ENUMITEM(Item1) \
//                             ENUMITEM(Item2) \
//                             ...
//                             ENUMITEM(LastItem)
////////////////////////////////////////////////////////////////////////
// OPTIONAL INPUT PARAMETERS:
// If you want to define a subclass instead of a namespace, you can
// #define IMPROVED_ENUM_SUBCLASS, or
// #define IMPROVED_ENUM_SUBCLASS_PARENT [ParentClass]
// to make subclass inherit from a ParentClass.
// If you want to extend an already defined ImprovedEnum, you have to
// define which type do you want to extend with
// IMPROVED_ENUM_INHERITED_NAME and IMPROVED_ENUM_INHERITED_FILE
// input parameters.
////////////////////////////////////////////////////////////////////////

// Checking ENUMITEM and ENUMITEM_VALUE macros are not already defined
#if defined(ENUMITEM)
#error ENUMITEM macro cannot be already defined
#elif defined(ENUMITEM_VALUE)
#error ENUMITEM_VALUE macro cannot be already defined
#endif

// Standard string class
#include <string>


//! We define the IMPROVED_ENUM_NAME namespace, which contains
//! the enum type and the conversion functions from the
//! enum type to the string type and vice versa.
///////////////////////////////////////////////////////////
#define STATIC_METHOD static
namespace IMPROVED_ENUM_NAME
{
	//! Some stuff to get the string of the IMPROVED_ENUM_NAME
	///////////////////////////////////////////////////////////
#define GET_MACRO_STRING_EXPANDED(Macro)  #Macro
#define GET_MACRO_STRING(Macro)  GET_MACRO_STRING_EXPANDED(Macro)
#define ENUM_SEPARATOR  "::"
#define ENUM_TYPE_NAME  GET_MACRO_STRING(IMPROVED_ENUM_NAME)
#define FULL_ENUM_TYPE_NAME    ENUM_TYPE_NAME

	//! This defines the enumerated type:
	//////////////////////////////////////////
	typedef enum EnumTypeTag
	{
		//////////////////////////////////////////
		// With this mini-macro we make ENUMITEM file/s
		// a list of items separated by commas:
#define  ENUMITEM(EnumItem) EnumItem,
#define  ENUMITEM_VALUE(EnumItem, Value) EnumItem = Value,
#ifdef   IMPROVED_ENUM_INHERITED_FILE
#include IMPROVED_ENUM_INHERITED_FILE
#endif// IMPROVED_ENUM_INHERITED_FILE
#ifdef   IMPROVED_ENUM_FILE
#include IMPROVED_ENUM_FILE
#else // IMPROVED_ENUM_LIST
		IMPROVED_ENUM_LIST
#endif// IMPROVED_ENUM_FILE
#undef   ENUMITEM_VALUE
#undef   ENUMITEM
		//////////////////////////////////////////
		NotValidEnumItem // We add this item to all enums
	} EnumType, Type;

#ifndef __CUDACC__
	STATIC_METHOD inline const std::string name() { return ENUM_TYPE_NAME; }

	//! Conversion from enum to string:
	//////////////////////////////////////////
	STATIC_METHOD inline const std::string to_string(const EnumType& t)
	{
		switch (t)
		{
			//////////////////////////////////////////
			// With this mini-macro we make ENUMITEM file/s
			// a CASE list which returns the stringized value:
#define  ENUMITEM(EnumItem) case EnumItem : return #EnumItem;
#define  ENUMITEM_VALUE(EnumItem, Value) ENUMITEM(EnumItem)
#ifdef   IMPROVED_ENUM_INHERITED_FILE
#include IMPROVED_ENUM_INHERITED_FILE
#endif// IMPROVED_ENUM_INHERITED_FILE
#ifdef   IMPROVED_ENUM_FILE
#include IMPROVED_ENUM_FILE
#else // IMPROVED_ENUM_LIST
			IMPROVED_ENUM_LIST
#endif// IMPROVED_ENUM_FILE
#undef   ENUMITEM_VALUE
#undef   ENUMITEM
				//////////////////////////////////////////
		}
		return ""; // NotValidEnumItem
	}

	//! Conversion from string to enum:
	//////////////////////////////////////////
	STATIC_METHOD inline const EnumType to_enum(const std::string& s)
	{
		if (s == "") return NotValidEnumItem;
		//////////////////////////////////////////
		// With this mini-macro we make ENUMITEM file/s
		// an IF list which returns the enum item:
#define  ENUMITEM(EnumItem) if (s == #EnumItem) return EnumItem;
#define  ENUMITEM_VALUE(EnumItem, Value) ENUMITEM(EnumItem)
#ifdef   IMPROVED_ENUM_INHERITED_FILE
#include IMPROVED_ENUM_INHERITED_FILE
#endif// IMPROVED_ENUM_INHERITED_FILE
#ifdef   IMPROVED_ENUM_FILE
#include IMPROVED_ENUM_FILE
#else // IMPROVED_ENUM_LIST
		IMPROVED_ENUM_LIST
#endif// IMPROVED_ENUM_FILE
#undef   ENUMITEM_VALUE
#undef   ENUMITEM
			//////////////////////////////////////////
			return NotValidEnumItem;
	}

	STATIC_METHOD inline const int to_int(const std::string& s)
	{
		return static_cast<int>(to_enum(s));
	}

	//! Enum iteration to next:
	//////////////////////////////////////////
	STATIC_METHOD inline const EnumType next(const EnumType& t)
	{
		switch (t)
		{
		case NotValidEnumItem : 
			//////////////////////////////////////////
			// With this mini-macro we make ENUMITEM file/s
			// a CASE list which returns the next item:
#define  ENUMITEM(EnumItem) return EnumItem; case EnumItem : 
#define  ENUMITEM_VALUE(EnumItem, Value) ENUMITEM(EnumItem)
#ifdef   IMPROVED_ENUM_INHERITED_FILE
#include IMPROVED_ENUM_INHERITED_FILE
#endif// IMPROVED_ENUM_INHERITED_FILE
#ifdef   IMPROVED_ENUM_FILE
#include IMPROVED_ENUM_FILE
#else // IMPROVED_ENUM_LIST
			IMPROVED_ENUM_LIST
#endif// IMPROVED_ENUM_FILE
#undef   ENUMITEM_VALUE
#undef   ENUMITEM
				//////////////////////////////////////////
				return NotValidEnumItem; // (This indentation is intentional)
		}
		return NotValidEnumItem; // (This line is intentional too, do not remove)
	}

	//! Enum iteration to previous:
	//////////////////////////////////////////
	STATIC_METHOD inline const EnumType previous(const EnumType& t)
	{
		EnumType tprev = NotValidEnumItem;
		//////////////////////////////////////////
		// With this mini-macro we make ENUMITEM file/s
		// an IF list which returns the previous item:
#define  ENUMITEM(EnumItem) \
	if (t == EnumItem) return tprev; else tprev = EnumItem;
#define  ENUMITEM_VALUE(EnumItem, Value) ENUMITEM(EnumItem)
#ifdef   IMPROVED_ENUM_INHERITED_FILE
#include IMPROVED_ENUM_INHERITED_FILE
#endif// IMPROVED_ENUM_INHERITED_FILE
#ifdef   IMPROVED_ENUM_FILE
#include IMPROVED_ENUM_FILE
#else // IMPROVED_ENUM_LIST
		IMPROVED_ENUM_LIST
#endif// IMPROVED_ENUM_FILE
#undef   ENUMITEM_VALUE
#undef   ENUMITEM
			//////////////////////////////////////////
			return tprev;
	}

	//! The first and the last Enums:
	//////////////////////////////////////////
	STATIC_METHOD inline const EnumType first()
	{ return next(NotValidEnumItem); }
	STATIC_METHOD inline const EnumType last()
	{ return previous(NotValidEnumItem); }


	//! Number of enum items:
	//////////////////////////////////////////
	constexpr STATIC_METHOD inline const int count()
	{
		return 0
			//////////////////////////////////////////
			// With this mini-macro we make ENUMITEM file/s
			// a counter list:
#define  ENUMITEM(EnumItem) +1
#define  ENUMITEM_VALUE(EnumItem, Value) ENUMITEM(EnumItem)
#ifdef   IMPROVED_ENUM_INHERITED_FILE
#include IMPROVED_ENUM_INHERITED_FILE
#endif// IMPROVED_ENUM_INHERITED_FILE
#ifdef   IMPROVED_ENUM_FILE
#include IMPROVED_ENUM_FILE
#else // IMPROVED_ENUM_LIST
			IMPROVED_ENUM_LIST
#endif// IMPROVED_ENUM_FILE
#undef   ENUMITEM_VALUE
#undef   ENUMITEM
			//////////////////////////////////////////
			;
	}

	STATIC_METHOD inline const std::string get_full_string(const char * separator = " ")
	{
		EnumType firs = first();
		std::string res = std::string("");
		for(int i = 0; i < count(); i++)
		{
			if(i != 0)
				res.append(separator);
			res += to_string(firs);
			firs = next(firs);
		} while(firs != NotValidEnumItem);
		return res;
	}

	STATIC_METHOD inline const std::string get_option_string()
	{
		std::string data = "";
		auto f = first();
		while(f != NotValidEnumItem)
		{
			data += to_string(f) + '\0';
			f = next(f);
		}
		return data;
	}
#endif
	// Free temporary macros:
	///////////////////////////
#undef STATIC_METHOD
#undef ENUM_SEPARATOR
#undef ENUM_TYPE_NAME
#undef PARENT_ENUM_TYPE_NAME
#undef FULL_ENUM_TYPE_NAME
#undef GET_MACRO_STRING
#undef GET_MACRO_STRING_EXPANDED
}

// Free this file's parameters:
////////////////////////////////
#undef IMPROVED_ENUM_NAME
#undef IMPROVED_ENUM_FILE
#undef IMPROVED_ENUM_LIST
// Do not use directives like: #endif (reason above)