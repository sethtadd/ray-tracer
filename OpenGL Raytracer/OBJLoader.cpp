#define _CRT_SECURE_NO_WARNINGS

#include "OBJLoader.h"
#include <fstream>
#include <string>
#include <cstring>
#include <tuple>
#include <sstream>
#include <iostream>
#include <stdio.h>


OBJLoader::OBJLoader() {}


OBJLoader::~OBJLoader() {}

std::tuple<std::vector<glm::vec3>, std::vector<glm::vec2>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>> OBJLoader::loadObj(std::string filename) {
	//vertex data
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	//index data
	std::vector<glm::vec3> vertexIndices;
	std::vector<glm::vec3> uvIndices;
	std::vector<glm::vec3> normalIndices;

	FILE * file;
	int err;

	if( ( file = fopen(filename.c_str(), "r") ) == nullptr ) {
		std::cout << "Error, NULL file." << std::endl;
	} else {
		int ln = 0;
		while(1) {
			char linehead[128];
			int r = fscanf(file, "%s", linehead, 128);
			if (r == EOF) break;

			if(strcmp(linehead, "v") == 0) { //vertex data
				glm::vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z, 128);
				vertices.push_back(vertex);
			} else if (strcmp(linehead, "vt") == 0) { //UV data
				glm::vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y, 128);
				uvs.push_back(uv);
			} else if(strcmp(linehead, "vn") == 0) { //normal data
				glm::vec3 normal;
				fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z, 128);
				normals.push_back(normal);
			} else if(strcmp(linehead, "f") == 0) { //index data
													//get all index values (could be more than 3 vertices per face)
				char dat[1024];
				fgets(dat, 1024, file);
				std::string data(dat);
				std::vector<std::string> tokens;
				std::istringstream ss(data.substr(1));
				std::string token;
				std::string delim = " ";
				for(std::string d; ss >> d;) {
					tokens.push_back(d);
				}

				//hello memory leaks
				int *vind = (int *)malloc(sizeof * vind * tokens.size());
				int *uvind = (int *)malloc(sizeof * uvind * tokens.size());
				int *nind = (int *)malloc(sizeof * nind * tokens.size());

				std::vector<glm::vec3> faceVertices(tokens.size() - 2);
				std::vector<glm::vec3> faceUVs(tokens.size() - 2);
				std::vector<glm::vec3> faceNormals(tokens.size() - 2);


				for(int i = 0; i < tokens.size(); i++) {
					std::string token = tokens[i];
					sscanf(token.c_str(), "%d/%d/%d", &vind[i], &uvind[i], &nind[i], 32);
				}

				//triangulate polygons to work with ray-tracing algorithm
				int n = 0;
				faceVertices[n].x = vind[0] - 1;
				faceVertices[n].y = vind[1] - 1;
				faceVertices[n].z = vind[2] - 1;

				faceUVs[n].x = uvind[0] - 1;
				faceUVs[n].y = uvind[1] - 1;
				faceUVs[n].z = uvind[2] - 1;

				faceNormals[n].x = nind[0] - 1;
				faceNormals[n].y = nind[1] - 1;
				faceNormals[n].z = nind[2] - 1;

				n++;
				for(int i = 3; i < tokens.size(); i++) {
					//also 0-index the indices here
					faceVertices[n].x = vind[i - 3] - 1;
					faceVertices[n].y = vind[i - 1] - 1;
					faceVertices[n].z = vind[i] - 1;

					faceUVs[n].x = uvind[i - 3] - 1;
					faceUVs[n].y = uvind[i - 1] - 1;
					faceUVs[n].z = uvind[i] - 1;

					faceNormals[n].x = nind[i - 3] - 1;
					faceNormals[n].y = nind[i - 1] - 1;
					faceNormals[n].z = nind[i] - 1;
					n++;
				}

				//push back index data
				for(int i = 0; i < tokens.size() - 2; i++) {
					vertexIndices.push_back(faceVertices[i]);
					uvIndices.push_back(faceUVs[i]);
					normalIndices.push_back(faceNormals[i]);
				}


				//bye bye memory leaks
				free(vind);
				free(uvind);
				free(nind);
			}
			if(ln % 5000 == 0) {
				std::cout << ln << " vertices parsed\n";
			}
			ln++;
		}
	}

	std::cout << "Num tris: " << vertexIndices.size() << std::endl;
	return std::make_tuple(vertices, uvs, normals, vertexIndices, uvIndices, normalIndices);
}

