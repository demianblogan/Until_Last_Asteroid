#include "AppDataPath.h"

#include <cstdlib>

namespace AppDataPath
{
	std::filesystem::path Resolve(std::string_view fileName)
	{
		char* localAppData = nullptr;

		// Second output parameter of _dupenv_s: the length (in characters,
		// including the null terminator) of the environment variable's
		// value that got copied into localAppData. Unused here beyond
		// satisfying the function's signature -- only the value itself and
		// whether the lookup succeeded matter.
		std::size_t environmentVariableLength = 0;

		if (_dupenv_s(&localAppData, &environmentVariableLength, "LOCALAPPDATA") == 0 &&
			localAppData != nullptr)
		{
			const std::filesystem::path directory =
				std::filesystem::path(localAppData) /
				"Alone Bull Company" /
				"Until Last Asteroid";

			std::free(localAppData);

			return directory / fileName;
		}

		std::free(localAppData);

		return std::filesystem::current_path() / "user_data" / fileName;
	}
}