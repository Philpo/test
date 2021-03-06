// [header]
// A very basic raytracer example.
// [/header]
// [compile]
// c++ -o raytracer -O3 -Wall rayTracer->cpp
// [/compile]
// [ignore]
// Copyright (C) 2012  www.scratchapixel.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// [/ignore]

#include <thread>
#include "RapidXML\rapidxml.hpp"
#include "RapidXML\rapidxml_utils.hpp"
#include "rayTracer.h"
#include "StringUtils.h"
#include "Pass.h"

using namespace rapidxml;

int main(int argc, char **argv) {
  srand(13);

  std::string spheresFilePath, movesFilePath, threadMethod, ioMethod, outputFile;
	int numThreads = 1;

  if (argc == 9 || argc == 10) {
    numThreads = 0;
    for (int i = 1; i < 5; i += 2) {
      if (strcmp(argv[i], "-i") == 0) {
        spheresFilePath = argv[i + 1];
      }
      else if (strcmp(argv[i], "-m") == 0) {
        movesFilePath = argv[i + 1];
      }
    }

		if (argc == 9) {
			if (strcmp(argv[5], "-tm1") == 0) {
				threadMethod = "tm1";
			}
      else {
        return -1;
      }
      if (strcmp(argv[6], "-tio") == 0) {
        ioMethod = "tio";
      }
      else if (strcmp(argv[6], "-io") == 0) {
        ioMethod = "io";
      }
      else {
        return -1;
      }
      if (strcmp(argv[7], "-f") == 0) {
        outputFile == argv[8];
      }
      else {
        return -1;
      }
		}
		else if (argc == 10) {
			if (strcmp(argv[5], "-tm3") == 0) {
				threadMethod = "tm3";
				numThreads = convertStringToNumber<int>(argv[6]);
			}
			else if (strcmp(argv[5], "-tm2") == 0) {
				threadMethod = "tm2";
				numThreads = convertStringToNumber<int>(argv[6]);
			}
      else {
        return -1;
      }
      if (strcmp(argv[7], "-tio") == 0) {
        ioMethod = "tio";
      }
      else if (strcmp(argv[7], "-io") == 0) {
        ioMethod = "io";
      }
      else {
        return -1;
      }
      if (strcmp(argv[8], "-f") == 0) {
        outputFile == argv[9];
      }
      else {
        return -1;
      }
		}

    std::string directory;
    auto now = std::time(nullptr);
    std::ostringstream os;
    os << std::put_time(std::gmtime(&now), "%Y-%m-%d_%H%M%S");
    directory = "spheres" + os.str();

    std::string mkdirCommand = "mkdir .\\" + directory;
    system(mkdirCommand.c_str());

    std::vector<Pass> passes;
    int passCount = 0;

    try {
      file<> sceneFile(spheresFilePath.c_str());
      xml_document<> doc;
      doc.parse<0>(sceneFile.data());
      xml_node<>* rootNode = doc.first_node();

      float x, y, z, radius, r, g, b, reflection, transparency, emissionR, emissionG, emissionB;
      reflection = transparency = emissionR = emissionG = emissionB = 0.0f;

      for (xml_node<>* sphereNode = rootNode->first_node(); sphereNode; sphereNode = sphereNode->next_sibling()) {
        x = convertStringToNumber<float>(sphereNode->first_attribute("x")->value());
        y = convertStringToNumber<float>(sphereNode->first_attribute("y")->value());
        z = convertStringToNumber<float>(sphereNode->first_attribute("z")->value());
        radius = convertStringToNumber<float>(sphereNode->first_attribute("radius")->value());
        r = convertStringToNumber<float>(sphereNode->first_attribute("r")->value());
        g = convertStringToNumber<float>(sphereNode->first_attribute("g")->value());
        b = convertStringToNumber<float>(sphereNode->first_attribute("b")->value());

        if (sphereNode->first_attribute("reflection")) {
          reflection = convertStringToNumber<float>(sphereNode->first_attribute("reflection")->value());
        }
        if (sphereNode->first_attribute("transparency")) {
          transparency = convertStringToNumber<float>(sphereNode->first_attribute("transparency")->value());
        }
        if (sphereNode->first_attribute("emissionR")) {
          emissionR = convertStringToNumber<float>(sphereNode->first_attribute("emissionR")->value());
        }
        if (sphereNode->first_attribute("emissionG")) {
          emissionG = convertStringToNumber<float>(sphereNode->first_attribute("emissionG")->value());
        }
        if (sphereNode->first_attribute("emissionB")) {
          emissionB = convertStringToNumber<float>(sphereNode->first_attribute("emissionB")->value());
        }

        spheres.push_back(Sphere(Vec3f(x, y, z), radius, Vec3f(r, g, b), reflection, transparency, Vec3f(emissionR, emissionG, emissionB)));
      }
    }
    catch (parse_error& e) {
      ofstream errorFile;
      errorFile.open("error_file.txt");
      errorFile << "Error reading scene file " << ": " << e.what() << endl;
      errorFile.close();
      return 0;
    }

    speedResults.open(outputFile);

    try {
      file<> passFile(movesFilePath.c_str());
      xml_document<> doc;
      doc.parse<0>(passFile.data());
      xml_node<>* rootNode = doc.first_node();

      for (xml_node<>* passNode = rootNode->first_node(); passNode; passNode = passNode->next_sibling()) {
        passes.push_back(Pass(passNode, directory, passCount++, threadMethod, numThreads, ioMethod));
      }
    }
    catch (parse_error& e) {
      ofstream errorFile;
      errorFile.open("error_file.txt");
      errorFile << "Error reading pass file " << ": " << e.what() << endl;
      errorFile.close();
      return 0;
    }

    xml_document<> doc;    // character type defaults to char
    string text = "<?xml version=\"1.0\" encoding=\"utf - 8\"?><a>a</a>";
    doc.parse<0>((char*) &text[0]);

    for (Pass pass : passes) {
      pass.render();
    }

    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    std::string ffmpegCommand = "ffmpeg -i .\\" + directory + "\\spheres%03d.ppm -y .\\" + directory + "\\out.mp4";
    system(ffmpegCommand.c_str());
    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = endTime - start;
    total_elapsed_time += elapsed_time;

    std::cout << "**********************" << std::endl;
    std::cout << "Finished video render in " << elapsed_time.count() << std::endl;
    std::cout << "**********************" << std::endl;
    std::cout << "**********************" << std::endl;
    std::cout << "Total Render Time: " << total_elapsed_time.count() << std::endl;
    std::cout << "**********************" << std::endl;

    speedResults << "**********************" << std::endl;
    speedResults << "Finished video render in " << elapsed_time.count() << std::endl;
    speedResults << "**********************" << std::endl;
    speedResults << "**********************" << std::endl;
    speedResults << "Total Render Time: " << total_elapsed_time.count() << std::endl;
    speedResults << "**********************" << std::endl;

    speedResults.close();

#ifndef _DEBUG
		string command = "del " + directory + "\\*.ppm";
		system(command.c_str());
#endif
  }

  system("PAUSE");

  return 0;
}