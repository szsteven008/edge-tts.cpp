#include <iostream>
#include <sndfile.h>
#include <portaudio.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

using namespace std;

void usage(const string name, po::options_description& opts) {
    cout << "usage: " << name << " [-f FILE]" << endl 
         << "optional arguments:" << endl << opts << endl;
}

static int paCallback(const void *input, void *output,
                      unsigned long frameCount,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData) {
    SNDFILE * infile = (SNDFILE *)userData;
    float * out = (float *)output;
    sf_count_t bytes = sf_read_float(infile, out, frameCount);
    if (bytes < frameCount) {
        for (int i=bytes; i<frameCount; i++) {
            out[i] = 0;
        }
        return paComplete;
    }
    return paContinue;
}

int main(int argc, char * argv[]) {
    po::options_description opts;
    opts.add_options()
                    ("file,f", po::value<string>()->value_name("FILE"), "the mp3 file being played.");

    if (argc != 3) {
        usage(argv[0], opts);
        return -1;
    }

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);

    if (vm.count("file") == 0) {
        usage(argv[0], opts);
        return -1;
    }

    string filename = vm["file"].as<string>();
    SNDFILE * infile;
    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(SF_INFO));
    infile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (infile == nullptr) {
        cout << "fail to open " << filename << endl;
        return -1;
    }

    PaStreamParameters outputParameters;
    PaStream * stream;
    PaError err;

    const int frames_per_buffer = 64;

    err = Pa_Initialize();
    if (err != paNoError) goto PA_ERROR;

    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        cout << "No default output device." << endl;
        goto PA_ERROR;
    }
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream, 
                        nullptr, 
                        &outputParameters, 
                        sfinfo.samplerate, 
                        frames_per_buffer, 
                        0, 
                        paCallback, 
                        infile);
    if (err != paNoError) {
        cout << "fail to open stream. error: " << err << endl;
        goto PA_ERROR;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        cout << "fail to start stream. error: " << err << endl;
        goto PA_ERROR;
    }

    cout << filename << " is playing..." << endl;

    while (Pa_IsStreamActive(stream)) {
        Pa_Sleep(100);
    }

    Pa_CloseStream(stream);

    cout << "end." << endl;

PA_ERROR:
    Pa_Terminate();
    sf_close(infile);

    return 0;
}