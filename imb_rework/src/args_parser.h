#pragma once

#include <vector>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <string>
#include <map>

// TODO:
// proposed moding to add:
// SILENT -- no output
// NOHELP -- don't detect help option
// ALLOW_EXTRA -- don't regards extra args as an error
// NODUPLICATE -- don't allow duplicate options
// NODEFAULTSDUMP -- don't put defaults into dump

class args_parser {
    protected:
    int argc;
    char **argv;
    const char *option_starter;
    const char option_delimiter;
    const static int version;

    public:
    args_parser(int &_argc, char **&_argv, const char *opt_st = "--", char opt_delim = '=', bool silnt = false) : argc(_argc), argv(_argv), option_starter(opt_st), option_delimiter(opt_delim),
                                             prev_option_descr(NULL), silent(silnt) {}
    typedef enum { STRING, INT, FLOAT, BOOL } arg_t;

    class value {
        public:
            value() : initialized(false) {}
            value(float v) : initialized(true), i(0), str("(none)"), b(false) { type = FLOAT; f = v; }
            value(int v) : initialized(true), f(0), str("(none)"), b(false) { type = INT; i = v; }
            value(bool v) : initialized(true), i(0), f(0), str("(none)") { type = BOOL; b = v; }
            value(std::string v) : initialized(true), i(0), f(0), b(false) { type = STRING; str = v; }
            value(const char *v) : initialized(true), i(0), f(0), b(false) { type = STRING; str.assign(v); }
        public:
            bool initialized;
            int i;
            float f;
            std::string str;
            bool b;
            arg_t type;
        public:
            bool is_initialized() const { return initialized; };
            value &operator=(const value &other);
            bool parse(const char *sval, arg_t _type);
            friend std::ostream &operator<<(std::ostream &s, const args_parser::value &val);
            void sanity_check(arg_t _type) const;
            static const std::string get_type_str(arg_t _type); 
    };
    struct descr {
        enum { MAX_VEC_SIZE = 1024 };
        std::string str;
        arg_t type;
        bool required;
        bool defaulted;
        char vec_delimiter;
        int vec_min;
        int vec_max;
        args_parser::value def;
        std::vector<args_parser::value> val;
        std::string caption;
        std::string vec_def;
        descr() {};
        descr(std::string _str, arg_t _type, char _vec_delimiter, int _vec_min, int _vec_max, const std::string &_vec_def)  :
            str(_str), type(_type), required(false), defaulted(false), vec_delimiter(_vec_delimiter), vec_min(_vec_min), vec_max(_vec_max), vec_def(_vec_def)
        { }
        //{ val.resize(vec_max == 0 ? 1 : vec_max); }
        descr(std::string _str, arg_t _type, char _vec_delimiter, int _vec_min, int _vec_max)  :
            str(_str), type(_type), required(true), defaulted(false), vec_delimiter(_vec_delimiter), vec_min(_vec_min), vec_max(_vec_max)
        { }
        //{ val.resize(vec_max == 0 ? 1 : vec_max); }
        descr(std::string _str, arg_t _type) :
            str(_str), type(_type), required(true), defaulted(false), vec_delimiter(0), vec_min(0), vec_max(0)
        { val.resize(1); }
        descr(std::string _str, arg_t _type, value _def) :
            str(_str), type(_type), required(false), defaulted(false), vec_delimiter(0), vec_min(0), vec_max(0), def(_def)
        { val.resize(1); def.sanity_check(type); }
        void print() const { std::cout << str << ": " << val[0] << std::endl; }
    };
    protected:
    std::string current_group;
    std::map<std::string, std::vector<descr> > expected_args;
    std::vector<std::string> unknown_args;
    descr *prev_option_descr;
    bool silent;
   
    bool match(std::string &arg, std::string pattern) const;
    bool match(std::string &arg, descr &exp) const;
    bool get_value(std::string &arg, descr &exp);
    void get_default_value(descr &d);
    
    const std::vector<args_parser::descr> &get_extra_args_info(int &num_extra_args, int &num_required_extra_args) const;
    std::vector<args_parser::descr> &get_extra_args_info(int &num_extra_args, int &num_required_extra_args);

    void print_err_required_arg(descr &arg) const;
    void print_err_required_extra_arg() const;
    void print_err_parse(descr &arg) const; 
    void print_err_parse_extra_args() const;
    void print_err_extra_args() const;
    void print_single_option_usage(const descr &d, size_t header_size, bool is_first, bool no_option_name = false) const;

    public:
    void print_help_advice() const;
    void print_help() const;
    void print() const;
    bool parse();
    args_parser &add_required_option(const char *s, arg_t type);
    args_parser &add_option_with_defaults(const char *s, arg_t type, value v);
    args_parser &add_required_option_vec(const char *s, arg_t type, char delim, int min = 1, int max = descr::MAX_VEC_SIZE);
    args_parser &add_option_with_defaults_vec(const char *s, arg_t type, const std::string &defaults, char delim, int min = 1, int max = descr::MAX_VEC_SIZE);

    args_parser &set_current_group(const std::string &g) { current_group = g; return *this; }
    args_parser &set_default_current_group() { current_group = ""; return *this; }

    args_parser &set_caption(const char *s, const char *cap);
    args_parser &set_caption(int n, const char *cap);

    const std::vector<value> &get_result(const std::string &s);

    int get_result_int(const std::string &s);
    float get_result_float(const std::string &s);
    bool get_result_bool(const std::string &s);
    std::string get_result_string(const std::string &s);

    void get_result_vec_int(const std::string &s, std::vector<int> &r);
    void get_result_vec_float(const std::string &s, std::vector<float> &r);
    void get_result_vec_bool(const std::string &s, std::vector<bool> &r);
    void get_result_vec_string(const std::string &s, std::vector<std::string> &r);

    void get_unknown_args(std::vector<std::string> &r);

    bool parse_special_int(std::string &s, int &r);
    bool parse_special_float(std::string &s, float &r);
    bool parse_special_bool(std::string &s, bool &r);
    bool parse_special_string(std::string &s, std::string &r);

    bool parse_special_vec_int(std::string &s, std::vector<int> &r, char delim, int min, int max);
    bool parse_special_vec_float(std::string &s, std::vector<float> &r, char delim, int min, int max);
    bool parse_special_vec_bool(std::string &s, std::vector<bool> &r, char delim, int min, int max);
    bool parse_special_vec_string(std::string &s, std::vector<std::string> &r, char delim, int min, int max);

    void clean_args() { argc = 0; }
    std::string dump() const;
    bool load(const std::string &input);
    bool load(std::istream &in);

    protected:
    // NOTE: see source for usage comments
    enum foreach_t { FOREACH_FIRST, FOREACH_NEXT };
    bool in_expected_args(enum foreach_t t, const std::string *&group, descr *&arg);    
    bool in_expected_args(enum foreach_t t, const std::string *&group, const descr *&arg) const;    
};

