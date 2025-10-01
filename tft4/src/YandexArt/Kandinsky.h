#pragma once
#include <Arduino.h>
// config
#define FUSION_HOST "llm.api.cloud.yandex.net"
#define FUSION_PORT 443
#define FUSION_PERIOD 30000
#define FUSION_TRIES 1
#define FUS_LOG(x) Serial.println(x)
// #define GHTTP_HEADERS_LOG Serial
#include <GSON.h>
#include <ArduinoJson.h>
#include <GyverHTTP.h>
#include "StreamB64.h"
#include "tjpgd/tjpgd.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#define FUSION_CLIENT BearSSL::WiFiClientSecure
#else
#include <WiFi.h>
#include <WiFiClientSecure.h>
#define FUSION_CLIENT WiFiClientSecure
#endif

// Константы, определяющие внутреннюю область
const int INTERNAL_X = 48;
const int INTERNAL_Y = 80;
const int INTERNAL_WIDTH = 320;
const int INTERNAL_HEIGHT = 480;

// Вспомогательные макросы для вычисления минимума/максимума
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


class Kandinsky {
    typedef std::function<void(int x, int y, int w, int h, uint8_t* buf)> RenderCallback;
    typedef std::function<void()> RenderEndCallback;
    enum class State : uint8_t {
        GetModels,
        Generate,
        Status,
        GetStyles,
    };
   public:


    Kandinsky() {
        // Определение статических фильтров
         successFilter["id"] = true;
         successFilter["done"] = true;
         successFilter["error"] = true;

         errorFilter["error"] = true;

         // TODO Убрать заглушку
        //  _uuid = "fbvph7vco5udtap3b6k71";
    }

    Kandinsky(const String& api_id, const String& folder_id) : Kandinsky()  {
        setKey(api_id, folder_id);
    }

    void setKey(const String& api_id, const String& folder_id) {
        if (api_id.length() && folder_id.length()) {
            _api_id = api_id;
            _folder_id = folder_id;
        }
    }
    void onRender(RenderCallback cb) {
        _rnd_cb = cb;
    }
    void onRenderEnd(RenderEndCallback cb) {
        _end_cb = cb;
    }
    // 1, 2, 4, 8
    void setScale(uint8_t scale) {
        switch (scale) {
            case 1: _scale = 0; break;
            case 2: _scale = 1; break;
            case 4: _scale = 2; break;
            case 8: _scale = 3; break;
            default: _scale = 0; break;
        }
    }
    bool begin() {
        if (!_api_id.length()) return false;
        if (!_folder_id.length()) return false;
        return false;
    }

    bool getStyles() {
        // if (!_api_key.length()) return false;
        return false;
        //return request(State::GetStyles, "cdn.fusionbrain.ai", "/static/styles/web");
    }
    bool generate(Text query, uint16_t width = 512, uint16_t height = 512, Text style = "DEFAULT", Text negative = "") {
        _uuid = "fbv2ad972upgdk5qnu88";
        return true;

        status = "wrong config";
        if (!_api_id.length()) return false;
        if (!_folder_id.length()) return false;
        if (!query.length()) return false;
        //if (!_id.length()) return false;
        

        // Создание JSON-документа для тела запроса
        DynamicJsonDocument jsonDoc(256);
        jsonDoc["model_uri"] = "art://" + _folder_id + "/yandex-art/latest";
        // jsonDoc["model_uri"] = "art://b2g/yandex-art/latest";
        
        // // Создание массива messages
        JsonArray messages = jsonDoc.createNestedArray("messages");
        JsonObject message1 = messages.createNestedObject();
        message1["text"] = query;
        message1["weight"] = 1;
        
        // Создание объекта generation_options
        JsonObject generationOptions = jsonDoc.createNestedObject("generation_options");
        generationOptions["mime_type"] = "image/jpeg";
        
        JsonObject aspectRatio = generationOptions.createNestedObject("aspectRatio");
        aspectRatio["widthRatio"] = width;
        aspectRatio["heightRatio"] = height;
        



        // FUS_LOG("JSON being sent:");
        // FUS_LOG(json);

        String id;
        bool done;
        String errorMsg;

        uint8_t tries = FUSION_TRIES;
        while (tries--) {
            if (performGenerateHttpRequest(FUSION_HOST, "/foundationModels/v1/imageGenerationAsync", "POST", jsonDoc, id, done, errorMsg)) {
                FUS_LOG("Gen request sent");
                _tmr = millis();
                _uuid = id;
                if (!_uuid.length()) {
                    status = "operation ID unknown";
                    return false;
                } 
                status = "wait result";
                return true;
            } else {
                FUS_LOG("Gen request error");
                delay(2000);
            }
        }
        status = "gen request error";
        return false;
    }
    bool getImage() {
        if (!_api_id.length()) return false;
        if (!_uuid.length()) return false;
        FUS_LOG("Check status...");
        
        String errorMsg;
        String url("/operations/");
        url += _uuid;
        return performGetImageRequest(FUSION_HOST, url, "GET", errorMsg);
    }

