#pragma once
#include <string>
#include <wtypes.h>
#include <vector>
#include <memory>
#include <stdexcept>
#include <regex>

// header only string functions
namespace StdUtility
{
	template<typename ... Args>
	std::string string_format(const std::string& format, Args ... args)
	{
		int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
		auto size = static_cast<size_t>(size_s);
		auto buf = std::make_unique<char[]>(size);
		std::snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
	}
	template<typename ... Args>
	std::wstring string_format(const std::wstring& format, Args ... args)
	{
		int size_s = std::swprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
		auto size = static_cast<size_t>(size_s);
		auto buf = std::make_unique<wchar_t[]>(size);
		std::swprintf(buf.get(), size, format.c_str(), args ...);
		return std::wstring(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
	}

	template<typename T>
	std::vector< std::vector<T> > split(std::vector<T> vec, uint64_t n)
	{
		if (vec.size() == 0) {
			// return an empty list
			return std::vector< std::vector<T> >();
		}
		if (vec.size() < n) {
			// cannot split up a list smaller than n
			// return the whole list back
			std::vector< std::vector<T> > vec_of_vecs(1);
			vec_of_vecs[0] = std::vector<T>(vec.begin(), vec.end());
			return vec_of_vecs;
		}

		std::vector< std::vector<T> > vec_of_vecs(n);

		uint64_t quotient = vec.size() / n;
		uint64_t reminder = vec.size() % n;
		uint64_t first = 0;
		uint64_t last;
		for (uint64_t i = 0; i < n; ++i) {
			if (i < reminder) {
				last = first + quotient + 1;
				vec_of_vecs[i] = std::vector<T>(vec.begin() + first, vec.begin() + last);
				first = last;
			}
			else if (i != n - 1) {
				last = first + quotient;
				vec_of_vecs[i] = std::vector<T>(vec.begin() + first, vec.begin() + last);
				first = last;
			}
			else
				vec_of_vecs[i] = std::vector<T>(vec.begin() + first, vec.end());
		}

		return vec_of_vecs;
	}

	// trim from start (in place)
	inline void ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
	}

	// trim from end (in place)
	inline void rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
	}
	// trim from both ends (in place)
	inline void trim(std::string& s) {
		rtrim(s);
		ltrim(s);
	}

	// trim from start (copying)
	inline std::string ltrim_copy(std::string s) {
		ltrim(s);
		return s;
	}

	// trim from end (copying)
	inline std::string rtrim_copy(std::string s) {
		rtrim(s);
		return s;
	}

	// trim from both ends (copying)
	inline std::string trim_copy(std::string s) {
		trim(s);
		return s;
	}

	inline std::string first_numberstring(std::string const& str)
	{
		char const* digits = "0123456789";
		std::size_t const n = str.find_first_of(digits);
		if (n != std::string::npos)
		{
			std::size_t const m = str.find_first_not_of(digits, n);
			return str.substr(n, m != std::string::npos ? m - n : m);
		}
		return std::string();
	}

	inline void appendSlash(std::string& str, bool backSlash = true)
	{
		if (!str.empty()) {
			// add // or \ if not on end
			if (!(str[str.size() - 1] == '\\' || str[str.size() - 1] == '/')) {
				if (backSlash) {
					str += "\\";
				}
				else {
					str += "/";
				}
			}
		}
	}
	inline void appendSlash(std::wstring& str, bool backSlash = true)
	{
		if (!str.empty()) {
			// add // or \ if not on end
			if (!(str[str.size() - 1] == L'\\' || str[str.size() - 1] == L'/')) {
				if (backSlash) {
					str += L"\\";
				}
				else {
					str += L"/";
				}
			}
		}
	}
};
