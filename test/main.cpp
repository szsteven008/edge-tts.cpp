#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <chrono>
#include <boost/program_options.hpp>

#include "fmt/format.h"
#include "../include/tts.h"

namespace po = boost::program_options;

void usage(const char * name, po::options_description& opts) {
    cout << "usgae: " << name 
         << " [-h] [-l] [-t TEXT] [-v VOICE] [--rate RATE] [--volume VOLUME] [--pitch PITCH]" 
         << " [-o OUTFILE] [--proxy PROXY]" << endl << endl << "Microsoft Edge TTS" << endl << endl 
         << "optional arguments:" << endl << opts << endl;
}


string mktimestamp(int t) {
    int hours = (t / 10000000) / 3600;
    int minutes = ((t / 10000000) % 3600) / 60;
    int seconds = ((t / 10000000) % 3600) % 60;
    int millions = (t % 10000000) / 10000;

    return fmt::format("{:02d}:{:02d}:{:02d}.{:03d}", 
                       hours, minutes, seconds, millions);
}

void echo_subtitle(const vector<tuple<int, int, string>>& v, int& start) {
    cout << mktimestamp(start) << " --> ";

    int end = get<0>(*(v.end() - 1));
    cout << mktimestamp(end) << endl;
    start = end;

    for (auto item: v) {
        string text;
        tie(ignore, ignore, text) = item;
        cout << text << " ";
    }
    cout << endl << endl;
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
        ("webvtt,w", "show subtitles")
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

            vector<tuple<int, int, string>> meta;
            vector<string> data;

            tts_request(voice, text, meta, data);

            ofstream o(outfile);
            for (string item: data) {
                o << item;
            }
            o.close();

            if (vm.count("webvtt") > 0) {
                cout << endl << endl;

                cout << "WEBVTT" << endl << endl;
                int words = 10;
                int lines = meta.size() / words;
                int start = get<0>(meta[0]);
                for (int i=0; i<=lines; i++) {
                    if ((i * words) >= meta.size()) break;

                    vector<tuple<int, int, string>> v;
                    if (i == lines) {
                        v.assign(meta.begin() + i * words, meta.end());
                    } else {
                        v.assign(meta.begin() + i * words, meta.begin() + (i + 1) * words);
                    }
                    echo_subtitle(v, start);
                }

                cout << endl << endl;
            }

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