#ifndef UTILS_H
#define	UTILS_H

#include <cstdio>
#include <cctype>
#include <random>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <algorithm>

class Utils {

	public:
		static bool isValidHexString(const std::string& str = "");
		static bool isValidBinString(const std::string& str = "");
		static const std::string removeSpaces(const std::string& str = "");
		static std::string randomString(size_t len);
		static std::string trim(const std::string& s);

};

#endif /* UTILS_H */
