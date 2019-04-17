#pragma once
#include <vector>
#include <glm/glm.hpp>

class OBJLoader {
public:
	OBJLoader();
	~OBJLoader();

	//welcome to std::hell
	std::tuple<std::vector<glm::vec3>, std::vector<glm::vec2>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>> loadObj(std::string filename);
};

