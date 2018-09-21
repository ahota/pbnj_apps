#include <pbnj.h>
#include <Camera.h>
#include <Configuration.h>
#include <Particles.h>
#include <Renderer.h>
#include <Streamlines.h>
#include <TimeSeries.h>
#include <TransferFunction.h>
#include <Volume.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <dirent.h>

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

int main(int argc, const char **argv)
{
    if(argc != 2 || std::strcmp(argv[1], "-h") == 0 ||std::strcmp(argv[1], "--help") == 0) {
        {
            std::cerr << "Creates a number of renders for config files in a"
                << std::endl
                << " given directory. For use with testing goldfish."
                << std::endl
                << " Only works for single volumes, no timeseries data"
                << std::endl;
        }
        std::cerr << "Usage: " << argv[0] << " <config dir>" << std::endl;
        return 1;
    }

    DIR *directory = opendir(argv[1]);
    struct dirent *ent;
    std::vector<std::string> configFiles;

    if(directory != NULL)
    {
        ent = readdir(directory);
        while(ent != NULL)
        {
            if(ent->d_name[0] != '.')
            {
                std::string fullname = std::string(argv[1])+'/'+ent->d_name;
                configFiles.push_back(fullname);
            }
            ent = readdir(directory);
        }
    }
    else
    {
        std::cerr << "Could not open directory " << argv[1] << std::endl;
        return 1;
    }

    for(int configIndex = 0; configIndex < configFiles.size(); configIndex++)
    {
        std::cerr << "file: " << configFiles[configIndex] << std::endl;

        pbnj::ConfigReader *reader = new pbnj::ConfigReader();
        rapidjson::Document json;
        reader->parseConfigFile(configFiles[configIndex], json);
        pbnj::Configuration *config = new pbnj::Configuration(json);

        pbnj::pbnjInit(&argc, argv);

        unsigned int numRenders = 1;
        unsigned int specularSteps = 10;
        float specularDelta = 0.1;

        pbnj::Volume *volume;
        pbnj::Streamlines *streamlines;
        pbnj::Particles *particles;

        bool isSurface = false;
        pbnj::CONFTYPE confType = config->getConfigType(config->dataFilename);
        if(confType == pbnj::PBNJ_VOLUME)
        {
            pbnj::CONFSTATE confState = config->getConfigState();
            if(confState == pbnj::SINGLE_VAR)
            {
                volume = new pbnj::Volume(config->dataFilename,
                        config->dataVariable, config->dataXDim, config->dataYDim,
                        config->dataZDim);
            }
            else if(confState == pbnj::SINGLE_NOVAR)
            {
                volume = new pbnj::Volume(config->dataFilename, config->dataXDim,
                        config->dataYDim, config->dataZDim);
            }
            isSurface = (config->isosurfaceValues.size() > 0);
        }
        else if(confType == pbnj::PBNJ_STREAMLINES)
        {
            streamlines = new pbnj::Streamlines(config->dataFilename);
            isSurface = true;
        }
        else if(confType == pbnj::PBNJ_PARTICLES)
        {
            particles = new pbnj::Particles(config->dataFilename);
            isSurface = true;
        }
        else
        {
            std::cerr << "Config file either invalid or unsupported";
            std::cerr << std::endl;
        }

        pbnj::Camera *camera = new pbnj::Camera(config->imageWidth, 
                config->imageHeight);
        camera->setPosition(config->cameraX, config->cameraY, config->cameraZ);
        camera->setUpVector(config->cameraUpX, config->cameraUpY,
                config->cameraUpZ);
        if(confType == pbnj::PBNJ_STREAMLINES &&
                config->dataFilename.find("bfield") == std::string::npos)
        {
            camera->setView(-config->cameraX, -config->cameraY, -50);
        }
        else
        {
            camera->centerView();
        }

        std::random_device rand_device;
        std::mt19937 generator(rand_device());
        std::uniform_int_distribution<> cam_x(-1.0*config->dataXDim,
                1.0*config->dataXDim);
        std::uniform_int_distribution<> cam_y(-1.0*config->dataYDim,
                1.0*config->dataYDim);
        std::uniform_int_distribution<> cam_z(-1.0*config->dataZDim,
                1.0*config->dataZDim);

        /*
        if(confType == pbnj::PBNJ_STREAMLINES)
        {
            cam_x = std::uniform_int_distribution<>(streamlines->getBounds()[0], streamlines->getBounds()[1]);
            cam_y = std::uniform_int_distribution<>(streamlines->getBounds()[2], streamlines->getBounds()[3]);
            cam_z = std::uniform_int_distribution<>(streamlines->getBounds()[4], streamlines->getBounds()[5]);
        }
        */

        pbnj::Renderer *renderer = new pbnj::Renderer();
        renderer->setSamples(config->samples);
        renderer->setBackgroundColor(config->bgColor);
        renderer->setCamera(camera);

        if(!isSurface)
        {
            volume->setColorMap(config->colorMap);
            volume->setOpacityMap(config->opacityMap);
            volume->attenuateOpacity(config->opacityAttenuation);
        }

        bool volumeRender = (config->isosurfaceValues.size() == 0);

        for(int render = 0; render < numRenders; render++)
        {
            for(int step = 0; step < specularSteps; step++)
            {
                float Ks = specularDelta * step;
                if(confType == pbnj::PBNJ_STREAMLINES)
                {
                    streamlines->setSpecular(Ks);
                    renderer->addSubject(streamlines);
                }
                else if(confType == pbnj::PBNJ_PARTICLES)
                {
                    particles->setSpecular(Ks);
                    renderer->addSubject(particles);
                }
                else
                {
                    std::cerr << "oops" << std::endl;
                }
                std::string imageFilename = imageFamily(config->imageFilename, render, Ks);
                renderer->renderImage(imageFilename);
                std::cout << "rendered to " << imageFilename << std::endl;
            }

            float cx = cam_x(generator), cy = cam_y(generator), cz = cam_z(generator);
            std::cerr << "DEBUG: " << render << " " << cx << " " << cy << " " << cz << std::endl;

            camera->setPosition(cx, cy, cz);
            camera->setUpVector(0, 1, 0);
            camera->centerView();
            //std::vector<float> streamlinesCenter = streamlines->getCenter();
            //std::vector<float> view = {cx - streamlinesCenter[0], cy - streamlinesCenter[1], cz - streamlinesCenter[2]};
            //camera->setView(streamlines->getCenter());
        }
            /*
            if(volumeRender)
            {
                //renderer->setVolume(volume);
                renderer->addSubject(volume);
                std::string imageFilename = imageFamily(config->imageFilename, render);
                renderer->renderImage(imageFilename);
            }
            else
            {
                //renderer->setIsosurface(volume, config->isosurfaceValues);
                for(int step = 0; step < specularSteps; step++)
                {
                    float Ks = specularDelta * step;
                    //renderer->setMaterialSpecular(Ks);
                    renderer->addSubject(volume, Ks);
                    std::string imageFilename = imageFamily(config->imageFilename,
                            render, Ks);
                    renderer->renderImage(imageFilename);
                }
            }
        }
        */
        //std::string imageFilename = imageFamily(config->imageFilename, numRenders);
        //renderer->renderImage(imageFilename);
    }

    return 0;
}
