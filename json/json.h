#ifndef __JSON__
#define __JSON__

#include <unordered_map>
#include <vector>
#include <memory>

namespace{
    std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
    std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
    std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
}
namespace json{

    enum class data_type {
        INTEGER,
        REAL,
        STRING,
        LIST,
        NONE,
        HASH
    };

    enum class token_type {
        LEFT_BRACKET, //  (
        RIGHT_BRACKET, // )
        LEFT_BRACKET_F, // {
        RIGHT_BRACKET_F, // }
        LEFT_BRACKET_S, // [
        RIGHT_BRACKET_S, // ]
        NUMBER, //  12354
        WORD, // dsklfjkdfsdf
        LIST, // [any, type, this]
        NONE, // null or NONE or Null etc.
        COMMA, // ,
        COLON, // :
        QUOTE, // "
        DOT, // .
    };

    struct Value {
    public:
        Value();
        Value(void * value, data_type type);
        const data_type& type();
        template<typename T>
        T& value(){
            return *(T*)(this->m_value);
        }
        void set(void *value, data_type type);
        template<typename T>
        operator T&(){
            return this->value<T>();
        }
        Value& operator=(const Value& other);
        Value& operator=(const Value* other);
        friend std::ostream& operator<<(std::ostream& os, Value& value);
        friend std::ostream& operator<<(std::ostream& os, Value* value);
        void clear();
        ~Value();
    private:
        void * m_value;
        data_type m_type;
    };
    
    struct Token{
    public:
        Token() { this->type = token_type::NONE; };
        Token(std::string value, token_type type);
        std::string value;
        token_type type;
    };

    class JSON{
    public:
        typedef std::unordered_map<std::string, Value*> std_dict;
        typedef JSON dict;
        
        JSON(std::string filepath="");
        bool load_from_file(std::string filepath);
        bool load_from_string(std::string json_str);
        
        Value& operator[](const std::string str);

        friend std::ostream& operator<<(std::ostream& os, JSON& value);

        ~JSON();
    private:
        typedef std::unordered_map<std::string, std::unique_ptr<Value>>::iterator it_json;
        std::vector<Token> Tokenize(std::string str);
        int Lexer(std::vector<Token> tokens, dict& j);

        std::unordered_map<std::string, std::unique_ptr<Value>> m_json;

    };
}

#endif // !__JSON__