    void tick() {
        if (_uuid.length() && millis() - _tmr >= FUSION_PERIOD) {
            _tmr = millis();
            getImage();
        }
    }
    String modelID() { return _id; }
    String styles = "";
    String status = "";
   private:
    String _folder_id;
    String _api_id;
    String _uuid;
    uint8_t _scale = 0;
    uint32_t _tmr = 0;
    String _id;
    RenderCallback _rnd_cb = nullptr;
    RenderEndCallback _end_cb = nullptr;
    StreamB64* _stream = nullptr;
    uint8_t _finalBuffer[512];

    // static
    static Kandinsky* self;

    // Фильтры для успешного ответа и ошибки
    StaticJsonDocument<200> successFilter;
    StaticJsonDocument<100> errorFilter;

    static size_t jd_input_cb(JDEC* jdec, uint8_t* buf, size_t len) {
        if (self) {
            self->_stream->readBytes(buf, len);
        }
        return len;
    }
   
    static int jd_output_cb1(JDEC* jdec, void* bitmap, JRECT* rect) {
        if (self && self->_rnd_cb) {
            // FUS_LOG("Rectangle");
            // FUS_LOG(rect->left);
            // FUS_LOG(rect->top);
            // FUS_LOG(rect->right - rect->left + 1);
            // FUS_LOG(rect->bottom - rect->top + 1);

            self->_rnd_cb(rect->left, rect->top, rect->right - rect->left + 1, rect->bottom - rect->top + 1, (uint8_t*)bitmap);
        }
        return 1;
    }

