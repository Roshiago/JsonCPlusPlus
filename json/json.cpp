#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>
#include <algorithm>

#include "json.h"

#define DEBUG 0

#if DEBUG == 1
    #define TODO(msg) (throw new std::runtime_error(msg))
    #define NOT_IMPLEMENTED (TODO("NOT IMPLEMENTED YET"))
#else
    #define TODO(msg)
    #define NOT_IMPLEMENTED
#endif

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

namespace json {
    Value::Value(void* value, ValueType type) {
        this->set(value, type);
    }

    Value::Value(ValuePair& pair) {
        this->set(pair.first, pair.second);
    }

    Value::Value(Value& pair) {
        *this = pair;
    }

    Value::Value(Value&& pair) noexcept {
        this->set(pair.m_value, pair.m_type);

        pair.m_type = ValueType::NONE;
        pair.m_value = nullptr;
    }

    void Value::set(void* value, ValueType type) {
        this->reset();

        this->m_value = value;
        this->m_type = type;
    }

    Value& Value::operator=(const Value& other) {
        this->reset();
        this->m_type = other.m_type;

        switch (this->m_type) {
            case ValueType::INTEGER: {
                this->m_value = new int;
                std::memcpy(this->m_value, other.m_value, sizeof(int));
                break;
            }

            case ValueType::JSON: {
                this->m_value = new JSON;
                std::memcpy(this->m_value, other.m_value, sizeof(JSON));
                break;
            }
            
            case ValueType::DOUBLE: {
                this->m_value = new double;
                std::memcpy(this->m_value, other.m_value, sizeof(double));
                break;
            }

            case ValueType::STRING: {
                this->m_value = new double;
                std::memcpy(this->m_value, other.m_value, sizeof(double));
                break;
            }

            default:
                NOT_IMPLEMENTED;
        }

        return *this;
    }

    Value& Value::operator=(const Value* other) {
        if (other == nullptr) {
            this->reset();
            return *this;
        }

        *this = *other;
        return *this;
    }

    Value& Value::operator=(const ValuePair& pair) {
        if (pair.first == nullptr) {
            this->reset();
            return *this;
        }

        this->set(pair.first, pair.second);

        return *this;
    }

    const ValueType& Value::type() const {
        return this->m_type;
    }

    void Value::reset() {
        if (this->m_value == nullptr) {
            return;
        }

        switch (this->m_type) {
            case ValueType::INTEGER: {
                delete static_cast<int*>(this->m_value);
                break;
            }

            case ValueType::JSON: {
                delete static_cast<PtrJson>(m_value);
                break;
            }

            case ValueType::DOUBLE: {
                delete static_cast<double*>(this->m_value);
                break;
            }

            case ValueType::STRING: {
                delete static_cast<std::string*>(m_value);
                break;
            }

            case ValueType::LIST: {
                PtrList p = static_cast<PtrList>(this->m_value);

                for (auto i : *p) {
                    delete i;
                }

                delete p;
                break;
            }

            default:
                NOT_IMPLEMENTED;
        }

        this->m_value = nullptr;
        this->m_type = ValueType::NONE;
    }

    std::ostream& operator<<(std::ostream& os, Value& v) {
        if (v.m_value == nullptr)
        {
            os << "<empty>";
            return os;
        }

        switch (v.m_type)
        {
        case ValueType::INTEGER: {
            os << v.value<int>();
            break;
        }

        case ValueType::DOUBLE: {
            os << v.value<double>();
            break;
        }

        case ValueType::JSON: {
            os << v.value<JSON>();
            break;
        }

        case ValueType::STRING: {
            os << "\"" << v.value<std::string>() << "\"";
            break;
        }

        case ValueType::LIST: {
            auto& p = v.value<List>();
            os << "[";
            for (auto& it : p) {
                if (p.back() == it) {
                    os << (*it);
                    continue;
                }
                os << (*it) << ",";
            }
            os << "]";
            break;
        }

        default:
            NOT_IMPLEMENTED;
        }

        return os;
    }

    std::ostream& operator<<(std::ostream& os, Value* value) {
        os << *value;
        return os;
    }

