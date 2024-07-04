#include <iostream>
#include <fstream>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/endian/conversion.hpp>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <ctime>

#include "../include/tts.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace uuids = boost::uuids;
namespace endian = boost::endian;

using tcp = net::ip::tcp;
using json = nlohmann::json;

class session {
    net::io_context ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    ssl::stream<beast::tcp_stream> stream_;
    websocket::stream<ssl::stream<beast::tcp_stream>> ws_;

    beast::flat_buffer buffer_;

    string rate_;
    string volume_;
    string pitch_;

    const string trusted_client_token = "6A5AA1D4EAFF4E9FB37E23D68491D6F4";

    public:
        session() : 
            ctx_(ssl::context::tlsv12_client), resolver_(ioc_), 
            stream_(ioc_, ctx_), ws_(ioc_, ctx_)
        {}

        int voices(json& v) {
            try {
                const string voices_host = "speech.platform.bing.com";
                const string voices_port = "443";
                string voices_uri = "/consumer/speech/synthesize/readaloud/voices/list?TrustedClientToken="
                                    + trusted_client_token;
                const int version = 11;

                auto const results = resolver_.resolve(voices_host, voices_port);
                beast::get_lowest_layer(stream_).connect(results);
                stream_.handshake(ssl::stream_base::client);

                http::request<http::string_body> req(http::verb::get, voices_uri, version);
                req.set(http::field::host, voices_host);

                http::write(stream_, req);

                http::response<http::dynamic_body> res;
                size_t bytes = http::read(stream_, buffer_, res);
                v = json::parse(beast::buffers_to_string(res.body().data()));
                buffer_.consume(bytes);

                stream_.lowest_layer().close();
                return 0;
            } catch (exception const& e) {
                cout << "exception error: " << e.what() << endl;
            }
            return -1;
        }

        int set(const string& rate, 
                const string& volume,
                const string& pitch) {
            rate_ = rate;
            volume_ = volume;
            pitch_ = pitch;
            return 0;
        }

        int connect() {
            try {
                const string ws_host = "speech.platform.bing.com";
                const string ws_port = "443";
                string ws_uri = "/consumer/speech/synthesize/readaloud/edge/v1?TrustedClientToken=" 
                                + trusted_client_token + "&ConnectionId=" + connect_id();

                auto results = resolver_.resolve(ws_host, ws_port);
                beast::get_lowest_layer(ws_).connect(results);
                ws_.next_layer().handshake(ssl::stream_base::client);

                ws_.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
                    req.set(http::field::pragma, "no-cache");
                    req.set(http::field::cache_control, "no-cache");
                    req.set(http::field::origin, "chrome-extension://jdiccldimpdaibmpdkjnbmckianbfold");
                    req.set(http::field::accept_encoding, "gzip, deflate, br");
                    req.set(http::field::accept_language, "en-US,en;q=0.9");
                    req.set(http::field::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
                                                     " (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41");
                }));
                ws_.handshake(ws_host + ":" + ws_port, ws_uri);
                ws_.write(net::buffer(command_str()));
                return 0;
            } catch(exception const& e) {
                cout << "exception error: " << e.what() << endl;
            }
            return -1;
        }

        int close() {
            try {
                ws_.close(websocket::close_code::normal);
            } catch (exception const& e) {
                //cout << "exception error: " << e.what() << endl;
            }
            return 0;
        }

        int request(const string& voice, const string& text, const string& outfile) {
            try {
                ofstream o(outfile, ios::binary);
                ws_.write(net::buffer(ssml_str(voice, text)));

                beast::flat_buffer buffer;
                for (;;) {
                    size_t bytes = ws_.read(buffer);
                    if (ws_.got_text()) {
                        string data = beast::buffers_to_string(buffer.data());
                        map<string, string> headers;
                        parse_headers(data.substr(0, data.find("\r\n\r\n")), headers);
                        if (headers["Path"] == "turn.end") break;
                    } else if (ws_.got_binary()) {
                        string data = beast::buffers_to_string(buffer.data());
                        unsigned short headers_length = endian::endian_reverse(*((unsigned short *)data.c_str()));
                        o << data.substr(sizeof(unsigned short) + headers_length);
                    } else {
                        break;
                    }
                    buffer.consume(bytes);
                }
                o.close();
            } catch (exception const& e) {
                cout << "request exception error: " << e.what() << endl;
                return -1;
            }
            return 0;
        }

    private:
        string connect_id() {
            uuids::random_generator gen;
            uuids::uuid u = gen();
            return to_string(u);
        }

        string date_to_string() {
            time_t now = time(nullptr);
            char mbstr[100];
            strftime(mbstr, sizeof(mbstr), "%c %Z", localtime(&now));
            return mbstr;
        }

        string command_str() {
            return fmt::format("X-Timestamp:{}\r\n"
                               "Content-Type:application/json; charset=utf-8\r\n"
                               "Path:speech.config\r\n\r\n"
                               "{{\"context\":{{\"synthesis\":{{\"audio\":{{\"metadataoptions\":{{"
                               "\"sentenceBoundaryEnabled\":false,\"wordBoundaryEnabled\":true}},"
                               "\"outputFormat\":\"audio-24khz-48kbitrate-mono-mp3\""
                               "}}}}}}}}\r\n", date_to_string());
        }

        string ssml_str(const string& voice, const string& text) {
            string ssml = fmt::format("<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-US'>"
                                      "<voice name='{}'><prosody pitch='{}' rate='{}' volume='{}'>"
                                      "{}</prosody></voice></speak>", voice, pitch_, rate_, volume_, text);
            return fmt::format("X-RequestId:{}\r\n"
                               "Content-Type:application/ssml+xml\r\n"
                               "X-Timestamp:{}\r\n"
                               "Path:ssml\r\n\r\n"
                               "{}", connect_id(), date_to_string(), ssml);
        }

        int parse_headers_line(const string& line, map<string, string>& headers) {
            size_t pos = line.find(":");
            if (pos == string::npos) return -1;
            headers[line.substr(0, pos)] = line.substr(pos + strlen(":"));
            return 0;
        }

        int parse_headers(const string& data, map<string, string>& headers) {
            string s = data;
            while (s.length() > 0) {
                size_t pos = s.find("\r\n");
                string line = (pos != string::npos) ? s.substr(0, pos) : s;
                parse_headers_line(line, headers);
                if (pos == string::npos) break;
                s = s.substr(pos + strlen("\r\n"));
            }

            return 0;
        }
};

static session cli;

int voices_list(vector<tuple<string, string>>& voices) {
    json v;
    if (cli.voices(v) < 0) return -1;

    for (auto item: v) {
        voices.emplace_back(make_tuple(item["ShortName"].get<string>(), 
                                       item["Gender"].get<string>()));
    }
    return 0;
}

int tts_open(const string& rate, const string pitch, const string& volume) {
    if (cli.connect() < 0) return -1;
    cli.set(rate, volume, pitch);
    return 0;
}

int tts_request(const string& voice, const string& text, const string& outfile) {
    if (cli.request(voice, text, outfile) < 0) return -1;
    return 0;
}

int tts_close() {
    cli.close();
    return 0;
}