    static int jd_output_cb(JDEC* jdec, void* bitmap, JRECT* rect) {
        if (self && self->_rnd_cb) {
            filter_points(rect->left, rect->top, rect->right - rect->left + 1, rect->bottom - rect->top + 1, (uint8_t*)bitmap);
            // self->_rnd_cb(rect->left, rect->top, rect->right - rect->left + 1, rect->bottom - rect->top + 1, (uint8_t*)bitmap);
        }
        return 1;
    }

// Функция для фильтрации точек
    static void filter_points(int x, int y, int width, int height, uint8_t* src_buf) {
        // Вычисляем параметры пересечения прямоугольников
        int intersect_x = MAX(x, INTERNAL_X);
        int intersect_y = MAX(y, INTERNAL_Y);
        int intersect_x_end = MIN(x + width, INTERNAL_X + INTERNAL_WIDTH);
        int intersect_y_end = MIN(y + height, INTERNAL_Y + INTERNAL_HEIGHT);
        
        int new_width = intersect_x_end - intersect_x;
        int new_height = intersect_y_end - intersect_y;
        
        // Проверка на полное вхождение
        bool fully_contained = (x >= INTERNAL_X) && (y >= INTERNAL_Y) &&
                                ((x + width) <= (INTERNAL_X + INTERNAL_WIDTH)) &&
                                ((y + height) <= (INTERNAL_Y + INTERNAL_HEIGHT));
        
        if (fully_contained) {
            // Если входящая область полностью входит в внутреннюю, передаем исходный буфер
            self->_rnd_cb(x - INTERNAL_X, y - INTERNAL_Y, width, height, src_buf);
            return;
        }
        
        // Если входящая область не полностью входит, проверяем, есть ли пересечение
        if (new_width <= 0 || new_height <= 0) {
            // Если пересечение пустое, очищаем filtered_buf
            return;
        }
        
        // // Рассчитываем размер выходного буфера
        // int buffer_size = new_width * new_height * 2; // 2 байта на точку
        // // Перевыделяем память для filtered_buf, если необходимо
        // if (filtered_buf != nullptr) {
        //     delete[] filtered_buf;
        // }
        // filtered_buf = new uint8_t[buffer_size];
        
        // Копируем подходящие точки
        for (int dy = 0; dy < new_height; dy++) {
            for (int dx = 0; dx < new_width; dx++) {
                // Вычисляем координаты в исходном буфере
                int src_x = intersect_x - x + dx;
                int src_y = intersect_y - y + dy;
                
                // Индексы в памяти (по 2 байта на точку)
                int src_index = (src_y * width + src_x) * 2;
                int dst_index = (dy * new_width + dx) * 2;
                
                // Копирование данных
                self->_finalBuffer[dst_index]     = src_buf[src_index];
                self->_finalBuffer[dst_index + 1] = src_buf[src_index + 1];
            }
        }
        
        // Вызов целевой функции с новыми параметрами
        self->_rnd_cb(intersect_x - INTERNAL_X, intersect_y - INTERNAL_Y, new_width, new_height, self->_finalBuffer);
    }

    // system
    bool performGenerateHttpRequest(Text host, Text url, Text method, DynamicJsonDocument& jsonDoc, String& id, bool& done, String& errorMsg) {
        // Сериализация JSON в строку
        String jsonString;
        serializeJson(jsonDoc, jsonString);
        
        // Установка заголовков
        ghttp::Client::Headers headers;
        headers.add("Content-Type", "application/json");
        headers.add("Authorization", "Api-Key " + _api_id);
        headers.add("Accept", "*/*");
        
        FUSION_CLIENT client;
#ifdef ESP8266
        client.setBufferSizes(512, 512);
#endif
        client.setInsecure();
        ghttp::Client http(client, host.str(), FUSION_PORT);


        // Отправка запроса с JSON в теле
        // String body = "{\"model_uri\":\"art://b1g/yandex-art/latest\"}";

        bool ok = http.request(url, method, headers, jsonString);
        
        if (!ok) {
            FUS_LOG("Request error");
            http.flush();
            return false;
        }
        
        FUS_LOG("Host " + host.toString());
        FUS_LOG("Url " + url.toString());
        FUS_LOG("Body " + jsonString);
        FUS_LOG("Headers");
        FUS_LOG(headers);

        // Получение ответа
        ghttp::Client::Response resp = http.getResponse();
        
        if (resp) {
            FUS_LOG("Response");      
        } else {
            FUS_LOG("Response not exists");
            http.flush();
            return false;               
        }
        
        StreamReader responseBodyReader(resp.body());

        int httpStatus = resp.code();
        FUS_LOG("Status " + String(httpStatus));        
        
        // Проверка HTTP статуса
        if (httpStatus < 200 || httpStatus > 299) {
            // Парсинг ответа с использованием статического фильтра для ошибки

            DynamicJsonDocument docError(100);
            DeserializationError err = deserializeJson(docError, responseBodyReader, DeserializationOption::Filter(errorFilter));

            if (err) {
                FUS_LOG("Failed to parse response JSON for error");
                FUS_LOG(err.c_str());
                http.flush();
                return false;
            }
            
            // Извлечение поля "error"
            if (docError.containsKey("error")) {
                errorMsg = docError["error"].as<String>();
            } else {
                errorMsg = "Unknown error";
            }
            FUS_LOG(errorMsg.c_str());
            http.flush();
            return false;
        }
        
        DynamicJsonDocument docSuccess(200);
        DeserializationError err = deserializeJson(docSuccess, responseBodyReader, DeserializationOption::Filter(successFilter));
        
        if (err) {
            FUS_LOG("Failed to parse response JSON for success");
            FUS_LOG(err.c_str());
            http.flush();
            return false;
        }
        
        // Извлечение интересующих полей
        if (docSuccess.containsKey("id")) {
            id = docSuccess["id"].as<String>();
        } else {
            id = "";
        }
        
        if (docSuccess.containsKey("done")) {
            done = docSuccess["done"].as<bool>();
        } else {
            done = false;
        }
        FUS_LOG("Operation id " + id);
        http.flush();
        return true;
    }


