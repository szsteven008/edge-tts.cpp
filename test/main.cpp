#include <iostream>
#include <boost/program_options.hpp>

#include "../include/tts.h"

namespace po = boost::program_options;

void usage(const char * name, po::options_description& opts) {
    cout << "usgae: " << name 
         << " [-h] [-l] [-t TEXT] [-v VOICE] [--rate RATE] [--volume VOLUME] [--pitch PITCH]" 
         << " [-o OUTFILE] [--proxy PROXY]" << endl << endl << "Microsoft Edge TTS" << endl << endl 
         << "optional arguments:" << endl << opts << endl;
}

int main(int argc, char * argv[]) {
    po::options_description opts;
    opts.add_options()
        ("help,h", "show this help message and exit")
        ("list-voices,l", "lists available voices and exits")
        ("text,t", po::value<string>()->value_name("TEXT"), "what TTS will say")
        ("voice,v", po::value<string>()->value_name("VOICE")->default_value("en-US-AriaNeural"), "voice for TTS. Default: en-US-AriaNeural")
        ("rate", po::value<string>()->value_name("RATE")->default_value("+0%"), "set TTS rate. Default +0%.")
        ("volume", po::value<string>()->value_name("VOLUME")->default_value("+0%"), "set TTS volume. Default +0%.")
        ("pitch,r", po::value<string>()->value_name("PITCH")->default_value("+0Hz"), "set TTS pitch. Default +0Hz.")
        ("outfile,o", po::value<string>()->value_name("OUTFILE")->default_value("output.mp3"), "set TTS output mp3 file. Default output.mp3.")
        ("proxy", po::value<string>()->value_name("PROXY"), "use a proxy for TTS and voice list.")
        ;
    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, opts), vm);
        po::notify(vm);

        if (vm.count("list-voices") > 0) {
            vector<tuple<string, string>> voices;
            if (voices_list(voices) < 0) return -1;
            for (auto voice: voices) {
                string name, gender;
                tie(name, gender) = voice;
                cout << "Name: " << name << " Gender: " << gender << endl;
            }
            return 0;
        } else if (vm.count("text") > 0) {
            string text = vm["text"].as<string>();
            string outfile = vm["outfile"].as<string>();
            string rate = vm["rate"].as<string>();
            string volume = vm["volume"].as<string>();
            string pitch = vm["pitch"].as<string>();
            string voice = vm["voice"].as<string>();

            cout << "voice: " << voice << " text: " << text << endl;

            if (tts_open(rate, pitch, volume) < 0) {
                cout << "fail to open the connection." << endl;
                return -1;
            }

            tts_request(voice, text, outfile);

            tts_close();

            cout << outfile << " is ok!" << endl;
            return 0;
        } else {
            usage(argv[0], opts);
        }
    } catch (exception e) {
        cout << "error: " << e.what() << endl;
        usage(argv[0], opts);
    }

    return 0;
}