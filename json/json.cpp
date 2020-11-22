#include "json.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstring>

namespace {
    std::string& ltrim(std::string& str, const std::string& chars)
    {
        str.erase(0, str.find_first_not_of(chars));
        return str;
    }

    std::string& rtrim(std::string& str, const std::string& chars)
    {
        str.erase(str.find_last_not_of(chars) + 1);
        return str;
    }
    
    std::string& trim(std::string& str, const std::string& chars)
    {
        return ltrim(rtrim(str, chars), chars);
    }
}

namespace json{
    Value::Value(){
        this->m_value = nullptr;
        this->set(nullptr, data_type::NONE);
    }
    Value::Value(void * value, data_type type){
        this->m_value = nullptr;
        this->set(value, type);
    }
    void Value::set(void* value, data_type type){
        if (this->m_value){
            delete this->m_value;
        }
        this->m_value = value;
        this->m_type = type;
    }
    Value& Value::operator=(const Value& other){
        this->clear();
        this->m_type = other.m_type;
        if (this->m_type == data_type::INTEGER){
            this->m_value = new int;
            std::memcpy(this->m_value, other.m_value, sizeof(int));
        }
        else if (this->m_type == data_type::HASH){
            this->m_value = new JSON::dict;
            std::memcpy(this->m_value, other.m_value, sizeof(JSON::dict));
        }
        else if (this->m_type == data_type::REAL){
            this->m_value = new double;
            std::memcpy(this->m_value, other.m_value, sizeof(double));
        }
        else if (this->m_type == data_type::STRING){
            this->m_value = new std::string;
            std::memcpy(this->m_value, other.m_value, sizeof(std::string));
        }   
        else{
            throw "NOT IMPLEMENTED!";
        }
        return *this;
    }
    Value& Value::operator=(const Value* other){
        if (!other){
            this->clear();
            return *this;
        }
        *this = *other;
        return *this;
    }
    const data_type& Value::type(){
        return this->m_type;
    } 
    void Value::clear(){
        if (m_value){
            if (this->m_type == data_type::INTEGER){
                delete static_cast<int*>(m_value);
            }
            else if (this->m_type == data_type::REAL){
                delete static_cast<double*>(m_value);
            }
            else if (this->m_type == data_type::HASH){
                delete static_cast<JSON::dict*>(m_value);
            }
            else if (this->m_type == data_type::STRING){
                delete static_cast<std::string*>(m_value);
            }
            else if (this->m_type == data_type::LIST){
                std::vector<Value*>* p = static_cast<std::vector<Value*>*>(this->m_value);
                for (auto i: *p){
                    delete i;
                }
                delete p;
            }
            
            this->m_value = nullptr;
            this->m_type = data_type::NONE;
        }
    }
    std::ostream& operator<<(std::ostream& os, Value& value){
        if (value.m_value){
            if (value.m_type == data_type::INTEGER){
                os << value.value<int>();
            }
            else if (value.m_type == data_type::REAL){
                os << value.value<double>();
            }
            else if (value.m_type == data_type::HASH){
                os << value.value<JSON>();
            }
            else if (value.m_type == data_type::STRING){
                os << value.value<std::string>();
            }
            else if (value.m_type == data_type::LIST){
                auto& p = value.value<std::vector<Value*>>();
                os << "[";
                for (auto& it: p){
                    if (p.back() == it){
                        os << (*it);
                        continue;
                    }
                    os << (*it) << ",";
                }
                os << "]";
            }
        }
        else {
            os << "empty";
        }
        return os;
    }
    std::ostream& operator<<(std::ostream& os, Value* value){
        os << *value;
        return os;
    }
    std::ostream& operator<<(std::ostream& os, JSON& value)
    {  
        os << "{";
        auto t = value.m_json.begin();
        while (t != value.m_json.end()) {
            os << "\"" << t->first << "\": " << t->second;
            t++;
            if (t != value.m_json.end()) {
                os << ",";
            }
        }
        os << "}";
        return os;
    }
    Value::~Value(){
        this->clear();
    }
    Token::Token(std::string value, token_type type){
        this->value = value;
        this->type = type;
    }
    JSON::JSON(std::string filepath){
        if (filepath != ""){
            this->load_from_file(filepath);
        }
    }
    bool JSON::load_from_file(std::string filepath){
        std::ifstream file;
        file.open(filepath);

        if (file.is_open()){
            std::stringstream ss;
            while(!file.eof()){
                std::string str;
                std::getline(file, str);
                ss << str;
            }
            auto tokens = Tokenize(ss.str());
            Lexer(tokens, *this);
            return true;
        }
        std::cout << "FILE NOT FOUND! FILE PATH: " << filepath << "\r\n";
        return false;        
    }
    bool JSON::load_from_string(std::string json_str) {
        auto tokens = Tokenize(json_str);
        Lexer(tokens, *this);
        return true;
    }
    Value& JSON::operator[](const std::string str){
        if (m_json.find(str) == m_json.end()){
            m_json[str] = std::unique_ptr<Value>(new Value);
        }
        return *m_json[str];
    }
    JSON::~JSON(){
    }
    std::vector<Token> JSON::Tokenize(std::string str){
        std::vector<Token> Tokens;
        std::unordered_map<char, token_type> map2type;
        std::string token = "";
        size_t i = 0;

        auto check_space = [&](char t) -> bool{
            return isspace(t)
                && (
                    Tokens.back().type != token_type::QUOTE
                    || Tokens.back().type == token_type::COLON
                )
                && !token.size();
        };

        map2type['('] = token_type::LEFT_BRACKET;
        map2type[')'] = token_type::RIGHT_BRACKET;
        map2type['{'] = token_type::LEFT_BRACKET_F;
        map2type['}'] = token_type::RIGHT_BRACKET_F;
        map2type['['] = token_type::LEFT_BRACKET_S;
        map2type[']'] = token_type::RIGHT_BRACKET_S;
        map2type[':'] = token_type::COLON;
        map2type[','] = token_type::COMMA;
        map2type['.'] = token_type::DOT;
        map2type['\"'] = token_type::QUOTE;
        
        while(i < str.size()){
            char t = str[i];
            if (map2type.find(t) != map2type.end()){
                if (token.size()){
                    token_type type = token_type::WORD;
                    Tokens.push_back(Token(token, type));
                    token = "";
                }
                token_type type = map2type[t];
                token += t;
                Tokens.push_back(Token(token, type));
                token = "";
                i++;
                continue;
            }
            if (check_space(t)){
                i++;
                continue;
            }
            token += t;
            i++;
        }

        return Tokens;
    }
    int JSON::Lexer(std::vector<Token> tokens, dict& j){
        size_t i = 0;
        auto check_key = [&](Token& token)-> bool{
            return token.type == token_type::QUOTE // check if this is not string structure
                    && (
                        i == 0 // if starting point, this is true
                        || (i > 0) // if this is value
                        && tokens[i-1].type != token_type::COLON // check not : because this is must be a value
                    );
        };

        auto parseHash = [&]()-> dict*{
            dict* temp_dict = new dict();
            i += Lexer(std::vector<Token>(tokens.cbegin() + i, tokens.cend()), *temp_dict);
            return temp_dict;
        };

        auto parseStr = [&]()-> std::string* {
            std::string temp_str = "";
            i++;
            Token token = tokens[i];
            while (token.type != token_type::QUOTE) {
                temp_str += token.value;
                i++;
                token = tokens[i];
            }
            std::string* str = new std::string(temp_str);
            i++;
            token = tokens[i];
            return str;
        };

        auto parseValue = [&](Token token, Value& v)-> Value* {
            std::string temp_str;
            while (token.type != token_type::COMMA 
                && token.type != token_type::RIGHT_BRACKET_F 
                && token.type != token_type::RIGHT_BRACKET_S
            ) {
                temp_str += token.value;
                i++;
                token = tokens[i];
            }

            if (temp_str.find('.') != std::string::npos) {
                double* d = new double();
                *d = std::stod(temp_str);
                v.set((void*)d, data_type::REAL);
            }
            else {
                int* integer = new int();
                *integer = std::stoi(temp_str);
                v.set((void*)integer, data_type::INTEGER);
            }
            return &v;
        };

        while(i < tokens.size()){
            auto token = tokens[i];
            if (token.type == token_type::LEFT_BRACKET_F){
                i++;
                continue;
            }
            if (token.type == token_type::RIGHT_BRACKET_F){
                i++;
                break;
            }
            if (check_key(token)){
                i++;
                token = tokens[i];
                std::string key = token.value;
                j[key] = nullptr;
                i+=3;
                token = tokens[i];
                if (token.type == token_type::LEFT_BRACKET_F){ // parse hash
                    j[key].set((void*)(parseHash()), data_type::HASH);
                    continue;
                }
                if (token.type == token_type::QUOTE) {
                    j[key].set(parseStr(), data_type::STRING);
                    token = tokens[i];
                    continue;
                }

                if (token.type == token_type::LEFT_BRACKET_S){
                    std::vector<Value*>* list = new std::vector<Value*>();
                    i++;
                    token = tokens[i];
                    std::string temp_str;
                    while(token.type != token_type::RIGHT_BRACKET_S){
                        if (token.type == token_type::QUOTE){
                            list->push_back(
                                new Value(
                                    parseStr(),
                                    data_type::STRING
                                )
                            );
                            token = tokens[i];
                            continue;
                        }
                        if (token.type == token_type::LEFT_BRACKET_F) { // parse hash
                            list->push_back(
                                new Value(
                                    parseHash(),
                                    data_type::HASH
                                )
                            );
                            token = tokens[i];
                            continue;
                        }

                        if (token.type != token_type::COMMA) {
                            Value* v = parseValue(token, *(new Value()));
                            list->push_back(v);
                        }
                        if (token.type == token_type::RIGHT_BRACKET_S) {
                            break;
                        }
                        i++;
                        token = tokens[i];
                    }
                    j[key].set((void*)(list), data_type::LIST);
                    i++;
                    continue;
                }
                
                // parse real or integer
                parseValue(token, j[key]);
                continue;
            }
            i++;
        }
        return i;
    }
}