    // -----------------------------

    bool performGetImageRequest(Text host, Text url, Text method, String& errorMsg) {
        FUSION_CLIENT client;
// #ifdef ESP8266
        // client.setBufferSizes(512, 512);
// #endif
        client.setInsecure();
        ghttp::Client http(client, host.str(), FUSION_PORT);
        // Установка заголовков
        ghttp::Client::Headers headers;
        headers.add("Content-Type", "application/json");
        headers.add("Authorization", "Api-Key " + _api_id);
        headers.add("Accept", "*/*");


        FUS_LOG("Host " + host.toString());
        FUS_LOG("Url " + url.toString());
        // FUS_LOG("Body " + jsonString);
        FUS_LOG("Headers");
        FUS_LOG(headers);


        bool ok = http.request(url, method, headers);

        if (!ok) {
            FUS_LOG("Request error");
            http.flush();
            return false;
        }

        ghttp::Client::Response resp = http.getResponse();

        int httpStatus = resp.code();
        FUS_LOG("Response code: " + String(httpStatus));

        if (resp) {
            FUS_LOG("Response");      
        } else {
            FUS_LOG("Response not exists");
            http.flush();
            return false;               
        }
        // int httpStatus = resp.code();
        // FUS_LOG("Response code: " + String(httpStatus));

        // httpStatus = 200;

        // String plugStr = "{\"id\":\"fbva1kvaki5eho7ssp61\",";
        // plugStr  +=  "\"description\":\"\",\"createdAt\":null,\"createdBy\":\"\",\"modifiedAt\":null,\"done\":true,\"metadata\":null,";
        //   plugStr  += "\"response\":{\"@type\":\"type.googleapis.com/yandex.cloud.ai.foundation_models.v1.image_generation.ImageGenerationResponse\",";
        //   plugStr  += "\"image\":\"MTIzNDU2Nzg5YXNkZmc=\",\"modelVersion\":\"\"}}";

        
        
        // StreamReader bodyReader(bodyStreamPlug, bodyStreamPlug.len());



        if (httpStatus < 200 || httpStatus > 299) {
            FUS_LOG("Parsing error ...");
            // Парсинг ответа с использованием статического фильтра для ошибки

            StreamReader responseBodyReader(resp.body());
            DynamicJsonDocument docError(100);
            DeserializationError err = deserializeJson(docError, responseBodyReader, DeserializationOption::Filter(errorFilter));

            if (err) {
                FUS_LOG("Failed to parse response JSON for error");
                FUS_LOG(err.c_str());
                http.flush();
                return false;
            }
            
            // Извлечение поля "error"
            if (docError.containsKey("error")) {
                errorMsg = docError["error"].as<String>();
            } else {
                errorMsg = "Unknown error";
            }
            FUS_LOG(errorMsg.c_str());
            http.flush();
            return false;
        } else {
                FUS_LOG("Parsing success document ...");
                bool ok = parseStatus(resp.body());
                http.flush();
                return ok;
        }

        return false;
    }