    std::ostream& operator<<(std::ostream& os, JSON& value) {  
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

    Value::~Value() {
        this->reset();
    }

    JSON::JSON(std::string filepath) {
        if (filepath != "") {
            this->load_from_file(filepath);
        }
    }

    bool JSON::load_from_file(std::string filepath){
        std::ifstream file;
        file.open(filepath);

        if (file.is_open()) {
            std::stringstream ss;
            while(!file.eof()){
                std::string str;
                std::getline(file, str);
                ss << str;
            }
            auto tokens = parser::Tokenize(ss.str());
            parser::Lexer(tokens, *this);
            return true;
        }

        std::cout << "FILE NOT FOUND! FILE PATH: " << filepath << "\r\n";
        return false;
    }

    bool JSON::load_from_string(std::string json_str) {
        auto tokens = parser::Tokenize(json_str);
        parser::Lexer(tokens, *this);
        return true;
    }

    Value& JSON::operator[](const std::string str){
        if (m_json.find(str) == m_json.end()){
            m_json[str] = JSON::JsonValue(new Value());
        }

        return *m_json[str];
    }

    JSON::~JSON() {
        // TODO: remove all data from map
    }

    namespace parser {
        struct SpecialTokens {
            TokenType operator[](char key) {
                switch (key) {
                    case '(':  return TokenType::L_PAREN;
                    case ')':  return TokenType::R_PAREN;
                    case '{':  return TokenType::LC_BRACKET;
                    case '}':  return TokenType::RC_BRACKET;
                    case '[':  return TokenType::L_BRACKET;
                    case ']':  return TokenType::R_BRACKET;
                    case ':':  return TokenType::COLON;
                    case ',':  return TokenType::COMMA;
                    case '.':  return TokenType::DOT;
                    case '\"': return TokenType::DOUBLE_QUOTE;
                    default:   return TokenType::NONE;
                }
            }

            bool is_special_char(char t) {
                return (*this)[t] != TokenType::NONE;
            }
        };

        std::vector<Token> Tokenize(std::string str) {
            auto is_skippable = [](std::string& token, std::vector<Token>& tokens, char t) -> bool {
                return (
                    isspace(t)
                    && tokens.size() > 0
                    && (
                        tokens.back().type != TokenType::DOUBLE_QUOTE
                        || tokens.back().type == TokenType::COLON
                    )
                    && !token.size()
                );
            };
            

            std::vector<Token> tokens;
            std::string token;

            SpecialTokens special_tokens;

            for (char t : str) {
                if (is_skippable(token, tokens, t)) {
                    continue;
                }

                if (special_tokens.is_special_char(t)) {
                    if (token.size() > 0) {
                        tokens.push_back(Token(token, TokenType::WORD));
                        tokens.push_back(Token({ t }, special_tokens[t]));
                    }
                    else {
                        tokens.push_back(Token({ t }, special_tokens[t]));
                    }

                    token = "";
                    continue;
                }

                token += t;
            }

            return tokens;
        }

        size_t Lexer(std::vector<Token>& tokens, JSON& out_json) {
            VectorView<Token> view = tokens;
            return Lexer(view, out_json);
        }

        size_t Lexer(VectorView<Token>& tokens, JSON& out_json) {
            size_t i = 0;

            std::cout << "LEXER: " << out_json << std::endl;

            auto check_token = [&](Token& token)-> bool {
                return 
                    token.type == TokenType::DOUBLE_QUOTE // check if this is not string structure
                    && (
                        i == 0 // if starting point, this is true
                        || (i > 0) // if this is value
                        && tokens[i - 1].type != TokenType::COLON // check not : because this is must be a value
                    );
            };

            auto parseInnerJson = [&](size_t inner_index) -> PtrJson {
                PtrJson parsed_json = new JSON();
                VectorView<Token> view(tokens, inner_index);

                // recurcive (((
                i += Lexer(view, *parsed_json);
                return parsed_json;
            };

            auto parseStr = [&]()-> std::string* {
                std::string temp_str;
                ++i; // consume first quote

                for (Token token = tokens[i]; token.type != TokenType::DOUBLE_QUOTE; token = tokens[++i]) {
                    temp_str += token.value;
                }

                std::string* str = new std::string(temp_str);
                ++i; // consume last quote
                return str;
            };

            auto parseValue = [&](Value& v) -> Value* {
                std::string possible_value;

                for (
                    Token token = tokens[i];
                    (
                        token.type != TokenType::COMMA
                        && token.type != TokenType::RC_BRACKET
                        && token.type != TokenType::R_BRACKET
                    );
                    token = tokens[++i]
                ) {
                    possible_value += token.value;
                }

                if (possible_value.find('.') != std::string::npos) {
                    double* value = new double();
                    *value = std::stod(possible_value);
                    v = ValuePair{ value, ValueType::DOUBLE };
                }
                else {
                    int* value = new int();
                    *value = std::stoi(possible_value);
                    v = ValuePair{ value, ValueType::INTEGER };
                }

                return &v;
            };

            while (i < tokens.size()) {
                Token token = tokens[i];

                if (token.type == TokenType::LC_BRACKET) {
                    i++;
                    continue;
                }

                if (token.type == TokenType::RC_BRACKET) {
                    i++;
                    break;
                }

                if (!check_token(token))
                {
                    i++;
                    continue;
                }
                
                i++;
                token = tokens[i];
                std::string key = token.value;

                i += 3; // this is +3 because token<key>, token<">, token<:> and than continues
                token = tokens[i];

                switch (token.type) {
                    case TokenType::LC_BRACKET: { // parse inner json
                        out_json[key] = ValuePair{ parseInnerJson(i), ValueType::JSON };
                        continue;
                    }

                    case TokenType::DOUBLE_QUOTE: {
                        out_json[key] = ValuePair{ parseStr(), ValueType::STRING };
                        token = tokens[i];
                        continue;
                    }

                    case TokenType::L_BRACKET: {
                        PtrList list = new List();
                        i++;
                        token = tokens[i];
                        std::string temp_str;

                        while (token.type != TokenType::R_BRACKET)
                        {
                            switch (token.type)
                            {
                                case TokenType::DOUBLE_QUOTE: {
                                    list->push_back(
                                        new Value(
                                            parseStr(),
                                            ValueType::STRING
                                        )
                                    );
                                    token = tokens[i];
                                    continue;
                                }

                                case TokenType::LC_BRACKET: { // parse inner json in list
                                    auto parsed_json = parseInnerJson(i);
                                    std::cout << *parsed_json << std::endl;

                                    list->push_back(
                                        new Value(
                                            parsed_json,
                                            ValueType::JSON
                                        )
                                    );
                                    token = tokens[i];
                                    continue;
                                }

                                default: {
                                    if (token.type != TokenType::COMMA) {
                                        Value* v = parseValue(*(new Value()));
                                        list->push_back(v);
                                    }
                                    break;
                                }
                            }

                            i++;
                            token = tokens[i];
                        }

                        out_json[key] = ValuePair{ list, ValueType::LIST };
                        i++;
                        continue;
                    }
                }

                // parse double or integer
                parseValue(out_json[key]);
            }

            return i;
        }
    }
}