#ifndef __JSON__
#define __JSON__

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace {
    std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
    std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
    std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
}

namespace json {
    template<typename V>
    struct VectorView {
    public:
        VectorView(std::vector<V>& in_vec, size_t start = 0, size_t end = -1) :
            m_view(in_vec), m_start(start)
        {
            if (end == -1)
            {
                end = in_vec.size();
            }

            this->m_end = end;
        }

        VectorView(VectorView& other, size_t start = 0, size_t end = -1) :
            m_view(other.m_view), m_start(other.m_start + start)
        {
            if (end == -1)
            {
                end = other.size();
            }

            this->m_end = other.m_start + end;
        }

        V back() const {
            return *(this->cend() - 1);
        }

        V operator[](size_t i) {
            return this->m_view[this->m_start + i];
        }

        std::vector<V>& view() const {
            return this->m_view;
        }

        size_t size() const {
            return this->m_end - this->m_start;
        }

        typename std::vector<V>::iterator begin() const {
            return this->m_view.begin() + this->m_start;
        }

        typename std::vector<V>::iterator end() const {
            return this->m_view.end() + this->m_end;
        }

        typename std::vector<V>::const_iterator cbegin() const {
            return this->m_view.cbegin() + this->m_start;
        }

        typename std::vector<V>::const_iterator cend() const {
            return this->m_view.cend() + this->m_end;
        }

    private:
        std::vector<V>& m_view;
        size_t m_start;
        size_t m_end;
    };
}

namespace json {
    enum class ValueType {
        NONE,
        INTEGER,
        DOUBLE,
        STRING,
        LIST,
        JSON
    };

    using ValuePair = std::pair<void*, ValueType>;

    struct Value {
    public:
        Value(): m_value(nullptr), m_type(ValueType::NONE) {};

        Value(void* value, ValueType type);
        Value(ValuePair& pair);

        Value(Value& other);
        Value(Value&& other) noexcept;

        ~Value();

        Value& operator=(const Value& other);
        Value& operator=(const Value* other);
        Value& operator=(const ValuePair& other);

        friend std::ostream& operator<<(std::ostream& os, Value& value);
        friend std::ostream& operator<<(std::ostream& os, Value* value);
        
        template<typename T>
        T& value(){
            return *(T*)(this->m_value);
        }

        template<typename T>
        operator T&(){
            return this->value<T>();
        }

        const ValueType& type() const;

    private:
        void reset();
        void set(void* value, ValueType type);

    private:
        void* m_value;
        ValueType m_type;
    };

    using List = std::vector<Value*>;
    using PtrList = List*;

    class JSON {
    public:
        using JsonKey = std::string;
        using JsonValue = std::unique_ptr<Value>;
        using JsonStore = std::unordered_map<JsonKey, JsonValue>;

        JSON(std::string filepath="");
        bool load_from_file(std::string filepath);
        bool load_from_string(std::string json_str);
        
        Value& operator[](const std::string str);

        friend std::ostream& operator<<(std::ostream& os, JSON& value);

        JsonStore::iterator begin() {
            return this->m_json.begin();
        }

        JsonStore::iterator end() {
            return this->m_json.end();
        }

        JsonStore::const_iterator cbegin() {
            return this->m_json.cbegin();
        }

        JsonStore::const_iterator cend() {
            return this->m_json.cend();
        }

        ~JSON();
    private:
        JsonStore m_json;
    };

    using PtrJson = JSON*;

    namespace parser {
        enum class TokenType {
            L_PAREN, //  (
            R_PAREN, // )
            LC_BRACKET, // {
            RC_BRACKET, // }
            L_BRACKET, // [
            R_BRACKET, // ]
            COMMA, // ,
            COLON, // :
            DOUBLE_QUOTE, // "
            DOT, // .
            NUMBER, //  12354
            WORD, // dsklfjkdfsdf
            LIST, // [any, type, this]
            NONE, // null or NONE or Null etc.
        };

        struct Token {
        public:
            Token() = default;

            Token(std::string value, TokenType type) {
                this->value = value;
                this->type = type;
            }

            std::string value;
            TokenType type = TokenType::NONE;
        };

        std::vector<Token> Tokenize(std::string str);
        size_t Lexer(std::vector<Token>& tokens, JSON& out_json);
        size_t Lexer(VectorView<Token>& tokens, JSON& out_json);
    }
}

#endif // !__JSON__