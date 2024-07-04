#pragma once

#include <string>
#include <vector>
#include <tuple>

using namespace std;

int voices_list(vector<tuple<string, string>>& voices);
int tts_open(const string& rate, const string pitch, const string& volume);
int tts_request(const string& voice, const string& text, const string& outfile);
int tts_close();
