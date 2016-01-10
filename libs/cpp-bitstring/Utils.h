#ifndef UTILS_H
#define	UTILS_H

#include <cstdio>
#include <cctype>
#include <string>
#include <cstdlib>
#include <sstream>

using namespace std;

class Utils {

	public:
		static bool isValidHexString(const string& str = "");
		static bool isValidBinString(const string& str = "");
		static const string removeSpaces(const string& str = "");

};

#endif /* UTILS_H */
