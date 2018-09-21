#include <pbnj.h>
#include <Camera.h>
#include <Configuration.h>
#include <Particles.h>
#include <Renderer.h>

#include <cmath>
#include <iostream>

std::string imageFamily(std::string filename, unsigned int count,
        float Ks=-1.f)
{
    std::string::size_type index;
    index = filename.rfind('.');
    std::string base = filename.substr(0, index);
    std::string family = std::to_string(count);
    if(Ks != -1.f)
    {
        char spec[8];
        std::sprintf(spec, "%.1f", Ks);
        family += '.';
        family += spec;
        family.insert(0, 8 - family.length(), '0');
    }
    else
    {
        family.insert(0, 4 - family.length(), '0');
    }
    std::string ext = filename.substr(index);
    return base + '.' + family + ext;
}

void pathSpiralOut(pbnj::Renderer *renderer, pbnj::Camera *camera,
        std::vector<float> start, float maxDistScale,
        unsigned int numSteps, float degreesPerStep,
        std::string imageFilename)
{
    float startDist = std::sqrt(start[0]*start[0] +
                                start[1]*start[1] +
                                start[2]*start[2]);
    float maxDist = startDist * maxDistScale;
    float curDeg = 0;
    float curDist = startDist;

    for(unsigned int step = 0; step < numSteps; step++) {
        std::cout << step + 1 << "/" << numSteps << std::flush;
        camera->setPosition(start[0], start[1], start[2]);
        camera->setUpVector(0, 1, 0);
        camera->centerView();

        std::string filename = imageFamily(imageFilename, step);
        renderer->renderImage(filename);

        curDeg += degreesPerStep;
        float alongCurve = (std::pow(10.f, ((float)step / (numSteps - 1))) - 1) / 9;
        curDist = startDist + alongCurve * (maxDist - startDist);

        start[0] = std::sin(degreesPerStep*(step+1) * 3.14159 / 180.0) * curDist;
        start[2] = std::cos(degreesPerStep*(step+1) * 3.14159 / 180.0) * curDist;
        std::cout << "\r" << std::flush;
    }
    std::cout << std::endl;
}

int main(int argc, const char **argv)
{
    pbnj::pbnjInit(&argc, argv);

    pbnj::ConfigReader *reader = new pbnj::ConfigReader();
    rapidjson::Document json;
    reader->parseConfigFile(argv[1], json);
    pbnj::Configuration *config = new pbnj::Configuration(json);

    pbnj::Particles *particles = new pbnj::Particles(config->dataFilename);
    particles->setSpecular(0.3);

    pbnj::Camera *camera = new pbnj::Camera(config->imageWidth, config->imageHeight);
    pbnj::Renderer *renderer = new pbnj::Renderer();
    renderer->setSamples(config->samples);
    renderer->setBackgroundColor(config->bgColor);
    renderer->setCamera(camera);
    renderer->addSubject(particles);

    pathSpiralOut(renderer, camera,
        std::vector<float>({config->cameraX, config->cameraY, config->cameraZ}),
        1.5, 5, 0.5, config->imageFilename);

    return 0;
}