    String readValue(Stream& stream) {
        String result = "";
        int c;
        
        int nextChar = stream.peek();

        while ((c = stream.read()) != -1) {
            FUS_LOG("Char " +  String((char)c));
             // Читаем символы из потока
            if (c == 'n' || c == 't' || c == 'f') {
                // Это null или true или false
                result = String((char)c) + stream.readStringUntil(',');
                break;
            } else if ( c == '"') {
               result = stream.readStringUntil('"');
               break;
            } else if ( c == ',' || c == '}') {
                break;
            }
                // Это какое-то неожиданное число
                // Просто пропустим
            
        }

        return result;
    }

    bool parseStatus(Stream& stream) {
        bool found = false;
        bool insideResult = false;
        while (stream.available()) {
            FUS_LOG("Find start key");
            stream.readStringUntil('"');
            FUS_LOG("Start key");
            String key = stream.readStringUntil('"');
            if (key == "response") {
                insideResult = true;
                continue;
            }
            if (insideResult && key == "image") {
                FUS_LOG("image found");
                found = true;
                break;
            }

            // Только что прочли ключ, значит едем до двоеточия
            stream.readStringUntil(':');
            FUS_LOG("---");
            FUS_LOG("Key " + key);            
            // Теперь читаем значение
            String val = readValue(stream);

            FUS_LOG("Val " + val);
            FUS_LOG("---");

            if (!key.length() ) {
                FUS_LOG("Key not found");
                break;
            }

            // TODO 
            if (key == "done") {
                switch (Text(val).hash()) {
                    case SH("true"):
                        FUS_LOG("Done is true");
                        // В ответе должен быть image
                        _uuid = "";
                        break;
                    case SH("false"):
                        FUS_LOG("Done is false");
                        return true;
                        break;
                    default:
                        FUS_LOG("Done is unknown");
                        return true;
                }
            }
        }

// TODO Убрать заглушку
// FUS_LOG("Exit from loop");
//         return false;
        
        if (found) {
            stream.readStringUntil('"');
            uint8_t* workspace = new uint8_t[TJPGD_WORKSPACE_SIZE];
            if (!workspace) {
                FUS_LOG("allocate error");
                return false;
            }
            JDEC jdec;
            jdec.swap = 0;
            JRESULT jresult = JDR_OK;
            StreamB64 sb64(stream);
            _stream = &sb64;
            self = this;
            jresult = jd_prepare(&jdec, jd_input_cb, workspace, TJPGD_WORKSPACE_SIZE, 0);
            if (jresult == JDR_OK) {
                jresult = jd_decomp(&jdec, jd_output_cb, _scale);
                if (jresult == JDR_OK && _end_cb) _end_cb();
            } else {
                FUS_LOG("jdec error");
            }
            self = nullptr;
            delete[] workspace;
            status = jresult == JDR_OK ? "gen done" : "jpg error";
            return jresult == JDR_OK;
        }
        return true;
    }



    bool parse(State state, gson::Parser& json) {
        // Удалить
        switch (state) {
            case State::GetStyles:
                styles = "";
                for (int i = 0; i < (int)json.rootLength(); i++) {
                    if (i) styles += ';';
                    json[i]["name"].addString(styles);
                }
                return styles.length();
            case State::GetModels:
                json[0]["id"].toString(_id);
                if (_id.length()) return true;
                break;
            case State::Generate:
                _tmr = millis();
                json["uuid"].toString(_uuid);
                if (_uuid.length()) return true;
                break;
            default:
                break;
        }
        return false;
    }
};




Kandinsky* Kandinsky::self __attribute__((weak)) = nullptr;
